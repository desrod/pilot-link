/*
 * reminders.c:  Translate Palm datebook into REMIND format
 *
 * Copyright (c) 1996, Kenneth Albanowski
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include "getopt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-datebook.h"
#include "pi-dlp.h"
#include "pi-header.h"

/* Declare prototypes */
static void display_help(char *progname);
void print_splash(char *progname);
int pilot_connect(char *port);

struct option options[] = {
	{"port",        required_argument, NULL, 'p'},
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
	{NULL,          0,                 NULL, 0}
};

static const char *optstring = "p:hv";

char 	*Weekday[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" },
	*Month[12]  = { "jan", "feb", "mar", "apr", "may", "jun", "jul", 
			"aug", "sep", "oct", "nov", "dec"};

static void display_help(char *progname)
{
	printf("   Exports your Palm Datebook database into a 'remind' data file format.\n\n");
	printf("   Usage: %s -p <port>\n\n", progname);
	printf("   Options:\n");
	printf("     -p, --port <port>       Use device file <port> to communicate with Palm\n");
	printf("     -h, --help              Display help information for %s\n", progname);
	printf("     -v, --version           Display %s version information\n\n", progname);
	printf("   Examples:\n");
	printf("     %s -p /dev/pilot   # Exports to STDOUT\n", progname);
	printf("     %s -p /dev/pilot > ~/Palm/remind.file\n\n", progname);
	printf("   Your Datebook database will be written to STDOUT as it is converted\n");
	printf("   unless redirected to a file.\n\n");
	printf("   Please see http://www.roaringpenguin.com/remind.html for more\n");
	printf("   information on the Remind Calendar Program.\n\n");
	
	return;
}

int main(int argc, char *argv[])
{
	int 	c,		/* switch */
		db,
		i,
		sd 		= -1;
	char 	*progname 	= argv[0],
		*port 		= NULL;
	unsigned char buffer[0xffff];

	while ((c = getopt_long(argc, argv, optstring, options, NULL)) != -1) {
		switch (c) {

		case 'h':
			display_help(progname);
			return 0;
		case 'v':
			print_splash(progname);
			return 0;
		case 'p':
			port = optarg;
			break;
		default:
			display_help(progname);
			return 0;
		}
	}
	
	sd = pilot_connect(port);
	if (sd < 0)
		goto error;
	
	/* Open the Datebook's database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "DatebookDB", &db) < 0) {
		puts("Unable to open DatebookDB");
		dlp_AddSyncLogEntry(sd, "Unable to open DatebookDB.\n");
		pi_close(sd);
		exit(1);
	}
	
	printf("PUSH-OMIT-CONTEXT\n");
	printf("CLEAR-OMIT-CONTEXT\n");
	
	for (i = 0;; i++) {
		int 	attr,
			j;
		char 	delta[80],
			satisfy[256];
		struct 	Appointment a;
	
		int len =
			dlp_ReadRecordByIndex(sd, db, i, buffer, 0, 0, &attr,
					      0);
	
		if (len < 0)
			break;
	
		/* Skip deleted records */
		if ((attr & dlpRecAttrDeleted)
		    || (attr & dlpRecAttrArchived))
			continue;
	
		unpack_Appointment(&a, buffer, len);
	
		strcpy(delta, "+7 ");
		satisfy[0] = 0;
	
		if (a.exceptions) {
			printf("PUSH-OMIT-CONTEXT\n");
			for (j = 0; j < a.exceptions; j++) {
				printf("OMIT %d %s %d\n",
				       a.exception[j].tm_mday,
				       Month[a.exception[j].tm_mon],
				       a.exception[j].tm_year + 1900);
			}
		}
	
		if (a.advance) {
			sprintf(delta + strlen(delta),
				"AT %2.2d:%2.2d +%d ", a.begin.tm_hour,
				a.begin.tm_min,
				a.advance *
				((a.advanceUnits ==
				  advMinutes) ? 1 : (a.advanceUnits ==
						     advHours) ? 60 : (a.
								       advanceUnits
								       ==
								       advDays)
				 ? 60 * 24 : 0));
		}
	
		if (!a.repeatForever) {
			sprintf(delta + strlen(delta), "UNTIL %d %s %d ",
				a.repeatEnd.tm_mday,
				Month[a.repeatEnd.tm_mon],
				a.repeatEnd.tm_year + 1900);
		}
	
		if (a.repeatFrequency) {
			if (a.repeatType == repeatDaily) {
				/* On the specified day... */
				printf("REM %d %s %d ", a.begin.tm_mday,
				       Month[a.begin.tm_mon],
				       a.begin.tm_year + 1900);
				if (a.repeatFrequency > 1) {
					/* And every x days afterwords */
					printf("*%d ", a.repeatFrequency);
				}
			} else if (a.repeatType == repeatMonthlyByDate) {
				/* On the x of every month */
				printf("REM %d ", a.begin.tm_mday);
	
				if (a.repeatFrequency > 1) {
	
					/* if the month is equal to the starting month mod x */
					snprintf(satisfy, 256,
						 "SATISFY [(trigdate()>=date(%d,%d,%d)) "
						 "&& (!isomitted(trigdate())) "
						 "&& (((monnum(trigdate())-1+year(trigdate())*12)%%%d) "
						 "== ((%d+%d*12)%%%d))] ",
						 a.begin.tm_year + 1900, a.begin.tm_mon + 1,
						 a.begin.tm_mday, a.repeatFrequency,
						 a.begin.tm_year + 1900, a.begin.tm_mon,
						 a.repeatFrequency);
				} else {
					snprintf(satisfy,  256,
						 "SATISFY [(trigdate()>=date(%d,%d,%d))"
						 "&& (!isomitted(trigdate()))] ",
						 a.begin.tm_year + 1900, a.begin.tm_mon + 1,
						 a.begin.tm_mday);
				}
			} else if (a.repeatType == repeatWeekly) {
				int k;
	
				/* On the chosen days of the week */
				printf("REM ");
				for (k = 0; k < 7; k++)
					if (a.repeatDays[k])
						printf("%s ", Weekday[k]);
	
				if (a.repeatFrequency > 1) {
					/* if the week is equal to the starting week mod x */
					snprintf(satisfy, 256,
						 "SATISFY [(trigdate()>=date(%d,%d,%d)) "
						 "&& (!isomitted(trigdate()))"							"&& (((coerce(\"int\",trigdate())/7)%%%d)"
						 "== ((coerce(\"int\",date(%d,%d,%d))/7)%%%d))] ",
						 a.begin.tm_year + 1900, a.begin.tm_mon + 1,
						 a.begin.tm_mday, a.repeatFrequency,
						 a.begin.tm_year + 1900, a.begin.tm_mon + 1,
						 a.begin.tm_mday, a.repeatFrequency);
				} else {
					snprintf(satisfy, 256,
						 "SATISFY [(trigdate()>=date(%d,%d,%d))  "
						 "&& (!isomitted(trigdate()))] ",
						 a.begin.tm_year + 1900, a.begin.tm_mon + 1,
						 a.begin.tm_mday);
				}
			} else if (a.repeatType == repeatMonthlyByDay) {
				int 	day,
					weekday;
	
				if (a.repeatDay >= domLastSun) {
					day = 1;
					weekday = a.repeatDay % 7;
					printf("REM %s %d -7 ",
					       Weekday[weekday], day);
				} else {
					day = a.repeatDay / 7 * 7 + 1;
					weekday = a.repeatDay % 7;
					printf("REM %s %d ",
					       Weekday[weekday], day);
				}
	
				if (a.repeatFrequency > 1) {
	
					snprintf(satisfy, 256,
						 "SATISFY [(trigdate()>=date(%d,%d,%d)) "
						 "&& (!isomitted(trigdate())) "
						 "&& (((monnum(trigdate())-1+year(trigdate())*12)%%%d) "
						 "== ((%d+%d*12)%%%d))]", a.begin.tm_year + 1900, 
						 a.begin.tm_mon + 1, a.begin.tm_mday, a.repeatFrequency, 
						 a.begin.tm_year + 1900, a.begin.tm_mon, a.repeatFrequency);
				} else {
					snprintf(satisfy, 256,
						 "SATISFY [(trigdate()>=date(%d,%d,%d)) "
						 "&& (!isomitted(trigdate()))]", a.begin.tm_year + 1900,
						 a.begin.tm_mon + 1, a.begin.tm_mday);
				}
			} else if (a.repeatType == repeatYearly) {
				/* On one day each year */
				printf("REM %d %s ", a.begin.tm_mday,
				       Month[a.begin.tm_mon]);
	
				if (a.repeatFrequency > 1) {
	
					/* if the year is equal to the starting year, mod x */
					snprintf(satisfy, 256,
						 "SATISFY [(trigdate()>=date(%d,%d,%d)) "
						 "&& (!isomitted(trigdate())) "
						 "&& ((year(trigdate())%%%d) == (%d%%%d))]", 
						 a.begin.tm_year + 1900, a.begin.tm_mon + 1, 
						 a.begin.tm_mday, a.repeatFrequency, 
						 a.begin.tm_year + 1900, a.repeatFrequency);
				} else {
					snprintf(satisfy, 256,
						 "SATISFY [(trigdate()>=date(%d,%d,%d)) "
						 "&& (!isomitted(trigdate()))]", a.begin.tm_year + 1900, 
						 a.begin.tm_mon + 1, a.begin.tm_mday);
				}
			}
	
		} else {
				/* On that one day */
			printf("REM %d %s %d ", a.begin.tm_mday,
			       Month[a.begin.tm_mon],
			       a.begin.tm_year + 1900);
		}
	
		printf("%s%s", delta, satisfy);
	
		printf("MSG %s %%a", a.description);
		if (a.note)
			printf(" (%s)", a.note);
	
		if (!a.event) {
			printf(" from %2.2d:%2.2d", a.begin.tm_hour,
			       a.begin.tm_min);
			printf(" to %2.2d:%2.2d", a.end.tm_hour,
			       a.end.tm_min);
		}
		printf("\n");
	
		if (a.exceptions)
			printf("POP-OMIT-CONTEXT\n");
	
		free_Appointment(&a);
	
	}
	printf("POP-OMIT-CONTEXT\n");
	
	/* Close the database */
	dlp_CloseDB(sd, db);
	dlp_AddSyncLogEntry(sd, "Successfully read Datebook from Palm\n"
			    "Thank you for using pilot-link\n");
	dlp_EndOfSync(sd, 0);
	pi_close(sd);

	return 0;

error:
	return -1;
}
