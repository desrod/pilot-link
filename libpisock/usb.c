/*
 * usb.c: Interface layer to serial HotSync connections
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
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "pi-debug.h"
#include "pi-source.h"
#include "pi-usb.h"
#include "pi-net.h"
#include "pi-cmp.h"

static int pi_usb_connect(pi_socket_t *ps, struct sockaddr *addr, 
			     socklen_t addrlen);
static int pi_usb_bind(pi_socket_t *ps, struct sockaddr *addr,
			  socklen_t addrlen);
static int pi_usb_listen(pi_socket_t *ps, int backlog);
static int pi_usb_accept(pi_socket_t *ps, struct sockaddr *addr,
			    socklen_t *addrlen);
static int pi_usb_getsockopt(pi_socket_t *ps, int level, int option_name, 
				void *option_value, size_t *option_len);
static int pi_usb_setsockopt(pi_socket_t *ps, int level, int option_name, 
				const void *option_value, size_t *option_len);
static int pi_usb_close(pi_socket_t *ps);

int pi_socket_init(pi_socket_t *ps);

/* Protocol Functions */
/***********************************************************************
 *
 * Function:    pi_usb_protocol_dup
 *
 * Summary:     creates a new copy of a USB pi_protocol instance
 *
 * Parameters:	pi_protocol_t*
 *
 * Returns:     new pi_protocol_t* or NULL if operation failed
 *
 ***********************************************************************/
