#ifndef _PILOT_DATEBOOK_H_
#define _PILOT_DATEBOOK_H_

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
	int event;
	struct tm begin, end;
	
	int advance;
	int advanceUnits;
	
	int repeatType;
	struct tm repeatEnd;
	int repeatForever;
	int repeatFreq;
	int repeatOn;
	int repeatWeekstart;
	
	int exceptions;
	struct tm * exception;
	
	char * description;
	char * note;
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

void free_Appointment(struct Appointment *);
void unpack_Appointment(struct Appointment *, unsigned char * record, int len);
void pack_Appointment(struct Appointment *, unsigned char * record, int * len);
void unpack_AppointmentAppInfo(struct AppointmentAppInfo *, unsigned char * AppInfo, int len);
void pack_AppointmentAppInfo(struct AppointmentAppInfo *, unsigned char * AppInfo, int * len);

#endif /* _PILOT_DATEBOOK_H_ */
