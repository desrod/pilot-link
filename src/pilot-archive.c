/*
 * pilot-archive.c:  Output "Archived" records in CSV format to STDOUT
 *                   from ToDo application
 *
 * Copyright (c) 2002, David A. Desrosiers
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

#include "pi-source.h"
#include "pi-todo.h"
#include "pi-dlp.h"

/* Declare prototypes */
static void display_help(const char *progname);
void print_splash(const char *progname);
int pilot_connect(const char *porg);

static void display_help(const char *progname)
{
	printf("   Exports any records marked as \"Archived\" on your Palm to CSV format\n\n");
	printf("   Usage: %s -p <port>\n\n", progname);
	printf("   Options:\n");
	printf("     -p, --port <port>       Use device file <port> to communicate with Palm\n");
	printf("     -h, --help              Display help information for %s\n", progname);
	printf("     -v, --version           Display %s version information\n\n", progname);

	return;
}

int main(int argc, const char *argv[])
{
	int 	c,	/* switch */
	 	db, 
		i, 
		sd = -1;

	const char
                *progname 	= argv[0], 
		*port 		= NULL;

	struct 	ToDoAppInfo tai;
	pi_buffer_t *buffer;

	poptContext po;
	
	struct poptOption options[] = {
                {"port",	'p', POPT_ARG_STRING, &port, 0,  "Use device <port> to communicate with Palm"},
                {"help",	'h', POPT_ARG_NONE, NULL,   'h', "Display this information"},
                {"version",	'v', POPT_ARG_NONE, NULL,   'v', "Display version information"},
                POPT_AUTOHELP
                { NULL, 0, 0, NULL, 0 }
	};

	po = poptGetContext("pilot-archive", argc, argv, options, 0);

	while ((c = poptGetNextOpt(po)) >= 0) {
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

	sd = pilot_connect(port);
	if (sd < 0)
		goto error;

	/* Open the ToDo database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "ToDoDB", &db) < 0) {
		puts("Unable to open ToDoDB");
		dlp_AddSyncLogEntry(sd, "Unable to open ToDoDB.\n");
		exit(EXIT_FAILURE);
	}

	buffer = pi_buffer_new (0xffff);

	dlp_ReadAppBlock(sd, db, 0, buffer->data, 0xffff);

	unpack_ToDoAppInfo(&tai, buffer->data, 0xffff);

	for (i = 0;; i++) {
		int attr, category, len = 0;
		struct ToDo todo;

		if (port) {
			len = dlp_ReadRecordByIndex(sd, db, i, buffer, 0, 
				&attr, &category);

			if (len < 0)
				break;
		}

		/* Only if records are marked as "Archive to Desktop" */
		if (attr & dlpRecAttrArchived) {

			unpack_ToDo(&todo, buffer->data, buffer->used);

			printf("\"Category\", ");
			printf("\"%s\", ", tai.category.name[category]);
			printf("\"Priority\", ");
			printf("\"%d\", ", todo.priority);
			printf("\"Completed\", ");
			printf("\"%s\", ", todo.complete ? "Yes" : "No");

			if (todo.indefinite) {
				printf("\"Due\", \"No Date\", ");
			} else {
				printf("\"Due\", ");
				printf("\"%s\", ", asctime(&todo.due));
			}

			if (todo.description) {
				printf("\"Description\", ");
				printf("\"%s\", ", todo.description);
			}

			if (todo.note) {
				printf("\"Note\", ");
				printf("\"%s\", ", todo.note);
			}

			printf("\n\n");

			free_ToDo(&todo);
		}
	}

	pi_buffer_free (buffer);

	dlp_CloseDB(sd, db);
	dlp_AddSyncLogEntry(sd, "Successfully printed archived records\n"
				"from the ToDo database on your Palm\n"
				"handheld.\n\n"
			    "Thank you for using pilot-link\n");
	dlp_EndOfSync(sd, 0);
	pi_close(sd);

	return 0;

      error:
	return -1;
}
