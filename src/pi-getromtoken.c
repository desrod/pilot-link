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

#include "popt.h"
#include <stdio.h>

#include "pi-dlp.h"
#include "pi-header.h"
#include "pi-source.h"
#include "pi-syspkt.h"


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
	printf("   Reads a ROM token from a Palm Handheld device\n\n");
	printf("   Usage: %s -p <port> -t <romtoken>\n\n", progname);
	printf("   Options:\n");
	printf("     -p, --port <port>       Use device file <port> to communicate with Palm\n");
	printf("     -h, --help              Display help information\n");
	printf("     -v, --version           Display version information\n");
	printf("     -t <token>              A ROM token to read (i.e. snum)\n\n");
	printf("   Example: %s -p /dev/pilot -t snum\n\n", progname);
	printf("   Other tokens you may currently extract are:\n");
	printf("       adcc:  Entropy for internal A->D convertor calibration\n");
	printf("       irda:  Present only on memory card w/IrDA support\n");
	printf("       snum:  Device serial number (from Memory Card Flash ID)\n\n");

	return;
}


int main(int argc, char *argv[])
{
	int 	c,		/* switch */
		sd 		= -1;
	char 	*progname 	= argv[0],
		*port 		= NULL,
	        *token 		= NULL;

	long    long_token;
	size_t	size;

	char    buffer[50];
	
	poptContext pc;

	struct poptOption options[] = {
		{"port", 'p', POPT_ARG_STRING, &port, 0,
		 "Use device file <port> to communicate with Palm", "port"},
		{"help", 'h', POPT_ARG_NONE, NULL, 'h',
		 "Display help information", NULL},
		{"version", 'v', POPT_ARG_NONE, NULL, 'v',
		 "Show program version information", NULL},
		{"token", 't', POPT_ARG_STRING, &token, 0,
		 "A ROM token to read (i.e. snum)", "token"},
		POPT_TABLEEND
	};

	pc = poptGetContext("pi-getromtoken", argc, argv, options, 0);

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

	
	if (!token) {
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
	return -1;
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
