/*
 * pi-getrom:  Fetch ROM image from Palm, without using getrom.prc.
 *             Contains all functional code from getram and getromtoken,
 *             too, with additional copyrights.
 *
 * Copyright (C) 2002, Owen Stenseth
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
#include "pi-userland.h"

#ifndef DEFAULT_MODE
#define DEFAULT_MODE mode_getrom
#endif

/* Cancel code, common to all the get* */

int cancel = 0;

static void sighandler(int signo)
{
	cancel = 1;
}


/* Per-action code. */

/* Check ROM version, return < 0 if the ROM doesn't support RPC.
 * Otherwise, fill buffer with filename + romversion string.
 */

static int check_romversion(int sd, plu_romversion_t *version)
{

	if (plu_getromversion(sd,version) < 0) {
		fprintf(stderr,"   ERROR: Could not retrieve ROM version.\n");
		return -1;
	}

	/* OS5 devices no longer support RPC, and will crash if we try to call it */
	if ((version->major >= 5)) {
		printf("   Unfortunately, Palm changed the underlying protocol used to fetch ROM\n"
			"   images from the handheld in a fatal way, and accessing them with these\n"
			"   tools will cause the Palm to crash.\n\n"
			"   Future versions of these tools may be updated to work around these\n"
			"   problems. For now, we'd like to avoid crashing your device.\n\n");
		return -2;
	}
	return 0;
}

