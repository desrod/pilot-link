/*
 * $Id$
 *
 * linuxusb.c: device i/o for linux usb
 *
 * Copyright (c) 1996, 1997, D. Jeff Dionne & Kenneth Albanowski.
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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "pi-debug.h"
#include "pi-source.h"
#include "pi-usb.h"
#include "pi-error.h"

#ifdef HAVE_SYS_IOCTL_COMPAT_H
#include <sys/ioctl_compat.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifndef O_NONBLOCK
# define O_NONBLOCK 0
#endif


static int u_open(pi_socket_t *ps, struct pi_sockaddr *addr, size_t addrlen);
static int u_close(pi_socket_t *ps);
static int u_write(pi_socket_t *ps, unsigned char *buf, size_t len, int flags);
static int u_read(pi_socket_t *ps, pi_buffer_t *buf, size_t len, int flags);
static int u_poll(pi_socket_t *ps, int timeout);
static int u_flush(pi_socket_t *ps, int flags);

void pi_usb_impl_init (struct pi_usb_impl *impl)
{
	impl->open 		= u_open;
	impl->close 		= u_close;
	impl->write 		= u_write;
	impl->read 		= u_read;
	impl->flush		= u_flush;
	impl->poll 		= u_poll;
	impl->wait_for_device	= NULL;		/* not implemented in linuxusb yet */
	impl->changebaud	= NULL;		/* we don't need this one on linuxusb
						 * as USB serial adapters redirect to serial ports
						 */
	impl->control_request	= NULL;
	impl->interrupt_read = NULL;
}


/***********************************************************************
 *
 * Function:    u_open
 *
 * Summary:     Open the usb port and establish a connection for
 *
 * Parameters:  None
 *
 * Returns:     The file descriptor
 *
 ***********************************************************************/
static int
u_open(pi_socket_t *ps, struct pi_sockaddr *addr, size_t addrlen)
{
	int 	fd, 
		i;
	char 	*tty 	= addr->pi_device;

	if ((fd = open(tty, O_RDWR | O_NONBLOCK)) < 0) {
		ps->last_error = PI_ERR_GENERIC_SYSTEM;
		return PI_ERR_GENERIC_SYSTEM;	/* errno already set */
	}

	if (!isatty(fd)) {
		close(fd);
		errno = EINVAL;
		ps->last_error = PI_ERR_GENERIC_SYSTEM;
		return PI_ERR_GENERIC_SYSTEM;
	}

	if ((i = fcntl(fd, F_GETFL, 0)) != -1) {
		i &= ~O_NONBLOCK;
		fcntl(fd, F_SETFL, i);
	}

	if ((i = pi_socket_setsd(ps, fd)) < 0)
		return i;

	return fd;
}


/***********************************************************************
 *
 * Function:    u_close
 *
 * Summary:     Close the open socket/file descriptor
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int
u_close(pi_socket_t *ps)
{
	LOG((PI_DBG_DEV, PI_DBG_LVL_INFO,
		"DEV CLOSE linuxusb fd: %d\n", ps->sd));

	return close(ps->sd);
}


/***********************************************************************
 *
 * Function:    u_poll
 *
 * Summary:     Poll the open socket/file descriptor
 *
 * Parameters:  None
 *
 * Returns:     1 on success, PI_ERR_SOCK_TIMEOUT on timeout
 *
 ***********************************************************************/
static int
u_poll(pi_socket_t *ps, int timeout)
{
	struct 	timeval t;
	fd_set 	ready;

	FD_ZERO(&ready);
	FD_SET(ps->sd, &ready);

	/* If timeout == 0, wait forever for packet, otherwise wait till
	   timeout milliseconds */
	if (timeout == 0)
		select(ps->sd + 1, &ready, 0, 0, 0);
	else {
		t.tv_sec 	= timeout / 1000;
		t.tv_usec 	= (timeout % 1000) * 1000;
		select(ps->sd + 1, &ready, 0, 0, &t);
	}

	if (!FD_ISSET(ps->sd, &ready)) {
		/* otherwise throw out any current packet and return */
		LOG((PI_DBG_DEV, PI_DBG_LVL_WARN,
			 "DEV POLL linuxusb timeout\n"));
		errno = ETIMEDOUT;
		return pi_set_error(ps->sd, PI_ERR_SOCK_TIMEOUT);
	}

	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG,
		"DEV POLL linuxusb found data on fd: %d\n", ps->sd));

	return 1;
}


/***********************************************************************
 *
 * Function:    u_write
 *
 * Summary:     Write to the open socket/file descriptor
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int
u_write(pi_socket_t *ps, unsigned char *buf, size_t len, int flags)
{
	int 	total,
		nwrote;
	struct 	pi_usb_data *data = (struct pi_usb_data *)ps->device->data;
	struct 	timeval t;
	fd_set 	ready;

	FD_ZERO(&ready);
	FD_SET(ps->sd, &ready);

	total = len;
	while (total > 0) {
		if (data->timeout == 0)
			select(ps->sd + 1, 0, &ready, 0, 0);
		else {
			t.tv_sec 	= data->timeout / 1000;
			t.tv_usec 	= (data->timeout % 1000) * 1000;
			if (select(ps->sd + 1, 0, &ready, 0, &t))
				return pi_set_error(ps->sd, PI_ERR_SOCK_TIMEOUT);
		}

		if (!FD_ISSET(ps->sd, &ready)) {
			ps->state = PI_SOCK_CONN_BREAK;
			return pi_set_error(ps->sd, PI_ERR_SOCK_DISCONNECTED);
		}

		nwrote = write(ps->sd, buf, len);
		if (nwrote < 0) {
			ps->state = PI_SOCK_CONN_BREAK;
			return pi_set_error(ps->sd, PI_ERR_SOCK_DISCONNECTED);
		}

		total -= nwrote;
	}

	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG,
		"DEV TX linuxusb wrote %d bytes\n", len));

	return len;
}


/***********************************************************************
 *
 * Function:    u_read_buf
 *
 * Summary:     read buffer
 *
 * Parameters:  pi_socket_t*, char* to buffer, length of buffer
 *
 * Returns:     number of bytes read
 *
 ***********************************************************************/
