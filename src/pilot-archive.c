/*
 * pilot-archive.c:  Output "Archived" records in human-readable format to
 *                   STDOUT from ToDo application
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

#include <stdio.h>
#include <stdlib.h>

#include "pi-source.h"
#include "pi-todo.h"
#include "pi-dlp.h"
#include "pi-userland.h"

int main(int argc, const char *argv[])
{
	fprintf(stderr,"   DEPRECATED: Use read-todos -w --archived instead.\n");
	return 1;

#if 0
	int 	c,	/* switch */
	 	db,
		i,
		sd = -1;

	struct 	ToDoAppInfo tai;
	pi_buffer_t *buffer;

	poptContext po;

	struct poptOption options[] = {
		USERLAND_RESERVED_OPTIONS
		POPT_TABLEEND
	};

	po = poptGetContext("pilot-archive", argc, argv, options, 0);
	poptSetOtherOptionHelp(po,"\n\n"
	"   Exports any ToDo records marked as \"Archived\" on your Palm to\n"
	"   human-readable format\n\n");

	if (argc < 2) {
		poptPrintUsage(po,stderr,0);
		return 1;
	}
	while ((c = poptGetNextOpt(po)) >= 0) {
	}
	if (c < -1) {
		plu_badoption(po,c);
	}

	sd = plu_connect();
	if (sd < 0)
		goto error;

	/* Open the ToDo database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "ToDoDB", &db) < 0) {
		fprintf(stderr,"   ERROR: Unable to open ToDoDB on Palm.\n");
		dlp_AddSyncLogEntry(sd, "Unable to open ToDoDB.\n");
		pi_close(sd);
		goto error;
	}

	buffer = pi_buffer_new (0xffff);

	dlp_ReadAppBlock(sd, db, 0, buffer->allocated, buffer);

	unpack_ToDoAppInfo(&tai, buffer->data, buffer->used);

	for (i = 0;; i++) {
		int attr, category, len = 0;
		struct ToDo todo;

		len = dlp_ReadRecordByIndex(sd, db, i, buffer, 0,
			&attr, &category);

		if (len < 0)
			break;

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
#endif
}
