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
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>		/* Needed for Redhat 6.x machines */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "pi-debug.h"
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-inet.h"
#include "pi-cmp.h"
#include "pi-net.h"
#include "pi-error.h"

/* Declare prototypes */
static int pi_inet_connect(pi_socket_t *ps, struct sockaddr *addr,
			   size_t addrlen);
static int pi_inet_bind(pi_socket_t *ps, struct sockaddr *addr, 
		       size_t addrlen);
static int pi_inet_listen(pi_socket_t *ps, int backlog);
static int pi_inet_accept(pi_socket_t *ps, struct sockaddr *addr,
			 size_t *addrlen);
static ssize_t pi_inet_read(pi_socket_t *ps, pi_buffer_t *msg, size_t len, int flags);
static ssize_t pi_inet_write(pi_socket_t *ps, unsigned char *msg, size_t len, int flags);
static int pi_inet_getsockopt(pi_socket_t *ps, int level, int option_name, 
			      void *option_value, size_t *option_len);
static int pi_inet_setsockopt(pi_socket_t *ps, int level, int option_name, 
			      const void *option_value, size_t *option_len);

static int pi_inet_close(pi_socket_t *ps);
static int pi_inet_flush(pi_socket_t *ps, int flags);

int pi_socket_init(pi_socket_t *ps);

/* Protocol Functions */
/***********************************************************************
 *
 * Function:    pi_inet_protocol_dup
 *
 * Summary:     clones an existing pi_protocol struct
 *
 * Parameters:  pi_protocol_t*
 *
 * Returns:     pi_protocol_t* or NULL if operation failed
 *
 ***********************************************************************/
static pi_protocol_t
*pi_inet_protocol_dup (pi_protocol_t *prot)
{
	pi_protocol_t *new_prot;

	ASSERT (prot != NULL);
	
	new_prot = (pi_protocol_t *)malloc (sizeof (pi_protocol_t));

	if (new_prot != NULL) {
		new_prot->level 	= prot->level;
		new_prot->dup 		= prot->dup;
		new_prot->free 		= prot->free;
		new_prot->read 		= prot->read;
		new_prot->write 	= prot->write;
		new_prot->flush		= prot->flush;
		new_prot->getsockopt 	= prot->getsockopt;
		new_prot->setsockopt 	= prot->setsockopt;
		new_prot->data 		= NULL;
	}

	return new_prot;
}


/***********************************************************************
 *
 * Function:    pi_inet_protocol_free
 *
 * Summary:     frees an existing pi_protocol struct
 *
 * Parameters:  pi_protocol*
 *
 * Returns:     void
 *
 ***********************************************************************/
static void
pi_inet_protocol_free (pi_protocol_t *prot)
{
	ASSERT (prot != NULL);
	if (prot != NULL)
		free(prot);
}


/***********************************************************************
 *
 * Function:    pi_inet_protocol
 *
 * Summary:     creates and inits pi_protocol struct instance
 *
 * Parameters:  pi_device_t*
 *
 * Returns:     pi_protocol_t* or NULL if operation failed
 *
 ***********************************************************************/
static pi_protocol_t
*pi_inet_protocol (pi_device_t *dev)
{	
	pi_protocol_t *prot;
	pi_inet_data_t *data;

	ASSERT (dev != NULL);
	
	data = dev->data;
	
	prot 	= (pi_protocol_t *)malloc (sizeof (pi_protocol_t));

	if (prot != NULL) {
		prot->level 		= PI_LEVEL_DEV;
		prot->dup 		= pi_inet_protocol_dup;
		prot->free 		= pi_inet_protocol_free;
		prot->read 		= pi_inet_read;
		prot->write 		= pi_inet_write;
		prot->flush		= pi_inet_flush;
		prot->getsockopt 	= pi_inet_getsockopt;
		prot->setsockopt 	= pi_inet_setsockopt;
		prot->data = NULL;
	}
	
	return prot;
}

/***********************************************************************
 *
 * Function:    pi_inet_device_free
 *
 * Summary:     frees an existing pi_device struct
 *
 * Parameters:  pi_device_t*
 *
 * Returns:     void
 *
 ***********************************************************************/
