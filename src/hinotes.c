/*
 * hinotes.c:  Translate Palm Hi-Note Text Memos into e-mail format
 *
 * Copyright (c) 1996, Kenneth Albanowski
 * Based on code by Bill Goodman, modified by Michael Bravo
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
#include <string.h>
#include <sys/stat.h>

#include "pi-source.h"
#include "pi-hinote.h"
#include "pi-dlp.h"
#include "pi-header.h"
#include "pi-userland.h"

/* constants to determine how to produce memos */
#define MEMO_MBOX_STDOUT 0
#define MEMO_DIRECTORY 1
#define MAXDIRNAMELEN 1024

void write_memo_mbox(struct PilotUser User, struct HiNoteNote m,
		     struct HiNoteAppInfo mai, int category);

void write_memo_in_directory(char *dirname, struct HiNoteNote m,
			     struct HiNoteAppInfo mai, int category);

void write_memo_mbox(struct PilotUser User, struct HiNoteNote m,
		     struct HiNoteAppInfo mai, int category)
{
	int 	j;

	time_t 	ltime;
	struct 	tm *tm_ptr;
	char 	c,
		fromtmbuf[80],
		recvtmbuf[80];

	time(&ltime);
	tm_ptr 	= localtime(&ltime);
	c 	= *asctime(tm_ptr);

	strftime(fromtmbuf, 80, "%a, %d %b %H:%M:%S %Y (%Z)\n", tm_ptr);
	strftime(recvtmbuf, 80, "%d %b %H:%M:%S %Y\n", tm_ptr);

	printf("From your.Palm.device %s"
	       "Received: On your Palm by Hi-Note %s"
	       "To: %s\n"
	       "Date: %s"
	       "Subject: [%s] ", fromtmbuf, recvtmbuf, User.username,
	       fromtmbuf, mai.category.name[category]);

	/* print (at least part of) first line as part of subject: */
	for (j = 0; j < 40; j++) {
		if ((!m.text[j]) || (m.text[j] == '\n'))
			break;
		printf("%c", m.text[j]);
	}
	if (j == 40)
		printf("...\n");
	else
		printf("\n");
	printf("\n");
	printf(m.text);
	printf("\n");
}

void write_memo_in_directory(char *dirname, struct HiNoteNote m,
			     struct HiNoteAppInfo mai, int category)
{
	int 	j;
	char 	pathbuffer[MAXDIRNAMELEN + (128 * 3)] = "",
		tmp[5] = "";
	FILE *fd;

	/* Should check if dirname exists and is a directory */
	mkdir(dirname, 0755);

	/* create a directory for the category */
	strncat(pathbuffer, dirname, MAXDIRNAMELEN);
	strncat(pathbuffer, "/", 1);

	/* Should make sure category doesn't have slashes in it */
	strncat(pathbuffer, mai.category.name[category], 60);

	/* Should check if dirname exists and is a directory */
	mkdir(pathbuffer, 0755);

	/* Should check if there were problems creating directory */

	/* open the actual file to write */
	strncat(pathbuffer, "/", 1);
	for (j = 0; j < 40; j++) {
		if ((!m.text[j]) || (m.text[j] == '\n'))
			break;
		if (m.text[j] == '/') {
			strncat(pathbuffer, "=2F", 3);
			continue;
		}
		if (m.text[j] == '=') {
			strncat(pathbuffer, "=3D", 3);
			continue;
		}
		/* escape if it's an ISO8859 control chcter (note: some
		   are printable on the Palm) */
		if ((m.text[j] | 0x7f) < ' ') {
			tmp[0] = '\0';
			sprintf(tmp, "=%2X", (unsigned char) m.text[j]);
		} else {
			tmp[0] = m.text[j];
			tmp[1] = '\0';
		}
		strcat(pathbuffer, tmp);
	}

	if (!plu_quiet) {
		printf("   Writing to file %s\n", pathbuffer);
	}
	if (!(fd = fopen(pathbuffer, "w"))) {
		fprintf(stderr,"   WARNING: can't open file \"%s\" for writing\n",
		       pathbuffer);
		return;
	}
	fputs(m.text, fd);
	fclose(fd);
}


