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

#include "getopt.h"
#include <stdio.h>
#include <stdlib.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-header.h"

int pilot_connect(const char *port);
static void Help(char *progname);

struct option options[] = {
	{"help",        no_argument,       NULL, 'h'},
	{"port",        required_argument, NULL, 'p'},
	{NULL,          0,                 NULL, 0}
};

static const char *optstring = "hp:";

int main(int argc, char *argv[])
{
	int 	chara
		len,
		ret,
		sd = -1,
		sd2 = -1, 	/* This is the network socket */

	char 	buffer[0xffff],
		*progname = argv[0],
		*port = NULL;
	
	struct 	sockaddr_in 	addr2;
	struct 	NetSyncInfo 	Net;

	while ((chara = getopt(argc, argv, optstring)) != -1) {
		switch (chara) {

		  case 'h':
			  Help(progname);
			  exit(0);
		  case 'p':
			  port = optarg;
			  break;
		  default:
		}
	}

	if (argc < 2 && !getenv("PILOTPORT")) {
		PalmHeader(progname);
	} else if (port == NULL && getenv("PILOTPORT")) {
		port = getenv("PILOTPORT");
	}

	if (port == NULL && argc > 1) {
		printf
		    ("\nERROR: At least one command parameter of '-p <port>' must be set, or the\n"
		     "environment variable $PILOTPORT must be used if '-p' is omitted or missing.\n");
		exit(1);
	} else if (port != NULL) {
		
		sd = pilot_connect(port);

		/* Did we get a valid socket descriptor back? */
		if (dlp_OpenConduit(sd) < 0) {
			exit(1);
		}

		if (dlp_ReadNetSyncInfo(sd, &Net) < 0) {
			fprintf(stderr,
				"Unable to read network information, cancelling sync.\n");
			exit(1);
		}
	
		if (!Net.lanSync) {
			fprintf(stderr,
				"LAN Sync not enabled on your Palm, cancelling sync.\n");
			exit(1);
		}
	
		/* DEBUG log for this Network Hotsync session */
		putenv("PILOTLOGFILE=PiDebugNet.log");
	
		sd2 = pi_socket(AF_INET, PI_SOCK_STREAM, 0);
		if (sd2 < 0) {
			perror("Unable to get socket 2");
			exit(1);
		}
		printf("Got socket 2\n");
	
		memset(&addr2, 0, sizeof(addr2));
		addr2.sin_family = AF_INET;
		addr2.sin_port = htons(14238);
	
		if ((addr2.sin_addr.s_addr = inet_addr(Net.hostAddress)) == -1) {
			fprintf(stderr, "Unable to parse PC address '%s'\n",
				Net.hostAddress);
			exit(1);
		}
	
		ret = pi_connect(sd2, (struct sockaddr *) &addr2, sizeof(addr2));
	
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
		dlp_AddSyncLogEntry(sd, "pi-nredir, exited normally.\n"
					"Thank you for using pilot-link.\n");
		dlp_EndOfSync(sd, 0);
		pi_close(sd);
		close(sd2);
	}
	return 0;
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
