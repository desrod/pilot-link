/*
 * bluetooth.c: Interface layer to bluetooth HotSync connections
 * 		using the Bluez stack (http://www.bluez.org/)
 *
 * Copyright (c) 1996, 1997, D. Jeff Dionne & Kenneth Albanowski
 * Copyright (c) 1999, Tilo Christ
 * Copyright (c) 2005, Florent Pillet
 * Copyright (c) 2004, Paul Evans <leonerd@leonerd.org.uk>
 * Copyright (c) 2005, Bastien Nocera <hadess@hadess.net>
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
#include <sys/time.h>		/* Needed for Redhat 6.x machines */

#include <sys/socket.h>
#include <sys/ioctl.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "pi-debug.h"
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-bluetooth.h"
#include "pi-cmp.h"
#include "pi-net.h"
#include "pi-error.h"
#include "pi-serial.h"
#include "pi-util.h"

#ifdef OS2
#include <sys/select.h>
#endif

/* Declare prototypes */
static int pi_bluetooth_open(struct pi_socket *ps, struct pi_sockaddr *addr,
	int addrlen);
static int pi_bluetooth_connect(pi_socket_t *ps, struct sockaddr *addr, 
	size_t addrlen);
static int pi_bluetooth_bind(pi_socket_t *ps, struct sockaddr *addr,
	size_t addrlen);
static int pi_bluetooth_listen(pi_socket_t *ps, int backlog);
static int pi_bluetooth_accept(pi_socket_t *ps, struct sockaddr *addr,
	size_t *addrlen);
static int pi_bluetooth_getsockopt(pi_socket_t *ps, int level,
	int option_name, void *option_value,
	size_t *option_len);
static int pi_bluetooth_setsockopt(pi_socket_t *ps, int level,
	int option_name, const void *option_value,
	size_t *option_len);
static int pi_bluetooth_close(pi_socket_t *ps);
static int pi_bluetooth_flush(pi_socket_t *ps, int flags);
static ssize_t pi_bluetooth_write(struct pi_socket *ps,
	const unsigned char *buf, size_t len, int flags);
static ssize_t pi_bluetooth_read(struct pi_socket *ps,
	pi_buffer_t *buf, size_t len, int flags);

extern int pi_socket_init(pi_socket_t *ps);

/* SDP Helper functions */
/*
 *  Copyright (C) 2004  Edd Dumbill <edd@usefulinc.com>
 *  Copyright (C) 2005  Bastien Nocera <hadess@hadess.net>
 *  Mostly borrowed from BlueZ
 *
 *  Copyright (C) 2001-2002  Nokia Corporation
 *  Copyright (C) 2002-2003  Maxim Krasnyansky <maxk@qualcomm.com>
 *  Copyright (C) 2002-2005  Marcel Holtmann <marcel@holtmann.org>
 *  Copyright (C) 2002-2003  Stephen Crane <steve.crane@rococosoft.com>
 *  Copyright (C) 2002-2003  Jean Tourrilhes <jt@hpl.hp.com>
 *
 */
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

typedef struct {
	char *name;
	char *provider;
	char *desc;

	unsigned int class;
	unsigned int profile;
	unsigned int channel;
} svc_info_t;

#define HOTSYNC_CHANNEL 22

static unsigned char hotsync_uuid[] = {
	0xD8, 0x0C, 0xF9, 0xEA, 0x13, 0x4C, 0x11, 0xD5,
	0x83, 0xCE, 0x00, 0x30, 0x65, 0x7C, 0x54, 0x3C
};

