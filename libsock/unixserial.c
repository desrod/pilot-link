/*
 * unixserial.c: tty line interface code for Pilot serial comms under UNIX
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
#include <fcntl.h>
#include <string.h>

#include "pi-debug.h"
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-serial.h"

/* if this is running on a NeXT system... */
#ifdef NeXT
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/file.h>
#endif

#ifdef HAVE_SYS_IOCTL_COMPAT_H
#include <sys/ioctl_compat.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifndef SGTTY

#ifndef HAVE_CFMAKERAW
#define cfmakeraw(ptr) (ptr)->c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR\
					 |IGNCR|ICRNL|IXON);\
                       (ptr)->c_oflag &= ~OPOST;\
                       (ptr)->c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);\
                       (ptr)->c_cflag &= ~(CSIZE|PARENB);\
                       (ptr)->c_cflag |= CS8
#endif

#ifndef HAVE_CFSETSPEED
#if defined(HAVE_CFSETISPEED) && defined(HAVE_CFSETOSPEED)
#define cfsetspeed(t,speed) \
  (cfsetispeed(t,speed) || cfsetospeed(t,speed))
#else
static int cfsetspeed(struct termios *t, int speed)
{
#ifdef HAVE_TERMIOS_CSPEED
	t->c_ispeed = speed;
	t->c_ospeed = speed;
#else
	t->c_cflag |= speed;
#endif
	return 0;
}
#endif
#endif

#endif /* SGTTY */
static int calcrate(int baudrate)
{
#ifdef B300
	if (baudrate == 300)
		return B300;
#endif
#ifdef B1200
	if (baudrate == 1200)
		return B1200;
#endif
#ifdef B2400
	if (baudrate == 2400)
		return B2400;
#endif
#ifdef B4800
	if (baudrate == 4800)
		return B4800;
#endif
#ifdef B9600
	if (baudrate == 9600)
		return B9600;
#endif
#ifdef B19200
	else if (baudrate == 19200)
		return B19200;
#endif
#ifdef B38400
	else if (baudrate == 38400)
		return B38400;
#endif
#ifdef B57600
	else if (baudrate == 57600)
		return B57600;
#endif
#ifdef B115200
	else if (baudrate == 115200)
		return B115200;
#endif
#ifdef B230400
	else if (baudrate == 230400)
		return B230400;
#endif
#ifdef B460800
	else if (baudrate == 460800)
		return B460800;
#endif
	else {
		LOG(PI_DBG_DEV, PI_DBG_LVL_ERR,
		    "DEV Serial CHANGEBAUD Unable to set baud rate %d\n",
		    baudrate);
		abort();	/* invalid baud rate */
	}
}

#ifndef O_NONBLOCK
# define O_NONBLOCK 0
#endif


static int s_open(struct pi_socket *ps, struct pi_sockaddr *addr, int addrlen);
static int s_close(struct pi_socket *ps);
static int s_changebaud(struct pi_socket *ps);
static int s_write(struct pi_socket *ps, unsigned char *buf, int len, int flags);
static int s_read(struct pi_socket *ps, unsigned char *buf, int len, int flags);
static int s_poll(struct pi_socket *ps, int timeout);

void pi_serial_impl_init (struct pi_serial_impl *impl)
{
	impl->open = s_open;
	impl->close = s_close;
	impl->changebaud = s_changebaud;
	impl->write = s_write;
	impl->read = s_read;
	impl->poll = s_poll;
}


/***********************************************************************
 *
 * Function:    s_open
 *
 * Summary:     Open the serial port and establish a connection for
 *		unix
 *
 * Parmeters:   None
 *
 * Returns:     The file descriptor
 *
 ***********************************************************************/
