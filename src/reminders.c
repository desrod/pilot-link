/* reminders.c:  Translate Pilot datebook into REMIND format
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
#include "pi-datebook.h"
#include "pi-dlp.h"

char *Weekday[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
char *Month[12] = {"jan","feb","mar","apr","may","jun","jul","aug","sep","oct","nov","dec"};
                  
int main(int argc, char *argv[])
{
  struct pi_sockaddr addr;
  int db;
  int sd;
  int i;
  struct PilotUser U;
  int ret;
  unsigned char buffer[0xffff];

  if (argc < 2) {
    fprintf(stderr,"usage:%s %s\n",argv[0],TTYPrompt);
    exit(2);
  }
  if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP))) {
    perror("pi_socket");
    exit(1);
  }
    
  addr.pi_family = PI_AF_SLP;
  strcpy(addr.pi_device,argv[1]);
  
  ret = pi_bind(sd, (struct sockaddr*)&addr, sizeof(addr));
  if(ret == -1) {
    perror("pi_bind");
    exit(1);
  }

  ret = pi_listen(sd,1);
  if(ret == -1) {
    perror("pi_listen");
    exit(1);
  }

  sd = pi_accept(sd, 0, 0);
  if(sd == -1) {
    perror("pi_accept");
    exit(1);
  }

  /* Ask the pilot who it is. */
  dlp_ReadUserInfo(sd,&U);
  
  /* Tell user (via Pilot) that we are starting things up */
  dlp_OpenConduit(sd);
  
  /* Open the Datebook's database, store access handle in db */
  if(dlp_OpenDB(sd, 0, 0x80|0x40, "DatebookDB", &db) < 0) {
    puts("Unable to open DatebookDB");
    dlp_AddSyncLogEntry(sd, "Unable to open DatebookDB.\n");
    pi_close(sd);
    exit(1);
  }

  printf("PUSH-OMIT-CONTEXT\n");  
  printf("CLEAR-OMIT-CONTEXT\n");  
  for (i=0;1;i++) {
  	int j;
  	struct Appointment a;
  	int attr;
  	char delta[80];
  	char satisfy[256];
  	                           
  	int len = dlp_ReadRecordByIndex(sd, db, i, buffer, 0, 0, &attr, 0);
  	if(len<0)
  		break;
  		
  	/* Skip deleted records */
  	if((attr & dlpRecAttrDeleted) || (attr & dlpRecAttrArchived))
  		continue;
  		
	unpack_Appointment(&a, buffer, len);

	strcpy(delta, "+7 ");
	satisfy[0] = 0;
	
	if (a.exceptions) {
		printf("PUSH-OMIT-CONTEXT\n");
		for(j=0;j<a.exceptions;j++) {
			printf("OMIT %d %s %d\n", a.exception[j].tm_mday,
						Month[a.exception[j].tm_mon],
						a.exception[j].tm_year+1900);
		}
	}
	
	
	if (a.advance) {
		sprintf(delta + strlen(delta), "AT %2.2d:%2.2d +%d ",
			a.begin.tm_hour,a.begin.tm_min, 
			a.advance * ( 
				(a.advanceUnits == advMinutes) ? 1 :
				(a.advanceUnits == advHours) ? 60 :
				(a.advanceUnits == advDays) ? 60*24 :
				0));
	}
	
	if (!a.repeatForever) {
		sprintf(delta + strlen(delta), "UNTIL %d %s %d ", 
			a.repeatEnd.tm_mday, Month[a.repeatEnd.tm_mon], a.repeatEnd.tm_year+1900);
	}

	if(a.repeatFrequency) {
		if(a.repeatType == repeatDaily) {
		        /* On the specified day... */
			printf("REM %d %s %d ",a.begin.tm_mday,Month[a.begin.tm_mon],a.begin.tm_year+1900);
			if(a.repeatFrequency > 1) {
				/* And every x days afterwords */
				printf("*%d ",a.repeatFrequency);
			}
		} else if(a.repeatType == repeatMonthlyByDate) {
			/* On the x of every month */
			printf("REM %d ", a.begin.tm_mday);

			if(a.repeatFrequency>1) {

				/* if the month is equal to the starting month mod x */
				sprintf(satisfy,"SATISFY \
[(trigdate()>=date(%d,%d,%d)) && \
 (!isomitted(trigdate())) && \
(((monnum(trigdate())-1+year(trigdate())*12)%%%d) == ((%d+%d*12)%%%d))] ",
					a.begin.tm_year+1900,
					a.begin.tm_mon+1,
					a.begin.tm_mday,
					a.repeatFrequency,
					a.begin.tm_year+1900,
					a.begin.tm_mon,
					a.repeatFrequency);
			} else {
				sprintf(satisfy, "SATISFY [(trigdate()>=date(%d,%d,%d)) && (!isomitted(trigdate()))] ",
					a.begin.tm_year+1900,
					a.begin.tm_mon+1,
					a.begin.tm_mday);
			}
		} else if(a.repeatType == repeatWeekly) {
			int k;
			/* On the chosen days of the week */
			printf("REM ");
			for(k=0;k<7;k++)
				if(a.repeatDays[i])
					printf("%s ", Weekday[i]);

			if(a.repeatFrequency>1) {
				/* if the week is equal to the starting week mod x */
				sprintf(satisfy, "SATISFY \
[(trigdate()>=date(%d,%d,%d)) &&\
  (!isomitted(trigdate())) &&\
  (((coerce(\"int\",trigdate())/7)%%%d) == ((coerce(\"int\",date(%d,%d,%d))/7)%%%d))] ",
		a.begin.tm_year+1900,
		a.begin.tm_mon+1,
		a.begin.tm_mday,
		a.repeatFrequency,
		a.begin.tm_year+1900,
		a.begin.tm_mon+1,
		a.begin.tm_mday,
		a.repeatFrequency);
			} else {
				sprintf(satisfy, "SATISFY [(trigdate()>=date(%d,%d,%d))  && (!isomitted(trigdate()))] ",
					a.begin.tm_year+1900,
					a.begin.tm_mon+1,
					a.begin.tm_mday);
			}
		} else if(a.repeatType == repeatMonthlyByDay) {
			int day;
			int weekday;
			
			if(a.repeatDay>=domLastSun) {
				day = 1;
				weekday = a.repeatDay % 7;
				printf("REM %s %d -7 ", Weekday[weekday], day);
			} else {
				day = a.repeatDay / 7 * 7 + 1;
				weekday = a.repeatDay % 7;
				printf("REM %s %d ", Weekday[weekday], day);
			}

			if( a.repeatFrequency > 1) {

				sprintf(satisfy,"SATISFY \
[(trigdate()>=date(%d,%d,%d)) && \
 (!isomitted(trigdate())) && \
(((monnum(trigdate())-1+year(trigdate())*12)%%%d) == ((%d+%d*12)%%%d))] ",
					a.begin.tm_year+1900,
					a.begin.tm_mon+1,
					a.begin.tm_mday,
					a.repeatFrequency,
					a.begin.tm_year+1900,
					a.begin.tm_mon,
					a.repeatFrequency);
			} else {
				sprintf(satisfy, "SATISFY [(trigdate()>=date(%d,%d,%d))  && (!isomitted(trigdate()))] ",
					a.begin.tm_year+1900,
					a.begin.tm_mon+1,
					a.begin.tm_mday);
			}
		} else if(a.repeatType == repeatYearly) {
			/* On one day each year */
			printf("REM %d %s ", a.begin.tm_mday, Month[a.begin.tm_mon]);
	
			if(a.repeatFrequency>1) {

				/* if the year is equal to the starting year, mod x */
				sprintf(satisfy,"SATISFY \
[(trigdate()>=date(%d,%d,%d)) &&\
 (!isomitted(trigdate())) &&\
 ((year(trigdate())%%%d) == (%d%%%d))] ",
					a.begin.tm_year+1900,
					a.begin.tm_mon+1,
					a.begin.tm_mday,
					a.repeatFrequency,
					a.begin.tm_year+1900,
					a.repeatFrequency);
			} else {
				sprintf(satisfy, "SATISFY [(trigdate()>=date(%d,%d,%d)) && (!isomitted(trigdate()))]",
					a.begin.tm_year+1900,
					a.begin.tm_mon+1,
					a.begin.tm_mday);
			}
		} 

	} else {
		/* On that one day */
		printf("REM %d %s %d ",a.begin.tm_mday, Month[a.begin.tm_mon], a.begin.tm_year+1900);
	}		                           
	
	printf("%s%s",delta,satisfy);
	
	printf("MSG %s %%a", a.description);
	if(a.note)
		printf(" (%s)", a.note);

	if(!a.event) {
		printf(" from %2.2d:%2.2d", a.begin.tm_hour, a.begin.tm_min);
		printf(" to %2.2d:%2.2d", a.end.tm_hour, a.end.tm_min);
	}
	printf("\n");

	if (a.exceptions)
		printf("POP-OMIT-CONTEXT\n");

	free_Appointment(&a);
		
  }
  printf("POP-OMIT-CONTEXT\n");

  /* Close the database */
  dlp_CloseDB(sd, db);

  dlp_AddSyncLogEntry(sd, "Read datebook from Pilot.\n");

  pi_close(sd);  
  
  exit(0);
}

