/*
 * freebsdusb.c: device IO for FreeBSD usb devices
 *
 * Copyright (c) 1996, 1997, D. Jeff Dionne & Kenneth Albanowski.
 *               2002, Anish Mistry
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>		/* Needed for Redhat 6.x machines */
#include <fcntl.h>
#include <string.h>

#include "pi-debug.h"
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-usb.h"

#ifdef HAVE_SYS_IOCTL_COMPAT_H
#include <sys/ioctl_compat.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifndef O_NONBLOCK
# define O_NONBLOCK 0
#endif

#if defined(__FreeBSD__)
/* freebsd usb header */
#include <dev/usb/usb.h>
#define MAX_BUF 256
#endif

/* Declare prototypes */
static int u_open(pi_socket_t *ps, struct pi_sockaddr *addr,
	int addrlen);
static int u_close(pi_socket_t *ps);
static int u_write(pi_socket_t *ps, unsigned char *buf,
	int len, int flags);
static int u_read(pi_socket_t *ps, unsigned char *buf, int len,
	int flags);
static int u_poll(pi_socket_t *ps, int timeout);

void pi_usb_impl_init (struct pi_usb_impl *impl)
{
	impl->open 		= u_open;
	impl->close 		= u_close;
	impl->write 		= u_write;
	impl->read 		= u_read;
	impl->poll 		= u_poll;
}


/***********************************************************************
 *
 * Function:    u_open
 *
 * Summary:     Open the usb port and establish a connection for
 *		communicating over the usb port.
 *
 * Parameters:  ps is of type pi_socket_t which will given a copy of the
 *              valid file descriptor that is returned by the function.
 * 
 * 		addr of type pi_socketaddr contains the member pi_device
 *              which is a character string of the usb device.
 *
 *		XXX addrlen is of type int contain the size of addr?  I'm
 *		not using this, should I?  Possible buffer overflow because
 *		I'm not checking something?
 *
 * Returns:     The file descriptor
 *
 ***********************************************************************/
static int
u_open(pi_socket_t *ps, struct pi_sockaddr *addr, int addrlen)
{
	int 	fd,
		i,
		endpoint_fd;

	struct 	usb_device_info udi;
	struct 	usb_ctl_request ur; 
	unsigned char usbresponse[50];

	/* The +4 is to remove the "usb:" part from the string so you are
	   just left with "/dev/ugenX" */
	char 	*tty 		= addr->pi_device + 4;
	char 	*pEndPoint 	= NULL;

	/* open the usb device */
	if ((fd = open(tty, O_RDWR, 0)) == -1) {
		return -1;	/* errno already set */
	}

	/* check for a valid file descriptor */
	if (fd < 0) {
		return -1;
	}

	/* fill the udi structure with information about the handheld, after
	   this is where you will probably want to do device specific stuff
	   */

	if (ioctl(fd, USB_GET_DEVICEINFO, &udi)) {
		(void) close(fd);
		return -1;
	}

	LOG((PI_DBG_DEV, PI_DBG_LVL_INFO,
		"DEV USB_GET_DEVICE_INFO USB FreeBSD fd: %d\n", fd));

	/* set the configuration */
	i = 1;
	if (ioctl(fd, USB_SET_CONFIG, &i) < 0) {
		LOG((PI_DBG_DEV, PI_DBG_LVL_ERR,
		 "DEV USB_SET_CONFIG USB FreeBSD fd: %d failed\n", fd));

		close(fd);
		return -1;
	}

	/* close the main communication pipe since we have initilized
	  everything we needed to
	NOTE: we HAVE to do all this stuff to the main pipe or we will
	 cause a kernel panic when data is sent over the endpoint */
	(void) close(fd);

	/* open endpoint */
	/* allocate data for the usb endpoint string */
	pEndPoint = malloc(strlen(tty)+20);
	if(!pEndPoint)
		return -1;
	/* create device endpoint name string */
	sprintf(pEndPoint, "%s.%d", tty, 2);

	/* open the endpoint with read write access */
	endpoint_fd = open(pEndPoint, O_RDWR, 0);
	if(endpoint_fd < 0) {
		/* we failed to open the endpoint */
		free(pEndPoint);
		return -1;
	}

	if ((i = fcntl(endpoint_fd, F_GETFL, 0)) != -1) {
		i &= ~O_NONBLOCK;
		fcntl(endpoint_fd, F_SETFL, i);
	}

	/* set short transfer so that we can allow "short" reads since we
	   will don't know exactly
	what is coming so we can't specify exact byte amounts */
	i = 1;
	if (ioctl(endpoint_fd, USB_SET_SHORT_XFER, &i) < 0) {
		LOG((PI_DBG_DEV, PI_DBG_LVL_WARN,
		 "DEV USB_SET_SHORT_XFER USB FreeBSD fd: %d failed\n",
			endpoint_fd));
	}

	/* 0 timeout value will cause us the wait until the device has data
           available or is disconnected */
	i = 0;
	if (ioctl(endpoint_fd, USB_SET_TIMEOUT, &i) < 0) {
		LOG((PI_DBG_DEV, PI_DBG_LVL_WARN,
		 "DEV USB_SET_TIMEOUT USB FreeBSD fd: %d failed\n",
			endpoint_fd));
	}

	/* save our file descriptor in the pi_socket structure */
	if (pi_socket_setsd(ps, endpoint_fd) < 0) {
		free(pEndPoint);
		return -1;
	}

	/* free endpoint string memory */
	free(pEndPoint);

	LOG((PI_DBG_DEV, PI_DBG_LVL_INFO,
		"DEV OPEN USB FreeBSD fd: %d\n", endpoint_fd));

	/* return our endpoint file descriptor since this is where the
	   reading and writing will be done */
	return endpoint_fd;

}

