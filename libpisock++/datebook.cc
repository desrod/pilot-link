#include "pi-datebook.h"

#define alarmFlag 64
#define repeatFlag 32
#define noteFlag 16
#define exceptFlag 8
#define descFlag 4

appointmentAppInfo_t::appointmentAppInfo_t(void *ai) 
     : appInfo_t(ai) 
{
     uchar_t *ptr = ((uchar_t *) ai) + BASE_APP_INFO_SIZE;

     _startOfWeek = get_byte(ptr);
}

void *appointmentAppInfo_t::pack(void) 
{
     uchar_t *buffer = new uchar_t [APPOINTMENT_APP_INFO_SIZE];
     
     baseAppInfoPack(buffer);
     
     uchar_t *ptr = buffer + BASE_APP_INFO_SIZE;
     
     set_byte(ptr, _startOfWeek);
     
     return buffer;
}

void appointment_t::unpack(void *buf, bool firstTime) 
{
     // If we unpack more than once, we need to free up any old data first
     // so that we don't leak memory
     if (firstTime == false) {
	  if (_repeatEnd != NULL)
	       delete _repeatEnd;
	  if (_numExceptions != 0)
	       delete [] _exceptions;
	  if (_description)
	       delete _description;
	  if (_note)
	       delete _note;
     }
     
     uchar_t *ptr = (uchar_t *) buf;

     unsigned short int d;
     
     _begin.tm_hour = get_byte(ptr);
     
     ++ptr;
     _begin.tm_min = get_byte(ptr);
     _begin.tm_sec = 0;
     
     ptr += 3;
     getBufTm(&_begin, ptr, false);
     
     (void) memcpy(&_end, &_begin, sizeof(tm));
     
     _end.tm_hour = get_byte(((uchar_t *) buf) + 2);
     _end.tm_min = get_byte(((uchar_t *) buf) + 3);

     if (get_short(buf) == 0xffff) {
	  _begin.tm_hour = 0;
	  _begin.tm_min = 0;
	  _end.tm_hour = 0;
	  _end.tm_min = 0;
	  _untimed = true;
     } else
	  _untimed = false;

     mktime(&_end);

     ptr += 2;
     
     int flags = get_byte(ptr);

     ptr += 2;

     if (flags & alarmFlag) {
	  _hasAlarm = true;
	  _advance = get_byte(ptr);

	  ++ptr;
	  _advanceUnits = (alarmUnits_t) get_byte(ptr);

	  ++ptr;
     } else
	  _hasAlarm = false;

     if (flags & repeatFlag) {
	  _repeatType = (repeatType_t) get_byte(ptr);

	  ptr += 2;
	  d = (unsigned short int) get_short(ptr);

	       
	  if (d != 0xffff) {
	       _repeatEnd = new tm;
	       _repeatEnd->tm_year = (d >> 9) + 4;
	       _repeatEnd->tm_mon = ((d >> 5) & 15) - 1;
	       _repeatEnd->tm_mday = d & 31;
	       _repeatEnd->tm_min = 0;
	       _repeatEnd->tm_hour = 0;
	       _repeatEnd->tm_sec = 0;
	       _repeatEnd->tm_isdst = -1;
	       mktime(_repeatEnd);
	  } else
	       _repeatEnd = NULL;

	  ptr += 2;
	  _repeatFreq = get_byte(ptr);

	  ++ptr;
	  _repeatOn = get_byte(ptr);

	  ++ptr;
	  _repeatWeekstart = get_byte(ptr);

	  ptr += 2;
     } else {
	  _repeatType = none;
	  _repeatEnd = NULL;
     }

     if (flags & exceptFlag) {
	  _numExceptions = get_short(ptr);

	  ptr += 2;
	  _exceptions = new tm [_numExceptions];

	  for (int i = 0; i < _numExceptions; i++, ptr += 2)
	       getBufTm(&_exceptions[i], ptr, false);
     } else {
	  _numExceptions = 0;
	  _exceptions = NULL;
     }
     
     int len;

     if (flags & descFlag) {
	  len = strlen((const char *) ptr) + 1;
	  _description = new char [len];
	  (void) strcpy((char *) _description, (const char *) ptr);
	  ptr += len;
     } else
	  _description = NULL;
     
     if (flags & noteFlag) {
	  len = strlen((const char *) ptr) + 1;
	  _note = new char [len];
	  (void) strcpy((char *) _note, (const char *) ptr);
	  ptr += len;
     } else
	  _note = NULL;

     _next = NULL;
}