int main(int argc, const char **argv)
{

	int 	c,		/* switch */
		db,
		i,
		sd 		= -1,
		mode 		= MEMO_MBOX_STDOUT;

	char	*dirname = NULL;

	struct 	HiNoteAppInfo mai;
	struct 	PilotUser User;

	pi_buffer_t *buffer;

	poptContext pc;

	struct poptOption options[] = {
		USERLAND_RESERVED_OPTIONS
		{"dirname", 'd', POPT_ARG_STRING, &dirname, 0, "Save memos in <dir> instead of writing to STDOUT", "dir"},
		 POPT_TABLEEND
	};

	pc = poptGetContext("hinotes", argc, argv, options, 0);
	poptSetOtherOptionHelp(pc,"\n\n"
	"   Synchronize your Hi-Notes database with your desktop machine\n\n"
	"   By default, the contents of your Palm's Hi-Notes database will be written to\n"
	"   STDOUT as a standard Unix mailbox (in mbox-format) file, with each\n"
	"   memo as a separate message.  The subject of each message will be the\n"
	"   category.\n\n"
	"   The memos will be written to STDOUT unless the '-d' option is specified.\n"
	"   Using '-d' will be save the memos in subdirectories of <dir>.  Each\n"
	"   subdirectory will contain the name of a category on the Palm where the\n"
	"   record was stored, and will contain the memos found in that category. \n\n"
	"   Each memo's filename will be the first line (up to the first 40\n"
	"   characters) of the memo.  Control characters, slashes, and equal signs\n"
	"   that would otherwise appear in filenames are converted using the correct\n"
	"   MIME's quoted-printable encoding.\n\n"
	"   ----------------------------------------------------------------------\n"
	"   WARNING  WARNING  WARNING  WARNING  WARNING  WARNING  WARNING  WARNING\n"
        "   ----------------------------------------------------------------------\n"
	"   Note that if you have two memos in the same category whose first lines\n"
	"   are identical, one of them will be OVERWRITTEN! This is unavoidable at\n"
	"   the present time, but may be fixed in a future release. Also, please note\n"
	"   that syncronizing Hi-Note images is not supported at this time, only text.\n\n"
	"   Please see http://www.cyclos.com/ for more information on Hi-Note.\n\n"
	"   Example arguments:\n"
	"      -p /dev/pilot -d ~/Palm\n\n");

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


	if (dirname != NULL) {
		mode = MEMO_DIRECTORY;
	}

		sd = plu_connect();
		if (sd<0) {
			goto error;
		}

		/* Did we get a valid socket descriptor back? */
		if (dlp_OpenConduit(sd) < 0) {
			goto error_close;
		}

		/* Tell user (via Palm) that we are starting things up */
		dlp_ReadUserInfo(sd, &User);
		dlp_OpenConduit(sd);

		/* Open the Memo Pad's database, store access handle in db */
		if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "Hi-NoteDB", &db) < 0) {
			fprintf(stderr,"   ERROR: Unable to open Hi-NoteDB on the Palm.\n");
			dlp_AddSyncLogEntry(sd,
					    "Unable to locate or open Hi-NoteDB.\nFile not found.\n");
			goto error_close;
		}

		buffer = pi_buffer_new (0xffff);
		
		dlp_ReadAppBlock(sd, db, 0, 0xffff, buffer);
		unpack_HiNoteAppInfo(&mai, buffer->data, 0xffff);

		for (i = 0;; i++) {
			int 	attr,
				category;
			struct 	HiNoteNote m;

			int len =
			    dlp_ReadRecordByIndex(sd, db, i, buffer, NULL,
						  &attr,
						  &category);

			if (len < 0)
				break;

			/* Skip deleted records */
			if ((attr & dlpRecAttrDeleted)
			    || (attr & dlpRecAttrArchived))
				continue;

			unpack_HiNoteNote(&m, buffer->data, buffer->used);
			switch (mode) {
			  case MEMO_MBOX_STDOUT:
				  write_memo_mbox(User, m, mai, category);
				  break;
			  case MEMO_DIRECTORY:
				  write_memo_in_directory(dirname, m, mai,
							  category);
				  break;
			}
		}

		pi_buffer_free(buffer);

	/* Close the Hi-Note database and write out to the Palm logfile */
	dlp_CloseDB(sd, db);
	dlp_AddSyncLogEntry(sd, "Successfully read Hi-Notes from Palm.\nThank you for using pilot-link.\n");
	dlp_EndOfSync(sd, 0);
	pi_close(sd);
	return 0;

error_close:
	pi_close(sd);

error:
	return -1;
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
