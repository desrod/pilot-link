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

static void display_help(char *progname);

struct option options[] = {
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
	{"port",        required_argument, NULL, 'p'},
	{"file",        required_argument, NULL, 'f'},
	{NULL,          0,                 NULL, 0}
};

static const char *optstring = "hvp:f:";

static void display_help(char *progname)
{
	printf("   Syncronize your ToDo database with your desktop or server machine\n");
	printf("   Usage: %s -p /dev/pilot [options]\n\n" "   Options:\n", progname);
	printf("     -p <port>      Use device file <port> to communicate with Palm\n");
	printf("     -f <filename>  Save ToDO entries in <filename> instead of writing to STDOUT\n");
	printf("     -h             Display this information\n\n");
	printf("   Examples: %s -p /dev/pilot -d ~/Palm\n\n", progname);
	printf("   By default, the contents of your Palm's memo database will be written to\n");
	printf("   standard output as a standard Unix mailbox (mbox-format) file, with each\n");
	printf("   memo as a separate message.  The subject of each message will be the\n");
	printf("   category.\n\n");
	
	exit(0);
}

int main(int argc, char *argv[])
{
	int 	c,		/* switch */
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
	
	while ((c = getopt_long(argc, argv, optstring, options, NULL)) != -1) {
		switch (c) {
		  case 'h':
			  display_help(progname);
			  exit(0);
                  case 'v':
			  print_splash(progname);
			  return 0;
		  case 'p':
			  port = optarg;
			  filename = NULL;
			  break;
		  case 'f':
			  filename = optarg;
			  break;
		}
	}
		
        sd = pilot_connect(port);
        if (sd < 0)
                goto error;   

        if (dlp_ReadUserInfo(sd, &User) < 0)
                goto error_close;
		
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
	} else if (filename) {
		pi_file_close(pif);
	}
	return 0;
	
error_close:
        pi_close(sd);

error:
        return -1;
}
