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
#include "pi-socket.h"
#include "dlp.h"
#include "datebook.h"

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
  char * p2;
  unsigned long d;
  int j;
  
  a->begin.tm_hour = get_byte(buffer);
  a->begin.tm_min = get_byte(buffer+1);
  a->begin.tm_sec = 0;
  d = (unsigned short int)get_short(buffer+4);
  a->begin.tm_year = (d >> 9) + 4;
  a->begin.tm_mon = ((d >> 5) & 15) - 1;
  a->begin.tm_mday = d & 31;
  a->end = a->begin;

  a->end.tm_hour = get_byte(buffer+2);
  a->end.tm_min = get_byte(buffer+3);
	
  if(a->end.tm_hour == -1)
    a->event = 1;
  else
    a->event = 0;
	  
  iflags = get_byte(buffer+6);

  /* buffer+7 is gapfil */
	
	p2 = buffer+8;
	
#define alarmFlag 64
#define repeatFlag 32
#define noteFlag 16
#define exceptFlag 8
#define descFlag 4
	
	if (iflags & alarmFlag) 
		{
		a->advance = get_byte(p2);
		p2+=1;
		a->advanceUnits = get_byte(p2);
		p2+=1;
		
		}
	else {
		a->advance = 0;
		a->advanceUnits = 0;
	}
		
	if (iflags & repeatFlag)
		{
		a->repeatType = get_byte(p2); p2+=2;
		d = (unsigned short int)get_short(p2); p2+=2;
		if(d==0xffff)
			a->repeatForever=1;
		else {
			a->repeatEnd.tm_year = (d >> 9) + 4;
			a->repeatEnd.tm_mon = ((d >> 5) & 15) - 1;
			a->repeatEnd.tm_mday = d & 31;
			a->repeatEnd.tm_min = 0;
			a->repeatEnd.tm_hour = 0;
			a->repeatEnd.tm_sec = 0;
			a->repeatForever = 0;
		}
		a->repeatFreq = get_byte(p2); p2++;
		a->repeatOn = get_byte(p2); p2++;
		a->repeatWeekstart = get_byte(p2); p2++;
		p2++;
		}
	else {
		a->repeatForever = 1;
		a->repeatFreq = 0;
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

void pack_Appointment(struct Appointment *, unsigned char * record, int * len);
                  
