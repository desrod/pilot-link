/*
 * pi-nredir.c: Redirect a connection over the network
 *
 * Copyright (C) 1997, Kenneth Albanowski
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
#include <string.h>

#include "pi-source.h"
#include "pi-dlp.h"
#include "pi-header.h"


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
	printf("   Accept connection and redirect via Network Hotsync Protocol\n\n");
	printf("   Usage: %s -p <port>\n\n", progname);
	printf("   Options:\n");
	printf("     -p --port <port>    Use device file <port> to communicate with Palm\n");
	printf("     -h, --help          Display help information\n");   
	printf("     -v, --version       Display version information\n\n");  
	printf("   Examples: %s -p serial:/dev/ttyUSB0\n\n", progname);
	printf("   This will bind your locally connected device to a network port, and\n");
	printf("   redirect them through the network device to a listening server as\n");
	printf("   specified in the LANSync Preferences panel on your Palm.\n\n");

	return;
}


int main(int argc, const char *argv[])
{
	int 	c,		/* switch */
		len,
		sd 		= -1,
		sd2 		= -1, /* This is the network socket */
		state;

	size_t	size; 	

	const char
                *progname = argv[0];

	char	*port = NULL,
		port2[255] = "net:";

	pi_buffer_t *buffer;

	struct 	NetSyncInfo 	Net;

	poptContext po;
	
	struct poptOption options[] = {
        	{"port", 	'p', POPT_ARG_STRING, &port, 0, "Use device <port> to communicate with Palm"},
	        {"help", 	'h', POPT_ARG_NONE, NULL, 'h', "Display this information"},
                {"version", 	'v', POPT_ARG_NONE, NULL, 'v', "Display version information"},
	        POPT_AUTOHELP
                { NULL, 0, 0, NULL, 0 }
	};
	
	po = poptGetContext("pi-nredir", argc, argv, options, 0);

	while ((c = poptGetNextOpt(po)) >= 0) {
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
		default:
			display_help(progname);
			return 0;
		}
	}

	sd = pilot_connect(port);
	if (sd < 0)
		goto error;

	if (dlp_ReadNetSyncInfo(sd, &Net) < 0) {
		fprintf(stderr,
			"Unable to read network information, cancelling sync.\n");
		exit(EXIT_FAILURE);
	}
	
	if (!Net.lanSync) {
		fprintf(stderr,
			"LANSync not enabled on your Palm, cancelling sync.\n");
		exit(EXIT_FAILURE);
	}
	
	sd2 = pi_socket(PI_AF_PILOT, PI_SOCK_STREAM, PI_PF_NET);
	if (sd2 < 0)
		goto error_close;
	
	strncat(port2, Net.hostAddress, 251 - strlen(Net.hostAddress));
	
	printf("\tTrying %s... ", Net.hostAddress);
	fflush(stdout);
	if (pi_connect(sd2, port2) < 0) {
		printf("Failed\n");
		goto error;
	}
	printf("Connected\n");
	
	buffer = pi_buffer_new (0xffff);

	while ((len = pi_read(sd2, buffer, 0xffff)) > 0) {
		pi_write(sd, buffer->data, len);
		buffer->used = 0;
		len = pi_read(sd, buffer, 0xffff);
		if (len < 0)
			break;
		pi_write(sd2, buffer->data, len);
		buffer->used = 0;
	}
	
	pi_buffer_free (buffer);
	
	state = PI_SOCK_CONEN;
	size = sizeof (state);
	pi_setsockopt (sd, PI_LEVEL_SOCK, PI_SOCK_STATE, &state, &size);
	pi_setsockopt (sd2, PI_LEVEL_SOCK, PI_SOCK_STATE, &state, &size);
	
	pi_close(sd);
	pi_close(sd2);

	return 0;

 error_close:
	pi_close(sd);

 error:
	return -1;
}
