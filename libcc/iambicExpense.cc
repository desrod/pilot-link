#include <ctype.h>		// isspace
#include <stdio.h>		// sprintf
#include <stdlib.h>		// strtoul
#include <string.h>		// strcpy
#include <math.h>		// floor
#include "pi-source.h"
#include "pi-iambicExpense.h"

/*
 * I left these field names the same as Iambic used, even though the three
 * I put a comment next to seem strangely named.
 */
 
#define hasNote 0x01
#define hasActivity 0x02        /* The type field */
#define hasProject 0x04         /* The payee field */
#define hasClient 0x08          /* The paid by field */
#define hasExchangeRate 0x10
#define hasOdometer 0x20
#define hasAmount 0x40
#define whenInfoChanged 0x80
#define hasOptions 0x100


iambicExpenseAppInfo_t::iambicExpenseAppInfo_t(void *ai) 
     : appInfo_t(ai) 
{
     unsigned char *ptr = ((unsigned char *) ai) + BASE_APP_INFO_SIZE;

     (void) memcpy(_conversionNames, ptr, 256);
}

iambicExpenseList_t::~iambicExpenseList_t() 
{
     iambicExpense_t *next;
     
     for (iambicExpense_t *head = _head; head != NULL; head = next) {
          next = head->_next;
          delete head;
     }
}

// We can't just point to the data, as it might be deleted.  Make a copy
void iambicExpenseList_t::merge(iambicExpense_t &iambicExpense) 
{
     iambicExpense._next = _head;
     _head = new iambicExpense_t(iambicExpense);
}
 
// We can't just point to the data in the list, as it might get deleted on
// us. We need to make a real copy
void iambicExpenseList_t::merge(iambicExpenseList_t &list) 
{
     iambicExpense_t *newguy;
 
     for (iambicExpense_t *ptr = list._head; ptr != NULL; ptr = ptr->_next) {
          newguy = new iambicExpense_t(ptr);
          newguy->_next = _head;
          _head = newguy;
     }
}

/*
 * Palm used a really annoying method of encryption armor for their floating
 * point amounts.  It took forever, but with huge amounts of help from 
 * Kenneth Albanowski we were able to decode it.
 *
 * Note the double pointer to an unsigned char here.  I did that so that this
 * function can increment the pointer that is passed in so that the caller
 * doesn't have to remember to do so every time.
 */
static double getDouble(unsigned char **buf, int isExchangeRate)
{
     unsigned char *ptr = *buf;
     
     char hexbuf[9];
     (void) sprintf(hexbuf, "%lx", get_long(ptr));
     unsigned long div = strtoul(hexbuf, (char **)NULL, 16);
     
     int x = get_byte(ptr + 5) - 0xda;
     
     double min = 0.78125 * (1 << x);
     double max = 0.78125 * (1 << (x + 1));

     double amount = (div - 0x80000000) / (0x80000000 * 1.0);
     amount *= max - min;
     amount += min;

     /* If it's an exchange rate save precision to 3 decimal places, not 2 */
     if (isExchangeRate)
	  amount = floor(amount * 10.0) / 1000.0;
     else
	  amount = floor(amount) / 100.0;
     
     /*
      * Move over the value, which is 6 bytes.  There seems to be two
      * bytes following every value.  They are always 0x01 0x00 but I
      * have no clue what they are for.
      */
     
     *buf += 8;
     
     return amount;
}

/* Note the double pointer, just like in getDouble above */
static char *getString(unsigned char **buf) 
{
     /* This had better not ever happen! */
     if (**buf == '\0')
	  return NULL;

     unsigned char *ptr = *buf;
     
     while (isspace(*ptr))
	  ptr++;

     unsigned char *end;
     for (end = ptr; *end != '\0'; end++)
	  ;

     /*
      * We need to chop off any trailing whitespace, but at the same time
      * we need to remember where the end of the field was so that we can
      * move over it accordingly.
      */
     unsigned char *realEnd = end;
     
     end--;
     while (isspace(*end))
	  end--;
     *(++end) = '\0';

     /*
      * end - ptr will move us to the null byte.  Add one more to go past
      * it so we can grab the next string, if it exists.
      */
     *buf += realEnd - ptr + 1;

     char *ret = new char [strlen((const char *) ptr) + 1];
     return strcpy(ret, (const char *) ptr);
}

