/*
 * read-todos.c:  Translate Palm ToDo database into generic format
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

/* 12-27-2003:
   FIXME: Add "Private" and "Delete" flags */

#include "getopt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "pi-socket.h"
#include "pi-todo.h"
#include "pi-file.h"
#include "pi-header.h"

struct option options[] = {
	{"port",        required_argument, NULL, 'p'},
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
	{"file",        required_argument, NULL, 'f'},
	{NULL,          0,                 NULL, 0}
};

static const char *optstring = "p:hvf:";

static void display_help(const char *progname)
{
	printf("   Syncronize your ToDo database with your desktop machine\n\n");
	printf("   Usage: %s -p <port> [options]\n\n", progname);
	printf("   Options:\n");
	printf("     -p, --port <port>       Use device file <port> to communicate with Palm\n");
	printf("     -h, --help              Display help information for %s\n", progname); 
	printf("     -v, --version           Display %s version information\n", progname);  
	printf("     -f, --file <filename>   Save ToDO entries in <filename> instead of STDOUT\n\n");
	printf("   Examples:\n\n");
	printf("\tRead the on-Palm ToDo database, and dump to STDOUT\n");
	printf("\t\t%s -p /dev/pilot \n\n", progname);
	printf("\tRead the ToDoDB.pdb file on disk, and dump to STDOUT\n");
	printf("\t\t%s -f ToDoDB.pdb\n\n", progname);
	printf("   By default, the contents of your Palm's ToDo database will be written to\n");
	printf("   standard output in a generic text format if the -p <port> parameters are\n");
	printf("   passed.\n\n");
	printf("   If you supply the -f <file> parameter, (omitting the -p <port>), your\n");
	printf("   ToDo database file on disk will be read and sent to STDOUT in the same\n");
	printf("   manner.\n\n");
	
	return;
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
    pi_buffer_t *recbuf;
	
	while ((c = getopt_long(argc, argv, optstring, options, NULL)) != -1) {
		switch (c) {
		case 'h':
			display_help(progname);
			return 0;
		case 'v':
			print_splash(progname);
			return 0;
		case 'p':
			port = optarg;
			filename = NULL;
			break;
		case 'f':
			filename = optarg;
			port = NULL;
			break;
		default:
			display_help(progname);
			return 0;
		}
	}
		
	/* Read ToDoDB.pdb from the Palm directly */
	if (port) {
		sd = pilot_connect(port);
		if (sd < 0)
			goto error;
	
		if (dlp_ReadUserInfo(sd, &User) < 0)
			goto error_close;
			
		/* Open the ToDo database, store access handle in db */
		if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "ToDoDB", &db) < 0) {
			puts("   Unable to open ToDoDB");
			dlp_AddSyncLogEntry(sd,
					    "   Unable to open ToDoDB.\n");
			exit(EXIT_FAILURE);
		}
	
		dlp_ReadAppBlock(sd, db, 0, buffer, 0xffff);

	/* Read ToDoDB.pdb from disk */
	} else if (filename) {
		size_t 	len;

		pif = pi_file_open(filename);
		if (!pif) {
			fprintf(stderr, "   ERROR: %s\n", strerror(errno));
			fprintf(stderr, "   Does %s exist?\n\n", filename);
			exit(EXIT_FAILURE);
		}

		pi_file_get_app_info(pif, (void *) &ptr, &len);

		memcpy(buffer, ptr, len);

	/* No arguments specified */
	} else {
		printf("ERROR: Insufficient number of arguments\n\n");
		display_help(progname);
		exit(EXIT_FAILURE);
	}

	unpack_ToDoAppInfo(&tai, buffer, 0xffff);

    recbuf = pi_buffer_new (0xffff);
    
	for (i = 0;; i++) {
		int 	attr,
			category;

		size_t	len;

		struct 	ToDo todo;

		if (port) {
			len =
			    dlp_ReadRecordByIndex(sd, db, i, recbuf, 0, 
						  &attr, &category);

			if (len < 0)
				break;
		}
		if (filename) {
			if (pi_file_read_record
			    (pif, i, (void *) &ptr, &len, &attr, &category,
			     0))
				break;

            pi_buffer_clear(recbuf);
            pi_buffer_append(recbuf, ptr, len);
		}

		/* Skip deleted records */
		if ((attr & dlpRecAttrDeleted)
		    || (attr & dlpRecAttrArchived))
			continue;

		unpack_ToDo(&todo, recbuf->data, recbuf->used);

		printf("Category: %s\n", tai.category.name[category]);
		printf("Priority: %d\n", todo.priority);
		printf("Completed: %s\n", todo.complete ? "Yes" : "No");

		if (todo.indefinite) {
			printf("Due: No Date");
		} else {
			printf("Due: %s", asctime(&todo.due));
		}

		if (todo.description)
			printf("Description: %s\n", todo.description);

		if (todo.note)
			printf("Note: %s\n", todo.note);

		printf("\n");

		free_ToDo(&todo);
	}

    pi_buffer_free (recbuf);
  
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

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