static int
add_hotsync(sdp_session_t *session,
		  sdp_record_t *rec,
		  bdaddr_t *interface,
		  svc_info_t *si,
		  uint32_t *handle,
		  int *channel_ret)
{
	sdp_record_t record;
	sdp_list_t *root, *svclass, *proto;
	uuid_t root_uuid, svclass_uuid, l2cap_uuid, rfcomm_uuid;
	uint8_t channel = HOTSYNC_CHANNEL;

	memset(&record, 0, sizeof(record));
	record.handle = 0xffffffff;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(NULL, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto = sdp_list_append(NULL, sdp_list_append(NULL, &l2cap_uuid));

	sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
	proto = sdp_list_append(proto, sdp_list_append(
	sdp_list_append(NULL, &rfcomm_uuid), sdp_data_alloc(SDP_UINT8, &channel)));

	sdp_set_access_protos(&record, sdp_list_append(NULL, proto));

	sdp_uuid128_create(&svclass_uuid, (void *) hotsync_uuid);
	svclass = sdp_list_append(NULL, &svclass_uuid);
	sdp_set_service_classes(&record, svclass);

	sdp_set_info_attr(&record, "PalmOS HotSync", NULL, NULL);

	if (sdp_device_record_register(session, interface, &record, 0) < 0) {
		LOG((PI_DBG_DEV, PI_DBG_LVL_ERR,
			"bluetooth: service record registration failed\n"));
		return -1;
	}

	*handle = record.handle;
	*channel_ret = HOTSYNC_CHANNEL;
	return 0;
}

static int
register_sdp (uint32_t *handle, int *channel, sdp_session_t **sess)
{
	svc_info_t si;
	bdaddr_t interface;

	bacpy (&interface, BDADDR_ANY);
	*sess = sdp_connect (&interface, BDADDR_LOCAL, 0);

	if (*sess == NULL)
		return -1;
	memset (&si, 0, sizeof(si));
	si.name = "HOTSYNC";
	return add_hotsync (*sess, 0, &interface, &si, handle, channel);
}

static int
deregister_sdp(uint32_t handle, sdp_session_t *sess)
{
	uint32_t range = 0x0000ffff;
	sdp_list_t *attr;
	sdp_record_t *rec;
	bdaddr_t interface;

	if (!sess)
		return 0;

	bacpy(&interface, BDADDR_ANY);

	attr = sdp_list_append(0, &range);
	rec = sdp_service_attr_req(sess, handle, SDP_ATTR_REQ_RANGE, attr);
	sdp_list_free(attr, 0);
	if (!rec) {
		LOG((PI_DBG_DEV, PI_DBG_LVL_ERR,
			"bluetooth: service record not found\n"));
		sdp_close(sess);
		return -1;
	}
	if (sdp_record_unregister(sess, rec)) {
		LOG((PI_DBG_DEV, PI_DBG_LVL_ERR,
			"bluetooth: failed to unregister service record: %s\n",
			strerror(errno)));
		sdp_close(sess);
		return -1;
	}
	LOG((PI_DBG_DEV, PI_DBG_LVL_INFO, "bluetooth: service record deleted\n"));
	sdp_close(sess);
	return 0;
}

/* Protocol Functions */
/***********************************************************************
 *
 * Function:    pi_bluetooth_protocol_dup
 *
 * Summary:     clones an existing pi_protocol struct
 *
 * Parameters:  pi_protocol*
 *
 * Returns:     pi_protocol_t* or NULL if operation failed
 *
 ***********************************************************************/
static pi_protocol_t*
pi_bluetooth_protocol_dup (pi_protocol_t *prot)
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
		new_prot->flush		= prot->flush;
		new_prot->getsockopt 	= prot->getsockopt;
		new_prot->setsockopt 	= prot->setsockopt;
		new_prot->data 		= NULL;
	}

	return new_prot;
}

/***********************************************************************
 *
 * Function:    pi_bluetooth_protocol_free
 *
 * Summary:     frees an existing pi_protocol struct
 *
 * Parameters:  pi_protocol*
 *
 * Returns:     void
 *
 ***********************************************************************/
static void
pi_bluetooth_protocol_free (pi_protocol_t *prot)
{
	ASSERT (prot != NULL);
	if (prot != NULL)
		free(prot);
}

/***********************************************************************
 *
 * Function:    pi_bluetooth_protocol
 *
 * Summary:     creates and inits pi_protocol struct instance
 *
 * Parameters:  pi_device_t*
 *
 * Returns:     pi_protocol_t* or NULL if operation failed
 *
 ***********************************************************************/
