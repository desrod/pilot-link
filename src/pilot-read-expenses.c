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
#include "pi-expense.h"
#include "pi-dlp.h"
#include "pi-header.h"
#include "pi-userland.h"

int main(int argc, const char *argv[])
{
	int 	db,
		i,
		ret,
		c		= -1,
		sd 		= -1;
	enum { mode_none, mode_write = 257 } run_mode = mode_none;

	char buffer[0xffff];
	char buffer2[0xffff];
	pi_buffer_t	*recbuf,
		*appblock;

	struct 	PilotUser User;
	struct 	ExpenseAppInfo tai;
	struct 	ExpensePref tp;

	poptContext po;

	struct poptOption options[] = {
		USERLAND_RESERVED_OPTIONS
		{"write",'w', POPT_ARG_NONE,NULL,mode_write,"Write data"},
	        POPT_TABLEEND
	};

	po = poptGetContext("read-expenses", argc, argv, options, 0);
	poptSetOtherOptionHelp(po,"\n\n"
		"   Export Palm Expense application database data into text format\n\n");

	if ((argc < 2)) {
		poptPrintUsage(po,stderr,0);
		return 1;
	}

	while ((c = poptGetNextOpt(po)) >= 0) {
		switch(c) {
		case mode_write :
			if (run_mode == mode_none) {
				run_mode = c;
			} else {
				if (c != run_mode) {
					fprintf(stderr,"   ERROR: Specify exactly one of -w.\n");
					return 1;
				}
			}
			break;
		default:
			fprintf(stderr,"   ERROR: Unhandled option %d.\n",c);
			return 1;
		}
	}

	if (c < -1) {
		plu_badoption(po,c);
	}
	if (mode_none == run_mode) {
		fprintf(stderr,"   ERROR: Specify --write (-w) to output data.\n");
		return 1;
	}

        sd = plu_connect();
        if (sd < 0)
                goto error;

        if (dlp_ReadUserInfo(sd, &User) < 0) {
                goto error_close;
	}


	/* Note that under PalmOS 1.x, you can only read preferences before
	   the DB is opened
	 */
	ret = dlp_ReadAppPreference(sd, Expense_Creator, Expense_Pref, 1,
				  0xffff, buffer, 0, 0);

	/* Open the Expense database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "ExpenseDB", &db) < 0) {
		fprintf(stderr,"   ERROR: Unable to open ExpenseDB on Palm.\n");
		dlp_AddSyncLogEntry(sd, "Unable to open ExpenseDB.\n");
		goto error_close;
	}

	if (ret >= 0) {
		unpack_ExpensePref(&tp, buffer, 0xffff);
		i = pack_ExpensePref(&tp, buffer2, 0xffff);

#ifdef DEBUG
		fprintf(stderr, "Orig prefs, %d bytes:\n", ret);
		dumpdata(buffer, ret);
		fprintf(stderr, "New prefs, %d bytes:\n", i);
		dumpdata(buffer2, i);
#endif
		printf("Expense prefs, current category %d, default currency %d\n",
			tp.currentCategory, tp.defaultCurrency);
		printf("  Attendee font %d, Note font %d, Show all categories %d, Show currency %d, Save backup %d\n",
			tp.attendeeFont, tp.noteFont, tp.showAllCategories, tp.showCurrency,
			tp.saveBackup);
		printf("  Allow quickfill %d, Distance unit %d\n\n",
			tp.allowQuickFill, tp.unitOfDistance);
		printf("Currencies:\n");
		for (i = 0; i < 5; i++) {
			fprintf(stderr, "  %d", tp.currencies[i]);
		}
		printf("\n\n");
	}

	appblock = pi_buffer_new(0xffff);
	ret = dlp_ReadAppBlock(sd, db, 0, 0xffff, appblock);
	unpack_ExpenseAppInfo(&tai, appblock->data, appblock->used);
	pi_buffer_free(appblock);
#ifdef DEBUG
	i = pack_ExpenseAppInfo(&tai, buffer2, 0xffff);
	printf("Orig length %d, new length %d, orig data:\n", ret, i);
	dumpdata(buffer, ret);
	printf("New data:\n");
	dumpdata(buffer2, i);
#endif
	printf("Expense app info, sort order %d\n", tai.sortOrder);
	printf(" Currency 1, name '%s', symbol '%s', rate '%s'\n",
		tai.currencies[0].name, tai.currencies[0].symbol,
		tai.currencies[0].rate);
	printf(" Currency 2, name '%s', symbol '%s', rate '%s'\n",
		tai.currencies[1].name, tai.currencies[1].symbol,
		tai.currencies[1].rate);
	printf(" Currency 3, name '%s', symbol '%s', rate '%s'\n",
		tai.currencies[2].name, tai.currencies[2].symbol,
		tai.currencies[2].rate);
	printf(" Currency 4, name '%s', symbol '%s', rate '%s'\n\n",
		tai.currencies[3].name, tai.currencies[3].symbol,
		tai.currencies[3].rate);

	recbuf = pi_buffer_new (0xffff);

	for (i = 0;; i++) {
		int 	attr,
			category;
		struct Expense t;

		int len =
		    dlp_ReadRecordByIndex(sd, db, i, recbuf, 0, &attr,
					  &category);

		if (len < 0)
			break;

		/* Skip deleted records */
		if ((attr & dlpRecAttrDeleted)
		    || (attr & dlpRecAttrArchived))
			continue;

		unpack_Expense(&t, recbuf->data, recbuf->used);
		ret = pack_Expense(&t, buffer2, 0xffff);
#ifdef DEBUG
		fprintf(stderr, "Orig length %d, data:\n", len);
		dumpdata(buffer, len);
		fprintf(stderr, "New length %d, data:\n", ret);
		dumpdata(buffer2, ret);
#endif
		printf("Category: %s\n", tai.category.name[category]);
		printf("  Type: %s (%3d)\n  Payment: %s (%3d)\n  Currency: %3d\n",
			ExpenseTypeNames[t.type],t.type,
			ExpensePaymentNames[t.payment],t.payment,
			t.currency);
		printf("  Amount: %s\n  Vendor: %s\n  City: %s\n",
			t.amount ? t.amount : "<None>",
			t.vendor ? t.vendor : "<None>",
			t.city ? t.city : "<None>");
		printf("  Attendees: %s\n  Note: %s\n",
			t.attendees ? t.attendees : "<None>",
			t.note ? t.note : "<None>");
		printf("  Date: %s", asctime(&t.date));
		printf("\n");

		free_Expense(&t);
	}
	pi_buffer_free(recbuf);

	/* Close the database */
	dlp_CloseDB(sd, db);

	dlp_AddSyncLogEntry(sd, "Successfully read Expenses from Palm.\n"
				"Thank you for using pilot-link\n");
	pi_close(sd);
	return 0;

error_close:
        pi_close(sd);

error:
        return -1;
}
