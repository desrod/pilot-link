/*
 * winserial.c: tty line interface code for Pilot comms under Win32
 *
 * Copyright (c) 1999 Jeff Senn
 * Copyright (c) 1999 Tilo Christ
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

#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-serial.h"
#include "pi-slp.h"
#include "pi-padp.h"

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
 * Summary:     Open the serial port and establish a connection for 
 *		Win32
 *
 * Parmeters:   None
 *
 * Returns:     The file descriptor
 *
 ***********************************************************************/
int
pi_serial_open(struct pi_socket *ps, struct pi_sockaddr *addr, int addrlen)
{
	HANDLE fd;
	char *tty = addr->pi_device;

	/* open the device */
	if ((!tty) || !strlen(tty))
		tty = getenv("PILOTPORT");
	if (!tty)
		tty = "<Null>";
	fd = CreateFile(tty, GENERIC_READ | GENERIC_WRITE, 0, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (fd == INVALID_HANDLE_VALUE) {
		/* can't open */
		errno = GetLastError();
		return -1;
	}

	ps->mac->fd = (int) fd;

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

	return ((int) fd);
}

/***********************************************************************
 *
 * Function:    so_changebaud
 *
 * Summary:     Change the speed of the socket/file descriptor
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int so_changebaud(struct pi_socket *ps)
{
	HANDLE fd = (HANDLE) ps->mac->fd;

	return win_changebaud(fd, ps->rate);
}

/***********************************************************************
 *
 * Function:    win_changebaud
 *
 * Summary:     Change the connection parameters on Win32
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int win_changebaud(HANDLE fd, int rate)
{
	BOOL rc;
	COMMTIMEOUTS ctmoCommPort;
	DCB dcbCommPort;

	/* Initialize queue size */
	SetupComm(fd, 4096, 4096);

	/* Timeouts equal to those used by Palm Desktop */
	ctmoCommPort.ReadIntervalTimeout = 2000;
	ctmoCommPort.ReadTotalTimeoutMultiplier = 15;
	ctmoCommPort.ReadTotalTimeoutConstant = 500;
	ctmoCommPort.WriteTotalTimeoutMultiplier = 15;
	ctmoCommPort.WriteTotalTimeoutConstant = 500;
	SetCommTimeouts(fd, &ctmoCommPort);

	dcbCommPort.DCBlength 		= sizeof(DCB);
	GetCommState(fd, &dcbCommPort);
	dcbCommPort.BaudRate 		= rate;
	dcbCommPort.fOutxCtsFlow 	= (rate > 9600) ? TRUE : FALSE;		// CTS output flow control 
	dcbCommPort.fOutxDsrFlow 	= 0;					// DSR output flow control 
	dcbCommPort.fDtrControl 	= DTR_CONTROL_ENABLE;			// DTR flow control type 
	dcbCommPort.fRtsControl 	= (rate > 9600) ? RTS_CONTROL_HANDSHAKE : RTS_CONTROL_ENABLE;	// RTS flow control type 
	dcbCommPort.fTXContinueOnXoff 	= 0;					// XOFF continues Tx 
	dcbCommPort.fOutX 		= 0;					// XON/XOFF out flow control 
	dcbCommPort.fInX 		= 0;					// XON/XOFF in flow control 
	dcbCommPort.ByteSize 		= 8;
	dcbCommPort.Parity 		= NOPARITY;				// no parity
	dcbCommPort.StopBits 		= 0;					// 1 stop bit

	rc = SetCommState(fd, &dcbCommPort);

/*  Sleep(100);  */

	if (rc)
		return 0;
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
	CloseHandle((HANDLE) ps->mac->fd);
	return (0);
}

/***********************************************************************
 *
 * Function:    so_write
 *
 * Summary:     Write to the open socket/file descriptor 
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
	HANDLE hCommPort = (HANDLE) ps->mac->fd;

	if (ps->txq) {
		ps->busy++;
		skb = ps->txq;
		ps->txq = skb->next;
		len = 0;
		while (len < skb->len) {
			nwrote = 0;
			rc = WriteFile(hCommPort, skb->data + len,
				       skb->len - len, &nwrote, NULL);
			if (!rc || nwrote <= 0)
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
	HANDLE hCommPort = (HANDLE) ps->mac->fd;

/*  pi_socket_set_timeout(ps,timeout,-1);  */
/*  Palm Desktop doesn't do this           */
	pi_serial_flush(ps);	/* We likely want to be in sync with tx */

	if (!ps->mac->expect)
		slp_rx(ps);	/* let SLP know we want a packet */

	while (ps->mac->expect) {
		buf = ps->mac->buf;

		while (ps->mac->expect) {
			rc = ReadFile(hCommPort, buf, ps->mac->expect, &r,
				      NULL);
			if (!rc) {
				/* otherwise throw out any current packet and return */
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

/***********************************************************************
 *
 * Function:    win_peek
 *
 * Summary:     Wait for incoming connection from Palm device
 *
 * Parmeters:   None
 *
 * Returns:     -1 if no connection, 0 otherwise
 *
 ***********************************************************************/
int win_peek(struct pi_socket *ps, int timeout)
{
	int time = timeout;
	COMSTAT comstat;
	DWORD error;
	HANDLE hCommPort = (HANDLE) ps->mac->fd;

/*  win_changebaud(hCommPort, 9600);  */

	for (;;) {
		ClearCommError(hCommPort, &error, &comstat);
		if (comstat.cbInQue > 0)
			return 0;
		else {
			time -= 100;

			if ((time <= 0) && (timeout != 0))
				return -1;

			Sleep(100);
		}
	}
}
