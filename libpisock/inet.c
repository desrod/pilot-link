/*
 * inet.c: Interface layer to TCP/IP NetSync connections
 *
 * Copyright (c) 1997, Kenneth Albanowski
 * Copyright (c) 1999, Tilo Christ
 * Copyright (c) 1999, John Franks
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef WIN32
#include <winsock.h>
#include <fcntl.h>
#include <io.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include <stdio.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-inet.h"
#include "pi-slp.h"
#include "pi-syspkt.h"
#include "pi-padp.h"
#include "pi-dlp.h"

static int pi_net_listen(struct pi_socket *ps, int backlog);
static int pi_net_accept(struct pi_socket *ps, struct sockaddr *addr,
			 int *addrlen);
static int pi_net_send(struct pi_socket *ps, void *msg, int len,
		       unsigned int flags);
static int pi_net_recv(struct pi_socket *ps, void *msg, int len,
		       unsigned int flags);
static int pi_net_tickle(struct pi_socket *ps);
static int pi_net_close(struct pi_socket *ps);

extern int dlp_trace;

/***********************************************************************
 *
 * Function:    pi_inet_connect
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_inet_connect(struct pi_socket *ps, struct sockaddr *addr, int addrlen)
{
	struct sockaddr_in serv_addr;
	char msg1[22] = "\x90\x01\x00\x00\x00\x00\x00\x00\x00\x20\x00\x00\x00"
                        "\x08\x01\x00\x00\x00\x00\x00\x00\x00";
	char msg2[50] = "\x92\x01\x00\x00\x00\x00\x00\x00\x00\x20\x00\x00\x00"
                        "\x24\xff\xff\xff\xff\x00\x3c\x00\x3c\x40\x00\x00\x00"
                        "\x01\x00\x00\x00\xc0\xa8\xa5\x1e\x04\x01\x00\x00\x00"
                        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
	char msg3[8]  = "\x93\x00\x00\x00\x00\x00\x00\x00";
	char buffer[200];

	ps->mac->fd = socket(AF_INET, SOCK_STREAM, 0);

	if (ps->sd) {
		int orig = ps->mac->fd;

#ifdef HAVE_DUP2
		ps->mac->fd = dup2(ps->mac->fd, ps->sd);
#else
#ifdef F_DUPFD
		close(ps->sd);
		ps->mac->fd = fcntl(ps->mac->fd, F_DUPFD, ps->sd);
#else
		close(ps->sd);
		ps->mac->fd = dup(ps->mac->fd);	/* Unreliable */
#endif
#endif
		if (ps->mac->fd != orig)
			close(orig);
	}

	if (addr->sa_family == AF_INET) {
		memcpy(&serv_addr, addr, addrlen);
	} else {
		struct pi_sockaddr *paddr = (struct pi_sockaddr *) addr;
		char *device = paddr->pi_device + 1;

		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		serv_addr.sin_port = htons(14238);
		if (strlen(device) > 1) {
			if ((serv_addr.sin_addr.s_addr =
			     inet_addr(device)) == -1) {
				struct hostent *hostent =
				    gethostbyname(device);

				if (!hostent) {
					fprintf(stderr,
						"Unable to resolve host '%s'",
						device);
					return -1;
				}
				memcpy((char *) &serv_addr.sin_addr.s_addr,
				       hostent->h_addr, hostent->h_length);
			}
		}
	}

	if (connect
	    (ps->mac->fd, (struct sockaddr *) &serv_addr,
	     sizeof(serv_addr))
	    < 0)
		return -1;

	ps->socket_listen = pi_net_listen;
	ps->socket_accept = pi_net_accept;
	ps->socket_send = pi_net_send;
	ps->socket_recv = pi_net_recv;
	ps->socket_tickle = pi_net_tickle;
	ps->socket_close = pi_net_close;

	ps->initiator = 1;

	ps->connected = 1;

	ps->version = 0x0101;

	pi_net_send(ps, msg1, 22, 0);
	pi_net_recv(ps, buffer, 200, 0);
	pi_net_send(ps, msg2, 50, 0);
	pi_net_recv(ps, buffer, 200, 0);
	pi_net_send(ps, msg3, 8, 0);

