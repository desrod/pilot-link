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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pi-buffer.h"

/***********************************************************************
 *
 * Function:    pi_buffer_new
 *
 * Summary:     Allocate a new buffer with a default storage capacity
 *
 * Parameters:  capacity		--> base allocation size
 *
 * Returns:     new buffer structure. If allocation failed, the `allocated'
 *				and `data' members are set to 0
 *
 ***********************************************************************/
pi_buffer_t*
pi_buffer_new (size_t capacity) 
{
	pi_buffer_t* buf;
	buf = (struct pi_buffer_t *) malloc (sizeof (struct pi_buffer_t));
	if (buf == NULL)
		return NULL;

	buf->data = (unsigned char *) malloc (capacity);
	if (buf->data == NULL) {
		free (buf);
		return NULL;
	}

	buf->allocated = capacity;
	buf->used = 0;
	return buf;
}

/***********************************************************************
 *
 * Function:    pi_buffer_expect
 *
 * Summary:     Grow a buffer's storage capacity so that the expected
 *				number of bytes can be directly stored in the storage area
 *
 * Parameters:  buf				--> ptr to the buffer structure
 *				new_capacity	--> new allocated capacity
 *
 * Returns:     on return, buf struct is updated. If allocation failed,
 *				both `allocated' and `data' members are set to 0 and the
 *				function returns NULL
 *
 ***********************************************************************/
pi_buffer_t*
pi_buffer_expect (pi_buffer_t *buf, size_t expect)
{
	if ((buf->allocated - buf->used) >= expect)
		return buf;

	if (buf->data)
		buf->data = (unsigned char *) realloc (buf->data, buf->used + expect);
	else
		buf->data = (unsigned char *) malloc (expect);

	if (buf->data == NULL) {
		buf->allocated = 0;
		buf->used = 0;
		return NULL;
	}

	buf->allocated = buf->used + expect;
	return buf;
}

/***********************************************************************
 *
 * Function:    pi_buffer_append
 *
 * Summary:     Add data to a buffer, grow it as needed
 *
 * Parameters:  buf	--> ptr to the buffer structure
 *		data	--> data to append
 *		len	--> data size to append
 *
 * Returns:     on return, buf struct is updated. If allocation failed,
 *				both `allocated' and `data' members are set to 0
 *
 ***********************************************************************/
pi_buffer_t*
pi_buffer_append (pi_buffer_t *buf, void *data, size_t len)
{
	if (pi_buffer_expect (buf, len) == NULL)
		return NULL;

	memcpy (buf->data + buf->used, data, len);
	buf->used += len;

	return buf;
}

/***********************************************************************
 *
 * Function:    pi_buffer_append_buffer
 *
 * Summary:     Append a buffer to another buffer
 *
 * Parameters:  dest	--> ptr to the buffer to append to
 *		src     --> ptr to the source buffer to append
 *
 * Returns:     on return, dest struct is updated. If allocation failed,
 *				both `allocated' and `data' members are set to 0
 *
 ***********************************************************************/
pi_buffer_t *
pi_buffer_append_buffer (pi_buffer_t *dest, pi_buffer_t *src)
{
	return pi_buffer_append (dest, src->data, src->used);
}

/***********************************************************************
 *
 * Function:    pi_buffer_clear
 *
 * Summary:     Clear data in buffer
 *
 * Parameters:  buf	--> ptr to the buffer structure
 *
 * Returns:     nothing
 *
 ***********************************************************************/
void
pi_buffer_clear (pi_buffer_t *buf)
{
	buf->used = 0;
}

/***********************************************************************
 *
 * Function:    pi_buffer_free
 *
 * Summary:     Add data to a buffer, grow it as needed
 *
 * Parameters:  buf	--> ptr to the buffer structure
 *		data	--> data to append
 *		len     --> data size to append
 *
 * Returns:     on return, buf struct is updated. If allocation failed,
 *				both `allocated' and `data' members are set to 0
 *
 ***********************************************************************/
void
pi_buffer_free (pi_buffer_t* buf)
{
	if (buf->data)
		free (buf->data);
	free (buf);
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
