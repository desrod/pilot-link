#ifndef _IAMBICEXPENSE_HXX	/* -*- C++ -*- */
#define _IAMBICEXPENSE_HXX

#include "pi-appinfo.hxx"

#define reimburse 0x01
#define receipt 0x02

const int IAMBIC_EXPENSE_APP_INFO_SIZE = 512;

class iambicExpenseAppInfo_t : public appInfo_t 
{
     category_t _conversionNames;

   public:
     iambicExpenseAppInfo_t(void *);
     
     const category_t &conversionNames(void) const { return _conversionNames; }

     // You can't install an expense entry yet, but we have to provide this
     // function or the compiler will choke, as the parent defines this as
     // a pure virtual function.
     void *pack(void) { return NULL; }
};

class iambicExpenseList_t;		// Forward declaration

class iambicExpense_t : public baseApp_t
{
     friend iambicExpenseList_t;
     
     short _flags;
     char *_type;
     char *_paidby;
     char *_payee;
     char *_note;
     double _amount;
     double _milesStart, _milesEnd;
     double _exchangeRate;
     tm _date;

     iambicExpense_t *_next;

     // Will never get called, but we need the name
     void *internalPack(unsigned char *a) { return NULL; }
     
   public:
     iambicExpense_t(void) : baseApp_t() {
	  (void) memset(this, '\0', sizeof(iambicExpense_t));
     }
     iambicExpense_t(void *buf) : baseApp_t() { unpack(buf); }
     iambicExpense_t(void *buf, int attr, recordid_t id, int category)
	  : baseApp_t(attr, id, category)
     {
	  unpack(buf);
     }
     iambicExpense_t(const iambicExpense_t &);
     
     ~iambicExpense_t();

     const char *type(void) const { return _type; }
     const char *paidBy(void) const { return _paidby; }
     const char *paidby(void) const { return _paidby; }
     const char *payee(void) const { return _payee; }
     const char *note(void) const { return _note; }
     double amount(void) const { return _amount; }
     double milesStart(void) const { return _milesStart; }
     double milesEnd(void) const { return _milesEnd; }
     double exchangeRate(void) const { return _exchangeRate; }
     const tm *date(void) const { return &_date; }

     void unpack(void *);
     
     // We don't let you pack one of these, but we must provide the name
     void *pack(int *a) { return NULL; }
     void *pack(void *a, int *b) { return NULL; }
};

class iambicExpenseList_t 
{
     iambicExpense_t *_head;
     
   public:
     iambicExpenseList_t(void) : _head(NULL) {}
     ~iambicExpenseList_t();
     
     iambicExpense_t *first() { return _head; }
     iambicExpense_t *next(iambicExpense_t *ptr) { return ptr->_next; }

     void merge(iambicExpense_t &);
     void merge(iambicExpenseList_t &);
};

#endif // _IAMBICEXPENSE_HXX
