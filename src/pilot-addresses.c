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
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <unistd.h>

#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-address.h"
#include "pi-header.h"
#include "pi-userland.h"

/* These are indexes in the tabledelims array */
enum terminators { term_newline=0,
	term_comma=1,
	term_semi=2,
	term_tab=3 } ;
char 	tabledelims[4] = { '\n', ',', ';', '\t' };



/* Define prototypes */
int inchar(FILE * in);
int read_field(char *dest, FILE * in, size_t length);
void outchar(char c, FILE * out);
int write_field(FILE * out, const char *source, enum terminators more);
int match_phone(char *buf, struct AddressAppInfo *aai);
int read_file(FILE * in, int sd, int db, struct AddressAppInfo *aai);
int write_file(FILE * out, int sd, int db, struct AddressAppInfo *aai, int human /* human-readable or CSV */);

int realentry[21] =
    { 0, 1, 13, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 15, 16, 17, 18, 19, 20 };

char *tableheads[21] = {
	"Last name",	/* 0 	*/
	"First name", 	/* 1	*/
	"Title", 	/* 2	(real entry 13)*/
	"Company", 	/* 3	*/
	"Phone1", 	/* 4	*/
	"Phone2",	/* 5	*/
	"Phone3", 	/* 6	*/
	"Phone4", 	/* 7	*/
	"Phone5", 	/* 8	*/
	"Address", 	/* 9	*/
	"City", 	/* 10	*/
	"State",	/* 11	*/
	"Zip Code",	/* 12	*/
	"Country", 	/* 13	*/
	"Custom 1", 	/* 14	*/
	"Custom 2", 	/* 15	*/
	"Custom 3", 	/* 16	*/
	"Custom 4", 	/* 17	*/
	"Note",		/* 18	*/
	"Private", 	/* 19	*/
	"Category"	/* 20	*/
};

int
	tabledelim 	= term_comma,
	augment 	= 0,
	defaultcategory = 0;



/***********************************************************************
 *
 * Function:    inchar
 *
 * Summary:     Turn the protected name back into the "original"
 *		characters
 *
 * Parameters:
 *
 * Returns:     Modified character, 'c'
 *
 ***********************************************************************/
