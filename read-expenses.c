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
#include "pi-socket.h"
#include "pi-expense.h"
#include "pi-dlp.h"
#include "pi-header.h"

int pilot_connect(const char *port);
static void Help(char *progname);

/* Not used yet, getopt_long() coming soon! 
struct option options[] = {
	{"help",        no_argument,       NULL, 'h'},
	{"port",        required_argument, NULL, 'p'},
	{NULL,          0,                 NULL, 0}
};
*/

static const char *optstring = "hp:";

static void Help(char *progname)
{
	printf("   Export Palm Expense application database data into text format\n"
	       "   Usage: %s -p <port>\n\n"
	       "   Options:\n"
	       "     -p <port>         Use device file <port> to communicate with Palm\n"
	       "     -h                Display this information\n\n"
	       "   Examples: %s -p /dev/pilot\n\n", progname, progname);
	return;
}

int main(int argc, char *argv[])
{
	int 	count,
		db,
		idx,
		ret,
		sd 		= -1;
	char 	*progname 	= argv[0],
		*port 		= NULL;
	unsigned char buffer[0xffff];
	unsigned char buffer2[0xffff];
	struct 	PilotUser User;
	struct 	ExpenseAppInfo tai;
	struct 	ExpensePref tp;
		
	while ((count = getopt(argc, argv, optstring)) != -1) {
		switch (count) {
		  case 'h':
			  Help(progname);
			  exit(0);
		  case 'p':
			  port = optarg;
			  break;
		  default:
		}
	}

	if (argc < 2 && !getenv("PILOTPORT")) {
		PalmHeader(progname);
	} else if (port == NULL && getenv("PILOTPORT")) {
		port = getenv("PILOTPORT");
	}

	if (port == NULL && argc > 1) {
		printf
		    ("\nERROR: At least one command parameter of '-p <port>' must be set, or the\n"
		     "environment variable $PILOTPORT must be used if '-p' is omitted or missing.\n");
		exit(1);
	} else if (port != NULL) {
		sd = pilot_connect(port);

		/* Did we get a valid socket descriptor back? */
		if (dlp_OpenConduit(sd) < 0) {
			exit(1);
		} else {

			/* Ask the pilot who it is. */
			dlp_ReadUserInfo(sd, &User);
		
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
				printf("Unable to open ExpenseDB");
				dlp_AddSyncLogEntry(sd, "Unable to open ExpenseDB.\n");
				exit(1);
			}
		
			if (ret >= 0) {
				unpack_ExpensePref(&tp, buffer, 0xffff);
				idx = pack_ExpensePref(&tp, buffer2, 0xffff);
#ifdef DEBUG
				fprintf(stderr, "Orig prefs, %d bytes:\n", ret);
				dumpdata(buffer, ret);
				fprintf(stderr, "New prefs, %d bytes:\n", ret);
				dumpdata(buffer2, idx);
#endif
				printf("Expense prefs, current category %d, default category %d\n",
					tp.currentCategory, tp.defaultCategory);
				printf("  Note font %d, Show all categories %d, Show currency %d, Save backup %d\n",
					tp.noteFont, tp.showAllCategories, tp.showCurrency,
					tp.saveBackup);
				printf("  Allow quickfill %d, Distance unit %d\n\n",
					tp.allowQuickFill, tp.unitOfDistance);
				printf("Currencies:\n");
				for (idx = 0; idx < 7; idx++) {
					fprintf(stderr, "  %d", tp.currencies[idx]);
				}
				printf("\n\n");
			}
		
			ret = dlp_ReadAppBlock(sd, db, 0, buffer, 0xffff);
			unpack_ExpenseAppInfo(&tai, buffer, 0xffff);
#ifdef DEBUG
			idx = pack_ExpenseAppInfo(&tai, buffer2, 0xffff);
			printf("Orig length %d, new length %d, orig data:\n", ret, idx);
			dumpdata(buffer, ret);
			printf("New data:\n");
			dumpdata(buffer2, idx);
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
		
			for (idx = 0;; idx++) {
				int 	attr,
					category;
				struct Expense t;
		
				int len =
				    dlp_ReadRecordByIndex(sd, db, idx, buffer, 0, 0, &attr,
							  &category);
		
				if (len < 0)
					break;
		
				/* Skip deleted records */
				if ((attr & dlpRecAttrDeleted)
				    || (attr & dlpRecAttrArchived))
					continue;
		
				unpack_Expense(&t, buffer, len);
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
		}
	}
	/* Close the database */
	dlp_CloseDB(sd, db);

	dlp_AddSyncLogEntry(sd, "Successfully read Expenses from Palm.\n"
				"Thank you for using pilot-link\n");
	pi_close(sd);
	exit(0);
}