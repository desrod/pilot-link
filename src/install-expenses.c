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

int pilot_connect(const char *port);
static void print_help(char *progname);

/* Not used yet, getopt_long() coming soon! 
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
*/

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

static void print_help(char *progname)
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

int main(int argc, char *argv[])
{
	int 	db,
		sd		= -1,
		i,
		l,
		category,
		c,		/* switch */
		quiet, 
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

	quiet = replace_category = add_title = 0;


	while ((c = getopt(argc, argv, optstring)) != -1)
		switch (c) {
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
		}

	argc -= optind;
	argv += optind;

	if (argc < 1) {
		fprintf(stderr, "%s: insufficient number of arguments\n",
			progname);
		print_help(progname);
	}

	if (replace_category && !category_name) {
		fprintf(stderr,
			"%s: expense category required when specifying replace\n",
			progname);
		print_help(progname);
	}

	if (!quiet)
		printf
		    ("Please insert Palm in cradle on %s and press HotSync button.\n",
		     addr.pi_device);

	if (!(sd = pi_socket(PI_AF_PILOT, PI_SOCK_STREAM, PI_PF_DLP))) {
		perror("pi_socket");
		exit(1);
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
	unpack_ExpenseAppInfo(&mai, (unsigned char *) buf, l);

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
		char 	buff[256], *b;

		category 		= 0;	/* unfiled */
		theExpense.currency 	= 0;
		theExpense.attendees 	= "Northern Alliance";

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
	User.lastSyncPC = 0x00010000;
	User.successfulSyncDate = time(NULL);
	User.lastSyncDate = User.successfulSyncDate;
	dlp_WriteUserInfo(sd, &User);

	dlp_AddSyncLogEntry(sd, "Wrote memo(s) to Palm.\n");

	/* All of the following code is now unnecessary, but harmless */

	dlp_EndOfSync(sd, 0);
	pi_close(sd);

	return 0;

 error_close:
	pi_close(sd);
	
 error:
	perror("   ERROR");
	fprintf(stderr, "\n");

	return -1;
}