pi_protocol_t *
pi_usb_protocol_dup (pi_protocol_t *prot)
{
	pi_protocol_t *new_prot;

	ASSERT(prot != NULL);
	
	new_prot = (pi_protocol_t *)malloc (sizeof (pi_protocol_t));

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
 * Function:    pi_usb_protocol_free
 *
 * Summary:     frees USB pi_protocol instance
 *
 * Parameters:	pi_protocol_t*
 *
 * Returns:     void
 *
 ***********************************************************************/
static void
pi_usb_protocol_free (pi_protocol_t *prot)
{
	ASSERT(prot != NULL);
	
	if (prot != NULL)
		free(prot);
}


/***********************************************************************
 *
 * Function:    pi_usb_protocol
 *
 * Summary:     creates a new USB pi_protocol instance
 *
 * Parameters:	pi_device_t*
 *
 * Returns:     new pi_protocol_t* or NULL if operation failed
 *
 ***********************************************************************/
static pi_protocol_t *
pi_usb_protocol (pi_device_t *dev)
{	
	pi_protocol_t *prot;
	pi_usb_data_t *data;

	ASSERT(dev != NULL);
	
	data = dev->data;
	
	prot 	= (pi_protocol_t *)malloc (sizeof (pi_protocol_t));

	if (prot != NULL) {
		prot->level 		= PI_LEVEL_DEV;
		prot->dup 		= pi_usb_protocol_dup;
		prot->free 		= pi_usb_protocol_free;
		prot->read 		= data->impl.read;
		prot->write 		= data->impl.write;
		prot->getsockopt 	= pi_usb_getsockopt;
		prot->setsockopt 	= pi_usb_setsockopt;
		prot->data 		= NULL;
	}
	
	return prot;
}

/* Device Functions */
/***********************************************************************
 *
 * Function:    pi_usb_device_dup
 *
 * Summary:     creates a new copy of a USB pi_device instance
 *
 * Parameters:	pi_device_t*
 *
 * Returns:     new pi_device_t* or NULL if operation failed
 *
 ***********************************************************************/
static pi_device_t *
pi_usb_device_dup (pi_device_t *dev)
{
	pi_device_t *new_dev = NULL;

	struct 	pi_usb_data	*new_data = NULL,
				*data = NULL;

	ASSERT(dev != NULL);
	
	new_dev	= (pi_device_t *)malloc (sizeof (pi_device_t));
		if (new_dev != NULL) {
			new_data = (pi_usb_data_t *)malloc
				(sizeof (struct pi_usb_data));
			if (new_data == NULL) {
				free(new_dev);
				new_dev = NULL;
			}
		}

	if ( (new_dev != NULL) && (new_data != NULL) ) {

		new_dev->dup 		= dev->dup;
		new_dev->free 		= dev->free;
		new_dev->protocol 	= dev->protocol;	
		new_dev->bind 		= dev->bind;
		new_dev->listen 	= dev->listen;
		new_dev->accept 	= dev->accept;
		new_dev->connect 	= dev->connect;
		new_dev->close 		= dev->close;
	
		data 	= (pi_usb_data_t *)dev->data;
		new_data->impl 		= data->impl;
		memcpy(new_data->buf, data->buf, data->buf_size);
		new_data->buf_size 	= data->buf_size;
		new_data->ref           = NULL;
		new_data->timeout 	= data->timeout;
		new_dev->data 		= new_data;
	}
	
	return new_dev;
}


/***********************************************************************
 *
 * Function:    pi_usb_device_free
 *
 * Summary:     frees USB pi_device instance
 *
 * Parameters:	pi_device_t*
 *
 * Returns:     void
 *
 ***********************************************************************/
static void
pi_usb_device_free (pi_device_t *dev) 
{
	pi_usb_data_t *data = (pi_usb_data_t *)dev->data;

	ASSERT(dev != NULL);

	if (data != NULL)
		free(data);
	if (dev != NULL)
		free(dev);
}


/***********************************************************************
 *
 * Function:    pi_usb_device
 *
 * Summary:     creates a new USB pi_device instance
 *
 * Parameters:	pi_device_t*
 *
 * Returns:     new pi_device_t* or NULL if operation failed
 *
 ***********************************************************************/
pi_device_t *
pi_usb_device (int type) 
{
	pi_device_t *dev;
	pi_usb_data_t *data;
	
	dev = (pi_device_t *)malloc (sizeof (pi_device_t));
	if (dev != NULL) {
		data = (pi_usb_data_t *)malloc (sizeof (struct pi_usb_data));
		if (data == NULL) {
			free(dev);
			dev = NULL;
		}
	}
	
	if ( (dev != NULL) && (data != NULL) ) {

		dev->dup 		= pi_usb_device_dup;
		dev->free 		= pi_usb_device_free;
		dev->protocol 		= pi_usb_protocol;	
		dev->bind 		= pi_usb_bind;
		dev->listen 		= pi_usb_listen;
		dev->accept 		= pi_usb_accept;
		dev->connect 		= pi_usb_connect;
		dev->close 		= pi_usb_close;

		switch (type) {
		case PI_USB_DEV:
			pi_usb_impl_init (&data->impl);
			break;
		default:
			pi_usb_impl_init (&data->impl);
			break;
		}
	
		data->buf_size 		= 0;
		data->ref               = NULL;
		data->timeout 		= 0;
		dev->data 		= data;
	}
	
	return dev;
}

/***********************************************************************
 *
 * Function:    pi_usb_connect
 *
 * Summary:     Connect socket to a given address
 *
 * Parameters:  pi_socket_t*, sockaddr*, socket length
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
static int
pi_usb_connect(pi_socket_t *ps, struct sockaddr *addr, socklen_t addrlen)
{
	struct 	pi_usb_data *data = (pi_usb_data_t *)ps->device->data;
	struct 	pi_sockaddr *pa = (struct pi_sockaddr *) addr;

	if (data->impl.open(ps, pa, addrlen) == -1)
		return -1;	/* errno already set */

	ps->raddr 	= malloc(addrlen);
	memcpy(ps->raddr, addr, addrlen);
	ps->raddrlen 	= addrlen;
	ps->laddr 	= malloc(addrlen);
	memcpy(ps->laddr, addr, addrlen);
	ps->laddrlen 	= addrlen;

	if (ps->type == PI_SOCK_STREAM) {
		switch (ps->cmd) {
		case PI_CMD_CMP:
			break;
			
		case PI_CMD_NET:
			if (net_tx_handshake(ps) < 0)
				return -1;
			break;
		}
	}
	ps->state = PI_SOCK_CONIN;
	ps->command = 0;

	return 0;
}

