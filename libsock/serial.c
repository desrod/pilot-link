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

static int pi_serial_connect(struct pi_socket *ps, struct sockaddr *addr, 
			     int addrlen);
static int pi_serial_bind(struct pi_socket *ps, struct sockaddr *addr,
			  int addrlen);
static int pi_serial_listen(struct pi_socket *ps, int backlog);
static int pi_serial_accept(struct pi_socket *ps, struct sockaddr *addr,
			    int *addrlen);
static int pi_serial_getsockopt(struct pi_socket *ps, int level, int option_name, 
				void *option_value, int *option_len);
static int pi_serial_setsockopt(struct pi_socket *ps, int level, int option_name, 
				const void *option_value, int *option_len);

static int pi_serial_tickle(struct pi_socket *ps);
static int pi_serial_close(struct pi_socket *ps);

static struct pi_protocol *pi_serial_protocol (struct pi_device *dev);

extern int dlp_trace;

static struct pi_protocol *pi_serial_protocol_dup (struct pi_protocol *prot)
{
	struct pi_protocol *new_prot;
	
	new_prot = (struct pi_protocol *)malloc (sizeof (struct pi_protocol));
	new_prot->level = prot->level;
	new_prot->dup = prot->dup;
	new_prot->read = prot->read;
	new_prot->write = prot->write;
	new_prot->getsockopt = prot->getsockopt;
	new_prot->setsockopt = prot->setsockopt;
	new_prot->data = NULL;

	return new_prot;
}

static struct pi_protocol *pi_serial_protocol (struct pi_device *dev)
{	
	struct pi_protocol *prot;
	struct pi_serial_data *data;
	
	data = dev->data;
	
	prot = (struct pi_protocol *)malloc (sizeof (struct pi_protocol));
	prot->level = PI_LEVEL_SOCKET;
	prot->dup = pi_serial_protocol_dup;
	prot->read = data->impl.read;
	prot->write = data->impl.write;
	prot->getsockopt = pi_serial_getsockopt;
	prot->setsockopt = pi_serial_setsockopt;
	prot->data = NULL;
	
	return prot;
}


static struct pi_device *pi_serial_device_dup (struct pi_device *dev)
{
	struct pi_device *new_dev;
	struct pi_serial_data *new_data, *data;
	
	new_dev = (struct pi_device *)malloc (sizeof (struct pi_device));
	new_dev->dup = dev->dup;
	new_dev->protocol = dev->protocol;	
	new_dev->bind = dev->bind;
	new_dev->listen = dev->listen;
	new_dev->accept = dev->accept;
	new_dev->connect = dev->connect;
	new_dev->close = dev->close;
	
	new_data = (struct pi_serial_data *)malloc (sizeof (struct pi_serial_data));
	data = (struct pi_serial_data *)dev->data;
	new_data->impl = data->impl;
	new_data->fd = dup(data->fd);
	printf ("New FD %d\n", new_data->fd);
	new_data->rate = data->rate;
	new_data->establishrate = data->establishrate;
	new_data->establishhighrate = data->establishhighrate;
	new_data->timeout = data->timeout;
	new_data->rx_bytes = 0;
	new_data->rx_errors = 0;
	new_data->tx_bytes = 0;
	new_data->tx_errors = 0;
	new_dev->data = new_data;
	
	return new_dev;
}

struct pi_device *pi_serial_device (void) 
{
	struct pi_device *dev;
	struct pi_serial_data *data;
	
	dev = (struct pi_device *)malloc (sizeof (struct pi_device));
	data = (struct pi_serial_data *)malloc (sizeof (struct pi_serial_data));

	dev->dup = pi_serial_device_dup;
	dev->protocol = pi_serial_protocol;	
	dev->bind = pi_serial_bind;
	dev->listen = pi_serial_listen;
	dev->accept = pi_serial_accept;
	dev->connect = pi_serial_connect;
	dev->close = pi_serial_close;

	pi_serial_impl_init (&data->impl);
	data->fd = 0;
	data->rate = -1;
	data->establishrate = -1;
	data->establishhighrate = -1;
	data->timeout = 0;
	data->rx_bytes = 0;
	data->rx_errors = 0;
	data->tx_bytes = 0;
	data->tx_errors = 0;
	dev->data = data;
	
