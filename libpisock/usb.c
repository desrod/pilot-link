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
#include "pi-error.h"

pi_protocol_t *pi_usb_protocol_dup (pi_protocol_t *prot);

static int pi_usb_connect(pi_socket_t *ps, struct sockaddr *addr, 
			     size_t addrlen);
static int pi_usb_bind(pi_socket_t *ps, struct sockaddr *addr,
			  size_t addrlen);
static int pi_usb_listen(pi_socket_t *ps, int backlog);
static int pi_usb_accept(pi_socket_t *ps, struct sockaddr *addr,
			    size_t *addrlen);
static int pi_usb_getsockopt(pi_socket_t *ps, int level, int option_name, 
				void *option_value, size_t *option_len);
static int pi_usb_setsockopt(pi_socket_t *ps, int level, int option_name, 
				const void *option_value, size_t *option_len);
static int pi_usb_close(pi_socket_t *ps);

static int USB_configure_visor (pi_usb_data_t *dev, u_int8_t *input_pipe, u_int8_t *output_pipe);
static int USB_configure_generic (pi_usb_data_t *dev, u_int8_t *input_pipe, u_int8_t *output_pipe);

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
		new_prot->flush		= prot->flush;
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
		prot->flush		= data->impl.flush;
		prot->getsockopt 	= pi_usb_getsockopt;
		prot->setsockopt 	= pi_usb_setsockopt;
		prot->data 		= NULL;
	}
	
	return prot;
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
	
	if (dev != NULL && data != NULL) {

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
pi_usb_connect(pi_socket_t *ps, struct sockaddr *addr, size_t addrlen)
{
	struct 	pi_usb_data *data = (pi_usb_data_t *)ps->device->data;
	struct 	pi_sockaddr *pa = (struct pi_sockaddr *) addr;
	int result;

	result = data->impl.open(ps, pa, addrlen);
	if (result < 0)
		return result;

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
			if ((result = net_tx_handshake(ps)) < 0)
				return result;
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
pi_usb_bind(pi_socket_t *ps, struct sockaddr *addr, size_t addrlen)
{
	struct 	pi_usb_data *data = (pi_usb_data_t *)ps->device->data;
	struct 	pi_sockaddr *pa = (struct pi_sockaddr *) addr;
	int result;

	result = data->impl.open(ps, pa, addrlen);
	if (result < 0)
		return result;

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
 * Returns:     pi_socket descriptor or negative on error
 *
 ***********************************************************************/
static int
pi_usb_accept(pi_socket_t *ps, struct sockaddr *addr, size_t *addrlen)
{
	struct 	pi_usb_data *data = (pi_usb_data_t *)ps->device->data;
	int result;

	/* Wait for data */
	result = data->impl.poll(ps, ps->accept_to);
	if (result < 0) {
		pi_close(ps->sd);
		return result;
	}

	data->timeout = ps->accept_to * 1000;

	pi_socket_init(ps);

	if (ps->type == PI_SOCK_STREAM) {
		switch (ps->cmd) {
			case PI_CMD_CMP:
				if ((result = cmp_rx_handshake(ps, 57600, 1)) < 0)
					return result;
				break;
			case PI_CMD_NET:
				if ((result = net_rx_handshake(ps)) < 0)
					return result;
			break;
		}
		ps->dlprecord = 0;
	}

	data->timeout = 0;
	ps->command = 0;
	ps->state 	= PI_SOCK_CONAC;

	return ps->sd;
}


/***********************************************************************
 *
 * Function:    pi_usb_getsockopt
 *
 * Summary:     get options on USB socket
 *
 * Parameters:  pi_socket*, level, option name, option value, option length
 *
 * Returns:     0 for success, negative otherwise
 *
 ***********************************************************************/
static int
pi_usb_getsockopt(pi_socket_t *ps, int level, int option_name, 
		     void *option_value, size_t *option_len)
{
	pi_usb_data_t *data = (pi_usb_data_t *)ps->device->data;

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
 * Function:    pi_usb_setsockopt
 *
 * Summary:     set options on USB socket
 *
 * Parameters:  pi_socket*, level, option name, option value, option length
 *
 * Returns:     0 for success, negative otherwise
 *
 ***********************************************************************/
static int
pi_usb_setsockopt(pi_socket_t *ps, int level, int option_name, 
		     const void *option_value, size_t *option_len)
{
	pi_usb_data_t *data = (pi_usb_data_t *)ps->device->data;

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

	if (ps->sd != 0)  {
		data->impl.close (ps);
		ps->sd = 0;
	}

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


/*
 * Start of the identification and init code.
 */

/*
 * This is the table of USB devices that we know about, what they are, and
 * some flags.
 * 
 * This table helps us determine whether a connecting USB device is one we'd
 * like to talk to.
 *
 */

pi_usb_dev_t known_devices[] = {
	/* Sony */
	{
		.vendor 	= 0x054c,
		.product 	= 0x0038,
		.idstr 		= "Sony S S320 and other Palm OS 3.5 devices",
		.flags 		= USB_INIT_SONY_CILE,
	},

	{
		.vendor 	= 0x054c,
		.product 	= 0x0066,
		.idstr 		= "Sony T, SJ series, and other Palm OS 4.0 devices",
	},

	{
		.vendor 	= 0x054c,
		.product 	= 0x0095,
		.idstr 		= "Sony S360",
	},

	{
		.vendor 	= 0x054c,
		.product 	= 0x000a,
		.idstr 		= "Sony NR and other Palm OS 4.1 devices",
	},

	{
		.vendor 	= 0x054c,
		.product 	= 0x00da,
		.idstr 		= "Sony NX",
	},

	{
		.vendor 	= 0x054c,
		.product 	= 0x00e9,
		.idstr 		= "Sony NZ",
	},

	{
		.vendor 	= 0x054c,
		.product 	= 0x0144,
		.idstr 		= "Sony UX",
	},

	{
		.vendor 	= 0x054c,
		.product 	= 0x0169,
		.idstr 		= "Sony TJ",
	},

	/* AlphaSmart */
	{
		.vendor 	= 0x081e,
		.product 	= 0xdf00,
		.idstr 		= "Alphasmart Dana",
	},

	/* HANDSPRING (vendor 0x082d) */
	{
		.vendor 	= 0x082d,
		.product 	= 0x0100,
		.idstr 		= "Visor, Treo 300",
		.flags 		= USB_INIT_VISOR,
	},

	{
		.vendor 	= 0x082d,
		.product 	= 0x0200,
		.idstr 		= "Treo",
	},

	{
		.vendor 	= 0x082d,
		.product 	= 0x0300,
		.idstr 		= "Treo 600",
	},

	/* PalmOne, Palm Inc */
	{
		.vendor 	= 0x0830,
		.product 	= 0x0001,
		.idstr 		= "m500",
	},

	{
		.vendor 	= 0x0830,
		.product 	= 0x0002,
		.idstr 		= "m505",
	},

	{
		.vendor 	= 0x0830,
		.product 	= 0x0003,
		.idstr 		= "m515",
	},

	{
		.vendor 	= 0x0830,
		.product 	= 0x0010,
	},

	{
		.vendor 	= 0x0830,
		.product 	= 0x0011,
	},

	{
		.vendor 	= 0x0830,
		.product 	= 0x0020,
		.idstr 		= "i705",
	},

	{
		.vendor 	= 0x0830,
		.product 	= 0x0030,
		.idstr		= "Tungsten|Z",
	},

	{
		.vendor 	= 0x0830,
		.product 	= 0x0031,
		.idstr 		= "Tungsten|W",
	},

	{
		.vendor 	= 0x0830,
		.product 	= 0x0040,
		.idstr 		= "m125",
	},

	{
		.vendor 	= 0x0830,
		.product 	= 0x0050,
		.idstr 		= "m130",
	},

	{
		.vendor 	= 0x0830,
		.product 	= 0x0051,
	},

	{
		.vendor 	= 0x0830,
		.product 	= 0x0052,
	},

	{
		.vendor 	= 0x0830,
		.product 	= 0x0053,
	},

	{
		.vendor 	= 0x0830,
		.product 	= 0x0060,
		.idstr 		= "Tungsten series, Zire 71",
	},

	{
		.vendor 	= 0x0830,
		.product 	= 0x0061,
		.idstr 		= "Zire 31, 72",
	},

	{
		.vendor 	= 0x0830,
		.product 	= 0x0062,
	},

	{
		.vendor 	= 0x0830,
		.product 	= 0x0063,
	},

	{
		.vendor 	= 0x0830,
		.product 	= 0x0070,
		.idstr 		= "Zire",
	},

	{
		.vendor 	= 0x0830,
		.product 	= 0x0071,
	},

	{
		.vendor 	= 0x0830,
		.product 	= 0x0080,
		.idstr 		= "m100",
		.flags 		= USB_INIT_NONE,
	},

	{
		.vendor 	= 0x0830,
		.product 	= 0x0099,
	},

	{
		.vendor 	= 0x0830,
		.product 	= 0x0100,
	},

	/* GARMIN */
	{
		.vendor 	= 0x091e,
		.product 	= 0x0004,
		.idstr 		= "IQUE 3600",
	},

	/* Kyocera */
	{
		.vendor 	= 0x0c88,
		.product 	= 0x0021,
		.idstr 		= "7135 Smartphone",
	},

	{
		.vendor 	= 0x0c88,
		.product 	= 0xa226,
		.idstr 		= "6035 Smartphone",
	},

	/* Tapwave */
	{
		.vendor 	= 0x12ef,
		.product 	= 0x0100,
		.idstr 		= "Zodiac, Zodiac2",
		.flags 		= USB_INIT_TAPWAVE,
	},

	/* ACEECA */
	{
		.vendor 	= 0x4766,
		.product 	= 0x0001,
		.idstr 		= "MEZ1000",
	},

	/* Samsung */
	{
		.vendor 	= 0x04e8,
		.product 	= 0x8001,
		.idstr 		= "i330",
	},
};

static pi_usb_dev_t override_device;

int
USB_check_device (pi_usb_data_t *dev, u_int16_t vendor, u_int16_t product)
{
	unsigned int i;
	if (override_device.vendor) {
		if (override_device.vendor == vendor) {
			if (!override_device.product ||
					override_device.product == product) {
				dev->dev.flags |= override_device.flags;
				return 0;
			}
		}
		return -1;
	}

	for (i = 0; i < (sizeof (known_devices) / sizeof (known_devices[0])); i++) {
		if (known_devices[i].vendor == vendor) {
			if (!known_devices[i].product ||
					known_devices[i].product == product) {
				dev->dev.flags |= override_device.flags;
				return 0;
			}
		}
	}

	return -1;
}


/*
 * Device configuration, ugh.
 */

int
USB_configure_device (pi_usb_data_t *dev, u_int8_t *input_pipe, u_int8_t *output_pipe)
{
	int ret;
	u_int32_t flags = dev->dev.flags;

	*input_pipe = 0xff;
	*output_pipe = 0xff;

	/*
	 * Device specific magic incantations
	 *
	 * Many devices agree on talking only if you say the "magic" incantation first.
	 * Usually, it's a control request or a sequence of control requests
	 *
	 */

	if (flags & USB_INIT_NONE)
		return 0;
	else if (flags & USB_INIT_VISOR)
		ret = USB_configure_visor (dev, input_pipe, output_pipe);
	else if (flags & USB_INIT_SONY_CILE) {
		/* according to linux code, PEG S-300 awaits these two requests */
		/* USB_REQ_GET_CONFIGURATION */
		ret = dev->impl.control_request (dev, 0x80, 0x08, 0, 0, NULL, 1, 0);
		if (ret < 0) {
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "usb: Sony USB_REQ_GET_CONFIGURATION failed (err=%08x)\n", ret));
		}
		/* USB_REQ_GET_INTERFACE */
		ret = dev->impl.control_request (dev, 0x80, 0x0A, 0, 0, NULL, 1, 0);
		if (ret < 0) {
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "usb: Sony USB_REQ_GET_INTERFACE failed (err=%08x)\n", ret));
		}
	} else {
		/* other devices will either accept or deny this generic call */
		ret = USB_configure_generic (dev, input_pipe, output_pipe);
	}

	/* query bytes available. Not that we really care,
	   but most devices expect to receive this before
	   they agree on talking to us. */
	if (!(flags & USB_INIT_TAPWAVE)) {
		unsigned char ba[2] = { 0 };

		ret = dev->impl.control_request (dev, 0xc2, GENERIC_REQUEST_BYTES_AVAILABLE, 0, 0, &ba[0], 2, 0);
		if (ret < 0) {
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "usb: GENERIC_REQUEST_BYTES_AVAILABLE failed (err=%08x)\n", ret));
		}
		LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "GENERIC_REQUEST_BYTES_AVAILABLE returns 0x%02x%02x\n", ba[0], ba[1]));
	}

    return 0;
}

