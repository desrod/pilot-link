/* 
 * debugsh.c:  Simple debugging console
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>

#include "pi-source.h"
#include "pi-dlp.h"
#include "pi-syspkt.h"
#include "pi-header.h"

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

/* Declare prototypes */
void read_user(int sd);
void read_pilot(int sd);
void sig(int signal);

int 	done = 0;

void read_user(int sd)
{
	char	line[256];
	int 	l = read(fileno(stdin), line, 256);
	
	if (l > 0)
		line[l - 1] = 0;

	if (strcmp(line, "apps") == 0) {
		sys_RemoteEvent(sd, 1, 5, 170, 0, 0, 0, 0);	/* Set the pen down */
		sys_RemoteEvent(sd, 0, 5, 170, 0, 0, 0, 0);	/* And then lift it up */
	} else if (strcmp(line, "menu") == 0) {
		sys_RemoteEvent(sd, 1, 5, 200, 0, 0, 0, 0);	/* Set the pen down */
		sys_RemoteEvent(sd, 0, 5, 200, 0, 0, 0, 0);	/* And then lift it up */
	} else if (strcmp(line, "reboot") == 0) {
		RPC(sd, 1, 0xA08C, 2, RPC_End);
	} else if (strcmp(line, "coldboot") == 0) {
		RPC(sd, 1, 0xA08B, 2, RPC_Long(0), RPC_Long(0),
		    RPC_Long(0), RPC_Long(0), RPC_End);
	} else if (strcmp(line, "numdb") == 0) {
		printf("Number of databases on card 0: %d\n",
		       RPC(sd, 1, 0xA043, 0, RPC_Short(0), RPC_End)
		    );
	} else if (strcmp(line, "dbinfo") == 0) {
		long creator, type, appInfo, sortInfo, modnum, backdate,
		    moddate, crdate, version, attr;
		char name[32];

		int id_ =
		    RPC(sd, 1, 0xA044, 0, RPC_Short(0), RPC_Short(0),
			RPC_End);

		RPC(sd, 1, 0xA046, 0, RPC_Short(0), RPC_Long(id_),
		    RPC_Ptr(name, 32),
		    RPC_ShortRef(attr), RPC_ShortRef(version),
		    RPC_LongRef(crdate), RPC_LongRef(moddate),
		    RPC_LongRef(backdate), RPC_LongRef(modnum),
		    RPC_LongRef(appInfo), RPC_LongRef(sortInfo),
		    RPC_LongRef(type), RPC_LongRef(creator), RPC_End);

		printf("The name of db 0 (LocalID %x) is %s\n", id_, name);

	} else if (strcmp(line, "quit") == 0) {
		done = 1;
	} else if (l > 1) {
		printf
		    ("unknown command '%s' (try 'apps', 'menu', 'coldboot', 'reboot', 'dbinfo', or 'quit')\n",
		     line);
	}

	if (!done) {
		printf("debugsh>");
		fflush(stdout);
	}
	if (l == 0)
		done = 1;
}

void read_pilot(int sd)
{
	pi_buffer_t *buf = pi_buffer_new (4096);
	int 	l = pi_read(sd, buf, 4096);
	
	printf("From Palm %d:", l);
	if (l < 0)
		exit(EXIT_FAILURE);
	
	dumpdata(buf->data, l);

	if (buf->data[2] == 0) {			/* SysPkt command 	*/
		if (buf->data[0] == 1) {		/* Console 		*/
			if (buf->data[4] == 0x7f) {	/* Message from Palm 	*/
				int i;

				for (i = 6; i < l; i++)
					if (buf->data[i] == '\r')
						buf->data[i] = '\n';
				printf("%s", buf->data + 6);
			}
		}
	}

	pi_buffer_free (buf);

	if (!done) {
		printf("debugsh>");
		fflush(stdout);
	}

}

void sig(int signal)
{
	done = 1;
}

int main(int argc, char *argv[])
{
	int 	max,
		sd = -1;
	
	fd_set r, rin;

	if (argc < 2) {
		fprintf(stderr, "   Usage: %s %s\n\n", argv[0], TTYPrompt);
		exit(2);
	}

	sd = pi_socket(PI_AF_PILOT, PI_SOCK_STREAM, PI_PF_SYS);
	if (sd < 0) {
		fprintf(stderr, "Unable to create socket\n");
		return -1;
	}

	if (pi_connect(sd, argv[1]) < 0) {
		fprintf(stderr, "Unable to connect\n");
		return -1;
	}
	
	/* Now we can read and write packets: to get the Palm to send a
	   packet, write a ".2" shortcut, which starts the debugging mode.
	   (Make sure to reset your Palm after finishing this example!) */

	FD_ZERO(&r);
	FD_SET(sd, &r);
	FD_SET(fileno(stdin), &r);

	max = sd;
	if (fileno(stdin) > max)
		max = fileno(stdin);

	printf("debugsh>");
	fflush(stdout);

	signal(SIGINT, sig);

	while (!done) {
		rin = r;
		if (select(max + 1, &rin, 0, 0, 0) >= 0) {
			if (FD_ISSET(fileno(stdin), &rin)) {
				read_user(sd);
			} else if (FD_ISSET(sd, &rin)) {
				read_pilot(sd);
			}
		} else {
			break;
		}
	}

	printf("\n   Exiting...\n");
	pi_close(sd);

	return 0;
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
