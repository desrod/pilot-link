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
#include <sys/socket.h>
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

/* Declare prototypes */
static int pi_serial_connect(pi_socket_t *ps, struct sockaddr *addr, 
			socklen_t addrlen);
static int pi_serial_bind(pi_socket_t *ps, struct sockaddr *addr,
			socklen_t addrlen);
static int pi_serial_listen(pi_socket_t *ps, int backlog);
static int pi_serial_accept(pi_socket_t *ps, struct sockaddr *addr,
			socklen_t *addrlen);
static int pi_serial_getsockopt(pi_socket_t *ps, int level,
			int option_name, void *option_value,
			size_t *option_len);
static int pi_serial_setsockopt(pi_socket_t *ps, int level,
			int option_name, const void *option_value,
			size_t *option_len);
static int pi_serial_close(pi_socket_t *ps);

int pi_socket_init(pi_socket_t *ps);


/* Protocol Functions */
/***********************************************************************
 *
 * Function:    pi_serial_protocol_dup
 *
 * Summary:     clones an existing pi_protocol struct
 *
 * Parameters:  pi_protocol*
 *
 * Returns:     pi_protocol_t* or NULL if operation failed
 *
 ***********************************************************************/
static pi_protocol_t*
pi_serial_protocol_dup (pi_protocol_t *prot)
{
	pi_protocol_t *new_prot;

	ASSERT (prot != NULL);
	
	new_prot = (pi_protocol_t *) malloc(sizeof (pi_protocol_t));

	if (new_prot != NULL) {
		new_prot->level 	= prot->level;
		new_prot->dup 		= prot->dup;
		new_prot->free 		= prot->free;
		new_prot->read 		= prot->read;
		new_prot->write 	= prot->write;
		new_prot->getsockopt 	= prot->getsockopt;
		new_prot->setsockopt 	= prot->setsockopt;
		new_prot->data 		= NULL;
	}

	return new_prot;
}


/***********************************************************************
 *
 * Function:    pi_serial_protocol_free
 *
 * Summary:     frees an existing pi_protocol struct
 *
 * Parameters:  pi_protocol*
 *
 * Returns:     void
 *
 ***********************************************************************/
static void
pi_serial_protocol_free (pi_protocol_t *prot)
{
	ASSERT (prot != NULL);

	if (prot != NULL)
		free(prot);
}


/***********************************************************************
 *
 * Function:    pi_serial_protocol
 *
 * Summary:     creates and inits pi_protocol struct instance
 *
 * Parameters:  pi_device_t*
 *
 * Returns:     pi_protocol_t* or NULL if operation failed
 *
 ***********************************************************************/
static pi_protocol_t*
pi_serial_protocol (pi_device_t *dev)
{	
	pi_protocol_t *prot;
	struct pi_serial_data *data;

	ASSERT (dev != NULL);
	
	data = dev->data;
	
	prot = (pi_protocol_t *) malloc(sizeof (pi_protocol_t));

	if (prot != NULL) {
		prot->level 		= PI_LEVEL_DEV;
		prot->dup 		= pi_serial_protocol_dup;
		prot->free 		= pi_serial_protocol_free;
		prot->read 		= data->impl.read;
		prot->write 		= data->impl.write;
		prot->getsockopt 	= pi_serial_getsockopt;
		prot->setsockopt 	= pi_serial_setsockopt;
		prot->data 		= NULL;
	}
	
	return prot;
}


/* Device Functions */
/***********************************************************************
 *
 * Function:    pi_serial_device_dup
 *
 * Summary:     clones an existing pi_device struct
 *
 * Parameters:  pi_device_t*
 *
 * Returns:     pi_device_t* or NULL if operation failed
 *
 ***********************************************************************/
static pi_device_t*
pi_serial_device_dup (pi_device_t *dev)
{
	pi_device_t *new_dev = NULL;
	struct 	pi_serial_data	*new_data = NULL,
				*data = NULL;

	ASSERT (dev != NULL);
	
	new_dev	= (pi_device_t *) malloc(sizeof (pi_device_t));
	new_data = (struct pi_serial_data *)
				malloc(sizeof (struct pi_serial_data));

	if ((new_dev != NULL) && (new_data != NULL)) { 
		new_dev->dup 		= dev->dup;
		new_dev->free 		= dev->free;
		new_dev->protocol 	= dev->protocol;	
		new_dev->bind 		= dev->bind;
		new_dev->listen 	= dev->listen;
		new_dev->accept 	= dev->accept;
		new_dev->connect 	= dev->connect;
		new_dev->close 		= dev->close;
	
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

	} else if (new_dev != NULL) {
		free(new_dev);
		new_dev = NULL;

	} else if (new_data != NULL) {
		free(new_data);
		new_data = NULL;
	}
		
	return new_dev;
}


/***********************************************************************
 *
 * Function:    pi_serial_device_free
 *
 * Summary:     frees an existing pi_device struct
 *
 * Parameters:  pi_device_t*
 *
 * Returns:     void
 *
 ***********************************************************************/
static void
pi_serial_device_free (pi_device_t *dev) 
{
	struct pi_serial_data *data;

	ASSERT (dev != NULL);

	data = (struct pi_serial_data *)dev->data;

	(*(data->ref))--;

	if (*(data->ref) == 0)
		free (data->ref);
	free(data);

	free(dev);
}


/***********************************************************************
 *
 * Function:    pi_serial_device
 *
 * Summary:     creates and inits pi_device struct instance 
 *
 * Parameters:  device type
 *
 * Returns:     pi_device_t* or NULL if operation failed
 *
 ***********************************************************************/
