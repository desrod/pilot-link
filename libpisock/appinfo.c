/*
 * appinfo.c:  Translate Pilot category info
 *
 * Copyright (c) 1996, 1997, Kenneth Albanowski
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
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-appinfo.h"

/***********************************************************************
 *
 * Function:    unpack_CategoryAppInfo
 *
 * Summary:     Unpack the AppInfo block into the structure
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
unpack_CategoryAppInfo(struct CategoryAppInfo *ai, unsigned char *record,
		       int len)
{
	int i;
	int r;

	if (len < 2 + 16 * 16 + 16 + 4)
		return 0;
	r = get_short(record);
	for (i = 0; i < 16; i++) {
		if (r & (1 << i))
			ai->renamed[i] = 1;
		else
			ai->renamed[i] = 0;
	}
	record += 2;
	for (i = 0; i < 16; i++) {
		memcpy(ai->name[i], record, 16);
		record += 16;
	}
	memcpy(ai->ID, record, 16);
	record += 16;
	ai->lastUniqueID = get_byte(record);
	record += 4;
	return 2 + 16 * 16 + 16 + 4;
}

/***********************************************************************
 *
 * Function:    pack_CategoryAppInfo
 *
 * Summary:     Pack the AppInfo structure 
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pack_CategoryAppInfo(struct CategoryAppInfo *ai, unsigned char *record,
		     int len)
{
	int i;
	int r;
	unsigned char *start = record;

	if (!record) {
		return 2 + 16 * 16 + 16 + 4;
	}
	if (len < (2 + 16 * 16 + 16 + 4))
		return 0;	/* not enough room */
	r = 0;
	for (i = 0; i < 16; i++) {
		if (ai->renamed[i])
			r |= (1 << i);
	}
	set_short(record, r);
	record += 2;
	for (i = 0; i < 16; i++) {
		memcpy(record, ai->name[i], 16);
		record += 16;
	}
	memcpy(record, ai->ID, 16);
	record += 16;
	set_byte(record, ai->lastUniqueID);
	record++;
	set_byte(record, 0);		/* gapfill */
	set_short(record + 1, 0);	/* gapfill */
	record += 3;

	return (record - start);
}
