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
 *
 * -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef WIN32
#include <winsock.h>
#include <fcntl.h>
#include <io.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>		/* Needed for Redhat 6.x machines */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
#include <netinet/in.h>

#include "pi-debug.h"
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-inet.h"
#include "pi-cmp.h"
#include "pi-net.h"

static int pi_inet_connect(struct pi_socket *ps, struct sockaddr *addr,
			  int addrlen);
static int pi_inet_bind(struct pi_socket *ps, struct sockaddr *addr, 
		       int addrlen);
static int pi_inet_listen(struct pi_socket *ps, int backlog);
static int pi_inet_accept(struct pi_socket *ps, struct sockaddr *addr,
			 int *addrlen);
static int pi_inet_read(struct pi_socket *ps, unsigned char *msg, int len, int flags);
static int pi_inet_write(struct pi_socket *ps, unsigned char *msg, int len, int flags);
static int pi_inet_getsockopt(struct pi_socket *ps, int level, int option_name, 
			      void *option_value, int *option_len);
static int pi_inet_setsockopt(struct pi_socket *ps, int level, int option_name, 
			      const void *option_value, int *option_len);

static int pi_inet_close(struct pi_socket *ps);

/* Protocol Functions */
static struct pi_protocol *pi_inet_protocol_dup (struct pi_protocol *prot)
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

static void pi_inet_protocol_free (struct pi_protocol *prot)
{
	free(prot);
}

static struct pi_protocol *pi_inet_protocol (struct pi_device *dev)
{	
	struct 	pi_protocol *prot;
	struct 	pi_inet_data *data;
	
	data = dev->data;
	
	prot 			= (struct pi_protocol *)malloc (sizeof (struct pi_protocol));
	prot->level 		= PI_LEVEL_DEV;
	prot->dup 		= pi_inet_protocol_dup;
	prot->free 		= pi_inet_protocol_free;
	prot->read 		= pi_inet_read;
	prot->write 		= pi_inet_write;
	prot->getsockopt 	= pi_inet_getsockopt;
	prot->setsockopt 	= pi_inet_setsockopt;
	prot->data = NULL;
	
	return prot;
}

/* Device Functions */
static struct pi_device *pi_inet_device_dup (struct pi_device *dev)
{
	struct 	pi_device *new_dev;
	struct 	pi_inet_data *new_data, *data;
	
	new_dev 		= (struct pi_device *)malloc (sizeof (struct pi_device));
	new_dev->dup 		= dev->dup;
	new_dev->free 		= dev->free;
	new_dev->protocol 	= dev->protocol;	
	new_dev->bind 		= dev->bind;
	new_dev->listen 	= dev->listen;
	new_dev->accept 	= dev->accept;
	new_dev->connect 	= dev->connect;
	new_dev->close 		= dev->close;
	
	new_data 		= (struct pi_inet_data *)malloc (sizeof (struct pi_inet_data));
	data = (struct pi_inet_data *)dev->data;
	new_data->timeout 	= data->timeout;
	new_data->rx_bytes 	= 0;
	new_data->rx_errors 	= 0;
	new_data->tx_bytes 	= 0;
	new_data->tx_errors 	= 0;
	new_dev->data 		= new_data;
	
	return new_dev;
}

static void pi_inet_device_free (struct pi_device *dev) 
{
	free(dev->data);
	free(dev);
}

struct pi_device *pi_inet_device (int type) 
{
	struct pi_device *dev;
	struct pi_inet_data *data;
	
	dev = (struct pi_device *)malloc (sizeof (struct pi_device));
	data = (struct pi_inet_data *)malloc (sizeof (struct pi_inet_data));

