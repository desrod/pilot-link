/* address.cc:
 *
 * Copyright (c) 1997, 1998, Scott Grosch
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

#include <stdlib.h>
#include <string.h>

#include "pi-source.h"
#include "pi-address.h"

static inline int hi(const unsigned int x) { return (x >> 4) & 0x0f; }
static inline int lo(const unsigned int x) { return x & 0x0f; }
static inline int pair(const unsigned int x, const unsigned int y) 
{
     return (x << 4) | y;
}

addressAppInfo_t::addressAppInfo_t(void *ai) 
     : appInfo_t(ai) 
{
     unsigned char *ptr = ((unsigned char *) ai) + BASE_APP_INFO_SIZE;
 
     _dirtyFieldLabels = get_long(ptr);

     ptr += 4;
     (void) memcpy(_labels, ptr, 352);

     ptr += 352;
     _country = get_short(ptr);

     ptr += 2;
     _sortByCompany = get_byte(ptr);

     int i;
     for (i = 3; i < 8; i++)
	  (void) strcpy(_phoneLabels[i-3], _labels[i]);

     for (i = 19; i < 22; i++)
	  (void) strcpy(_phoneLabels[i-14], _labels[i]);
}

void *addressAppInfo_t::pack(void) 
{
     unsigned char *buffer = new unsigned char [ADDRESS_APP_INFO_SIZE];
     
     baseAppInfoPack(buffer);
     
     unsigned char *ptr = buffer + BASE_APP_INFO_SIZE;
     
     set_long(ptr, _dirtyFieldLabels);

     ptr += 4;
     memcpy(ptr, _labels, 352);

     ptr += 352;
     set_short(ptr, _country);

     ptr += 2;
     set_byte(ptr, _sortByCompany);

     return buffer;
}

const addressLabels_t *addressAppInfo_t::labels(void)
{
	return &_labels;
}

const addressPhoneLabels_t *addressAppInfo_t::phoneLabels(void)
{
	return &_phoneLabels;
}

int addressAppInfo_t::country(void)
{
	return _country;
}

int addressAppInfo_t::sortByCompany(void)
{
	return _sortByCompany;
}

address_t::address_t(void *buf)
{
	unpack(buf); 
}

address_t::address_t(void)
{
	memset(this, '\0', sizeof(address_t));
}

	
address_t::address_t(void *buf, int attr, recordid_t id_, int category)
	: baseApp_t(attr, id_, category)
{
	unpack(buf);
}

address_t::address_t(const address_t &oldCopy) 
{
     (void) memcpy(this, &oldCopy, sizeof(address_t));

     int len;
     
     for (short int i = 0; i < 19; i++)
	  if (oldCopy._entry[i]) {
	       len = strlen(oldCopy._entry[i]);
	       _entry[i] = new char [len + 1];
	       (void) strcpy(_entry[i], oldCopy._entry[i]);
	  }
}

char *address_t::entry(labelTypes_t idx)
{ 
	return _entry[idx]; 
}

int address_t::whichPhone(void) 
{ 
	return _whichPhone;
}

int address_t::phoneLabel(int idx)
{ 
	return _phoneLabels[idx];
}

void address_t::unpack(void *buf) 
{
     int i;
     
	  for (i = 0; i < 19; i++)
	       if (_entry[i])
		    delete _entry[i];

     unsigned char *ptr = ((unsigned char *) buf) + 1;

     _whichPhone = hi(get_byte(ptr));
     _phoneLabels[4] = lo(get_byte(ptr));
     _phoneLabels[3] = hi(get_byte(++ptr));
     _phoneLabels[2] = lo(get_byte(ptr));
     _phoneLabels[1] = hi(get_byte(++ptr));
     _phoneLabels[0] = lo(get_byte(ptr));

     unsigned long contents = get_long(++ptr);

     ptr += 5;

     int len;
     for (i = 0; i < 19; i++) {
	  if (contents & (1 << i)) {
	       len = strlen((char *) ptr) + 1;
	       _entry[i] = new char [len];
	       (void) strcpy(_entry[i], (char *) ptr);
	       ptr += len;
	  } else
	       _entry[i] = NULL;
     }
}

address_t::~address_t(void) 
{
     for (int i = 0; i < 19; i++)
	  if (_entry[i])
	       delete _entry[i];
}

void *address_t::internalPack(unsigned char *buf) 
{
     unsigned char offset = 0;
     int len;
     recordid_t contents = 0;

     unsigned char *ptr = buf + 9;
     
     for (short int i = 0; i < 19; i++) {
	  if (_entry[i]) {
	       len = strlen(_entry[i]) + 1;
	       contents |= (1 << i);
	       memcpy(ptr, _entry[i], len);
	       ptr += len;
	  } else
	       len = 0;
	  
	  if (i < address_t::company)
	       offset += len;
     }

     if (_entry[address_t::company])
	  ++offset;
     else
	  offset = 0;
     
     recordid_t phoneFlag = _phoneLabels[0];
     phoneFlag |= _phoneLabels[1] << 4;
     phoneFlag |= _phoneLabels[2] << 8;
     phoneFlag |= _phoneLabels[3] << 12;
     phoneFlag |= _phoneLabels[4] << 16;
     phoneFlag |= _whichPhone << 20;

     set_long(ptr = buf, phoneFlag);

     ptr += 4;
     set_long(ptr, contents);

     ptr += 4;
     set_byte(ptr, offset);

     return buf;
}

void *address_t::pack(int *len) 
{
     *len = 9;

     for (short int i = 0; i < 19; i++)
	  if (_entry[i] && _entry[i][0] != '\0')
	       *len += strlen(_entry[i]) + 1;
     
     unsigned char *ret = new unsigned char [*len];
     return internalPack(ret);
}
 
void *address_t::pack(void *buf, int *len) 
{
     int totalLength = 9;
 
     for (short int i = 0; i < 19; i++)
	  if (_entry[i] && _entry[i][0] != '\0')
	       totalLength += strlen(_entry[i]) + 1;

     if (*len < totalLength)
          return NULL;
 
     *len = totalLength;
 
     return internalPack((unsigned char *) buf);
}


// We can't just point to the data, as it might be deleted.  Make a copy
address_t *addressList_t::first()
{
	return _head; 
}

address_t *addressList_t::next(address_t *ptr)
{
	return ptr->_next;
}

void addressList_t::merge(address_t &address) 
{
     address._next = _head;
     _head = new address_t(address);
}
 
// We can't just point to the data in the list, as it might get deleted on
// us. We need to make a real copy
void addressList_t::merge(addressList_t &list) 
{
     address_t *newguy;
 
     for (address_t *ptr = list._head; ptr != NULL; ptr = ptr->_next) {
          newguy = new address_t(ptr);
          newguy->_next = _head;
          _head = newguy;
     }
}
 
addressList_t::~addressList_t(void) {
     address_t *ptr, *next;
 
     for (ptr = _head; ptr != NULL; ptr = next) {
          next = ptr->_next;
          delete ptr;
     }
}
