
/* datebook.cc:
 *
 * Copyright (c) 1997, 1998, Scott Grosch
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */
      
#include "pi-source.h"
#include "pi-datebook.h"

#define alarmFlag 	64
#define repeatFlag 	32
#define noteFlag 	16
#define exceptFlag 	8
#define descFlag 	4

appointmentAppInfo_t::appointmentAppInfo_t(void *ai) 
     : appInfo_t(ai) 
{
     unsigned char *ptr = ((unsigned char *) ai) + BASE_APP_INFO_SIZE;

     _startOfWeek = get_byte(ptr);
}

void *appointmentAppInfo_t::pack(void) 
{
     unsigned char *buffer = new unsigned char [APPOINTMENT_APP_INFO_SIZE];
     
     baseAppInfoPack(buffer);
     
     unsigned char *ptr = buffer + BASE_APP_INFO_SIZE;
     
     set_byte(ptr, _startOfWeek);
     
     return buffer;
}

appointment_t::appointment_t(const appointment_t &oldCopy) 
{
     (void) memcpy(this, &oldCopy, sizeof(appointment_t));

     // Now fix up any pointers manually
     int len;

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

     if (oldCopy._repeatEnd) {
	  _repeatEnd = new tm;
	  (void) memcpy(_repeatEnd, oldCopy._repeatEnd, sizeof(tm));
     }

     if (_numExceptions) {
	  _exceptions = new tm[_numExceptions];
	  (void) memcpy(_exceptions, oldCopy._exceptions,
			_numExceptions * sizeof(tm));
     }
}

void appointment_t::blank(void) 
{
     _untimed 		= _hasAlarm = _advance = 0;
     _repeatType 	= none;
     _repeatOn 		= 0;
     _numExceptions 	= 0;
     _exceptions 	= NULL;
     _description 	= _note = NULL;
     _next 		= NULL;
     _repeatEnd 	= NULL;
}

void appointment_t::unpack(void *buf
#if 0
, int firstTime 
#endif
)
{
// If we unpack more than once, we need to free up any old data first
// so that we don't leak memory
#if 0
     if (!firstTime) {
#endif
	  if (_repeatEnd != NULL)
	       delete _repeatEnd;
	  if (_numExceptions != 0)
	       delete [] _exceptions;
	  if (_description)
	       delete _description;
	  if (_note)
	       delete _note;
#if 0
     }
#endif
     
     unsigned char *ptr = (unsigned char *) buf;

     unsigned short int d;
     
     _begin.tm_hour = get_byte(ptr);
     
     ++ptr;
     _begin.tm_min = get_byte(ptr);
     _begin.tm_sec = 0;
     
     ptr += 3;
     getBufTm(&_begin, ptr, 0);
     
     (void) memcpy(&_end, &_begin, sizeof(tm));
     
     _end.tm_hour = get_byte(((unsigned char *) buf) + 2);
     _end.tm_min = get_byte(((unsigned char *) buf) + 3);

     if (get_short(buf) == 0xffff) {
	  _begin.tm_hour = 0;
	  _begin.tm_min = 0;
	  _end.tm_hour = 0;
	  _end.tm_min = 0;
	  _untimed = 1;
     } else
	  _untimed = 0;

     mktime(&_end);

     ptr += 2;
     
     int flags = get_byte(ptr);

     ptr += 2;

     if (flags & alarmFlag) {
	  _hasAlarm = 1;
	  _advance = get_byte(ptr);

	  ++ptr;
	  _advanceUnits = (alarmUnits_t) get_byte(ptr);

	  ++ptr;
     } else
	  _hasAlarm = 0;

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
	       getBufTm(&_exceptions[i], ptr, 0);
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

void *appointment_t::internalPack(unsigned char *buf) 
{
     unsigned char *ptr = buf;
     
     set_byte(ptr, _begin.tm_hour);
     set_byte(++ptr, _begin.tm_min);
     set_byte(++ptr, _end.tm_hour);
     set_byte(++ptr, _end.tm_min);

     set_short(++ptr, ((_begin.tm_year - 4) << 9) | ((_begin.tm_mon  + 1) << 5) | _begin.tm_mday);
     
     if (_untimed)
	  set_long(ptr, 0xffffffff);

     ptr += 4;	// Now at buf + 8

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

     unsigned char *ret = new unsigned char [*len];
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

     return internalPack((unsigned char *) buf);
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

int appointment_t::operator<(const appointment_t &right)
{
     tm a, b;

     (void) memcpy(&a, &_begin, sizeof(tm));
     (void) memcpy(&b, &(right._begin), sizeof(tm));

     return mktime(&a) < mktime(&b);
}

int appointment_t::operator>(const appointment_t &right)
{
     tm a, b;

     (void) memcpy(&a, &_begin, sizeof(tm));
     (void) memcpy(&b, &(right._begin), sizeof(tm));

     return mktime(&a) > mktime(&b);
}

// I doubt this works properly.  I haven't done any testing yet.
int appointment_t::operator==(const appointment_t &right) 
{
     tm a, b;

     (void) memcpy(&a, &_begin, sizeof(tm));
     (void) memcpy(&b, &(right._begin), sizeof(tm));

     if (mktime(&a) != mktime(&b)) 
	return 0;

     (void) memcpy(&a, &_end, sizeof(tm));
     (void) memcpy(&b, &(right._end), sizeof(tm));

     if (mktime(&a) != mktime(&b)) 
	return 0;
     
     if (strcmp(_description, right._description))
	return 0;

     if (strcmp(_note, right._note))
	return 0;

     if (_hasAlarm) {
	if (right._hasAlarm == 0)
	    return 0;

	if (_advance != right._advance || _advanceUnits != right._advanceUnits)
	    return 0;
     }

     if (_repeatType != right._repeatType)
	return 0;

     if (_numExceptions != right._numExceptions)
	return 0;

     return 0;
}

appointmentList_t::~appointmentList_t() 
{
     appointment_t *next;
     
     for (appointment_t *head = _head; head != NULL; head = next) {
	  next = head->_next;
	  delete head;
     }
}

// We can't just point to the data, as it might be deleted.  Make a copy
void appointmentList_t::merge(appointment_t &appointment) 
{
     appointment._next = _head;
     _head = new appointment_t(appointment);
}
 
// We can't just point to the data in the list, as it might get deleted on
// us. We need to make a real copy
void appointmentList_t::merge(appointmentList_t &list) 
{
     appointment_t *newguy;
 
     for (appointment_t *ptr = list._head; ptr != NULL; ptr = ptr->_next) {
          newguy = new appointment_t(ptr);
          newguy->_next = _head;
          _head = newguy;
     }
}
     
