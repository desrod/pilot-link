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
#include <string.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-header.h"

int pilot_connect(const char *port);

struct option options[] = {
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
	{"port",        required_argument, NULL, 'p'},
	{"enable",      no_argument,       NULL, 'e'},
	{"hostname",    required_argument, NULL, 'H'},
	{"address",     required_argument, NULL, 'a'},
	{"netmask",     required_argument, NULL, 'n'},
	{NULL,          0,                 NULL, 0}
};

static const char *optstring = "hvp:eH:a:n:l:";

static void Help(char *progname)
{
	printf("   Assigns your Palm device NetSync information\n\n"
	       "   Usage: %s -p <port> -H <hostname> -a <ip> -n <subnet>\n\n"
	       "   Options:\n"
	       "     -p <port>         Use device file <port> to communicate with Palm\n"
	       "     -e <hostname>     Enables LANSync on the Palm\n"
	       "     -H <hostname>     The hostname of the desktop you are syncing with\n"
	       "     -a <ip address>   IP address of the machine you connect your Palm to\n"
	       "     -n <netmask>      The subnet mask of the network your Palm is on\n"
	       "     -h, --help        Display this information\n"
	       "     -v, --version     Display version information\n\n"
	       "   Examples: %s -p /dev/pilot -H \"localhost\" -a 127.0.0.1 -n 255.255.255.0\n\n",
	       progname, progname);
	return;
}

int main(int argc, char *argv[])
{
	int 	ch,
		enable          = 0,
		sd 		= -1;
	char 	*progname 	= argv[0],
		*port 		= NULL,
		*hostname 	= NULL,
		*address 	= NULL,
		*netmask 	= NULL;
	struct 	NetSyncInfo 	Net;

	opterr = 0;

	while ((ch = getopt_long(argc, argv, optstring, options, NULL)) != -1) {
		switch (ch) {

		case 'h':
			Help(progname);
			return 0;
		case 'v':
			PalmHeader(progname);
			return 0;
		case 'p':
			port = optarg;
			break;
		case 'e':
			enable = 1;
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
		}
	}
	
	sd = pilot_connect(port);
	if (sd < 0)
		goto error;

	if (dlp_OpenConduit(sd) < 0)
		goto error_close;

	/* Read and write the LanSync data to the Palm device */
	if (dlp_ReadNetSyncInfo(sd, &Net) < 0)
		goto error_close;

	if (!hostname && !address && !netmask) {
		printf("   NetSync : LanSync: %d\n", Net.lanSync);
		printf("   Hostname: %s\n", Net.hostName);
		printf("   IP Addr : %s\n", Net.hostAddress);
		printf("   Netmask : %s\n", Net.hostSubnetMask);
	}
	
	if (enable)
		Net.lanSync = 1;
	if (hostname != NULL)
		strncpy(Net.hostName, hostname, sizeof(Net.hostName));
	if (address != NULL)
		strncpy(Net.hostAddress, address, sizeof(Net.hostAddress));
	if (netmask != NULL)
		strncpy(Net.hostSubnetMask, netmask,
			sizeof(Net.hostSubnetMask));
	if (dlp_WriteNetSyncInfo(sd, &Net) < 0)
		goto error_close;

	if (enable)
		printf("\tEnabled NetSync");
	if (hostname != NULL)
		printf("\tInstalled Host Name: %s\n", Net.hostName);
	if (address != NULL)
		printf("\tInstalled IP Address: %s\n", Net.hostAddress);
	if (netmask != NULL)
		printf("\tInstalled Net Mask: %s\n", Net.hostSubnetMask);
	printf("\n");

	if (dlp_AddSyncLogEntry(sd, "install-netsync, exited normally.\n"
				"Thank you for using pilot-link.\n") < 0)
		goto error_close;
	
	if (dlp_EndOfSync(sd, 0) < 0)
		goto error_close;

	if (pi_close(sd) < 0)
		goto error;
	
	return 0;
	
 error_close:
	pi_close(sd);
	
 error:
	perror("   ERROR");
	fprintf(stderr, "\n");

	return -1;
}