int
s_open(struct pi_socket *ps, struct pi_sockaddr *addr, int addrlen)
{
	int 	fd, 
		i;
	char 	*tty 	= addr->pi_device;
	struct pi_serial_data *data = (struct pi_serial_data *)ps->device->data;



	
#ifndef SGTTY
	struct termios tcn;
#else
	struct sgttyb tcn;
#endif
	if ((fd = open(tty, O_RDWR | O_NONBLOCK)) == -1) {
		return -1;	/* errno already set */
	}

	if (!isatty(fd)) {
		close(fd);
		errno = EINVAL;
		return -1;
	}
#ifndef SGTTY
	/* Set the tty to raw and to the correct speed */
	tcgetattr(fd, &tcn);

	data->tco 	= tcn;
	tcn.c_oflag 	= 0;
	tcn.c_iflag 	= IGNBRK | IGNPAR;
	tcn.c_cflag 	= CREAD | CLOCAL | CS8;

	(void) cfsetspeed(&tcn, calcrate(data->rate));

	tcn.c_lflag = NOFLSH;

	cfmakeraw(&tcn);

	for (i = 0; i < 16; i++)
		tcn.c_cc[i] = 0;

	tcn.c_cc[VMIN] = 1;
	tcn.c_cc[VTIME] = 0;

	tcsetattr(fd, TCSANOW, &tcn);
#else
	/* Set the tty to raw and to the correct speed */
	ioctl(fd, TIOCGETP, &tcn);

	data->tco = tcn;

	tcn.sg_flags = RAW;
	tcn.sg_ispeed = calcrate(data->rate);
	tcn.sg_ospeed = calcrate(data->rate);

	ioctl(fd, TIOCSETN, &tcn);
#endif

	if ((i = fcntl(fd, F_GETFL, 0)) != -1) {
		i &= ~O_NONBLOCK;
		fcntl(fd, F_SETFL, i);
	}

	if (pi_socket_setsd(ps, fd) < 0)
		return -1;

	return fd;
}

/* Linux versions "before 2.1.8 or so" fail to flush hardware FIFO on port
   close */
#ifdef linux
# ifndef LINUX_VERSION_CODE
#  include <linux/version.h>
# endif
# ifndef LINUX_VERSION_CODE
#  define sleeping_beauty
# else
#  if (LINUX_VERSION_CODE < 0x020108)
#   define sleeping_beauty
#  endif
# endif
#endif

/* Unspecified NetBSD versions fail to flush hardware FIFO on port close */
#if defined(__NetBSD__) || defined (__OpenBSD__)
# define sleeping_beauty
#endif

/* Unspecified BSD/OS versions fail to flush hardware FIFO on port close */
#ifdef __bsdi__
# define sleeping_beauty
#endif

/* SGI IRIX fails to flush hardware FIFO on port close */
#ifdef __sgi
# define sleeping_beauty
#endif

#ifdef sleeping_beauty
/***********************************************************************
 *
 * Function:    s_delay
 *
 * Summary:     Delay for a given period have time
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static s_delay(int sec, int usec)
{
	struct 	timeval tv;

	tv.tv_sec 	= sec;
	tv.tv_usec 	= usec;

	select(0, 0, 0, 0, &tv);
}
#endif

/***********************************************************************
 *
 * Function:    s_changebaud
 *
 * Summary:     Change the speed of the socket
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int s_changebaud(struct pi_socket *ps)
{
	struct 	pi_serial_data *data = (struct pi_serial_data *)ps->device->data;
#ifndef SGTTY
	struct 	termios tcn;

#ifdef sleeping_beauty
	s_delay(0, 200000);
#endif
	/* Set the tty to the new speed */ tcgetattr(ps->sd, &tcn);

	tcn.c_cflag 	= CREAD | CLOCAL | CS8;
	(void) cfsetspeed(&tcn, calcrate(data->rate));

	tcsetattr(ps->sd, TCSADRAIN, &tcn);

#else
	struct sgttyb tcn;

	ioctl(ps->sd, TIOCGETP, &tcn);

	tcn.sg_ispeed 	= calcrate(data->rate);
	tcn.sg_ospeed 	= calcrate(data->rate);

	ioctl(ps->sd, TIOCSETN, &tcn);
#endif

#ifdef sleeping_beauty
	s_delay(0, 200000);
