/*
 * pilot-schlep.c:  Utility to transfer arbitrary data to/from your Palm
 *
 * Copyright (c) 1996, Kenneth Albanowski.
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
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-file.h"
#include "pi-dlp.h"
#include "pi-header.h"

int pilot_connect(const char *port);
static void Help(char *progname);

/* Not used yet, getopt_long() coming soon! 
struct option options[] = {
	{"help",        no_argument,       NULL, 'h'},
	{"port",        required_argument, NULL, 'p'},
	{"install",     no_argument,       NULL, 'i'},
	{"fetch",       no_argument,       NULL, 'f'},
	{"delete",      no_argument,       NULL, 'd'},
	{NULL,          0,                 NULL, 0}
};
*/

static const char *optstring = "hp:ifd";

/* Declare prototypes */
#define pi_mktag(c1,c2,c3,c4) (((c1)<<24)|((c2)<<16)|((c3)<<8)|(c4))


int main(int argc, char *argv[])
{
	int 	ch,
		sd 		= -1,
		segment 	= 4096,
		Install 	= -1,
		Fetch 		= -1,
		Delete 		= -1;
	char 	*progname 	= argv[0],
		*port 		= NULL,
		*install 	= NULL,
		*fetch 		= NULL,
		*delete 	= NULL;

	while ((ch = getopt(argc, argv, optstring)) != -1) {
		switch (ch) {

		  case 'h':
			  Help(progname);
			  exit(0);
		  case 'p':
			  port = optarg;
			  break;
		  case 'i':
			  install = optarg;
			  Install = 1;
			  break;
		  case 'f':
			  fetch = optarg;
			  Fetch = 1;
			  break;
		  case 'd':
			  delete = optarg;
			  Delete = 1;
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
	} else if (port != NULL) {
		
		sd = pilot_connect(port);

		/* Did we get a valid socket descriptor back? */
		if (dlp_OpenConduit(sd) < 0) {
			exit(1);

		/* Install here, not Fetch */
		} else 	if (Install && Fetch == -1) {
			int 	db,
				j,
				l;
			unsigned long len;
			char 	buffer[0xffff];
		
			/* Need to fix this, we should not require redirection */
			if (isatty(fileno(stdin))) {
				printf("Cannot install from tty, please redirect from file.\n");
				exit(1);
			}
		
			dlp_DeleteDB(sd, 0, "Schlep");
			if (dlp_CreateDB
			    (sd, pi_mktag('S', 'h', 'l', 'p'),
			     pi_mktag('D', 'A', 'T', 'A'), 0, dlpDBFlagResource, 1,
			     "Schlep", &db) < 0)
				return 0;
		
			printf("\nPlease stand by... installing data");
		
			l = 0;
			for (j = 0; (len = read(fileno(stdin), buffer, segment)) > 0; j++) {
				if (dlp_WriteResource
				    (sd, db, pi_mktag('D', 'A', 'T', 'A'), j, buffer,
				     len) < 0)
					break;
				l += len;
				fprintf(stderr, ".");
			}
			printf("\n%d bytes written\n", l);
		
			dlp_CloseDB(sd, db);
		/* Fetch here, not Install */
		} else if (Fetch && Install == -1) {
			int 	db,
				idx,
				l;
			char 	buffer[0xffff];
		
			if (dlp_OpenDB(sd, 0, dlpOpenRead, "Schlep", &db) < 0)
				return 0;
		
			for (idx = 0;
			     (l =
			      dlp_ReadResourceByType(sd, db, pi_mktag('D', 'A', 'T', 'A'),
						     idx, buffer, 0, 0)) > 0; idx++) {
				write(fileno(stdout), buffer, l);
				fprintf(stderr, ".");
			}
			printf("\nFile successfully retrieved, please verify.\n");

		} else if (Delete && (Fetch == -1 && Install == -1)) {
			dlp_DeleteDB(sd, 0, "Schlep");
			printf("Delete successfully completed.\n");
		}
	dlp_AddSyncLogEntry(sd, "pilot-schlep, exited normally.\n"
				"Thank you for using pilot-link.\n");
	dlp_EndOfSync(sd, 0);
	pi_close(sd);
	}
	return 0;
}


static void Help(char *progname)
{
	printf("   Package up any arbitrary file and sync it to your Palm device\n\n"
	       "   Usage: %s -p <port> [options]\n\n"
	       "   Options:\n"
	       "     -p <port>      Use device file <port> to communicate with Palm\n"
	       "     -i < filename  Pack up and install the arbitrary file to your Palm\n"
	       "     -f > filename  Unpack the arbitrary file from your Palm device\n"
	       "     -d             Delete the packaged 'Schlep' file from your Palm device\n"
	       "     -h             Display this information\n\n"
	       "   Examples:\n"
	       "   To package up and store a file for later retrieval on your Palm:\n"
	       "             %s -p /dev/pilot -i < InstallThis.zip\n\n"
	       "   To unpack a file that has been stored on your Palm device with %s:\n"
	       "             %s -p /dev/pilot -f > RetrieveThis.pdf\n\n"
	       "   Please notice that you must use redirection to Install or Fetch files\n"
	       "   using %s. Currently the stored name and file type is not\n"             
	       "   queried so you can potentially Install a PDF file, and retrieve it as a\n"
	       "   ZIP file.  You must take care to remember what type of file you are\n"
	       "   installing and fetching. This will be updated in a later release to\n"
	       "   handle this type of capability, as well as handle multiple 'Schlep' files.\n\n", 
		progname, progname, progname, progname, progname);
	return;
}

