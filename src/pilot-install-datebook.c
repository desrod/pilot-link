/* 
 * install-datebook.c:  Palm datebook installer
 *
 * Copyright 1997, Tero Kivinen
 * Copyright 1996, Robert A. Kaplan (original install-todos.c)
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "popt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pi-source.h"
#include "pi-dlp.h"
#include "pi-datebook.h"

/* Declare prototypes */
static void display_help(const char *progname);
void print_splash(char *progname);
int pilot_connect(const char *porg);
extern time_t parsedate(char *p);

static void display_help(const char *progname)
{
	printf("   Installs new Datebook entries onto your Palm handheld device\n\n");
	printf("   Usage: %s -p <port> -r [file]\n\n", progname);
	printf("   Options:\n");
	printf("     -p <port>         Use device file <port> to communicate with Palm\n");
	printf("     -h --help         Display this information\n");
	printf("     -v --version      Display version information\n");
	printf("     -r [file]	       Read entries from file\n\n");
	printf("   Examples: \n");
	printf("      %s -p /dev/pilot -r db.txt\n\n", progname);

        return;
}

int main(int argc, char *argv[])
{
	int 	c,		/* switch */
		Appointment_size,
		db,	
		fieldno,
		filelen,
		sd		= -1;
	
	char 	*progname	= argv[0],
		*port		= NULL,
		*cPtr		= NULL,
		*filename	= NULL,
		*file_text	= NULL,
		*fields[4];
	
	unsigned char Appointment_buf[0xffff];
	FILE 	*f;
	
	struct 	PilotUser User;
	struct 	Appointment appointment;

	poptContext pc;

	struct poptOption options[] = {
		{"port", 'p', POPT_ARG_STRING, &port, 0,
		 "Use device file <port> to communicate with Palm", "port"},
		{"help", 'h', POPT_ARG_NONE, NULL, 'h',
		 "Display help information", NULL},
		{"version", 'v', POPT_ARG_NONE, NULL, 'v',
		 "Show program version information", NULL},
		{"read", 'r', POPT_ARG_STRING, &filename, 0,
		 "Read entries from <file>", "file"},
		POPT_TABLEEND
	};

	pc = poptGetContext("install-datebook", argc, argv, options, 0);

	while ((c = poptGetNextOpt(pc)) >= 0) {
                switch (c) {

                case 'h':
                        display_help(progname);
                        return 0;
                case 'v':
                        print_splash(progname);
                        return 0;
		default:
                        display_help(progname);
                        return 0;
                }
        }

	if (c < -1) {
		/* an error occurred during option processing */
		fprintf(stderr, "%s: %s\n",
		    poptBadOption(pc, POPT_BADOPTION_NOALIAS),
		    poptStrerror(c));
		return 1;
	}
	if (filename == NULL) {
		display_help(progname);
		exit(EXIT_FAILURE);
	}

	sd = pilot_connect(port);
	if (sd < 0)
		goto error;

	if (dlp_OpenConduit(sd) < 0)
		goto error_close;

	dlp_ReadUserInfo(sd, &User);
	dlp_OpenConduit(sd);

	/* Open the Datebook's database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "DatebookDB", &db) < 0) {
		puts("Unable to open DatebookDB");
		dlp_AddSyncLogEntry(sd, "Unable to open DatebookDB.\n");
		pi_close(sd);
		exit(EXIT_FAILURE);
	}

	f = fopen(filename, "r");
	if (f == NULL) {
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	fseek(f, 0, SEEK_END);
	filelen = ftell(f);
	fseek(f, 0, SEEK_SET);

	file_text = (char *) malloc(filelen + 1);
	if (file_text == NULL) {
		perror("malloc()");
		exit(EXIT_FAILURE);
	}

	fread(file_text, filelen, 1, f);

	file_text[filelen] 	= '\0';
	cPtr 			= file_text;
	fieldno 		= 0;
	fields[fieldno++] 	= cPtr;
	
	while (cPtr - file_text < filelen) {
		if (*cPtr == '\t') {
			cPtr++;
			if (fieldno < 4) {
				fields[fieldno] = cPtr;
			}
			fieldno++;
		} else if (*cPtr == '\n') {
			/* replace CR with terminator */
			*cPtr++ = '\0';

			/* ensure right number of fields */
			if (fieldno != 4) {
				printf("Too ");
				printf(fieldno < 4 ? "few" : "many");
				printf(" fields on the line : %s\n", fields[0]);


				fieldno = 0;
				fields[fieldno++] = cPtr;
				continue;
			}

			/* replace tabs with terminators */
			for (fieldno = 1; fieldno < 4; fieldno++) {
				*(fields[fieldno]-1) = '\0';
			}

			/* woo hoo! */
			fieldno = 0;
			if (fields[0][0] == '\0' || fields[1][0] == '\0') {	/* no start time */
				appointment.event = 1;
			} else {
				appointment.event = 0;
			}
			if (fields[0][0] != '\0') {
				time_t t;

				t = parsedate(fields[0]);
				if (t == -1) {
					printf("Invalid start date or time : %s\n",
						fields[0]);
					continue;
				}
				appointment.begin = *localtime(&t);
			}
			if (fields[1][0] != '\0') {
				time_t t;

				t = parsedate(fields[1]);
				if (t == -1) {
					printf("Invalid end date or time : %s\n",
						fields[1]);
					continue;
				}
				appointment.end = *localtime(&t);
			}
			if (fields[2][0] != '\0') {
				appointment.alarm = 1;
				appointment.advance =
				    atoi(fields[2]);
				if (strchr(fields[2], 'm'))
					appointment.advanceUnits =
					    advMinutes;
				else if (strchr(fields[2], 'h'))
					appointment.advanceUnits =
					    advHours;
				else if (strchr(fields[2], 'd'))
					appointment.advanceUnits =
					    advDays;
			} else {
				appointment.alarm 	= 0;
				appointment.advance 	= 0;
				appointment.advanceUnits = 0;
			}
			appointment.repeatType 		= repeatNone;
			appointment.repeatForever 	= 0;
			appointment.repeatEnd.tm_mday 	= 0;
			appointment.repeatEnd.tm_mon 	= 0;
			appointment.repeatEnd.tm_wday 	= 0;
			appointment.repeatFrequency 	= 0;
			appointment.repeatWeekstart 	= 0;
			appointment.exceptions 		= 0;
			appointment.exception 		= NULL;
			appointment.description 	= fields[3];
			appointment.note 		= NULL;

			Appointment_size =
			    pack_Appointment(&appointment,
					     Appointment_buf,
					     sizeof
					     (Appointment_buf));
			printf("Description: %s, %s\n",
				appointment.description, appointment.note);

			printf("date: %d/%d/%d %d:%02d\n", 
				appointment.begin.tm_mon + 1, 
				appointment.begin.tm_mday, 
				appointment.begin.tm_year + 1900, 
				appointment.begin.tm_hour, 
				appointment.begin.tm_min);

			dlp_WriteRecord(sd, db, 0, 0, 0,
					Appointment_buf,
					Appointment_size, 0);
			fields[fieldno++] = cPtr;
		} else {
			cPtr++;
		}
	}

	/* Close the database */
	dlp_CloseDB(sd, db);

	/* Tell the user who it is, with a different PC id. */
	User.lastSyncPC 	= 0x00010000;
	User.successfulSyncDate = time(NULL);
	User.lastSyncDate 	= User.successfulSyncDate;
	dlp_WriteUserInfo(sd, &User);

	if (dlp_AddSyncLogEntry(sd, "Successfully wrote Appointment to Palm.\n"
				"Thank you for using pilot-link.\n") < 0);
		goto error_close;

	if(dlp_EndOfSync(sd, 0) < 0);
		goto error_close;
	
	if(pi_close(sd) < 0)
		goto error;
	
error_close:
	pi_close(sd);
	
error:
	return -1;
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
