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
static void Help(char *progname, char *port);

struct option options[] = {
	{"help", no_argument, NULL, 'h'},
	{"port", required_argument, NULL, 'p'},
	{"user", 0, NULL, 'u'},
	{"userid", 0, NULL, 'i'},
	{NULL, 0, NULL, 0}
};

static const char *optstring = "hp:u:i:";

int main(int argc, char *argv[])
{
	int c;
	int index;
	int sd = -1;
	char *progname = argv[0];
	char *port = NULL;
	char *user = NULL;
	char *userid = NULL;
	struct PilotUser U;
	struct SysInfo S;
	struct CardInfo C;
	struct NetSyncInfo N;
	unsigned long romversion;


	while ((c =
		getopt_long(argc, argv, optstring, options,
			    &index)) != -1) {
		switch (c) {

		  case 'p':
			  port = optarg;
			  break;
		  case 'h':
			  Help(progname, port);
		  case 'u':
			  user = optarg;
			  break;
		  case 'i':
			  userid = optarg;
			  break;
		  case ':':
		}
	}

	if (port == NULL) {
		fprintf(stderr, "You forgot to specify a valid port\n");
		Help(progname, port);
		exit(1);
	} else if (port != NULL) {
		sd = pilot_connect(port);
		dlp_ReadUserInfo(sd, &U);
		dlp_ReadSysInfo(sd, &S);
		C.card = -1;
		C.more = 1;
		while (C.more) {
			if (dlp_ReadStorageInfo(sd, C.card + 1, &C) < 0)
				break;
			printf
			    ("\n   Card #%d has %lu bytes of ROM, and %lu bytes of RAM (%lu of that is free)\n",
			     C.card, C.romSize, C.ramSize, C.ramFree);
			printf
			    ("   It is called '%s', and was made by '%s'.\n",
			     C.name, C.manufacturer);
		}

		if (user != NULL && userid != NULL) {
			strcpy(U.username, user);
			if (user && userid) {
				U.userID = atoi(userid);
			}
			U.lastSyncDate = time((time_t *) 0);
			dlp_WriteUserInfo(sd, &U);
		} else {
			printf("   Palm username: %s\n", U.username);
			printf("   Palm UserID  : %ld \n", U.userID);
		}
	}

	printf
	    ("\n   Through ReadSysInfo: ROM Version: 0x%8.8lX, locale: 0x%8.8lX, name: '%s'\n",
	     S.romVersion, S.locale, S.name);
	dlp_ReadFeature(sd, makelong("psys"), 1, &romversion);
	printf("   ROM Version through ReadFeature: 0x%8.8lX\n", romversion);

	if (dlp_ReadNetSyncInfo(sd, &N) >= 0) {
		printf
		    ("   NetSync: LAN sync = %d, Host name = '%s', address = '%s', netmask ='%s'\n\n",
		     N.lanSync, N.hostName, N.hostAddress,
		     N.hostSubnetMask);
	}
	pi_close(sd);
	exit(0);

}

static void Help(char *progname, char *port)
{
	PalmHeader(progname);

	fprintf(stderr,
		"   Assigns your Palm device a Username and unique UserID\n");
	fprintf(stderr,
		"   Usage: %s -p /dev/ttyS0 -u \"User name\" -i <userid>\n\n",
		progname);
	fprintf(stderr,
		"   Example: %s -p /dev/ttyS0 -u \"John Q. Public\" -i 12345\n\n",
		progname);
}
