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

#include <stdio.h>
#include <stdlib.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-datebook.h"

extern time_t parsedate(char *p);

int main(int argc, char *argv[])
{
	struct pi_sockaddr addr;
	struct PilotUser U;
	struct Appointment appointment;
	int Appointment_size;
	int db;
	int fieldno;
	int filelen;
	int i;
	int ret;
	int sd;
	char *cPtr;
	char *file_text;
	char *fields[4];
	unsigned char Appointment_buf[0xffff];
	FILE *f;

	if (argc < 3) {
		fprintf(stderr, "usage:%s %s file [file] ...\n", argv[0],
			TTYPrompt);
		exit(2);
	}
	if (!(sd = pi_socket(PI_AF_PILOT, PI_SOCK_STREAM, PI_PF_PADP))) {
		perror("pi_socket");
		exit(1);
	}

	addr.pi_family = PI_AF_PILOT;
	strcpy(addr.pi_device, argv[1]);

	ret = pi_bind(sd, (struct sockaddr *) &addr, sizeof(addr));
	if (ret == -1) {
		perror("pi_bind");
		exit(1);
	}

	ret = pi_listen(sd, 1);
	if (ret == -1) {
		perror("pi_listen");
		exit(1);
	}

	sd = pi_accept(sd, 0, 0);
	if (sd == -1) {
		perror("pi_accept");
		exit(1);
	}

	/* Ask the pilot who it is. */
	dlp_ReadUserInfo(sd, &U);

	/* Tell user (via Palm) that we are starting things up */
	dlp_OpenConduit(sd);

	/* Open the Datebook's database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "DatebookDB", &db) < 0) {
		puts("Unable to open DatebookDB");
		dlp_AddSyncLogEntry(sd, "Unable to open DatebookDB.\n");
		pi_close(sd);
		exit(1);
	}

	for (i = 2; i < argc; i++) {

		f = fopen(argv[i], "r");
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
						fprintf(stderr,
							"Too many fields on the line : %s\n",
							fields[fieldno -
							       1]);
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
					fprintf(stderr,
						"Too few fields : %s",
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
						fprintf(stderr,
							"Invalid start date or time : %s\n",
							fields[0]);
						continue;
					}
					appointment.begin = *localtime(&t);
				}
				if (fields[1][0] != '\0') {
					time_t t;

					t = parsedate(fields[1]);
					if (t == -1) {
						fprintf(stderr,
							"Invalid end date or time : %s\n",
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
	U.lastSyncPC = 0x00010000;
	U.successfulSyncDate = time(NULL);
	U.lastSyncDate = U.successfulSyncDate;
	dlp_WriteUserInfo(sd, &U);

	dlp_AddSyncLogEntry(sd, "Wrote Appointment to Palm.\n");

	/* All of the following code is now unnecessary, but harmless */

	dlp_EndOfSync(sd, 0);
	pi_close(sd);
	exit(0);
}
