/*
 * darwinusbcore.h: I/O support for Darwin (Mac OS X) USB
 *
 * Copyright (c) 2004, Florent Pillet.
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
#ifndef _DARWINUSBCORE_H_
#define _DARWINUSBCORE_H_

#ifdef __cplusplus
extern "C" {
#endif
	extern int
	darwin_usb_start_listening();

	extern void
	darwin_usb_stop_listening();

	extern int
	darwin_usb_write(unsigned char *buf, int len);

	extern int
	darwin_usb_poll(int timeout);

	extern int
	darwin_usb_read(unsigned char *buf, int len, int timeout, int flags);
#ifdef __cplusplus
}
#endif

#endif

