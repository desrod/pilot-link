/*
 * addresses.c:  Translate Palm address book into a generic format
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

#include "popt.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "pi-socket.h"
#include "pi-address.h"
#include "pi-dlp.h"
#include "pi-header.h"
#include "userland.h"



int main(int argc, char *argv[])
{
	int 	db,
		index,
		sd 		= -1,
		po_err		= -1;

	char 	*progname 	= argv[0];

	struct 	AddressAppInfo aai;
	poptContext po;

	pi_buffer_t *buffer;

	struct poptOption options[] = {
		USERLAND_RESERVED_OPTIONS
		POPT_TABLEEND
	} ;
        po = poptGetContext("addresses", argc, (const char **) argv, options, 0);
	poptSetOtherOptionHelp(po," [- port]\n\n"
		"   Dumps the Palm AddressDB database into a generic text output format\n\n");


        while ((po_err = poptGetNextOpt(po)) >= 0) {
		/* No arguments not handled by popt. */
		fprintf(stderr,"   ERROR: Unhandled option %d.\n",po_err);
		return 1;
        }

	if (po_err < -1) userland_badoption(po,po_err);
	sd = userland_connect();

	if (sd < 0)
		goto error;

	if (dlp_OpenConduit(sd) < 0)
		goto error_close;

	/* Open the Address book's database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "AddressDB", &db) < 0) {
		fprintf(stderr,"   ERROR: Unable to open AddressDB\n");
		dlp_AddSyncLogEntry(sd, "Unable to open AddressDB.\n");
		goto error_close;
	}

	buffer = pi_buffer_new (0xffff);

	dlp_ReadAppBlock(sd, db, 0, buffer->data, 0xffff);
	unpack_AddressAppInfo(&aai, buffer->data, 0xffff);

	for (index = 0;; index++) {
		int 	i,
			attr,
			category;

		struct 	Address addr;

		int len =
		    dlp_ReadRecordByIndex(sd, db, index, buffer, 0, &attr,
					  &category);

		if (len < 0)
			break;

		/* Skip deleted records */
		if ((attr & dlpRecAttrDeleted)
		    || (attr & dlpRecAttrArchived))
			continue;

		unpack_Address(&addr, buffer->data, buffer->used);

		printf("Category: %s\n", aai.category.name[category]);

		for (i = 0; i < 19; i++) {
			if (addr.entry[i]) {
				int l = i;

				if ((l >= entryPhone1) && (l <= entryPhone5)) {
					printf("%s: %s\n",
						aai.phoneLabels[addr.phoneLabel[l - entryPhone1]],
						addr.entry[i]);
				} else {
					printf("%s: %s\n", aai.labels[l],
						addr.entry[i]);
				}
			}
		}
		printf("\n");
		free_Address(&addr);
	}

	pi_buffer_free (buffer);

	/* Close the database */
	dlp_CloseDB(sd, db);
        dlp_AddSyncLogEntry(sd, "Successfully read addresses from Palm.\n\n"
				"Thank you for using pilot-link.\n");
	dlp_EndOfSync(sd, 0);
	pi_close(sd);
	return 0;

 error_close:
        pi_close(sd);

 error:
        return -1;
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
