/*
 * pilot-archive.c:  Create Archive files of Palm databases with "archived"
 * 		     records within them
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

/* Declare prototypes */
static void display_help(char *progname);
void print_splash(char *progname);
int pilot_connect(char *port);

struct option options[] = {
	{"port",        required_argument, NULL, 'p'},
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
	{NULL,          0,                 NULL, 0}
};

static const char *optstring = "p:hv";

static void display_help(char *progname)
{
	printf("   Exports any \"archived\" records on your Palm to an archive file.\n\n");
	printf("   Usage: %s -p <port>\n\n", progname);
	printf("   Options:\n");
	printf("     -p, --port <port>       Use device file <port> to communicate with Palm\n");
	printf("     -h, --help              Display help information for %s\n", progname);
	printf("     -v, --version           Display %s version information\n\n", progname);
	
	exit(0);
}

int main(int argc, char *argv[])
{
	int 	c,		/* switch */
		db,
		i,
		sd 		= -1,
		attr;
	char 	*progname 	= argv[0],
		*port 		= NULL,
                *filename       = NULL,
                *ptr;

        struct  PilotUser User;
        struct  pi_file *pif    = NULL;
        struct  ToDoAppInfo tai;

        unsigned char buffer[0xffff];

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
			break;
		}
	}
	
	sd = pilot_connect(port);
	if (sd < 0)
		goto error;

        /* Open the ToDo database, store access handle in db */
        if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "ToDoDB", &db) < 0) {
                puts("Unable to open ToDoDB");
                dlp_AddSyncLogEntry(sd,
                                    "Unable to open ToDoDB.\n");
                exit(1);
        }

        dlp_ReadAppBlock(sd, db, 0, buffer, 0xffff);
	
        unpack_ToDoAppInfo(&tai, buffer, 0xffff);

        for (i = 0;; i++) {
                int     attr,
                        category,
                        len;
                struct  ToDo t;

                if (port) {
                        len =
                            dlp_ReadRecordByIndex(sd, db, i, buffer, 0, 0,
                                                  &attr, &category);

                        if (len < 0)
                                break;
                }

		if (attr & dlpRecAttrArchived) {

	                unpack_ToDo(&t, buffer, len);

	                printf("\"Category\", ");
			printf("\"%s\", ", tai.category.name[category]);
	                printf("\"Priority\", ");
			printf("\"%d\", ", t.priority);
	                printf("\"Completed\", ");
			printf("\"%s\", ", t.complete ? "Yes" : "No");

	                if (t.indefinite) {
	                        printf("\"Due\", \"No Date\", ");
			} else {
	                        printf("\"Due\", ");
				printf("\"%s\", ", asctime(&t.due));
			}

	                if (t.description) {
	                        printf("\"Description\", ");
				printf("\"%s\", ", t.description);
			}

	                if (t.note) {
	                        printf("\"Note\", ");
				printf("\"%s\", ", t.note);
			}

	                printf("\n\n");

	                free_ToDo(&t);
			}
	        }

	/* Close the database */
	dlp_CloseDB(sd, db);
	dlp_AddSyncLogEntry(sd, "Successfully archived records from\n"
				"the ToDo database on your Palm\n"
				"Thank you for using pilot-link\n");
	dlp_EndOfSync(sd, 0);
	pi_close(sd);

	return 0;

 error:
	return -1;
}