/*
 * Format of an expense record:
 * ba 4b 80 00 00 00 ff e1 01 00 00 0b 00 10 c7 4b
 *
 * 2     Date
 * 6     Amount of money
 * 2     Unknown
 * 1     allBits field of an ExpenseDBRecordOptions - Seems unrelavent
 * 1     Flags for reimbursable and receipt available
 * 1     allBits field of an ExpenseDBRecordFlags - Seems unrelavent
 * 1     Flags for record info (hasNote, hasActivity, etc...)
 *
 * This is then followed by the data.  If fields appear, they are separated
 * by two bytes (0x01 0x00), and seem to appear in this order:
 *
 *   exchange rate
 *   odometer begin 0x01 0x00 odometer end
 *   paid by
 *   payee
 *   type
 *   note
 *
 * If odometer is set in the flags, then the amount of money is the number of
 * miles traveled.
 */

void iambicExpense_t::unpack(void *buf, int firstTimeThrough) 
{
     if (firstTimeThrough == 0) {
	  if (_paidby)
	       delete _paidby;
	  if (_payee)
	       delete _payee;
	  if (_type)
	       delete _type;
	  if (_note)
	       delete _note;
     }
     
     unsigned char *ptr = (unsigned char *) buf;
     
     unsigned short d = (unsigned short) get_short(ptr);

     /*
      * This is what the pilot stores for the date:
      *
      * typedef struct {
      *   Word year  :7;
      *   Word month :4; 
      *   Word day   :5;
      * } DateType;
      *
      *
      * We want to pop the last 9 bits (the month and day) off
      * of the struct, and move the year bits down to the lower
      * bits.  That's the shift.
      *
      * The 4 is because of OS differences.  The pilot saves the
      * date in macintosh time format, since that's what the
      * dragonball chip uses.  Macs reference the year as of 1904.
      * UNIX, on the other hand, references the year as of 1900.  So
      * we have to add 4 to the number of years to make UNIX happy.
      */
     
     _date.tm_year = (d >> 9) + 4;
	  
     /*
      * Drop off the 5 bites of the day.  Then & with the last 4
      * bits so we only get the month part.  Then subtract 1 since
      * a tm_mon is referenced from 0 on UNIX
      */
     _date.tm_mon = ((d >> 5) & 0xF) - 1;
     
     /* Just take the last 5 bits for a day */
     _date.tm_mday = d & 0x1F;
     
     /* We don't know if daylight savings time is in effect */
     _date.tm_isdst = -1;
     
     mktime(&_date);
     
     ptr += 2;
     _amount = getDouble(&ptr, 0);
     
     // Used to be:  options = *++ptr; ptr += 2;
     ptr += 3;
     
     _flags = *ptr++;
     
     if (_flags & hasExchangeRate)
	  _exchangeRate = getDouble(&ptr, 1);
     else
	  _exchangeRate = 1.0;
     
     if (_flags & hasOdometer) {
	  _milesStart = getDouble(&ptr, 0);
	  _milesEnd = getDouble(&ptr, 0);
     }
     
     if (_flags & hasClient)
	  _paidby = getString(&ptr);
     else
	  _paidby = NULL;
          
     if (_flags & hasProject)
	  _payee = getString(&ptr);
     else
	  _payee = NULL;
     
     if (_flags & hasActivity)
	  _type = getString(&ptr);
     else
	  _type = NULL;
     
     if (_flags & hasNote)
	  _note = getString(&ptr);
     else
	  _note = NULL;
}

iambicExpense_t::iambicExpense_t(const iambicExpense_t &oldCopy)
     : baseApp_t()
{
     (void) memcpy(this, &oldCopy, sizeof(iambicExpense_t));

     if (_paidby) {
	  _paidby = new char[strlen(oldCopy._paidby) + 1];
	  (void) strcpy(_paidby, oldCopy._paidby);
     }
     if (_payee) {
	  _payee = new char[strlen(oldCopy._payee) + 1];
	  (void) strcpy(_payee, oldCopy._payee);
     }
     if (_type) {
	  _type = new char[strlen(oldCopy._type) + 1];
	  (void) strcpy(_type, oldCopy._type);
     }
     if (_note) {
	  _note = new char[strlen(oldCopy._note) + 1];
	  (void) strcpy(_note, oldCopy._note);
     }
}

iambicExpense_t::~iambicExpense_t() 
{
     if (_paidby)
	  delete _paidby;
     if (_payee)
	  delete _payee;
     if (_type)
	  delete _type;
     if (_note)
	  delete _note;
}

