/* datebook.c:  Translate Pilot datebook data formats
 *
 * Copyright (c) 1996, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-datebook.h"

/* dom1stSun = REM Sun 1  
 dom1stMon = Rem Mon 1 
 dom2ndSun = REM Sun 8 
 domLastSun = REM Sun 1 -7*/

void free_Appointment(struct Appointment * a) {
  if(a->exception)
    free(a->exception);
  if(a->description)
    free(a->description);
  if(a->note)
    free(a->note);
}

void unpack_Appointment(struct Appointment * a, unsigned char * buffer, int len) {
  int iflags;
  unsigned char * p2;
  unsigned long d;
  int j;
  
  /* Note: There are possible timezone conversion problems related to the
           use of the begin, end, repeatEnd, and exception[] members of a
           struct Appointment. As they are kept in local (wall) time in
           struct tm's, the timezone of the Pilot is irrelevant, _assuming_
           that any UNIX program keeping time in time_t's converts them to
           the correct local time. If the Pilot is in a different timezone
           than the UNIX box, it may not be simple to deduce that correct
           (desired) timezone.
           
           The easiest solution is to keep apointments in struct tm's, and
           out of time_t's. Of course, this might not actually be a help if
           you are constantly darting across timezones and trying to keep
           appointments.
                                                                    -- KJA
           */
  
  a->begin.tm_hour = get_byte(buffer);
  a->begin.tm_min = get_byte(buffer+1);
  a->begin.tm_sec = 0;
  d = (unsigned short int)get_short(buffer+4);
  a->begin.tm_year = (d >> 9) + 4;
  a->begin.tm_mon = ((d >> 5) & 15) - 1;
  a->begin.tm_mday = d & 31;
  a->begin.tm_isdst = -1;
  a->end = a->begin;

  a->end.tm_hour = get_byte(buffer+2);
  a->end.tm_min = get_byte(buffer+3);
	
  if(get_short(buffer) == 0xffff) {
    a->event = 1;
    a->begin.tm_hour = 0;
    a->begin.tm_min = 0;
    a->end.tm_hour = 0;
    a->end.tm_min = 0;
  } else {
    a->event = 0;
  }
  
  mktime(&a->begin);
  mktime(&a->end);
	  
  iflags = get_byte(buffer+6);

  /* buffer+7 is gapfil */
	
	p2 = (char*)buffer+8;
	
#define alarmFlag 64
#define repeatFlag 32
#define noteFlag 16
#define exceptFlag 8
#define descFlag 4
	
	if (iflags & alarmFlag) 
		{
		a->alarm = 1;
		a->advance = get_byte(p2);
		p2+=1;
		a->advanceUnits = get_byte(p2);
		p2+=1;
		
		}
	else {
		a->alarm = 0;
		a->advance = 0;
		a->advanceUnits = 0;
	}
		
	if (iflags & repeatFlag)
		{
		a->repeatType = get_byte(p2); p2+=2;
		d = (unsigned short int)get_short(p2); p2+=2;
		if(d==0xffff)
			a->repeatForever=1; /* repeatEnd is invalid */
		else {
			a->repeatEnd.tm_year = (d >> 9) + 4;
			a->repeatEnd.tm_mon = ((d >> 5) & 15) - 1;
			a->repeatEnd.tm_mday = d & 31;
			a->repeatEnd.tm_min = 0;
			a->repeatEnd.tm_hour = 0;
			a->repeatEnd.tm_sec = 0;
			a->repeatEnd.tm_isdst = -1;
			mktime(&a->repeatEnd);
			a->repeatForever = 0;
		}
		a->repeatFreq = get_byte(p2); p2++;
		a->repeatOn = get_byte(p2); p2++;
		a->repeatWeekstart = get_byte(p2); p2++;
		p2++;
		}
	else {
		a->repeatType = 0;
		a->repeatForever = 1; /* repeatEnd is invalid */
		a->repeatFreq = 0;
		a->repeatOn = 0;
		a->repeatWeekstart = 0;
	}

	if (iflags & exceptFlag)
		{
		a->exceptions = get_short(p2);p2+=2;
		a->exception = malloc(sizeof(struct tm)*a->exceptions);
		
		for(j=0;j<a->exceptions;j++,p2+=2) {
			d = (unsigned short int)get_short(p2);
			a->exception[j].tm_year = (d >> 9) + 4;
			a->exception[j].tm_mon = ((d >> 5) & 15) - 1;
			a->exception[j].tm_mday = d & 31;
			a->exception[j].tm_hour = 0;
			a->exception[j].tm_min = 0;
			a->exception[j].tm_sec = 0;
			a->exception[j].tm_isdst = -1;
			mktime(&a->exception[j]);
		}
		
		}
	else  {
		a->exceptions = 0;
		a->exception = 0;
	}

	if (iflags & descFlag)
	{
		a->description = strdup(p2);
		p2 += strlen(p2) + 1;
	} else
		a->description = 0;

	if (iflags & noteFlag)
	{
		a->note = strdup(p2);
		p2 += strlen(p2) + 1;
	}
	else {
		a->note = 0;
	}
  
}

