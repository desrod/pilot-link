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
 *
 * -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#include <winsock.h>
#include <io.h>
#else
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>		/* Needed for Redhat 6.x machines */
#endif

#include "pi-debug.h"
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-serial.h"
#include "pi-net.h"
#include "pi-cmp.h"

#ifdef OS2
#include <sys/select.h>
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
static int pi_serial_close(struct pi_socket *ps);

int pi_socket_init(struct pi_socket *ps);

/* Protocol Functions */
static struct pi_protocol *pi_serial_protocol_dup (struct pi_protocol *prot)
{
	struct pi_protocol *new_prot;
	
	new_prot 		= (struct pi_protocol *)malloc (sizeof (struct pi_protocol));
	new_prot->level 	= prot->level;
	new_prot->dup 		= prot->dup;
	new_prot->free 		= prot->free;
	new_prot->read 		= prot->read;
	new_prot->write 	= prot->write;
	new_prot->getsockopt 	= prot->getsockopt;
	new_prot->setsockopt 	= prot->setsockopt;
	new_prot->data 		= NULL;

	return new_prot;
}

static void pi_serial_protocol_free (struct pi_protocol *prot)
{
	free(prot);
}

static struct pi_protocol *pi_serial_protocol (struct pi_device *dev)
{	
	struct pi_protocol *prot;
	struct pi_serial_data *data;
	
	data = dev->data;
	
	prot 			= (struct pi_protocol *)malloc (sizeof (struct pi_protocol));
	prot->level 		= PI_LEVEL_DEV;
	prot->dup 		= pi_serial_protocol_dup;
	prot->free 		= pi_serial_protocol_free;
	prot->read 		= data->impl.read;
	prot->write 		= data->impl.write;
	prot->getsockopt 	= pi_serial_getsockopt;
	prot->setsockopt 	= pi_serial_setsockopt;
	prot->data 		= NULL;
	
	return prot;
}

/* Device Functions */
static struct pi_device *pi_serial_device_dup (struct pi_device *dev)
{
	struct 	pi_device *new_dev;
	struct 	pi_serial_data *new_data, *data;
	
	new_dev 		= (struct pi_device *)malloc (sizeof (struct pi_device));
	new_dev->dup 		= dev->dup;
	new_dev->free 		= dev->free;
	new_dev->protocol 	= dev->protocol;	
	new_dev->bind 		= dev->bind;
	new_dev->listen 	= dev->listen;
	new_dev->accept 	= dev->accept;
	new_dev->connect 	= dev->connect;
	new_dev->close 		= dev->close;
	
	new_data 		= (struct pi_serial_data *)malloc (sizeof (struct pi_serial_data));
	data 			= (struct pi_serial_data *)dev->data;
	new_data->impl 		= data->impl;
	memcpy(new_data->buf, data->buf, data->buf_size);
	new_data->buf_size 	= data->buf_size;
	new_data->ref           = data->ref;
	(*(new_data->ref))++;
#ifndef WIN32
#ifndef OS2
	new_data->tco = data->tco;
#endif
#endif
	new_data->rate 		= data->rate;
	new_data->establishrate = data->establishrate;
	new_data->establishhighrate = data->establishhighrate;
	new_data->timeout 	= data->timeout;
	new_data->rx_bytes 	= 0;
	new_data->rx_errors 	= 0;
	new_data->tx_bytes 	= 0;
	new_data->tx_errors 	= 0;
	new_dev->data 		= new_data;
	
	return new_dev;
}

static void pi_serial_device_free (struct pi_device *dev) 
{
	struct pi_serial_data *data = (struct pi_serial_data *)dev->data;

	if (*(data->ref) == 0)
		free (data->ref);
	free(data);

	free(dev);
}

struct pi_device *pi_serial_device (int type) 
{
	struct 	pi_device *dev;
	struct 	pi_serial_data *data;
	
	dev 	= (struct pi_device *)malloc (sizeof (struct pi_device));
	data 	= (struct pi_serial_data *)malloc (sizeof (struct pi_serial_data));

