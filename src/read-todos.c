/*
 * todos.c:  Translate Palm ToDo database into generic format
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

#include "getopt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-todo.h"
#include "pi-dlp.h"
#include "pi-file.h"
#include "pi-header.h"

int pilot_connect(const char *port);
static void Help(char *progname);

/* Not used yet, getopt_long() coming soon! 
struct option options[] = {
	{"help",        no_argument,       NULL, 'h'},
	{"port",        required_argument, NULL, 'p'},
	{"file",        required_argument, NULL, 'f'},
	{NULL,          0,                 NULL, 0}
};
*/

static const char *optstring = "hp:f:";

static void Help(char *progname)
{
	printf
	    ("   Syncronize your ToDo database with your desktop or server machine\n"
	     "   Usage: %s -p /dev/pilot [options]\n\n" "   Options:\n"
	     "     -p <port>      Use device file <port> to communicate with Palm\n"
	     "     -f <filename>  Save ToDO entries in <filename> instead of writing to STDOUT\n"
	     "     -h             Display this information\n\n"
	     "   Examples: %s -p /dev/pilot -d ~/Palm\n\n"
	     "   By default, the contents of your Palm's memo database will be written to\n"
	     "   standard output as a standard Unix mailbox (mbox-format) file, with each\n"
	     "   memo as a separate message.  The subject of each message will be the\n"
	     "   category.\n\n", progname, progname);
	return;
}

int main(int argc, char *argv[])
{
	int 	ch,
		db,
		i,
		sd 		= -1;
	char 	*progname 	= argv[0],
		*port 		= NULL,
		*filename 	= NULL,
		*ptr;
	struct 	PilotUser User;
	struct 	pi_file *pif 	= NULL;
	struct 	ToDoAppInfo tai;
	unsigned char buffer[0xffff];
	
	while (((ch = getopt(argc, argv, optstring)) != -1)) {
		switch (ch) {
		  case 'h':
			  Help(progname);
			  exit(0);
		  case 'p':
			  port = optarg;
			  filename = NULL;
			  break;
		  case 'f':
			  filename = optarg;
			  break;
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
		
		/* Open the ToDo database, store access handle in db */
		if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "ToDoDB", &db) < 0) {
			puts("Unable to open ToDoDB");
			dlp_AddSyncLogEntry(sd,
					    "Unable to open ToDoDB.\n");
			exit(1);
		}

		dlp_ReadAppBlock(sd, db, 0, buffer, 0xffff);


		if (filename) {
			int 	len;
	
			pif = pi_file_open(filename);
			if (!pif) {
				perror("pi_file_open");
				exit(1);
			}
	
			sd = pi_file_get_app_info(pif, (void *) &ptr, &len);
			if (sd == -1) {
				perror("pi_file_get_app_info");
				exit(1);
			}
	
			memcpy(buffer, ptr, len);
		}
	
		unpack_ToDoAppInfo(&tai, buffer, 0xffff);
	
		for (i = 0;; i++) {
			int 	attr,
				category,
				len;
			struct 	ToDo t;
	
			if (port) {
				len =
				    dlp_ReadRecordByIndex(sd, db, i, buffer, 0, 0,
							  &attr, &category);
	
				if (len < 0)
					break;
			}
			if (filename) {
				if (pi_file_read_record
				    (pif, i, (void *) &ptr, &len, &attr, &category,
				     0))
					break;
	
				memcpy(buffer, ptr, len);
			}
	
			/* Skip deleted records */
			if ((attr & dlpRecAttrDeleted)
			    || (attr & dlpRecAttrArchived))
				continue;
	
			unpack_ToDo(&t, buffer, len);
	
			printf("Category: %s\n", tai.category.name[category]);
			printf("Priority: %d\n", t.priority);
			printf("Completed: %s\n", t.complete ? "Yes" : "No");
			if (t.indefinite)
				printf("Due: No Date");
			else
				printf("Due: %s", asctime(&t.due));
			if (t.description)
				printf("Description: %s\n", t.description);
			if (t.note)
				printf("Note: %s\n", t.note);
			printf("\n");
	
			free_ToDo(&t);
		}
	
		if (port) {
			/* Close the database */
			dlp_CloseDB(sd, db);
			dlp_AddSyncLogEntry(sd, "Successfully read ToDos from Palm.\n"
						"Thank you for using pilot-link.");
			dlp_EndOfSync(sd, 0);
			pi_close(sd);
		}
		if (filename) {
			pi_file_close(pif);
		}
	}
	return 0;
}
