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

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-address.h"
#include "pi-header.h"

#define PILOTPORT "/dev/pilot"

/* Define prototypes */
int inchar(FILE * in);
int read_field(char *dest, FILE * in);
void outchar(char c, FILE * out);
int write_field(FILE * out, char *source, int more);
int match_category(char *buf, struct AddressAppInfo *aai);
int match_phone(char *buf, struct AddressAppInfo *aai);
int read_file(FILE * in, int sd, int db, struct AddressAppInfo *aai);
int write_file(FILE * out, int sd, int db, struct AddressAppInfo *aai);

int pilot_connect(const char *port);
static void Help(char *progname);

/* Yet more hair: reorganize fields to match visible appearence */
int realentry[19] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18 };

char *tableheads[22] = { 
	"Last name", "First name", "Company", "Work", "Home", "Fax",
	"Other", "E-mail", "Address", "City", "State", "Zip Code",
	"Country", "Title", "Custom 1", "Custom 2", "Custom 3", "Custom 4",
	"Note", "Main", "Pager", "Mobile"
};

static const char *optstring = "DTeqp:t:d:c:arw";

int 	tableformat 	= 0,
	tabledelim 	= 1,
	encodechars 	= 0,
	augment 	= 0,
	tablehead 	= 0,
	defaultcategory = 0;

char 	tabledelims[5] = { '\n', ',', ';', '\t' },
	*progname;

