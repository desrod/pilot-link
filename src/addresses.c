/*
 * addresses.c:  Translate Palm address book into a generic format
 *
 * Copyright (c) 1996, Kenneth Albanowski
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
#include <sys/stat.h>
#include <signal.h>
#include <utime.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-address.h"
#include "pi-dlp.h"
#include "pi-header.h"

int sd = 0;
void Disconnect(void);

RETSIGTYPE SigHandler(int signal);

void Disconnect(void)
{
	if (sd == 0)
		return;

	dlp_EndOfSync(sd, 0);
	pi_close(sd);
	sd = 0;
}

RETSIGTYPE SigHandler(int signal)
{
	puts("   Abort on signal!");
	Disconnect();
	exit(3);
}

int main(int argc, char *argv[])
{
	struct pi_sockaddr addr;
	int db;
	int i;
	struct PilotUser U;
	int ret;
	unsigned char buffer[0xffff];
	struct AddressAppInfo aai;
	char *progname = argv[0];
	char *port = argv[1];

	PalmHeader(progname);

	if (argc < 2) {
		fprintf(stderr, "   Usage: %s %s\n\n", progname,
			TTYPrompt);
		exit(2);
	}

	if (sd && sd != 0)
		return 0;

	signal(SIGHUP, SigHandler);
	signal(SIGINT, SigHandler);
	signal(SIGSEGV, SigHandler);

	if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP))) {
		perror("pi_socket");
		exit(1);
	}

	addr.pi_family = PI_AF_SLP;
	strcpy(addr.pi_device, port);

	ret = pi_bind(sd, (struct sockaddr *) &addr, sizeof(addr));
	if (ret == -1) {
		fprintf(stderr, "\n   Unable to bind to port %s\n", port);
		perror("   pi_bind");
		fprintf(stderr, "\n");
		exit(1);
	}

	printf
	    ("   Port: %s\n\n   Please press the HotSync button now...\n",
	     port);

	ret = pi_listen(sd, 1);
	if (ret == -1) {
		fprintf(stderr, "\n   Error listening on %s\n", port);
		perror("   pi_listen");
		fprintf(stderr, "\n");
		exit(1);
	}

	sd = pi_accept(sd, 0, 0);
	if (sd == -1) {
		fprintf(stderr, "\n   Error accepting data on %s\n", port);
		perror("   pi_accept");
		fprintf(stderr, "\n");
		exit(1);
	}

	fprintf(stderr, "Connected...\n");

	/* Ask the pilot who it is. */
	dlp_ReadUserInfo(sd, &U);

	/* Tell user (via Palm) that we are starting things up */
	dlp_OpenConduit(sd);

	/* Open the Address book's database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "AddressDB", &db) < 0) {
		puts("Unable to open AddressDB");
		dlp_AddSyncLogEntry(sd, "Unable to open AddressDB.\n");
		exit(1);
	}

	dlp_ReadAppBlock(sd, db, 0, buffer, 0xffff);
	unpack_AddressAppInfo(&aai, buffer, 0xffff);

	for (i = 0;; i++) {
		struct Address a;
		int attr, category;
		int j;

		int len =
		    dlp_ReadRecordByIndex(sd, db, i, buffer, 0, 0, &attr,
					  &category);

		if (len < 0)
			break;

		/* Skip deleted records */
		if ((attr & dlpRecAttrDeleted)
		    || (attr & dlpRecAttrArchived))
			continue;

		unpack_Address(&a, buffer, len);

		printf("Category: %s\n", aai.category.name[category]);
		for (j = 0; j < 19; j++) {
			if (a.entry[j]) {
				int l = j;

				if ((l >= entryPhone1)
				    && (l <= entryPhone5))
					printf("%s: %s\n",
					       aai.phoneLabels[a.
							       phoneLabel[l
									  -
									  entryPhone1]],
					       a.entry[j]);
				else
					printf("%s: %s\n", aai.labels[l],
					       a.entry[j]);
			}
		}
		printf("\n");

		free_Address(&a);
	}

	/* Close the database */
	dlp_CloseDB(sd, db);

	dlp_AddSyncLogEntry(sd, "Read addresses from Palm.\n");

	pi_close(sd);

	return 0;
}
