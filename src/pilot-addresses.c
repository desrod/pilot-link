/*
 * pilot-addresses.c:  Palm address transfer utility
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

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-address.h"
#include "pi-header.h"

#define PILOTPORT "/dev/pilot"

/* Yet more hair: reorganize fields to match visible appearence */

int realentry[19] =
    { 0, 1, 13, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 15, 16, 17, 18 };

char *tableheads[22] =
    { "Last name", "First name", "Company", "Work", "Home",
	"Fax", "Other", "Email", "Address", "City", "State",
	"Zip Code", "Country", "Title", "Custom1", "Custom2", "Custom3",
	"Custom4", "Note", "Main", "Pager", "Mobile"
};

int tableformat = 0;
int tabledelim = 1;
char tabledelims[5] = { '\n', ',', ';', '\t' };
int encodechars = 0;

int inchar(FILE * in)
{
	int c;

	c = getc(in);
	if (encodechars && c == '\\') {
		c = getc(in);
		switch (c) {
		case 'b':
			c = '\b';
			break;
		case 'f':
			c = '\f';
			break;
		case 'n':
			c = '\n';
			break;
		case 't':
			c = '\t';
			break;
		case 'r':
			c = '\r';
			break;
		case 'v':
			c = '\v';
			break;
		case '\\':
			c = '\\';
			break;
		default:
			ungetc(c, in);
			c = '\\';
			break;
		}
	}
	return c;
}


int read_field(char *dest, FILE * in)
{
	int c;

	do {			/* Absorb whitespace */
		c = getc(in);
	} while ((c != EOF)
		 && ((c == ' ') || (c == '\t') || (c == '\n')
		     || (c == '\r')));

	if (c == '"') {
		c = inchar(in);

		while (c != EOF) {
			if (c == '"') {
				c = inchar(in);
				if (c != '"')
					break;
			}
			*dest++ = c;
			c = inchar(in);
		}
	} else {
		while (c != EOF) {
			if (c == ','
			    || (tableformat
				&& c == tabledelims[tabledelim])) {
				break;
			}
			*dest++ = c;
			c = inchar(in);
		}
	}
	*dest++ = '\0';

	while ((c != EOF) && ((c == ' ') || (c == '\t')))	/* Absorb whitespace */
		c = getc(in);

	if (c == ',')
		return 1;
	else if (c == ';')
		return 2;
	else if (c == '\t')
		return 3;
	else if (c == EOF)
		return -1;	/* No more */
	else
		return 0;
}

void outchar(char c, FILE * out)
{

	if (encodechars) {
		switch (c) {
		case '"':
			putc('"', out);
			putc('"', out);
			break;
		case '\b':
			putc('\\', out);
			putc('b', out);
			break;
		case '\f':
			putc('\\', out);
			putc('f', out);
			break;
		case '\n':
			putc('\\', out);
			putc('n', out);
			break;
		case '\t':
			putc('\\', out);
			putc('t', out);
			break;
		case '\r':
			putc('\\', out);
			putc('r', out);
			break;
		case '\v':
			putc('\\', out);
			putc('v', out);
			break;
		case '\\':
			putc('\\', out);
			putc('\\', out);
			break;
		default:
			putc(c, out);
			break;
		}
	} else {
		putc(c, out);
		if (c == '"')
			putc('"', out);
	}
}

int write_field(FILE * out, char *source, int more)
{

	putc('"', out);
	while (*source) {
		outchar(*source, out);
		source++;
	}
	putc('"', out);

	putc(tabledelims[more], out);
#if 0
	if (more == 1)
		putc(',', out);
	else if (more == 2)
		putc(';', out);
	else if (more == 3)
		putc('\t', out);
	else if (more == 0)
		putc('\n', out);
#endif
	return 0;
}

int match_category(char *buf, struct AddressAppInfo *aai)
{
	int i;

	for (i = 0; i < 16; i++)
		if (strcasecmp(buf, aai->category.name[i]) == 0)
			return i;
	return atoi(buf);	/* 0 is default */
}

