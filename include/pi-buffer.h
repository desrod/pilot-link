/*
 * pi-buffer.c:  simple data block management for variable data storage
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
 *
 * -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*-
 */
#ifndef _PILOT_BUFFER_H_
#define _PILOT_BUFFER_H_

#include "pi-args.h"

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct pi_buffer_t {
		unsigned char *data;
		size_t allocated;
		size_t used;
	} pi_buffer_t;

	extern pi_buffer_t* pi_buffer_new
		PI_ARGS((size_t capacity));
	
	extern pi_buffer_t* pi_buffer_expect
		PI_ARGS((pi_buffer_t *buf, size_t new_capacity));

	extern pi_buffer_t* pi_buffer_append
		PI_ARGS((pi_buffer_t *buf, void *data, size_t len));

	extern pi_buffer_t* pi_buffer_append_buffer
		PI_ARGS((pi_buffer_t *dest, pi_buffer_t *src));

	extern void pi_buffer_clear
		PI_ARGS((pi_buffer_t *buf));

	extern void pi_buffer_free
		PI_ARGS((pi_buffer_t *buf));

#ifdef __cplusplus
}
#endif

#endif
