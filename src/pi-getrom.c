/*
 * pi-getrom:  Fetch ROM image from Palm, without using getrom.prc
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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <termios.h>

#include "pi-source.h"
#include "pi-syspkt.h"
#include "pi-dlp.h"
#include "pi-header.h"

void Help(char *progname)
{
	fprintf(stderr, "   Usage: %s %s [pilot.rom]\n\n", progname,
		TTYPrompt);
	exit(2);
}

int cancel = 0;
RETSIGTYPE sighandler(int signo)
{
	cancel = 1;
}

struct record *records = 0;

int main(int argc, char *argv[])
{
	char name[256];
	char print[256];
	char *progname = argv[0];
	char *device = argv[1];
	int i, sd;
	struct RPC_params p;
	struct pi_sockaddr addr;
	extern char *optarg;
	extern int optind;

	/* int err; */
	int ret;
	int file;
	unsigned long ROMstart, ROMlength, ROMversion, offset, left;
	int majorVersion, minorVersion, bugfixVersion, build, state;

	PalmHeader(progname);

	if (argc < 2)
		Help(progname);

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

	fprintf(stderr, "   Warning: Please completely back up your Palm (with pilot-xfer -b)\n");
	fprintf(stderr, "            before using this program!\n\n");
	fprintf(stderr, "   NOTICE: Use of this program may place you in violation\n");
	fprintf(stderr, "           of your license agreement with Palm Computing.\n\n");
	fprintf(stderr, "           Please read your Palm Computing handbook (\"Software\n");
	fprintf(stderr, "           License Agreement\") before running this program.\n\n");
	fprintf(stderr, "   Start HotSync (not getrom.prc) on your Palm.\n");
	fprintf(stderr, "   Port: %s\n\n   Please press the HotSync button...\n", device);

	sd = pi_accept(sd, 0, 0);
	if (sd == -1) {
		fprintf(stderr, "\n   Error accepting data on %s\n",
			device);
		perror("   pi_accept");
		fprintf(stderr, "\n");
		exit(1);
	}

	/* Tell user (via Palm) that we are starting things up */
	dlp_OpenConduit(sd);

	PackRPC(&p, 0xA23E, RPC_IntReply, RPC_Long(0xFFFFFFFF), RPC_End);
	/* err = */ dlp_RPC(sd, &p, &ROMstart);
	PackRPC(&p, 0xA23E, RPC_IntReply, RPC_Long(ROMstart), RPC_End);
	/* err = */ dlp_RPC(sd, &p, &ROMlength);

	dlp_ReadFeature(sd, makelong("psys"), 1, &ROMversion);

	if (argc < 3)
		strcpy(name, "pilot.rom");
	else
		strcpy(name, argv[2]);

	majorVersion =
	    (((ROMversion >> 28) & 0xf) * 10) + ((ROMversion >> 24) & 0xf);
	minorVersion = ((ROMversion >> 20) & 0xf);
	bugfixVersion = ((ROMversion >> 16) & 0xf);
	state = ((ROMversion >> 12) & 0xf);
	build =
	    (((ROMversion >> 8) & 0xf) * 10) +
	    (((ROMversion >> 4) & 0xf) * 10) + (ROMversion & 0xf);

	/* As Steve said, "Bummer." */
	if ((majorVersion == 3) && (minorVersion == 0)
	    && (ROMlength == 0x100000)) {
		ROMlength = 0x200000;
	}

	sprintf(name + strlen(name), "%d.%d.%d", majorVersion,
		minorVersion, bugfixVersion);
	if (state != 3)
		sprintf(name + strlen(name), "%s%d",
			((state == 0) ? "d" : (state ==
					       1) ? "a" : (state ==
							   2) ? "b" : "u"),
			build);

	printf("Generating %s\n", name);

	file = open(name, O_RDWR | O_CREAT, 0666);

	offset = lseek(file, 0, SEEK_END);
	offset &= ~255;
	lseek(file, offset, SEEK_SET);

	PackRPC(&p, 0xA164, RPC_IntReply, RPC_Byte(1), RPC_End);
	/* err = */ dlp_RPC(sd, &p, 0);

	sprintf(print, "Downloading byte %ld", offset);
	PackRPC(&p, 0xA220, RPC_IntReply, RPC_Ptr(print, strlen(print)),
		RPC_Short(strlen(print)), RPC_Short(0), RPC_Short(28),
		RPC_End);
	/* err = */ dlp_RPC(sd, &p, 0);

	signal(SIGINT, sighandler);
	left = ROMlength - offset;
	i = offset;
	while (left > 0) {
		char buffer[256];
		int len = left;
		int j;

		if (len > 256)
			len = 256;
		printf("\r%ld of %ld bytes", offset, ROMlength);
		fflush(stdout);
		PackRPC(&p, 0xA026, RPC_IntReply, RPC_Ptr(buffer, len),
			RPC_Long(offset + ROMstart), RPC_Long(len),
			RPC_End);
		/* err = */ dlp_RPC(sd, &p, 0);
		left -= len;
		/* If the buffer only contains zeros, skip instead of
		   writing, so that the file will be holey. */
		for (j = 0; j < len; j++)
			if (buffer[j])
				break;
		if (j == len)
			lseek(file, len, SEEK_CUR);
		else
			write(file, buffer, len);
		offset += len;
		if (cancel || !(i++ % 8))
			if (cancel || (dlp_OpenConduit(sd) < 0)) {
				printf("\nCancelled!\n");
				goto cancel;
			}
		if (!(i % 16)) {
			sprintf(print, "%ld", offset);
			PackRPC(&p, 0xA220, RPC_IntReply,
				RPC_Ptr(print, strlen(print)),
				RPC_Short(strlen(print)), RPC_Short(92),
				RPC_Short(28), RPC_End);
			/* err = */ dlp_RPC(sd, &p, 0);
		}
	}
	printf("\r%ld of %ld bytes\n", offset, ROMlength);
	printf("ROM fetch complete\n");

      cancel:
	close(file);

	pi_close(sd);

	return 0;
}