	return dev;
}

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
static int
pi_serial_connect(struct pi_socket *ps, struct sockaddr *addr, int addrlen)
{
	struct pi_serial_data *data = (struct pi_serial_data *)ps->device->data;
	struct cmp c;
	struct pi_sockaddr *pa = (struct pi_sockaddr *) addr;
	char *rate_env;

	if (ps->type == PI_SOCK_STREAM) {
		if (data->establishrate == -1) {
			data->establishrate = 9600;	/* Default PADP connection rate */
			rate_env = getenv("PILOTRATE");
			if (rate_env) {
				if (rate_env[0] == 'H') {	/* Establish high rate */
					data->establishrate =
					    atoi(rate_env + 1);
					data->establishhighrate = -1;
				} else {
					data->establishrate = atoi(rate_env);
					data->establishhighrate = 0;
				}
			}
		}
		data->rate = 9600;	/* Mandatory CMP conncetion rate */
	} else if (ps->type == PI_SOCK_RAW) {
		data->establishrate = data->rate = 57600;	/* Mandatory SysPkt connection rate */
	}

	if (data->impl.open(ps, pa, addrlen) == -1) {
		return -1;	/* errno already set */
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
				data->rate = c.baudrate;
				if (data->impl.changebaud(ps) < 0)
					return -1;

			}

		} else if (c.type == 3) {
			/* CMP abort packet -- the other side didn't like us */
			data->impl.close(ps);

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
static int
pi_serial_bind(struct pi_socket *ps, struct sockaddr *addr, int addrlen)
{
	struct pi_serial_data *data = (struct pi_serial_data *)ps->device->data;
	struct pi_sockaddr *pa = (struct pi_sockaddr *) addr;
	char *rate_env;

	if (ps->type == PI_SOCK_STREAM) {
		if (data->establishrate == -1) {
			data->establishrate = 9600;	/* Default PADP connection rate */
			rate_env = getenv("PILOTRATE");
			if (rate_env) {
				if (rate_env[0] == 'H') {	/* Establish high rate */
					data->establishrate =
					    atoi(rate_env + 1);
					data->establishhighrate = -1;
				} else {
					data->establishrate = atoi(rate_env);
					data->establishhighrate = 0;
				}
			}
		}
		data->rate = 9600;	/* Mandatory CMP connection rate */
	} else if (ps->type == PI_SOCK_RAW) {
		data->establishrate = data->rate = 57600;	/* Mandatory SysPkt connection rate */
	}

	if (data->impl.open(ps, pa, addrlen) == -1) {
		return -1;	/* errno already set */
	}

	ps->raddr = malloc(addrlen);
	memcpy(ps->raddr, addr, addrlen);
	ps->raddrlen = addrlen;
	ps->laddr = malloc(addrlen);
	memcpy(ps->laddr, addr, addrlen);
	ps->laddrlen = addrlen;

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
	struct pi_serial_data *data = (struct pi_serial_data *)ps->device->data;

	return data->impl.changebaud(ps);	/* ps->rate has been set by bind */
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
	struct pi_serial_data *data = (struct pi_serial_data *)ps->device->data;
	struct pi_socket *accept = NULL;

	if (ps->type == PI_SOCK_STREAM) {
		struct timeval tv;
		struct cmp c;

		/* Wait for data */
		if (data->impl.poll(ps, 0) < 0) {
			errno = ETIMEDOUT;
			goto fail;
		}

		accept = pi_socket_copy(ps);

		/* Check for a proper cmp connection */
		if (cmp_rx(accept, &c) < 0)
			goto fail;	/* Failed to establish connection, errno already set */

		if ((c.version & 0xFF00) == 0x0100) {
			if ((unsigned long) data->establishrate >
			    c.baudrate) {
				if (!data->establishhighrate) {
					fprintf(stderr,
						"Rate %d too high, dropping to %ld\n",
						data->establishrate,
						c.baudrate);
					data->establishrate = c.baudrate;
				}
			}

			data->rate = data->establishrate;
			accept->version = c.version;

			if (cmp_init(accept, data->rate) < 0)
				goto fail;

			/* We always reconfigure our port, no matter what */
			if (data->impl.changebaud(accept) < 0)
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
		accept = pi_socket_copy(ps);

		accept->connected = 1;
		accept->accepted = 1;
	}

	accept->initiator = 0;	/* We accepted the link, we did not initiate
				   it */

	return accept->sd;

 fail:
	if (accept)
		pi_close (accept->sd);
	return -1;
}

static int
pi_serial_getsockopt(struct pi_socket *ps, int level, int option_name, 
		     void *option_value, int *option_len)
{
	struct pi_serial_data *data = (struct pi_serial_data *)ps->device->data;

	switch (option_name) {
	case PI_SOCKET_RATE:
		if (*option_len < sizeof (data->rate))
			goto error;
		memcpy (option_value, &data->rate, sizeof (data->rate));
		*option_len = sizeof (data->rate);
		break;
	case PI_SOCKET_ESTRATE:
		if (*option_len < sizeof (data->establishrate))
			goto error;
		memcpy (option_value, &data->establishrate, 
			sizeof (data->establishrate));
		*option_len = sizeof (data->establishrate);
		break;
	case PI_SOCKET_HIGHRATE:
		if (*option_len < sizeof (data->establishhighrate))
			goto error;
		memcpy (option_value, &data->establishhighrate,
			sizeof (data->establishhighrate));
		*option_len = sizeof (data->establishhighrate);
		break;
	}

	return 0;
	
 error:
	errno = EINVAL;
	return -1;
}

static int
pi_serial_setsockopt(struct pi_socket *ps, int level, int option_name, 
		     const void *option_value, int *option_len)
{
	struct pi_serial_data *data = (struct pi_serial_data *)ps->device->data;

	/* FIXME: can't change stuff if already connected */

	switch (option_name) {
	case PI_SOCKET_ESTRATE:
		if (*option_len != sizeof (data->establishrate))
			goto error;
		memcpy (&data->establishrate, option_value,
			sizeof (data->establishrate));
		break;
	case PI_SOCKET_HIGHRATE:
		if (*option_len != sizeof (data->establishhighrate))
			goto error;
		memcpy (&data->establishhighrate, option_value,
			sizeof (data->establishhighrate));
		break;
	}

	return 0;
	
 error:
	errno = EINVAL;
	return -1;
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
		int type, size;

		if (!ps->connected)
			return -1;
		pd.type = padTickle;
		pd.flags = 0x00;
		pd.size = 0x00;

		type = padTickle;
		size = sizeof(type);
		pi_setsockopt(ps->sd, PI_LEVEL_PADP, PI_PADP_TYPE, 
			      &type, &size);

		return padp_tx(ps, (void *) &pd, 0);
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
	struct pi_serial_data *data = (struct pi_serial_data *)ps->device->data;

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

	/* If device still has a /dev/null handle */
	/* Close /dev/null handle */
	if (ps->sd && (ps->sd != data->fd))
		close(ps->sd);

	/* If device was opened */
	if (data->fd)
		data->impl.close(ps);

	if (ps->laddr)
		free(ps->laddr);
	if (ps->raddr)
		free(ps->raddr);

	return 0;
}
