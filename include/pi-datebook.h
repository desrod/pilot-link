#ifndef _PILOT_DATEBOOK_H_
#define _PILOT_DATEBOOK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/time.h>

enum alarmTypes {advMinutes, advHours, advDays};

enum repeatTypes {
   repeatNone,
   repeatDaily,
   repeatWeekly,
   repeatMonthlyByDay,
   repeatMonthlyByDate,
   repeatYearly
};

enum  DayOfMonthType{
       dom1stSun, dom1stMon, dom1stTue, dom1stWen, dom1stThu, dom1stFri, dom1stSat,
       dom2ndSun, dom2ndMon, dom2ndTue, dom2ndWen, dom2ndThu, dom2ndFri, dom2ndSat,
       dom3rdSun, dom3rdMon, dom3rdTue, dom3rdWen, dom3rdThu, dom3rdFri, dom3rdSat,
       dom4thSun, dom4thMon, dom4thTue, dom4thWen, dom4thThu, dom4thFri, dom4thSat,
       domLastSun, domLastMon, domLastTue, domLastWen, domLastThu, domLastFri,
      domLastSat
};

struct Appointment {
	int event;            /* Is this a timeless event? */
	struct tm begin, end; /* When does this appointment start and end? */
	
	int alarm;            /* Should an alarm go off? */
	int advance;          /* How far in advance should it be? */
	int advanceUnits;     /* What am I measuring the advance in? */
	
	int repeatType;       /* How should I repeat this appointment, if at all? */
	int repeatForever;    /* Do the repetitions end at some date? */
	struct tm repeatEnd;  /* What date to they end on? */
	int repeatFreq;       /* Should I skip an interval for each repetition? */
	int repeatOn;         /* What things are the targets of repetition? */
	int repeatWeekstart;  /* What day did the user decide starts the week? */
	
	int exceptions;       /* How many repetitions are their to be ignored? */
	struct tm * exception; /* What are they? */
	
	char * description;   /* What is the description of this appointment? */
	char * note;          /* Is there a note to go along with it? */
};

struct AppointmentAppInfo {
  unsigned int renamedcategories; /* Bitfield of categories with changed names */
  char CategoryName[16][16]; /* 16 categories of 15 characters+nul each */
  unsigned char CategoryID[16]; 
  unsigned char lastUniqueID; /* Each category gets a unique ID, for sync tracking
                                 purposes. Those from the Pilot are between 0 & 127.
                                 Those from the PC are between 128 & 255. I'm not
                                 sure what role lastUniqueID plays. */
  int startOfWeek;
};

extern void free_Appointment(struct Appointment *);
extern void unpack_Appointment(struct Appointment *, unsigned char * record, int len);
extern void pack_Appointment(struct Appointment *, unsigned char * record, int * len);
extern void unpack_AppointmentAppInfo(struct AppointmentAppInfo *, unsigned char * AppInfo, int len);
extern void pack_AppointmentAppInfo(struct AppointmentAppInfo *, unsigned char * AppInfo, int * len);

#ifdef __cplusplus
}

#include <sys/time.h>
#include "pi-appinfo.h"

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

class appointment_t
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
     bool _untimed;

     bool _hasAlarm;
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

     void *internalPack(uchar_t *);
     
   public:
     appointment_t(void *buf) { unpack(buf, true); }
     appointment_t(void) { memset(this, '\0', sizeof(appointment_t)); }
     ~appointment_t(void) ;

     void unpack(void *, bool = false);

     void *pack(int *);
     void *pack(void *, int *);
     
     tm *beginTime(void) { return &_begin; }
     tm *endTime(void) { return &_end; }
     bool untimed(void) const { return _untimed; }
     
     bool hasAlarm(void) const { return _hasAlarm; }
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
};

class appointmentList_t 
{
     appointment_t *_head;
     const bool _shouldFreeList;
     
   public:
     appointmentList_t(appointment_t *, bool);
     appointmentList_t(bool f = true) : _head(NULL), _shouldFreeList(f) { }
     ~appointmentList_t();
     
     appointment_t *first() { return _head; }
     appointment_t *next(appointment_t *ptr) { return ptr->_next; }
     void add(appointment_t *ptr) {
	  ptr->_next = _head;
	  _head = ptr;
     }
};

#endif /* __cplusplus */

#endif /* _PILOT_DATEBOOK_H_ */
