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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "pi-source.h"
#include "pi-dlp.h"
#include "pi-header.h"
#include "pi-userland.h"

poptContext po;

int main(int argc, char *argv[])
{
	int 	enable	= -1,
		sd 	= -1,
		po_err	= -1;

	const char
		*hostname 	= NULL,
		*address 	= NULL,
		*netmask 	= NULL;

	const struct poptOption options[] = {
		USERLAND_RESERVED_OPTIONS
		{ "enable",  'e', POPT_ARG_VAL,    &enable, 1, "Enables LANSync on the Palm"},
		{ "disable", 'd', POPT_ARG_VAL,    &enable, 0, "Disable the LANSync setting on the Palm"},
		{ "name",    'n', POPT_ARG_STRING, &hostname, 0, "The hostname of the remote machine you sync with", "<name>"},
		{ "address", 'a', POPT_ARG_STRING, &address, 0, "IP address of the remote machine you connect to", "<address>"},
		{ "mask",    'm', POPT_ARG_STRING, &netmask, 0, "Subnet mask of the network your Palm is on", "<netmask>"},
		POPT_TABLEEND
	};

	struct 	NetSyncInfo 	Net;
	struct in_addr addr;

	po = poptGetContext("install-netsync", argc, (const char **) argv, options, 0);
	poptSetOtherOptionHelp(po," [-p port] <netsync options>\n\n"
		"   Assigns your Palm device NetSync information\n\n"
		"   Example:\n"
		"      -p /dev/pilot -H \"localhost\" -a 127.0.0.1 -n 255.255.255.0\n\n");

	if (argc < 2) {
		poptPrintUsage(po,stderr,0);
		return 1;
	}

	while ((po_err = poptGetNextOpt(po)) >= 0) {
	}
	if (po_err < -1) plu_badoption(po,po_err);

	/* FIXME: Take the user-supplied IP or hostname and reverse it to
	   get the other component, which reduces the complexity of this by
	   one argument passed in. getnameinfo() will help here. */
	if (address && !inet_pton(AF_INET, address, &addr)) {
		fprintf(stderr,"   ERROR: The address you supplied, '%s' is in invalid.\n"
			"      Please supply a dotted quad, such as 1.2.3.4\n\n", address);
		return 1;
	}

	if (netmask && !inet_pton(AF_INET, netmask, &addr)) {
		fprintf(stderr,"   ERROR: The netmask you supplied, '%s' is in invalid.\n"
			"      Please supply a dotted quad, such as 255.255.255.0\n\n", netmask);
		return 1;
	}

	sd = plu_connect();
	if (sd < 0)
		goto error;

	if (dlp_OpenConduit(sd) < 0)
		goto error_close;

	/* Read and write the LANSync data to the Palm device */
	if (dlp_ReadNetSyncInfo(sd, &Net) < 0)
		goto error_close;

	if (enable != -1)
		Net.lanSync = enable;

	if (!plu_quiet) {
		printf("   LANSync....: %sabled\n", (Net.lanSync == 1 ? "En" : "Dis"));
		fflush(stdout);
	}

	if (hostname)
		strncpy(Net.hostName, hostname, sizeof(Net.hostName));

	if (address)
		strncpy(Net.hostAddress, address, sizeof(Net.hostAddress));

	if (netmask)
		strncpy(Net.hostSubnetMask, netmask,
			sizeof(Net.hostSubnetMask));

	if (!plu_quiet) {
		printf("   Hostname...: %s\n", Net.hostName);
		printf("   IP Address.: %s\n", Net.hostAddress);
		printf("   Netmask....: %s\n\n", Net.hostSubnetMask);
	}

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

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