#endif
	return 0;
}

/***********************************************************************
 *
 * Function:    s_close
 *
 * Summary:     Close the open socket/file descriptor
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int s_close(struct pi_socket *ps)
{
	struct 	pi_serial_data *data = (struct pi_serial_data *)ps->device->data;

#ifdef sleeping_beauty
	s_delay(2, 0);
#endif

#ifndef SGTTY
	tcsetattr(ps->sd, TCSADRAIN, &data->tco);
#else
	ioctl(ps->sd, TIOCSETP, &data->tco);
#endif

	LOG(PI_DBG_DEV, PI_DBG_LVL_INFO, "DEV Serial CLOSE fd: %d\n", ps->sd);

	return close(ps->sd);
}

static int s_poll(struct pi_socket *ps, int timeout)
{
	struct 	pi_serial_data *data = (struct pi_serial_data *)ps->device->data;
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
		LOG(PI_DBG_DEV, PI_DBG_LVL_WARN, "DEV POLL Serial Unix timeout\n");
		data->rx_errors++;
		return -1;
	}
	LOG(PI_DBG_DEV, PI_DBG_LVL_DEBUG, "DEV POLL Serial Unix Read data on %d\n", ps->sd);

	return 0;
}

/***********************************************************************
 *
 * Function:    s_write
 *
 * Summary:     Write to the open socket/file descriptor
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int s_write(struct pi_socket *ps, unsigned char *buf, int len, int flags)
{
	int 	total,
		nwrote;
	struct pi_serial_data *data = (struct pi_serial_data *)ps->device->data;


	total = len;
	while (total > 0) {
		nwrote = write(ps->sd, buf, len);
		if (nwrote < 0)
			return -1;
		total -= nwrote;
	}
	data->tx_bytes += len;

	/* hack to slow things down so that the Visor will work */
	usleep(10 + len);

	LOG(PI_DBG_DEV, PI_DBG_LVL_INFO, "DEV TX Unix Serial Bytes: %d\n", len);

	return len;
}

static int s_read_buf (struct pi_socket *ps, unsigned char *buf, int len) 
{
	int 	rbuf;
	struct pi_serial_data *data = (struct pi_serial_data *)ps->device->data;

	rbuf = data->buf_size;
	if (rbuf > len)
		rbuf = len;
	memcpy(buf, data->buf, rbuf);
	data->buf_size -= rbuf;
	
	if (data->buf_size > 0)
		memcpy(data->buf, &data->buf[rbuf], data->buf_size);
	
	LOG(PI_DBG_DEV, PI_DBG_LVL_INFO, "DEV RX Unix Serial Buffer Read %d bytes\n", rbuf);
	
	return rbuf;
}

/***********************************************************************
 *
 * Function:    s_read
 *
 * Summary:     Read incoming data from the socket/file descriptor
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int s_read(struct pi_socket *ps, unsigned char *buf, int len, int flags)
{
	int 	rbuf;
	struct 	pi_serial_data *data = (struct pi_serial_data *)ps->device->data;
	struct 	timeval t;
	fd_set 	ready;

	FD_ZERO(&ready);
	FD_SET(ps->sd, &ready);

	if (data->buf_size > 0)
		return s_read_buf(ps, buf, len);
	
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
	if (FD_ISSET(ps->sd, &ready)) {
		if (flags == PI_MSG_PEEK && len > 256)
			len = 256;
		rbuf = read(ps->sd, buf, len);
		if (flags == PI_MSG_PEEK) {
			memcpy(data->buf, buf, rbuf);
			data->buf_size = rbuf;
		}
	} else {
		/* otherwise throw out any current packet and return */
		LOG(PI_DBG_DEV, PI_DBG_LVL_WARN, "DEV RX Unix Serial timeout\n");
		data->rx_errors++;
		return 0;
	}
	data->rx_bytes += rbuf;

	LOG(PI_DBG_DEV, PI_DBG_LVL_INFO, "DEV RX Unix Serial Bytes: %d\n", rbuf);

	return rbuf;
}


