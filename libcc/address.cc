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
     uchar_t *ptr = ((uchar_t *) ai) + BASE_APP_INFO_SIZE;
 
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
     uchar_t *buffer = new uchar_t [ADDRESS_APP_INFO_SIZE];
     
     baseAppInfoPack(buffer);
     
     uchar_t *ptr = buffer + BASE_APP_INFO_SIZE;
     
     set_long(ptr, _dirtyFieldLabels);

     ptr += 4;
     memcpy(ptr, _labels, 352);

     ptr += 352;
     set_short(ptr, _country);

     ptr += 2;
     set_byte(ptr, _sortByCompany);

     return buffer;
}

void address_t::unpack(void *buf, bool firstTime) 
{
     int i;
     
     if (firstTime == false)
	  for (i = 0; i < 19; i++)
	       if (_entry[i])
		    delete _entry[i];

     uchar_t *ptr = ((uchar_t *) buf) + 1;

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

void *address_t::internalPack(uchar_t *buf) 
{
     uchar_t offset = 0;
     int len;
     recordid_t contents = 0;

     uchar_t *ptr = buf + 9;
     
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
     
     uchar_t *ret = new uchar_t [*len];
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
 
     return internalPack((uchar_t *) buf);
}
