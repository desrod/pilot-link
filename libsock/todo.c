/*
 * todo.c:  Translate Palm ToDo application data formats
 *
 * Copyright (c) 1996, Kenneth Albanowski
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
#include <string.h>

#include "pi-source.h"
#include "pi-dlp.h"
#include "pi-todo.h"

/***********************************************************************
 *
 * Function:    free_ToDo
 *
 * Summary:     Free the memory and filehandle from the record alloc. 
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void free_ToDo(struct ToDo *a)
{
	if (a->description)
		free(a->description);
	if (a->note)
		free(a->note);
}

/***********************************************************************
 *
 * Function:    unpack_ToDo
 *
 * Summary:     Unpack the ToDo structure into records we can chew on
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int unpack_ToDo(struct ToDo *a, unsigned char *buffer, int len)
{
	unsigned long d;
	unsigned char *start = buffer;

	/* Note: There are possible timezone conversion problems related to
	   the use of the due member of a struct ToDo. As it is kept in
	   local (wall) time in struct tm's, the timezone of the Palm is
	   irrelevant, _assuming_ that any UNIX program keeping time in
	   time_t's converts them to the correct local time. If the Palm is
	   in a different timezone than the UNIX box, it may not be simple
	   to deduce that correct (desired) timezone.

	   The easiest solution is to keep apointments in struct tm's, and
	   out of time_t's. Of course, this might not actually be a help if
	   you are constantly darting across timezones and trying to keep
	   appointments.
	   -- KJA */

	if (len < 3)
		return 0;
	d = (unsigned short int) get_short(buffer);
	if (d != 0xffff) {
		a->due.tm_year = (d >> 9) + 4;
		a->due.tm_mon = ((d >> 5) & 15) - 1;
		a->due.tm_mday = d & 31;
		a->due.tm_hour = 0;
		a->due.tm_min = 0;
		a->due.tm_sec = 0;
		a->due.tm_isdst = -1;
		mktime(&a->due);
		a->indefinite = 0;
	} else {
		a->indefinite = 1;	/* a->due is invalid */
	}

	a->priority = get_byte(buffer + 2);
	if (a->priority & 0x80) {
		a->complete = 1;
		a->priority &= 0x7f;
	} else {
		a->complete = 0;
	}

	buffer += 3;
	len -= 3;

	if (len < 1)
		return 0;
	a->description = strdup((char *) buffer);

	buffer += strlen(a->description) + 1;
	len -= strlen(a->description) + 1;

	if (len < 1) {
		free(a->description);
		a->description = 0;
		return 0;
	}
	a->note = strdup((char *) buffer);

	buffer += strlen(a->note) + 1;
	len -= strlen(a->note) + 1;

	return (buffer - start);	/* FIXME: return real length */
}

/***********************************************************************
 *
 * Function:    pack_ToDo
 *
 * Summary:     Pack the ToDo records into a structure
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pack_ToDo(struct ToDo *a, unsigned char *buf, int len)
{
	int pos;
	int destlen = 3;

	if (a->description)
		destlen += strlen(a->description);
	destlen++;
	if (a->note)
		destlen += strlen(a->note);
	destlen++;

	if (!buf)
		return destlen;
	if (len < destlen)
		return 0;

	if (a->indefinite) {
		buf[0] = 0xff;
		buf[1] = 0xff;
	} else {
		set_short(buf,
			  ((a->due.tm_year - 4) << 9) | ((a->due.tm_mon +
							  1) << 5) | a->
			  due.tm_mday);
	}
	buf[2] = a->priority;
	if (a->complete) {
		buf[2] |= 0x80;
	}

	pos = 3;
	if (a->description) {
		strcpy((char *) buf + pos, a->description);
		pos += strlen(a->description) + 1;
	} else {
		buf[pos++] = 0;
	}

	if (a->note) {
		strcpy((char *) buf + pos, a->note);
		pos += strlen(a->note) + 1;
	} else {
		buf[pos++] = 0;
	}

	return pos;
}

/***********************************************************************
 *
 * Function:    unpack_ToDoAppInfo
 *
 * Summary:     Unpack the ToDo AppInfo block from the structure
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
unpack_ToDoAppInfo(struct ToDoAppInfo *ai, unsigned char *record, int len)
{
	int i;
	unsigned char *start = record;

	i = unpack_CategoryAppInfo(&ai->category, record, len);
	if (!i)
		return 0;
	record += i;
	len -= i;
	if (len < 4)
		return 0;
	ai->dirty = get_short(record);
	record += 2;
	ai->sortByPriority = get_byte(record);
	record += 2;
	return (record - start);
}

/***********************************************************************
 *
 * Function:    pack_ToDoAppInfo
 *
 * Summary:     Pack the AppInfo block/record back into the structure
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pack_ToDoAppInfo(struct ToDoAppInfo *ai, unsigned char *record, int len)
{
	int i;
	unsigned char *start = record;

	i = pack_CategoryAppInfo(&ai->category, record, len);
	if (!record)
		return i + 4;
	if (!i)
		return 0;
	record += i;
	len -= i;
	if (len < 4)
		return 0;
	set_short(record, ai->dirty);
	set_byte(record + 2, ai->sortByPriority);
	set_byte(record + 3, 0);	/* gapfill */
	record += 4;

	return (record - start);
}
