
/* todo.cc:
 *
 * Copyright (c) 1997, 1998, Scott Grosch
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

#include "pi-source.h"
#include <string.h>
#include "pi-todo.h"

todoAppInfo_t::todoAppInfo_t(void *ap) 
     : appInfo_t(ap)
{
     unsigned char *ptr = ((unsigned char *) ap) + BASE_APP_INFO_SIZE;
     
     _dirty = get_short(ptr);
     
     ptr += 2;
     _sortByPriority = get_byte(ptr);
}

void *todoAppInfo_t::pack(void) 
{
     unsigned char *buffer = new unsigned char [TODO_APP_INFO_SIZE];

     baseAppInfoPack(buffer);

     unsigned char *ptr = buffer + BASE_APP_INFO_SIZE;

     set_short(ptr, _dirty);

     ptr += 2;
     set_short(ptr, _sortByPriority);

     return buffer;
}

todo_t::todo_t(const todo_t &oldCopy) 
{
     (void) memcpy(this, &oldCopy, sizeof(todo_t));

     int len;

     if (oldCopy._due) {
	  _due = new tm;
	  (void) memcpy(_due, oldCopy._due, sizeof(tm));
     }

     if (oldCopy._description) {
	  len = strlen(oldCopy._description);
	  _description = new char [len + 1];
	  (void) strcpy(_description, oldCopy._description);
     }

     if (oldCopy._note) {
	  len = strlen(oldCopy._note);
	  _note = new char [len + 1];
	  (void) strcpy(_note, oldCopy._note);
     }
}

void todo_t::unpack(void *buf) 
{
	  if (_due)
	       delete _due;
	  if (_description)
	       delete _description;
	  if (_note)
	       delete _note;
	  
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

     unsigned char *ptr = ((unsigned char *)buf) + 2;
     _priority = get_byte(ptr);

     ptr++;
     if (_priority & 0x80) {
	  _complete = 1;
	  _priority &= 0x7f;
     } else
	  _complete = 0;

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

void *todo_t::internalPack(unsigned char *buffer) 
{
     if (_due)
	  setBufTm(buffer, _due);
     else
	  *buffer = *(buffer + 1) = 0xff;
     
     unsigned char *ptr = buffer + 2;
     
     *ptr = (unsigned char) _priority;
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
     
     unsigned char *ret = new unsigned char [*len];

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

     return internalPack((unsigned char *) buffer);
}

// We can't just point to the data, as it might be deleted.  Make a copy
void todoList_t::merge(todo_t &todo) 
{
     todo._next = _head;
     _head = new todo_t(todo);
}
 
// We can't just point to the data in the list, as it might get deleted on
// us. We need to make a real copy
void todoList_t::merge(todoList_t &list) 
{
     todo_t *newguy;
 
     for (todo_t *ptr = list._head; ptr != NULL; ptr = ptr->_next) {
          newguy = new todo_t(ptr);
          newguy->_next = _head;
          _head = newguy;
     }
}
 
todoList_t::~todoList_t(void) {
     todo_t *ptr, *next;
 
     for (ptr = _head; ptr != NULL; ptr = next) {
          next = ptr->_next;
          delete ptr;
     }
}
