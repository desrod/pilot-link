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

#include "getopt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-address.h"
#include "pi-header.h"

/* Define prototypes */
static void display_help(char *progname);
void print_splash(char *progname);
int pilot_connect(char *port);

int inchar(FILE * in);
int read_field(char *dest, FILE * in);
void outchar(char c, FILE * out);
int write_field(FILE * out, char *source, int more);
int match_category(char *buf, struct AddressAppInfo *aai);
int match_phone(char *buf, struct AddressAppInfo *aai);
int read_file(FILE * in, int sd, int db, struct AddressAppInfo *aai);
int write_file(FILE * out, int sd, int db, struct AddressAppInfo *aai);

int realentry[21] = 
    { 0, 1, 13, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 15, 16, 17, 18, 19, 20 };

char *tableheads[23] = { 
	"Last Name", "First Name", "Title", "Company", "Work", "Home",
	"Fax", "Other", "E-Mail", "Address", "City", "State", "Zip",
	"Country", "Custom 1", "Custom 2", "Custom 3", "Custom 4", "Note",
	"Private", "Category"
};

struct option options[] = {
	{"port",        required_argument, NULL, 'p'},
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
	{"delall",	no_argument,       NULL, 'D'},
	{"titles", 	no_argument,       NULL, 'T'},
	{"escape",	no_argument,       NULL, 'e'},
	{"quiet",	no_argument,       NULL, 'q'},
	{"tdelim",	required_argument, NULL, 't'},
	{"delcat",	required_argument, NULL, 'd'},
	{"install",	required_argument, NULL, 'c'},
	{"augment",	no_argument,       NULL, 'a'},
	{"read",        required_argument, NULL, 'r'},
	{"write",	required_argument, NULL, 'w'},
	{NULL,          0,                 NULL, 0}
};

static const char *optstring = "p:hvDTeqt:d:c:arw";

int 	tableformat 	= 0,
	tabledelim 	= 1,
	encodechars 	= 0,
	augment 	= 0,
	tablehead 	= 0,
	defaultcategory = 0;

char 	tabledelims[5] = { '\n', ',', ';', '\t' },
	*progname;