pi_device_t*
pi_serial_device (int type) 
{
	pi_device_t *dev;
	struct 	pi_serial_data *data;
	
	dev = (pi_device_t *) malloc(sizeof (pi_device_t));
	data = (struct pi_serial_data *)
			malloc(sizeof (struct pi_serial_data));

	if ( (dev != NULL) && (data != NULL) ) {
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
		data->rate 		= (speed_t)-1;
		data->establishrate 	= (speed_t)-1;
		data->establishhighrate = -1;
		data->timeout 		= 0;
		data->rx_bytes 		= 0;
		data->rx_errors 	= 0;
		data->tx_bytes 		= 0;
		data->tx_errors 	= 0;

		dev->data 		= data;

	} else if (dev != NULL) {
		free(dev);
		dev = NULL;

	} else if (data != NULL) {
		free(data);
		data = NULL;
	}
	
	return dev;
}


/***********************************************************************
 *
 * Function:    pi_serial_connect
 *
 * Summary:     Connect socket to a given address
 *
 * Parameters:  pi_socket*, sockaddr*, socklen_t
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
static int
pi_serial_connect(pi_socket_t *ps, struct sockaddr *addr,
	socklen_t addrlen)
{
	char 	*rate_env;
	struct 	pi_serial_data *data =
		(struct pi_serial_data *)ps->device->data;
	struct 	pi_sockaddr *pa = (struct pi_sockaddr *) addr;

	if (ps->type == PI_SOCK_STREAM) {
		if (ps->protocol == PI_PF_SYS) {
			data->establishrate = data->rate = 57600;
		} else {
			if (data->establishrate == (speed_t) -1) {
				/* Default PADP connection rate */
				data->establishrate = 9600;
				rate_env = getenv("PILOTRATE");
				if (rate_env) {
					/* Establish high rate */
					if (rate_env[0] == 'H') {
						data->establishrate =
							atoi(rate_env + 1);
						data->establishhighrate = -1;
					} else {
						data->establishrate =
							atoi(rate_env);
						data->establishhighrate = 0;
					}
				}
			}
			/* Mandatory CMP connection rate */
			data->rate = 9600;
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
		size_t 	size;
		
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
 * Parameters:  pi_socket*, sockaddr*, socklen_t
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
static int
pi_serial_bind(pi_socket_t *ps, struct sockaddr *addr, socklen_t addrlen)
{
	char 	*rate_env;
	struct 	pi_serial_data *data =
			(struct pi_serial_data *)ps->device->data;
	struct 	pi_sockaddr *pa = (struct pi_sockaddr *) addr;

	if (ps->type == PI_SOCK_STREAM) {
		if (data->establishrate == (speed_t) -1) {
			/* Default PADP connection rate */
			data->establishrate = 9600;
			rate_env = getenv("PILOTRATE");
			if (rate_env) {
				/* Establish high rate */
				if (rate_env[0] == 'H') {
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
		/* Mandatory SysPkt connection rate */
		data->establishrate = data->rate = 57600;
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
 * Parameters:  pi_socket*, backlog
 *
 * Returns:     0 for success, -1 otherwise
 *
 ***********************************************************************/
static int pi_serial_listen(pi_socket_t *ps, int backlog)
{
	int 	result;
	struct 	pi_serial_data *data =
		(struct pi_serial_data *)ps->device->data;
	
	/* ps->rate has been set by bind */
	result = data->impl.changebaud(ps);
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
 * Parameters:  pi_socket*, sockaddr*
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int
pi_serial_accept(pi_socket_t *ps, struct sockaddr *addr,
	socklen_t *addrlen)
{
	struct 	pi_serial_data *data =
		(struct pi_serial_data *)ps->device->data;
	size_t 	size;
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
			if (cmp_rx_handshake(accept, data->establishrate,
				data->establishhighrate) < 0)
				return -1;

			size = sizeof(data->rate);
			pi_getsockopt(accept->sd, PI_LEVEL_CMP, PI_CMP_BAUD,
				      &data->rate, &size);
			
			/* We always reconfigure our port, no matter what */
			if (data->impl.changebaud(accept) < 0)
				goto fail;
			
			/*Palm device needs some time to reconfigure its port*/
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


/***********************************************************************
 *
 * Function:    pi_serial_getsockopt
 *
 * Summary:     get options on socket
 *
 * Parameters:  pi_socket*, level, option name, option value, option length
 *
 * Returns:     0 for success, -1 otherwise
 *
 ***********************************************************************/
static int
pi_serial_getsockopt(pi_socket_t *ps, int level, int option_name, 
		     void *option_value, size_t *option_len)
{
	struct pi_serial_data *data =
		(struct pi_serial_data *)ps->device->data;

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


/***********************************************************************
 *
 * Function:    pi_serial_setsockopt
 *
 * Summary:     set options on socket
 *
 * Parameters:  pi_socket*, level, option name, option value, option length
 *
 * Returns:     0 for success, -1 otherwise
 *
 ***********************************************************************/
static int
pi_serial_setsockopt(pi_socket_t *ps, int level, int option_name, 
		     const void *option_value, size_t *option_len)
{
	struct pi_serial_data *data =
		(struct pi_serial_data *)ps->device->data;

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
 * Parameters:  pi_socket*
 *
 * Returns:     always 0 for success
 *
 ***********************************************************************/
static int pi_serial_close(pi_socket_t *ps)
{
	struct pi_serial_data *data =
		(struct pi_serial_data *)ps->device->data;

	if (ps->sd)
		data->impl.close (ps);

	if (ps->laddr) {
		free(ps->laddr);
		ps->laddr = NULL;
	}

	if (ps->raddr) {
		free(ps->raddr);
		ps->raddr = NULL;
	}

	return 0;
}
