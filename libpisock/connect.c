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
#include <errno.h>
#include <sys/stat.h>

#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-header.h"

/***********************************************************************
 *
 * Function:    pilot_connect
 *
 * Summary:     Connect to a Palm device.
 *
 * Parameters:  port. Communications port through which the Palm device is
 *              connected.  If this is NULL, pilot_connect() will attempt to
 *              discover the correct port, and fall back to /dev/pilot.
 *
 * Returns:     Socket descriptor of type 'client_sd', if successful.
 *		Returns 1, if the connection can not be established.
 *
 *  If 'port' is NULL, the PILOTPORT environment variable is checked.
 *
 *  If neither of them are set, the port defaults to /dev/pilot.
 *
 *  A socket is created. A message is displayed, reminding the user to press
 *  the HotSync button. pilot_connect() waits for communication to be
 *  established... potentially forever. Once communication is established,
 *  the socket descriptor is returned.
 *
 ***********************************************************************/
int pilot_connect(char *port)
{
	int 	parent_sd	= -1, 	/* Client socket, formerly sd	*/
		client_sd	= -1,	/* Parent socket, formerly sd2	*/
		result, 
		count	= 0,
		err	= 0;
	struct 	pi_sockaddr addr;
	struct 	stat attr;
	struct  SysInfo sys_info;
	char 	*defport = "/dev/pilot";

	if (port == NULL && (port = getenv("PILOTPORT")) == NULL) {
		fprintf(stderr, "   No $PILOTPORT specified and no -p "
			"<port> given.\n"
			"   Defaulting to '%s'\n", defport);
		port = defport;
		err = stat(port, &attr);
	}

	if (err) {
		fprintf(stderr, "   ERROR: %s (%d)\n\n", strerror(errno), errno);
		fprintf(stderr, "   Error accessing: '%s'. Does '%s' exist?\n",
		       port, port);
		fprintf(stderr, "   Please use --help for more information\n\n");
		exit(1);
	}

	fprintf(stderr, "\n");
	begin:
	if (!(parent_sd = pi_socket(PI_AF_PILOT, PI_SOCK_STREAM, PI_PF_DLP))) {
		fprintf(stderr, "\n   Unable to create socket '%s'\n",
			port ? port : getenv("PILOTPORT"));
		return -1;
	}

	if (port != NULL) {
		addr.pi_family = PI_AF_PILOT;
		strncpy(addr.pi_device, port, sizeof(addr.pi_device));
		result =
		    pi_bind(parent_sd, (struct sockaddr *) &addr, sizeof(addr));
	} else {
		result = pi_bind(parent_sd, NULL, 0);
	}


	if (result < 0) {
		int 	save_errno = errno;
		char 	*portname;

		portname = (port != NULL) ? port : getenv("PILOTPORT");


		if (portname) {
			char realport[50];
			realpath(portname, realport);
			errno = save_errno;

			if (errno == 2) {
				fprintf(stderr, "   The device %s does not exist..\n",
					portname);
				fprintf(stderr, "   Possible solution:\n\n\tmknod %s c "
					"<major> <minor>\n\n", portname);

			} else if (errno == 13) {
				fprintf(stderr, "   Please check the "
					"permissions on %s..\n", realport);
				fprintf(stderr, "   Possible solution:\n\n\tchmod 0666 "
					"%s\n\n", realport);

			} else if (errno == 19) {
				while (count <= 5) {
					fprintf(stderr, "\r   Port not connected, sleeping for 2 seconds, ");
					fprintf(stderr, "%d retries..", 5-count);
					sleep(2);
					count++;
					goto begin;
				}
				fprintf(stderr, "\n\n   Device not found on %s, Did you hit HotSync?\n\n", realport);

			} else if (errno == 21) {
				fprintf(stderr, "   The port specified must contain a "
					"device name, and %s was a directory.\n"
					"   Please change that to reference a real "
					"device, and try again\n\n", portname);
			}

			fprintf(stderr, "   Unable to bind to port: %s\n", 
				portname);
	                fprintf(stderr, "   Please use --help for more "
				"information\n\n");
		} else
			fprintf(stderr, "\n   No port specified\n");
		return -1;
	}

	fprintf(stderr,
		"\n   Listening to port: %s\n\n   Please press the HotSync "
		"button now... ",
		port ? port : getenv("PILOTPORT"));

	if (pi_listen(parent_sd, 1) == -1) {
		fprintf(stderr, "\n   Error listening on %s\n", port);
		pi_close(parent_sd);
		pi_close(client_sd);
		return -1;
	}

	client_sd = pi_accept(parent_sd, 0, 0);
	if (client_sd == -1) {
		fprintf(stderr, "\n   Error accepting data on %s\n", port);
		pi_close(parent_sd);
		pi_close(client_sd);
		return -1;
	}

	fprintf(stderr, "Connected\n\n");

	if (dlp_ReadSysInfo(client_sd, &sys_info) < 0) {
		fprintf(stderr, "\n   Error read system info on %s\n", port);
		pi_close(parent_sd);
		pi_close(client_sd);
		return -1;
	}

	dlp_OpenConduit(client_sd);
	return client_sd;
}
