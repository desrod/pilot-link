/*
 * addresses.c:  Translate Palm address book into a generic format
 *
 * Copyright (c) 1996, Kenneth Albanowski
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
#include <sys/stat.h>
#include <signal.h>
#include <utime.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-address.h"
#include "pi-dlp.h"
#include "pi-header.h"

int pilot_connect(const char *port);
static void print_help(char *progname);

struct option options[] = {
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
	{"port",        required_argument, NULL, 'p'},
	{NULL,          0,                 NULL, 0}
};

static const char *optstring = "hvp:";

static void print_help(char *progname)
{
	printf("   Dumps the Palm AddressDB database into a generic text output format\n\n"
               "   Usage: %s -p <port> [options]\n\n"
	       "   Options:\n"
	       "   -p <port>      Use device file <port> to communicate with Palm\n"
	       "   -h             Display this information\n\n"
	       "   Only the port option is required, the other options are... optional.\n\n"
	       "   Example: %s -p /dev/pilot\n\n"
	       "   You can redirect the output of %s to a file instead of the default\n"
	       "   STDOUT by using redirection and pipes as necessary.\n\n"
	       "   Example: %s -p /dev/pilot -f > MyAddresses.txt\n\n", progname, progname, progname, progname);
	return;
}

int main(int argc, char *argv[])
{
	int 	c,		/* switch */
		db,
		index,
		sd 		= -1;
	
	char 	*progname 	= argv[0],
		*port 		= NULL;
	
	struct 	AddressAppInfo aai;

	unsigned char buffer[0xffff];
	
        while ((c = getopt_long(argc, argv, optstring, options, NULL)) != -1) {
                switch (c) {

                  case 'h':
                          print_help(progname);
                          exit(0);
                  case 'v':
                          print_splash(progname);
                          exit(0);
                  case 'p':
                          port = optarg;
                          break;
                }
        }

	sd = pilot_connect(port);

	if (sd < 0)
		goto error;

	if (dlp_OpenConduit(sd) < 0)
		goto error_close;
	
	/* Open the Address book's database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "AddressDB", &db) < 0) {
		puts("Unable to open AddressDB");
		dlp_AddSyncLogEntry(sd, "Unable to open AddressDB.\n");
		exit(1);
	}
	
	dlp_ReadAppBlock(sd, db, 0, buffer, 0xffff);
	unpack_AddressAppInfo(&aai, buffer, 0xffff);
	
	for (index = 0;; index++) {
		int 	attr,
			category,
			i;
		struct 	Address a;

		int len =
		    dlp_ReadRecordByIndex(sd, db, index, buffer, 0, 0, &attr,
					  &category);
	
		if (len < 0)
			break;
	
		/* Skip deleted records */
		if ((attr & dlpRecAttrDeleted)
		    || (attr & dlpRecAttrArchived))
			continue;
	
		unpack_Address(&a, buffer, len);

		printf("Category: %s\n", aai.category.name[category]);

		for (i = 0; i < 19; i++) {
			if (a.entry[i]) {
				int l = i;
	
				if ((l >= entryPhone1) && (l <= entryPhone5)) {
					printf("%s: %s\n", aai.phoneLabels[a.phoneLabel[l - entryPhone1]], a.entry[i]);
				} else {
					printf("%s: %s\n", aai.labels[l], a.entry[i]);
				}
			}
		}
		printf("\n");
		free_Address(&a);
	}

	/* Close the database */
	dlp_CloseDB(sd, db);
        dlp_AddSyncLogEntry(sd, "Successfully read addresses from Palm.\n"
				"Thank you for using pilot-link.\n");
	dlp_EndOfSync(sd, 0);
	pi_close(sd);
	return 0;

 error_close:
        pi_close(sd);

 error:
        return -1;
}
