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

#include "pi-macros.h"
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
	int 	idx,
		rec;

	if (len < 2 + 16 * 16 + 16 + 4)
		return 0;
	rec = get_short(record);
	for (idx = 0; idx < 16; idx++) {
		if (rec & (1 << idx))
			ai->renamed[idx] = 1;
		else
			ai->renamed[idx] = 0;
	}
	record += 2;
	for (idx = 0; idx < 16; idx++) {
		memcpy(ai->name[idx], record, 16);
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
	int 	idx,
		rec;
	
	unsigned char *start = record;

	if (!record) {
		return 2 + 16 * 16 + 16 + 4;
	}
	if (len < (2 + 16 * 16 + 16 + 4))
		return 0;	/* not enough room */
	rec = 0;
	for (idx = 0; idx < 16; idx++) {
		if (ai->renamed[idx])
			rec |= (1 << idx);
	}
	set_short(record, rec);
	record += 2;
	for (idx = 0; idx < 16; idx++) {
		memcpy(record, ai->name[idx], 16);
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