/***********************************************************************
 *
 * Function:    u_close
 *
 * Summary:     Close the open socket/file descriptor
 *
 * Parameters:  ps is of type pi_socket that contains the member sd which is
 *              the file descriptor that is to be closed.
 *
 * Returns:     0 if sucessful or -1 if it failed and errno set to the error
 *
 ***********************************************************************/
static int
u_close(pi_socket_t *ps)
{
	LOG((PI_DBG_DEV, PI_DBG_LVL_INFO,
		"DEV CLOSE USB FreeBSD fd: %d\n", ps->sd));

	return close(ps->sd);
}

/***********************************************************************
 *
 * Function:    u_poll
 *
 * Summary:     Poll the open socket/file descriptor
 *
 * Parameters:	None
 *
 * Returns:     0 always since we buffer our own data and you can't poll()
 *		FreeBSD USB devices?
 *
 ***********************************************************************/
static int
u_poll(pi_socket_t *ps, int timeout)
{
	/* stub this function and log an error that this should never needed
	   to be called */
	LOG((PI_DBG_DEV, PI_DBG_LVL_WARN,
		"DEV POLL USB FreeBSD Timeout: %d\npoll()"
		" should not be called for FreeBSD USB\n", timeout));

	return 1;
}

/***********************************************************************
 *
 * Function:    u_write
 *
 * Parameters:	ps is of type pi_socket that contains the sd member which is
 *              the file descriptor that the data in buf will be written to. 
 *
 *              buf is a unsigned char pointer that points to the data that
 *              is to be written to ps->sd.  
 *
 *		len is of type int and indicated the number of bytes to
 *              write to ps->sd.  
 * 
 *		flags is of type int and contains various write flags.  What
 *              flags and should I respect them, since I am currently not
 *              checking this variable?
 *
 * Parameters:  None
 *
 * Returns:     The number of bytes written from buf or -1 to indicate an
 * 		error
 *
 ***********************************************************************/
static int
u_write(pi_socket_t *ps, unsigned char *buf, int len, int flags)
{
	int 	nwrote, 
		total, 
		write_len;

	fd_set 	ready;

	total 		= len;
	write_len 	= len;

	while (total > 0) {
		FD_ZERO(&ready);
		FD_SET(ps->sd, &ready);

		if (!FD_ISSET(ps->sd, &ready))
			return -1;
		nwrote = write(ps->sd, buf, write_len);

		if (nwrote < 0)
			return -1;

		write_len -= nwrote;
		buf += nwrote;
		total -= nwrote;
	}

	LOG((PI_DBG_DEV, PI_DBG_LVL_INFO,
		"DEV TX USB FreeBSD Bytes: %d\n", len));

	return len;
}

