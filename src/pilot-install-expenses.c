/*
 * install-expense.c: Palm expense installer
 *
 * Copyright (C) Boisy G. Pitre, year unknown - 2001 probably.
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
#include "pi-dlp.h"
#include "pi-expense.h"
#include "pi-userland.h"




int main(int argc, const char *argv[])
{
	int 	db,
		sd		= -1,
		i,
		l,
		category,
		po_err		= -1,
		replace_category = 0;

	char
		*category_name 	= NULL,
		*expenseType	= NULL,
		*paymentType	= NULL;
	size_t size;
	int found;

	unsigned char buf[0xffff];
	unsigned char *b;
	pi_buffer_t *appblock;

	struct 	PilotUser User;
	struct 	ExpenseAppInfo eai;
	struct 	Expense theExpense;

	poptContext po;

	struct poptOption options[] = {
		USERLAND_RESERVED_OPTIONS
	        {"ptype", 	't', POPT_ARG_STRING, &paymentType, 0,"Payment type (Cash, Check, etc.)"},
        	{"etype", 	'e', POPT_ARG_STRING, &expenseType, 0, "Expense type (Airfare, Hotel, etc.)"},
	        {"amount", 	'a', POPT_ARG_STRING, &theExpense.amount, 0, "Payment amount"},
        	{"vendor", 	'V', POPT_ARG_STRING, &theExpense.vendor, 0, "Expense vendor name (Joe's Restaurant)"},
	        {"city", 	'i', POPT_ARG_STRING, &theExpense.city, 0, "Location/city for this expense entry"},
        	{"guests", 	'g', POPT_ARG_STRING, &theExpense.attendees, 0, "Number of guests for this expense entry","NUMBER"},
	        {"note", 	'n', POPT_ARG_STRING, &theExpense.note, 0, "Notes for this expense entry"},
        	{"category", 	'c', POPT_ARG_STRING, &category_name, 0, "Install entry into this category", "CATEGORY" },
                {"replace", 	0, POPT_ARG_VAL, &replace_category, 1, "Replace all entries in category by this one"},
        	POPT_TABLEEND
	};

	/* Zero 'em out to be sure. */
	theExpense.amount=theExpense.vendor=theExpense.city=
	theExpense.attendees=theExpense.note = NULL ;

	po = poptGetContext("install-expenses", argc, argv, options, 0);
	poptSetOtherOptionHelp(po,"\n\n"
		"   Install Expense application entries to your Palm device\n\n"
		"   Example arguments:\n"
		"     %s -p /dev/pilot -c Unfiled -t Cash -e Meals -a 10.00 -V McDonalds \n"
		"                      -g 21 -l \"San Francisco\" -N \"This is a note\"\n\n");

	if (argc < 2) {
		poptPrintUsage(po,stderr,0);
		return 1;
        }

	 while ((po_err = poptGetNextOpt(po)) >= 0) {
		fprintf(stderr,"   ERROR: Unhandled option %d.\n",po_err);
		return 1;
	}

	theExpense.type = etBus;
	found = 0;
	for (i = 0; expenseType && ExpenseTypeNames[i] != NULL; i++)
	{
		if (strcasecmp(expenseType, ExpenseTypeNames[i]) == 0)
		{
			theExpense.type = i;
			found = 1;
			break;
		}
	}
	if (!found) {
		fprintf(stderr,"   WARNING: Expense type '%s' not recognized, using 'Bus Fare'.\n",expenseType);
	}

	theExpense.payment = epCash;
	found = 0;
	for (i = 0; paymentType && ExpensePaymentNames[i] != NULL; i++)
	{
		if (strcasecmp(paymentType, ExpensePaymentNames[i]) == 0)
		{
			theExpense.payment = i;
			found = 1;
			break;
		}
	}
	if (!found) {
		fprintf(stderr,"   WARNING: Payment type '%s' not recognized, using 'Cash'.\n", paymentType);
	}

	if (replace_category && (!category_name)) {
		fprintf(stderr,
			"   ERROR: category required when specifying replace\n");
		return 1;
	}


	if (!(theExpense.amount || theExpense.vendor || theExpense.city ||
		theExpense.attendees ||theExpense.note)) {
		fprintf(stderr,"   ERROR: Must specify at least one of amount, vendor, city, attendees or note.\n");
		return 1;
	}

	sd = plu_connect();
	if (sd < 0)
		goto error;

	if (dlp_OpenConduit(sd) < 0)
		goto error_close;

	dlp_ReadUserInfo(sd, &User);
	dlp_OpenConduit(sd);

	/* Open the Expense's database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, Expense_DB, &db) < 0) {
		fprintf(stderr,"   ERROR: Unable to open ExpenseDB on Palm.");
		dlp_AddSyncLogEntry(sd, "Unable to open ExpenseDB.\n");
		goto error_close;
	}

	appblock = pi_buffer_new(0xffff);
	l = dlp_ReadAppBlock(sd, db, 0, 0xffff, appblock);
	unpack_ExpenseAppInfo(&eai, appblock->data, l);
	pi_buffer_free(appblock);

	category = 0;	/* unfiled */
	if (category_name) {
		category = plu_findcategory(&eai.category,category_name,
			PLU_CAT_CASE_INSENSITIVE | PLU_CAT_WARN_UNKNOWN);
		if (category < 0) {
			goto error_close;
		}

		if (replace_category) {
			dlp_DeleteCategory(sd, db, category);
		}

	}

		theExpense.currency 	= 0;

		if (!theExpense.amount) {
			theExpense.amount = "";
		}
		if (!theExpense.vendor) {
			theExpense.vendor = "";
		}
		if (!theExpense.city) {
			theExpense.city = "";
		}
		if (!theExpense.attendees) {
			theExpense.attendees 	= "";
		}
		if (!theExpense.note) {
			theExpense.note = "";
		}

		b = buf;

		/* Date */
		*(b++) 	= 0xc3;
		*(b++) 	= 0x45;

		*(b++) 	= theExpense.type;
		*(b++) 	= theExpense.payment;
		*(b++) 	= theExpense.currency;
		*(b++) 	= 0x00;

		strcpy(b, theExpense.amount);
		b += strlen(theExpense.amount) + 1;

		strcpy(b, theExpense.vendor);
		b += strlen(theExpense.vendor) + 1;

		strcpy(b, theExpense.city);
		b += strlen(theExpense.city) + 1;

		strcpy(b, theExpense.attendees);
		b += strlen(theExpense.attendees) + 1;

		strcpy(b, theExpense.note);
		b += strlen(theExpense.note) + 1;

		size = b - buf;
		dlp_WriteRecord(sd, (unsigned char)db, 0, 0, category,
				(unsigned char *)buf, size, 0);

	/* Close the database */
	dlp_CloseDB(sd, db);

	/* Tell the user who it is, with a different PC id. */
	User.lastSyncPC 	= 0x00010000;
	User.successfulSyncDate = time(NULL);
	User.lastSyncDate 	= User.successfulSyncDate;
	dlp_WriteUserInfo(sd, &User);

	dlp_AddSyncLogEntry(sd, "Wrote expense entry to Palm.\n");
	dlp_EndOfSync(sd, 0);
	pi_close(sd);
	poptFreeContext(po);
	return 0;

error_close:
	pi_close(sd);

error:
	return -1;
}