	dev->dup 		= pi_serial_device_dup;
	dev->free 		= pi_serial_device_free;
	dev->protocol 		= pi_serial_protocol;	
	dev->bind 		= pi_serial_bind;
	dev->listen 		= pi_serial_listen;
	dev->accept 		= pi_serial_accept;
	dev->connect 		= pi_serial_connect;
	dev->close 		= pi_serial_close;

	switch (type) {
	case PI_SERIAL_DEV:
		pi_serial_impl_init (&data->impl);
		break;
	default:
		pi_serial_impl_init (&data->impl);
		break;
	}
	
	data->buf_size 		= 0;
	data->ref               = (int *)malloc (sizeof (int));
	*(data->ref)            = 1;
	data->rate 		= -1;
	data->establishrate 	= -1;
	data->establishhighrate = -1;
	data->timeout 		= 0;
	data->rx_bytes 		= 0;
	data->rx_errors 	= 0;
	data->tx_bytes 		= 0;
	data->tx_errors 	= 0;
	dev->data 		= data;
	
	return dev;
}

/***********************************************************************
 *
 * Function:    pi_serial_connect
 *
 * Summary:     Connect socket to a given address
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
static int
pi_serial_connect(struct pi_socket *ps, struct sockaddr *addr, int addrlen)
{
	char 	*rate_env;
	struct 	pi_serial_data *data = (struct pi_serial_data *)ps->device->data;
	struct 	pi_sockaddr *pa = (struct pi_sockaddr *) addr;

	if (ps->type == PI_SOCK_STREAM) {
		if (ps->protocol == PI_PF_SYS) {
			data->establishrate = data->rate = 57600;
		} else {
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
		}
	} else if (ps->type == PI_SOCK_RAW) {
		data->establishrate = data->rate = 57600;
	}

	if (data->impl.open(ps, pa, addrlen) == -1)
		return -1;	/* errno already set */

	ps->raddr 	= malloc(addrlen);
	memcpy(ps->raddr, addr, addrlen);
	ps->raddrlen 	= addrlen;
	ps->laddr 	= malloc(addrlen);
	memcpy(ps->laddr, addr, addrlen);
	ps->laddrlen 	= addrlen;

	if (ps->type == PI_SOCK_STREAM) {
		int 	size;
		
		switch (ps->cmd) {
		case PI_CMD_CMP:
			if (cmp_tx_handshake(ps) < 0)
				goto fail;
			
			size = sizeof(data->rate);
			pi_getsockopt(ps->sd, PI_LEVEL_CMP, PI_CMP_BAUD,
				      &data->rate, &size);

			if (data->impl.changebaud(ps) < 0)
				goto fail;

			break;
			
		case PI_CMD_NET:
			if (data->impl.changebaud(ps) < 0)
				goto fail;
			break;

		case PI_CMD_SYS:
			if (data->impl.changebaud(ps) < 0)
				goto fail;
			break;
		}
	}
	ps->state = PI_SOCK_CONIN;
	ps->command = 0;

	return 0;

 fail:
	pi_close (ps->sd);
	return -1;
}

/***********************************************************************
 *
 * Function:    pi_serial_bind
 *
 * Summary:     Bind address to a local socket
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
static int
pi_serial_bind(struct pi_socket *ps, struct sockaddr *addr, int addrlen)
{
	char 	*rate_env;
	struct 	pi_serial_data *data = (struct pi_serial_data *)ps->device->data;
	struct 	pi_sockaddr *pa = (struct pi_sockaddr *) addr;

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

	ps->raddr 	= malloc(addrlen);
	memcpy(ps->raddr, addr, addrlen);
	ps->raddrlen 	= addrlen;
	ps->laddr 	= malloc(addrlen);
	memcpy(ps->laddr, addr, addrlen);
	ps->laddrlen 	= addrlen;

	return 0;
}

/***********************************************************************
 *
 * Function:    pi_serial_listen
 *
 * Summary:     Prepare for incoming connections
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int pi_serial_listen(struct pi_socket *ps, int backlog)
{
	int 	result;
	struct 	pi_serial_data *data = (struct pi_serial_data *)ps->device->data;
	
	result = data->impl.changebaud(ps);	/* ps->rate has been set by bind */
	if (result == 0)
		ps->state = PI_SOCK_LISTN;
	
	return result;
}

