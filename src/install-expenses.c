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

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-expense.h"

int pilot_connect(const char *port);
static void Help(char *progname);

struct option options[] = {
	{"help",        no_argument,       NULL, 'h'},
	{"port",        required_argument, NULL, 'p'},
	{"ttype",       required_argument, NULL, 't'},
	{"who",         required_argument, NULL, 'w'},
	{"etype",       required_argument, NULL, 'e'},
	{"amount",      required_argument, NULL, 'a'},
	{"vendor",      required_argument, NULL, 'v'},
	{"city",        required_argument, NULL, 'i'},
	{"note",        required_argument, NULL, 'n'},
	{NULL,          0,                 NULL, 0}
};

static const char *optstring = "hp:t:w:e:a:v:i:n:c:qrt";

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

int main(int argc, char *argv[])
{
	int 	db,
		sd,
		idx,
		l,
		ret,
		category,
		ch,
		quiet, 
		replace_category, 
		add_title;
	char 	buf[0xffff],
		*progname = argv[0],
		*category_name = NULL;
	struct 	pi_sockaddr addr;
	struct 	PilotUser U;
	struct 	ExpenseAppInfo mai;
	struct 	Expense theExpense;

	quiet = replace_category = add_title = 0;

	while ((ch = getopt(argc, argv, optstring)) != -1)
		switch (ch) {
		case 'e':
			theExpense.type = etBus;
			for (idx = 0; expenseTypes[idx] != NULL; idx++)
			{
				if (strcasecmp(optarg, expenseTypes[idx]) == 0)
				{
					theExpense.type = idx;
					break;
				}
			}
			break;
		case 't':
			theExpense.payment = epCash;
			for (idx = 0; paymentTypes[idx] != NULL; idx++)
			{
				if (strcasecmp(optarg, paymentTypes[idx]) == 0)
				{
					theExpense.payment = idx;
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
		case 'v':
			theExpense.vendor = optarg;
			break;
		case 'l':
			theExpense.city = optarg;
			break;
		case 'n':
			theExpense.note = optarg;
			break;
		case 'c':
			category_name = optarg;
			break;
		case 'p':
			/* optarg is name of port to use instead of
			   $PILOTPORT or /dev/pilot */
			strcpy(addr.pi_device, optarg);
			break;
		case 'q':
			quiet++;
			break;
		case 'r':
			replace_category++;
			break;
		case '?':
		default:
			Help(progname);
		}

	argc -= optind;
	argv += optind;

	if (argc < 1) {
		fprintf(stderr, "%s: insufficient number of arguments\n",
			progname);
		Help(progname);
	}

	if (replace_category && !category_name) {
		fprintf(stderr,
			"%s: expense category required when specifying replace\n",
			progname);
		Help(progname);
	}

	if (!quiet)
		printf
		    ("Please insert Palm in cradle on %s and press HotSync button.\n",
		     addr.pi_device);

	if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP))) {
		perror("pi_socket");
		exit(1);
	}

	addr.pi_family = PI_AF_SLP;

	ret = pi_bind(sd, (struct sockaddr *) &addr, sizeof(addr));
	if (ret == -1) {
		perror("pi_bind");
		exit(1);
	}

	ret = pi_listen(sd, 1);
	if (ret == -1) {
		perror("pi_listen");
		exit(1);
	}

	sd = pi_accept(sd, 0, 0);
	if (sd == -1) {
		perror("pi_accept");
		exit(1);
	}

	/* Ask the pilot who it is. */
	dlp_ReadUserInfo(sd, &U);

	/* Tell user (via Palm) that we are starting things up */
	dlp_OpenConduit(sd);

	/* Open the Expense's database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, Expense_DB, &db) < 0) {
		puts("Unable to open ExpenseDB");
		dlp_AddSyncLogEntry(sd, "Unable to open ExpenseDB.\n");
		exit(1);
	}

	l = dlp_ReadAppBlock(sd, db, 0, (unsigned char *) buf, 0xffff);
	unpack_ExpenseAppInfo(&mai, (unsigned char *) buf, l);

	if (category_name) {
		category = -1;	/* invalid category */
		for (idx = 0; idx < 16; idx++)
			if (!strcasecmp
			    (mai.category.name[idx], category_name)) {
				category = idx;
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
		char buff[256], *b;
		int size;

		category = 0;	/* unfiled */

		theExpense.currency = 0;
		theExpense.attendees = "Northern Alliance";

		b = buff;

		/* Date */
		*(b++) = 0xc3;
		*(b++) = 0x45;

		*(b++) = theExpense.type;
		*(b++) = theExpense.payment;
		*(b++) = theExpense.currency;
		*(b++) = 0x00;
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
	U.lastSyncPC = 0x00010000;
	U.successfulSyncDate = time(NULL);
	U.lastSyncDate = U.successfulSyncDate;
	dlp_WriteUserInfo(sd, &U);

	dlp_AddSyncLogEntry(sd, "Wrote memo(s) to Palm.\n");

	/* All of the following code is now unnecessary, but harmless */

	dlp_EndOfSync(sd, 0);
	pi_close(sd);

	return 0;
}

static void Help(char *progname)
{
	printf("Usage: %s [-qrt] [-c category] -p <port> file [file] ...\n",
		progname);
	printf("       -q = do not prompt for HotSync button press\n");
	printf("       -t = payment type\n");
	printf("       -e = expense type\n");
	printf("       -a = payment amount\n");
	printf("       -v = vendor\n");
	printf("       -g = attendees\n");
	printf("       -l = city\n");
	printf("       -n = note\n");
	exit(2);
}
