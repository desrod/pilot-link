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
#include "pi-util.h"
#include "userland.h"



int main(int argc, const char *argv[])
{
	int 	c,		/* switch */
		sd 		= -1;

	const char
                *progname 	= argv[0],
		*port 		= NULL,
	        *token 		= NULL;

	long    long_token;
	size_t	size;

	char    buffer[50];

	poptContext pc;

	struct poptOption options[] = {
		USERLAND_RESERVED_OPTIONS
		{"token", 't', POPT_ARG_STRING, &token, 0,
		 "A ROM token to read (i.e. snum)", "token"},
		POPT_TABLEEND
	};

	pc = poptGetContext("pi-getromtoken", argc, argv, options, 0);
	poptSetOtherOptionHelp(pc,"\n\n",
		"   Reads a ROM token from a Palm Handheld device.\n"
		"   Tokens you may currently extract are:\n"
		"       adcc:  Entropy for internal A->D convertor calibration\n"
		"       irda:  Present only on memory card w/IrDA support\n"
		"       snum:  Device serial number (from Memory Card Flash ID)\n\n"
		"   Example arguments:\n"
		"      -p /dev/pilot -t snum\n\n");

	if (argc < 2) {
		poptPrintUsage(pc,stderr,0);
		return 1;
	}

	while ((c = poptGetNextOpt(pc)) >= 0) {
		fprintf(stderr,"   ERROR: Unhandled option %d.\n",c);
		return 1;
	}

	if (c < -1) {
		plu_badoption(pc,c);
	}


	if (!token) {
		fprintf(stderr,"   ERROR: Must specify a token.\n");
		return 1;
	}

	long_token = pi_mktag(token);

	sd = plu_connect();

	if (sd < 0)
		goto error;

	if (dlp_GetROMToken(sd, long_token, buffer, &size) < 0)
		goto error_close;

	if (dlp_EndOfSync(sd, 0) < 0)
		goto error_close;

	if (pi_close(sd) < 0)
		goto error;

	fprintf(stdout, "Token for '%s' was: %s\n", token, buffer);

	return 0;

error_close:
	pi_close(sd);

error:
	return -1;
}

/* vi: set ts=4 sw=4 sts=4 noexpandtab: cin */
