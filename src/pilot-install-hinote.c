/* 
 * install-hinote.c:  Palm Hi-Note note installer
 *
 * Copyright 1997 Bill Goodman
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
#include "pi-hinote.h"
#include "pi-header.h"

int main(int argc, char *argv[])
{
	struct pi_sockaddr addr;
	int db;
	int sd;
	int i;
	int j;
	int filenamelen;
	int filelen;
	char *file_text;
	unsigned char note_buf[0x8000];
	int note_size;
	FILE *f;
	struct PilotUser U;
	int ret;
	char buf[0xffff];
	int category;
	struct HiNoteAppInfo mai;
	struct HiNoteNote note;
	char *progname = argv[0];
	char *device = argv[1];

	PalmHeader(progname);


	if (argc < 3) {
		fprintf(stderr,
			"   Usage: %s %s [-c category] file [file] ...\n\n",
			argv[0], TTYPrompt);
		exit(2);
	}
	if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP))) {
		perror("pi_socket");
		exit(1);
	}

	addr.pi_family = PI_AF_SLP;
	strcpy(addr.pi_device, device);

	ret = pi_bind(sd, (struct sockaddr *) &addr, sizeof(addr));
	if (ret == -1) {
		fprintf(stderr, "\n   Unable to bind to port %s\n",
			device);
		perror("   pi_bind");
		fprintf(stderr, "\n");
		exit(1);
	}

	printf
	    ("   Port: %s\n\n   Please press the HotSync button now...\n",
	     device);

	ret = pi_listen(sd, 1);
	if (ret == -1) {
		fprintf(stderr, "\n   Error listening on %s\n", device);
		perror("   pi_listen");
		fprintf(stderr, "\n");
		exit(1);
	}

	sd = pi_accept(sd, 0, 0);
	if (sd == -1) {
		fprintf(stderr, "\n   Error accepting data on %s\n",
			device);
		perror("   pi_accept");
		fprintf(stderr, "\n");
		exit(1);
	}

	fprintf(stderr, "Connected...\n");

	/* Ask the pilot who it is. */
	dlp_ReadUserInfo(sd, &U);

	/* Tell user (via Palm) that we are starting things up */
	dlp_OpenConduit(sd);

	/* Open Hi-Note's database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "Hi-NoteDB", &db) < 0) {
		puts("Unable to open Hi-NoteDB");
		dlp_AddSyncLogEntry(sd, "Unable to open Hi-NoteDB.\n");
		exit(1);
	}

	j = dlp_ReadAppBlock(sd, db, 0, (unsigned char *) buf, 0xffff);
	unpack_HiNoteAppInfo(&mai, (unsigned char *) buf, j);	/* should check result */

	category = 0;

	for (i = 2; i < argc; i++) {

		if (strcmp(argv[i], "-c") == 0) {
			for (j = 0; j < 16; j++)
				if (strcasecmp
				    (mai.category.name[j],
				     argv[i + 1]) == 0) {
					category = j;
					break;
				}
			if (j == 16)
				category = atoi(argv[i + 1]);
			i++;
			continue;
		}

		f = fopen(argv[i], "r");
		if (f == NULL) {
			perror("fopen");
			exit(1);
		}

		fseek(f, 0, SEEK_END);
		filelen = ftell(f);
		fseek(f, 0, SEEK_SET);

		filenamelen = strlen(argv[i]);

		file_text = (char *) malloc(filelen + filenamelen + 2);
		if (file_text == NULL) {
			perror("malloc()");
			exit(1);
		}

		strcpy(file_text, argv[i]);
		file_text[filenamelen] = '\n';

		fread(file_text + filenamelen + 1, filelen, 1, f);
		file_text[filenamelen + 1 + filelen] = '\0';

		note.text = file_text;
		note.flags = 0x40;
		note.level = 0;
		note_size =
		    pack_HiNoteNote(&note, note_buf, sizeof(note_buf));

		/* dlp_exec(sd, 0x26, 0x20, &db, 1, NULL, 0); */
		dlp_WriteRecord(sd, db, 0, 0, category, note_buf,
				note_size, 0);
		free(file_text);
	}

	/* Close the database */
	dlp_CloseDB(sd, db);

	/* Tell the user who it is, with a different PC id. */
	U.lastSyncPC = 0x00010000;
	U.successfulSyncDate = time(NULL);
	U.lastSyncDate = U.successfulSyncDate;
	dlp_WriteUserInfo(sd, &U);

	dlp_AddSyncLogEntry(sd, "Wrote Hi-Note note to Palm.\n");

	/* All of the following code is now unnecessary, but harmless */

	dlp_EndOfSync(sd, 0);
	pi_close(sd);

	return 0;
}
