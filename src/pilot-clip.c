/*
 * pilot-clip.c:  Transfer data to or from the Palm's clipboard
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
#include <unistd.h>
#include <netinet/in.h>

#include "pi-source.h"
#include "pi-syspkt.h"
#include "pi-dlp.h"
#include "pi-header.h"
#include "userland.h"

static void *GetClip(int socket, int type, int *length)
{
	int 	l,
		err;
	struct 	RPC_params p;
	unsigned long handle, ptr;
	void 	*buffer;

	/* ClipboardGetItem */
	PackRPC(&p, 0xA10C, RPC_PtrReply, RPC_Byte(type), RPC_ShortPtr(&l),
		RPC_End);
	err = dlp_RPC(socket, &p, &handle);
	if (err)
		return 0;

	if (!handle)
		return 0;

	/* MemHandleLock */
	PackRPC(&p, 0xA021, RPC_PtrReply, RPC_Long(handle), RPC_End);
	err = dlp_RPC(socket, &p, &ptr);

	if (err)
		return 0;

	buffer = malloc(l);

	/* MemMove */
	PackRPC(&p, 0xA026, RPC_IntReply, RPC_Ptr(buffer, l),
		RPC_Long(ptr), RPC_Long(l), RPC_End);
	err = dlp_RPC(socket, &p, 0);

	/* MemHandleUnlock */
	PackRPC(&p, 0xA022, RPC_IntReply, RPC_Long(handle), RPC_End);
	err = dlp_RPC(socket, &p, 0);

	if (length)
		*length = l;

	return buffer;
}

static int SetClip(int socket, int type, void *data, int length)
{
	int 	err;
	char 	*b = data;
	struct 	RPC_params p;
	unsigned long handle, ptr;

	/* MemHandleNew */
	PackRPC(&p, 0xA01E, RPC_PtrReply, RPC_Long(length), RPC_End);
	err = dlp_RPC(socket, &p, &handle);
	if (err)
		return 0;

	if (!handle)
		return 0;

	/* MemHandleLock */
	PackRPC(&p, 0xA021, RPC_PtrReply, RPC_Long(handle), RPC_End);
	err = dlp_RPC(socket, &p, &ptr);

	if (err)
		return 0;

	/* MemMove */
	PackRPC(&p, 0xA026, RPC_IntReply, RPC_Long(ptr),
		RPC_Ptr(b, length), RPC_Long(length), RPC_End);
	err = dlp_RPC(socket, &p, 0);

	length--;

	/* ClipboardAddItem */
	PackRPC(&p, 0xA10A, RPC_IntReply, RPC_Byte(type), RPC_Long(ptr),
		RPC_Short(length), RPC_End);
	err = dlp_RPC(socket, &p, 0);

	/* MemPtrFree */
	PackRPC(&p, 0xA012, RPC_IntReply, RPC_Long(ptr), RPC_End);
	err = dlp_RPC(socket, &p, 0);
	return 1;
}

static void display_help(const char *progname)
{
	printf("   Get or Set the Palm Clipboard contents from STDOUT/STDIN\n\n");
	printf("   Usage: %s -p <port> -g | -s <value>\n\n", progname);
	printf("   Options:\n");
	printf("     -p, --port <port>       Use device file <port> to communicate with Palm\n");
	printf("     -h, --help              Display help information for %s\n", progname);
	printf("     -v, --version           Display %s version information\n", progname);
	printf("     -g, --get               Get the contents of the clipboard\n");
	printf("     -s, --set <value>       Set the value <value> in the clipboard\n\n");
	printf("   Examples: %s -p serial:/dev/ttyUSB0 -g\n", progname);
	printf("             %s -p serial:/dev/ttyUSB0 -s \"Put this in the clipboard\"\n\n", progname);

	return;
}

int main(int argc, const char *argv[])
{
	int 	c,		/* switch */
		sd 		= -1,
		getset          = -1,
		ret;

	const char
                *progname 	= argv[0];

	char 	buffer[0xffff],
		*port		= NULL;

	poptContext po;

	struct poptOption options[] = {
        	{"port",	'p', POPT_ARG_STRING, &port, 0,  "Use device <port> to communicate with Palm"},
	        {"help",	'h', POPT_ARG_NONE, NULL,   'h', "Display this information"},
                {"version",	'v', POPT_ARG_NONE, NULL,   'v', "Display version information"},
	        {"get",		'g', POPT_ARG_NONE, NULL,   'g', "Get the contents of the clipboard"},
        	{"set",		's', POPT_ARG_NONE, NULL,   's', "Set the value <value> in the clipboard"},
	         POPT_AUTOHELP
                { NULL, 0, 0, NULL, 0 }
	};

	po = poptGetContext("pilot-clip", argc, argv, options, 0);

	while ((c = poptGetNextOpt(po)) >= 0) {
		switch (c) {

		case 'h':
			display_help(progname);
			return 0;
		case 'v':
			print_splash(progname);
			return 0;
		case 'g':
			getset = 0;
			break;
		case 's':
			getset = 1;
			break;
		default:
			display_help(progname);
			return 0;
		}
	}
	if (getset < 0) {
		display_help(progname);
		fprintf(stderr, "ERROR: You must specify whether to "
				"\"Get\" or \"Set\" the clipboard\n");
		return -1;
	}

	sd = pilot_connect(port);
	if (sd < 0)
		goto error;

	if (dlp_OpenConduit(sd) < 0)
		goto error_close;


	if (getset == 1) {
		ret = read(fileno(stdin), buffer, 0xffff);
		if (ret >= 0) {
			buffer[ret++] = 0;
			if (SetClip(sd, 0, buffer, ret) <= 0)
				goto error_close;
		}
	} else {
		char *b;

		ret = 0;
		b = GetClip(sd, 0, &ret);
		if (b == NULL)
			goto error_close;
		if (ret > 0)
			write(fileno(stdout), b, ret);
	}

	if (pi_close(sd) < 0)
		goto error;

	return 0;

error_close:
	pi_close(sd);

error:
	return -1;
}
