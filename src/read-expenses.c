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

#include "getopt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pi-source.h"
#include "pi-expense.h"
#include "pi-dlp.h"
#include "pi-header.h"

struct option options[] = {
	{"port",        required_argument, NULL, 'p'},
	{"help",        no_argument,       NULL, 'h'},
	{"version",	no_argument,       NULL, 'v'},
	{NULL,          0,                 NULL, 0}
};

static const char *optstring = "p:hv";

static void display_help(const char *progname)
{
	printf("   Export Palm Expense application database data into text format\n\n");
	printf("   Usage: %s -p <port>\n\n", progname);
	printf("   Options:\n");
	printf("     -p, --port <port>       Use device file <port> to communicate with Palm\n");
	printf("     -h, --help              Display help information for %s\n", progname);
	printf("     -v, --version           Display %s version information\n\n", progname);
	printf("   Examples: %s -p /dev/pilot\n\n", progname);

	return;
}

int main(int argc, char *argv[])
{
	int 	c,		/* switch */
		db,
		i,
		ret,
		sd 		= -1;

	char 	*progname 	= argv[0],
		*port 		= NULL;

	char buffer[0xffff];
	char buffer2[0xffff];
	pi_buffer_t *recbuf;

	struct 	PilotUser User;
	struct 	ExpenseAppInfo tai;
	struct 	ExpensePref tp;
		
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
		default:
			display_help(progname);
			return 0;
		}
	}

        sd = pilot_connect(port);
        if (sd < 0)
                goto error;

        if (dlp_ReadUserInfo(sd, &User) < 0)
                goto error_close;

	
	/* Note that under PalmOS 1.x, you can only read preferences before
	   the DB is opened 
	 */
	ret = dlp_ReadAppPreference(sd, Expense_Creator, Expense_Pref, 1,
				  0xffff, buffer, 0, 0);
		
	/* Open the Expense database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "ExpenseDB", &db) < 0) {
		printf("Unable to open ExpenseDB");
		dlp_AddSyncLogEntry(sd, "Unable to open ExpenseDB.\n");
		exit(EXIT_FAILURE);
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
		
	ret = dlp_ReadAppBlock(sd, db, 0, buffer, 0xffff);
	unpack_ExpenseAppInfo(&tai, buffer, 0xffff);
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
		printf("  Type: %3d\n  Payment: %3d\n  Currency: %3d\n",
			t.type, t.payment, t.currency);
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
