/*
 * read-expenses.c: Sample code to translate Palm Expense database into
 *                  generic format
 *
 * Copyright (c) 1997, Kenneth Albanowski
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
#include "pi-expense.h"
#include "pi-dlp.h"
#include "pi-header.h"

int main(int argc, char *argv[])
{
	struct pi_sockaddr addr;
	struct PilotUser U;
	struct ExpenseAppInfo tai;
	struct ExpensePref tp;
	int db;
	int i;
	int ret;
	int sd;
	unsigned char buffer[0xffff];
	unsigned char buffer2[0xffff];
	char *progname = argv[0];
	char *device = argv[1];

	PalmHeader(progname);

	if (argc < 2) {
		fprintf(stderr, "   Usage: %s %s\n\n", argv[0], TTYPrompt);
		exit(2);
	}
	if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP))) {
		perror("pi_socket");
		exit(1);
	}

	addr.pi_family = PI_AF_SLP;
	strcpy(addr.pi_device, device);

	ret = pi_bind(sd, (struct sockaddr *) &addr, sizeof(addr));
	if (ret == -1) {
		fprintf(stderr, "\n   Unable to bind to port %s\n",
			device);
		perror("   pi_bind");
		fprintf(stderr, "\n");
		exit(1);
	}

	printf
	    ("   Port: %s\n\n   Please press the HotSync button now...\n",
	     device);

	ret = pi_listen(sd, 1);
	if (ret == -1) {
		fprintf(stderr, "\n   Error listening on %s\n", device);
		perror("   pi_listen");
		fprintf(stderr, "\n");
		exit(1);
	}

	sd = pi_accept(sd, 0, 0);
	if (sd == -1) {
		fprintf(stderr, "\n   Error accepting data on %s\n",
			device);
		perror("   pi_accept");
		fprintf(stderr, "\n");
		exit(1);
	}

	fprintf(stderr, "Connected...\n");

	/* Ask the pilot who it is. */
	dlp_ReadUserInfo(sd, &U);

	/* Tell user (via Palm) that we are starting things up */
	dlp_OpenConduit(sd);

	/* Note that under PalmOS 1.x, you can only read preferences before
	   the DB is opened 
	 */
	ret =
	    dlp_ReadAppPreference(sd, Expense_Creator, Expense_Pref, 1,
				  0xffff, buffer, 0, 0);

	/* Open the ToDo database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "ExpenseDB", &db) < 0) {
		puts("Unable to open ExpenseDB");
		dlp_AddSyncLogEntry(sd, "Unable to open ExpenseDB.\n");
		exit(1);
	}

	if (ret >= 0) {
		unpack_ExpensePref(&tp, buffer, 0xffff);
		i = pack_ExpensePref(&tp, buffer2, 0xffff);
		fprintf(stderr, "Orig prefs, %d bytes:\n", ret);
		dumpdata(buffer, ret);
		fprintf(stderr, "New prefs, %d bytes:\n", ret);
		dumpdata(buffer2, i);
		fprintf(stderr,
			"Expense prefs, current category %d, default category %d\n",
			tp.currentCategory, tp.defaultCategory);
		fprintf(stderr,
			"  Note font %d, Show all categories %d, Show currency %d, Save backup %d\n",
			tp.noteFont, tp.showAllCategories, tp.showCurrency,
			tp.saveBackup);
		fprintf(stderr,
			"  Allow quickfill %d, Distance unit %d, Currencies:\n",
			tp.allowQuickFill, tp.unitOfDistance);
		for (i = 0; i < 7; i++) {
			fprintf(stderr, " %d", tp.currencies[i]);
		}
		fprintf(stderr, "\n");
	}

	ret = dlp_ReadAppBlock(sd, db, 0, buffer, 0xffff);
	unpack_ExpenseAppInfo(&tai, buffer, 0xffff);
	i = pack_ExpenseAppInfo(&tai, buffer2, 0xffff);
	fprintf(stderr, "Orig length %d, new length %d, orig data:\n", ret,
		i);
	dumpdata(buffer, ret);
	fprintf(stderr, "New data:\n");
	dumpdata(buffer2, i);

	fprintf(stderr, "Expense app info, sort order %d\n",
		tai.sortOrder);
	fprintf(stderr, " Currency 1, name '%s', symbol '%s', rate '%s'\n",
		tai.currencies[0].name, tai.currencies[0].symbol,
		tai.currencies[0].rate);
	fprintf(stderr, " Currency 2, name '%s', symbol '%s', rate '%s'\n",
		tai.currencies[1].name, tai.currencies[1].symbol,
		tai.currencies[1].rate);
	fprintf(stderr, " Currency 3, name '%s', symbol '%s', rate '%s'\n",
		tai.currencies[2].name, tai.currencies[2].symbol,
		tai.currencies[2].rate);
	fprintf(stderr, " Currency 4, name '%s', symbol '%s', rate '%s'\n",
		tai.currencies[3].name, tai.currencies[3].symbol,
		tai.currencies[3].rate);

	for (i = 0;; i++) {
		struct Expense t;
		int attr;
		int category;

		int len =
		    dlp_ReadRecordByIndex(sd, db, i, buffer, 0, 0, &attr,
					  &category);

		if (len < 0)
			break;

		/* Skip deleted records */
		if ((attr & dlpRecAttrDeleted)
		    || (attr & dlpRecAttrArchived))
			continue;

		unpack_Expense(&t, buffer, len);
		ret = pack_Expense(&t, buffer2, 0xffff);
		fprintf(stderr, "Orig length %d, data:\n", len);
		dumpdata(buffer, len);
		fprintf(stderr, "New length %d, data:\n", ret);
		dumpdata(buffer2, ret);

		fprintf(stderr, "Category: %s\n",
			tai.category.name[category]);
		fprintf(stderr, "Type: %d, Payment: %d, Currency: %d\n",
			t.type, t.payment, t.currency);
		fprintf(stderr, "Amount: '%s', Vendor: '%s', City: '%s'\n",
			t.amount ? t.amount : "<None>",
			t.vendor ? t.vendor : "<None>",
			t.city ? t.city : "<None>");
		fprintf(stderr, "Attendees: '%s', Note: '%s'\n",
			t.attendees ? t.attendees : "<None>",
			t.note ? t.note : "<None>");
		fprintf(stderr, "Date: %s", asctime(&t.date));
		fprintf(stderr, "\n");

		free_Expense(&t);
	}

	/* Close the database */
	dlp_CloseDB(sd, db);

	dlp_AddSyncLogEntry(sd, "Read expenses from Palm.\n");

	pi_close(sd);
	exit(0);
}
