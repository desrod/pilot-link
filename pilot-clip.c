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

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-syspkt.h"
#include "pi-dlp.h"
#include "pi-header.h"

/* Declare prototypes */
void *GetClip(int socket, int type, int *length);
int SetClip(int socket, int type, void *data, int length);


void *GetClip(int socket, int type, int *length)
{
	struct RPC_params p;
	int l, err;
	unsigned long handle, ptr;
	void *buffer;

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

int SetClip(int socket, int type, void *data, int length)
{
	struct RPC_params p;
	int err;
	char *b = data;
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


int main(int argc, char *argv[])
{
	struct pi_sockaddr addr;
	int sd;
	int ret;
	char buffer[0xffff];
	char *progname = argv[0];
	char *device = argv[1];

	PalmHeader(progname);

	if (argc < 3) {
		fprintf(stderr, "   Usage: %s %s -s|-g\n\n", argv[0],
			TTYPrompt);
		fprintf(stderr,
			"     -s     = set the value of the clipboard, i.e.\n");
		fprintf(stderr,
			"              create data on the clipboard\n");
		fprintf(stderr,
			"     -g     = get the current data from the clipboard\n\n");
		exit(2);
	}

	if (strcmp(argv[2], "-s") && strcmp(argv[2], "-g")) {
		fprintf(stderr, "   Usage: %s %s -s[et]|-g[et]\n\n",
			argv[0], TTYPrompt);
		exit(2);
	}

	if (!(sd = pi_socket(PI_AF_PILOT, PI_SOCK_STREAM, PI_PF_PADP))) {
		perror("pi_socket");
		exit(1);
	}

	addr.pi_family = PI_AF_PILOT;
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

	if (strcmp(argv[2], "-s") == 0) {
		ret = read(fileno(stdin), buffer, 0xffff);
/*		dumpdata(buffer, ret); */
		if (ret >= 0) {
			buffer[ret++] = 0;
			SetClip(sd, 0, buffer, ret);
		}
	} else {
		char *b;

		ret = 0;
		b = GetClip(sd, 0, &ret);
		if (ret > 0)
			write(fileno(stdout), b, ret);
	}

	pi_close(sd);

	return 0;
}
