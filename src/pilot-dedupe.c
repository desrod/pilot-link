/*
 * pilot-dedupe.c:  Palm utility to remove duplicate records
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

#include "pi-header.h"
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"

int pilot_connect(const char *port);

struct option options[] = {
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
	{"port",        required_argument, NULL, 'p'},
	{NULL,          0,                 NULL, 0}
};

static const char *optstring = "hvp:";

struct record {
	struct record *next;
	unsigned long id;
	char *data;
	int cat;
	int index;
	int len;
};

/***********************************************************************
 *
 * Function:    compare_r
 *
 * Summary:     Compare records
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int compare_r(const void *av, const void *bv)
{
	int 	i,
		o;
	struct record *a = *(struct record **) av;
	struct record *b = *(struct record **) bv;

	if (a->cat < b->cat)
		o = -1;
	else if (a->cat > b->cat)
		o = 1;
	else if (a->len < b->len)
		o = -1;
	else if (a->len > b->len)
		o = 1;
	else if ((i = memcmp(a->data, b->data, a->len)))
		o = i;
	else if (a->index < b->index)
		o = -1;
	else if (a->index > b->index)
		o = 1;
	else
		o = 0;

	return o;
}

static int DeDupe (int sd, char *dbname) 
{
	int 	c,
		db,
		dupe 	= 0,
		j,
		k,
		l;
	char buf[0xffff];
	struct record *r,
		      **sortidx,
		      *records = NULL;

	/* Open the database, store access handle in db */
	printf("Opening %s\n", dbname);
	if (dlp_OpenDB(sd, 0, dlpOpenReadWrite, dbname, &db) < 0) {
		printf("Unable to open %s\n", dbname);
		return -1;
	}

	printf("Reading records...\n");

	l 	= 0;
	c 	= 0;
	for (;;) {
		int 	attr,
			cat;
		recordid_t id;
		int len =
			dlp_ReadRecordByIndex(sd, db, l,
					      (unsigned char *) buf,
					      &id,
					      0, &attr, &cat);

		l++;

		if (len < 0)
			break;

		/* Skip deleted records */
		if ((attr & dlpRecAttrDeleted)
		    || (attr & dlpRecAttrArchived))
			continue;

		count++;

		r = (struct record *)
			malloc(sizeof(struct record));

		r->data 	= (char *) malloc(len);
		memcpy(r->data, buf, len);
		r->len 		= len;
		r->cat 		= cat;
		r->id 		= id;
		r->index 	= l;

		r->next 	= records;
		records 	= r;

	}

	sortidx = malloc(sizeof(struct record *) * c);

	r = records;
	for (k = 0; r && (k < count); k++, r = r->next)
		sortidx[k] = r;

	qsort(sortidx, count, sizeof(struct record *), compare_r);

	printf("Scanning for duplicates...\n");

	for (k = 0; k < c; k++) {
		struct 	record *r2;
		int 	d = 0;

		r = sortidx[k];

		if (r->len < 0)
			continue;

		for (j = k + 1; j < c; j++) {
			r2 = sortidx[j];

			if (r2->len < 0)
				continue;

			if ((r->len != r2->len)
			    || memcmp(r->data, r2->data, r->len))
				break;

			printf
				("Deleting record %d, duplicate #%d of record %d\n",
				 r2->index, ++d, r->index);
			dupe++;
			dlp_DeleteRecord(sd, db, 0, r2->id);

			r2->len = -1;
			r2->id = 0;

		}
		k = j - 1;

	}

	free(sortidx);

	while (records) {
		if (records->data)
			free(records->data);
		r 	= records;
		records = records->next;
		free(r);
	}

	/* Close the database */
	dlp_CloseDB(sd, db);
	sprintf(buf, "Removed %d duplicates from %s\n", dupe,
		dbname);
	printf("%s", buf);
	dlp_AddSyncLogEntry(sd, buf);

	return 0;
}

static void Help(char *progname)
{
	printf("   Removes duplicate records from any Palm database\n\n"
	       "   Usage: %s -p <port> dbname [dbname ...]\n"
	       "                       -o <hostname> -a <ip> -n <subnet>\n\n"
	       "   Options:\n"
	       "     -p <port>         Use device file <port> to communicate with Palm\n"
	       "     -h, --help        Display this information\n"
	       "     -v, --version     Display version information\n\n"
	       "   Examples: %s -p /dev/pilot AddressDb\n\n",
	       progname, progname);
	return;
}


int main(int argc, char *argv[])
{
	int     c,		/* switch */
		sd		= -1;
	char 	*port 	        = NULL,
		*progname 	= argv[0];

	while ((c = getopt_long(argc, argv, optstring, options, NULL)) != -1) {
		switch (c) {

		case 'h':
			Help(progname);
			return 0;
		case 'v':
			PalmHeader(progname);
			return 0;
		case 'p':
			port = optarg;
			break;
		}
	}

	if (optind < 0) {
		Help(progname);
		fprintf(stderr, "\tERROR: You must specify atleast one database\n");
		return -1;		
	}
	
	sd = pilot_connect (port);
	if (sd < 0)
		goto error;

	for (; optind < argc; optind++)
		DeDupe (sd, argv[optind]);

	if (dlp_ResetLastSyncPC(sd) < 0)
		goto error_close;

	if (pi_close(sd) < 0)
		goto error;
	
	return 0;

 error_close:
	pi_close(sd);
	
 error:
	perror("   ERROR");
	fprintf(stderr, "\n");

	return -1;
}
