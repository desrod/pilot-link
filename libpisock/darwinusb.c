/*
 * darwinusb.c: i/o wrapper for Darwin (Mac OS X) USB
 *
 * Copyright (c) 2004, Florent Pillet.
 * Modeled after linuxusb.c by Jeff Dionne and Kenneth Albanowski
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "pi-debug.h"
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-usb.h"
#include "darwinusbcore.h"

static int u_open(struct pi_socket *ps, struct pi_sockaddr *addr, int addrlen);
static int u_close(struct pi_socket *ps);
static int u_write(struct pi_socket *ps, unsigned char *buf, int len, int flags);
static int u_read(struct pi_socket *ps, unsigned char *buf, int len, int flags);
static int u_poll(struct pi_socket *ps, int timeout);

void
pi_usb_impl_init (struct pi_usb_impl *impl)
{
	impl->open 		= u_open;
	impl->close		= u_close;
	impl->write 	= u_write;
	impl->read 		= u_read;
	impl->poll 		= u_poll;
}

static int
u_open(struct pi_socket *ps, struct pi_sockaddr *addr, int addrlen)
{
	if (darwin_usb_start_listening() != 0) {
		errno = EINVAL;
		return -1;
	}
	return 1;
}

static int
u_close(struct pi_socket *ps)
{
	darwin_usb_stop_listening();
	return 0;
}

static int
u_poll(struct pi_socket *ps, int timeout)
{
	int n = darwin_usb_poll(timeout);
	return n;
}

static int
u_write(struct pi_socket *ps, unsigned char *buf, int len, int flags)
{
	len = darwin_usb_write(buf, len);
	return len;
}

static int
u_read(struct pi_socket *ps, unsigned char *buf, int len, int flags)
{
	int n = darwin_usb_read(buf, len, ((struct pi_usb_data *)ps->device->data)->timeout, flags);
	return n;
}

