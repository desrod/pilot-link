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
#include <sys/stat.h>
#include <string.h>

#include "pi-source.h"
#include "pi-dlp.h"
#include "pi-hinote.h"
#include "pi-header.h"
#include "pi-userland.h"

/*
 * Size of maximum buffer, containing 28k of note + a filename + \n + NUL at the end.
 */
#define HINOTE_BUFFER_SIZE (28 * 1024 + FILENAME_MAX + 2)

int main(int argc, const char *argv[])
{
	int 	db,
		sd	= -1,
		c,		/* switch */
		j,
		filenamelen,
		filelen,
		err,
		category 	= 0,
		note_size;

	const char
                *file_arg;

	char    *file_text,
		*cat 		= NULL,
		buf[0xffff];
	pi_buffer_t *appblock;

	unsigned char note_buf[0x8000];
	FILE 	*f;
	struct 	PilotUser User;
	struct 	stat info;
	struct 	HiNoteAppInfo mai;
	struct 	HiNoteNote note;

	poptContext pc;

	struct poptOption options[] = {
		USERLAND_RESERVED_OPTIONS
		{"category", 'c', POPT_ARG_STRING, &cat, 0, "Write files to <category> in the Hi-NOte application",
		 "category"},
		POPT_TABLEEND
	};

	pc = poptGetContext("install-hinote", argc, argv, options, 0);
	poptSetOtherOptionHelp(pc,"<file> ...\n\n"
		"   Install local files into your Hi-Notes database on your Palm device\n"
		"   Please see http://www.cyclos.com/ for more information on Hi-Note.\n\n"
		"   Example arguments:\n"
		"      -p /dev/pilot -c 1 ~/Palm/Note1.txt ~/Note2.txt\n\n");

	if (argc < 2) {
		poptPrintUsage(pc,stderr,0);
		return 1;
	}

	while ((c = poptGetNextOpt(pc)) >= 0) {
		fprintf(stderr,"   ERROR: Unhandled option %d.\n",c);
		return -1;
	}

	if (c < -1) {
		plu_badoption(pc,c);
	}

	if(poptPeekArg(pc) == NULL) {
		fprintf(stderr, "   WARNING: No files to install\n");
		return 0;
	}

	/* Allocate buffer for biggest possible file */
	file_text = (char *) malloc(HINOTE_BUFFER_SIZE);
	if (file_text == NULL) {
		fprintf(stderr,"   ERROR: Cannot allocate memory for files.\n");
		return 1;
	}


	sd = plu_connect();
        if (sd < 0)
                goto error;

        if (dlp_ReadUserInfo(sd, &User) < 0)
                goto error_close;

	/* Open Hi-Note's database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "Hi-NoteDB", &db) < 0) {
		fprintf(stderr,"   ERROR: Unable to open Hi-NoteDB on Palm.\n");
		dlp_AddSyncLogEntry(sd, "Unable to open Hi-NoteDB.\n");
		goto error_close;
	}

	appblock = pi_buffer_new(0xffff);
	j = dlp_ReadAppBlock(sd, db, 0, 0xffff, appblock);
	unpack_HiNoteAppInfo(&mai, appblock->data, j);	/* should check result */
	pi_buffer_free(appblock);

	if (cat != NULL) {
		category = plu_findcategory(&mai.category,cat,
			PLU_CAT_CASE_INSENSITIVE |
			PLU_CAT_MATCH_NUMBERS |
			PLU_CAT_DEFAULT_UNFILED);
	}

	while((file_arg = poptGetArg(pc)) != NULL) {

		/* Attempt to check the file size */
		/* stat() returns nonzero on error */
		err = stat(file_arg, &info);
		if (err) {
		   /* FIXME: use perror() */
		   fprintf(stderr,"   WARNING: Checking file '%s': %s\n", file_arg,strerror(errno));
		   continue;
		}

		/* If size is good, open the file. */
		filelen = info.st_size;
		if (filelen > 28672) {

			fprintf(stderr,"   WARNING: This note (%i bytes) is larger than allowed size of 28k (28,672 bytes),\n"
			       "             please reduce into two or more pieces and sync each again.\n\n",
				filelen);

			continue;
		} else {
			f = fopen(file_arg, "r");
		}

		if (f == NULL) {
			fprintf(stderr,"   WARNING: Opening file '%s': %s\n",file_arg,strerror(errno));
			continue;
		}

		filenamelen = strlen(file_arg);


		memset(file_text,0,HINOTE_BUFFER_SIZE);
		strcpy(file_text, file_arg);
		file_text[filenamelen] = '\n';

		fread(file_text + filenamelen + 1, filelen, 1, f);
		file_text[filenamelen + 1 + filelen] = '\0';


		note.text = file_text;
		note.flags = 0x40;
		note.level = 0;
		note_size = pack_HiNoteNote(&note, note_buf, sizeof(note_buf));

		/* dlp_exec(sd, 0x26, 0x20, &db, 1, NULL, 0); */
		if (!plu_quiet) {
			fprintf(stdout, "   Installing %s to Hi-Note application...\n", file_arg);
		}
		dlp_WriteRecord(sd, db, 0, 0, category, note_buf,
				note_size, 0);
	}
	free(file_text);

	/* Close the database */
	dlp_CloseDB(sd, db);

	/* Tell the user who it is, with a different PC id. */
	User.lastSyncPC = 0x00010000;
	User.successfulSyncDate = time(NULL);
	User.lastSyncDate = User.successfulSyncDate;
	dlp_WriteUserInfo(sd, &User);

	dlp_AddSyncLogEntry(sd, "Successfully wrote Hi-Note notes to Palm.\n"
				"Thank you for using pilot-link.\n");
	dlp_EndOfSync(sd, 0);
	pi_close(sd);
	return 0;

error_close:
	pi_close(sd);

error:
	return -1;
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
