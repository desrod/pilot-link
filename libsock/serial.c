/*
 * serial.c: Interface layer to serial HotSync connections
 *
 * Copyright (c) 1996, 1997, D. Jeff Dionne & Kenneth Albanowski
 * Copyright (c) 1999, Tilo Christ
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
#include <windows.h>
#include <winsock.h>
#include <io.h>
#else
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-serial.h"
#include "pi-inetserial.h"
#include "pi-padp.h"
#include "pi-cmp.h"
#include "pi-dlp.h"
#include "pi-syspkt.h"

#ifdef OS2
#include <sys/select.h>
#endif

#ifdef WIN32
extern int win_peek(struct pi_socket *ps, int timeout);
#endif

static int pi_serial_listen(struct pi_socket *ps, int backlog);
static int pi_serial_accept(struct pi_socket *ps, struct sockaddr *addr,
			    int *addrlen);
static int pi_serial_send(struct pi_socket *ps, void *msg, int len,
			  unsigned int flags);
static int pi_serial_recv(struct pi_socket *ps, void *msg, int len,
			  unsigned int flags);
static int pi_serial_tickle(struct pi_socket *ps);
static int pi_serial_close(struct pi_socket *ps);

extern int dlp_trace;

/***********************************************************************
 *
 * Function:    pi_serial_connect
 *
 * Summary:     Connect socket to a given address
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
pi_serial_connect(struct pi_socket *ps, struct sockaddr *addr, int addrlen)
{
	struct cmp c;
	struct pi_sockaddr *pa = (struct pi_sockaddr *) addr;
	char *rate_env;

	if (ps->type == PI_SOCK_STREAM) {
		if (ps->establishrate == -1) {
			ps->establishrate = 9600;	/* Default PADP connection rate */
			rate_env = getenv("PILOTRATE");
			if (rate_env) {
				if (rate_env[0] == 'H') {	/* Establish high rate */
					ps->establishrate =
					    atoi(rate_env + 1);
					ps->establishhighrate = -1;
				} else {
					ps->establishrate = atoi(rate_env);
					ps->establishhighrate = 0;
				}
			}
		}
		ps->rate = 9600;	/* Mandatory CMP conncetion rate */
	} else if (ps->type == PI_SOCK_RAW) {
		ps->establishrate = ps->rate = 57600;	/* Mandatory SysPkt connection rate */
	}

	if ((addr->sa_family == PI_AF_INETSLP)
	    || ((addr->sa_family == PI_AF_SLP)
		&& (pa->pi_device[0] == ':'))) {
#ifdef _PILOT_INETSERIAL_H_
		if (pi_inetserial_open(ps, addr, addrlen) == -1) {
			return -1;	/* errno already set */
		}
#else
		errno = EINVAL;
		return -1;
#endif
	} else {
		if (pi_serial_open(ps, pa, addrlen) == -1) {
			return -1;	/* errno already set */
		}
	}

	ps->raddr = malloc(addrlen);
	memcpy(ps->raddr, addr, addrlen);
	ps->raddrlen = addrlen;
	ps->laddr = malloc(addrlen);
	memcpy(ps->laddr, addr, addrlen);
	ps->laddrlen = addrlen;

	if (ps->type == PI_SOCK_STREAM) {

		if (cmp_wakeup(ps, 38400) < 0)	/* Assume this box can't go over 38400 */
			return -1;

		if (cmp_rx(ps, &c) < 0)
			return -1;	/* failed to read, errno already set */

		if (c.type == 2) {
			/* CMP init packet */

			if (c.flags & 0x80) {
				/* Change baud rate */
				ps->rate = c.baudrate;
				if (ps->serial_changebaud(ps) < 0)
					return -1;

			}

		} else if (c.type == 3) {
			/* CMP abort packet -- the other side didn't like us */
			ps->serial_close(ps);

#ifdef DEBUG
			fprintf(stderr,
				"Received CMP abort from client\n");
#endif
			errno = -EIO;
			return -1;
		}
	}
	ps->connected = 1;

	ps->initiator = 1;	/* We initiated the link */

	ps->socket_listen 	= pi_serial_listen;
	ps->socket_accept 	= pi_serial_accept;
	ps->socket_close 	= pi_serial_close;
	ps->socket_send 	= pi_serial_send;
	ps->socket_recv 	= pi_serial_recv;
	ps->socket_tickle 	= pi_serial_tickle;

	return 0;
}

