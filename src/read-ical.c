/*
 * read-ical.c:  Translate Palm ToDo and Datebook databases into ical 2.0
 *               format
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
#include "pi-todo.h"
#include "pi-datebook.h"
#include "pi-dlp.h"
#include "pi-header.h"

int pilot_connect(const char *port);
static void Help(char *progname);

struct option options[] = {
	{"help", 	no_argument,        NULL, 'h'},
	{"port", 	required_argument,  NULL, 'p'},
	{"datebook", 	no_argument,        NULL, 'd'},
	{"pubtext", 	no_argument,        NULL, 't'},
	{NULL, 		0,                  NULL, 0}
};


/* Declare prototypes */
char *tclquote(char *in);

static const char *optstring = "hp:dt:f:";

int main(int argc, char *argv[])
{
	int c;
	int db;
	int sd = -1;
	int i;
	int read_todos = -1;
	FILE *ical;
	unsigned char buffer[0xffff];
	struct ToDoAppInfo tai;
	char cmd[255];
	char *dbonly = NULL;
	char *ptext = NULL;
	char *icalfile = NULL;
	char *port = NULL;
	char *pubtext = NULL;
	char *progname = argv[0];

	while ((c = getopt(argc, argv, optstring)) != -1) {
		switch (c) {

		  case 'h':
			  Help(progname);
			  exit(0);
		  case 'p':
			  port = optarg;
			  break;
		  case 'd':
			  dbonly = optarg;
			  read_todos = 0;
			  break;
		  case 't':
			  ptext = optarg;
			  break;
		  case 'f':
			  icalfile = optarg;
			  strncpy(icalfile, optarg, sizeof(icalfile));
			  break;
		  default:
		}
	}

	if (argc < 2 && !getenv("PILOTPORT")) {
		PalmHeader(progname);
	} else if (port == NULL && getenv("PILOTPORT")) {
		port = getenv("PILOTPORT");
	}

	if (port == NULL && argc > 1) {
		printf
		    ("\nERROR: At least one command parameter of '-p <port>' must be set, or the\n"
		     "environment variable $PILOTPORT must be if '-p' is omitted or missing.\n");
		exit(1);
	} else if (port != NULL) {

		if (icalfile == NULL) {
			printf("\nERROR: You must supply an output filename to write the ical\n"
			       "file to. Without this, the ical application cannot read\n"
			       "the data. Please see %s -h for more information.\n", progname);
			exit(1);
		} 

		sd = pilot_connect(port);

		/* Did we get a valid socket descriptor back? */
		if (dlp_OpenConduit(sd) < 0) {
			exit(1);
		} else {

			unlink(icalfile);
			printf("About to run ical here\n");
			sprintf(cmd, "ical -list -f - -calendar %s", icalfile);
			printf("Just executed the ical command 'cal -f - -calendar %s'\n", icalfile);
			ical = popen(cmd, "w");

			fprintf(ical, "calendar cal $ical(calendar)\n");

			if (read_todos) {
				/* Open the ToDo database, store access handle in db */
				if (dlp_OpenDB
				    (sd, 0, 0x80 | 0x40, "ToDoDB", &db) < 0) {
					puts("Unable to open ToDoDB");
					dlp_AddSyncLogEntry(sd, "Unable to open ToDoDB.\n");
					exit(1);
				}

				dlp_ReadAppBlock(sd, db, 0, buffer,
						 0xffff);
				unpack_ToDoAppInfo(&tai, buffer, 0xffff);

				for (i = 0;; i++) {
					struct ToDo t;
					int attr, category;
					recordid_t id;
					char id_buf[255];

					int len =
					    dlp_ReadRecordByIndex(sd, db, i, buffer, &id, 0, &attr, &category);

					if (len < 0)
						break;

					/* Skip deleted records */
					if ((attr & dlpRecAttrDeleted)
					    || (attr & dlpRecAttrArchived))
						continue;

					unpack_ToDo(&t, buffer, len);

					fprintf(ical, "set n [notice]\n");

					/* '\x95' is the "bullet" character */
					fprintf(ical, "$n text %s\n", tclquote((pubtext && t.description[0] != '\x95') ? pubtext : t.description));
					fprintf(ical, "$n date [date today]\n");
					fprintf(ical, "$n todo 1\n");
					fprintf(ical, "$n option Priority %d\n", t.priority);
					sprintf(id_buf, "%lx", id);
					fprintf(ical, "$n option PilotRecordId %s\n", id_buf);
					fprintf(ical, "$n done %d\n", t.complete ? 1 : 0);
					fprintf(ical, "cal add $n\n");

					free_ToDo(&t);
				}

				/* Close the database */
				dlp_CloseDB(sd, db);

				dlp_AddSyncLogEntry(sd, "Read todos from Palm.\n");
			}

			/* Open the Datebook's database, store access handle in db */
			if (dlp_OpenDB
			    (sd, 0, 0x80 | 0x40, "DatebookDB", &db) < 0) {
				puts("Unable to open DatebookDB");
				dlp_AddSyncLogEntry(sd, "Unable to open DatebookDB.\n");
				pi_close(sd);
				exit(1);
			}

			for (i = 0;; i++) {
				int j;
				struct Appointment a;
				int attr;
				recordid_t id;
				char id_buf[255];

				int len =
				    dlp_ReadRecordByIndex(sd, db, i, buffer, &id, 0, &attr, 0);

				if (len < 0)
					break;

				/* Skip deleted records */
				if ((attr & dlpRecAttrDeleted)
				    || (attr & dlpRecAttrArchived))
					continue;

				unpack_Appointment(&a, buffer, len);

				if (a.event) {
					fprintf(ical, "set i [notice]\n");

				} else {
					int start, end;

					fprintf(ical, "set i [appointment]\n");

					start =
					    a.begin.tm_hour * 60 +
					    a.begin.tm_min;
					end =
					    a.end.tm_hour * 60 +
					    a.end.tm_min;

					fprintf(ical, "$i starttime %d\n", start);
					fprintf(ical, "$i length %d\n", end - start);
				}

				/* Don't hilight private (secret) records */
				if (attr & dlpRecAttrSecret) {
					fprintf(ical, "$i hilite never\n");
				}

				/* Handle alarms */
				if (a.alarm) {
					if (a.event) {
						if (a.advanceUnits == 2) {
							fprintf(ical, "$i earlywarning %d\n", a.advance);
						} else {
							printf("Minute or hour alarm on untimed event ignored: %s\n", a.description);
						}
					} else {
						switch (a.advanceUnits) {
						  case 0:
							  fprintf(ical, "$i alarms { %d }\n", a.advance);
							  break;
						  case 1:
							  fprintf(ical, "$i alarms { %d }\n", a.advance * 60);
							  break;
						  case 2:
							  fprintf(ical, "$i earlywarning %d\n", a.advance);
							  break;
						}
					}
				}

				/* '\x95' is the "bullet" character */
				fprintf(ical, "$i text %s\n",
					tclquote((pubtext
						  && a.description[0] !=
						  '\x95') ? pubtext : a.
						 description));

				fprintf(ical,
					"set begin [date make %d %d %d]\n",
					a.begin.tm_mday,
					a.begin.tm_mon + 1,
					a.begin.tm_year + 1900);

				if (a.repeatFrequency) {
					if (a.repeatType == repeatDaily) {
						fprintf(ical,
							"$i dayrepeat %d $begin\n", a.repeatFrequency);
					} else if (a.repeatType == repeatMonthlyByDate) {
						fprintf(ical, "$i month_day %d $begin %d\n", a.begin.tm_mon + 1, a.repeatFrequency);
					} else if (a.repeatType == repeatMonthlyByDay) {
						if (a.repeatDay >= domLastSun) {
							fprintf(ical, "$i month_last_week_day %d 1 $begin %d\n", a.repeatDay % 7 + 1, a.repeatFrequency); 
						} else {
							fprintf(ical, "$i month_week_day %d %d $begin %d\n", a.repeatDay % 7 + 1, a.repeatDay / 7 + 1, a.repeatFrequency);
						}
					} else if (a.repeatType == repeatWeekly) {
						/*
						 * Handle the case where the user said weekly repeat, but
						 * really meant daily repeat every n*7 days.  Note: We can't
						 * do days of the week and a repeat-frequency > 1, so do the
						 * best we can and go on.
						 */
						if (a.repeatFrequency > 1) {
							int ii, found;

							for (ii = 0, found = 0; ii < 7; ii++) {
								if (a.repeatDays [ii])
									found++;
							}
							if (found > 1)
								printf("Incomplete translation of %s\n", a.description);
							fprintf(ical, "$i dayrepeat %d $begin\n", a.repeatFrequency * 7);
						} else {
							int ii;

							fprintf(ical, "$i weekdays ");
							for (ii = 0; ii < 7; ii++)
								if (a.repeatDays [ii])
									fprintf(ical, "%d ", ii + 1);
							fprintf(ical, "\n");
						}
					} else if (a.repeatType == repeatYearly) {
						fprintf(ical, "$i monthrepeat %d $begin\n", 12 * a.repeatFrequency);
					}
					fprintf(ical, "$i start $begin\n");
					if (!a.repeatForever)
						fprintf(ical,
							"$i finish [date make %d %d %d]\n",
							a.repeatEnd.tm_mday, a.repeatEnd.tm_mon + 1, a.repeatEnd.tm_year + 1900);
					if (a.exceptions)
						for (j = 0; j < a.exceptions; j++)
							fprintf(ical, "$i deletion [date make %d %d %d]\n", a.exception[j].tm_mday, a.exception[j].tm_mon + 1, a.exception[j].tm_year + 1900);
				} else
					fprintf(ical, "$i date $begin\n");

				sprintf(id_buf, "%lx", id);
				fprintf(ical, "$i option PilotRecordId %s\n", id_buf);
				fprintf(ical, "cal add $i\n");

				free_Appointment(&a);

			}

			fprintf(ical, "cal save [cal main]\n");
			fprintf(ical, "exit\n");

			pclose(ical);

			/* Close the database */
			dlp_CloseDB(sd, db);

			dlp_AddSyncLogEntry(sd, "read-ical successfully read Datebook from Palm.\nThank you for using pilot-link.\n");
			pi_close(sd);
			return 0;
		}
	}
	return 0;
}


