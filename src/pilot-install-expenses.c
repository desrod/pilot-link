/* 
 * install-expense.c: Palm expense installer
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
#include "pi-dlp.h"
#include "pi-expense.h"

/* Declare prototypes */
static void display_help(char *progname);
void display_splash(char *progname);
int pilot_connect(char *port);

struct option options[] = {
	{"port",        required_argument, NULL, 'p'},
	{"help",        no_argument,       NULL, 'h'},
        {"version",     no_argument,       NULL, 'v'},
	{"ptype",       required_argument, NULL, 't'},
	{"etype",       required_argument, NULL, 'e'},
	{"amount",      required_argument, NULL, 'a'},
	{"vendor",      required_argument, NULL, 'V'},
	{"city",        required_argument, NULL, 'i'},
	{"guests",	required_argument, NULL, 'g'},
	{"note",        required_argument, NULL, 'n'},
	{"category",    required_argument, NULL, 'c'},
        {"replace",     required_argument, NULL, 'r'},
	{NULL,          0,                 NULL, 0}
};

static const char *optstring = "p:hvt:w:e:a:V:i:g:n:c:rt";

char *paymentTypes[] = 
{
	"amex",
	"cash",
	"check",
	"creditcard",
	"mastercard",
	"prepaid",
	"visa",
	"unfiled",
	NULL
};


char *expenseTypes[] = 
{
	"airfare", 
	"breakfast",
	"bus",
	"businessmeals",
	"carrental",
	"dinner",
	"entertainment",
	"fax",
	"gas",
	"gifts",
	"hotel",
	"incidentals",
	"laundry",
	"limo",
	"lodging",
	"lunch",
	"mileage",
	"other",
	"parking",
	"postage",
	"snack",
	"subway",
	"supplies",
	"taxi",
	"telephone",
	"tips",
	"tolls",
	"train",
	NULL
};

static void display_help(char *progname)
{
	printf("   Install Expense application entries to your Palm device\n\n");
	printf("   Usage: %s [-qrt] [-c category] -p <port> file [file] ...\n", progname);
	printf("   Options:\n");
	printf("     -p, --port <port>       Use device file <port> to communicate with Palm\n");
	printf("     -h, --help              Display help information for %s\n", progname); 
	printf("     -v, --version           Display %s version information\n", progname);  
	printf("     -t, --ttype <ptype>     Payment type (Cash, Check, etc.)\n");
	printf("     -e, --etype <etype>     Expense type (Airfare, Hotel, etc.)\n");
	printf("     -a, --amount [amount]   Payment amount\n");
	printf("     -V, --vendor [vendor]   Expense vendor name (Joe's Restaurant)\n");
	printf("     -g, --guests <guests>   Nuber of guests for this expense entry\n");
	printf("     -i, --city [city]       Location/city for this expense entry\n");
	printf("     -n, --note [note]       Notes for this expense entry\n");
	printf("     -c, --category <cat>    Install entry into this category\n\n");
	printf("     -r, --replace <cat>     Replace entry in this category\n\n");
	printf("   Example:\n");
	printf("     %s -p /dev/pilot -c Unfiled -t Cash -e Meals -a 10.00 -V McDonalds \n");
	printf("                      -g 21 -l \"San Francisco\" -N \"This is a note\"\n\n", progname);
	exit(0);
}

int main(int argc, char *argv[])
{
	int 	db,
		sd		= -1,
		i,
		l,
		category,
		c,		/* switch */
		replace_category, 
		add_title;
	
	char 	*port		= NULL,
		buf[0xffff],
		*progname 	= argv[0],
		*category_name 	= NULL;
	
	struct 	pi_sockaddr addr;
	struct 	PilotUser User;
	struct 	ExpenseAppInfo mai;
	struct 	Expense theExpense;

	while ((c = getopt_long(argc, argv, optstring, options, NULL)) != -1) {
		switch (c) {
                case 'h':
                        display_help(progname);
                        exit(0);
		case 'e':
			theExpense.type = etBus;
			for (i = 0; expenseTypes[i] != NULL; i++)
			{
				if (strcasecmp(optarg, expenseTypes[i]) == 0)
				{
					theExpense.type = i;
					break;
				}
			}
			break;
		case 't':
			theExpense.payment = epCash;
			for (i = 0; paymentTypes[i] != NULL; i++)
			{
				if (strcasecmp(optarg, paymentTypes[i]) == 0)
				{
					theExpense.payment = i;
					break;
				}
			}
			break;
		case 'g':
			theExpense.attendees = optarg;
			break;
		case 'a':
			theExpense.amount = optarg;
			break;
		case 'V':
			theExpense.vendor = optarg;
			break;
		case 'i':
			theExpense.city = optarg;
			break;
		case 'n':
			theExpense.note = optarg;
			break;
		case 'c':
			category_name = optarg;
			break;
		case 'p':
			port = optarg;
			break;
		case 'r':
			replace_category++;
			break;
                case 'v':
                        display_splash(progname);
                        exit(0);
		}
	}

	argc -= optind;
	argv += optind;

	if (replace_category && !category_name) {
		fprintf(stderr,
			"%s: expense category required when specifying replace\n",
			progname);
		display_help(progname);
	}

	sd = pilot_connect(port);
	if (sd < 0)
		goto error;

	if (dlp_OpenConduit(sd) < 0)
		goto error_close;
	
	dlp_ReadUserInfo(sd, &User);
	dlp_OpenConduit(sd);

	/* Open the Expense's database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, Expense_DB, &db) < 0) {
		puts("Unable to open ExpenseDB");
		dlp_AddSyncLogEntry(sd, "Unable to open ExpenseDB.\n");
		exit(1);
	}

	l = dlp_ReadAppBlock(sd, db, 0, (unsigned char *) buf, 0xffff);
	unpack_ExpenseAppInfo(&mai, buf, l);

	if (category_name) {
		category = -1;	/* invalid category */
		for (i = 0; i < 16; i++)
			if (!strcasecmp
			    (mai.category.name[i], category_name)) {
				category = i;
				break;
			}
		if (category < 0) {
			fprintf(stderr,
				"%s: category %s not found on Palm\n",
				progname, category_name);
			exit(2);
		}

		if (replace_category)
			dlp_DeleteCategory(sd, db, category);

	} else {
		int 	size;
		char 	buff[256], 
			*b;

		category 		= 0;	/* unfiled */
		theExpense.currency 	= 0;
		theExpense.attendees 	= "";

		b = buff;

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

		size = b - buff;
		dlp_WriteRecord(sd, (unsigned char)db, 0, 0, category,
				(unsigned char *)buff, size, 0);
	}

	/* Close the database */
	dlp_CloseDB(sd, db);

	/* Tell the user who it is, with a different PC id. */
	User.lastSyncPC 	= 0x00010000;
	User.successfulSyncDate = time(NULL);
	User.lastSyncDate 	= User.successfulSyncDate;
	dlp_WriteUserInfo(sd, &User);

	dlp_AddSyncLogEntry(sd, "Wrote memo(s) to Palm.\n");

	/* All of the following code is now unnecessary, but harmless */
	dlp_EndOfSync(sd, 0);
	pi_close(sd);

	return 0;

 error_close:
	pi_close(sd);
	
 error:
	return -1;
}

