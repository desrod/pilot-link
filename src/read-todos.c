/* ex: set tabstop=4 expandtab: */
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "pi-socket.h"
#include "pi-todo.h"
#include "pi-file.h"
#include "pi-header.h"
#include "userland.h"

int main(int argc, const char *argv[])
{
	int 	c,		/* switch */
		db,
		i,
		sd 		= -1;

	char
		*filename 	= NULL,
		*ptr;

	struct 	PilotUser User;
	struct 	pi_file *pif 	= NULL;
	struct 	ToDoAppInfo tai;
	unsigned char buffer[0xffff];

	pi_buffer_t *recbuf;

	poptContext po;

	struct poptOption options[] = {
		USERLAND_RESERVED_OPTIONS
	        {"file", 	'f', POPT_ARG_STRING, &filename, 0, "Save ToDO entries in <filename> instead of STDOUT"},
		POPT_TABLEEND
	};

	po = poptGetContext("read-todos", argc, argv, options, 0);
	poptSetOtherOptionHelp(po,"\n\n"
	"   Synchronize your ToDo database with your desktop machine.\n"
	"   If you use --port, the contents of your Palm's ToDo database will be written to\n"
	"   standard output in a generic text format/ Otherwise, use --file to read a todo\n"
	"   database file from disk for printing.\n\n"
	"   Example arguments:\n"
	"      -p serial:/dev/ttyUSB0 \n"
	"      -f ToDoDB.pdb\n");


	if (argc<2) {
		poptPrintUsage(po,stderr,0);
		return 1;
	}

	while ((c = poptGetNextOpt(po)) >= 0) {
		fprintf(stderr,"   ERROR: Unhandled option %d.\n",c);
		return 1;
	}

	if (c < -1) {
		plu_badoption(po,c);
	}

	if (!plu_port && !filename) {
		fprintf(stderr,"   ERROR: Specify either --port or --file.\n");
		return 1;
	}

	/* Read ToDoDB.pdb from the Palm directly */
	if (!filename) {
		sd = plu_connect();
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
	} else {
		size_t 	len;

		pif = pi_file_open(filename);
		if (!pif) {
			fprintf(stderr, "   ERROR: %s\n", strerror(errno));
			fprintf(stderr, "   Does %s exist?\n\n", filename);
			exit(EXIT_FAILURE);
		}

		pi_file_get_app_info(pif, (void *) &ptr, &len);

		memcpy(buffer, ptr, len);
	}

	unpack_ToDoAppInfo(&tai, buffer, 0xffff);

	recbuf = pi_buffer_new (0xffff);

	for (i = 0;; i++) {
		int 	attr,
			category;

		size_t	len;

		struct 	ToDo todo;

		if (!filename) {
			len =
			    dlp_ReadRecordByIndex(sd, db, i, recbuf, 0,
						  &attr, &category);

			if (len < 0)
				break;
		}
		else {
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

	if (!filename) {
		/* Close the database */
		dlp_CloseDB(sd, db);
		dlp_AddSyncLogEntry(sd, "Successfully read ToDos from Palm.\n"
					"Thank you for using pilot-link.");
		dlp_EndOfSync(sd, 0);
		pi_close(sd);

	} else {
		pi_file_close(pif);
	}
	return 0;

error_close:
	pi_close(sd);

error:
	return -1;
}
