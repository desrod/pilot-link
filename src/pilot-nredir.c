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

#include "getopt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-header.h"

int pilot_connect(const char *port);
static void Help(char *progname);

struct option options[] = {
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
	{"port",        required_argument, NULL, 'p'},
	{NULL,          0,                 NULL, 0}
};

static const char *optstring = "hvp:";

int main(int argc, char *argv[])
{
	int 	count,
		len,
		sd = -1,
		sd2 = -1; 	/* This is the network socket */

	char 	buffer[0xffff],
		*progname = argv[0],
		*port = NULL;
	
	struct 	pi_sockaddr 	addr;
	struct 	NetSyncInfo 	Net;

	while ((count = getopt_long(argc, argv, optstring, options, NULL)) != -1) {
		switch (count) {

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

	sd = pilot_connect(port);
	if (sd < 0)
		goto error;

	if (dlp_ReadNetSyncInfo(sd, &Net) < 0) {
		fprintf(stderr,
			"Unable to read network information, cancelling sync.\n");
		exit(1);
	}
	
	if (!Net.lanSync) {
		fprintf(stderr,
			"LANSync not enabled on your Palm, cancelling sync.\n");
		exit(1);
	}
	
	sd2 = pi_socket(PI_AF_PILOT, PI_SOCK_STREAM, PI_PF_NET);
	if (sd2 < 0)
		goto error_close;
	
	memset(&addr, 0, sizeof(addr));
	addr.pi_family = PI_AF_PILOT;
	strcpy(addr.pi_device, "net:");
	strcpy(addr.pi_device + 4, Net.hostAddress);
	
	printf("\tTrying %s... ", Net.hostAddress);
	fflush(stdout);
	if (pi_connect(sd2, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		printf("Failed\n");
		goto error;
	}
	printf("Connected\n");
	
	while ((len = pi_read(sd2, buffer, 0xffff)) > 0) {
		pi_write(sd, buffer, len);
		len = pi_read(sd, buffer, 0xffff);
		if (len < 0)
			break;
		pi_write(sd2, buffer, len);
	}

	pi_close(sd);
	pi_close(sd2);

	return 0;

 error_close:
	pi_close(sd);

 error:
	perror("\tERROR:");
	fprintf(stderr, "\n");

	return -1;
}

static void Help(char *progname)
{
	printf("   Accept connection and redirect via Network Hotsync Protocol\n\n"
	       "   Usage: %s -p <port>\n\n"
	       "   Options:\n"
	       "     -p <port>           Use device file <port> to communicate with Palm\n"
	       "     -h                  Display this information\n\n"
	       "   Examples: %s -p /dev/pilot\n\n"
	       "   This will bind your locally connected device to a network port,and\n"
	       "   redirect them through the network device to a listening server as\n"
	       "   specified in the LANSync Preferences panel on your Palm.\n\n",
	       progname, progname);
	return;
}
