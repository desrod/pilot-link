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

struct option options[] = {
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
	{"port",        required_argument, NULL, 'p'},
	{"enable",      no_argument,       NULL, 'e'},
	{"name",        required_argument, NULL, 'n'},
	{"ip",          required_argument, NULL, 'i'},
	{"mask",        required_argument, NULL, 'm'},
	{NULL,          0,                 NULL, 0}
};

static const char *optstring = "hvp:en:i:m:";

static void display_help(char *progname)
{
	printf("   Assigns your Palm device NetSync information\n\n");
	printf("   Usage: %s -p <port> -H <hostname> -a <ip> -n <subnet>\n\n", progname);
	printf("   Options:\n");
	printf("     -p <port>         Use device file <port> to communicate with Palm\n");
	printf("     -e                Enables LANSync on the Palm\n");
	printf("     -n <name>         The hostname of the desktop you are syncing with\n");
	printf("     -i, --ip <ip>     IP address of the machine you connect your Palm to\n");
	printf("     -m <mask>         The subnet mask of the network your Palm is on\n");
	printf("     -h, --help        Display this information\n");
	printf("     -v, --version     Display version information\n\n");
	printf("   Examples: %s -p /dev/pilot -H \"localhost\" -a 127.0.0.1 -n 255.255.255.0\n\n", progname);

	exit(0);
}

int main(int argc, char *argv[])
{
	int 	c,		/* switch */
		enable          = 0,
		sd 		= -1;
	char 	*progname 	= argv[0],
		*port 		= NULL,
		*hostname 	= NULL,
		*address 	= NULL,
		*netmask 	= NULL;
	struct 	NetSyncInfo 	Net;

	opterr = 0;

	while ((c = getopt_long(argc, argv, optstring, options, NULL)) != -1) {
		switch (c) {

		case 'h':
			display_help(progname);
			return 0;
		case 'v':
			print_splash(progname);
			return 0;
		case 'p':
			port = optarg;
			break;
		case 'e':
			enable = 1;
			break;
		case 'n':
			hostname = optarg;
			break;
		case 'i':
			address = optarg;
			break;
		case 'm':
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

	if (enable > 0)
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
	return -1;
}