char *tclquote(char *in)
{
	static char *buffer = 0;
	char *out;
	char *pos;
	int len;

	/* Skip leading bullet (and any whitespace after) */
	if (in[0] == '\x95') {
		++in;
		while (in[0] == ' ' || in[0] == '\t') {
			++in;
		}
	}

	len = 3;
	pos = in;
	while (*pos) {
		if ((*pos == '\\') || (*pos == '"') || (*pos == '[')
		    || (*pos == '{')
		    || (*pos == '$'))
			len++;
		len++;
		pos++;
	}

	if (buffer)
		free(buffer);
	buffer = (char *) malloc(len);
	out = buffer;

	pos = in;
	*out++ = '"';
	while (*pos) {
		if ((*pos == '\\') || (*pos == '"') || (*pos == '[')
		    || (*pos == '{')
		    || (*pos == '$'))
			*out++ = '\\';
		*out++ = *pos++;
	}
	*out++ = '"';
	*out++ = '\0';

	return buffer;
}

static void Help(char *progname)
{
	printf("   Dumps the DatebookDB and/or ToDo applications to ical format\n\n"
	       "   Usage: %s -p <port> [-d] [-t pubtext] -f icalfile\n"
	       "   Options:\n"
	       "   Note: calfile will be overwritten!\n"
	       "   -p <port> Use device file <port> to communicate with Palm\n"
	       "   -d             Datebook only, no ToDos\n"
	       "   -t pubtext     Replace text of items not starting with a bullet\n"
	       "                  with pubtext\n"
	       "   -f filename    Write the ical formatted file to this filename\n"
	       "   -h             Display this information\n\n", progname);
	return;
}
