/*
 * money2qif.c:  Translate Palm MoneyManager database into QIF format
 *
 * Copyright (c) 1998, Rui Oliveira
 * Copyright (c) 1996, Kenneth Albanowski (original read-todos.c)
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

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-money.h"
#include "pi-dlp.h"
#include "pi-header.h"
#include "pi-userland.h"


int main(int argc, const char *argv[])
{
	int
		db,
		index,
		po_err		= -1,
		sd 		= -1;

	char 	*noteln,
		*port 		= NULL,
		*account 	= NULL;

	int match_category;

	struct 	MoneyAppInfo mai;
	pi_buffer_t *buffer;

	poptContext po;

	struct poptOption options[] = {
		USERLAND_RESERVED_OPTIONS
	        {"account", 	'a', POPT_ARG_STRING, &account, 0, "The name of the Account category in MicroMoney"},
        	POPT_TABLEEND
	};

	po = poptGetContext("money2qif", argc, argv, options, 0);
	poptSetOtherOptionHelp(po,"\n\n"
	"   Convert and sync your MicroMoney account data Quicken QIF format.\n"
	"   Please see http://www.techstop.com.my/MicroMoney.htm for more information\n"
	"   on MicroMoney.\n\n"
	"   NOTE: MicroMoney is no longer supported or supplied by Landware, and has\n"
	"   been superceded by PocketQuicken. There is no PocketQuicken conduit in\n"
	"   pilot-link.\n\n"
	"   Example argumentss:\n"
	"      -p /dev/pilot -a BankGlobal\n\n");

	if (argc < 2) {
		poptPrintUsage(po,stderr,0);
		return 1;
	}
	while ((po_err = poptGetNextOpt(po)) >= 0) {
		fprintf(stderr,"   ERROR: Unhandled option %d.\n",po_err);
		return 1;
	}

	if (po_err < -1) {
		plu_badoption(po,po_err);
	}

	if (!account) {
		fprintf(stderr, "   ERROR: You must specify an Account Category"
			" as found in MicroMoney \n");
		return -1;
	}

	sd = pilot_connect(port);
	if (sd < 0)
		goto error;

	/* Open the Money database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "MoneyDB", &db) < 0) {
		fprintf(stderr,"   ERROR: Unable to open MoneyDB on Palm.\n");
		dlp_AddSyncLogEntry(sd, "Unable to open MoneyDB.\n");
		pi_close(sd);
		goto error;
	}

	buffer = pi_buffer_new (0xffff);

	dlp_ReadAppBlock(sd, db, 0, buffer->allocated, buffer);
	unpack_MoneyAppInfo(&mai, buffer->data, buffer->used);

	match_category = plu_findcategory(&mai.category,account,PLU_CAT_NOFLAGS);

	if (index >= 0) {

		printf("!Type:Bank\n");

		for (index = 0;; index++) {
			int 	attr,
				category;
			struct 	Transaction t;

			int len =
				dlp_ReadRecordByIndex(sd, db, index, buffer, 0,
						      &attr,
						      &category);

			if (len < 0)
				break;

			if ((attr & dlpRecAttrDeleted)
			    || (attr & dlpRecAttrArchived))
				continue; 	/* Skip deleted records */

			if (match_category != category) {
				continue;
			}

			unpack_Transaction(&t, buffer->data, buffer->used);

			printf("D%02d/%02d/%2d\n", t.month, t.day,
			       t.year - 1900);
			if (t.checknum)
				printf("N%d\n", t.checknum);
			printf("T%ld.%02d\n", t.amount, t.amountc);
			printf("P%s\n", t.description);
			if (t.xfer == category)
				printf("L%s\n",
				       mai.typeLabels[(int) t.type]);
			else
				printf("L[%s]\n",
				       mai.category.name[(int) t.xfer]);
			if (strcmp(t.note, "")) {
				while ((noteln = strchr(t.note, '\n')))
					*noteln = ' ';
				printf("M%s\n", t.note);
			}
			if (t.flags & 1)
				printf("CX\n");
			printf("\n^\n");

		}
	} else {
		fprintf(stderr,"   ERROR: Category '%s' not found.\n",account);
	}

	pi_buffer_free (buffer);

	/* Close the database */
	dlp_CloseDB(sd, db);
	dlp_AddSyncLogEntry(sd, "money2qif, successfully read MoneyDB from Palm.\n"
			    "Thank you for using pilot-link.\n");
	pi_close(sd);

	return 0;

error:
	return -1;
}