/***********************************************************************
 *
 * Function:    u_read
 *
 * Summary:	Read incoming data from the socket/file descriptor and
 *              buffer any extra data that is read in the process of
 *              receiving data from the device.
 *
 * Parameters:	ps is of type pi_socket that contains the sd member which is
 *              the file descriptor that the data in buf will be read. It
 *              also contains the read buffer.
 *
 *		buf is a pointer to an unsigned character string that will
 *              receive the data the is read from the device.
 *
 *		len is of type int that is the the number of bytes to read
 *              from the device/buffer into buf.
 *
 *		flags is of type in that currently only indicated if you
 *              want to peek data from the device/buffer .
 *
 * Returns:	The number of bytes that was read from the device or from
 *              the buffer and copied to buf.
 *
 ***********************************************************************/
static int
u_read(pi_socket_t *ps, unsigned char *buf, int len, int flags)
{
        struct pi_usb_data *data = (struct pi_usb_data *) ps->device->data;
        int bytes_want = 0;
        int rlen = 0;
        int bytes_read = 0;
        unsigned char *pbuf_pos = NULL;
        fd_set ready;

        if (flags == PI_MSG_PEEK && len > MAX_BUF)
                len = MAX_BUF;

        if (data->buf_size > 0) {
                /* copy buffered data to storage */
                if (len <= data->buf_size) {
                        memcpy(buf, data->pos, len);
                        if (flags != PI_MSG_PEEK) {
                                data->pos += len;
                                data->buf_size -= len;
                        }
                        bytes_read = len;
                } else {
                        memcpy(buf, data->pos, data->buf_size);
                        bytes_read += data->buf_size;
                        data->buf_size = 0;
                }
        } else if (data->buf_size == 0 && bytes_read < len) {
                /* reset data buffer */
                data->pos = data->buf;
                /* offset storage to no overwrite data copied the buffer */
                pbuf_pos = buf + bytes_read;
                bytes_want = len - bytes_read;
                do {
                        if (bytes_want >= MAX_BUF) {
                                /* check to see if device is ready for read */
                                FD_ZERO(&ready);
                                FD_SET(ps->sd, &ready);
                                if (!FD_ISSET(ps->sd, &ready)) {
                                        LOG((PI_DBG_DEV, PI_DBG_LVL_WARN,
                                            "DEV RX USB FreeBSD timeout\n"));
                                        errno = ETIMEDOUT;
                                        return -1;
                                }
                                /* read directly into storage */
                                rlen = read(ps->sd, pbuf_pos, MAX_BUF);
                                if (rlen < 0)
                                        return -1;
                                bytes_read += rlen;
                                bytes_want -= rlen;
                                pbuf_pos += rlen;
                        } else if (bytes_want < MAX_BUF) {
                                /* check to see if device is ready for read */
                                FD_ZERO(&ready);
                                FD_SET(ps->sd, &ready);
                                if (!FD_ISSET(ps->sd, &ready)) {
                                        LOG((PI_DBG_DEV, PI_DBG_LVL_WARN,
                                            "DEV RX USB FreeBSD timeout\n"));
                                        errno = ETIMEDOUT;
                                        return -1;
                                }
                                /* read to buffer */
                                rlen = read(ps->sd, data->pos, MAX_BUF);
                                if (rlen < 0)
                                        return -1;
                                if (rlen >= bytes_want) {
                                        memcpy(pbuf_pos, data->pos,
                                               bytes_want);
                                        data->pos += bytes_want;
                                        data->buf_size = rlen - bytes_want;
                                        bytes_read += bytes_want;
                                        bytes_want = 0;
                                } else {
                                        memcpy(pbuf_pos, data->pos, rlen);
                                        bytes_want -= rlen;
                                        bytes_read += rlen;
                                        pbuf_pos += rlen;
                                }
                        }
                } while (bytes_want > 0);
                if (flags == PI_MSG_PEEK) {
                        /* copy back to the buffer since we are only peeking */
                        memcpy(data->buf, buf, bytes_read);
                        data->buf_size = bytes_read;
                        data->pos = data->buf;


                }
        }
        LOG((PI_DBG_DEV, PI_DBG_LVL_INFO,
            "DEV RX USB FreeBSD Bytes: %d:%d\n", bytes_read,
            bytes_read + data->buf_size));

        return bytes_read;
}

