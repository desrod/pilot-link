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
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-file.h"
#include "pi-dlp.h"
#include "pi-header.h"

int pilot_connect(const char *port);

struct option options[] = {
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
	{"port",        required_argument, NULL, 'p'},
	{"install",     required_argument, NULL, 'i'},
	{"fetch",       required_argument, NULL, 'f'},
	{"delete",      no_argument,       NULL, 'd'},
	{NULL,          0,                 NULL, 0}
};

static const char *optstring = "hvp:i:f:d";

/* Declare prototypes */
#define pi_mktag(c1,c2,c3,c4) (((c1)<<24)|((c2)<<16)|((c3)<<8)|(c4))

static int Fetch(int sd, char *filename) 
{
	int 	db,
		i,
		l,
		fd;
	char 	buffer[0xffff];
		
	fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC);
	if (fd < 0)
		return -1;

	printf("\tFetching to %s ", filename);
	fflush(stdout);
	if (dlp_OpenDB(sd, 0, dlpOpenRead, "Schlep", &db) < 0)
		return -1;

	for (i = 0; 
	     (l = dlp_ReadResourceByType(sd, db, pi_mktag('D', 'A', 'T', 'A'),
					 i, buffer, 0, 0)) > 0; i++) {
		if (write(fd, buffer, l) < 0) {
			printf("%d bytes read (Incomplete)\n\n", l);
			close(fd);
			return -1;
		}
		printf(".");
		fflush(stdout);
	}

	close(fd);
	printf("%d bytes read\n\n", l);
	return 0;
}

static int Install(int sd, char *filename) 
{
	int 	db,
		j,
		l,
		fd,
		segment 	= 4096;
	unsigned long len;
	char 	buffer[0xffff];
		
	fd = open(filename, O_RDONLY);
	if (fd < 0)
		return -1;
	
	dlp_DeleteDB(sd, 0, "Schlep");

	printf("\tInstalling %s ", filename);
	fflush(stdout);
	
	if (dlp_CreateDB (sd, pi_mktag('S', 'h', 'l', 'p'),
			  pi_mktag('D', 'A', 'T', 'A'), 0, 
			  dlpDBFlagResource, 1, "Schlep", &db) < 0)
		return -1;
		
	l = 0;
	for (j = 0; (len = read(fd, buffer, segment)) > 0; j++) {
		if (dlp_WriteResource (sd, db, pi_mktag('D', 'A', 'T', 'A'),
				       j, buffer, len) < 0) {
			printf("  %d bytes written (Incomplete)\n\n", l);
			close(fd);
			dlp_CloseDB(sd, db);
			return -1;
		}
		l += len;
		printf(".");
		fflush(stdout);
	}
	close(fd);
	printf("  %d bytes written\n\n", l);
		
	if (dlp_CloseDB(sd, db) < 0)
		return -1;
	
	return 0;
}

static int Delete(int sd) 
{
	printf("\tDeleting... ");
	fflush(stdout);
	if (dlp_DeleteDB(sd, 0, "Schlep") < 0) {
		printf("failed\n\n");
		return 0;
	}
	printf("completed\n\n");
	
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

int main(int argc, char *argv[])
{
	int 	c,		/* switch */
		sd 		= -1,

		install 	= -1,
		fetch 		= -1,
		delete 		= -1;
	
	char 	*progname 	= argv[0],
		*port 		= NULL,
		*filename 	= NULL;

	while ((c = getopt_long(argc, argv, optstring, options, NULL)) != -1) {
		switch (c) {

		case 'h':
			Help(progname);
			exit(0);
		case 'v':
			PalmHeader(progname);
			return 0;
		case 'p':
			port = optarg;
			break;
		case 'i':
			filename = optarg;
			install = 1;
			break;
		case 'f':
			filename = optarg;
			fetch = 1;
			break;
		case 'd':
			delete = 1;
			break;
		}
	}
	
	if (install + fetch + delete > -1) {
		Help(progname);
		fprintf(stderr, "ERROR: You must specify only one action\n");
		return -1;
	} else if (install + fetch + delete == -3) {
		Help(progname);
		fprintf(stderr, "ERROR: You must specify atleast one action\n");
		return -1;
	}
		
	sd = pilot_connect(port);
	if (sd < 0)
		goto error;
	
	if (dlp_OpenConduit(sd) < 0)
		goto error_close;

	if (install == 1) {
		if (Install (sd, filename) < 0)
			goto error_close;
	} else if (fetch == 1) {
		if (Fetch (sd, filename) < 0)
			goto error_close;
	} else if (delete == 1) {
		if (Delete (sd) < 0)
			goto error_close;
	}
	
	if (dlp_AddSyncLogEntry(sd, "pilot-schlep, exited normally.\n"
				"Thank you for using pilot-link.\n") < 0)
		goto error_close;

	if (dlp_EndOfSync(sd, 0) < 0)
		goto error_close;

	if (pi_close(sd) < 0)
		goto error;

	return 0;

 error_close:
	pi_close(sd);
	
 error:
	perror("\tError");
	fprintf(stderr, "\n");

	return -1;
}

