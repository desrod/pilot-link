#ifndef _PILOT_DATEBOOK_H_
#define _PILOT_DATEBOOK_H_

#include "pi-args.h"
#include "pi-appinfo.h"

#ifdef __cplusplus
extern "C" {
#endif

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
	int repeatFrequency;       /* Should I skip an interval for each repetition? */
	int repeatOn;         /* What things are the targets of repetition? */
	int repeatWeekstart;  /* What day did the user decide starts the week? */
	
	int exceptions;       /* How many repetitions are their to be ignored? */
	struct tm * exception; /* What are they? */
	
	char * description;   /* What is the description of this appointment? */
	char * note;          /* Is there a note to go along with it? */
};

struct AppointmentAppInfo {
  struct CategoryAppInfo category;
  int startOfWeek;
};

extern void free_Appointment PI_ARGS((struct Appointment *));
extern int unpack_Appointment PI_ARGS((struct Appointment *, unsigned char * record, int len));
extern int pack_Appointment PI_ARGS((struct Appointment *, unsigned char * record, int len));
extern int unpack_AppointmentAppInfo PI_ARGS((struct AppointmentAppInfo *, unsigned char * AppInfo, int len));
extern int pack_AppointmentAppInfo PI_ARGS((struct AppointmentAppInfo *, unsigned char * AppInfo, int len));

#ifdef __cplusplus
}

#include "pi-datebook.hxx"

#endif /* __cplusplus */

#endif /* _PILOT_DATEBOOK_H_ */