void pack_Appointment(struct Appointment *a,
                      unsigned char *buf,
                      int *len)
{
    int iflags;
    char *pos;

    set_byte(buf, a->begin.tm_hour);
    set_byte(buf+1, a->begin.tm_min);
    set_byte(buf+2, a->end.tm_hour);
    set_byte(buf+3, a->end.tm_min);
    set_short(buf+4, ((a->begin.tm_year - 4) << 9) |
              ((a->begin.tm_mon  + 1) << 5) |
              a->begin.tm_mday);
    
    if (a->event)
    {
        set_long(buf, 0xffffffff);
    }

#define alarmFlag 64
#define repeatFlag 32
#define noteFlag 16
#define exceptFlag 8
#define descFlag 4

    iflags = 0;
    
    pos = (char *)buf + 8;

    if (a->alarm)
    {
        iflags |= alarmFlag;
        
        set_byte(pos, a->advance);
        set_byte(pos+1, a->advanceUnits);
        pos+=2;
    }
    
    if (a->repeatType) {

        iflags |= repeatFlag;

    	set_byte(pos, a->repeatType);
    	set_byte(pos+1, 0);
    	pos+=2;
    	
    	if (a->repeatForever)
    	    set_short(pos, 0xffff);
    	else
    	    set_short(pos, ((a->repeatEnd.tm_year - 4) << 9) |
                ((a->repeatEnd.tm_mon  + 1) << 5) |
                a->repeatEnd.tm_mday);
        
        pos+=2;
        
        set_byte(pos, a->repeatFreq); pos++;
        set_byte(pos, a->repeatOn); pos++;
        set_byte(pos, a->repeatWeekstart); pos++;
        set_byte(pos, 0); pos++;
    }
    
    if (a->exceptions) {
        int i;
        
        iflags |= exceptFlag;
        
        set_short(pos, a->exceptions); pos+=2;
        
        for(i=0;i<a->exceptions;i++,pos+=2)
    	    set_short(pos, ((a->exception[i].tm_year - 4) << 9) |
                ((a->exception[i].tm_mon  + 1) << 5) |
                a->exception[i].tm_mday);
    }

    if (a->description != NULL)
    {
        iflags |= descFlag;
        
        strcpy(pos, a->description);
        pos += strlen(pos) + 1;
    }
    
    if (a->note != NULL)
    {
        iflags |= noteFlag;
        
        strcpy(pos, a->note);
        pos += strlen(pos) + 1;
    }

    set_byte(buf+6, iflags);

    *len = ((long)pos - (long)buf);
}

                  
void unpack_AppointmentAppInfo(struct AppointmentAppInfo * ai, unsigned char * record, int len) {
  int i;
  ai->renamedcategories = get_short(record);
  record+=2;
  for(i=0;i<16;i++) {
    memcpy(ai->CategoryName[i], record, 16);
    record += 16;
  }
  memcpy(ai->CategoryID, record, 16);
  record += 16;
  ai->lastUniqueID = get_byte(record);
  record += 4;
  ai->startOfWeek = get_byte(record);
}

void pack_AppointmentAppInfo(struct AppointmentAppInfo * ai, unsigned char * record, int * len) {
  int i;
  set_short(record, ai->renamedcategories);
  record += 2;
  for(i=0;i<16;i++) {
    memcpy(record, ai->CategoryName[i], 16);
    record += 16;
  }
  memcpy(record, ai->CategoryID, 16);
  record += 16;
  set_byte(record, ai->lastUniqueID);
  record ++;
  set_byte(record, 0); /* gapfil */
  set_short(record+1, 0); /* gapfil */
  set_byte(record+3, ai->startOfWeek);
}
