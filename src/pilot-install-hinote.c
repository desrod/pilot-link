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

#include "popt.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#include "pi-source.h"
#include "pi-dlp.h"
#include "pi-hinote.h"
#include "pi-header.h"

static void display_help(const char *progname)
{
	printf("   Install local files into your Hi-Notes database on your Palm device\n\n");
	printf("   Usage: %s -p /dev/pilot -c [category] <file> <file> ..n\n\n", progname);
	printf("   Options:\n");
	printf("     -p <port>      Use device file <port> to communicate with Palm\n");
	printf("     -c category    Write files to <category> in the Hi-NOte application\n");
	printf("     -h             Display this information\n\n");
	printf("   Examples: %s -p /dev/pilot -c 1 ~/Palm/Note1.txt ~/Note2.txt\n\n", progname);
	printf("   Please see http://www.cyclos.com/ for more information on Hi-Note.\n\n");

	return;
}

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
                *progname 	= argv[0],
                *file_arg;

	char    *file_text,
		*port 		= NULL,
		*cat 		= NULL,
		buf[0xffff];
	
	unsigned char note_buf[0x8000];
	FILE 	*f;
	struct 	PilotUser User;
	struct 	stat info;
	struct 	HiNoteAppInfo mai;
	struct 	HiNoteNote note;
		
	poptContext pc;

	struct poptOption options[] = {
		{"port", 'p', POPT_ARG_STRING, &port, 0, "Use device file <port> to communicate with Palm", "port"},
		{"help", 'h', POPT_ARG_NONE, NULL, 'h', "Display help information", NULL},
		{"version", 'v', POPT_ARG_NONE, NULL, 'v', "Show program version information", NULL},
		{"category", 'c', POPT_ARG_STRING, &cat, 0, "Write files to <category> in the Hi-NOte application",
		 "category"},
		POPT_TABLEEND
	};

	pc = poptGetContext("install-hinote", argc, argv, options, 0);

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

	if(poptPeekArg(pc) == NULL) {
		fprintf(stderr, "%s: No files listed to install\n", progname);
		return 0;
	}

        sd = pilot_connect(port);
        if (sd < 0)
                goto error;

        if (dlp_ReadUserInfo(sd, &User) < 0)
                goto error_close;

	/* Open Hi-Note's database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "Hi-NoteDB", &db) < 0) {
		puts("Unable to open Hi-NoteDB");
		dlp_AddSyncLogEntry(sd, "Unable to open Hi-NoteDB.\n");
		exit(EXIT_FAILURE);
	}
	
	j = dlp_ReadAppBlock(sd, db, 0, (unsigned char *) buf, 0xffff);
	unpack_HiNoteAppInfo(&mai, (unsigned char *) buf, j);	/* should check result */


	if (cat != NULL) {
		for (j = 0; j < 16; j++) {
			if (strcasecmp
			    (mai.category.name[j],
				cat) == 0) {
				category = j;
				break;
			}
		}
		if (j == 16)
			category = atoi(cat);
	}
	
	while((file_arg = poptGetArg(pc)) != NULL) {
	
		/* Attempt to check the file size */
		/* stat() returns nonzero on error */
		err = stat(file_arg, &info);
		if (err) {
		   /* FIXME: use perror() */
		   printf("Error accessing file: %s\n", file_arg);
		   exit(EXIT_FAILURE);
		}
	
		/* If size is good, open the file. */
		if (info.st_size > 28672) {

			printf("\nNote size of this note (%i bytes) is greater than allowed size of 28k\n"
			       "(28,672 bytes), please reduce into two or more pieces and sync each again.\n\n", 
				(int)info.st_size);

			exit(EXIT_FAILURE);
		} else {
			f = fopen(file_arg, "r");
		}
	
		if (f == NULL) {
			perror("fopen");
			exit(EXIT_FAILURE);
		}
	
		fseek(f, 0, SEEK_END);
		filelen = ftell(f);
		fseek(f, 0, SEEK_SET);
	
		filenamelen = strlen(file_arg);

		file_text = (char *) malloc(filelen + filenamelen + 2);
		if (file_text == NULL) {
			perror("malloc()");
			exit(EXIT_FAILURE);
		}
	
		strcpy(file_text, file_arg);
		file_text[filenamelen] = '\n';
	
		fread(file_text + filenamelen + 1, filelen, 1, f);
		file_text[filenamelen + 1 + filelen] = '\0';
	
	
		note.text = file_text;
		note.flags = 0x40;
		note.level = 0;
		note_size = pack_HiNoteNote(&note, note_buf, sizeof(note_buf));

		/* dlp_exec(sd, 0x26, 0x20, &db, 1, NULL, 0); */
		fprintf(stderr, "Installing %s to Hi-Note application...\n", file_arg);
		dlp_WriteRecord(sd, db, 0, 0, category, note_buf,
				note_size, 0);
		free(file_text);
	}
	
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
