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

/* Note: if you use this program to change the user name on the Palm, I
   _highly_ reccomend that you perform a hard reset before HotSyncing with a
   Windows machine. This is because the user-id information has only been
   partially altered, and it is not worth trying to predict what the Desktop
   will do. - KJA */

#include <stdio.h>
#include <stdlib.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-header.h"

int main(int argc, char *argv[])
{
	struct pi_sockaddr addr;
	struct PilotUser U;
	struct SysInfo S;
	struct CardInfo C;
	struct NetSyncInfo N;
	int ret;
	int sd;
	unsigned long romversion;
	char *progname = argv[0];
	char *device = argv[1];

	if (argc < 2) {
		fprintf(stderr,
			"   Assigns your Palm device a user name and unique userid\n\n");
		fprintf(stderr, "   Usage: %s %s [User name] <userid>\n\n",
			argv[0], TTYPrompt);
		fprintf(stderr,
			"   Example: %s /dev/ttyS0 \"John Q. Public\" 12345\n\n",
			progname);
		exit(2);
	}

	if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP))) {
		perror("pi_socket");
		exit(1);
	}

	addr.pi_family = PI_AF_SLP;
	strcpy(addr.pi_device, device);

	ret = pi_bind(sd, (struct sockaddr *) &addr, sizeof(addr));
	if (ret == -1) {
		fprintf(stderr, "\n   Unable to bind to port %s\n",
			device);
		perror("   pi_bind");
		fprintf(stderr, "\n");
		exit(1);
	}

	printf
	    ("   Port: %s\n\n   Please press the HotSync button now...\n",
	     device);

	ret = pi_listen(sd, 1);
	if (ret == -1) {
		fprintf(stderr, "\n   Error listening on %s\n", device);
		perror("   pi_listen");
		fprintf(stderr, "\n");
		exit(1);
	}

	sd = pi_accept(sd, 0, 0);
	if (sd == -1) {
		fprintf(stderr, "\n   Error accepting data on %s\n",
			device);
		perror("   pi_accept");
		fprintf(stderr, "\n");
		exit(1);
	}

	fprintf(stderr, "Connected...\n");

	/* Tell user (via Palm) that we are starting things up */
	dlp_OpenConduit(sd);
	dlp_ReadUserInfo(sd, &U);
	dlp_ReadSysInfo(sd, &S);

	C.card = -1;
	C.more = 1;
	while (C.more) {
		if (dlp_ReadStorageInfo(sd, C.card + 1, &C) < 0)
			break;

		printf
		    (" Card #%d has %lu bytes of ROM, and %lu bytes of RAM (%lu of that is free)\n",
		     C.card, C.romSize, C.ramSize, C.ramFree);
		printf(" It is called '%s', and was made by '%s'.\n",
		       C.name, C.manufacturer);
	}

	if (argc == 2) {
		printf("Palm user %s\n", U.username);
		printf("UserID %ld \n", U.userID);
	} else {
		strcpy(U.username, argv[2]);
		if (argc == 4) {
			U.userID = atoi(argv[3]);
		}
		U.lastSyncDate = time((time_t *) 0);
		dlp_WriteUserInfo(sd, &U);
	}

	printf
	    ("Through ReadSysInfo: ROM Version: 0x%8.8lX, locale: 0x%8.8lX, name: '%s'\n",
	     S.romVersion, S.locale, S.name);

	dlp_ReadFeature(sd, makelong("psys"), 1, &romversion);

	printf("ROM Version through ReadFeature: 0x%8.8lX\n", romversion);

	if (dlp_ReadNetSyncInfo(sd, &N) >= 0) {
		printf
		    ("NetSync: LAN sync = %d, Host name = '%s', address = '%s', netmask ='%s'\n",
		     N.lanSync, N.hostName, N.hostAddress,
		     N.hostSubnetMask);
	}

	pi_close(sd);
	exit(0);
}