static pi_protocol_t*
pi_bluetooth_protocol (pi_device_t *dev)
{	
	pi_protocol_t *prot;
	struct pi_bluetooth_data *data;

	ASSERT (dev != NULL);

	prot = (pi_protocol_t *) malloc(sizeof (pi_protocol_t));

	data = (struct pi_bluetooth_data *)(dev->data);

	if (prot != NULL) {
		prot->level 		= PI_LEVEL_DEV;
		prot->dup 		= pi_bluetooth_protocol_dup;
		prot->free 		= pi_bluetooth_protocol_free;
		prot->read 		= pi_bluetooth_read;
		prot->write 		= pi_bluetooth_write;
		prot->flush		= pi_bluetooth_flush;
		prot->getsockopt 	= pi_bluetooth_getsockopt;
		prot->setsockopt 	= pi_bluetooth_setsockopt;
		prot->data 		= NULL;
	}

	return prot;
}

/* Device Functions */
/***********************************************************************
 *
 * Function:    pi_bluetooth_register_hotsync_sdp
 *
 * Summary:     Gets on which channel the HOTSYNC service is, and
 *              registers it if necessary
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 **********************************************************************/
static void
pi_bluetooth_register_hotsync_sdp (struct pi_bluetooth_data *data)
{
	if (register_sdp (&data->handle, &data->channel, &data->sess) < 0) {
		data->channel = -1;
		data->handle = 0;
		data->sess = NULL;
	}
}

/***********************************************************************
 *
 * Function:    pi_bluetooth_unregister_hotsync_sdp
 *
 * Summary:     Removes the HOTSYNC service
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 **********************************************************************/
static void
pi_bluetooth_unregister_hotsync_sdp (struct pi_bluetooth_data *data)
{
	if (data->handle != 0)
		deregister_sdp (data->handle, data->sess);
}

/***********************************************************************
 *
 * Function:    pi_bluetooth_device_free
 *
 * Summary:     frees an existing pi_device struct
 *
 * Parameters:  pi_device_t*
 *
 * Returns:     void
 *
 ***********************************************************************/
static void
pi_bluetooth_device_free (pi_device_t *dev) 
{
	struct pi_bluetooth_data *data;

	ASSERT (dev != NULL);

	if (dev->data) {
		data = dev->data;
		pi_bluetooth_unregister_hotsync_sdp (data);
		if (data->device)
		free (data->device);
		free(dev->data);
	}
	free(dev);
}

/***********************************************************************
 *
 * Function:    pi_bluetooth_open
 *
 * Summary:     Open a new BlueTooth RFCOMM socket
 *
 * Parameters:  None
 *
 * Returns:    A negative number on error, 0 otherwise
 *
 **********************************************************************/
static int
pi_bluetooth_open(struct pi_socket *ps, struct pi_sockaddr *addr, int addrlen)
{
	int     fd, flags;
	struct  pi_bluetooth_data *data = ps->device->data;

	pi_bluetooth_register_hotsync_sdp (data);
	/* If we didn't manage to create an SDP record, force channel 22 and try
	 * again */
	if (data->channel < 0) {
		fprintf (stderr, "didn't manage to get a channel\n");
		data->channel = 22;
	}

	if ((fd = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM)) < 0)
		return -1;

	if ((flags = fcntl(fd, F_GETFL, 0)) != -1) {
		flags &= ~O_NONBLOCK;
		fcntl(fd, F_SETFL, flags);
	}

	if (pi_socket_setsd(ps, fd) < 0)
		return -1;
	return 0;
}

/***********************************************************************
 *
 * Function:    pi_bluetooth_device
 *
 * Summary:     creates and inits pi_device struct instance 
 *
 * Parameters:  device type
 *
 * Returns:     pi_device_t* or NULL if operation failed
 *
 ***********************************************************************/
