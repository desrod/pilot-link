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

#include "getopt.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-syspkt.h"
#include "pi-dlp.h"
#include "pi-header.h"


int pilot_connect(const char *port);

struct option options[] = {
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
	{"port",        required_argument, NULL, 'p'},
	{"get",         no_argument,       NULL, 'g'},
	{"set",         no_argument,       NULL, 's'},
	{NULL,          0,                 NULL, 0}
};

static const char *optstring = "hvp:gs";

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

static void Help(char *progname)
{
	printf("   Get the clipboard contents to stdout or set the clipboard contents from stdin\n\n"
	       "   Usage: %s -p <port> -g | -s <value>\n"
	       "   Options:\n"
	       "     -p <port>         Use device file <port> to communicate with Palm\n"
	       "     -g                Get the contents of the clipboard\n"
	       "     -s <value>        Set the value <value> in the clipboard\n"
	       "     -h                Display this information\n\n"
	       "   Examples: %s -p /dev/pilot -g\n"
	       "             %s -p /dev/pilot -s \"Put this in the clipboard\"\n\n", 
	       progname, progname, progname);
	return;
}

int main(int argc, char *argv[])
{
	int 	count,
		sd 		= -1,
		getset          = -1,
		ret;
	char 	buffer[0xffff],
		*progname 	= argv[0],
		*port		= NULL;

	while ((count = getopt_long(argc, argv, optstring, options, NULL)) != -1) {
		switch (count) {

		case 'h':
			Help(progname);
			exit(0);
		case 'v':
			PalmHeader(progname);
			return 0;
		case 'p':
			port = optarg;
			break;
		case 'g':
			getset = 0;
			break;
		case 's':
			getset = 1;
			break;
		}
	}
	if (getset < 0) {
		Help(progname);
		fprintf(stderr, "ERROR: You must specify whether to get or set the clipboard\n");
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
	perror("\tERROR:");
	fprintf(stderr, "\n");

	return -1;	
}