static void
pi_inet_device_free (pi_device_t *dev) 
{
	ASSERT (dev != NULL);
	if (dev != NULL) {
		if (dev->data != NULL)
			free(dev->data);
		free(dev);
	}
}


/***********************************************************************
 *
 * Function:    pi_inet_device
 *
 * Summary:     creates and inits pi_device struct instance 
 *
 * Parameters:  device type
 *
 * Returns:     pi_device_t* or NULL if operation failed
 *
 ***********************************************************************/
pi_device_t
*pi_inet_device (int type) 
{
	pi_device_t *dev = NULL;
	pi_inet_data_t *data = NULL;

	dev = (pi_device_t *)malloc (sizeof (pi_device_t));
	if (dev != NULL) {
		data = (pi_inet_data_t *)malloc (sizeof (pi_inet_data_t));
		if (data == NULL) {
			free(dev);
			dev = NULL;
		}
	}

	if (dev != NULL && data != NULL) {

		dev->free 	= pi_inet_device_free;
		dev->protocol 	= pi_inet_protocol;	
		dev->bind 	= pi_inet_bind;
		dev->listen 	= pi_inet_listen;
		dev->accept 	= pi_inet_accept;
		dev->connect 	= pi_inet_connect;
		dev->close 	= pi_inet_close;
	
		data->timeout 	= 0;
		data->rx_bytes 	= 0;
		data->rx_errors	= 0;
		data->tx_bytes 	= 0;
		data->tx_errors	= 0;
		dev->data 	= data;
	}
	
	return dev;
}

/***********************************************************************
 *
 * Function:    pi_inet_connect
 *
 * Summary:	establish an inet connection
 *
 * Parameters:  pi_socket_t*, sockaddr*, address length
 *
 * Returns:     0 for success, negative otherwise
 *
 ***********************************************************************/
static int
pi_inet_connect(pi_socket_t *ps, struct sockaddr *addr, size_t addrlen)
{
	int 	sd,
		err;

	struct 	pi_sockaddr *paddr = (struct pi_sockaddr *) addr;
	struct 	sockaddr_in serv_addr;
	char 	*device = paddr->pi_device;
	
	/* Figure out the addresses to allow */
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	if (strlen(device) > 1) {
		serv_addr.sin_addr.s_addr = inet_addr(device);
		if (serv_addr.sin_addr.s_addr == (in_addr_t)-1) {
			struct hostent *hostent = gethostbyname(device);
		
			if (!hostent) {
				LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, 
				    "DEV CONNECT Inet: Unable"
					" to determine host\n"));
				return pi_set_error(ps->sd, PI_ERR_GENERIC_SYSTEM);
			}
			
			memcpy((char *) &serv_addr.sin_addr.s_addr,
			       hostent->h_addr, (size_t)hostent->h_length);
		}
	} else {
		serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	serv_addr.sin_port = htons(14238);

	sd = socket(AF_INET, SOCK_STREAM, 0);

	if (sd < 0) {
		LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, 
		    "DEV CONNECT Inet: Unable to create socket\n"));
		return pi_set_error(ps->sd, PI_ERR_GENERIC_SYSTEM);
	}

	if ((err = pi_socket_setsd (ps, sd)) < 0)
		return err;

	if (connect (ps->sd, (struct sockaddr *) &serv_addr,
		     sizeof(serv_addr)) < 0) {
		LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, 
		    "DEV CONNECT Inet: Unable to connect\n"));
		return pi_set_error(ps->sd, PI_ERR_GENERIC_SYSTEM);
	}

	ps->raddr 	= malloc(addrlen);
	memcpy(ps->raddr, addr, addrlen);
	ps->raddrlen 	= addrlen;
	ps->laddr 	= malloc(addrlen);
	memcpy(ps->laddr, addr, addrlen);
	ps->laddrlen 	= addrlen;

	switch (ps->cmd) {
		case PI_CMD_CMP:
			if ((err = cmp_tx_handshake(ps)) < 0)
				goto fail;
			break;
		case PI_CMD_NET:
			if ((err = net_tx_handshake(ps)) < 0)
				goto fail;
			break;
	}
	ps->state = PI_SOCK_CONIN;
	ps->command = 0;

	LOG((PI_DBG_DEV, PI_DBG_LVL_INFO, "DEV CONNECT Inet: Connected\n"));
	return 0;

 fail:
	pi_close (ps->sd);
	return err;
}