/***********************************************************************
 *
 * Function:    pi_serial_accept
 *
 * Summary:     Accept an incoming connection
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int
pi_serial_accept(struct pi_socket *ps, struct sockaddr *addr, int *addrlen)
{
	struct 	pi_serial_data *data = (struct pi_serial_data *)ps->device->data;
	int 	size;
	struct 	pi_socket *accept = NULL;

	/* Wait for data */
	if (data->impl.poll(ps, ps->accept_to) < 0)
		goto fail;
	
	accept = pi_socket_copy(ps);
	data = accept->device->data;
	data->timeout = accept->accept_to * 1000;

	pi_socket_init(accept);
	if (ps->type == PI_SOCK_STREAM) {
		struct timeval tv;

		switch (accept->cmd) {
		case PI_CMD_CMP:
			if (cmp_rx_handshake(accept, data->establishrate, data->establishhighrate) < 0)
				return -1;

			size = sizeof(data->rate);
			pi_getsockopt(accept->sd, PI_LEVEL_CMP, PI_CMP_BAUD,
				      &data->rate, &size);
			
			/* We always reconfigure our port, no matter what */
			if (data->impl.changebaud(accept) < 0)
				goto fail;
			
			/* Palm device needs some time to reconfigure its port */
#ifdef WIN32
			Sleep(100);
#else
			tv.tv_sec 	= 0;
			tv.tv_usec 	= 50000;
			select(0, 0, 0, 0, &tv);
#endif

			break;
		case PI_CMD_NET:
			if (net_rx_handshake(accept) < 0)
				return -1;
			break;
		}

		accept->dlprecord = 0;
	}

	data->timeout = 0;
	accept->command = 0;
	accept->state 	= PI_SOCK_CONAC;

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
	case PI_DEV_RATE:
		if (*option_len < sizeof (data->rate))
			goto error;
		memcpy (option_value, &data->rate, sizeof (data->rate));
		*option_len = sizeof (data->rate);
		break;
	case PI_DEV_ESTRATE:
		if (*option_len < sizeof (data->establishrate))
			goto error;
		memcpy (option_value, &data->establishrate, 
			sizeof (data->establishrate));
		*option_len = sizeof (data->establishrate);
		break;
	case PI_DEV_HIGHRATE:
		if (*option_len < sizeof (data->establishhighrate))
			goto error;
		memcpy (option_value, &data->establishhighrate,
			sizeof (data->establishhighrate));
		*option_len = sizeof (data->establishhighrate);
		break;
	case PI_DEV_TIMEOUT:
		if (*option_len < sizeof (data->timeout))
			goto error;
		memcpy (option_value, &data->timeout,
			sizeof (data->timeout));
		*option_len = sizeof (data->timeout);
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
	case PI_DEV_ESTRATE:
		if (*option_len != sizeof (data->establishrate))
			goto error;
		memcpy (&data->establishrate, option_value,
			sizeof (data->establishrate));
		break;
	case PI_DEV_HIGHRATE:
		if (*option_len != sizeof (data->establishhighrate))
			goto error;
		memcpy (&data->establishhighrate, option_value,
			sizeof (data->establishhighrate));
		break;
	case PI_DEV_TIMEOUT:
		if (*option_len != sizeof (data->timeout))
			goto error;
		memcpy (&data->timeout, option_value,
			sizeof (data->timeout));
		break;
	}

	return 0;
	
 error:
	errno = EINVAL;
	return -1;
}


/***********************************************************************
 *
 * Function:    pi_serial_close
 *
 * Summary:     Close a connection, destroy the socket
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int pi_serial_close(struct pi_socket *ps)
{
	struct pi_serial_data *data = (struct pi_serial_data *)ps->device->data;

	(*(data->ref))--;

	if (ps->sd)
		data->impl.close (ps);

	if (ps->laddr)
		free(ps->laddr);
	if (ps->raddr)
		free(ps->raddr);

	return 0;
}
