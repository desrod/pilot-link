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
#include <strings.h>
#include <errno.h>
#include <unistd.h>

#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-address.h"
#include "pi-header.h"

/* Define prototypes */
int inchar(FILE * in);
int read_field(char *dest, FILE * in);
void outchar(char c, FILE * out);
int write_field(FILE * out, char *source, int more);
int match_category(char *buf, struct AddressAppInfo *aai);
int match_phone(char *buf, struct AddressAppInfo *aai);
int read_file(FILE * in, int sd, int db, struct AddressAppInfo *aai);
int write_file(FILE * out, int sd, int db, struct AddressAppInfo *aai);
int read_csvline(FILE *f);

static void display_help(const char *progname);

int realentry[21] = 
    { 0, 1, 13, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 15, 16, 17, 18, 19, 20 };

char *tableheads[21] = { 
	"Last name",	/* 0 	*/
	"First name", 	/* 1	*/
	"Title", 	/* 2	*/
	"Company", 	/* 3	*/
	"Work", 	/* 4	*/
	"Home",		/* 5	*/
	"Fax", 		/* 6	*/
	"Other", 	/* 7	*/
	"E-mail", 	/* 8	*/
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

static const char *optstring = "hvDTeqp:t:d:c:arw";

struct option options[] = {
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
	{"delall",	no_argument,       NULL, 'D'},
	{"titles", 	no_argument,       NULL, 'T'},
	{"escape",	no_argument,       NULL, 'e'},
	{"quiet",	no_argument,       NULL, 'q'},
	{"port",        required_argument, NULL, 'p'},
	{"tdelim",	required_argument, NULL, 't'},
	{"delcat",	required_argument, NULL, 'd'},
	{"install",	required_argument, NULL, 'c'},
	{"augment",	no_argument,       NULL, 'a'},
	{"read",        required_argument, NULL, 'r'},
	{"write",	required_argument, NULL, 'w'},
	{NULL,          0,                 NULL, 0}
};


int 	tableformat 	= 0,
	tabledelim 	= 1,
	encodechars 	= 0,
	augment 	= 0,
	tablehead 	= 0,
	defaultcategory = 0;

char 	tabledelims[5] = { '\n', ',', ';', '\t' },
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


/***********************************************************************
 *
 * Function:    match_category
 *
 * Summary:     Find and match the specified category name in 'buf'
 *
 * Parameters:  
 *
 * Returns:     
 *
 ***********************************************************************/
int match_category(char *buf, struct AddressAppInfo *aai)
{
	int 	i;

	for (i = 0; i < 16; i++)
		if (strcasecmp(buf, aai->category.name[i]) == 0)
			return i;
	return atoi(buf);	/* 0 is default */
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
			category = match_category(buf, aai);
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
	
	size_t	l;

	char 	buf[0xffff];
	
	struct 	Address addr;
		
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
	for (i = 0;
	     (j =
	      dlp_ReadRecordByIndex(sd, db, i, (unsigned char *) buf, 0,
				    &l, &attribute, &category)) >= 0;
	     i++) {


		if (attribute & dlpRecAttrDeleted)
			continue;
		unpack_Address(&addr, (unsigned char *) buf, l);

/* Simplified system */
#if 0
		write_field(out, "Category", 1);
		write_field(out, aai->category.name[category], -1);

		for (j = 0; j < 19; j++) {
			if (addr.entry[j]) {
				putc(',', out);
				putc('\n', out);
				if ((j >= 4) && (j <= 8))
					write_field(out,
						    aai->phoneLabels[addr.phoneLabel
								     [j - 4]], 1);
				else
					write_field(out, aai->labels[j],
						    1);
				write_field(out, addr.entry[j], -1);
			}
		pi_tickle(ps->sd);
		}
		putc('\n', out);
#endif

/* Complex system */
#if 1
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
#ifdef NOT_ALL_LABELS
			if (augment && (j >= 4) && (j <= 8))
				if (addr.phoneLabel[j - 4] != j - 4)
					write_field(out,
						    aai->phoneLabels[addr.phoneLabel
								     [j -4]], 2);
			if (addr.entry[realentry[j]])
				write_field(out, addr.entry[realentry[j]],
					    tabledelim);

#else			/* print the phone labels if there is something in the field */
			if (addr.entry[realentry[j]]) {
				if (augment && (j >= 4) && (j <= 8))
					write_field(out,
						    aai->phoneLabels[addr.phoneLabel
								     [j - 4]], 2);
				write_field(out, addr.entry[realentry[j]],
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


/***********************************************************************
 *
 * Function:    display_help
 *
 * Summary:     Print out the --help options and arguments
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void display_help(const char *progname)
{
	printf("   Usage: %s [-aeqDT] [-t delim] [-p port] [-c category]\n", progname);
	printf("             [-d category] -r|-w [<file>]\n\n");
	printf("     -t delim          include category, use delimiter (3=tab, 2=;, 1=,)\n");
	printf("     -T                write header with titles\n");
	printf("     -q                do not prompt for HotSync button press\n");
	printf("     -a                augment records with additional information\n");
	printf("     -e                escape special chcters with backslash\n");
	printf("     -p port           use device file <port> to communicate with Palm\n");
	printf("     -c category       install to category <category> by default\n");
	printf("     -d category       delete old Palm records in <category>\n");
	printf("     -D, --delall      delete all Palm records in all categories\n");
	printf("     -r file           read records from <file> and install them to Palm\n");
	printf("     -w file           get records from Palm and write them to <file>\n");
	printf("     -h, --help        Display this information\n");
	printf("     -v, --version     Display version information\n\n");

	return;
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
		default:
			display_help(progname);
			return 0;
		}
	}
	
	if (argc < 2) {
		display_help(progname);
		return 0;
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
		exit(EXIT_FAILURE);
	}

	l = dlp_ReadAppBlock(sd, db, 0, (unsigned char *) buf, 0xffff);
	unpack_AddressAppInfo(&aai, (unsigned char *) buf, l);

	if (defaultcategoryname)
		defaultcategory =
		    match_category(defaultcategoryname, &aai);
	else
		defaultcategory = 0;	/* Unfiled */

	if (mode == 2) {		/* Write to file */
		/* FIXME - Must test for existing file first! DD 2002/03/18 */
		FILE *f = fopen(argv[optind], "w");

		if (f == NULL) {
			sprintf(buf, "%s: %s", argv[0], argv[optind]);
			perror(buf);
			exit(EXIT_FAILURE);
		}
		write_file(f, sd, db, &aai);
		if (deletecategory)
			dlp_DeleteCategory(sd, db,
					   match_category(deletecategory,
							  &aai));
		fclose(f);

	} else if (mode == 1) {	/* Read from file */
		FILE *f = fopen(argv[optind], "r");
		while (optind < argc) {
			if (f == NULL) {
				fprintf(stderr, "Unable to open input file");
				fprintf(stderr, " '%s' (%s)\n\n", 
					argv[optind], strerror(errno));   
				fprintf(stderr, "Please make sure the file");
				fprintf(stderr, "'%s' exists, and that\n",
					argv[optind]);

				fprintf(stderr, "it is readable by this user");
				fprintf(stderr, " before launching.\n\n");

				exit(EXIT_FAILURE);
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