int main(int argc, char *argv[])
{

	int 	ch,
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
		
	while (((ch = getopt(argc, argv, optstring)) != -1)
	       && (mode == 0)) {
		switch (ch) {

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
		case 'h':
			Help(progname);
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
	} else {
		sd = pilot_connect(port);
	
		/* Ask the pilot who it is. */
		dlp_ReadUserInfo(sd, &User);
	
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
		User.lastSyncPC = 0xDEADBEEF;
		User.successfulSyncDate = time(NULL);
		User.lastSyncDate = User.successfulSyncDate;
		dlp_WriteUserInfo(sd, &User);
	
		if (mode == 1) {
			dlp_AddSyncLogEntry(sd, "Wrote addresses to Palm.\n");
		} else if (mode == 2) {
			dlp_AddSyncLogEntry(sd, "Read addresses from Palm.\n");
		}
		
		dlp_EndOfSync(sd, 0);
		pi_close(sd);
	
		return 0;
	}
}

int inchar(FILE * in)
{
	int 	ch;

	ch = getc(in);
	if (encodechars && ch == '\\') {
		ch = getc(in);
		switch (ch) {
		case 'b':
			ch = '\b';
			break;
		case 'f':
			ch = '\f';
			break;
		case 'n':
			ch = '\n';
			break;
		case 't':
			ch = '\t';
			break;
		case 'r':
			ch = '\r';
			break;
		case 'v':
			ch = '\v';
			break;
		case '\\':
			ch = '\\';
			break;
		default:
			ungetc(ch, in);
			ch = '\\';
			break;
		}
	}
	return ch;
}


int read_field(char *dest, FILE * in)
{
	int 	ch;

	do {			/* Absorb whitespace */
		ch = getc(in);
	} while ((ch != EOF)
		 && ((ch == ' ') || (ch == '\t') || (ch == '\n')
		     || (ch == '\r')));

	if (ch == '"') {
		ch = inchar(in);

		while (ch != EOF) {
			if (ch == '"') {
				ch = inchar(in);
				if (ch != '"')
					break;
			}
			*dest++ = ch;
			ch = inchar(in);
		}
	} else {
		while (ch != EOF) {
			if (ch == ','
			    || (tableformat
				&& ch == tabledelims[tabledelim])) {
				break;
			}
			*dest++ = ch;
			ch = inchar(in);
		}
	}
	*dest++ = '\0';

	while ((ch != EOF) && ((ch == ' ') || (ch == '\t')))	/* Absorb whitespace */
		ch = getc(in);

	if (ch == ',')
		return 1;
	else if (ch == ';')
		return 2;
	else if (ch == '\t')
		return 3;
	else if (ch == EOF)
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
	int idx;

	for (idx = 0; idx < 16; idx++)
		if (strcasecmp(buf, aai->category.name[idx]) == 0)
			return idx;
	return atoi(buf);	/* 0 is default */
}

int match_phone(char *buf, struct AddressAppInfo *aai)
{
	int idx;

	for (idx = 0; idx < 8; idx++)
		if (strcasecmp(buf, aai->phoneLabels[idx]) == 0)
			return idx;
	return atoi(buf);	/* 0 is default */
}

int read_file(FILE * in, int sd, int db, struct AddressAppInfo *aai)
{
	int 	idx,
		l,
		attribute,
		category;
	char 	buf[0xffff];
	struct 	Address a;


	do {
		idx = read_field(buf, in);

		memset(&a, 0, sizeof(a));
		a.showPhone = 0;

		if (tableformat) {
			category = match_category(buf, aai);
			idx = read_field(buf, in);
		} else {
			if (idx == 2) {
				category = match_category(buf, aai);
				idx = read_field(buf, in);
				if (idx == 2) {
					a.showPhone =
					    match_phone(buf, aai);
					idx = read_field(buf, in);
				}
			} else
				category = defaultcategory;
		}
		if (idx < 0)
			break;

		attribute = 0;

		for (l = 0; (idx >= 0) && (l < 19); l++) {
			int l2 = realentry[l];

			if ((l2 >= 3) && (l2 <= 7)) {
				if (idx != 2 || tableformat)
					a.phoneLabel[l2 - 3] = l2 - 3;
				else {
					a.phoneLabel[l2 - 3] =
					    match_phone(buf, aai);
					idx = read_field(buf, in);
				}
			}

			a.entry[l2] = strdup(buf);

			if (idx == 0)
				break;

			idx = read_field(buf, in);
		}

		attribute = (atoi(buf) ? dlpRecAttrSecret : 0);

		while (idx > 0) {	/* Too many fields in record */
			idx = read_field(buf, in);
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

	} while (idx >= 0);

	return 0;
}

int write_file(FILE * out, int sd, int db, struct AddressAppInfo *aai)
{
	int 	idx,
		j,
		l,
		attribute,
		category;
	char 	buf[0xffff];
	struct 	Address a;
		
	if (tablehead) {
		fprintf(out, "# ");
		write_field(out, "Category", tabledelim);
		for (j = 0; j < 19; j++) {
			write_field(out, tableheads[realentry[j]],
				    tabledelim);
		}
		write_field(out, "Private-Flag", 0);
	}

	for (idx = 0;
	     (j =
	      dlp_ReadRecordByIndex(sd, db, idx, (unsigned char *) buf, 0,
				    &l, &attribute, &category)) >= 0;
	     idx++) {


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
						    aai->phoneLabels[a.phoneLabel
								     [j - 4]], 1);
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
						    aai->phoneLabels[a.phoneLabel
								     [j -4]], 2);
			if (a.entry[realentry[j]])
				write_field(out, a.entry[realentry[j]],
					    tabledelim);

#else			/* print the phone labels if there is something in the field */
			if (a.entry[realentry[j]]) {
				if (augment && (j >= 4) && (j <= 8))
					write_field(out,
						    aai->phoneLabels[a.phoneLabel
								     [j - 4]], 2);
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

static void Help(char *progname)
{
	printf("   Usage: %s [-aeqDT] [-t delim] [-p port] [-c category]\n"
	       "             [-d category] -r|-w [<file>]\n\n"
	       "     -t delim          include category, use delimiter (3=tab, 2=;, 1=,)\n"
	       "     -T                write header with titles\n"
	       "     -q                do not prompt for HotSync button press\n"
	       "     -a                augment records with additional information\n"
	       "     -e                escape special chcters with backslash\n"
	       "     -p port           use device file <port> to communicate with Palm\n"
	       "     -c category       install to category <category> by default\n"
	       "     -d category       delete old Palm records in <category>\n"
	       "     -D                delete all Palm records in all categories\n"
	       "     -r file           read records from <file> and install them to Palm\n"
	       "     -w file           get records from Palm and write them to <file>\n\n",
	progname);
	return;
}


