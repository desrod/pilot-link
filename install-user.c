/* 
 * install-user.c:  Palm Username installer
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

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-header.h"

int pilot_connect(char const *port);
static void Help(char *progname);

struct option options[] = {
	{"help",     no_argument,       NULL, 'h'},
	{"port",     required_argument, NULL, 'p'},
	{"user",     required_argument, NULL, 'u'},
	{"userid",   required_argument, NULL, 'i'},
	{"hostname", required_argument, NULL, 'n'},
	{"address",  required_argument, NULL, 'a'},
	{"netmask",  required_argument, NULL, 'm'},
	{NULL,       0,                 NULL, 0}
};

static const char *optstring = "hp:u:i:n:a:m:l:";

int main(int argc, char *argv[])
{
	int c;
	int index;
	int sd = -1;
	char *progname = argv[0];
	char *port = NULL;
	char *user = NULL;
	char *userid = NULL;
	char *hostname = NULL;
	char *address = NULL;
	char *netmask = NULL;
	char *netsync = NULL;
	struct PilotUser U;
	struct SysInfo S;
	struct CardInfo C;
	struct NetSyncInfo N;
	unsigned long romversion;

	while ((c =
		getopt_long(argc, argv, optstring, options,
			    &index)) != -1) {
		switch (c) {

		  case 'h':
			  Help(progname);
			  exit(0);
		  case 'p':
			  port = optarg;
			  break;
		  case 'u':
			  user = optarg;
			  break;
		  case 'i':
			  userid = optarg;
			  break;
		  case 'n':
			  hostname = optarg;
			  break;
		  case 'a':
			  address = optarg;
			  break;
		  case 'm':
			  netmask = optarg;
			  break;
		  case ':':
		}
	}

	if (port == NULL) {
		fprintf(stderr,
			"\n** ERROR: You forgot to specify a valid port\n");
		Help(progname);
		exit(1);
	} else if (port != NULL) {
		if (!user && userid) {
			fprintf(stderr,
				"\n** ERROR: You forgot to specify a valid username\n");
			Help(progname);
			exit(1);
		}
		if (user && !userid) {
			fprintf(stderr,
				"\n** ERROR: You forgot to specify a valid numeric UserID\n");
			Help(progname);
			exit(1);
		}
		sd = pilot_connect(port);
		if (dlp_OpenConduit(sd) < 0) {
			exit(1);
		} else {
			dlp_ReadUserInfo(sd, &U);
			dlp_ReadSysInfo(sd, &S);
			C.card = -1;
			C.more = 1;
			while (C.more) {
				if (dlp_ReadStorageInfo(sd, C.card + 1, &C)
				    < 0)
					break;
				printf
				    ("\n   Card #%d has %lu bytes of ROM, and %lu bytes of RAM (%lu of that is free)\n",
				     C.card, C.romSize, C.ramSize,
				     C.ramFree);
				printf
				    ("   It is called '%s', and was made by '%s'.\n",
				     C.name, C.manufacturer);
			}
			if (user != NULL && userid != NULL) {
				strncpy(U.username, user,
					sizeof(U.username) - 1);
				if (user && userid) {
					U.userID = atoi(userid);
				}

				U.lastSyncDate = time((time_t *) 0);

				dlp_WriteUserInfo(sd, &U);
			}

			printf("   Palm username: %s\n", U.username);
			printf("   Palm UserID  : %ld \n", U.userID);
			printf
			    ("\n   Values read through ReadSysInfo:\n   ROM Version: 0x%8.8lX, locale: 0x%8.8lX, name: '%s'\n",
			     S.romVersion, S.locale, S.name);
			dlp_ReadFeature(sd, makelong("psys"), 1,
					&romversion);
			printf
			    ("   ROM Version through ReadFeature: 0x%8.8lX\n",
			     romversion);


			if (dlp_ReadNetSyncInfo(sd, &N) >= 0) {
				if (N.lanSync == 0) {
					netsync = "Local HotSync";
				} else if (N.lanSync == 1) {
					netsync = "LANSync";
				}

				if (hostname != NULL)
					strncpy(N.hostName, hostname,
						sizeof(N.hostName));
				if (address != NULL)
					strncpy(N.hostAddress, address, 
						sizeof(N.hostAddress));
				if (netmask != NULL)
					strncpy(N.hostSubnetMask, netmask,
						sizeof(N.hostSubnetMask));
				dlp_WriteNetSyncInfo(sd, &N);

				printf("\n");
				printf("   NetSync:     = '%s'\n", netsync);
				printf("   Host name    = '%s'\n", N.hostName);
				printf("   IP address   = '%s'\n", N.hostAddress);
				printf("   Netmask      = '%s'\n\n", N.hostSubnetMask);
			}
			pi_close(sd);
			exit(0);
		}
	}
	return 0;
}

static void Help(char *progname)
{
	PalmHeader(progname);
	fprintf(stderr, "   Assigns your Palm device a Username and unique UserID and can query the\n");
	fprintf(stderr, "   device's Card Info\n\n");
	fprintf(stderr, "   Usage: %s -p <port> -u \"User name\" -i <userid>\n", progname);
	fprintf(stderr, "                       -n <hostname> -a <ip> -m <subnet>\n\n");
	fprintf(stderr, "   Only the port option is required, the other options are... optional.\n\n");
	fprintf(stderr, "   -p <port>       = use device file <port> to communicate with Palm\n");
	fprintf(stderr, "   -u <user>       = your username, use quotes for spaces (see example)\n");
	fprintf(stderr, "   -i <userid>     = a 5-digit numeric UserID, required for PalmOS\n");
	fprintf(stderr, "   -n <hostname>   = the hostname of the desktop you are syncing with\n");
	fprintf(stderr, "   -a <ip address> = ip address of the machine you connect your Palm to\n");
	fprintf(stderr, "   -m <netmask>    = the subnet mask of the network your Palm is on\n\n");
	fprintf(stderr, "   Example: %s -p /dev/ttyS0 -u \"John Q. Public\" -i 12345\n\n", progname);

	exit(0);
}