/***********************************************************************
 *
 * Function:    pi_usb_bind
 *
 * Summary:     Bind address to a local socket
 *
 * Parameters:  pi_socket_t*, sockaddr*, socket length
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
static int
pi_usb_bind(pi_socket_t *ps, struct sockaddr *addr, socklen_t addrlen)
{
	struct 	pi_usb_data *data = (pi_usb_data_t *)ps->device->data;
	struct 	pi_sockaddr *pa = (struct pi_sockaddr *) addr;

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
 * Function:    pi_usb_listen
 *
 * Summary:     Prepare for incoming connections
 *
 * Parameters:  pi_socket_t*, backlog
 *
 * Returns:     0
 *
 ***********************************************************************/
static int
pi_usb_listen(pi_socket_t *ps, int backlog)
{
	ps->state = PI_SOCK_LISTN;
	
	return 0;
}

/***********************************************************************
 *
 * Function:    pi_usb_accept
 *
 * Summary:     Accept an incoming connection
 *
 * Parameters:  pi_socket_t*, sockaddr*, socket length
 *
 * Returns:     pi_socket descriptor or -1 on error
 *
 ***********************************************************************/
static int
pi_usb_accept(pi_socket_t *ps, struct sockaddr *addr, socklen_t *addrlen)
{
	struct 	pi_usb_data *data = (pi_usb_data_t *)ps->device->data;
	struct 	pi_socket *accept = NULL;

	/* Wait for data */
	if (data->impl.poll(ps, ps->accept_to) < 0)
		goto fail;
	
	accept = pi_socket_copy(ps);
	data = accept->device->data;
	data->timeout = accept->accept_to * 1000;

	pi_socket_init(accept);
	if (ps->type == PI_SOCK_STREAM) {
		switch (accept->cmd) {
		case PI_CMD_CMP:
			if (cmp_rx_handshake(accept, 57600, 1) < 0)
				return -1;

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
 * Function:    pi_usb_getsockopt
 *
 * Summary:     get options on USB socket
 *
 * Parameters:  pi_socket*, level, option name, option value, option length
 *
 * Returns:     0 for success, -1 otherwise
 *
 ***********************************************************************/
static int
pi_usb_getsockopt(pi_socket_t *ps, int level, int option_name, 
		     void *option_value, size_t *option_len)
{
	pi_usb_data_t *data = (pi_usb_data_t *)ps->device->data;

	switch (option_name) {
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
 * Function:    pi_usb_setsockopt
 *
 * Summary:     set options on USB socket
 *
 * Parameters:  pi_socket*, level, option name, option value, option length
 *
 * Returns:     0 for success, -1 otherwise
 *
 ***********************************************************************/
static int
pi_usb_setsockopt(pi_socket_t *ps, int level, int option_name, 
		     const void *option_value, size_t *option_len)
{
	pi_usb_data_t *data = (pi_usb_data_t *)ps->device->data;

	switch (option_name) {
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
 * Function:    pi_usb_close
 *
 * Summary:     Close a connection, destroy the socket
 *
 * Parameters:  pi_socket_t*
 *
 * Returns:     0
 *
 ***********************************************************************/
static int
pi_usb_close(pi_socket_t *ps)
{
	pi_usb_data_t *data = (pi_usb_data_t *)ps->device->data;

	if (ps->sd != 0)
		data->impl.close (ps);

	if (ps->laddr != NULL) {
		free(ps->laddr);
		ps->laddr = NULL;
	}
	if (ps->raddr != NULL) {
		free(ps->raddr);
		ps->raddr = NULL;
	}

	return 0;
}
