/*
 * os2serial.c: tty line interface code for Pilot comms under OS/2
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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <io.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-serial.h"
#include "pi-slp.h"
#include "pi-syspkt.h"
#include "pi-padp.h"

#ifdef HAVE_SYS_IOCTL_COMPAT_H
#include <sys/ioctl_compat.h>
#endif

#define INCL_BASE
#define INCL_DOSFILEMGR		/* File System values */
#define INCL_DOSDEVIOCTL	/* DosDevIOCtl values */
#define INCL_DOSDEVICES		/* DosDevice   values */
#define INCL_MISC
#include <os2.h>

static int so_changebaud(struct pi_socket *ps);
static int so_close(struct pi_socket *ps);
static int pi_socket_set_timeout(struct pi_socket *ps, int read_timeout,
				 int write_timeout);
static int so_write(struct pi_socket *ps);
static int so_read(struct pi_socket *ps, int timeout);

/***********************************************************************
 *
 * Function:    pi_serial_open
 *
 * Summary:     Open the serial connection and listen for incoming data
 *
 * Parmeters:   None
 *
 * Returns:     The file descriptor used
 *
 ***********************************************************************/
int
pi_serial_open(struct pi_socket *ps, struct pi_sockaddr *addr, int addrlen)
{
	int rc;
	HFILE fd;
	unsigned long action;
	int filesize = 0;
	char *tty = addr->pi_device;

	if ((!tty) || !strlen(tty))
		tty = getenv("PILOTPORT");
	if (!tty)
		tty = "<Null>";

	/* open the device */
	rc = DosOpen(tty,						/* the device 					*/
		     &fd,						/* the file descriptor returned 		*/
		     &action,						/* the action taken 				*/
		     filesize,						/* the size of the file 			*/
		     FILE_NORMAL,					/* file permissions mode, not the same as UNIX 	*/
		     OPEN_ACTION_OPEN_IF_EXISTS,			/* file open action 				*/
		     OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_READWRITE,	/* open mode 					*/
		     0);						/* extended attributes 				*/
	if (rc) {
		switch (rc) {
		case 2:		/* ERROR_FILE_NOT_FOUND  	*/
			errno = ENOENT;
			break;
		case 3:		/* ERROR_PATH_NOT_FOUND  	*/
			errno = ENOTDIR;
			break;
		case 4:		/* ERROR_TOO_MANY_OPEN_FILES  	*/
			errno = EMFILE;
			break;
		case 5:		/* ERROR_ACCESS_DENIED  	*/
			errno = EACCES;
			break;
		case 32:	/* ERROR_SHARING_VIOLATION  	*/
			errno = EBUSY;
			break;
		case 82:	/* ERROR_CANNOT_MAKE  		*/
			errno = EEXIST;
			break;
		case 99:	/* ERROR_DEVICE_IN_USE  	*/
			errno = EBUSY;
			break;
		case 112:	/* ERROR_DISK_FULL  		*/
			errno = ENOSPC;
			break;
		case 87:	/* ERROR_INVALID_PARAMETER  	*/
			errno = EINVAL;
			break;
		default:
			errno = -ENOMSG;
			break;
		}
		return (-1);
	}

	ps->mac->fd = _imphandle(fd);			/* Let EMX know about this handle */
	ps->mac->fd = dup2(ps->mac->fd, ps->sd);	/* Substitute serial connection for
							   original NUL handle */

	so_changebaud(ps);
	pi_socket_set_timeout(ps, -1, 60000);

	ps->serial_close = so_close;
	ps->serial_read = so_read;
	ps->serial_write = so_write;
	ps->serial_changebaud = so_changebaud;

#ifndef NO_SERIAL_TRACE
	if (ps->debuglog) {
		ps->debugfd = open(ps->debuglog, O_WRONLY | O_CREAT, 0666);

		/* This sequence is magic used by my trace analyzer - kja */
		write(ps->debugfd, "\0\1\0\0\0\0\0\0\0\0", 10);
	}
#endif
	return (fd);
}

/* stuff for OS/2 ASYNC_EXTSETBAUDRATE this seems like a good place to put
   this, as it is only used here in so_changebaud(), MJJ. */
struct STR_EXTSETBAUDRATE {
	ULONG baudrate;
	UCHAR fraction;
};

