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

void free_Appointment(struct Appointment *);
void unpack_Appointment(struct Appointment *, unsigned char * record, int len);
void pack_Appointment(struct Appointment *, unsigned char * record, int * len);


#endif /* _PILOT_DATEBOOK_H_ */
