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
#include <sys/stat.h>
#include <sys/time.h>

#include "pi-debug.h"
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-usb.h"
#include "pi-net.h"
#include "pi-cmp.h"

static int pi_usb_connect(struct pi_socket *ps, struct sockaddr *addr, 
			     int addrlen);
static int pi_usb_bind(struct pi_socket *ps, struct sockaddr *addr,
			  int addrlen);
static int pi_usb_listen(struct pi_socket *ps, int backlog);
static int pi_usb_accept(struct pi_socket *ps, struct sockaddr *addr,
			    int *addrlen);
static int pi_usb_getsockopt(struct pi_socket *ps, int level, int option_name, 
				void *option_value, int *option_len);
static int pi_usb_setsockopt(struct pi_socket *ps, int level, int option_name, 
				const void *option_value, int *option_len);
static int pi_usb_close(struct pi_socket *ps);

int pi_socket_init(struct pi_socket *ps);

/* Protocol Functions */
static struct pi_protocol *pi_usb_protocol_dup (struct pi_protocol *prot)
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

static void pi_usb_protocol_free (struct pi_protocol *prot)
{
	free(prot);
}

static struct pi_protocol *pi_usb_protocol (struct pi_device *dev)
{	
	struct pi_protocol *prot;
	struct pi_usb_data *data;
	
	data = dev->data;
	
	prot 			= (struct pi_protocol *)malloc (sizeof (struct pi_protocol));
	prot->level 		= PI_LEVEL_DEV;
	prot->dup 		= pi_usb_protocol_dup;
	prot->free 		= pi_usb_protocol_free;
	prot->read 		= data->impl.read;
	prot->write 		= data->impl.write;
	prot->getsockopt 	= pi_usb_getsockopt;
	prot->setsockopt 	= pi_usb_setsockopt;
	prot->data 		= NULL;
	
	return prot;
}

/* Device Functions */
static struct pi_device *pi_usb_device_dup (struct pi_device *dev)
{
	struct 	pi_device *new_dev;
	struct 	pi_usb_data *new_data, *data;
	
	new_dev 		= (struct pi_device *)malloc (sizeof (struct pi_device));
	new_dev->dup 		= dev->dup;
	new_dev->free 		= dev->free;
	new_dev->protocol 	= dev->protocol;	
	new_dev->bind 		= dev->bind;
	new_dev->listen 	= dev->listen;
	new_dev->accept 	= dev->accept;
	new_dev->connect 	= dev->connect;
	new_dev->close 		= dev->close;
	
	new_data 		= (struct pi_usb_data *)malloc (sizeof (struct pi_usb_data));
	data 			= (struct pi_usb_data *)dev->data;
	new_data->impl 		= data->impl;
	memcpy(new_data->buf, data->buf, data->buf_size);
	new_data->buf_size 	= data->buf_size;
	new_data->ref           = data->ref;
	(*(new_data->ref))++;
	new_data->timeout 	= data->timeout;
	new_dev->data 		= new_data;
	
	return new_dev;
}

static void pi_usb_device_free (struct pi_device *dev) 
{
	free(dev->data);
	free(dev);
}

struct pi_device *pi_usb_device (int type) 
{
	struct 	pi_device *dev;
	struct 	pi_usb_data *data;
	
	dev 	= (struct pi_device *)malloc (sizeof (struct pi_device));
	data 	= (struct pi_usb_data *)malloc (sizeof (struct pi_usb_data));

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
	data->ref               = (int *)malloc (sizeof (int));
	data->timeout 		= 0;
	dev->data 		= data;
	
	return dev;
}

/***********************************************************************
 *
 * Function:    pi_usb_connect
 *
 * Summary:     Connect socket to a given address
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
static int
pi_usb_connect(struct pi_socket *ps, struct sockaddr *addr, int addrlen)
{
	struct 	pi_usb_data *data = (struct pi_usb_data *)ps->device->data;
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
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
static int
pi_usb_bind(struct pi_socket *ps, struct sockaddr *addr, int addrlen)
{
	struct 	pi_usb_data *data = (struct pi_usb_data *)ps->device->data;
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
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int pi_usb_listen(struct pi_socket *ps, int backlog)
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
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int
pi_usb_accept(struct pi_socket *ps, struct sockaddr *addr, int *addrlen)
{
	struct 	pi_usb_data *data = (struct pi_usb_data *)ps->device->data;
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

static int
pi_usb_getsockopt(struct pi_socket *ps, int level, int option_name, 
		     void *option_value, int *option_len)
{
	struct pi_usb_data *data = (struct pi_usb_data *)ps->device->data;

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

static int
pi_usb_setsockopt(struct pi_socket *ps, int level, int option_name, 
		     const void *option_value, int *option_len)
{
	struct pi_usb_data *data = (struct pi_usb_data *)ps->device->data;

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
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int pi_usb_close(struct pi_socket *ps)
{
	struct pi_usb_data *data = (struct pi_usb_data *)ps->device->data;

	(*(data->ref))--;

	if (ps->sd)
		data->impl.close (ps);

	if (ps->laddr)
		free(ps->laddr);
	if (ps->raddr)
		free(ps->raddr);

	return 0;
}
