/*
 * pi-getram:  Fetch RAM image from Palm
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

#include "getopt.h"
#include <stdio.h>
#include <stdlib.h>
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

/* Declare prototypes */
static void display_help(char *progname);
void print_splash(char *progname);
int pilot_connect(char *port);

int cancel = 0;

struct option options[] = {
	{"port",        required_argument, NULL, 'p'},
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
	{NULL,          0,                 NULL, 0}
};

static const char *optstring = "p:hvc";

static void display_help(char *progname)
{
	printf("   Retrieves the RAM image from your Palm device\n\n");
	printf("   Usage: %s -p <port> [pilot.ram]\n\n", progname);
	printf("   Options:\n");
	printf("     -p, --port <port>       Use device file <port> to communicate with Palm\n");
	printf("     -h, --help              Display help information for %s\n", progname);
	printf("     -v, --version           Display %s version information\n", progname);
	printf("   Only the port option is required, the other options are... optional.\n\n");
	printf("   Examples: %s -p /dev/pilot myram\n\n", progname);
}

static void sighandler(int signo)
{
	cancel = 1;
}

struct record *records = 0;

int main(int argc, char *argv[])
{
	int 	c,		/* switch */
		i,
		sd,
		file,
		j,
		majorVersion,
		minorVersion,
		bugfixVersion,
		build,
		state,
		timespent	= 0;	

	char 	name[256],
		print[256],
		*progname 	= argv[0],
		*port 		= NULL,
		*filename;

	time_t  start = time(NULL), end;
	
	struct 	RPC_params p;

	unsigned long SRAMstart, SRAMlength, ROMversion, offset, left;
	
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
		}
	}
	if (optind > 0)
		filename = argv[optind];
	else
		filename = NULL;

	printf("\tWarning: Please completely back up your Palm (with pilot-xfer -b)\n"
	       "\t         before using this program!\n\n");
	
	sd = pilot_connect(port);
	if (sd < 0) {
		return -1;
	}
	

	/* Tell user (via Palm) that we are starting things up */
	dlp_OpenConduit(sd);

	PackRPC(&p, 0xA23D, RPC_IntReply, RPC_Long(0xFFFFFFFE), RPC_End);
	/* err = */ dlp_RPC(sd, &p, &SRAMstart);
	PackRPC(&p, 0xA23D, RPC_IntReply, RPC_Long(SRAMstart), RPC_End);
	/* err = */ dlp_RPC(sd, &p, &SRAMlength);

#if 0
	printf
	    ("dramstart = %lu (%8.8lX), length = %lu (%8.8lX), end = %lu (%8.8lX)\n",
	     SRAMstart, SRAMstart, SRAMlength, SRAMlength,
	     SRAMstart + SRAMlength, SRAMstart + SRAMlength);
#endif

	dlp_ReadFeature(sd, makelong("psys"), 1, &ROMversion);

	if (!filename)
		strcpy(name, "pilot-");
	else
		strcpy(name, filename);

	majorVersion = 
		(((ROMversion >> 28) & 0xf) * 10) + ((ROMversion >> 24) & 0xf);
	minorVersion 	= ((ROMversion >> 20) & 0xf); 
	bugfixVersion 	= ((ROMversion >> 16) & 0xf);
	state 		= ((ROMversion >> 12) & 0xf);
	build =
	    (((ROMversion >> 8) & 0xf) * 10) +
	    (((ROMversion >> 4) & 0xf) * 10) + (ROMversion & 0xf);

	sprintf(name + strlen(name), "%d.%d.%d.ram", majorVersion, minorVersion, bugfixVersion);
	if (state != 3)
		sprintf(name + strlen(name), "-%s%d", (
			(state == 0) ? "d" : 
			(state == 1) ? "a" :
			(state == 2) ? "b" : "u"), build);

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

#if 0
	PackRPC(&p, 0xA026, RPC_IntReply, RPC_LongPtr(&penPtr),
		RPC_Long(364), RPC_Long(4), RPC_End);
	/* err = */ dlp_RPC(sd, &p, 0);

	printf("penPtr = %lu (%8.8lX)\n", penPtr, penPtr);

	PackRPC(&p, 0xA026, RPC_IntReply, RPC_Ptr(print, 8),
		RPC_Long(penPtr), RPC_Long(8), RPC_End);
	/* err = */ dlp_RPC(sd, &p, 0);
	dumpdata(print, 8);
#endif

	signal(SIGINT, sighandler);
	left 	= SRAMlength - offset;
	i 	= offset;
	while (left > 0) {
		int 	len 	= left;
		char 	buffer[256];
		double 	perc 	= ((double) offset / SRAMlength) * 100.0;

		if (len > 256)
			len = 256;

                printf("\r   %ld of %ld bytes (%.2f%%)", offset, SRAMlength, perc);
		fflush(stdout);
		PackRPC(&p, 0xA026, RPC_IntReply, RPC_Ptr(buffer, len),
			RPC_Long(offset + SRAMstart), RPC_Long(len),
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
		if (cancel || !(i++ % 4))
			if (cancel || (dlp_OpenConduit(sd) < 0)) {
				printf("\n   Operation cancelled!\n");
				dlp_AddSyncLogEntry(sd, "\npi-getrom ended unexpectedly.\n"
							"Entire RAM was not fetched.\n");
				goto cancel;
			}
		if (!(i % 8)) {
			sprintf(print, "%ld", offset);
			PackRPC(&p, 0xA220, RPC_IntReply,
				RPC_Ptr(print, strlen(print)),
				RPC_Short(strlen(print)), RPC_Short(92),
				RPC_Short(28), RPC_End);
			/* err = */ dlp_RPC(sd, &p, 0);
		}
	}

	printf("   RAM fetch complete\n");	
	end = time(NULL);
        timespent = (end-start);
	printf("   RAM fetched in: %d:%02d:%02d\n",timespent/3600, (timespent/60)%60, timespent%60);



      cancel:
	close(file);
	pi_close(sd);
	return 0;
}