int inchar(FILE * in)
{
	int 	c;	/* switch */

	c = getc(in);
	if (c == '\\') {
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


/***********************************************************************
 *
 * Function:    read_field
 *
 * Summary:     Reach each field of the CSV during read_file
 *
 * Parameters:  dest    <-> Buffer for storing field contents
 *              in      --> Inbound filehandle
 *              length  --> Size of buffer
 *
 * Returns:     0 for end of line
 *              1 for , termination
 *              2 for ; termination
 *              3 for \t termination
 *             -1 on end of file
 *
 *              Note that these correspond to indexes in the tabledelims
 *              array, and should be preserved.
 *
 ***********************************************************************/
int read_field(char *dest, FILE *in, size_t length)
{
	int 	c;

	if (length<=1) return -1;
	/* reserve space for trailing NUL */
	length--;

	do {	/* Absorb whitespace */
		c = getc(in);
		if(c == '\n') {
			*dest = 0;
			return term_newline;
		}

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
			if (!(--length))
				break;
			c = inchar(in);
		}
	} else {
		while (c != EOF) {
			if ((c == '\n') || (c == tabledelims[tabledelim])) {
				break;
			}
			*dest++ = c;
			if (!(--length))
				break;
			c = inchar(in);
		}
	}
	*dest++ = '\0';

	/* Absorb whitespace */
	while ((c != EOF) && ((c == ' ') || (c == '\t')))
		c = getc(in);

	if (c == ',')
		return term_comma;

	else if (c == ';')
		return term_semi;

	else if (c == '\t')
		return term_tab;

	else if (c == EOF)
		return -1;	/* No more */
	else
		return term_newline;
}


/***********************************************************************
 *
 * Function:    outchar
 *
 * Summary:     Protect each of the 'illegal' characters in the output
 *
 * Parameters:  filehandle
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void outchar(char c, FILE * out)
{
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
}


/***********************************************************************
 *
 * Function:    write_field
 *
 * Summary:     Write out each field in the CSV
 *
 * Parameters:  out    --> output file handle
 *              source --> NUL-terminated data to output
 *              more   --> delimiter number
 *
 * Returns:
 *
 ***********************************************************************/
int write_field(FILE * out, const char *source, enum terminators more)
{
	putc('"', out);

	while (*source) {
		outchar(*source, out);
		source++;
	}
	putc('"', out);

	putc(tabledelims[more], out);
	return 0;
}




/***********************************************************************
 *
 * Function:    match_phone
 *
 * Summary:     Find and match the 'phone' entries in 'buf'
 *
 * Parameters:
 *
 * Returns:
 *
 ***********************************************************************/
int match_phone(char *buf, struct AddressAppInfo *aai)
{
	int 	i;

	for (i = 0; i < 8; i++)
		if (strncasecmp(buf, aai->phoneLabels[i], sizeof(aai->phoneLabels[0])) == 0)
			return i;
	return atoi(buf);	/* 0 is default */
}


/***********************************************************************
 *
 * Function:    read_file
 *
 * Summary:    	Open specified file and read into address records
 *
 * Parameters:  filehandle
 *
 * Returns:
 *
 ***********************************************************************/
int read_file(FILE *f, int sd, int db, struct AddressAppInfo *aai)
{
	int 	i	= -1,
		l,
		attribute,
		category;
	char 	buf[0xffff];
	int showPhone = -1;

	struct 	Address addr;

	int fields = 0; /* Number of fields in this entry */
	int count = 0; /* Number of entries read */
	const char *progress = "   Reading CSV entries, writing to Palm Address Book... ";

	if (!plu_quiet) {
		printf("%s",progress);
		fflush(stdout);
	}

	while (!feof(f)) {
		fields = 0;
		l = getc(f);
		if (feof(f) || (l<0)) {
			break;
		}
		if ('#' == l) {
			/* skip remainder of line */
			while (!feof(f) && (l!='\n') && (l>=0)) {
				l = getc(f);
			}
			continue;
		} else {
			ungetc(l,f);
		}
		i = read_field(buf, f, sizeof(buf));
		/* fprintf(stderr,"* Field=%s\n",buf); */

		memset(&addr, 0, sizeof(addr));
		addr.showPhone = 0;
		showPhone = -1; /* None specified this record */

		if ((i == term_semi) && (tabledelim != term_semi)) {
			/* This is an augmented entry */
			category = plu_findcategory(&aai->category,buf,
				PLU_CAT_CASE_INSENSITIVE | PLU_CAT_DEFAULT_UNFILED);
			i = read_field(buf, f, sizeof(buf));
			if (i == term_semi) {
				showPhone = match_phone(buf, aai);
				i = read_field(buf, f, sizeof(buf));
			}
		} else {
			category = defaultcategory;
		}

		if (i < 0)
			break;

		attribute = 0;

		for (l = 0; (i >= 0) && (l < 21); l++) {
			int l2 = realentry[l];

			if ((l2 >= 3) && (l2 <= 7)) {
				if ((i != term_semi) || (tabledelim == term_semi)) {
					addr.phoneLabel[l2 - 3] = l2 - 3;
				}
				else {
					addr.phoneLabel[l2 - 3] = match_phone(buf, aai);
					i = read_field(buf, f, sizeof(buf));
				}
				if (buf[0]) {
					addr.entry[l2] = strdup(buf);
					++fields;
				} else {
					addr.entry[l2] = NULL;
				}
			} else if (19 <= l2) {
				if (19 == l2) {
					attribute = (atoi(buf) ? dlpRecAttrSecret : 0);
				}
				if (20 == l2) {
					category = plu_findcategory(&aai->category,buf,
						PLU_CAT_CASE_INSENSITIVE | PLU_CAT_DEFAULT_UNFILED);
				}
			} else {
				if (buf[0]) {
					addr.entry[l2] = strdup(buf);
					++fields;
				} else {
					addr.entry[l2] = NULL;
				}

			}

			if (i == 0)
				break;

			i = read_field(buf, f, sizeof(buf));
		}


		while (i > 0) {	/* Too many fields in record */
			i = read_field(buf, f, sizeof(buf));
		}

		if (showPhone >= 0) {
			/* Find which label matches the category to display */
			addr.showPhone = 0;
			for (i=0; i<5; ++i) {
				if (showPhone == addr.phoneLabel[i]) {
					addr.showPhone = i;
					break;
				}
			}
		}

		if (fields>0) {
			l = pack_Address(&addr, (unsigned char *) buf, sizeof(buf));
			dlp_WriteRecord(sd, db, attribute, 0, category,
					(unsigned char *) buf, l, 0);
			++count;
		}
		free_Address(&addr);

		if (!plu_quiet) {
			printf("\r%s%d",progress,count);
			fflush(stdout);
		}

	}

	if (!plu_quiet) {
		printf("\r%s%d\n   Done.\n",progress,count);
		fflush(stdout);
	}
	return 0;
}


/***********************************************************************
 *
 * Function:    write_file
 *
 * Summary:     Writes Address records in CSV format to <file>
 *
 * Parameters:  filehandle
 *
 * Returns:     0
 *
 ***********************************************************************/

void write_record_CSV(FILE *out, const struct AddressAppInfo *aai, const struct Address *addr, const int attribute, const int category)
{
	int j;
	char buffer[16];

	if (augment && (category || addr->showPhone)) {
		write_field(out,
				aai->category.name[category],
				term_semi);
		write_field(out,
				aai->phoneLabels[addr->phoneLabel[addr->showPhone]],
				term_semi);
	}

	for (j = 0; j < 19; j++) {
		if (addr->entry[realentry[j]]) {
			if (augment && (j >= 4) && (j <= 8)) {
				write_field(out,
						aai->phoneLabels[addr->phoneLabel
								[j - 4]], term_semi);
			}
			write_field(out, addr->entry[realentry[j]],
					tabledelim);
		} else {
			write_field(out, "", tabledelim);
		}
	}

	snprintf(buffer, sizeof(buffer), "%d", (attribute & dlpRecAttrSecret) ? 1 : 0);
	write_field(out, buffer, tabledelim);

	write_field(out,
		aai->category.name[category],
		term_newline);
}

void write_record_human(FILE *out, struct AddressAppInfo *aai, struct Address *addr, const int attribute, const int category)
{
	int i;

	printf("Category: %s\n", aai->category.name[category]);

	for (i = 0; i < 19; i++) {
		if (addr->entry[i]) {
			int l = i;

			if ((l >= entryPhone1) && (l <= entryPhone5)) {
				printf("%s: %s\n",
					aai->phoneLabels[addr->phoneLabel[l - entryPhone1]],
					addr->entry[i]);
			} else {
				printf("%s: %s\n", aai->labels[l],
					addr->entry[i]);
			}
		}
	}
	printf("\n");
}

int write_file(FILE * out, int sd, int db, struct AddressAppInfo *aai, int human)
{
	int 	i,
		j,
		attribute,
		category;
	struct 	Address addr;
	pi_buffer_t *buf;

	int count = 0;
	const char *progress = "   Writing Palm Address Book entries to file... ";

	if (!human) {
		/* Print out the header and fields with fields intact. Note we
		'ignore' the last field (Private flag) and print our own here, so
		we don't have to chop off the trailing comma at the end. Hacky. */
		fprintf(out, "# ");
		for (j = 0; j < 21; j++) {
			write_field(out, tableheads[j],
				j<20 ? tabledelim : term_newline);
		}
		if (augment) {
			fprintf(out,"### This in an augmented (non-standard) CSV file.\n");
		}
	}

	if (!plu_quiet) {
		printf("%s",progress);
		fflush(stdout);
	}

	buf = pi_buffer_new (0xffff);
	for (i = 0;
	     (j =
	      dlp_ReadRecordByIndex(sd, db, i, buf, 0,
				    &attribute, &category)) >= 0;
	     i++) {


		if (attribute & dlpRecAttrDeleted)
			continue;
		unpack_Address(&addr, buf->data, buf->used);

		if (!human) {
			write_record_CSV(out,aai,&addr,attribute,category);
		} else {
			write_record_human(out,aai,&addr,attribute,category);
		}


		++count;
		if (!plu_quiet) {
			printf("\r%s%d",progress,count);
			fflush(stdout);
		}
	}
	pi_buffer_free (buf);

	if (!plu_quiet) {
		printf("\r%s%d\n   Done.\n",progress,count);
		fflush(stdout);
	}
	return 0;
}


int main(int argc, const char *argv[])
{

	int 	c,			/* switch */
		db,
		l,
		sd 			= -1;

	enum { mode_none, mode_read, mode_write, mode_delete_all, mode_delete }
		run_mode = mode_none;

	const char
        	*progname 		= argv[0];

	char 	*defaultcategoryname 	= 0,
		*deletecategory 	= 0,
		*wrFilename		= NULL,
		*rdFilename		= NULL,
		buf[0xffff];
	int writehuman = 0;

	pi_buffer_t *appblock;

	struct 	AddressAppInfo 	aai;
	struct 	PilotUser 	User;

	poptContext po;

	struct poptOption options[] = {
		USERLAND_RESERVED_OPTIONS
	        {"delete-all",	 0 , POPT_ARG_NONE, NULL,  mode_delete_all, "Delete all Palm records in all categories"},
	        {"delimiter",	't', POPT_ARG_INT,  &tabledelim,          0, "Include category, use delimiter (3=tab, 2=;, 1=,)"},
        	{"delete-category",	'd', POPT_ARG_STRING, &deletecategory,'d', "Delete old Palm records in <category>", "category"},
	        {"category",	'c', POPT_ARG_STRING, &defaultcategoryname, 0, "Category to install to", "category"},
        	{"augment",	'a', POPT_ARG_NONE, &augment,             0, "Augment records with additional information"},
	        {"read",	'r', POPT_ARG_STRING, &rdFilename, 'r', "Read records from <file> and install them to Palm", "file"},
        	{"write",	'w', POPT_ARG_STRING, &wrFilename, 'w', "Get records from Palm and write them to <file>", "file"},
		{"human-readable",'C', POPT_ARG_NONE, &writehuman, 0, "Write generic human-readable output instead of CSV"},
	        POPT_TABLEEND
	};

	const char *mode_error = "   ERROR: Specify exactly one of read, write, delete or delete all.\n";

	po = poptGetContext("pilot-addresses", argc, argv, options, 0);
	poptSetOtherOptionHelp(po,"\n\n"
		"   Reads addresses from a file and installs on the Palm, or\n"
		"   writes addresses from the Palm to a file.\n\n"
		"   Provide exactly one of --read or --write.\n\n");
	plu_popt_alias(po,"delall",0,"--bad-option --delete-all");
	plu_popt_alias(po,"delcat",0,"--bad-option --delete-category");
	plu_popt_alias(po,"install",0,"--bad-option --category");
	/* Useful alias */
	plu_popt_alias(po,"no-csv",0,"--human-readable");

	if (argc < 2) {
		poptPrintUsage(po,stderr,0);
		return 1;
	}

	while ((c = poptGetNextOpt(po)) >= 0) {
		switch (c) {
		/* these are the mode-setters. delete-all does it through
		 * popt hooks, since it doesn't take an argument.
		 *
		 * Special case is that you can mix -w and -d to write the
		 * file and then delete a category.
		 */
		case mode_delete_all :
			if (run_mode != mode_none) {
				fprintf(stderr,"%s",mode_error);
				return 1;
			}
			run_mode = mode_delete_all;
			break;
		case 'r':
			if (run_mode != mode_none) {
				fprintf(stderr,"%s",mode_error);
				return 1;
			}
			run_mode = mode_read;
			break;
		case 'w':
			if ((run_mode != mode_none) && (run_mode != mode_delete)) {
				fprintf(stderr,"%s",mode_error);
				return 1;
			}
			run_mode = mode_write;
			break;
		case 'd':
			if ((run_mode != mode_none) && (run_mode != mode_write)) {
				fprintf(stderr,"%s",mode_error);
				return 1;
			}
			run_mode = mode_delete;
			break;
		default:
			fprintf(stderr,"   ERROR: Unhandled option %d.\n",c);
			return 1;
		}
	}

	if (c < -1)
		plu_badoption(po,c);

	if (mode_none == run_mode) {
		fprintf(stderr,"%s",mode_error);
		return 1;
	}

	/* The first implies that -t was given; the second that it wasn't,
	   so use default, and the third if handles weird values. */
	if ((tabledelim < 0) || (tabledelim > sizeof(tabledelim))) {
		fprintf(stderr,"   ERROR: Invalid delimiter number %d (use 0-%d).\n",
			tabledelim,(int)(sizeof(tabledelim)));
		return 1;
	}

	sd = plu_connect();

	if (sd < 0)
		goto error;

        if (dlp_ReadUserInfo(sd, &User) < 0)
                goto error_close;

	/* Open the AddressDB.pdb database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "AddressDB", &db) < 0) {
		puts("Unable to open AddressDB");
		dlp_AddSyncLogEntry(sd, "Unable to open AddressDB.\n");
		goto error_close;
	}

	appblock = pi_buffer_new(0xffff);
	l = dlp_ReadAppBlock(sd, db, 0, 0xffff, appblock);
	unpack_AddressAppInfo(&aai, appblock->data, l);
	pi_buffer_free(appblock);

	if (defaultcategoryname) {
		defaultcategory =
		    plu_findcategory(&aai.category,defaultcategoryname,
		    	PLU_CAT_CASE_INSENSITIVE | PLU_CAT_DEFAULT_UNFILED);
	} else {
		defaultcategory = 0;	/* Unfiled */
	}

	switch(run_mode) {
		FILE *f;
		int i;
		int old_quiet;
	case mode_none:
		/* impossible */
		fprintf(stderr,"%s",mode_error);
		break;
	case mode_write:
		/* FIXME - Must test for existing file first! DD 2002/03/18 */
		if (strcmp(wrFilename,"-") == 0) {
			f = stdout;
			old_quiet = plu_quiet;
			plu_quiet = 1;
		} else {
			f = fopen(wrFilename, "w");
		}
		if (f == NULL) {
			sprintf(buf, "%s: %s", progname, wrFilename);
			perror(buf);
			goto error_close;
		}
		write_file(f, sd, db, &aai, writehuman);
		if (f == stdout) {
			plu_quiet = old_quiet;
		}
		if (deletecategory) {
			dlp_DeleteCategory(sd, db,
				plu_findcategory(&aai.category,deletecategory,PLU_CAT_CASE_INSENSITIVE | PLU_CAT_WARN_UNKNOWN));
		}
		if (f != stdout) {
			fclose(f);
		}
		break;
	case mode_read:
		f = fopen(rdFilename, "r");

		if (f == NULL) {
			fprintf(stderr, "Unable to open input file");
			fprintf(stderr, " '%s' (%s)\n\n",
				rdFilename, strerror(errno));
			fprintf(stderr, "Please make sure the file");
			fprintf(stderr, "'%s' exists, and that\n",
				rdFilename);
			fprintf(stderr, "it is readable by this user");
			fprintf(stderr, " before launching.\n\n");

			goto error_close;
		}
		read_file(f, sd, db, &aai);
		fclose(f);
		break;
	case mode_delete:
		i = plu_findcategory (&aai.category,deletecategory,PLU_CAT_CASE_INSENSITIVE | PLU_CAT_WARN_UNKNOWN);
		if (i>=0) {
			dlp_DeleteCategory(sd, db, i);
		}
		break;
	case mode_delete_all:
		for (i = 0; i < 16; i++)
			if (aai.category.name[i][0])
				dlp_DeleteCategory(sd, db, i);
		break;
	}

	/* Close the database */
	dlp_CloseDB(sd, db);

	/* Tell the user who it is, with a different PC id. */
	User.lastSyncPC = 0x00010000;
	User.successfulSyncDate = time(NULL);
	User.lastSyncDate = User.successfulSyncDate;
	dlp_WriteUserInfo(sd, &User);

	if (run_mode == mode_read) {
		dlp_AddSyncLogEntry(sd, "Wrote entries to Palm Address Book.\n");
	} else if (run_mode == mode_write) {
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