	dev->dup 		= pi_inet_device_dup;
	dev->free 		= pi_inet_device_free;
	dev->protocol 		= pi_inet_protocol;	
	dev->bind 		= pi_inet_bind;
	dev->listen 		= pi_inet_listen;
	dev->accept 		= pi_inet_accept;
	dev->connect 		= pi_inet_connect;
	dev->close 		= pi_inet_close;
	
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
 * Function:    pi_inet_connect
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int
pi_inet_connect(struct pi_socket *ps, struct sockaddr *addr, int addrlen)
{
	int 	sd;
	struct 	pi_sockaddr *paddr = (struct pi_sockaddr *) addr;
	struct 	sockaddr_in serv_addr;
	char 	*device = paddr->pi_device + 4;
	
	/* Figure out the addresses to allow */
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	if (strlen(device) > 1) {
		serv_addr.sin_addr.s_addr = inet_addr(device);
		if (serv_addr.sin_addr.s_addr == -1) {
			struct hostent *hostent = gethostbyname(device);
		
			if (!hostent) {
				LOG(PI_DBG_DEV, PI_DBG_LVL_ERR, 
				    "DEV CONNECT Inet: Unable to determine host\n");
				return -1;
			}
			
			memcpy((char *) &serv_addr.sin_addr.s_addr,
			       hostent->h_addr, hostent->h_length);
		}
	} else {
		serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	serv_addr.sin_port = htons(14238);

	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0) {
		LOG(PI_DBG_DEV, PI_DBG_LVL_ERR, 
		    "DEV CONNECT Inet: Unable to create socket\n");
		return -1;
	}

	if (pi_socket_setsd (ps, sd) < 0)
		return -1;

	if (connect (ps->sd, (struct sockaddr *) &serv_addr,
		     sizeof(serv_addr)) < 0) {
		LOG(PI_DBG_DEV, PI_DBG_LVL_ERR, 
		    "DEV CONNECT Inet: Unable to connect\n");
		return -1;
	}
	
	ps->raddr 	= malloc(addrlen);
	memcpy(ps->raddr, addr, addrlen);
	ps->raddrlen 	= addrlen;
	ps->laddr 	= malloc(addrlen);
	memcpy(ps->laddr, addr, addrlen);
	ps->laddrlen 	= addrlen;

	switch (ps->cmd) {
	case PI_CMD_CMP:
		if (cmp_tx_handshake(ps) < 0)
			goto fail;
		break;
	case PI_CMD_NET:
		if (net_tx_handshake(ps) < 0)
			goto fail;
		break;
	}
	ps->state = PI_SOCK_CONIN;
	ps->command = 0;

	LOG(PI_DBG_DEV, PI_DBG_LVL_INFO, "DEV CONNECT Inet: Connected\n");
	return 0;

 fail:
	pi_close (ps->sd);
	return -1;
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
static int pi_inet_bind(struct pi_socket *ps, struct sockaddr *addr, int addrlen)
{
	int 	opt,
		optlen,
		sd;
	struct 	pi_sockaddr *paddr = (struct pi_sockaddr *) addr;
	struct 	sockaddr_in serv_addr;
	char 	*device = paddr->pi_device + 4, *port;

	/* Figure out the addresses to allow */
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	if (strlen(device) > 1 && strncmp(device, "any", 3)) {
		serv_addr.sin_addr.s_addr = inet_addr(device);
		if (serv_addr.sin_addr.s_addr == -1) {
			struct hostent *hostent = gethostbyname(device);
		
			if (!hostent)
				return -1;

			memcpy((char *) &serv_addr.sin_addr.s_addr,
			       hostent->h_addr, hostent->h_length);
		}
	} else {
		serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	if ((port = strchr(device, ':')) != NULL) {
		serv_addr.sin_port = htons(atoi(++port));
	} else {
		serv_addr.sin_port = htons(14238);
	}
	
	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0) {
		LOG(PI_DBG_DEV, PI_DBG_LVL_ERR, 
		    "DEV BIND Inet: Unable to create socket\n");
		return -1;
	}	
	if (pi_socket_setsd (ps, sd) < 0)
		return -1;

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

	if (bind(ps->sd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		return -1;

	LOG(PI_DBG_DEV, PI_DBG_LVL_INFO, "DEV BIND Inet Bound to %s\n", device);

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
 * Function:    pi_inet_listen
 *
 * Summary:     Wait for an incoming connection
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int pi_inet_listen(struct pi_socket *ps, int backlog)
{
	int 	result;
	
	result = listen(ps->sd, backlog);
	if (result == 0)
		ps->state = PI_SOCK_LISTN;

	return result;
}

/***********************************************************************
 *
 * Function:    pi_inet_accept
 *
 * Summary:     Accept an incoming connection
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int
pi_inet_accept(struct pi_socket *ps, struct sockaddr *addr, int *addrlen)
{
	struct 	pi_socket *acpt = NULL;

	acpt = pi_socket_copy(ps);
	
 	acpt->sd = accept(ps->sd, addr, addrlen);
	if (acpt->sd < 0)
		goto fail;

	pi_socket_init (acpt);

	switch (acpt->cmd) {
	case PI_CMD_CMP:
		if (cmp_rx_handshake(acpt, 57600, 0) < 0)
			return -1;
		break;
	case PI_CMD_NET:
		if (net_rx_handshake(acpt) < 0)
			return -1;
		break;
	}

	acpt->state 	= PI_SOCK_CONAC;
	acpt->command 	= 0;
	acpt->dlprecord = 0;

	pi_socket_recognize(acpt);

	LOG(PI_DBG_DEV, PI_DBG_LVL_INFO, "DEV ACCEPT Accepted\n");

	return acpt->sd;

 fail:
	if (acpt)
		pi_close (acpt->sd);
	return -1;
}

/***********************************************************************
 *
 * Function:    pi_inet_write
 *
 * Summary:     Send msg on a connected socket
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int
pi_inet_write(struct pi_socket *ps, unsigned char *msg, int len, int flags)
{
	int 	total,
		nwrote;
	struct 	pi_inet_data *data = (struct pi_inet_data *)ps->device->data;


	total = len;
	while (total > 0) {
		nwrote = write(ps->sd, msg, len);
		if (nwrote < 0)
			return -1;
		total -= nwrote;
	}
	data->tx_bytes += len;

	LOG(PI_DBG_DEV, PI_DBG_LVL_INFO, "DEV TX Inet Bytes: %d\n", len);

	return len;
}

/***********************************************************************
 *
 * Function:    pi_inet_read
 *
 * Summary:     Receive message on a connected socket
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int
pi_inet_read(struct pi_socket *ps, unsigned char *msg, int len, int flags)
{
	int 	r, 
		fl 	= 0;
	struct 	pi_inet_data *data = (struct pi_inet_data *)ps->device->data;
	fd_set 	ready;
	struct 	timeval t;

	switch (flags) {
	case PI_MSG_PEEK:
		fl = MSG_PEEK;
	}
	
	FD_ZERO(&ready);
	FD_SET(ps->sd, &ready);

	/* If timeout == 0, wait forever for packet, otherwise wait till
	   timeout milliseconds */
	if (data->timeout == 0)
		select(ps->sd + 1, &ready, 0, 0, 0);
	else {
		t.tv_sec 	= data->timeout / 1000;
		t.tv_usec 	= (data->timeout % 1000) * 1000;
		select(ps->sd + 1, &ready, 0, 0, &t);
	}
	/* If data is available in time, read it */
	if (FD_ISSET(ps->sd, &ready))
		r = recv(ps->sd, msg, len, fl);
	else {
		/* otherwise throw out any current packet and return */
		LOG(PI_DBG_DEV, PI_DBG_LVL_WARN, "DEV RX Inet timeout\n");
		data->rx_errors++;
		return 0;
	}
	data->rx_bytes += r;

	LOG(PI_DBG_DEV, PI_DBG_LVL_INFO, "DEV RX Inet Bytes: %d\n", r);

	return r;
}

static int
pi_inet_getsockopt(struct pi_socket *ps, int level, int option_name, 
		   void *option_value, int *option_len)
{
	struct 	pi_inet_data *data = (struct pi_inet_data *)ps->device->data;
		
	return 0;
}

static int
pi_inet_setsockopt(struct pi_socket *ps, int level, int option_name, 
		   const void *option_value, int *option_len)
{
	struct 	pi_inet_data *data = (struct pi_inet_data *)ps->device->data;

	return 0;
}


/***********************************************************************
 *
 * Function:    pi_inet_close
 *
 * Summary:     Close a connection, destroy the socket
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int pi_inet_close(struct pi_socket *ps)
{
	/* Close sd handle */
	if (ps->sd)
		close(ps->sd);

	if (ps->laddr)
		free(ps->laddr);
	if (ps->raddr)
		free(ps->raddr);

	return 0;
}
