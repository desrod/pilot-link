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

#ifdef __EMX__
# include <sys/types.h>
# include <netinet/in.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "pi-debug.h"
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-header.h"

int main(int argc, char *argv[])
{
	int len;
	int ret;
	int sd;
	int sd2;
	struct pi_sockaddr addr;
	struct NetSyncInfo N;
	char buffer[0xffff];
	char *progname = argv[0];
	char *device = argv[1];

	PalmHeader(progname);

	if (argc < 2) {
		fprintf(stderr, "   Usage: %s %s\n\n", argv[0], TTYPrompt);
		exit(2);
	}

	if (!(sd = pi_socket(PI_AF_PILOT, PI_SOCK_STREAM, PI_PF_PADP))) {
		perror("pi_socket");
		exit(1);
	}

	addr.pi_family = PI_AF_PILOT;
	strcpy(addr.pi_device, device);

	ret = pi_bind(sd, (struct sockaddr *) &addr, sizeof(addr));
	if (ret == -1) {
		fprintf(stderr, "\n   Unable to bind to port %s\n",
			device);
		perror("   pi_bind");
		fprintf(stderr, "\n");
		exit(1);
	}

	printf
	    ("   Port: %s\n\n   Please press the HotSync button now...\n",
	     device);

	ret = pi_listen(sd, 1);
	if (ret == -1) {
		fprintf(stderr, "\n   Error listening on %s\n", device);
		perror("   pi_listen");
		fprintf(stderr, "\n");
		exit(1);
	}

	sd = pi_accept(sd, 0, 0);
	if (sd == -1) {
		fprintf(stderr, "\n   Error accepting data on %s\n",
			device);
		perror("   pi_accept");
		fprintf(stderr, "\n");
		exit(1);
	}

	fprintf(stderr, "Connected...\n");

	if (dlp_ReadNetSyncInfo(sd, &N) < 0) {
		fprintf(stderr,
			"Unable to read network information, cancelling sync.\n");
		exit(1);
	}

	if (!N.lanSync) {
		fprintf(stderr,
			"LAN Sync not enabled on your Palm, cancelling sync.\n");
		exit(1);
	}

	sd2 = pi_socket(PI_AF_PILOT, PI_SOCK_STREAM, PI_PF_NET);
	if (sd2 < 0) {
		perror("Unable to get socket 2");
		exit(1);
	}

	memset(&addr, 0, sizeof(addr));
	addr.pi_family = PI_AF_PILOT;
	strcpy(addr.pi_device, "net:");
	strcpy(addr.pi_device + 4, N.hostAddress);
	
	ret = pi_connect(sd2, (struct sockaddr *) &addr, sizeof(addr));

	if (ret < 0) {
		perror("Unable to connect to PC");
		exit(1);
	}

	while ((len = pi_read(sd2, buffer, 0xffff)) > 0) {
		pi_write(sd, buffer, len);
		len = pi_read(sd, buffer, 0xffff);
		if (len < 0)
			break;
		pi_write(sd2, buffer, len);
	}

	dlp_AbortSync(sd);
	close(sd2);

	return 0;
}
