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

#include "popt.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <netinet/in.h>

#include "pi-header.h"
#include "pi-source.h"
#include "pi-syspkt.h"
#include "pi-dlp.h"
#include "pi-header.h"
#include "userland.h"

int cancel = 0;


static void sighandler(int signo)
{
	cancel = 1;
}

struct record *records = 0;

int main(int argc, const char *argv[])
{
	int 	c,		/* switch */
		i,
		sd		= -1,
		file,
		timespent	= 0;

	char 	name[256],
		print[256];
	const char **args = NULL;
	const char *filename = NULL;

	time_t 	start = time(NULL), end;

	struct 	RPC_params p;
	plu_romversion_t version;

	unsigned long ROMstart;
	unsigned long ROMlength;
	unsigned long ROMversion;
	unsigned long offset;
	unsigned long left;

	poptContext po;

	struct poptOption options[] = {
		USERLAND_RESERVED_OPTIONS
	        POPT_TABLEEND
	};

	po = poptGetContext("pi-getrom", argc, argv, options, 0);
	poptSetOtherOptionHelp(po,"[filename]\n\n"
		"   Retrieves the ROM image from your Palm device.\n\n");

	if (argc < 2) {
		poptPrintUsage(po,stderr,0);
		return 1;
	}
	while ((c = poptGetNextOpt(po)) >= 0) {
		fprintf(stderr,"   ERROR: Unhandled option %d.\n",c);
		return 1;
	}

	if ( c < -1) {
		plu_badoption(po,c);
	}

	args = poptGetArgs(po);
	if (args && args[0] && args[1]) {
		fprintf(stderr,"   ERROR: Specify at most one filename.\n");
		return 1;
	}
	if (args && args[0]) {
		filename = args[0];
	}



	printf("   WARNING: Please completely back up your Palm (with pilot-xfer -b)\n"
	       "            before using this program!\n\n"
	       "   NOTICE: Use of this program may place you in violation\n"
	       "           of your license agreement with Palm Computing.\n\n"
	       "           Please read your Palm Computing handbook (\"Software\n"
	       "           License Agreement\") before running this program.\n\n");

	sd = plu_connect();
	if (sd < 0) {
		return -1;
	}

	/* Tell user (via Palm) that we are starting things up */
	dlp_OpenConduit(sd);

	if (plu_getromversion(sd,&version) < 0) {
		fprintf(stderr,"   ERROR: Could not retrieve ROM version.\n");
		pi_close(sd);
		return -1;
	}

	/* OS5 devices no longer support RPC, and will crash if we try to call it */
	if ((version.major >= 5)) {
		printf("   Unfortunately, Palm changed the underlying protocol used to fetch ROM\n"
			"   images from the handheld in a fatal way, and accessing them with these\n"
			"   tools will cause the Palm to crash.\n\n"
			"   Future versions of these tools may be updated to work around these\n"
			"   problems. For now, we'd like to avoid crashing your device.\n\n");
		pi_close(sd);
		return 1;
	}

	PackRPC(&p, 0xA23E, RPC_IntReply, RPC_Long(0xFFFFFFFF), RPC_End);
	/* err = */ dlp_RPC(sd, &p, &ROMstart);
	PackRPC(&p, 0xA23E, RPC_IntReply, RPC_Long(ROMstart), RPC_End);
	/* err = */ dlp_RPC(sd, &p, &ROMlength);


	/* As Steve said, "Bummer." */
	if ((version.major == 3) && (version.minor == 0)
	    && (ROMlength == 0x100000)) {
		ROMlength = 0x200000;
	}


	snprintf(name, sizeof(name),"%s%s.rom",
		(filename ? filename : "pilot-"),
		version.name);

	printf("   Generating %s\n", name);

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
		int 	len = left,
			j;
		char 	buffer[256];
		double 	perc = ((double) offset / ROMlength) * 100.0;

		if (len > 256)
			len = 256;

		printf("\r   %ld of %ld bytes (%.2f%%)", offset, ROMlength, perc);

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
				printf("\n   Operation cancelled!\n");
				dlp_AddSyncLogEntry(sd, "\npi-getrom ended unexpectedly.\n"
							"Entire ROM was not fetched.\n");
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

	printf("\n   ROM fetch complete\n");
	end = time(NULL);
	timespent = (end-start);
	printf("   ROM fetched in: %d:%02d:%02d\n",timespent/3600, (timespent/60)%60, timespent%60);

      cancel:
	close(file);
	pi_close(sd);
	return 0;
}
