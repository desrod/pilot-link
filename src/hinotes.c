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

#include "getopt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-hinote.h"
#include "pi-dlp.h"
#include "pi-header.h"

int pilot_connect(const char *port);
static void Help(char *progname);

/* constants to determine how to produce memos */
#define MEMO_MBOX_STDOUT 0
#define MEMO_DIRECTORY 1
#define MAXDIRNAMELEN 1024

char *progname;
void write_memo_mbox(struct PilotUser User, struct HiNoteNote m,
		     struct HiNoteAppInfo mai, int category);

void write_memo_in_directory(char *dirname, struct HiNoteNote m,
			     struct HiNoteAppInfo mai, int category);

struct option options[] = {
	{"help",        no_argument,       NULL, 'h'},
	{"port",        required_argument, NULL, 'p'},
	{"dirname",     required_argument, NULL, 'd'},
	{NULL,          0,                 NULL, 0}
};


static const char *optstring = "hp:d:";

int main(int argc, char *argv[])
{

	int 	c,
		db,
		i,
		sd = -1,
		mode = MEMO_MBOX_STDOUT;

	char 	appblock[0xffff],
		dirname[MAXDIRNAMELEN] = "",
		*progname = argv[0],
		*port = NULL;

	struct 	HiNoteAppInfo mai;
	struct 	PilotUser User;

	unsigned char buffer[0xffff];

	while (((c = getopt(argc, argv, optstring)) != -1)) {
		switch (c) {
		  case 'h':
			  Help(progname);
			  exit(0);
		  case 'p':
			  port = optarg;
			  break;
		  case 'd':
			  /* Name of directory to create and store memos in */
			  strncpy(dirname, optarg, sizeof(dirname));
			  mode = MEMO_DIRECTORY;
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

		/* Tell user (via Palm) that we are starting things up */
		dlp_ReadUserInfo(sd, &User);
		dlp_OpenConduit(sd);

		/* Open the Memo Pad's database, store access handle in db */
		if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "Hi-NoteDB", &db) < 0) {
			printf("Unable to open Hi-NoteDB. Is Hi-Notes installed?\n"
			       "You must run Hi-Notes and create at least one entry first.\n");
			dlp_AddSyncLogEntry(sd,
					    "Unable to locate or open Hi-NoteDB.\nFile not found.\n");
			exit(1);
		}

		dlp_ReadAppBlock(sd, db, 0, (unsigned char *) appblock,
				 0xffff);
		unpack_HiNoteAppInfo(&mai, (unsigned char *) appblock,
				     0xffff);

		for (i = 0;; i++) {
			int 	attr,
				category;
			struct 	HiNoteNote m;
				
			int len =
			    dlp_ReadRecordByIndex(sd, db, i, buffer, 0, 0,
						  &attr,
						  &category);

			if (len < 0)
				break;

			/* Skip deleted records */
			if ((attr & dlpRecAttrDeleted)
			    || (attr & dlpRecAttrArchived))
				continue;

			unpack_HiNoteNote(&m, buffer, len);
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
	}

	/* Close the Hi-Note database and write out to the Palm logfile */
	dlp_CloseDB(sd, db);
	dlp_AddSyncLogEntry(sd, "Successfully read Hi-Notes from Palm.\nThank you for using pilot-link.\n");
	dlp_EndOfSync(sd, 0);
	pi_close(sd);
	return 0;
}


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
	tm_ptr = localtime(&ltime);
	c = *asctime(tm_ptr);

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
	puts("");
	puts(m.text);
	printf("\n");
}

void write_memo_in_directory(char *dirname, struct HiNoteNote m,
			     struct HiNoteAppInfo mai, int category)
{
	int 	j;
	char 	pathbuffer[MAXDIRNAMELEN + (128 * 3)] = "",
		tmp[5] = "";
	FILE *fd;

	/* SHOULD CHECK IF DIRNAME EXISTS AND IS A DIRECTORY */
	mkdir(dirname, 0755);

	/* SHOULD CHECK IF THERE WERE PROBLEMS CREATING DIRECTORY */

	/* create a directory for the category */
	strncat(pathbuffer, dirname, MAXDIRNAMELEN);
	strncat(pathbuffer, "/", 1);

	/* SHOULD MAKE SURE CATEGORY DOESN'T HAVE SLASHES IN IT */
	strncat(pathbuffer, mai.category.name[category], 60);

	/* SHOULD CHECK IF DIRNAME EXISTS AND IS A DIRECTORY */
	mkdir(pathbuffer, 0755);

	/* SHOULD CHECK IF THERE WERE PROBLEMS CREATING DIRECTORY */

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

	printf("Writing to file %s\n", pathbuffer);
	if (!(fd = fopen(pathbuffer, "w"))) {
		printf("%s: can't open file \"%s\" for writing\n",
		       progname, pathbuffer);
		exit(1);
	}
	fputs(m.text, fd);
	fclose(fd);
}

static void Help(char *progname)
{
	printf
	    ("   Syncronize your Hi-Notes database with your desktop or server machine\n\n"
	     "   Usage: %s -p /dev/pilot [options]\n\n" "   Options:\n"
	     "     -p <port>      Use device file <port> to communicate with Palm\n"
	     "     -d directory   Save memos in <dir> instead of writing to STDOUT\n"
	     "     -h             Display this information\n\n"
	     "   Examples: %s -p /dev/pilot -d ~/Palm\n\n"
	     "   By default, the contents of your Palm's memo database will be written to\n"
	     "   standard output as a standard Unix mailbox (mbox-format) file, with each\n"
	     "   memo as a separate message.  The subject of each message will be the\n"
	     "   category.\n\n"
	     "   The memos will be written to STDOUT unless the '-d' option is specified.\n"
	     "   Using '-d' will be save the memos in subdirectories of <dir>.  Each\n"
	     "   subdirectory will contain the name of a category on the Palm where the\n"
	     "   record was stored, and will contain the memos found in that category. \n\n"
	     "   Each memo's filename will be the first line (up to the first 40\n"
	     "   chcters) of the memo.  Control chcters, slashes, and equal signs\n"
	     "   that would otherwise appear in filenames are converted after the fashion\n"
	     "   of MIME's quoted-printable encoding.\n\n"
	     "   -- WARNING -- WARNING -- WARNING -- WARNING -- WARNING -- WARNING --\n"
	     "   Note that if you have two memos in the same category whose first lines\n"
	     "   are identical, one of them will be OVERWRITTEN! This is unavoidable at\n"
	     "   the present time, but may be fixed in a future release. Also, please note\n"
	     "   that syncronizing Hi-Note images is not supported at this time, only text.\n\n"
	     "   The serial port to connect to may be specified by the $PILOTPORT\n"
	     "   environment variable instead of by -p' on the command line. If not\n"
	     "   specified anywhere it will default to /dev/pilot.\n\n"
	     "   Please see http://www.cyclos.com/ for more information on Hi-Note.\n\n",
	     progname, progname);
	return;
}