int match_phone(char *buf, struct AddressAppInfo *aai)
{
	int i;

	for (i = 0; i < 8; i++)
		if (strcasecmp(buf, aai->phoneLabels[i]) == 0)
			return i;
	return atoi(buf);	/* 0 is default */
}

int defaultcategory = 0;

int read_file(FILE * in, int sd, int db, struct AddressAppInfo *aai)
{
	struct Address a;
	int i, l;
	char buf[0xffff];
	int category, attribute;

	do {
		i = read_field(buf, in);

		memset(&a, 0, sizeof(a));
		a.showPhone = 0;

		if (tableformat) {
			category = match_category(buf, aai);
			i = read_field(buf, in);
		} else {
			if (i == 2) {
				category = match_category(buf, aai);
				i = read_field(buf, in);
				if (i == 2) {
					a.showPhone =
					    match_phone(buf, aai);
					i = read_field(buf, in);
				}
			} else
				category = defaultcategory;
		}
		if (i < 0)
			break;

		attribute = 0;

		for (l = 0; (i >= 0) && (l < 19); l++) {
			int l2 = realentry[l];

			if ((l2 >= 3) && (l2 <= 7)) {
				if (i != 2 || tableformat)
					a.phoneLabel[l2 - 3] = l2 - 3;
				else {
					a.phoneLabel[l2 - 3] =
					    match_phone(buf, aai);
					i = read_field(buf, in);
				}
			}

			a.entry[l2] = strdup(buf);

			if (i == 0)
				break;

			i = read_field(buf, in);
		}

		attribute = (atoi(buf) ? dlpRecAttrSecret : 0);

		while (i > 0) {	/* Too many fields in record */
			i = read_field(buf, in);
		}

#ifdef DEBUG
//      printf("Category %s (%d)\n", aai->CategoryName[category], category);
		printf("Category %s (%d)\n", aai->category.name[category],
		       category);
		for (l = 0; l < 19; l++) {
			if ((l >= 3) && (l <= 7))
				printf(" %s (%d): %s\n",
				       aai->phoneLabels[a.
							phoneLabel[l - 3]],
				       a.phoneLabel[l - 3], a.entry[l]);
			else
				printf(" %s: %s\n", aai->labels[l],
				       a.entry[l]);
		}
#endif

		l = pack_Address(&a, (unsigned char *) buf, sizeof(buf));
#ifdef DEBUG
		dumpdata(buf, l);
#endif
		dlp_WriteRecord(sd, db, attribute, 0, category,
				(unsigned char *) buf, l, 0);

	} while (i >= 0);

	return 0;
}

int augment = 0;
int tablehead = 0;

int write_file(FILE * out, int sd, int db, struct AddressAppInfo *aai)
{
	struct Address a;
	int i, j, l;
	char buf[0xffff];
	int category, attribute;

	if (tablehead) {
		write_field(out, "Category", tabledelim);
		for (j = 0; j < 19; j++) {
			write_field(out, tableheads[realentry[j]],
				    tabledelim);
		}
		write_field(out, "Private-Flag", 0);
	}

	for (i = 0;
	     (j =
	      dlp_ReadRecordByIndex(sd, db, i, (unsigned char *) buf, 0,
				    &l, &attribute, &category)) >= 0;
	     i++) {


		if (attribute & dlpRecAttrDeleted)
			continue;
		unpack_Address(&a, (unsigned char *) buf, l);
		dlp_AddSyncLogEntry(sd, ".");

/* Simplified system */

#if 0
		write_field(out, "Category", 1);
		write_field(out, aai->category.name[category], -1);

		for (j = 0; j < 19; j++) {
			if (a.entry[j]) {
				putc(',', out);
				putc('\n', out);
				if ((j >= 4) && (j <= 8))
					write_field(out,
						    aai->phoneLabels[a.
								     phoneLabel
								     [j -
								      4]],
						    1);
				else
					write_field(out, aai->labels[j],
						    1);
				write_field(out, a.entry[j], -1);
			}
		}
		putc('\n', out);
#endif

/* Complex system */
#if 1
		if (tableformat || (augment && (category || a.showPhone))) {
			if (tableformat)
				write_field(out,
					    aai->category.name[category],
					    tabledelim);
			else
				write_field(out,
					    aai->category.name[category],
					    2);
			if (a.showPhone && (!tableformat)) {
				write_field(out,
					    aai->phoneLabels[a.showPhone],
					    2);
			}
		}

		for (j = 0; j < 19; j++) {
#ifdef NOT_ALL_LABELS
			if (augment && (j >= 4) && (j <= 8))
				if (a.phoneLabel[j - 4] != j - 4)
					write_field(out,
						    aai->phoneLabels[a.
								     phoneLabel
								     [j -
								      4]],
						    2);
			if (a.entry[realentry[j]])
				write_field(out, a.entry[realentry[j]],
					    tabledelim);

#else				/* print the phone labels if there is something in the field */

			if (a.entry[realentry[j]]) {
				if (augment && (j >= 4) && (j <= 8))
					write_field(out,
						    aai->phoneLabels[a.
								     phoneLabel
								     [j -
								      4]],
						    2);
				write_field(out, a.entry[realentry[j]],
					    tabledelim);
			}
#endif
			else
				write_field(out, "", tabledelim);
		}

		sprintf(buf, "%d", (attribute & dlpRecAttrSecret) ? 1 : 0);
		write_field(out, buf, 0);

#endif
	}

	return 0;
}

