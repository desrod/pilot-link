/*
 * pi-port.c:  Serial server
 *
 * Copyright (c) 1997, Kenneth Albanowski
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
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-serial.h"
#include "pi-slp.h"
#include "pi-header.h"

/* Declare prototypes */
void sigint(int num);
void do_read(struct pi_socket *ps, int type, char *buffer, int length);

void sigint(int num)
{
	*((char *) 0) = 0;
}

/***********************************************************************
 *
 * Function:    do_read
 *
 * Summary:     Read the incoming data from the network socket
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void do_read(struct pi_socket *ps, int type, char *buffer, int length)
{
	int len;

	fprintf(stderr,
		"A %d byte packet of type %d has been received from the network\n",
		length, type);
/*	dumpdata(buffer, length); */
	if (type == 0) {
		struct pi_skb *nskb;
		nskb = (struct pi_skb *) malloc(sizeof(struct pi_skb));

		nskb->source = buffer[0];
		nskb->dest = buffer[1];
		nskb->type = buffer[2];
		len = get_short(buffer + 3);
		nskb->id = buffer[5];

		memcpy(&nskb->data[10], buffer + 7, len);
		slp_tx(ps, nskb, len);

	} else if (type == 1) {

		ps->rate = get_long(buffer);
		pi_serial_flush(ps);
		ps->serial_changebaud(ps);
	}
}

int main(int argc, char *argv[])
{
	struct pi_sockaddr addr;
	int sd;
	struct pi_socket *ps;
	int ret;
	struct sockaddr_in serv_addr;
	int serverfd, fd;
	char *buffer;
	char *slpbuffer;
	char *device = argv[1];
	char *progname = argv[0];
	int port = 4386;

	extern char *optarg;
	extern int optind;

	PalmHeader(progname);

	if (argc < 2) {
		fprintf(stderr, "   Usage: %s /dev/tty<0..n>\n\n",
			progname);
		exit(0);
	}

	signal(SIGINT, sigint);

	buffer = malloc(0xFFFF + 128);
	slpbuffer = malloc(0xFFFF + 128);

	if ((serverfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to obtain socket");
		goto end;
	}

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);

	if (bind
	    (serverfd, (struct sockaddr *) &serv_addr,
	     sizeof(serv_addr)) < 0) {
		perror("Unable to bind local address");
		goto end;
	}

	listen(serverfd, 5);

	if (!(sd = pi_socket(PI_AF_PILOT, PI_SOCK_RAW, PI_PF_SLP))) {
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

	ps = find_pi_socket(sd);
	ps->rate = 9600;
	ps->serial_changebaud(ps);

	for (;;) {
		int l;
		int max;
		fd_set rset, wset, eset, oset;
		struct sockaddr_in conn_addr;
		unsigned int connlen = sizeof(conn_addr);
		int sent;

		fd = accept(serverfd, (struct sockaddr *) &conn_addr,
			    &connlen);

		if (fd < 0) {
			perror("Server: accept error");
			goto end;
		}

		FD_ZERO(&oset);
		FD_SET(fd, &oset);
		FD_SET(sd, &oset);

		sent = 0;
		l = 0;

		max = fd;
		if (sd > max)
			max = sd;
		max++;

		for (;;) {
			rset = wset = eset = oset;
			select(max, &rset, &wset, &eset, 0);

			if (FD_ISSET(fd, &rset)) {
				int r = read(fd, buffer + l, 0xFFFF - l);

				if (r < 0)
					break;

				if (r == 0)
					goto skip;

				fprintf(stderr,
					"Read %d bytes from network at block+%d:\n",
					r, l);
/*				dumpdata(buffer + l, r); */

				l += r;
				if (l >= 4) {
					int blen;

					while (l >= 4
					       && (l >=
						   (blen =
						    get_short(buffer +
							      2)) + 4)) {
						fprintf(stderr,
							"l = %d, blen = %d\n",
							l, blen);
						do_read(ps,
							get_short(buffer),
							buffer + 7, blen);
						l = l - blen - 4;
						if (l > blen) {
							memmove(buffer,
								buffer +
								4 + blen,
								l);
						}
						fprintf(stderr,
							"Buffer now is:\n");
/*						dumpdata(buffer, l); */
					}
				}
			      skip:
				;
			}
			if (FD_ISSET(sd, &rset)) {
				ps->serial_read(ps, 1);
				if (ps->rxq) {
					fprintf(stderr,
						"A %d byte packet has been received from the serial port\n",
						ps->rxq->len);
				}
			}
			if (FD_ISSET(fd, &wset) && ps->rxq) {
				int w =
				    write(fd, ps->rxq->data + sent,
					  ps->rxq->len - sent);

				if (w < 0)
					break;

				sent += w;
				if (w >= ps->rxq->len) {
					struct pi_skb *skb = ps->rxq;

					ps->rxq = ps->rxq->next;

					fprintf(stderr,
						"A %d byte packet has been sent over the network\n",
						skb->len);
					free(skb);
					sent = 0;
				}

			}
			if (FD_ISSET(sd, &wset) && ps->txq) {
				fprintf(stderr,
					"A %d byte packet is being sent to the serial port\n",
					ps->txq->len);
				ps->serial_write(ps);
			}
			if (FD_ISSET(fd, &eset)) {
				break;
			}
			if (FD_ISSET(sd, &eset)) {
				break;
			}
		}
	}
      end:
	return 0;
}
