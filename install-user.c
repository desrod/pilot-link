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
	{"user",        required_argument, NULL, 'u'},
	{"userid",      required_argument, NULL, 'i'},
	{"hostname",    required_argument, NULL, 'o'},
	{"address",     required_argument, NULL, 'a'},
	{"netmask",     required_argument, NULL, 'n'},
	{NULL,          0,                 NULL, 0}
};

static const char *optstring = "hp:u:i:o:a:n:l:";

int main(int argc, char *argv[])
{
	int 	count,
		sd 		= -1,
		opterr;
	char 	*progname 	= argv[0],
		*port 		= NULL,
		*user 		= NULL,
		*userid 	= NULL,
		*hostname 	= NULL,
		*address 	= NULL,
		*netmask 	= NULL,
		*netsync 	= NULL;

	struct 	PilotUser 	User;
	struct 	SysInfo 	Sys;
	struct 	CardInfo 	Card;
	struct 	NetSyncInfo 	Net;

	unsigned long romversion;

	opterr = 0;

	while ((count = getopt(argc, argv, optstring)) != -1) {
		switch (count) {

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
		  case 'o':
			  hostname = optarg;
			  break;
		  case 'a':
			  address = optarg;
			  break;
		  case 'n':
			  netmask = optarg;
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
		if (!user && userid) {
			Help(progname);
			printf
			    ("ERROR: You forgot to specify a valid username\n");
			exit(1);
		}
		if (user && !userid) {
			Help(progname);
			printf
			    ("ERROR: You forgot to specify a valid numeric UserID\n");
			exit(1);
		}
		sd = pilot_connect(port);

		/* Did we get a valid socket descriptor back? */
		if (dlp_OpenConduit(sd) < 0) {
			exit(1);
		} else {
			dlp_ReadUserInfo(sd, &User);
			dlp_ReadSysInfo(sd, &Sys);

			Card.card = -1;
			Card.more = 1;

			while (Card.more) {
				if (dlp_ReadStorageInfo(sd, Card.card + 1, &Card)
				    < 0)
					break;
				printf
				    ("   Card #%d has %lu bytes of ROM, and %lu bytes of RAM (%lu of that is free)\n",
				     Card.card, Card.romSize, Card.ramSize,
				     Card.ramFree);
				printf
				    ("   It is called '%s', and was made by '%s'.\n",
				     Card.name, Card.manufacturer);
			}


			/* Let's make sure we have valid arguments for these
			   before we write the data to the Palm */
			if (user != NULL && userid != NULL) {
				strncpy(User.username, user,
					sizeof(User.username) - 1);
				if (user && userid) {
					/* atoi should go away here, replace with strtoul() */
					User.userID = atoi(userid);
				}

				User.lastSyncDate = time((time_t *) 0);

				/* Write the data to the Palm device */
				dlp_WriteUserInfo(sd, &User);
			}

			printf("   Palm username: %s\n", User.username);
			printf("   Palm UserID  : %ld \n", User.userID);
			printf
			    ("\n   Values read through ReadSysInfo:\n   ROM Version: 0x%8.8lX, locale: 0x%8.8lX, name: '%s'\n",
			     Sys.romVersion, Sys.locale, Sys.name);
			dlp_ReadFeature(sd, makelong("psys"), 1,
					&romversion);
			printf
			    ("   ROM Version through ReadFeature: 0x%8.8lX\n",
			     romversion);


			/* Read and write the LanSync data to the Palm device */
			if (dlp_ReadNetSyncInfo(sd, &Net) >= 0) {
				if (Net.lanSync == 0) {
					netsync = "Local HotSync";
				} else if (Net.lanSync == 1) {
					netsync = "LANSync";
				}

				if (hostname != NULL)
					strncpy(Net.hostName, hostname,
						sizeof(Net.hostName));
				if (address != NULL)
					strncpy(Net.hostAddress, address,
						sizeof(Net.hostAddress));
				if (netmask != NULL)
					strncpy(Net.hostSubnetMask, netmask,
						sizeof(Net.hostSubnetMask));
				dlp_WriteNetSyncInfo(sd, &Net);

				printf("\n");
				printf("   NetSync:     = '%s'\n"
				       "   Host name    = '%s'\n"
				       "   IP address   = '%s'\n"
				       "   Netmask      = '%s'\n\n",
				       netsync, Net.hostName, Net.hostAddress,
				       Net.hostSubnetMask);
			}
			dlp_AddSyncLogEntry(sd, "install-user, exited normally.\n"
						"Thank you for using pilot-link.\n");
			dlp_EndOfSync(sd, 0);
			pi_close(sd);
			return 0;
		}
	}
	return 0;
}

static void Help(char *progname)
{
	printf("   Assigns your Palm device a Username and unique UserID and can query\n"
	       "   the device's Card Info\n\n"
	       "   Usage: %s -p <port> -u \"User name\" -i <userid>\n"
	       "                       -o <hostname> -a <ip> -n <subnet>\n\n"
	       "   Options:\n"
	       "     -p <port>         Use device file <port> to communicate with Palm\n"
	       "     -u <username>     Your username, use quotes for spaces (see example)\n"
	       "     -i <userid>       A 5-digit numeric UserID, required for PalmOS\n"
	       "     -o <hostname>     The hostname of the desktop you are syncing with\n"
	       "     -a <ip address>   IP address of the machine you connect your Palm to\n"
	       "     -n <netmask>      The subnet mask of the network your Palm is on\n"
	       "     -h                Display this information\n\n"
	       "   Only the port option is required, the other options are... optional.\n\n"
	       "   Examples: %s -p /dev/pilot -u \"John Q. Public\" -i 12345\n"
	       "             %s -p /dev/pilot -o Host -a 192.168.1.1 -n 255.255.255.0\n\n",
	       progname, progname, progname);
	return;
}
