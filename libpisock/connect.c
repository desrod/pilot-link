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
#include <string.h>

#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-header.h"

/* Declare prototypes */
int pilot_connect(const char *port, struct PilotUser *User, 
		  struct SysInfo *Sys, struct NetSyncInfo *Net, 
		  struct CardInfo *Card);

int pilot_connect(const char *port, struct PilotUser *User, 
                  struct SysInfo *Sys, struct NetSyncInfo *Net, 
                  struct CardInfo *Card)
{
	int 	sd;
	struct 	pi_sockaddr addr;

	if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP))) {
		perror("   Reason: pi_socket");
		return -1;
	}

	addr.pi_family = PI_AF_SLP;
	strncpy(addr.pi_device, port, sizeof(addr.pi_device));

	if (pi_bind(sd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		fprintf(stderr, "\n   Unable to bind to port '%s'\n", port);
		perror("   Reason: pi_bind");
		fprintf(stderr, "\n");
		pi_close(sd);
		return -1;
	}

	fprintf(stderr, "\n   Connecting to port: %s\n\n   Please press the HotSync button now...\n",
	     port);

	if (pi_listen(sd, 1) == -1) {
		fprintf(stderr, "\n   Error listening on %s\n", port);
		perror("   Reason: pi_listen");
		fprintf(stderr, "\n");
		pi_close(sd);
		return -1;
	}

	sd = pi_accept(sd, 0, 0);
	if (sd == -1) {
		fprintf(stderr, "\n   Error accepting data on %s\n", port);
		perror("   Reason: pi_accept");
		fprintf(stderr, "\n");
		pi_close(sd);
		return -1;
	}

	fprintf(stderr, "   Connected...\n\n");

	/* Tell user (via Palm) that we are starting things up */
	dlp_OpenConduit(sd);

	dlp_ReadUserInfo(sd, User);
	dlp_ReadSysInfo(sd, Sys);
	dlp_ReadNetSyncInfo(sd, Net);
	dlp_ReadStorageInfo(sd, 0, Card);

	return sd;
}