/***********************************************************************
 *
 * Function:    so_changebaud
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int so_changebaud(struct pi_socket *ps)
{
	int param_length;
	int rc;
	unsigned char linctrl[3] = { 8, 0, 0 };
	struct STR_EXTSETBAUDRATE extsetbaudrate;

	extsetbaudrate.baudrate = ps->rate;
	extsetbaudrate.fraction = 0;		/* if anyone knows some fractions 	*/
						/* that could be used here, let me know */

	param_length = sizeof(extsetbaudrate);
	rc = DosDevIOCtl(ps->mac->fd,				/* file decsriptor 			*/
			 IOCTL_ASYNC,				/* asyncronous change 			*/
			 ASYNC_EXTSETBAUDRATE,			/* set the baudrate 			*/
			 &extsetbaudrate,			/* pointer to the baudrate 		*/
			 param_length,				/* length of the previous parameter 	*/
			 (unsigned long *) &param_length,	/* max length of data ret 		*/
			 NULL,					/* data to be sent 			*/
			 0,					/* length of data 			*/
			 NULL);					/* length of data returned 		*/

	/* also set the port to 8N1 as OS/2 defaults to some braindead
	   values */

	if (!rc) {							/* but only if the previous operation succeeded 	*/
		param_length = 3;					/* 3 bytes for line control 				*/
		rc = DosDevIOCtl(ps->mac->fd,				/* file decsriptor 					*/
				 IOCTL_ASYNC,				/* asyncronous change 					*/
				 ASYNC_SETLINECTRL,			/* set the line controls 				*/
				 linctrl,				/* pointer to the configuration 			*/
				 param_length,				/* length of the previous parameter		 	*/
				 (unsigned long *) &param_length,	/* max length of params		 			*/
				 NULL,					/* data to be returned 					*/
				 0,					/* length of data 					*/
				 NULL);					/* length of data returned 				*/
	}

	if (rc) {
		switch (rc) {
		case 1:		/* ERROR_INVALID_FUNCTION 	*/
			errno = ENOTTY;
			break;
		case 6:		/* ERROR_INVALID_HANDLE 	*/
			errno = EBADF;
			break;
		case 87:	/* ERROR_INVALID_PARAMETER 	*/
			errno = EINVAL;
			break;
		default:
			errno = -ENOMSG;
			break;
		}
		return (-1);
	}
	/* this pause seems necessary under OS2 to let the serial port
	   realign itself */
	sleep(1);
#ifdef OS2_DEBUG
	fprintf(stderr, "set baudrate to %d\n", baudrate);
#endif
	return (0);
}

/***********************************************************************
 *
 * Function:    so_close
 *
 * Summary:     Close the open socket/file descriptor
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int so_close(struct pi_socket *ps)
{
#ifndef NO_SERIAL_TRACE
	if (ps->debugfd)
		close(ps->debugfd);
#endif

	DosClose(ps->mac->fd);
	return (0);
}

/***********************************************************************
 *
 * Function:    pi_socket_set_timeout
 *
 * Summary:     Set a timeout value for the socket/file descriptor
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 * Comments:    values for read_timeout and write_timeout
 *                 0           = infinite timeout  
 *                 1 to 65535  = timeout in seconds
 *                -1           = dont change timeout
 *
 ***********************************************************************/
