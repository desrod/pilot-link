extern "C" {
#include <memory.h>
}

// The correct header is <iostream>, <iostream.h> was never a standard C++
// header.  'cin', 'cout' etc. are located in the 'std::' namespace and
// shoudl be used as 'std::cin', 'std::cout' etc. or as 'cin', 'cout' if
// "using namespace std;"
//
// #include <iostream.h>
#include <iostream>
using namespace std;

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
     friend class appointmentList_t;
     
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

     void blank(void);
     
   public:
     appointment_t(void) : baseApp_t() { blank(); }
     appointment_t(void *buf) : baseApp_t() {
	  blank();
	  unpack(buf);
     }
     appointment_t(void *buf, int attr, recordid_t id, int category)
	  : baseApp_t(attr, id, category)
	  {
	       blank();
	       unpack(buf);
	  }
     appointment_t(char *desc, time_t begin, time_t end)
	  : baseApp_t() {
	       blank();
	       beginTime(begin);
	       endTime(end);
	       description(desc);
     }
     
     appointment_t(const appointment_t &);
     
     ~appointment_t(void) ;

     void unpack(void *);

     void *pack(int *);
     void *pack(void *, int *);
     
     tm *beginTime(void) { return &_begin; }
     void beginTime(time_t b) { _begin = *localtime(&b); } 
     
     tm *endTime(void) { return &_end; }
     void endTime(time_t e) { _end = *localtime(&e); }
     
     int untimed(void) const { return _untimed; }
     
     int hasAlarm(void) const { return _hasAlarm; }

     alarmUnits_t advanceUnits(void) const { return _advanceUnits; }
     void advanceUnits(alarmUnits_t u) { _advanceUnits = u; }
     
     int advance(void) const { return _advance; }
     void advance(int a) { _advance = a; }
     
     repeatType_t repeatType(void) const { return _repeatType; }
     void repeatType(repeatType_t r) { _repeatType = r; }
     
     tm *repeatEnd(void) const { return _repeatEnd; }
     void repeatEnd(tm *e) {
	  if (_repeatEnd)
	       delete _repeatEnd;
	  
	  _repeatEnd = new tm;
	  (void) memcpy(_repeatEnd, e, sizeof(tm));
     }
     
     int repeatFreq(void) const { return _repeatFreq; }
     void repeatFreq(int f) { _repeatFreq = f; }
     
     int repeatOn(void) const { return _repeatOn; }
     void repeatOn(int r) { _repeatOn = r; }
     
     int repeatWeekstart(void) const { return _repeatWeekstart; }
     void repeatWeekstart(int r) { _repeatWeekstart = r; }

     int numExceptions(void) const { return _numExceptions; }
     tm *exceptions(void) { return _exceptions; }
     
     const char *description(void) const { return _description; }
     void description(char *d) {
	  if (_description)
	       delete [] _description;
	  
	  _description = new char[strlen(d) + 1];
	  (void) strcpy(_description, d);
     }
     
     const char *note(void) const { return _note; }
     void note(const char *const n) {
	  if (_note)
	       delete [] _note;
	  
	  _note = new char[strlen(n) + 1];
	  (void) strcpy(_note, n);
     }

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