static int
u_read_buf (pi_socket_t *ps, pi_buffer_t *buf, size_t len, int flags) 
{
	struct 	pi_usb_data *data = (struct pi_usb_data *)ps->device->data;
	size_t rbuf = data->buf_size;
	
	if (rbuf > len)
		rbuf = len;

	if (pi_buffer_append (buf, data->buf, rbuf) == NULL) {
		errno = ENOMEM;
		return pi_set_error(ps->sd, PI_ERR_GENERIC_MEMORY);
	}

	if (flags != PI_MSG_PEEK) {
		data->buf_size -= rbuf;
		if (data->buf_size > 0)
			memmove(data->buf, &data->buf[rbuf], data->buf_size);
	}

	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG,
		"DEV RX linuxusb read %d bytes from read-ahead buffer\n", rbuf));
	
	return rbuf;
}


/***********************************************************************
 *
 * Function:    u_read
 *
 * Summary:     Read incoming data from the socket/file descriptor
 *
 * Parameters:  pi_socket_t*, char* to buffer, buffer length, flags
 *
 * Returns:     number of bytes read or negative otherwise
 *
 ***********************************************************************/
static int
u_read(pi_socket_t *ps, pi_buffer_t *buf, size_t len, int flags)
{
	ssize_t rbuf = 0,
		bytes;
	struct 	pi_usb_data *data = (struct pi_usb_data *)ps->device->data;
	struct 	timeval t;
	fd_set 	ready;

	/* check whether we have at least partial data in store */
	if (data->buf_size) {
		rbuf = u_read_buf(ps, buf, len, flags);
		if (rbuf < 0)
			return rbuf;
		len -= rbuf;
		if (len == 0)
			return rbuf;
	}

	/* If timeout == 0, wait forever for packet, otherwise wait till
	   timeout milliseconds */
	FD_ZERO(&ready);
	FD_SET(ps->sd, &ready);
	if (data->timeout == 0)
		select(ps->sd + 1, &ready, 0, 0, 0);
	else {
		t.tv_sec 	= data->timeout / 1000;
		t.tv_usec 	= (data->timeout % 1000) * 1000;
		if (select(ps->sd + 1, &ready, 0, 0, &t) == 0) {
			LOG((PI_DBG_DEV, PI_DBG_LVL_WARN,
				"DEV RX linuxusb timeout\n"));
			errno = ETIMEDOUT;
			return pi_set_error(ps->sd, PI_ERR_SOCK_TIMEOUT);
		}
	}

	/* If data is available in time, read it */
	if (FD_ISSET(ps->sd, &ready)) {
		if (flags == PI_MSG_PEEK && len > 256)
			len = 256;

		if (pi_buffer_expect (buf, len) == NULL) {
			errno = ENOMEM;
			return pi_set_error(ps->sd, PI_ERR_GENERIC_MEMORY);
		}

		bytes = read(ps->sd, buf->data + buf->used, len);

		if (bytes > 0) {
			if (flags == PI_MSG_PEEK) {
				memcpy(data->buf + data->buf_size, buf->data + buf->used, bytes);
				data->buf_size += bytes;
			}
			buf->used += bytes;
			rbuf += bytes;

			LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG,
				"DEV RX linuxusb read %d bytes\n", bytes));
		}
	} else {
		LOG((PI_DBG_DEV, PI_DBG_LVL_WARN,
			"DEV RX linuxusb timeout\n"));
		errno = ETIMEDOUT;
		return pi_set_error(ps->sd, PI_ERR_SOCK_TIMEOUT);
	}

	return rbuf;
}

/***********************************************************************
 *
 * Function:    u_flush
 *
 * Summary:	Flush incoming and/or outgoing data from the socket/file
 *		descriptor
 *
 * Parameters:	ps is of type pi_socket that contains the sd member which is
 *              the file descriptor that the data in buf will be read. It
 *              also contains the read buffer.
 *
 *		flags is of type int and can be a combination of
 *		PI_FLUSH_INPUT and PI_FLUSH_OUTPUT
 *
 * Returns:	0
 *
 ***********************************************************************/
static int
u_flush(pi_socket_t *ps, int flags)
{
	char buf[256];
	int fl;
	struct pi_usb_data *data = (struct pi_usb_data *) ps->device->data;

	if (flags & PI_FLUSH_INPUT) {
		/* clear internal buffer */
		data->buf_size = 0;

		/* flush pending data */
		if ((fl = fcntl(ps->sd, F_GETFL, 0)) != -1) {
			fcntl(ps->sd, F_SETFL, fl | O_NONBLOCK);
			while (recv(ps->sd, buf, sizeof(buf), 0) > 0)
				;
			fcntl(ps->sd, F_SETFL, 0);
		}

		LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG,
			"DEV FLUSH linuxusb flushed input buffer\n"));
	}
	return 0;
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
/* ex: set tabstop=4 expandtab: */
/* Local Variables: */
/* indent-tabs-mode: t */
/* c-basic-offset: 8 */
/* End: */