/***********************************************************************
 *
 * Function:    pi_serial_bind
 *
 * Summary:     Bind address to a local socket
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
pi_serial_bind(struct pi_socket *ps, struct sockaddr *addr, int addrlen)
{
	struct pi_sockaddr *pa = (struct pi_sockaddr *) addr;
	char *rate_env;

	if (ps->type == PI_SOCK_STREAM) {
		if (ps->establishrate == -1) {
			ps->establishrate = 9600;	/* Default PADP connection rate */
			rate_env = getenv("PILOTRATE");
			if (rate_env) {
				if (rate_env[0] == 'H') {	/* Establish high rate */
					ps->establishrate =
					    atoi(rate_env + 1);
					ps->establishhighrate = -1;
				} else {
					ps->establishrate = atoi(rate_env);
					ps->establishhighrate = 0;
				}
			}
		}
		ps->rate = 9600;	/* Mandatory CMP connection rate */
	} else if (ps->type == PI_SOCK_RAW) {
		ps->establishrate = ps->rate = 57600;	/* Mandatory SysPkt connection rate */
	}

	if ((addr->sa_family == PI_AF_INETSLP)
	    || ((addr->sa_family == PI_AF_SLP)
		&& (pa->pi_device[0] == ':'))) {
#ifdef _PILOT_INETSERIAL_H_
		if (pi_inetserial_open(ps, addr, addrlen) == -1) {
			return -1;	/* errno already set */
		}
#else
		errno = EINVAL;
		return -1;
#endif
	} else {
		if (pi_serial_open(ps, pa, addrlen) == -1) {
			return -1;	/* errno already set */
		}
	}

	ps->raddr = malloc(addrlen);
	memcpy(ps->raddr, addr, addrlen);
	ps->raddrlen = addrlen;
	ps->laddr = malloc(addrlen);
	memcpy(ps->laddr, addr, addrlen);
	ps->laddrlen = addrlen;

	ps->socket_listen = pi_serial_listen;
	ps->socket_accept = pi_serial_accept;
	ps->socket_close = pi_serial_close;
	ps->socket_send = pi_serial_send;
	ps->socket_recv = pi_serial_recv;
	ps->socket_tickle = pi_serial_tickle;

	return 0;
}

/***********************************************************************
 *
 * Function:    pi_serial_listen
 *
 * Summary:     Prepare for incoming connections
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int pi_serial_listen(struct pi_socket *ps, int backlog)
{
	return ps->serial_changebaud(ps);	/* ps->rate has been set by bind */
}

/***********************************************************************
 *
 * Function:    pi_serial_accept
 *
 * Summary:     Accept an incoming connection
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int
pi_serial_accept(struct pi_socket *ps, struct sockaddr *addr, int *addrlen)
{
	struct pi_socket *accept;
	struct timeval tv;
	struct cmp c;

	accept = malloc(sizeof(struct pi_socket));
	memcpy(accept, ps, sizeof(struct pi_socket));

	if (accept->type == PI_SOCK_STREAM) {
#ifdef WIN32
		int rc;

		rc = win_peek(ps, accept->accept_to);	/* Wait for data on the CommPort. */
		if (rc < 0) {
			errno = ETIMEDOUT;
			goto fail;
		}
#else
		accept->serial_read(accept, accept->accept_to);
#endif

		if (accept->rx_errors > 0) {
			errno = ETIMEDOUT;
			goto fail;
		}

		if (cmp_rx(accept, &c) < 0)
			goto fail;	/* Failed to establish connection, errno already set */

		if ((c.version & 0xFF00) == 0x0100) {
			if ((unsigned long) accept->establishrate >
			    c.baudrate) {
				if (!accept->establishhighrate) {
					fprintf(stderr,
						"Rate %d too high, dropping to %ld\n",
						ps->establishrate,
						c.baudrate);
					accept->establishrate = c.baudrate;
				}
			}

			accept->rate = accept->establishrate;
			accept->version = c.version;
			if (cmp_init(accept, accept->rate) < 0)
				goto fail;
			pi_serial_flush(accept);

			/* We always reconfigure our port, no matter what */
			if (accept->serial_changebaud(accept) < 0)
				goto fail;

			/* Palm device needs some time to reconfigure its port */
#ifdef WIN32
			Sleep(100);
#else
			tv.tv_sec = 0;
			tv.tv_usec = 50000;
			select(0, 0, 0, 0, &tv);
#endif

			accept->connected = 1;
			accept->accepted = 1;
			accept->dlprecord = 0;
		} else {
			cmp_abort(ps, 0x80);	/* 0x80 means the comm version wasn't compatible */

			fprintf(stderr,
				"pi_socket connection failed due to comm version mismatch\n");
			fprintf(stderr,
				" (expected version 01xx, got %4.4X)\n",
				c.version);

			errno = ECONNREFUSED;
			goto fail;
		}
	} else {
		accept->connected = 1;
		accept->accepted = 1;
	}

	accept->sd = dup(ps->sd);

	pi_socket_recognize(accept);

	accept->laddr = malloc(ps->laddrlen);
	accept->raddr = malloc(ps->raddrlen);
	memcpy(accept->laddr, ps->laddr, ps->laddrlen);
	memcpy(accept->raddr, ps->raddr, ps->raddrlen);

	accept->mac->ref++;	/* Keep mac around even if the bound socket
				   is closed */
	accept->initiator = 0;	/* We accepted the link, we did not initiate
				   it */

	return accept->sd;
      fail:
	free(accept);
	return -1;
}