int do_get_rom(int sd,const char *filename)
{
	int	i,
		file = -1,
		timespent = 0;

	struct 	RPC_params p;
	plu_romversion_t version;

	unsigned long ROMstart;
	unsigned long ROMlength;
	unsigned long offset;
	unsigned long left;

	char 	name[256],
		print[256];

	time_t 	start = time(NULL), end;

	/* Tell user (via Palm) that we are starting things up */
	dlp_OpenConduit(sd);

	if (check_romversion(sd,&version) < 0) {
		return -1;
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


	if (!plu_quiet) {
		printf("   Generating %s\n", name);
	}

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

		if (!plu_quiet) {
			printf("\r   %ld of %ld bytes (%.2f%%)", offset, ROMlength, perc);
		}

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

	end = time(NULL);
	timespent = (end-start);
	if (!plu_quiet) {
		printf("\n   ROM fetch complete\n");
		printf("   ROM fetched in: %d:%02d:%02d\n",timespent/3600, (timespent/60)%60, timespent%60);
	}

cancel:
	if (file>=0) {
		close(file);
	}
	return 0;
}



int do_get_ram(int sd, const char *filename)
{
	char 	name[256],
		print[256];
	time_t  start = time(NULL), end;

	struct 	RPC_params p;
	plu_romversion_t version;

	unsigned long SRAMstart, SRAMlength, offset, left;

	int	i,
		file = -1,
		j,
		timespent	= 0;

	/* Tell user (via Palm) that we are starting things up */
	dlp_OpenConduit(sd);

	if (check_romversion(sd, &version) < 0) {
		return 1;
	}

	PackRPC(&p, 0xA23D, RPC_IntReply, RPC_Long(0xFFFFFFFE), RPC_End);
	/* err = */ dlp_RPC(sd, &p, &SRAMstart);
	PackRPC(&p, 0xA23D, RPC_IntReply, RPC_Long(SRAMstart), RPC_End);
	/* err = */ dlp_RPC(sd, &p, &SRAMlength);

	snprintf(name, sizeof(name),"%s%s.ram",
		(filename ? filename : "pilot-"),
		version.name);

	if (!plu_quiet) {
		printf("   Generating %s\n", name);
	}

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

		if (!plu_quiet) {
                	printf("\r   %ld of %ld bytes (%.2f%%)", offset, SRAMlength, perc);
		}
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

	end = time(NULL);
        timespent = (end-start);
	if (!plu_quiet) {
		printf("   RAM fetch complete\n");
		printf("   RAM fetched in: %d:%02d:%02d\n",timespent/3600, (timespent/60)%60, timespent%60);
	}

cancel:
	if (file >=0) {
		close(file);
	}
	return 0;
}


int do_get_token(int sd, const char *token)
{
	unsigned long t = 0;
	size_t	size;
	char    buffer[50];

	if (!token) {
		return -1;
	}

	/* asking for trouble on aligned architectures */
	set_long(&t,*((long *)token));

	size = sizeof(buffer);
	if (dlp_GetROMToken(sd, t, buffer, &size) < 0) {
		return -1;
	}

	fprintf(stdout, "Token for '%s' was: %s\n", token, buffer);

	return 0;
}

int do_sysinfo(int sd)
{
	struct SysInfo info;
	char buf[256];

	if (dlp_ReadSysInfo(sd,&info) < 0) {
		fprintf(stderr,"   ERROR: Could not read sysinfo.\n");
		return -1;
	}


	memset(buf,0,sizeof(buf));
	strncpy(buf,info.prodID,info.prodIDLength < sizeof(buf)-1 ? info.prodIDLength : sizeof(buf)-1);

	fprintf(stdout,"ProdId:%s\n",buf);
	fprintf(stdout,"ROM   :%ld\n",info.romVersion);
	fprintf(stdout,"DLP   :%d.%d\n",info.dlpMajorVersion,info.dlpMinorVersion);
	fprintf(stdout,"Compat:%d.%d\n",info.compatMajorVersion,info.compatMinorVersion);
	fprintf(stdout,"Locale:%ld\n",info.locale);
	fprintf(stdout,"MaxRec:%ld\n",info.maxRecSize);

	return 0;
}



int main(int argc, const char *argv[])
{
	int 	c,		/* switch */
		sd		= -1;

	const char **args = NULL;
	const char *filename = NULL;
	char *token = NULL;

	enum { mode_none, mode_getrom = 'R', mode_getram = 'r', mode_gettoken = 't', mode_sysinfo='s' } mode = mode_none;

	poptContext po;

	struct poptOption options[] = {
		USERLAND_RESERVED_OPTIONS
		{"token", 't', POPT_ARG_STRING, &token, 't', "A ROM token to read (i.e. snum)", "token"},
		{"sysinfo",'s',POPT_ARG_NONE,     NULL, 's', "Print SysInfo", NULL},
		{"ram",    0 , POPT_ARG_NONE,     NULL, 'r', "Read RAM", NULL},
		{"rom",    0 , POPT_ARG_NONE,     NULL, 'R', "Read ROM", NULL},
		POPT_TABLEEND
	};

	const char *progname = NULL;
	const char *opthelp = NULL;

	/* These are optimized away compile-time */
	if (DEFAULT_MODE == mode_getrom) {
		progname = "pi-getrom";
		opthelp=
		"[filename]\n\n"
		"   Retrieves the ROM image from your Palm device.\n\n";
	} else if (DEFAULT_MODE == mode_gettoken) {
		progname="pi-getromtoken";
		opthelp=
		"\n\n"
		"   Reads a ROM token from a Palm Handheld device.\n"
		"   Tokens you may currently extract are:\n"
		"       adcc:  Entropy for internal A->D convertor calibration\n"
		"       irda:  Present only on memory card w/IrDA support\n"
		"       snum:  Device serial number (from Memory Card Flash ID)\n\n"
		"   Example arguments:\n"
		"      -p /dev/pilot -t snum\n\n";
	} else if (DEFAULT_MODE == mode_getram) {
		progname="pi-getram";
		opthelp=
		"[filename]\n\n"
		"   Retrieves the RAM image from your Palm device\n\n";
	}

	if (!progname) {
		fprintf(stderr,"   ERROR: Compile-time error.\n");
		return -1;
	}




	po = poptGetContext(progname, argc, argv, options, 0);
	poptSetOtherOptionHelp(po,opthelp);

	if (argc < 2) {
		poptPrintUsage(po,stderr,0);
		return 1;
	}
	while ((c = poptGetNextOpt(po)) >= 0) {
		switch(c) {
		case 's' :
		case 't' :
		case 'r' :
		case 'R' :
			if (mode != mode_none) {
				fprintf(stderr,"   ERROR: Use only one of --ram, --rom, and --token.\n");
				return 1;
			}
			mode =c;
			break;
		default:
			fprintf(stderr,"   ERROR: Unhandled option %d.\n",c);
			return 1;
		}
	}

	if ( c < -1) {
		plu_badoption(po,c);
	}

	if (mode == mode_none) {
		mode = DEFAULT_MODE;
	}

	switch(mode) {
	case mode_getrom:
	case mode_getram:
		args = poptGetArgs(po);
		if (args && args[0] && args[1]) {
			fprintf(stderr,"   ERROR: Specify at most one filename.\n");
			return 1;
		}
		if (args && args[0]) {
			filename = args[0];
		}
		break;
	case mode_gettoken:
		if (!token) {
			fprintf(stderr,"   ERROR: Must specify a token.\n");
			return 1;
		}
		break;
	case mode_sysinfo:
		break;
	case mode_none:
		/* impossible */
		return 1;
	}



	switch(mode) {
	case mode_getrom:
		printf(
		"   NOTICE: Use of this program may place you in violation\n"
		"           of your license agreement with Palm Computing.\n\n"
		"           Please read your Palm Computing handbook (\"Software\n"
		"           License Agreement\") before running this program.\n\n");
		/* FALLTHRU */
	case mode_getram:
		printf(
		"   WARNING: Please completely back up your Palm (with pilot-xfer -b)\n"
		"            before using this program!\n\n");
		break;
	case mode_sysinfo:
	case mode_none:
	case mode_gettoken:
		/* nothing to do */
		break;
	}


	sd = plu_connect();
	if (sd < 0) {
		return -1;
	}

	switch(mode) {
	case mode_getrom:
		c = do_get_rom(sd,filename);
		break;
	case mode_gettoken:
		c = do_get_token(sd,token);
		break;
	case mode_getram:
		c = do_get_ram(sd,filename);
		break;
	case mode_sysinfo:
		c = do_sysinfo(sd);
		break;
	case mode_none:
		/* impossible */
		break;
	}

	dlp_EndOfSync(sd,0);
	pi_close(sd);
	return c;
}