#ifndef NO_SERIAL_TRACE
	if (ps->debuglog) {
		ps->debugfd =
		    open(ps->debuglog, O_WRONLY | O_CREAT | O_APPEND,
			 0666);
		/* This sequence is magic used by my trace analyzer - kja */
		write(ps->debugfd, "\0\2\0\0\0\0\0\0\0\0", 10);
	}
#endif

	return 0;
}

/***********************************************************************
 *
 * Function:    pi_inet_bind
 *
 * Summary:     Bind address to a local socket
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_inet_bind(struct pi_socket *ps, struct sockaddr *addr, int addrlen)
{
	int opt, optlen;
	struct sockaddr_in serv_addr;

	ps->mac->fd = socket(AF_INET, SOCK_STREAM, 0);

	if (ps->sd) {
		int orig = ps->mac->fd;

#ifdef HAVE_DUP2
		ps->mac->fd = dup2(ps->mac->fd, ps->sd);
#else
#ifdef F_DUPFD
		close(ps->sd);
		ps->mac->fd = fcntl(ps->mac->fd, F_DUPFD, ps->sd);
#else
		close(ps->sd);
		ps->mac->fd = dup(ps->mac->fd);	/* Unreliable */
#endif
#endif
		if (ps->mac->fd != orig)
			close(orig);
		else {
			puts("Unable to duplicate socket");
			exit(1);
		}
	}

	if (addr->sa_family == AF_INET) {
		memcpy(&serv_addr, addr, addrlen);
	} else {
		struct pi_sockaddr *paddr = (struct pi_sockaddr *) addr;
		char *device = paddr->pi_device + 1;

		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		serv_addr.sin_port = htons(14238);
		if (strlen(device) > 1) {
			if ((serv_addr.sin_addr.s_addr =
			     inet_addr(device)) == -1) {
				struct hostent *hostent =
				    gethostbyname(device);

				if (!hostent) {
					fprintf(stderr,
						"Unable to resolve host '%s'",
						device);
					return -1;
				}
				memcpy((char *) &serv_addr.sin_addr.s_addr,
				       hostent->h_addr, hostent->h_length);
			}
		}
	}

	opt = 1;
	optlen = sizeof(opt);
#ifdef WIN32
	if (setsockopt
	    (ps->sd, SOL_SOCKET, SO_REUSEADDR, (const char *) &opt,
	     optlen) < 0) {
		return -1;
	}
#else
	if (setsockopt
	    (ps->sd, SOL_SOCKET, SO_REUSEADDR, (void *) &opt,
	     optlen) < 0) {
		return -1;
	}
#endif

	if (bind(ps->sd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))
	    < 0)
		return -1;

#ifndef NO_SERIAL_TRACE
	if (ps->debuglog) {
		ps->debugfd =
		    open(ps->debuglog, O_WRONLY | O_CREAT | O_APPEND,
			 0666);
		/* This sequence is magic used by my trace analyzer - kja */
		write(ps->debugfd, "\0\2\0\0\0\0\0\0\0\0", 10);
	}
#endif

	ps->socket_listen = pi_net_listen;
	ps->socket_accept = pi_net_accept;
	ps->socket_send = pi_net_send;
	ps->socket_recv = pi_net_recv;
	ps->socket_tickle = pi_net_tickle;
	ps->socket_close = pi_net_close;

	ps->initiator = 0;

	ps->connected = 1;

	ps->version = 0x0101;

	return 0;
}