pi_device_t*
pi_bluetooth_device (int type)
{
	pi_device_t *dev;
	struct 	pi_bluetooth_data *data;

	dev = (pi_device_t *) malloc(sizeof (pi_device_t));
	if (dev == NULL)
		return NULL;

	data = (struct pi_bluetooth_data *) malloc(sizeof (struct pi_bluetooth_data));
	if (data == NULL) {
		free(dev);
		return NULL;
	}

	dev->free       = pi_bluetooth_device_free;
	dev->protocol   = pi_bluetooth_protocol;	
	dev->bind       = pi_bluetooth_bind;
	dev->listen     = pi_bluetooth_listen;
	dev->accept     = pi_bluetooth_accept;
	dev->connect    = pi_bluetooth_connect;
	dev->close      = pi_bluetooth_close;

	data->timeout   = 0;
	dev->data       = data;

	return dev;
}

/***********************************************************************
 *
 * Function:    pi_bluetooth_connect
 *
 * Summary:     Connect socket to a given address
 *
 * Parameters:  pi_socket*, sockaddr*, size_t
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
static int
pi_bluetooth_connect(pi_socket_t *ps, struct sockaddr *addr,
	size_t addrlen)
{
	ps->raddr       = malloc(addrlen);
	memcpy(ps->raddr, addr, addrlen);
	ps->raddrlen    = addrlen;

	ps->laddr       = malloc(addrlen);
	memcpy(ps->laddr, addr, addrlen);
	ps->laddrlen    = addrlen;

	if ((	ps->type == PI_SOCK_STREAM &&
	    	ps->cmd  == PI_CMD_CMP) &&
	    	cmp_tx_handshake(ps) < 0)
	{
		pi_close(ps->sd);
		return -1;
	}
	ps->state = PI_SOCK_CONN_INIT;
	ps->command = 0;
	return 0;
}

/***********************************************************************
 *
 * Function:    pi_bluetooth_bind
 *
 * Summary:     Bind address to a local socket
 *
 * Parameters:  pi_socket*, sockaddr*, size_t
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
static int
pi_bluetooth_bind(pi_socket_t *ps, struct sockaddr *addr, size_t addrlen)
{
	struct pi_sockaddr *pa = (struct pi_sockaddr *) addr;
	struct pi_bluetooth_data *data =
		(struct pi_bluetooth_data *)ps->device->data;
	char *device = pa->pi_device;
	struct sockaddr_rc bindaddr;

	if (pi_bluetooth_open(ps, pa, addrlen) == -1)
		return -1;      /* errno already set */

	data->device = strdup (device);

	bindaddr.rc_family = AF_BLUETOOTH;
	bacpy(&bindaddr.rc_bdaddr, BDADDR_ANY);
	bindaddr.rc_channel = data->channel;

	if (bind(ps->sd, (struct sockaddr *)&bindaddr, sizeof(bindaddr)) < 0) {
		pi_set_error(ps->sd, PI_ERR_GENERIC_SYSTEM);
		return -1;
	}

	ps->laddr       = malloc(addrlen);
	memcpy(ps->laddr, addr, addrlen);
	ps->laddrlen    = addrlen;

    	return 0;
}

/***********************************************************************
 *
 * Function:    pi_bluetooth_listen
 *
 * Summary:     Prepare for incoming connections
 *
 * Parameters:  pi_socket*, backlog
 *
 * Returns:     0 for success, negative otherwise
 *
 ***********************************************************************/
static int pi_bluetooth_listen(pi_socket_t *ps, int backlog)
{
	// FIXME: no result is actually being taken into account
    	listen(ps->sd, 10);
    	ps->state = PI_SOCK_LISTEN;
    	return 0;
}

