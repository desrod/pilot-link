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
#include "userland.h"

/* Define prototypes */
int inchar(FILE * in);
int read_field(char *dest, FILE * in);
void outchar(char c, FILE * out);
int write_field(FILE * out, char *source, int more);
int match_phone(char *buf, struct AddressAppInfo *aai);
int read_file(FILE * in, int sd, int db, struct AddressAppInfo *aai);
int write_file(FILE * out, int sd, int db, struct AddressAppInfo *aai);
int read_csvline(FILE *f);

int realentry[21] =
    { 0, 1, 13, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 15, 16, 17, 18, 19, 20 };

char *tableheads[21] = {
	"Last name",	/* 0 	*/
	"First name", 	/* 1	*/
	"Title", 	/* 2	*/
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

int 	tableformat 	= 0,
	tabledelim 	= -1,
	encodechars 	= 0,
	augment 	= 0,
	tablehead 	= 0,
	defaultcategory = 0;

char 	tabledelims[4] = { '\n', ',', ';', '\t' },
        *field[100],
        *unquote(char *);


int read_csvline(FILE *f)
{

        int     nfield;
        char    *p,
		buf[0xffff],
		*q;

        if (fgets(buf, sizeof(buf), f) == NULL)
                return -1;

        nfield = 0;

        for (q = buf; (p=strtok(q, ",\n\r")) != NULL; q = NULL)
                field[nfield++] = unquote(p);

        return nfield;
}

char *unquote(char *p)
{
        if (p[0] == '"') {
                if (p[strlen(p)-1] == '"')
                        p[strlen(p)-1] = '\0';
                p++;
        }
        return p;
}


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


/***********************************************************************
 *
 * Function:    read_field
 *
 * Summary:     Reach each field of the CSV during read_file
 *
 * Parameters:  Inbound filehandle
 *
 * Returns:     0
 *
 ***********************************************************************/
int read_field(char *dest, FILE *in)
{
	int 	c;

	do {	/* Absorb whitespace */
		c = getc(in);
		if(c == '\n') {
			c = ' ';
			return 0;
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

	/* Absorb whitespace */
	while ((c != EOF) && ((c == ' ') || (c == '\t')))
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


/***********************************************************************
 *
 * Function:    write_field
 *
 * Summary:     Write out each field in the CSV
 *
 * Parameters:
 *
 * Returns:
 *
 ***********************************************************************/
int write_field(FILE * out, char *source, int more)
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
		if (strcasecmp(buf, aai->phoneLabels[i]) == 0)
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
		nf,
		attribute,
		category;
	char 	buf[0xffff];

	struct 	Address addr;

	printf("   Reading CSV entries, writing to Palm Address Book... ");
	fflush(stdout);

	while ((nf = read_csvline(f)) != -1) {
		i = read_field(buf, f);

		memset(&addr, 0, sizeof(addr));
		addr.showPhone = 0;

		if (i == 2) {
			category = userland_findcategory(&aai->category,buf);
			i = read_field(buf, f);
			if (i == 2) {
				addr.showPhone = match_phone(buf, aai);
				i = read_field(buf, f);
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
				if (i != 2 || tableformat)
					addr.phoneLabel[l2 - 3] = l2 - 3;
				else {
					addr.phoneLabel[l2 - 3] = match_phone(buf, aai);
					i = read_field(buf, f);
				}
			}

			if (i == 0)
				break;

			i = read_field(buf, f);
		}

		attribute = (atoi(buf) ? dlpRecAttrSecret : 0);

		while (i > 0) {	/* Too many fields in record */
			i = read_field(buf, f);
		}

		l = pack_Address(&addr, (unsigned char *) buf, sizeof(buf));

		dlp_WriteRecord(sd, db, attribute, 0, category,
				(unsigned char *) buf, l, 0);

	} while (i >= 0);

	printf("done.\n");
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
int write_file(FILE * out, int sd, int db, struct AddressAppInfo *aai)
{
	int 	i,
		j,
		attribute,
		category;
	struct 	Address addr;
	pi_buffer_t *buf;

	/* Print out the header and fields with fields intact. Note we
	   'ignore' the last field (Private flag) and print our own here, so
	   we don't have to chop off the trailing comma at the end. Hacky. */
	if (tablehead) {
		fprintf(out, "# ");
		for (j = 0; j < 19; j++) {
			write_field(out, tableheads[j],
				    tabledelim);
		}
		write_field(out, "Private", 0);
	}

	printf("   Writing Palm Address Book entries to file... ");
	fflush(stdout);
	buf = pi_buffer_new (0xffff);
	for (i = 0;
	     (j =
	      dlp_ReadRecordByIndex(sd, db, i, buf, 0,
				    &attribute, &category)) >= 0;
	     i++) {


		if (attribute & dlpRecAttrDeleted)
			continue;
		unpack_Address(&addr, buf->data, buf->used);

		if (tableformat || (augment && (category || addr.showPhone))) {
			if (tableformat)
				write_field(out,
					    aai->category.name[category],
					    tabledelim);
			else
				write_field(out,
					    aai->category.name[category],
					    2);
			if (addr.showPhone && (!tableformat)) {
				write_field(out,
					    aai->phoneLabels[addr.showPhone],
					    2);
			}
		}

		for (j = 0; j < 19; j++) {
			if (addr.entry[realentry[j]]) {
				if (augment && (j >= 4) && (j <= 8))
					write_field(out,
						    aai->phoneLabels[addr.phoneLabel
								     [j - 4]], 2);
				write_field(out, addr.entry[realentry[j]],
					    tabledelim);
			} else
				write_field(out, "", tabledelim);
		}

		sprintf((char *)buf->data, "%d", (attribute & dlpRecAttrSecret) ? 1 : 0);
		write_field(out, (char *)buf->data, 0);
	}
	pi_buffer_free (buf);
	printf("done.\n");
	return 0;
}


int main(int argc, const char *argv[])
{

	int 	c,			/* switch */
		deleteallcategories 	= 0,
		db,
		l,
		mode 			= 0,
		sd 			= -1;

	const char
        	*progname 		= argv[0];

	char 	*defaultcategoryname 	= 0,
		*deletecategory 	= 0,
		*wrFilename		= NULL,
		*rdFilename		= NULL,
		buf[0xffff];

	struct 	AddressAppInfo 	aai;
	struct 	PilotUser 	User;

	poptContext po;

	struct poptOption options[] = {
		USERLAND_RESERVED_OPTIONS
	        {"delete-all",	 0 , POPT_ARG_NONE, &deleteallcategories, 0, "Delete all Palm records in all categories"},
        	{"titles",	'T', POPT_ARG_NONE, &tablehead,           0, "Write header with titles"},
	        {"escape",	'e', POPT_ARG_NONE, &encodechars,         0, "Escape special chcters with backslash"},
	        {"delimiter",	't', POPT_ARG_INT,  &tabledelim,          0, "Include category, use delimiter (3=tab, 2=;, 1=,)"},
        	{"delete-category",	'd', POPT_ARG_STRING, &deletecategory,      0, "Delete old Palm records in <category>"},
	        {"category",	'c', POPT_ARG_STRING, &defaultcategoryname, 0, "Category to install to"},
        	{"augment",	'a', POPT_ARG_NONE, &augment,             0, "Augment records with additional information"},
	        {"read",	'r', POPT_ARG_STRING, &rdFilename, 'r', "Read records from <file> and install them to Palm"},
        	{"write",	'w', POPT_ARG_STRING, &wrFilename, 'w', "Get records from Palm and write them to <file>"},
	        POPT_TABLEEND
	};

	po = poptGetContext("pilot-addresses", argc, argv, options, 0);
	poptSetOtherOptionHelp(po," [-p port] <options> -r|-w file\n\n"
		"   Reads addresses from the Palm to CSV file or\n"
		"   writes addresses from CSV file to the Palm.\n\n");
	userland_popt_alias(po,"delall",0,"--bad-option --delete-all");
	userland_popt_alias(po,"delcat",0,"--bad-option --delete-category");
	userland_popt_alias(po,"install",0,"--bad-option --category");

	while ((c = poptGetNextOpt(po)) >= 0) {
		const char *duplicate_rw = "   ERROR: Can only specify one of -rw.\n";
		switch (c) {
		case 'r':
			if (mode) {
				fprintf(stderr,duplicate_rw);
				return 1;
			}
			mode = 1;
			break;
		case 'w':
			if (mode) {
				fprintf(stderr,duplicate_rw);
				return 1;
			}
			mode = 2;
			break;
		default:
			fprintf(stderr,"   ERROR: Unhandled option %d.\n",c);
			return 1;
		}
	}

	if (c < -1) userland_badoption(po,c);

	/* The first implies that -t was given; the second that it wasn't,
	   so use default, and the third if handles weird values. */
	if ((tabledelim > 0) && (tabledelim < sizeof(tabledelim))) tableformat = 1;
	if (tabledelim == -1) tabledelim = 1;
	if ((tabledelim < 0) || (tabledelim > sizeof(tabledelim))) {
		fprintf(stderr,"   ERROR: Invalid delimiter number %d.\n",tabledelim);
		return 1;
	}

	if ((mode < 1) || (mode > 2)) {
		fprintf(stderr,"   ERROR: Must specify one of -r -w.\n");
		return 1;
	}

	sd = userland_connect();

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

	l = dlp_ReadAppBlock(sd, db, 0, (unsigned char *) buf, 0xffff);
	unpack_AddressAppInfo(&aai, (unsigned char *) buf, l);

	if (defaultcategoryname)
		defaultcategory =
		    userland_findcategory(&aai.category,defaultcategoryname);
	else
		defaultcategory = 0;	/* Unfiled */

	if (mode == 2) {		/* Write to file */
		/* FIXME - Must test for existing file first! DD 2002/03/18 */
		FILE *f = fopen(wrFilename, "w");

		if (f == NULL) {
			sprintf(buf, "%s: %s", progname, wrFilename);
			perror(buf);
			goto error_close;
		}
		write_file(f, sd, db, &aai);
		if (deletecategory)
			dlp_DeleteCategory(sd, db,
				userland_findcategory(&aai.category,deletecategory));
		fclose(f);

	} else if (mode == 1) {	/* Read from file */
		FILE *f = fopen(rdFilename, "r");

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
	} else if (deletecategory)
		dlp_DeleteCategory(sd, db,
				userland_findcategory (&aai.category,deletecategory));
	  else
		if (deleteallcategories) {
			int i;

			for (i = 0; i < 16; i++)
				if (strlen(aai.category.name[i]) > 0)
					dlp_DeleteCategory(sd, db, i);
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