void *appointment_t::internalPack(uchar_t *buf) 
{
     uchar_t *ptr = buf;
     
     set_byte(ptr, _begin.tm_hour);
     set_byte(++ptr, _begin.tm_min);
     set_byte(++ptr, _end.tm_hour);
     set_byte(++ptr, _end.tm_min);

     set_short(++ptr, ((_begin.tm_year - 4) << 9) | ((_begin.tm_mon  + 1) << 5) | _begin.tm_mday);
     
     if (_untimed)
	  set_long(ptr, 0xffffffff);

     ptr += 4;			// Now at buf + 8

     int iflags = 0;
     
     if (_hasAlarm) {
	  iflags |= alarmFlag;
	  set_byte(ptr, _advance);
	  set_byte(++ptr, _advanceUnits);
	  ++ptr;
     }

     if (_repeatType != none) {
	  iflags |= repeatFlag;
	  set_byte(ptr, _repeatType);
	  set_byte(++ptr, 0);

	  ++ptr;

	  if (_repeatEnd)
	       setBufTm(ptr, _repeatEnd);
	  else
	       set_short(ptr, 0xffff);
	  
	  ptr += 2;

	  set_byte(ptr, _repeatFreq);
	  set_byte(++ptr, _repeatOn);
	  set_byte(++ptr, _repeatWeekstart);
	  set_byte(++ptr, 0);

	  ++ptr;
     }
     
     if (_exceptions) {
	  iflags |= exceptFlag;

	  set_short(ptr, _numExceptions);
	  ptr += 2;

	  for (int i = 0; i < _numExceptions; i++, ptr += 2)
	       setBufTm(ptr, &_exceptions[i]);
     }

     if (_description) {
	  iflags |= descFlag;

	  (void) strcpy((char *) ptr, _description);
	  ptr += strlen(_description) + 1;
     }

     set_byte(buf + 6, iflags);

     return buf;
}

void *appointment_t::pack(int *len) 
{
     *len = 8;

     if (_hasAlarm)
	  *len += 2;
	  
     if (_repeatType != none)
	  *len += 8;

     if (_exceptions)
	  *len += 2 + (2 * _numExceptions);
     
     if (_description)
	  *len += strlen(_description) + 1;

     uchar_t *ret = new uchar_t [*len];
     return internalPack(ret);
}

void *appointment_t::pack(void *buf, int *len) 
{
     int totalLength = 8;

     if (_hasAlarm)
	  totalLength += 2;
	  
     if (_repeatType != none)
	  totalLength += 8;

     if (_exceptions)
	  totalLength += 2 + (2 * _numExceptions);
     
     if (_description)
	  totalLength += strlen(_description) + 1;

     if (*len < totalLength)
	  return NULL;

     *len = totalLength;

     return internalPack((uchar_t *) buf);
}

appointment_t::~appointment_t(void) 
{
     if (_repeatEnd)
	  delete _repeatEnd;
     if (_exceptions)
	  delete _exceptions;
     if (_note)
	  delete _note;
     if (_description)
	  delete _description;
}

appointmentList_t::appointmentList_t(appointment_t *head, bool lf = true) 
     : _head(head), _shouldFreeList(lf)
{}

appointmentList_t::~appointmentList_t() 
{
     if (_shouldFreeList == false)
	  return;

     // I own the space, free it up
     appointment_t *next;
     
     for (appointment_t *head = _head; head != NULL; head = next) {
	  next = head->_next;
	  delete head;
     }
}