/***********************************************************************
 *
 * Function:    pi_serial_send
 *
 * Summary:     Send message on a connected socket
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int
pi_serial_send(struct pi_socket *ps, void *msg, int len,
	       unsigned int flags)
{
	if (ps->type == PI_SOCK_STREAM)
		return padp_tx(ps, msg, len, padData);
	else
#ifdef _PILOT_SYSPKT_H
		return syspkt_tx(ps, msg, len);
#else
		return -1;
#endif
}

/***********************************************************************
 *
 * Function:    pi_serial_recv
 *
 * Summary:     Receive message on a connected socket
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int
pi_serial_recv(struct pi_socket *ps, void *msg, int len,
	       unsigned int flags)
{
	if (ps->type == PI_SOCK_STREAM)
		return padp_rx(ps, msg, len);
	else
#ifdef _PILOT_SYSPKT_H
		return syspkt_rx(ps, msg, len);
#else
		return -1;
#endif
}

/***********************************************************************
 *
 * Function:    pi_serial_tickle
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int pi_serial_tickle(struct pi_socket *ps)
{
	if (ps->type == PI_SOCK_STREAM) {
		struct padp pd;
		int ret;

		if (ps->busy || !ps->connected)
			return -1;
		pd.type = padTickle;
		pd.flags = 0x00;
		pd.size = 0x00;
		ret = padp_tx(ps, (void *) &pd, 0, padTickle);
		pi_serial_flush(ps);
		return ret;
	} else {
		errno = EOPNOTSUPP;
		return -1;
	}
}

/***********************************************************************
 *
 * Function:    pi_serial_close
 *
 * Summary:     Close a connection, destroy the socket
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int pi_serial_close(struct pi_socket *ps)
{
#ifdef DEBUG
	fprintf(stderr, "pi_serial_close\n");
	fprintf(stderr, "connected: %d\n", ps->connected);
#endif
	if (ps->type == PI_SOCK_STREAM) {
		if (!(ps->broken))	/* If connection is not broken */
			if (ps->connected & 1)	/* And the socket is connected */
				if (!(ps->connected & 2))	/* And it wasn't end-of-synced */
					dlp_EndOfSync(ps->sd, 0);	/* then end sync, with clean status */
	}

	if (ps->sd && (ps->sd != ps->mac->fd))	/* If device still has a /dev/null handle */
		close(ps->sd);	/* Close /dev/null handle */

	if (ps->mac->fd) {	/* If device was opened */
		if (ps->connected) {
			if (!(ps->broken))
				pi_serial_flush(ps);	/* Flush the device, and set baud rate back to the initial setting */
#ifdef notdef
			ps->rate = 9600;
			ps->serial_changebaud(ps);
#endif
		}

		ps->connected = 0;
		ps->accepted = 0;
		ps->broken = -1;	/* Ban any future PADP traffic */

		if (--(ps->mac->ref) == 0) {	/* If no-one is using the device, close it */
			ps->serial_close(ps);
			free(ps->mac);
		}
	}

	if (ps->laddr)
		free(ps->laddr);
	if (ps->raddr)
		free(ps->raddr);

	return 0;
}

/***********************************************************************
 *
 * Function:    pi_serial_flush
 *
 * Summary:     Flush the socket of all data
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int pi_serial_flush(struct pi_socket *ps)
{
	while (ps->serial_write(ps));
	return 0;
}
