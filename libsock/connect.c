/*
 * connect.c:  Palm Serial, USB, IR connection routines
 *
 * Copyright (c) 2001, David A. Desrosiers
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. *
 */

#include <stdio.h>
#include <stdlib.h>

#include "pi-socket.h"
#include "pi-header.h"
#include "pi-dlp.h"

int pilot_connect(const char *port)
{
	int sd;
	struct PilotUser U;
	struct pi_sockaddr addr;

	if (!(sd = pi_socket(PI_AF_PILOT, PI_SOCK_STREAM, PI_PF_NET))) {
		perror("   Reason: pi_socket");
		return 1;
	}

	addr.pi_family = PI_AF_PILOT;
	strncpy(addr.pi_device, port, sizeof(addr.pi_device));

	if (pi_bind(sd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		printf("\n   Unable to bind to port '%s'\n", port);
		perror("   Reason: pi_bind");
		printf("\n");
		pi_close(sd);
		return 1;
	}

	printf
	    ("\n   Connecting to port: %s\n\n   Please press the HotSync button now...\n",
	     port);

	if (pi_listen(sd, 1) == -1) {
		printf("\n   Error listening on %s\n", port);
		perror("   Reason: pi_listen");
		printf("\n");
		pi_close(sd);
		return 1;
	}

	if (pi_accept(sd, 0, 0) == -1) {
		printf("\n   Error accepting data on %s\n", port);
		perror("   Reason: pi_accept");
		printf("\n");
		pi_close(sd);
		return 1;
	}

	printf("   Connected...\n");

	/* Ask the pilot who it is. */
	dlp_ReadUserInfo(sd, &U);

	/* Tell user (via Palm) that we are starting things up */
	dlp_OpenConduit(sd);

	return sd;
}
