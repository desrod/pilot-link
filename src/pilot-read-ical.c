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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "pi-source.h"
#include "pi-todo.h"
#include "pi-datebook.h"
#include "pi-dlp.h"
#include "pi-header.h"
#include "pi-userland.h"

char *tclquote(char *in)
{
	int 	len;
	char 	*out,
		*pos;
	static char *buffer = 0;

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


int main(int argc, const char *argv[])
{
	int 	c,		/* switch */
		db,
		sd 		= -1,
		i,
		read_todos 	= -1;
	FILE 	*ical = NULL;
	char 	cmd[255],
		*ptext 		= NULL,
		*icalfile 	= NULL,
		*pubtext 	= NULL;
	struct ToDoAppInfo tai;
	pi_buffer_t *recbuf,
	    *appblock;

	poptContext pc;

	struct poptOption options[] = {
		USERLAND_RESERVED_OPTIONS
		{"datebook", 'd', POPT_ARG_VAL, &read_todos, 0,
		 "Datebook only, no ToDos", NULL},
		{"pubtext", 't', POPT_ARG_STRING, &ptext, 0,
		 "Replace text of items not started with a bullet with <pubtext>",
		 "pubtext"},
		{"file", 'f', POPT_ARG_STRING, &icalfile, 0,
		 "Write the ical formatted data to <file> (overwrites existing <file>)",
		 "file"},
		POPT_TABLEEND
	};

	pc = poptGetContext("read-ical", argc, argv, options, 0);
	poptSetOtherOptionHelp(pc,"\n\n"
		"   Dumps the DatebookDB and/or ToDo applications to ical format.\n"
		"   Requires the program 'ical'.\n\n");

	if (argc < 2) {
		poptPrintUsage(pc,stderr,0);
		return 1;
	}

	while ((c = poptGetNextOpt(pc)) >= 0) {
		fprintf(stderr,"   ERROR: Unhandled option %d.\n",c);
		return 1;
	}

	if (c < -1) {
		plu_badoption(pc,c);
		return 1;
	}


	if (icalfile == NULL) {
		fprintf(stderr, "   ERROR: ical filename not specified. Please use the -f option.\n");
		return 1;
	}

        sd = plu_connect();
        if (sd < 0)
                goto error;

	unlink(icalfile);
	sprintf(cmd, "ical -list -f - -calendar %s", icalfile);
	ical = popen(cmd, "w");
	if (ical == NULL) {
		int e = errno;
		fprintf(stderr,"   ERROR: cannot start communication with ical.\n"
			"          %s\n",strerror(e));
		return 1;
	}
	fprintf(ical, "calendar cal $ical(calendar)\n");

        if (dlp_OpenConduit(sd) < 0)
                goto error_close;


	if (read_todos) {
		/* Open the ToDo database, store access handle in db */
		if (dlp_OpenDB
		    (sd, 0, 0x80 | 0x40, "ToDoDB", &db) < 0) {
			fprintf(stderr,"   ERROR: Unable to open ToDoDB on Palm.\n");
			dlp_AddSyncLogEntry(sd, "Unable to open ToDoDB.\n");
			goto error_close;
		}

		appblock = pi_buffer_new(0xffff);
		dlp_ReadAppBlock(sd, db, 0, 0xffff, appblock);
		unpack_ToDoAppInfo(&tai, appblock->data, appblock->used);
		pi_buffer_free(appblock);

		recbuf = pi_buffer_new (0xffff);

		for (i = 0;; i++) {
			int 	attr,
				category;
			char 	id_buf[255];
			struct 	ToDo t;
			recordid_t id_;

			int len = dlp_ReadRecordByIndex(sd, db, i, recbuf, &id_, &attr, &category);

			if (len < 0)
				break;

			/* Skip deleted records */
			if ((attr & dlpRecAttrDeleted)
			    || (attr & dlpRecAttrArchived))
				continue;

			unpack_ToDo(&t, recbuf->data, recbuf->used);

			fprintf(ical, "set n [notice]\n");

			/* '\x95' is the "bullet" chcter */
			fprintf(ical, "$n text %s\n", tclquote((pubtext && t.description[0] != '\x95') ? pubtext : t.description));
			fprintf(ical, "$n date [date today]\n");
			fprintf(ical, "$n todo 1\n");
			fprintf(ical, "$n option Priority %d\n", t.priority);
			sprintf(id_buf, "%lx", id_);
			fprintf(ical, "$n option PilotRecordId %s\n", id_buf);
			fprintf(ical, "$n done %d\n", t.complete ? 1 : 0);
			fprintf(ical, "cal add $n\n");

			free_ToDo(&t);
		}
		pi_buffer_free(recbuf);

		/* Close the database */
		dlp_CloseDB(sd, db);

		dlp_AddSyncLogEntry(sd, "Successfully read todos from Palm.\nThank you for using pilot-link.\n");
	}

	/* Open the Datebook's database, store access handle in db */
	if (dlp_OpenDB
	    (sd, 0, 0x80 | 0x40, "DatebookDB", &db) < 0) {
		fprintf(stderr,"   ERROR: Unable to open DatebookDB on Palm.\n");
		dlp_AddSyncLogEntry(sd, "Unable to open DatebookDB.\n");
		goto error_close;
	}

	recbuf = pi_buffer_new (0xffff);

	for (i = 0;; i++) {
		int 	j,
			attr;
		char 	id_buf[255];
		struct 	Appointment a;
		recordid_t id_;

		int len =
		    dlp_ReadRecordByIndex(sd, db, i, recbuf, &id_, &attr, 0);

		if (len < 0)
			break;

		/* Skip deleted records */
		if ((attr & dlpRecAttrDeleted)
		    || (attr & dlpRecAttrArchived))
			continue;

		unpack_Appointment(&a, recbuf->data, recbuf->used);

		if (a.event) {
			fprintf(ical, "set i [notice]\n");

		} else {
			int 	start,
				end;

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

		/* '\x95' is the "bullet" chcter */
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
					if (found > 1) {
						fprintf(stderr,"   WARNING: Incomplete translation of %s\n", a.description);
					}
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

		sprintf(id_buf, "%lx", id_);
		fprintf(ical, "$i option PilotRecordId %s\n", id_buf);
		fprintf(ical, "cal add $i\n");

		free_Appointment(&a);

	}

	pi_buffer_free (recbuf);

	fprintf(ical, "cal save [cal main]\n");
	fprintf(ical, "exit\n");

	pclose(ical);

	/* Close the database */
	dlp_CloseDB(sd, db);

	dlp_AddSyncLogEntry(sd, "read-ical successfully read Datebook from Palm.\n"
				"Thank you for using pilot-link.\n");
	dlp_EndOfSync(sd, 0);
	pi_close(sd);
	return 0;

error_close:
	if (ical) {
		/* explicitly do NOT save on failure. since we've already unlinked
		   the original ical file, we've still destroyed something. */
		fprintf(ical, "exit\n");
		pclose(ical);
	}
        pi_close(sd);

error:
        return -1;
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
