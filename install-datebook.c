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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-datebook.h"

extern time_t parsedate(char *p);
int pilot_connect(const char *port);

int main(int argc, char *argv[])
{
	int 	Appointment_size,
		db,	
		fieldno,
		filelen,
		index,
		sd		= -1;
	
	char 	*port		= NULL,
		*cPtr		= NULL,
		*file_text	= NULL,
		*fields[4];
	
	unsigned char Appointment_buf[0xffff];
	FILE *f;
	
	struct 	PilotUser User;
	struct 	Appointment appointment;

	sd = pilot_connect(port);
	if (sd < 0)
		goto error;

	if (dlp_OpenConduit(sd) < 0)
		goto error_close;

	/* Ask the pilot who it is. */
	dlp_ReadUserInfo(sd, &User);

	/* Tell user (via Palm) that we are starting things up */
	dlp_OpenConduit(sd);

	/* Open the Datebook's database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "DatebookDB", &db) < 0) {
		puts("Unable to open DatebookDB");
		dlp_AddSyncLogEntry(sd, "Unable to open DatebookDB.\n");
		pi_close(sd);
		exit(1);
	}

	for (index = 2; index < argc; index++) {

		f = fopen(argv[index], "r");
		if (f == NULL) {
			perror("fopen");
			exit(1);
		}

		fseek(f, 0, SEEK_END);
		filelen = ftell(f);
		fseek(f, 0, SEEK_SET);

		file_text = (char *) malloc(filelen + 1);
		if (file_text == NULL) {
			perror("malloc()");
			exit(1);
		}

		fread(file_text, filelen, 1, f);

		file_text[filelen] = '\0';
		cPtr = file_text;
		fieldno = 0;
		fields[fieldno++] = cPtr;
		while (cPtr - file_text < filelen) {
			if (*cPtr == '\t') {
				if (fieldno >= 4) {
					if (fieldno == 4)
						printf("Too many fields on the line : %s\n",
							fields[fieldno - 1]);
					fieldno++;
					continue;
				}
				/* replace tab with terminator */
				*cPtr++ = '\0';
				fields[fieldno++] = cPtr;
			} else if (*cPtr == '\n') {
				/* replace CR with terminator */
				*cPtr++ = '\0';
				if (fieldno != 4) {
					printf("Too few fields : %s",
						fields[0]);
					fieldno = 0;
					fields[fieldno++] = cPtr;
					continue;
				}
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
					appointment.alarm = 0;
					appointment.advance = 0;
					appointment.advanceUnits = 0;
				}
				appointment.repeatType = repeatNone;
				appointment.repeatForever = 0;
				appointment.repeatEnd.tm_mday = 0;
				appointment.repeatEnd.tm_mon = 0;
				appointment.repeatEnd.tm_wday = 0;
				appointment.repeatFrequency = 0;
				appointment.repeatWeekstart = 0;
				appointment.exceptions = 0;
				appointment.exception = NULL;
				appointment.description = fields[3];
				appointment.note = NULL;

				Appointment_size =
				    pack_Appointment(&appointment,
						     Appointment_buf,
						     sizeof
						     (Appointment_buf));
				printf("desc: %s\n",
				       appointment.description);
				dlp_WriteRecord(sd, db, 0, 0, 0,
						Appointment_buf,
						Appointment_size, 0);
				fields[fieldno++] = cPtr;
			} else {
				cPtr++;
			}
		}
	}

	/* Close the database */
	dlp_CloseDB(sd, db);

	/* Tell the user who it is, with a different PC id. */
	User.lastSyncPC = 0x00010000;
	User.successfulSyncDate = time(NULL);
	User.lastSyncDate = User.successfulSyncDate;
	dlp_WriteUserInfo(sd, &User);

	dlp_AddSyncLogEntry(sd, "Successfully wrote Appointment to Palm.\n"
				"Thank you for using pilot-link.\n");

	/* All of the following code is now unnecessary, but harmless */

	dlp_EndOfSync(sd, 0);
	pi_close(sd);
	
	return 0;
	
 error_close:
	pi_close(sd);
	
 error:
	perror("\tERROR:");
	fprintf(stderr, "\n");

	return -1;
}
