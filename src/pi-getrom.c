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

#ifndef HAVE_GETOPT_LONG
#include "getopt.h"
#else
#include <getopt.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <termios.h>

#include "pi-header.h"
#include "pi-source.h"
#include "pi-syspkt.h"
#include "pi-dlp.h"
#include "pi-header.h"

int cancel = 0;

struct option options[] = {
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
	{"port",        required_argument, NULL, 'p'},
	{NULL,          0,                 NULL, 0}
};

static const char *optstring = "hvp:";

static void Help(char *progname)
{
	printf("   Retrieves the ROM image from your Palm device\n\n"
	       "   Usage: %s -p <port> [--copilot] [pilot.rom]\n\n"
	       "   Options:\n"
	       "     -p <port>         Use device file <port> to communicate with Palm\n"
	       "     -h, --help        Display this information\n\n"
	       "     -v, --version     Display this information\n\n"
	       "   Only the port option is required, the other options are... optional.\n\n"
	       "   Examples: %s -p /dev/pilot myrom\n\n",
	       progname, progname);
}

static RETSIGTYPE sighandler(int signo)
{
	cancel = 1;
}

struct record *records = 0;

int main(int argc, char *argv[])
{
	int 	count,
		idx,
		sd,
		file,
		majorVersion,
		minorVersion,
		bugfixVersion,
		build, 
		state;
	
	char 	name[256],
		print[256],
		*progname 	= argv[0],
		*port 	        = NULL,
		*filename;
	
	struct 	RPC_params p;
	unsigned long ROMstart; 
	unsigned long ROMlength;
	unsigned long ROMversion;
	unsigned long offset;
	unsigned long left;

	while ((count = getopt_long(argc, argv, optstring, options, NULL)) != -1) {
		switch (count) {

		case 'h':
			Help(progname);
			return 0;
		case 'v':
			PalmHeader(progname);
			return 0;
		case 'p':
			port = optarg;
			break;
		}
	}
	if (optind > 0)
		filename = argv[optind];
	else
		filename = NULL;


	printf("   Warning: Please completely back up your Palm (with pilot-xfer -b)\n"
	       "            before using this program!\n\n"
	       "   NOTICE: Use of this program may place you in violation\n"
	       "           of your license agreement with Palm Computing.\n\n"
	       "           Please read your Palm Computing handbook (\"Software\n"
	       "           License Agreement\") before running this program.\n\n");

	sd = pilot_connect(port);
	if (sd < 0) {
		perror("\tERROR:");
		fprintf(stderr, "\n");
		return -1;
	}

	/* Tell user (via Palm) that we are starting things up */
	dlp_OpenConduit(sd);

	PackRPC(&p, 0xA23E, RPC_IntReply, RPC_Long(0xFFFFFFFF), RPC_End);
	/* err = */ dlp_RPC(sd, &p, &ROMstart);
	PackRPC(&p, 0xA23E, RPC_IntReply, RPC_Long(ROMstart), RPC_End);
	/* err = */ dlp_RPC(sd, &p, &ROMlength);

	dlp_ReadFeature(sd, makelong("psys"), 1, &ROMversion);

	if (!filename)
		strcpy(name, "pilot-");
	else
		strcpy(name, filename);

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

	sprintf(name + strlen(name), "%d.%d.%d.rom", majorVersion,
		minorVersion, bugfixVersion);
	if (state != 3)
		sprintf(name + strlen(name), "%s%d",
			((state == 0) ? "d" : (state ==
					       1) ? "a" : (state ==
							   2) ? "b" : "u"),
			build);

	printf("\tGenerating %s\n", name);

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
	idx = offset;
	while (left > 0) {
		char buffer[256];
		int len = left;
		int j;
		double perc = ((double) offset / ROMlength) * 100.0;

		if (len > 256)
			len = 256;

		printf("\r\t%ld of %ld bytes (%.2f%%)", offset, ROMlength, perc);

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
		if (cancel || !(idx++ % 8))
			if (cancel || (dlp_OpenConduit(sd) < 0)) {
				printf("\nCancelled!\n");
				goto cancel;
			}
		if (!(idx % 16)) {
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