/***********************************************************************
 *
 * Function:    pi_bluetooth_accept_rfcomm
 *
 * Summary:     Accept an incoming connection and associate it with an
 *              RFCOMM serial device
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int
pi_bluetooth_accept_rfcomm(struct pi_socket *ps, const char *device, struct sockaddr_rc *peeraddr, unsigned int *peeraddrlen)
{
	int rfcommfd;

	rfcommfd = accept(ps->sd, (struct sockaddr *)peeraddr, peeraddrlen);
	if (rfcommfd < 0) {
		pi_set_error(ps->sd, PI_ERR_GENERIC_SYSTEM);
		return -1;
	}

	/* Check whether we need to really accept this connection */
 	if (device[0] != '\0' && strcmp (device, "any") != 0) {
		bdaddr_t tmp;
		if (str2ba (device, &tmp) < 0) {
			close (rfcommfd);
			return -1;
		}
		if (bacmp (&tmp, &(peeraddr->rc_bdaddr)) != 0) {
			close (rfcommfd);
			return -1;
		}
	}

	return rfcommfd;
}

/***********************************************************************
 *
 * Function:    pi_bluetooth_accept
 *
 * Summary:     Accept an incoming connection
 *
 * Parameters:  pi_socket*, sockaddr*
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int
pi_bluetooth_accept(pi_socket_t *ps, struct sockaddr *addr,
	size_t *addrlen)
{
	struct pi_bluetooth_data *data =
		(struct  pi_bluetooth_data *)ps->device->data;
	struct sockaddr_rc peeraddr;
	unsigned int peeraddrlen = sizeof(peeraddr);
	int sd;

	/* Wait for data */
	sd = pi_bluetooth_accept_rfcomm(ps, data->device, &peeraddr, &peeraddrlen);
	if (sd < 0) {
		pi_set_error(ps->sd, PI_ERR_GENERIC_SYSTEM);
		return -1;
	}

	if (pi_socket_setsd(ps, sd) < 0)
		return -1;
	pi_socket_init(ps);

	if (ps->type == PI_SOCK_STREAM) {
		switch (ps->cmd) {
		case PI_CMD_CMP:
			if (cmp_rx_handshake(ps, 0, 0) < 0)
				return -1;
			break;
		case PI_CMD_NET:
			if (net_rx_handshake(ps) < 0)
				return -1;
			break;
		}

		ps->dlprecord = 0;
	}

	data->timeout = 0;
	ps->command = 0;
	ps->state = PI_SOCK_CONN_ACCEPT;

	return ps->sd;
}

static int
pi_bluetooth_flush(pi_socket_t *ps, int flags)
{
	char buf[256];
	int fl;

	if (flags & PI_FLUSH_INPUT) {
		if ((fl = fcntl(ps->sd, F_GETFL, 0)) != -1) {
			fcntl(ps->sd, F_SETFL, fl | O_NONBLOCK);
			while (read(ps->sd, buf, sizeof(buf)) > 0)
				;
			fcntl(ps->sd, F_SETFL, fl);
		}
	}
	return 0;
}

static ssize_t
pi_bluetooth_read(struct pi_socket *ps, pi_buffer_t *buf, size_t len, int flags)
{
	int     rbuf;
	struct  pi_bluetooth_data *data = (struct pi_bluetooth_data *)ps->device->data;
	struct  timeval t;
	fd_set  ready;

	if (pi_buffer_expect (buf, len) == NULL)
		return pi_set_error(ps->sd, PI_ERR_GENERIC_MEMORY);

	FD_ZERO(&ready);
	FD_SET(ps->sd, &ready);

	/* If timeout == 0, wait forever for packet, otherwise wait till
	   timeout milliseconds */
	if (data->timeout == 0)
		select(ps->sd + 1, &ready, 0, 0, 0);
	else {
		t.tv_sec        = data->timeout / 1000;
		t.tv_usec       = (data->timeout % 1000) * 1000;
		select(ps->sd + 1, &ready, 0, 0, &t);
	}

    	/* If data is available in time, read it */
    	if (FD_ISSET(ps->sd, &ready)) {
		if (flags == PI_MSG_PEEK && len > 256)
	    		len = 256;
		rbuf = read(ps->sd, buf->data + buf->used, len);
    	} else {
		LOG((PI_DBG_DEV, PI_DBG_LVL_WARN, "DEV RX BlueTooth timeout\n"));
		pi_set_error(ps->sd, PI_ERR_SOCK_TIMEOUT);
		return -1;
    	}
    	buf->used += rbuf;

    	LOG((PI_DBG_DEV, PI_DBG_LVL_INFO, "DEV RX bluetooth %d bytes\n", rbuf));

    	return rbuf;
}

