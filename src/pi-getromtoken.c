/* 
 * pi-getromtoken.c:  Retrieve a rom token from a device.
 * 
 * Copyright (C) 2002, Owen Stenseth
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
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <netinet/in.h>

#include "pi-header.h"
#include "pi-source.h"
#include "pi-syspkt.h"
#include "pi-dlp.h"

/* Declare prototypes */
static void display_help(char *progname);
void print_splash(char *progname);
int pilot_connect(char *port);

struct option options[] = {
	{"port",        required_argument, NULL, 'p'},
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
	{"token",       required_argument, NULL, 't'},
	{NULL,          0,                 NULL, 0}
};

static const char *optstring = "p:hvu:i:t:";

static void display_help(char *progname)
{
	printf("   Reads a ROM token from a Palm Handheld device\n\n");
	printf("   Usage: %s -p <port> -t <romtoken>\n\n", progname);
	printf("   Options:\n");
	printf("     -p, --port <port>       Use device file <port> to communicate with Palm\n");
	printf("     -h, --help              Display help information for %s\n", progname);
	printf("     -v, --version           Display %s version information\n", progname);
	printf("     -t <token>              A ROM token to read (i.e. snum)\n\n");
	printf("   Example: %s -p /dev/pilot -t snum\n\n", progname);
	printf("   Other tokens you may currently extract are:\n");
	printf("       adcc:  Entropy for internal A->D convertor calibration\n");
	printf("       irda:  Present only on memory card w/IrDA support\n");
	printf("       snum:  Device serial number (from Memory Card Flash ID)\n\n");

	exit(0);
}

int main(int argc, char *argv[])
{
	int 	c,		/* switch */
		sd 		= -1;
	char 	*progname 	= argv[0],
		*port 		= NULL,
	        *token 		= NULL;

	long    long_token;
	unsigned int size;

	char    buffer[50];
	
	opterr = 0;

	while ((c = getopt_long(argc, argv, optstring, options, NULL)) != -1) {
		switch (c) {

		case 'h':
			display_help(progname);
			return 0;
		case 'v':
			print_splash(progname);
			return 0;
		case 'p':
			port = optarg;
			break;
		case 't':
			token = optarg;
			break;
		}
	}
	
	if(token == NULL) {
		display_help(progname);
		return 0;
	}

	set_long(&long_token, *((long *)token));
	
	sd = pilot_connect(port);
	if (sd < 0)
		goto error;

	if (dlp_GetROMToken(sd, long_token, buffer, &size) < 0)
		goto error_close;
	
	if (dlp_EndOfSync(sd, 0) < 0)
		goto error_close;

	if (pi_close(sd) < 0)
		goto error;

	fprintf(stderr, "Token for '%s' was: %s\n", token, buffer);

	return 0;

 error_close:
	pi_close(sd);
	
 error:
	perror("   ERROR:");
	fprintf(stderr, "\n");

	return -1;
}