static int
USB_configure_visor (pi_usb_data_t *dev, u_int8_t *input_pipe, u_int8_t *output_pipe)
{
	int i, ret;
	visor_connection_info_t ci;

	ret = dev->impl.control_request (dev, 0xc2, VISOR_GET_CONNECTION_INFORMATION, 0, 0, &ci, sizeof (ci), 0);
	if (ret < 0) {
		LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "usb: VISOR_GET_CONNECTION_INFORMATION failed (err=%08x)\n", ret));
	} else {
		LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "usb: VISOR_GET_CONNECTION_INFORMATION, num_ports=%d\n", ci.num_ports));
		if (ci.num_ports > 2)
			ci.num_ports = 2;
		for (i=0; i < ci.num_ports; i++)
		{
			char *function_str;
			switch (ci.connections[i].port_function_id)
			{
				case VISOR_FUNCTION_GENERIC:
					function_str="GENERIC";
					break;
				case VISOR_FUNCTION_DEBUGGER:
					function_str="DEBUGGER";
					break;
				case VISOR_FUNCTION_HOTSYNC:
					function_str="HOTSYNC";
					break;
				case VISOR_FUNCTION_CONSOLE:
					function_str="CONSOLE";
					break;
				case VISOR_FUNCTION_REMOTE_FILE_SYS:
					function_str="REMOTE_FILE_SYSTEM";
					break;
				default:
					function_str="UNKNOWN";
					break;
			}
			LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "\t[%d] port_function_id=0x%02x (%s)\n", i, ci.connections[i].port_function_id, function_str));
			LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "\t[%d] port=%d\n", i, ci.connections[i].port));
		}
	}
	return ret;
}