char *progname;

void Help(char *progname)
{
	PalmHeader(progname);

	fprintf(stderr, "   Usage: %s [-aeqDT] [-t delim] [-p port] [-c category]\n",
		progname);
	fprintf(stderr, "             [-d category] -r|-w [<file>]\n\n");
	fprintf(stderr, "     -t delim    = include category, use delimiter (3=tab, 2=;, 1=,)\n");
	fprintf(stderr, "     -T          = write header with titles\n");
	fprintf(stderr, "     -q          = do not prompt for HotSync button press\n");
	fprintf(stderr, "     -a          = augment records with additional information\n");
	fprintf(stderr, "     -e          = escape special characters with backslash\n");
	fprintf(stderr, "     -p port     = use device file <port> to communicate with Palm\n");
	fprintf(stderr, "     -c category = install to category <category> by default\n");
	fprintf(stderr, "     -d category = delete old Palm records in <category>\n");
	fprintf(stderr, "     -D          = delete all Palm records in all categories\n");
	fprintf(stderr, "     -r file     = read records from <file> and install them to Palm\n");
	fprintf(stderr, "     -w file     = get records from Palm and write them to <file>\n\n");
	exit(2);
}

void Version(void)
{
	PalmHeader(progname);
	fprintf(stderr,
		"   Type 'man 7 %s' at your shell for more information.\n\n",
		progname);
	exit(0);
}

int main(int argc, char *argv[])
{
	struct pi_sockaddr addr;
	int db;
	int sd;
	int l;
	struct PilotUser U;
	int ret;
	char buf[0xffff];
	struct AddressAppInfo aai;
	char *defaultcategoryname = 0;
	char *deletecategory = 0;
	char *progname = argv[0];

/*   char *device = argv[1]; */
	int deleteallcategories = 0;
	int quiet = 0;
	int mode = 0;
	int c;
	extern char *optarg;
	extern int optind;

	if (argc < 3)
		Help(progname);

	if (getenv("PILOTPORT")) {
		strcpy(addr.pi_device, getenv("PILOTPORT"));
	} else {
		strcpy(addr.pi_device, PILOTPORT);
	}

	while (((c = getopt(argc, argv, "DTeqp:t:d:c:arw")) != -1)
	       && (mode == 0)) {
		switch (c) {

		case 't':
			tableformat = 1;
			tabledelim = atoi(optarg);
			break;
		case 'D':
			deleteallcategories = 1;
			break;
		case 'T':
			tablehead = 1;
			break;
		case 'a':
			augment = 1;
			break;
		case 'q':
			quiet = 1;
			break;
		case 'p':
			/* optarg is name of port to use instead of
			   $PILOTPORT or /dev/pilot */

			strcpy(addr.pi_device, optarg);
			break;
		case 'd':
			deletecategory = optarg;
			break;
		case 'e':
			encodechars = 1;
			break;
		case 'c':
			defaultcategoryname = optarg;
			break;
		case 'r':
			mode = 1;
			break;
		case 'w':
			mode = 2;
			break;
		case 'h':
		case '?':
			Help(progname);
		}
	}

	if (mode == 0)
		Help(progname);

	if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP))) {
		perror("pi_socket");
		exit(1);
	}

	addr.pi_family = PI_AF_SLP;
