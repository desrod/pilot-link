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

#include "getopt.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-hinote.h"
#include "pi-header.h"

int pilot_connect(const char *port);
static void Help(char *progname);

/* Not used yet, getopt_long() coming soon! 
struct option options[] = {
	{"help",        no_argument,       NULL, 'h'},
	{"port",        required_argument, NULL, 'p'},
	{"cat",         required_argument, NULL, 'c'},
	{NULL,          0,                 NULL, 0}
};
*/

static const char *optstring = "hp:c:";


int main(int argc, char *argv[])
{
	int 	db,
		sd,
		index,
		ch,
		j,
		filenamelen,
		filelen,
		err,
		category = 0,
		note_size;
	
	char 	*file_text,
		*progname = argv[0],
		*port = NULL,
		*cat = NULL,
		buf[0xffff];
	
	unsigned char note_buf[0x8000];
	FILE 	*f;
	struct 	PilotUser User;
	struct 	stat info;
	struct 	HiNoteAppInfo mai;
	struct 	HiNoteNote note;
		
	
	while (((ch = getopt(argc, argv, optstring)) != -1)) {
		switch (ch) {
		  case 'h':
			  Help(progname);
			  exit(0);
		  case 'p':
			  port = optarg;
			  break;
		  case 'c':
			  cat = optarg;
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

		sd = pilot_connect(port);

		/* Did we get a valid socket descriptor back? */
		if (dlp_OpenConduit(sd) < 0) {
			exit(1);
		}
		
		/* Ask the pilot who it is. */
		dlp_ReadUserInfo(sd, &User);
	
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
		
		for (index = 2; index < argc; index++) {
	
			if (strcmp(argv[index], "-c") == 0) {
				for (j = 0; j < 16; j++)
					if (strcasecmp
					    (mai.category.name[j],
					     argv[index + 1]) == 0) {
						category = j;
						break;
					}
				if (j == 16)
					category = atoi(argv[index + 1]);
				index++;
				continue;
			}
	
			/* Attempt to check the file size */
			/* stat() returns nonzero on error */
			err = stat(argv[index], &info);
			if (err) {
			   /* FIXME: use perror() */
			   printf("Error accessing file: %s\n", argv[index]);
			   exit(1);
			}
	
			/* If size is good, open the file. */
			if (info.st_size > 28672) {
	
				printf("\nNote size of this note (%i bytes) is greater than allowed size of 28k\n"
				       "(28,672 bytes), please reduce into two or more pieces and sync each again.\n\n", 
					(int)info.st_size);
	
				exit(1);
			} else {
				f = fopen(argv[index], "r");
			}
	
			if (f == NULL) {
				perror("fopen");
				exit(1);
			}
	
			fseek(f, 0, SEEK_END);
			filelen = ftell(f);
			fseek(f, 0, SEEK_SET);
	
			filenamelen = strlen(argv[index]);
	
			file_text = (char *) malloc(filelen + filenamelen + 2);
			if (file_text == NULL) {
				perror("malloc()");
				exit(1);
			}
	
			strcpy(file_text, argv[index]);
			file_text[filenamelen] = '\n';
	
			fread(file_text + filenamelen + 1, filelen, 1, f);
			file_text[filenamelen + 1 + filelen] = '\0';
	
	
			note.text = file_text;
			note.flags = 0x40;
			note.level = 0;
			note_size =
			    pack_HiNoteNote(&note, note_buf, sizeof(note_buf));
	
			/* dlp_exec(sd, 0x26, 0x20, &db, 1, NULL, 0); */
			fprintf(stderr, "Installing %s to Hi-Note application...\n", argv[index]);
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
	
		/* All of the following code is now unnecessary, but harmless */
	
		dlp_EndOfSync(sd, 0);
		pi_close(sd);
	}
	return 0;
}


static void Help(char *progname)
{
	printf
	    ("   Install local files into your Hi-Notes database on your Palm device\n\n"
	     "   Usage: %s -p /dev/pilot -c [category] <file> <file> ..n\n\n" 
	     "   Options:\n"
	     "     -p <port>      Use device file <port> to communicate with Palm\n"
	     "     -c category    Write files to <category> in the Hi-NOte application\n"
	     "     -h             Display this information\n\n"
	     "   Examples: %s -p /dev/pilot -c 1 ~/Palm/Note1.txt ~/Note2.txt\n\n"
	     "   Please see http://www.cyclos.com/ for more information on Hi-Note.\n\n",
	     progname, progname);
	return;
}