static int
USB_configure_generic (pi_usb_data_t *dev, u_int8_t *input_pipe, u_int8_t *output_pipe)
{
	int i, ret;
	palm_ext_connection_info_t ci;
	u_int32_t flags = dev->dev.flags;

	ret = dev->impl.control_request (dev, 0xc2, PALM_GET_EXT_CONNECTION_INFORMATION, 0, 0, &ci, sizeof (ci), 0);
	if (ret < 0) {
		LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "usb: PALM_GET_EXT_CONNECTION_INFORMATION failed (err=%08x)\n", ret));
	} else {
		LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "usb: PALM_GET_EXT_CONNECTION_INFORMATION, num_ports=%d, endpoint_numbers_different=%d\n", ci.num_ports, ci.endpoint_numbers_different));
		for (i=0; i < ci.num_ports; i++) {
			LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "\t[%d] port_function_id='%c%c%c%c'\n", i, ci.connections[i].port_function_id[0], ci.connections[i].port_function_id[1], ci.connections[i].port_function_id[2], ci.connections[i].port_function_id[3]));
			LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "\t[%d] port=%d\n", i, ci.connections[i].port));
			LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "\t[%d] endpoint_info=%d\n", i, ci.connections[i].endpoint_info));
			if (!memcmp(ci.connections[i].port_function_id, "cnys", 4)) {

				/* 'sync': we found the pipes to use for synchronization force
				   find_interfaces to select this one rather than another one */
				if (ci.endpoint_numbers_different) {
					if (input_pipe)
						*input_pipe = ci.connections[i].endpoint_info >> 4;
					if (output_pipe)
						*output_pipe = ci.connections[i].endpoint_info & 0x0f;
				} else {
					if (input_pipe)
						*input_pipe = ci.connections[i].port;
					if (output_pipe)
						*output_pipe = ci.connections[i].port;
				}
			}
		}
	}

	if (flags & USB_INIT_TAPWAVE) {
		/*
		 * Tapwave: for Zodiac, the TwUSBD.sys driver on Windows sends
		 * the ext-connection-info packet two additional times.
		 */
		ret = dev->impl.control_request (dev, 0xc2, PALM_GET_EXT_CONNECTION_INFORMATION, 0, 0, &ci, sizeof (ci), 0);
		ret = dev->impl.control_request (dev, 0xc2, PALM_GET_EXT_CONNECTION_INFORMATION, 0, 0, &ci, sizeof (ci), 0);
	}
	return ret;
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
