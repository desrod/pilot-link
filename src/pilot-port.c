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

#include "getopt.h"
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
void do_read(struct pi_socket *ps, int type, char *buffer, int length);

int pilot_connect(const char *port);
static void Help(char *progname);

/* Not used yet, getopt_long() coming soon! 
struct option options[] = {
	{"help",        no_argument,       NULL, 'h'},
	{"port",        required_argument, NULL, 'p'},
	{NULL,          0,                 NULL, 0}
};
*/

static const char *optstring = "hp:";


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
	int 	len;

	printf("A %d byte packet of type %d has been received from the network\n",
		length, type);
	dumpdata(buffer, length);
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


static void Help(char *progname)
{
	printf("   Reads incoming remote Palm data during a Network HotSync\n\n"
	       "   Usage: %s -p <port>\n\n"
	       "   Options:\n"
	       "     -p <port>    Use device file <port> to communicate with Palm\n"
	       "     -h           Display this information\n\n"
	       "   Examples: %s -p /dev/pilot\n\n", progname, progname);
	return;
}

int main(int argc, char *argv[])
{
	int 	c,		/* switch */
		sd 		= -1,
		netport 	= 4386,
		serverfd, fd;

	struct 	pi_socket *ps;
	struct 	sockaddr_in serv_addr;

	char 	*buffer,
		*slpbuffer,
		*progname 	= argv[0],
		*port 		= NULL;
	
	while ((c = getopt(argc, argv, optstring)) != -1) {
		switch (c) {

		  case 'h':
			  Help(progname);
			  exit(0);
		  case 'p':
			  port = optarg;
			  break;
		}
	}

	if (argc < 2 && !getenv("PILOTPORT")) {
		PalmHeader(progname);
	} else if (port == NULL && getenv("PILOTPORT")) {
		port = getenv("PILOTPORT");
	}

	if (port == NULL && argc > 1) {
		printf
		    ("\nERROR: At least one command parameter of '-p <port>' must be set, or the\n"
		     "environment variable $PILOTPORT must be used if '-p' is omitted or missing.\n");
		exit(1);
	} else if (port != NULL) {
	
		sd = pilot_connect(port);

		/* Did we get a valid socket descriptor back? */
		if (dlp_OpenConduit(sd) < 0) {
			exit(1);
		}

		buffer = malloc(0xFFFF + 128);
		slpbuffer = malloc(0xFFFF + 128);

		if ((serverfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			perror("Unable to obtain socket");
			goto end;
		}
	
		memset((char *) &serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		serv_addr.sin_port = htons(netport);
	
		if (bind
		    (serverfd, (struct sockaddr *) &serv_addr,
		     sizeof(serv_addr)) < 0) {
			perror("Unable to bind local address");
			goto end;
		}
	
		listen(serverfd, 5);
	
		sd = pilot_connect(port);
		
		ps = find_pi_socket(sd);
		ps->rate = 9600;
		ps->serial_changebaud(ps);
	
		for (;;) {
			int 	l,
				max,
				sent;

			fd_set 	rset,
				wset, 
				eset, 
				oset;

			struct 	sockaddr_in conn_addr;
			unsigned int connlen = sizeof(conn_addr);
	
			fd = accept(serverfd, (struct sockaddr *) &conn_addr,
				    &connlen);
	
			if (fd < 0) {
				perror("Server: accept error");
				goto end;
			}
	
			FD_ZERO(&oset);
			FD_SET(fd, &oset);
			FD_SET(sd, &oset);
	
			sent 	= 0;
			l 	= 0;
	
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
	
					printf("Read %d bytes from network at block+%d:\n",
						r, l);
					dumpdata(buffer + l, r);
	
					l += r;
					if (l >= 4) {
						int blen;
	
						while (l >= 4
						       && (l >=
							   (blen =
							    get_short(buffer +
								      2)) + 4)) {
							printf("l = %d, blen = %d\n",
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
							printf("Buffer now is:\n");
							dumpdata(buffer, l);
						}
					}
				      skip:
					;
				}
				if (FD_ISSET(sd, &rset)) {
					ps->serial_read(ps, 1);
					if (ps->rxq) {
						printf("A %d byte packet has been received from the serial port\n",
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
	
						printf("A %d byte packet has been sent over the network\n",
							skb->len);
						free(skb);
						sent = 0;
					}
	
				}
				if (FD_ISSET(sd, &wset) && ps->txq) {
					printf("A %d byte packet is being sent to the serial port\n",
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
	}
	end:
	return 0;
}