/***********************************************************************
 *
 * Function:    pi_inet_bind
 *
 * Summary:     Bind address to a local socket
 *
 * Parameters:  pi_socket_t*, sockaddr*, address length
 *
 * Returns:     0 for success, negative otherwise
 *
 ***********************************************************************/
static int
pi_inet_bind(pi_socket_t *ps, struct sockaddr *addr, size_t addrlen)
{
	int 	opt,
		sd,
		err;
	size_t	optlen;
	struct 	pi_sockaddr *paddr = (struct pi_sockaddr *) addr;
	struct 	sockaddr_in serv_addr;
	char 	*device = paddr->pi_device, 
		*port	= NULL;

	/* Figure out the addresses to allow */
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	if (strlen(device) > 1 && strncmp(device, "any", 3)) {
		serv_addr.sin_addr.s_addr = inet_addr(device);
		if (serv_addr.sin_addr.s_addr == (in_addr_t)-1) {
			struct hostent *hostent = gethostbyname(device);
		
			if (!hostent)
				return pi_set_error(ps->sd, PI_ERR_GENERIC_SYSTEM);

			memcpy((char *) &serv_addr.sin_addr.s_addr,
			       hostent->h_addr, (size_t)hostent->h_length);
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
		LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, 
		    "DEV BIND Inet: Unable to create socket\n"));
		return pi_set_error(ps->sd, PI_ERR_GENERIC_SYSTEM);
	}	
	if ((err = pi_socket_setsd (ps, sd)) < 0)
		return err;

	opt = 1;
	optlen = sizeof(opt);

	if (setsockopt
	    (ps->sd, SOL_SOCKET, SO_REUSEADDR, (void *) &opt,
	     (int)optlen) < 0) {
		return pi_set_error(ps->sd, PI_ERR_GENERIC_SYSTEM);
	}

	if (bind(ps->sd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		return pi_set_error(ps->sd, PI_ERR_GENERIC_SYSTEM);

	LOG((PI_DBG_DEV, PI_DBG_LVL_INFO,
		"DEV BIND Inet Bound to %s\n", device));

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
 * Parameters:  pi_socket_t*, backlog
 *
 * Returns:     0 on success, negative otherwise
 *
 ***********************************************************************/
static int
pi_inet_listen(pi_socket_t *ps, int backlog)
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
 * Parameters:  pi_socket_t*, sockaddr*, address length
 *
 * Returns:     socket descriptor on success, negative otherwise
 *
 ***********************************************************************/
#include <netinet/tcp.h>
static int
pi_inet_accept(pi_socket_t *ps, struct sockaddr *addr, size_t *addrlen)
{
	int	sd,
		err,
		split = 0,
		chunksize = 0;
	size_t	len;
	pl_socklen_t l = 0;
	
	if (addrlen)
		l = *addrlen;
 	sd = accept(ps->sd, addr, &l);
	if (addrlen)
		*addrlen = l;
	if (sd < 0)
		goto fail;

	pi_socket_setsd(ps, sd);
	pi_socket_init(ps);

	switch (ps->cmd) {
		case PI_CMD_CMP:
			if ((err = cmp_rx_handshake(ps, 57600, 0)) < 0)
				return err;
			break;
		case PI_CMD_NET:
			/* network: make sure we don't split writes. set socket option
			 * on both the command and non-command instances of the protocol
			 */
			len = sizeof (split);
			pi_setsockopt(ps->sd, PI_LEVEL_NET, PI_NET_SPLIT_WRITES,
				&split, &len);
			len = sizeof (chunksize);
			pi_setsockopt(ps->sd, PI_LEVEL_NET, PI_NET_WRITE_CHUNKSIZE,
				&chunksize, &len);

			ps->command ^= 1;
			len = sizeof (split);
			pi_setsockopt(ps->sd, PI_LEVEL_NET, PI_NET_SPLIT_WRITES,
				&split, &len);
			len = sizeof (chunksize);
			pi_setsockopt(ps->sd, PI_LEVEL_NET, PI_NET_WRITE_CHUNKSIZE,
				&chunksize, &len);
			ps->command ^= 1;

			if ((err = net_rx_handshake(ps)) < 0)
				return err;
			break;
	}

	ps->state 	= PI_SOCK_CONAC;
	ps->command 	= 0;
	ps->dlprecord = 0;

	LOG((PI_DBG_DEV, PI_DBG_LVL_INFO, "DEV ACCEPT Accepted\n"));

	return ps->sd;

 fail:
	if (ps)
		pi_close (ps->sd);
	return PI_ERR_GENERIC_SYSTEM;
}

/***********************************************************************
 *
 * Function:    pi_inet_flush
 *
 * Summary:     Flush input and/or output buffers on the socket
 *
 * Parameters:  pi_socket_t*, flags
 *
 * Returns:     0 on success or negative number on error
 *
 ***********************************************************************/
static int
pi_inet_flush(pi_socket_t *ps, int flags)
{
	char buf[256];
	int fl;

	if (flags & PI_FLUSH_INPUT) {
		if ((fl = fcntl(ps->sd, F_GETFL, 0)) != -1) {
			fcntl(ps->sd, F_SETFL, fl | O_NONBLOCK);
			while (recv(ps->sd, buf, sizeof(buf), 0) > 0)
				;
			fcntl(ps->sd, F_SETFL, fl);
		}
	}
	return 0;
}

/***********************************************************************
 *
 * Function:    pi_inet_write
 *
 * Summary:     Send msg on a connected socket
 *
 * Parameters:  pi_socket_t*, char* to message, length of message, flags
 *
 * Returns:     number of bytes written or negative on error
 *
 ***********************************************************************/
static ssize_t
pi_inet_write(pi_socket_t *ps, unsigned char *msg, size_t len, int flags)
{
	int 	total,
		nwrote;
	pi_inet_data_t *data = (pi_inet_data_t *)ps->device->data;
	struct 	timeval t;
	fd_set 	ready;

	FD_ZERO(&ready);
	FD_SET(ps->sd, &ready);

	total = len;
	while (total > 0) {
		if (data->timeout == 0) {
			if (select(ps->sd + 1, 0, &ready, 0, 0) < 0
				&& errno == EINTR)
				continue;
		} else {
			t.tv_sec 	= data->timeout / 1000;
			t.tv_usec 	= (data->timeout % 1000) * 1000;
			if (select(ps->sd + 1, 0, &ready, 0, &t) == 0)
				return pi_set_error(ps->sd, PI_ERR_SOCK_TIMEOUT);
		}
		if (!FD_ISSET(ps->sd, &ready)) {
			ps->state = PI_SOCK_CONBK;
			return pi_set_error(ps->sd, PI_ERR_SOCK_DISCONNECTED);
		}

		nwrote = write(ps->sd, msg, len);
		if (nwrote < 0) {
			/* test errno to properly set the socket error */
			if (errno == EPIPE || errno == EBADF) {
				ps->state = PI_SOCK_CONBK;
				return pi_set_error(ps->sd, PI_ERR_SOCK_DISCONNECTED);
			}
			return pi_set_error(ps->sd, PI_ERR_SOCK_IO);
		}

		total -= nwrote;
	}
	data->tx_bytes += len;

	LOG((PI_DBG_DEV, PI_DBG_LVL_INFO, "DEV TX Inet Bytes: %d\n", len));

	return len;
}

/***********************************************************************
 *
 * Function:    pi_inet_read
 *
 * Summary:     Receive message on a connected socket
 *
 * Parameters:  pi_socket_t*, char* to message, length of message, flags
 *
 * Returns:     number of bytes read or negative on error
 *
 ***********************************************************************/
static ssize_t
pi_inet_read(pi_socket_t *ps, pi_buffer_t *msg, size_t len, int flags)
{
	int 	r, 
		fl 	= 0;
	pi_inet_data_t *data = (pi_inet_data_t *)ps->device->data;
	fd_set 	ready;
	struct 	timeval t;

	if (pi_buffer_expect (msg, len) == NULL) {
		errno = ENOMEM;
		return pi_set_error(ps->sd, PI_ERR_GENERIC_MEMORY);
	}

	if (flags == PI_MSG_PEEK)
		fl = MSG_PEEK;
	
	FD_ZERO(&ready);
	FD_SET(ps->sd, &ready);

	/* If timeout == 0, wait forever for packet, otherwise wait till
	   timeout milliseconds */
	if (data->timeout == 0)
		select(ps->sd + 1, &ready, 0, 0, 0);
	else {
		t.tv_sec 	= data->timeout / 1000;
		t.tv_usec 	= (data->timeout % 1000) * 1000;
		if (select(ps->sd + 1, &ready, 0, 0, &t) == 0)
			return pi_set_error(ps->sd, PI_ERR_SOCK_TIMEOUT);
	}

	/* If data is available in time, read it */
	if (FD_ISSET(ps->sd, &ready)) {
		r = recv(ps->sd, msg->data + msg->used, len, fl);
		if (r < 0) {
			if (errno == EPIPE || errno == EBADF) {
				ps->state = PI_SOCK_CONBK;
				return pi_set_error(ps->sd, PI_ERR_SOCK_DISCONNECTED);
			}
			return pi_set_error(ps->sd, PI_ERR_SOCK_IO);
		}

		data->rx_bytes += r;
		msg->used += r;

		LOG((PI_DBG_DEV, PI_DBG_LVL_INFO, "DEV RX Inet Bytes: %d\n", r));
		return r;
	}

	/* otherwise throw out any current packet and return */
	LOG((PI_DBG_DEV, PI_DBG_LVL_WARN, "DEV RX Inet timeout\n"));
	data->rx_errors++;
	return 0;
}


/***********************************************************************
 *
 * Function:    pi_inet_getsockopt
 *
 * Summary:     get options on socket
 *
 * Parameters:  pi_socket*, level, option name, option value, option length
 *
 * Returns:     0 for success, negative otherwise
 *
 ***********************************************************************/
static int
pi_inet_getsockopt(pi_socket_t *ps, int level, int option_name, 
		   void *option_value, size_t *option_len)
{
	pi_inet_data_t *data = (pi_inet_data_t *)ps->device->data;

	switch (option_name) {
		case PI_DEV_TIMEOUT:
			if (*option_len != sizeof (data->timeout)) {
				errno = EINVAL;
				return pi_set_error(ps->sd, PI_ERR_GENERIC_ARGUMENT);
			}
			memcpy (option_value, &data->timeout,
				sizeof (data->timeout));
			*option_len = sizeof (data->timeout);
			break;
	}

	return 0;
}


/***********************************************************************
 *
 * Function:    pi_inet_setsockopt
 *
 * Summary:     set options on socket
 *
 * Parameters:  pi_socket*, level, option name, option value, option length
 *
 * Returns:     0 for success, negative otherwise
 *
 ***********************************************************************/
static int
pi_inet_setsockopt(pi_socket_t *ps, int level, int option_name, 
		   const void *option_value, size_t *option_len)
{
	pi_inet_data_t *data = (pi_inet_data_t *)ps->device->data;

	switch (option_name) {
		case PI_DEV_TIMEOUT:
			if (*option_len != sizeof (data->timeout)) {
				errno = EINVAL;
				return pi_set_error(ps->sd, PI_ERR_GENERIC_ARGUMENT);
			}
			memcpy (&data->timeout, option_value,
				sizeof (data->timeout));
			break;
	}

	return 0;
}


/***********************************************************************
 *
 * Function:    pi_inet_close
 *
 * Summary:     Close a connection, destroy the socket
 *
 * Parameters:  pi_socket_t*
 *
 * Returns:     0
 *
 ***********************************************************************/
static int pi_inet_close(pi_socket_t *ps)
{
	/* Close sd handle */
	if (ps->sd) {
		close(ps->sd);
		ps->sd = 0;
	}

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

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
