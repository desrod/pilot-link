/*
 * sync.h: Header for generic synchronization algorithm
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

#ifdef __cplusplus
extern "C" {
#endif

#include "pi-args.h"
#include "pi-config.h"

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