/*   strcpy(addr.pi_device, device); */

	ret = pi_bind(sd, (struct sockaddr *) &addr, sizeof(addr));
	if (ret == -1) {
		fprintf(stderr, "\n   Unable to bind to port %s\n",
			addr.pi_device);
		perror("   pi_bind");
		fprintf(stderr, "\n");
		exit(1);
	}

	if (!quiet) {
		printf
		    ("   Port: %s\n\n   Please press the HotSync button now...\n",
		     addr.pi_device);
	}

	ret = pi_listen(sd, 1);
	if (ret == -1) {
		fprintf(stderr, "\n   Error listening on %s\n",
			addr.pi_device);
		perror("   pi_listen");
		fprintf(stderr, "\n");
		exit(1);
	}

	sd = pi_accept(sd, 0, 0);
	if (sd == -1) {
		fprintf(stderr, "\n   Error accepting data on %s\n",
			addr.pi_device);
		perror("   pi_accept");
		fprintf(stderr, "\n");
		exit(1);
	}

	if (!quiet) {
		fprintf(stderr, "Connected...\n");
	}

	/* Ask the pilot who it is. */
	dlp_ReadUserInfo(sd, &U);

	/* Tell user (via the Palm) that we are starting things up */
	dlp_OpenConduit(sd);

	/* Open the MemoDB.pdb database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "AddressDB", &db) < 0) {
		puts("Unable to open AddressDB");
		dlp_AddSyncLogEntry(sd, "Unable to open AddressDB.\n");
		exit(1);
	}

	l = dlp_ReadAppBlock(sd, db, 0, (unsigned char *) buf, 0xffff);
	unpack_AddressAppInfo(&aai, (unsigned char *) buf, l);

	if (defaultcategoryname)
		defaultcategory =
		    match_category(defaultcategoryname, &aai);
	else
		defaultcategory = 0;	/* Unfiled */

	if (mode == 2) {	/* Write */
		FILE *f = fopen(argv[optind], "w");

		if (f == NULL) {
			sprintf(buf, "%s: %s", argv[0], argv[optind]);
			perror(buf);
			exit(1);
		}
		write_file(f, sd, db, &aai);
		if (deletecategory)
			dlp_DeleteCategory(sd, db,
					   match_category(deletecategory,
							  &aai));
		fclose(f);
	} else if (mode == 1) {
		FILE *f;

		while (optind < argc) {
			f = fopen(argv[optind], "r");
			if (f == NULL) {
				sprintf(buf, "%s: %s", argv[0],
					argv[optind]);
				perror(buf);
				continue;
			}
			if (deletecategory)
				dlp_DeleteCategory(sd, db,
						   match_category
						   (deletecategory, &aai));

			if (deleteallcategories) {
				int i;

				for (i = 0; i < 16; i++)
					if (strlen(aai.category.name[i]) >
					    0)
						dlp_DeleteCategory(sd, db,
								   i);
			}

			read_file(f, sd, db, &aai);
			fclose(f);
			optind++;
		}
	}

	/* Close the database */
	dlp_CloseDB(sd, db);

	/* Tell the user who it is, with a different PC id. */
	U.lastSyncPC = 0xDEADBEEF;
	U.successfulSyncDate = time(NULL);
	U.lastSyncDate = U.successfulSyncDate;
	dlp_WriteUserInfo(sd, &U);

	if (mode == 1) {
		dlp_AddSyncLogEntry(sd, "Wrote addresses to Palm.\n");
	} else if (mode == 2) {
		dlp_AddSyncLogEntry(sd, "Read addresses from Palm.\n");
	}

	/* All of the following code is now unnecessary, but harmless */

	dlp_EndOfSync(sd, 0);
	pi_close(sd);

	return 0;
}
