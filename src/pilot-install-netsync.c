/* ex: set tabstop=4 expandtab: */
/* 
 * install-netsync.c:  Palm Network Information Installer
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
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "pi-source.h"
#include "pi-dlp.h"
#include "pi-header.h"

struct option options[] = {
	{"port",        required_argument, NULL, 'p'},
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
	{"enable",      no_argument,       NULL, 'e'},
	{"disable",	no_argument,       NULL, 'd'},
	{"name",        required_argument, NULL, 'n'},
	{"ip",          required_argument, NULL, 'i'},
	{"mask",        required_argument, NULL, 'm'},
	{NULL,          0,                 NULL, 0}
};


static const char *optstring = "p:hvedn:i:m:";

static void display_help(const char *progname)
{
	printf("   Assigns your Palm device NetSync information\n\n");
	printf("   Usage: %s -p <port> -n <hostname> -i <ip> -m <subnet>\n\n", progname);
	printf("   Options:\n");
	printf("     -p <port>         Use device file <port> to communicate with Palm\n");
	printf("     -e, --enable      Enables LANSync on the Palm\n");
	printf("     -d, --disable     Disable the LANSync setting on the Palm\n");
	printf("     -n <name>         The hostname of the desktop you are syncing with\n");
	printf("     -i, --ip <ip>     IP address of the machine you connect your Palm to\n");
	printf("     -m <mask>         The subnet mask of the network your Palm is on\n");
	printf("     -h, --help        Display this information\n");
	printf("     -v, --version     Display version information\n\n");
	printf("   Examples: %s -p /dev/pilot -H \"localhost\" -a 127.0.0.1 -n 255.255.255.0\n\n", progname);

	return;
}

int main(int argc, char *argv[])
{
	int 	c,		/* switch */
		enable		= -1,
		sd 		= -1;

	char 	*progname 	= argv[0],
		*port 		= NULL,
		*hostname 	= NULL,
		*address 	= NULL,
		*netmask 	= NULL;

	struct 	NetSyncInfo 	Net;

	struct in_addr addr;

	struct poptOption long_options[] = {
		{"port", 'p', POPT_ARG_STRING, &port, 0, "Use device file <port> to communicate with Palm"},
		{"version", 'v', POPT_ARG_NONE, NULL, 0, "Display version information"},
        	{"enable", 'e', POPT_ARG_NONE, &enable, 0, "Enables LANSync on the Palm"},
        	{"disable", 'd', POPT_ARG_NONE, &enable, 0, "Disable the LANSync setting on the Palm"},
        	{"name", 'n', POPT_ARG_STRING, &hostname, 0, "The hostname of the desktop you are syncing with"},
        	{"ip", 'i', POPT_ARG_STRING, &address, 0, "IP address of the machine you connect your Palm to"},
        	{"mask", 'm', POPT_ARG_STRING, &netmask, 0, "The subnet mask of the network your Palm is on"},
		POPT_AUTOHELP
        	{ NULL, 0, 0, NULL, 0 }
	};

	poptContext pc;
	pc = poptGetContext("install-netsync", argc, (const char **) argv, long_options, 0);
	setlinebuf(stdout);

	while ((c = getopt_long(argc, argv, optstring, options, NULL)) != -1) {
		switch (c) {

		case 'h':
			display_help(progname);
			return 0;
		case 'v':
			print_splash(progname);
			return 0;
		case 'p':
			free(port);
			port = strdup(optarg);
			break;
		case 'e':
			enable = 1;
			break;
		case 'd':
			enable = 0;
			break;
		case 'n':
			free(hostname);
			hostname = strdup(optarg);
			break;
		case 'i':
			free(address);
			address = strdup(optarg);
			break;
		case 'm':
			free(netmask);
			netmask = strdup(optarg); 
			break;
		default:
			display_help(progname);
			return 0;
		}
	}

	if (address && !inet_pton(AF_INET, address, &addr)) {
		printf("   The address you supplied, '%s' is in invalid.\n"
			"   Please supply a dotted quad, such as 1.2.3.4\n\n", address);
		exit(EXIT_FAILURE);
	}

	if (netmask && !inet_pton(AF_INET, netmask, &addr)) {
		printf("   The netmask you supplied, '%s' is in invalid.\n"
			"   Please supply a dotted quad, such as 255.255.255.0\n\n", netmask);
		exit(EXIT_FAILURE);
	}

	sd = pilot_connect(port);
	if (sd < 0)
		goto error;

	if (dlp_OpenConduit(sd) < 0)
		goto error_close;

	/* Read and write the LANSync data to the Palm device */
	if (dlp_ReadNetSyncInfo(sd, &Net) < 0)
		goto error_close;

	if (enable != -1)
		Net.lanSync = enable;

	printf("   LANSync....: %sabled\n", (Net.lanSync == 1 ? "En" : "Dis"));

	if (hostname)
		strncpy(Net.hostName, hostname, sizeof(Net.hostName));

	if (address)
		strncpy(Net.hostAddress, address, sizeof(Net.hostAddress));

	if (netmask)
		strncpy(Net.hostSubnetMask, netmask,
			sizeof(Net.hostSubnetMask));

	printf("   Hostname...: %s\n", Net.hostName);
	printf("   IP Address.: %s\n", Net.hostAddress);
	printf("   Netmask....: %s\n", Net.hostSubnetMask);
	printf("\n");

	if (dlp_WriteNetSyncInfo(sd, &Net) < 0)
		goto error_close;

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