/***********************************************************************
 *
 * Function:    pi_net_listen
 *
 * Summary:     Wait for an incoming connection
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int pi_net_listen(struct pi_socket *ps, int backlog)
{
	return listen(ps->sd, backlog);
}

/***********************************************************************
 *
 * Function:    pi_net_accept
 *
 * Summary:     Accept an incoming connection
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int
pi_net_accept(struct pi_socket *ps, struct sockaddr *addr, int *addrlen)
{
	struct pi_socket *a;
	char msg1[50] = "\x12\x01\x00\x00\x00\x00\x00\x00\x00\x20\x00\x00\x00"
                        "\x24\xff\xff\xff\xff\x3c\x00\x3c\x00\x00\x00\x00\x00"
                        "\x00\x00\x00\x00\xc0\xa8\xa5\x1f\x04\x27\x00\x00\x00"
                        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
	char msg2[46] = "\x13\x01\x00\x00\x00\x00\x00\x00\x00\x20\x00\x00\x00"
                        "\x20\xff\xff\xff\xff\x00\x3c\x00\x3c\x00\x00\x00\x00"
                        "\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                        "\x00\x00\x00\x00\x00\x00\x00";
	char buffer[200];

	a = malloc(sizeof(struct pi_socket));
	memcpy(a, ps, sizeof(struct pi_socket));

	a->sd = accept(ps->sd, addr, addrlen);
	if (a->sd < 0)
		goto fail;

	pi_net_recv(a, buffer, 200, 0);
	pi_net_send(a, msg1, 50, 0);
	pi_net_recv(a, buffer, 200, 0);
	pi_net_send(a, msg2, 46, 0);
	pi_net_recv(a, buffer, 200, 0);

	pi_socket_recognize(a);

	a->connected = 1;

	return a->sd;
      fail:
	free(a);
	return -1;
}

/***********************************************************************
 *
 * Function:    pi_net_send
 *
 * Summary:     Send msg on a connected socket
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int
pi_net_send(struct pi_socket *ps, void *msg, int len, unsigned int flags)
{
	int n, l;
	unsigned char buf[6];

	buf[0] = 1;
	buf[1] = ps->xid;
	set_long(buf + 2, len);

	l = 0;
	while (l < 6) {
		n = write(ps->sd, buf + l, 6 - l);
		if (n > 0)
			l += n;
		if (n < 0)
			return n;
	}

	l = 0;
	while (l < len) {
		n = write(ps->sd, (char *) msg + l, len - l);
		if (n > 0)
			l += n;
		if (n < 0)
			return n;
	}

#ifndef NO_SERIAL_TRACE
	if (ps->debuglog) {
		buf[0] = 4;
		buf[1] = 0;
		set_long(buf + 2, len);
		write(ps->debugfd, buf, 6);
		write(ps->debugfd, msg, len);
	}
#endif

	return len;
}

/***********************************************************************
 *
 * Function:    pi_net_recv
 *
 * Summary:     Receive message on a connected socket
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int
pi_net_recv(struct pi_socket *ps, void *msg, int len, unsigned int flags)
{
	int n, l;
	int rlen;
	unsigned char buf[6];

	l = 0;
	while (l < 6) {
		n = read(ps->sd, buf + l, 6 - l);
		if (n > 0)
			l += n;
		if (n <= 0)
			return n;
	}

	rlen = get_long(buf + 2);

	if (len > rlen)
		len = rlen;

	l = 0;
	while (l < len) {
		n = read(ps->sd, (char *) msg + l, len - l);
		if (n > 0)
			l += n;
		if (n < 0)
			return n;
		if (n == 0) {
			len = l;
			break;
		}
	}

	if (l < rlen) {
		char discard;

		while (l < rlen) {
			n = read(ps->sd, &discard, 1);
			if (n > 0)
				l += n;
			if (n < 0)
				return n;
			if (n == 0)
				break;
		}
	}

	if (ps->initiator)
		ps->xid = buf[1];
	else {
		ps->xid++;
		if (ps->xid == 0xff)
			ps->xid = 1;
	}

#ifndef NO_SERIAL_TRACE
	if (ps->debuglog) {
		buf[0] = 3;
		buf[1] = 0;
		set_long(buf + 2, len);
		write(ps->debugfd, buf, 6);
		write(ps->debugfd, msg, len);
	}
#endif

	return len;
}

/***********************************************************************
 *
 * Function:    pi_net_tickle
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int pi_net_tickle(struct pi_socket *ps)
{
	return -1;
}

/***********************************************************************
 *
 * Function:    pi_net_close
 *
 * Summary:     Close a connection, destroy the socket
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int pi_net_close(struct pi_socket *ps)
{
	if (ps->type == PI_SOCK_STREAM) {
		if (ps->connected & 1)				/* If socket is connected 		*/
			if (!(ps->connected & 2))		/* And it wasn't end-of-synced 		*/
				dlp_EndOfSync(ps->sd, 0);	/* then end sync, with clean status 	*/
	}

	close(ps->sd);

#ifndef NO_SERIAL_TRACE
	if (ps->debugfd)
		close(ps->debugfd);
#endif

	return 0;
}
