/*
 * pi-util.h: Header for utility routines
 *
 * Copyright (c) 2000, Helix Code Inc.
 *
 * Author: JP Rosevear <jpr@helixcode.com>
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

#ifndef _PILOT_UTIL_H_
#define _PILOT_UTIL_H_

#ifndef PI_DEPRECATED
#if __GNUC__ - 0 > 3 || (__GNUC__ - 0 == 3 && __GNUC_MINOR__ - 0 >= 2)
# define PI_DEPRECATED __attribute__ ((deprecated))
#else
# define PI_DEPRECATED
#endif
#endif


#ifdef __cplusplus
extern "C" {
#endif

#include "pi-args.h"

/* pi_mktag Turn a sequence of characters into a long (er.. 32 bit quantity)
            like those used on the PalmOS device to identify creators and
            similar.
   pi_untag Given a 32 bit identifier, unpack it into the 5-byte char array
	    buf so it is suitable for printing.


   Both of these macros are deprecated for runtime use, but for
   calculating compile-time constants pi_mktag is ok.
*/
#define pi_mktag(c1,c2,c3,c4) (((c1)<<24)|((c2)<<16)|((c3)<<8)|(c4))
#define pi_untag(buf,tag) { buf[0]=(tag >> 24) & 0xff; \
	buf[1]=(tag >> 16) & 0xff; \
	buf[2]=(tag >> 8) & 0xff; \
	buf[3]=(tag) & 0xff; \
	buf[4]=0; }

	extern int convert_ToPilotChar
	    PI_ARGS((const char *charset, const char *text, int bytes,
		     char **ptext));

	extern int convert_FromPilotChar
	    PI_ARGS((const char *charset, const char *ptext, int bytes,
		     char **text));

#ifdef __cplusplus
}
#endif
#endif