static int
pi_socket_set_timeout(struct pi_socket *ps, int read_timeout,
		      int write_timeout)
{
	int param_length, ret_len;
	int rc;
	int newtimeout;
	DCBINFO devinfo;

	if ((ps->os2_read_timeout == read_timeout || read_timeout == -1)
	    && (ps->os2_write_timeout == write_timeout
		|| write_timeout == -1))
		return (0);

	ret_len = sizeof(DCBINFO);
	rc = DosDevIOCtl(ps->mac->fd,			/* file decsriptor 			*/
			 IOCTL_ASYNC,			/* asyncronous change 			*/
			 ASYNC_GETDCBINFO,		/* get device control block info 	*/
			 NULL,				/*  					*/
			 0,				/* length of the previous parameter 	*/
			 NULL,				/* max length of data ret 		*/
			 &devinfo,			/* data to be recieved 			*/
			 ret_len,			/* length of data 			*/
			 (unsigned long *) &ret_len);	/* length of data returned 		*/
	if (rc)
		goto error;

	if (read_timeout != -1) {
		if (read_timeout == 0) {
			devinfo.usReadTimeout = 65535;
		} else {
			newtimeout = read_timeout - 0.1;
			if (newtimeout > 65535)
				newtimeout = 65535;
			devinfo.usReadTimeout = newtimeout;
		}
	}
	if (write_timeout == -1) {
		if (write_timeout == 0) {
			devinfo.fbTimeout |= 0x01;
		} else {
			devinfo.fbTimeout &= 0xFE;
			newtimeout = write_timeout;
			if (newtimeout > 65535)
				newtimeout = 65535;
			devinfo.usWriteTimeout = newtimeout;
		}
	}
	param_length = sizeof(DCBINFO);
	rc = DosDevIOCtl(ps->mac->fd,				/* file decsriptor 			*/
			 IOCTL_ASYNC,				/* asyncronous change 			*/
			 ASYNC_SETDCBINFO,			/* get device control block info 	*/
			 &devinfo,				/* parameters to set  			*/
			 param_length,				/* length of the previous parameter 	*/
			 (unsigned long *) &param_length,	/* max length of parameters 		*/
			 NULL,					/* data to be recieved 			*/
			 0,					/* length of data 			*/
			 NULL);					/* length of data returned 		*/

      error:
	if (rc) {
		switch (rc) {
		case 1:		/* ERROR_INVALID_FUNCTION 	*/
			errno = ENOTTY;
			break;
		case 6:		/* ERROR_INVALID_HANDLE 	*/
			errno = EBADF;
			break;
		case 87:	/* ERROR_INVALID_PARAMETER 	*/
			errno = EINVAL;
			break;
		default:
			errno = -ENOMSG;
			break;
		}
		return (-1);
	}
	if (read_timeout != -1)
		ps->os2_read_timeout = read_timeout;
	if (write_timeout != -1)
		ps->os2_write_timeout = write_timeout;
#ifdef OS2_DEBUG
	fprintf(stderr, "set read_timeout to %d\n", read_timeout);
	fprintf(stderr, "set write_timeout to %d\n", write_timeout);
#endif
	return (0);
}

/***********************************************************************
 *
 * Function:    so_write
 *
 * Summary:     Write our data to the open socket/file descriptor
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int so_write(struct pi_socket *ps)
{
	struct pi_skb *skb;
	int nwrote, len;

#ifndef NO_SERIAL_TRACE
	int i;
#endif
	int rc;

	if (ps->txq) {

		ps->busy++;

		skb = ps->txq;
		ps->txq = skb->next;

		len = 0;
		while (len < skb->len) {
			nwrote = 0;
			rc = DosWrite(ps->mac->fd, skb->data, skb->len,
				      (unsigned long *) &nwrote);
			if (nwrote <= 0)
				break;	/* transmission failure */
			len += nwrote;
		}
#ifndef NO_SERIAL_TRACE
		if (ps->debuglog)
			for (i = 0; i < skb->len; i++) {
				write(ps->debugfd, "2", 1);
				write(ps->debugfd, skb->data + i, 1);
			}
#endif
		ps->tx_bytes += skb->len;
		free(skb);

		ps->busy--;

		return 1;
	}
	return 0;
}

/***********************************************************************
 *
 * Function:    so_read
 *
 * Summary:     Read incoming data from the socket/file descriptor
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int so_read(struct pi_socket *ps, int timeout)
{
	int r;
	unsigned char *buf;

#ifndef NO_SERIAL_TRACE
	int i;
#endif
	int rc;

	/* FIXME: if timeout == 0, wait forever for packet, otherwise wait
	   till timeout milli-seconds */

	/* for OS2, timeout of 0 is almost forever, only 1.8 hours if no
	   timeout is set at all, the timeout defaults to 1 minute */

	rc = pi_socket_set_timeout(ps, timeout / 100, -1);
	if (rc == -1) {
		fprintf(stderr,
			"error setting timeout, old timeout used\n");
	}

	pi_serial_flush(ps);	/* We likely want to be in sync with tx */
	if (!ps->mac->expect)
		slp_rx(ps);	/* let SLP know we want a packet */

	while (ps->mac->expect) {
		buf = ps->mac->buf;

		while (ps->mac->expect) {
			rc = DosRead(ps->mac->fd, buf, ps->mac->expect,
				     (unsigned long *) &r);
			if (rc) {
				/* otherwise throw out any current packet and return */
#ifdef DEBUG
				fprintf(stderr, "Serial RX: timeout\n");
#endif
				ps->mac->state = ps->mac->expect = 1;
				ps->mac->buf = ps->mac->rxb->data;
				ps->rx_errors++;
				return 0;
			}
#ifndef NO_SERIAL_TRACE
			if (ps->debuglog)
				for (i = 0; i < r; i++) {
					write(ps->debugfd, "1", 1);
					write(ps->debugfd, buf + i, 1);
				}
#endif
			ps->rx_bytes += r;
			buf += r;
			ps->mac->expect -= r;
		}
		slp_rx(ps);
	}
	return 0;
}