int inchar(FILE * in)
{
	int 	c;	/* switch */

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
	int 	c;

	do {			/* Absorb whitespace */
		c = getc(in);
		if(c == '\n')
			return 0;
	} while ((c != EOF) && ((c == ' ') || (c == '\t') || (c == '\r')));

	if (c == '"') {
		c = inchar(in);

		while (c != EOF) {
			if (c == '"') {
				c = inchar(in);
				if (c != '"')
					break;
			}
			*dest++ = c;
			c= inchar(in);
		}
	} else {
		while (c != EOF) {
			if (c == ',' || c == '\n'
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
	int 	i;

	for (i = 0; i < 16; i++)
		if (strcasecmp(buf, aai->category.name[i]) == 0)
			return i;
	return atoi(buf);	/* 0 is default */
}

int match_phone(char *buf, struct AddressAppInfo *aai)
{
	int 	i;

	for (i = 0; i < 8; i++)
		if (strcasecmp(buf, aai->phoneLabels[i]) == 0)
			return i;
	return atoi(buf);	/* 0 is default */
}

int read_file(FILE * in, int sd, int db, struct AddressAppInfo *aai)
{
	int 	i,
		l,
		attribute,
		category;
	char 	buf[0xffff];
	struct 	Address a;

	printf("Writing CSV entries to Palm Address Book... ");
	fflush(stdout);
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

		for (l = 0; (i >= 0) && (l < 21); l++) {
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
		printf("Category %s (%d)\n", aai->category.name[category],
		       category);
		for (l = 0; l < 21; l++) {
			if ((l >= 3) && (l <= 7))
				printf(" %s (%d): %s\n",
				       aai->phoneLabels[a.
							phoneLabel[l - 3]],
				       a.phoneLabel[l - 3], a.entry[l]);
			else
				printf(" %s: %s\n", aai->labels[l],
				       a.entry[l]);
		}
		printf(" read_field returns %d\n", i);
#endif

		l = pack_Address(&a, (unsigned char *) buf, sizeof(buf));
#ifdef DEBUG
		dumpdata(buf, l);
#endif
		dlp_WriteRecord(sd, db, attribute, 0, category,
				(unsigned char *) buf, l, 0);

	} while (i >= 0);

	printf("done.\n");
	return 0;
}

int write_file(FILE * out, int sd, int db, struct AddressAppInfo *aai)
{
	int 	i,
		j,
		l,
		attribute,
		category;
	
	char 	buf[0xffff];
	
	struct 	Address a;
		
	if (tablehead) {
		fprintf(out, "# ");
		for (j = 0; j < 21; j++) {
			write_field(out, tableheads[realentry[j]],
				    tabledelim);
			fprintf(out, " ");
		}
		fprintf(out, "\n");
	}

	printf("Writing Palm Address Book entries to file... ");
	fflush(stdout);
	for (i = 0;
	     (j =
	      dlp_ReadRecordByIndex(sd, db, i, (unsigned char *) buf, 0,
				    &l, &attribute, &category)) >= 0;
	     i++) {


		if (attribute & dlpRecAttrDeleted)
			continue;
		unpack_Address(&a, (unsigned char *) buf, l);

/* Simplified system */
#if 0
		write_field(out, "Category", 1);
		write_field(out, aai->category.name[category], -1);

		for (j = 0; j < 21; j++) {
			if (a.entry[j]) {
				putc(',', out);
				putc('\n', out);
				if ((j >= 4) && (j <= 8))
					write_field(out,
						    aai->phoneLabels[a.phoneLabel
								     [j - 4]], 1);
				else
					write_field(out, aai->labels[j],
						    1);
				write_field(out, a.entry[j], -1);
			}
		pi_tickle(ps->sd);
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

		for (j = 0; j < 21; j++) {
#ifdef NOT_ALL_LABELS
			if (augment && (j >= 4) && (j <= 8))
				if (a.phoneLabel[j - 4] != j - 4)
					write_field(out,
						    aai->phoneLabels[a.phoneLabel
								     [j -4]], 2);
			if (a.entry[realentry[j]])
				write_field(out, a.entry[realentry[j]],
					    tabledelim);

#else			/* print the phone labels if there is something in the field */
			if (a.entry[realentry[j]]) {
				if (augment && (j >= 3) && (j <= 7))
					write_field(out,
						    aai->phoneLabels[a.phoneLabel
								     [j - 3]], 2);
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
	printf("done.\n");
	return 0;
}

static void display_help(char *progname)
{
	printf("   Usage: %s [-aeqDT] [-t delim] [-p port] [-c category]\n", progname);
	printf("             [-d category] -r|-w [<file>]\n");
	printf("   Options:\n");
	printf("     -p, --port <port>       Use device file <port> to communicate with Palm\n");
	printf("     -h, --help              Display help information for %s\n", progname);
	printf("     -v, --version           Display %s version information\n", progname);
	printf("     -t, --tdelim <delim>    Include category, use delimiter (3=tab, 2=;, 1=,)\n");
	printf("     -T, --titles            Write header with titles\n");
	printf("     -q, --quiet             Do not prompt for HotSync button press\n");
	printf("     -a, --augment           Augment records with additional information\n");
	printf("     -e, --escape            Escape special chcters with backslash\n");
	printf("     -c, --category <cat>    Install to category <cat> by default\n");
	printf("     -d, --delcat <cat>      Delete old Palm records in <cat>\n");
	printf("     -D, --delall            Delete all Palm records in all categories\n");
	printf("     -r, --read [file]..     Read records from <file> and install them to Palm\n");
	printf("     -w, --write [file]..    Get records from Palm and write them to <file>\n\n");

	exit(0);
}

int main(int argc, char *argv[])
{

	int 	c,			/* switch */
		deleteallcategories 	= 0,
		db,
		l,
		mode 			= 0,
		quiet 			= 0,
		sd 			= -1;
	
	char 	*defaultcategoryname 	= 0,
		*deletecategory 	= 0,
		*progname 		= argv[0],
		*port			= NULL,
		buf[0xffff];

	struct 	AddressAppInfo 	aai;
	struct 	PilotUser 	User;
		
	while (((c = getopt_long(argc, argv, optstring, options, NULL)) != -1)
	       && (mode == 0)) {
		switch (c) {
		case 'h':
			display_help(progname);
			return 0;
		case 'v':
			print_splash(progname);
			return 0;
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
			port = optarg;
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
		}
	}

	sd = pilot_connect(port);
	if (sd < 0)
		goto error;
	
        if (dlp_ReadUserInfo(sd, &User) < 0)
                goto error_close;
	
	/* Open the AddressDB.pdb database, store access handle in db */
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

	if (mode == 2) {	/* Write to file */
		/* FIXME - Must test for existing file first! DD 2002/03/18 */
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
	User.lastSyncPC = 0xDEADBEEF;
	User.successfulSyncDate = time(NULL);
	User.lastSyncDate = User.successfulSyncDate;
	dlp_WriteUserInfo(sd, &User);

	if (mode == 1) {
		dlp_AddSyncLogEntry(sd, "Wrote entries to Palm Address Book.\n");
	} else if (mode == 2) {
		dlp_AddSyncLogEntry(sd, "Successfully read Address Book from Palm.\n");
	}
	
	dlp_EndOfSync(sd, 0);
	pi_close(sd);

	return 0;

error_close:
        pi_close(sd);

error:
        return -1;
}
