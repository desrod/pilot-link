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
#include "pi-userland.h"

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


int main(int argc, const char *argv[])
{
	int 	c,		/* switch */
		sd 		= -1,
		ret;

	enum { mode_none, mode_set='s', mode_get='g' } mode = mode_none;

	char 	buffer[0xffff];

	poptContext po;

	struct poptOption options[] = {
		USERLAND_RESERVED_OPTIONS
	        {"get",		'g', POPT_ARG_NONE, NULL,   'g', "Print the contents of the clipboard on STDOUT"},
        	{"set",		's', POPT_ARG_NONE, NULL,   's', "Set the value in the clipboard from STDIN"},
	         POPT_AUTOHELP
                { NULL, 0, 0, NULL, 0 }
	};

	po = poptGetContext("pilot-clip", argc, argv, options, 0);
	poptSetOtherOptionHelp(po,"\n\n"
	"   Get or Set the Palm Clipboard contents from STDOUT/STDIN.\n"
	"   Must provide one of -g or -s.\n\n");

	if (argc < 2) {
		poptPrintUsage(po,stderr,0);
		return 1;
	}

	while ((c = poptGetNextOpt(po)) >= 0) {
		switch (c) {
		case 's':
		case 'g':
			if (mode != mode_none) {
				fprintf(stderr,"   ERROR: Use only one of --get or --set.\n");
				return 1;
			}
			mode = c;
			break;
		default:
			fprintf(stderr,"   ERROR: Unhandled option %d.\n", c);
			return 1;
		}
	}
	if (mode_none == mode) {
		fprintf(stderr, "   ERROR: You must specify whether to "
				"\"Get\" or \"Set\" the clipboard\n");
		return -1;
	}

	sd = plu_connect();
	if (sd < 0)
		goto error;

	if (dlp_OpenConduit(sd) < 0)
		goto error_close;


	if (mode == mode_set) {
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
