#include <string.h>
#include "pi-todo.h"

todoAppInfo_t::todoAppInfo_t(void *ap) 
     : appInfo_t(ap)
{
     uchar_t *ptr = ((uchar_t *) ap) + BASE_APP_INFO_SIZE;
     
     _dirty = get_short(ptr);
     
     ptr += 2;
     _sortByPriority = get_byte(ptr);
}

void *todoAppInfo_t::pack(void) 
{
     uchar_t *buffer = new uchar_t [TODO_APP_INFO_SIZE];

     baseAppInfoPack(buffer);

     uchar_t *ptr = buffer + BASE_APP_INFO_SIZE;

     set_short(ptr, _dirty);

     ptr += 2;
     set_short(ptr, _sortByPriority);

     return buffer;
}

void todo_t::unpack(void *buf, bool firstTime) 
{
     // If we unpack more than once, we need to free up any old data first
     // so that we don't leak memory
     if (firstTime == false) {
	  if (_due)
	       delete _due;
	  if (_description)
	       delete _description;
	  if (_note)
	       delete _note;
     }
	  
     unsigned short d = get_short(buf);
     if (d != 0xffff) {
	  _due = new tm;

	  _due->tm_year = (d >> 9) + 4;
	  _due->tm_mon = ((d >> 5) & 15) - 1;
	  _due->tm_mday = d & 31;
	  _due->tm_hour = 0;
	  _due->tm_min = 0;
	  _due->tm_sec = 0;
	  _due->tm_isdst = -1;

	  (void) mktime(_due);
     } else
	  _due = NULL;

     uchar_t *ptr = ((uchar_t *)buf) + 2;
     _priority = get_byte(ptr);

     ptr++;
     if (_priority & 0x80) {
	  _complete = true;
	  _priority &= 0x7f;
     } else
	  _complete = false;

     int len = strlen((const char *) ptr);
     if (len) {
	  _description = new char [len + 1];
	  (void) strcpy(_description, (const char *) ptr);
     } else
	  _description = NULL;
     
     ptr += len + 1;
     if (*ptr != '\0') {
	  _note = new char [strlen((const char *) ptr) + 1];
	  (void) strcpy(_note, (const char *) ptr);
     } else
	  _note = NULL;
}

void *todo_t::internalPack(uchar_t *buffer) 
{
     if (_due)
	  setBufTm(buffer, _due);
     else
	  *buffer = *(buffer + 1) = 0xff;
     
     uchar_t *ptr = buffer + 2;
     
     *ptr = (uchar_t) _priority;
     if (_complete)
	  *ptr |= 0x80;

     ptr++;
     if (_description) {
	  (void) strcpy((char *) ptr, _description);
	  ptr += strlen(_description) + 1;
     } else
	  *ptr++ = '\0';

     if (_note)
	  (void) strcpy((char *) ptr, _note);
     else
	  *ptr = '\0';

     return buffer;
}

// Returns a pointer to the packed buffer, and set len to be the length of that
// "string"
void *todo_t::pack(int *len) 
{
     // 2 for the time, 1 for the priority
     *len = 3;
     if (_note)
	  *len += strlen(_note);

     // There is a null byte, whether or not there is a note
     ++(*len);
     
     if (_description)
	  *len += strlen(_description);

     // There is a null byte, whether or not there is a description
     ++(*len);
     
     uchar_t *ret = new uchar_t [*len];

     return internalPack(ret);
}

// If the length passed in is not large enough to hold the packed data, a NULL
// is returned.  Else, the buffer passed in is len bytes long.  Fill it in,
// and then reset the length to the length of the buffer returned
void *todo_t::pack(void *buffer, int *len) 
{
     // 2 for the time, 1 for the priority
     int totalLength = 3;

     if (_note)
	  totalLength += strlen(_note);

     // There is a null byte, whether or not there is a note
     totalLength++;
     
     if (_description)
	  totalLength += strlen(_description);
     
     // There is a null byte, whether or not there is a description
     totalLength++;
     
     if (*len < totalLength)
	  return NULL;

     *len = totalLength;

     return internalPack((uchar_t *) buffer);
}