static ssize_t
pi_bluetooth_write(struct pi_socket *ps, const unsigned char *buf, size_t len, int flags)
{
    	int     total,
	    	nwrote;
    	struct  pi_bluetooth_data *data = (struct pi_bluetooth_data *)ps->device->data;
    	struct  timeval t;
    	fd_set  ready;

    	FD_ZERO(&ready);
    	FD_SET(ps->sd, &ready);

    	total = len;
    	while (total > 0) {
		if (data->timeout == 0)
	    		select(ps->sd + 1, 0, &ready, 0, 0);
		else {
	    		t.tv_sec        = data->timeout / 1000;
	    		t.tv_usec       = (data->timeout % 1000) * 1000;
	    		select(ps->sd + 1, 0, &ready, 0, &t);
		}
		if (!FD_ISSET(ps->sd, &ready)) {
	    		pi_set_error(ps->sd, PI_ERR_SOCK_IO);
	    		return -1;
		}
		nwrote = write(ps->sd, buf, len);
		if (nwrote < 0) {
	    		pi_set_error(ps->sd, PI_ERR_SOCK_IO);
	    		return -1;
		}
		total -= nwrote;
    	}

    	LOG((PI_DBG_DEV, PI_DBG_LVL_INFO, "DEV TX bluetooth %d bytes\n", len));
    	return len;
}

/***********************************************************************
 *
 * Function:    pi_bluetooth_getsockopt
 *
 * Summary:     get options on socket
 *
 * Parameters:  pi_socket*, level, option name, option value, option length
 *
 * Returns:     0 for success, negative otherwise
 *
 ***********************************************************************/
static int
pi_bluetooth_getsockopt(pi_socket_t *ps, int level, int option_name, 
	void *option_value, size_t *option_len)
{
	struct pi_bluetooth_data *data =
		(struct pi_bluetooth_data *)ps->device->data;

	if (option_name == PI_DEV_TIMEOUT) {
		if (*option_len < sizeof (data->timeout)) {
		    pi_set_error(ps->sd, PI_ERR_GENERIC_ARGUMENT);
		    return -1;
		}
		memcpy (option_value, &data->timeout, sizeof (data->timeout));
		*option_len = sizeof (data->timeout);
	}

	return 0;
}

/***********************************************************************
 *
 * Function:    pi_bluetooth_setsockopt
 *
 * Summary:     set options on socket
 *
 * Parameters:  pi_socket*, level, option name, option value, option length
 *
 * Returns:     0 for success, negative otherwise
 *
 ***********************************************************************/
static int
pi_bluetooth_setsockopt(pi_socket_t *ps, int level, int option_name, 
	const void *option_value, size_t *option_len)
{
	struct pi_bluetooth_data *data =
		(struct pi_bluetooth_data *)ps->device->data;

	if (option_name == PI_DEV_TIMEOUT) {
		if (*option_len != sizeof (data->timeout)) {
		    pi_set_error(ps->sd, PI_ERR_GENERIC_ARGUMENT);
		    return -1;
		}
		memcpy (&data->timeout, option_value, sizeof (data->timeout));
	}
	return 0;
}

/***********************************************************************
 *
 * Function:    pi_bluetooth_close
 *
 * Summary:     Close a connection, destroy the socket
 *
 * Parameters:  pi_socket*
 *
 * Returns:     always 0 for success
 *
 ***********************************************************************/
static int pi_bluetooth_close(pi_socket_t *ps)
{
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
/* Local Variables: */
/* indent-tabs-mode: t */
/* c-basic-offset: 8 */
/* End: */
