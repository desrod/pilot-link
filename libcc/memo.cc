
/* memo.cc:
 *
 * Copyright (c) 1997, 1998, Scott Grosch
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

#include <string.h>

#include "pi-source.h"
#include "pi-memo.h"

void memo_t::unpack(void *text) 
{
     if (_text)
	  delete _text;
     
     _size = strlen((const char *) text) + 1;
     _text = new char [_size];
     
     (void) strcpy(_text, (const char *) text);
}

// The indirection just to make the base class happy
void *memo_t::internalPack(unsigned char *buf) 
{
     return strcpy((char *) buf, _text);
}

void *memo_t::pack(int *len) 
{
     *len = _size;
     unsigned char *ret = new unsigned char [_size];
     return internalPack(ret);
}

void *memo_t::pack(void *buf, int *len) 
{
     if (*len < _size)
	  return NULL;

     *len = _size;

     return internalPack((unsigned char *) buf);
}

memo_t::memo_t(const memo_t &oldCopy) 
{
     int len = strlen(oldCopy._text);
     _text = new char [len + 1];
     (void) strcpy(_text, oldCopy._text);

     _size = oldCopy._size;
     _next = oldCopy._next;
}

// We can't just point to the data, as it might be deleted.  Make a copy
void memoList_t::merge(memo_t &memo) 
{
     memo._next = _head;
     _head = new memo_t(memo);
}

// We can't just point to the data in the list, as it might get deleted on
// us. We need to make a real copy
void memoList_t::merge(memoList_t &list) 
{
     memo_t *newguy;

     for (memo_t *ptr = list._head; ptr != NULL; ptr = ptr->_next) {
	  newguy = new memo_t(ptr);
	  newguy->_next = _head;
	  _head = newguy;
     }
}

memoList_t::~memoList_t(void) {
     memo_t *ptr, *next;

     for (ptr = _head; ptr != NULL; ptr = next) {
	  next = ptr->_next;
	  delete ptr;
     }
}
