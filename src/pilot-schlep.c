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

#include "popt.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "pi-source.h"
#include "pi-file.h"
#include "pi-header.h"

#define pi_mktag(c1,c2,c3,c4) (((c1)<<24)|((c2)<<16)|((c3)<<8)|(c4))

static int Fetch(int sd, char *filename) 
{
	int 	db,
		i,
		l,
		fd;
	pi_buffer_t *buffer;
		
	fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC);
	if (fd < 0)
		return -1;

	printf("   Fetching to %s ", filename);
	fflush(stdout);
	if (dlp_OpenDB(sd, 0, dlpOpenRead, "Schlep", &db) < 0)
		return -1;

	buffer = pi_buffer_new (0xffff);
	for (i = 0; 
	     (l = dlp_ReadResourceByType(sd, db, pi_mktag('D', 'A', 'T', 'A'),
					 i, buffer, 0)) > 0; i++) {
		if (write(fd, buffer->data, l) < 0) {
			printf("%d bytes read (Incomplete)\n\n", l);
			close(fd);
			return -1;
		}
		printf(".");
		fflush(stdout);
	}
	pi_buffer_free(buffer);

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

	printf("   Installing %s ", filename);
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
	printf("   Deleting... ");
	fflush(stdout);
	if (dlp_DeleteDB(sd, 0, "Schlep") < 0) {
		printf("failed\n\n");
		return 0;
	}
	printf("completed\n\n");
	
	return 0;
}

static void display_help(const char *progname)
{
	printf("   Package up any arbitrary file and sync it to your Palm device\n\n");
	printf("   Usage: %s -p <port> [options]\n\n", progname);
	printf("   Options:\n");
	printf("     -p, --port <port>       Use device file <port> to communicate with Palm\n");
	printf("     -h, --help              Display this information\n");
	printf("     -v, --version           Display version information\n");
	printf("     -i, --install file      Pack and install the file to your Palm\n");
	printf("     -f, --fetch file        Unpack the file from your Palm device\n");
	printf("     -d, --delete            Delete the packaged file from your Palm device\n\n");
	printf("   Examples:\n");
	printf("     To package up and store a file for later retrieval on your Palm:\n");
	printf("             %s -p serial:/dev/ttyUSB0 -i InstallThis.zip\n\n", progname);
	printf("     To unpack a file that has been stored on your Palm with %s:\n", progname);
	printf("             %s -p serial:/dev/ttyUSB0 -f RetrieveThis.pdf\n\n", progname);

	printf("   Currently the stored name and file type is not queried so you can\n");
	printf("   potentially Install a PDF file, and retrieve it as a ZIP file.\n\n");

	printf("   You must take care to remember what type of file you are installing and\n");
	printf("   fetching. This will be updated in a later release to handle this type of\n");
	printf("   capability, as well as handle multiple 'Schlep' files.\n\n");

	return;
}

int main(int argc, char *argv[])
{
	int 	c,		/* switch */
		sd 		= -1,
	        actions,
		delete 		= 0;
	
	char 	*progname 	= argv[0],
	        *port 		= NULL,
	        *install_filename 	= NULL,
		*fetch_filename 	= NULL;

	poptContext pc;

	struct poptOption options[] = {
		{"port", 'p', POPT_ARG_STRING, &port, 0, "Use device file <port> to communicate with Palm", "port"},
		{"help", 'h', POPT_ARG_NONE, NULL, 'h', "Display this information", NULL},
		{"version", 'v', POPT_ARG_NONE, NULL, 'v', "Show program version information", NULL},
		{"install", 'i', POPT_ARG_STRING, &install_filename, 0, "Pack and install <file> to your Palm", "file"},
		{"fetch", 'f', POPT_ARG_STRING, &fetch_filename, 0, "Unpack the file from your Palm device", "file"},
		{"delete", 'd', POPT_ARG_NONE, &delete, 0, "Delete the packaged file from your Palm device", NULL},
		 POPT_TABLEEND
	};

	port = getenv("PILOTPORT"),

	pc = poptGetContext("pilot-schlep", argc, argv, options, 0);
	while ((c = poptGetNextOpt(pc)) >= 0) {
		switch (c) {

		case 'h':
			display_help(progname);
			return 0;
		case 'v':
			print_splash(progname);
			return 0;
		default:
			display_help(progname);
			return 0;
		}
	}

	if (c < -1) {
             /* an error occurred during option processing */
             fprintf(stderr, "%s: %s\n",
                     poptBadOption(pc, POPT_BADOPTION_NOALIAS),
                     poptStrerror(c));
             return 1;
	}

	actions = (install_filename != NULL) + 
	    (fetch_filename != NULL) +
	    (delete == 1);
	if (actions > 1) {
		fprintf(stderr, "%s: You must specify only one action\n",
		    progname);
		return -1;
	} else if (actions < 1) {
		display_help(progname);
		fprintf(stderr, "%s: You must specify at least one action\n",
		    progname);
		return -1;
	}
		
	if (port == NULL) {
		printf
		    ("\nERROR: At least one command parameter of '-p <port>' must be set, or the\n"
		     "environment variable $PILOTPORT must be if '-p' is omitted or missing.\n");
		return -1;
	}

	sd = pilot_connect(port);
	if (sd < 0)
		goto error;
	
	if (dlp_OpenConduit(sd) < 0)
		goto error_close;

	if (install_filename != NULL) {
		if (Install (sd, install_filename) < 0)
			goto error_close;
	} else if (fetch_filename != NULL) {
		if (Fetch (sd, fetch_filename) < 0)
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
        return -1;
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
