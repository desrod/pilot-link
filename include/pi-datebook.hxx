
#include <sys/time.h>
/*#include "pi-appinfo.h"

const int APPOINTMENT_APP_INFO_SIZE = 280;

class appointmentAppInfo_t : public appInfo_t
{
     int _startOfWeek;

   public:
     appointmentAppInfo_t(void *);

     int startOfWeek(void) const { return _startOfWeek; }

     void *pack(void);
};

class appointmentList_t;	// Forward declaration

class appointment_t : public baseApp_t
{
   public:
     enum repeatType_t {
	  none, daily, weekly, monthlyByDay, monthlyByDate, yearly
     };
     enum alarmUnits_t {
	  minutes, hours, days
     };

   private:
     friend appointmentList_t;
     
     tm _begin;			// When the appointment begins
     tm _end;			// When the appointment ends
     int _untimed;

     int _hasAlarm;
     int _advance;		// How far in advance the alarm should go off
     alarmUnits_t _advanceUnits; // What _advance is measured in

     repeatType_t _repeatType;
	     
     tm *_repeatEnd;
     int _repeatFreq;
     int _repeatOn;
     int _repeatWeekstart;

     int _numExceptions;
     tm *_exceptions;

     char *_description;
     char *_note;

     appointment_t *_next;

     void *internalPack(unsigned char *);
     
   public:
     appointment_t(void) : baseApp_t() {
	  (void) memset(this, '\0', sizeof(appointment_t));
     }
     appointment_t(void *buf) : baseApp_t() { unpack(buf, 1); }
     appointment_t(void *buf, int attr, recordid_t id, int category)
	  : baseApp_t(attr, id, category)
	  {
	       unpack(buf, 1);
	  }
     appointment_t(const appointment_t &);
     
     ~appointment_t(void) ;

     void unpack(void *, int = 0);

     void *pack(int *);
     void *pack(void *, int *);
     
     tm *beginTime(void) { return &_begin; }
     tm *endTime(void) { return &_end; }
     int untimed(void) const { return _untimed; }
     
     int hasAlarm(void) const { return _hasAlarm; }
     alarmUnits_t advanceUnits(void) const { return _advanceUnits; }
     int advance(void) const { return _advance; }

     repeatType_t repeatType(void) const { return _repeatType; }
     tm *repeatEnd(void) const { return _repeatEnd; }
     int repeatFreq(void) const { return _repeatFreq; }
     int repeatOn(void) const { return _repeatOn; }
     int repeatWeekstart(void) const { return _repeatWeekstart; }

     int numExceptions(void) const { return _numExceptions; }
     tm *exceptions(void) { return _exceptions; }
     
     const char *description(void) const { return _description; }
     const char *note(void) const { return _note; }

     int operator==(const appointment_t &);
     int operator<(const appointment_t &);
     int operator>(const appointment_t &);
};

class appointmentList_t 
{
     appointment_t *_head;
     
   public:
     appointmentList_t(void) : _head(NULL) { }
     ~appointmentList_t();
     
     appointment_t *first() { return _head; }
     appointment_t *next(appointment_t *ptr) { return ptr->_next; }
 
     void merge(appointment_t &);
     void merge(appointmentList_t &);
};
*/

#include "pi-appinfo.hxx"

const int APPOINTMENT_APP_INFO_SIZE = 280;

class appointmentAppInfo_t : public appInfo_t
{
     int _startOfWeek;

   public:
     appointmentAppInfo_t(void *);

     int startOfWeek(void) const { return _startOfWeek; }

     void *pack(void);
};

class appointmentList_t;	// Forward declaration

class appointment_t : public baseApp_t
{
   public:
     enum repeatType_t {
	  none, daily, weekly, monthlyByDay, monthlyByDate, yearly
     };
     enum alarmUnits_t {
	  minutes, hours, days
     };

   private:
     friend appointmentList_t;
     
     tm _begin;			// When the appointment begins
     tm _end;			// When the appointment ends
     int _untimed;

     int _hasAlarm;
     int _advance;		// How far in advance the alarm should go off
     alarmUnits_t _advanceUnits; // What _advance is measured in

     repeatType_t _repeatType;
	     
     tm *_repeatEnd;
     int _repeatFreq;
     int _repeatOn;
     int _repeatWeekstart;

     int _numExceptions;
     tm *_exceptions;

     char *_description;
     char *_note;

     appointment_t *_next;

     void *internalPack(unsigned char *);
     
   public:
     appointment_t(void) : baseApp_t() {
	  (void) memset(this, '\0', sizeof(appointment_t));
     }
     appointment_t(void *buf) : baseApp_t() { unpack(buf, 1); }
     appointment_t(void *buf, int attr, recordid_t id, int category)
	  : baseApp_t(attr, id, category)
	  {
	       unpack(buf, 1);
	  }
     appointment_t(const appointment_t &);
     
     ~appointment_t(void) ;

     void unpack(void *, int = 0);

     void *pack(int *);
     void *pack(void *, int *);
     
     tm *beginTime(void) { return &_begin; }
     tm *endTime(void) { return &_end; }
     int untimed(void) const { return _untimed; }
     
     int hasAlarm(void) const { return _hasAlarm; }
     alarmUnits_t advanceUnits(void) const { return _advanceUnits; }
     int advance(void) const { return _advance; }

     repeatType_t repeatType(void) const { return _repeatType; }
     tm *repeatEnd(void) const { return _repeatEnd; }
     int repeatFreq(void) const { return _repeatFreq; }
     int repeatOn(void) const { return _repeatOn; }
     int repeatWeekstart(void) const { return _repeatWeekstart; }

     int numExceptions(void) const { return _numExceptions; }
     tm *exceptions(void) { return _exceptions; }
     
     const char *description(void) const { return _description; }
     const char *note(void) const { return _note; }

     int operator==(const appointment_t &);
     int operator<(const appointment_t &);
     int operator>(const appointment_t &);
};

class appointmentList_t 
{
     appointment_t *_head;
     
   public:
     appointmentList_t(void) : _head(NULL) { }
     ~appointmentList_t();
     
     appointment_t *first() { return _head; }
     appointment_t *next(appointment_t *ptr) { return ptr->_next; }
 
     void merge(appointment_t &);
     void merge(appointmentList_t &);
};

