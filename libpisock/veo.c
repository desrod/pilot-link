/*
 * veo.c:  Translate veo traveler data formats
 *
 * Copyright (c) 2002, Angus Ainslie
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

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include "pi-macros.h"
#include "pi-veo.h"

/***********************************************************************
 *
 * Function:    free_Veo
 *
 * Summary:     Free the memory and filehandle from the record alloc. 
 *
 ***********************************************************************/
void free_Veo( struct Veo *a )
{
}

/***********************************************************************
 *
 * Function:    unpack_Veo
 *
 * Summary:     Unpack the Veo structure into records we can chew on
 *
 ***********************************************************************/
int unpack_Veo(struct Veo *v, unsigned char *buffer, int len)
{
   unsigned char *start = buffer;
   
   // consume unknown
   buffer += 1;
   v->quality = (unsigned char) get_byte(buffer);
   buffer += 1;
   v->resolution = (unsigned char) get_byte(buffer);
   buffer += 1;
   // consume 12 more unknowns
   buffer += 12;
   v->picnum = (unsigned long int) get_long(buffer);
   buffer += 4;
   v->day = (unsigned short int) get_short(buffer);
   buffer += 2;
   v->month = (unsigned short int) get_short(buffer);
   buffer += 2;
   v->year = (unsigned short int) get_short(buffer);
   buffer += 2;

   if( v->resolution == 0 )
     {
	v->width = 640;
	v->height = 480;
     }
   else if( v->resolution == 1 )
     {
	v->width = 320;
	v->height = 240;
     }
   else 
     fprintf( stderr, "unknown resolution\n" );
	
   return ( buffer - start );	/* FIXME: return real length */
}

/***********************************************************************
 *
 * Function:    pack_Veo
 *
 * Summary:     Pack the Veo records into a structure
 *
 ***********************************************************************/
int pack_Veo(struct Veo *a, unsigned char *buf, int len)
{
   return( 0 );
}

/***********************************************************************
 *
 * Function:    unpack_VeoAppInfo
 *
 * Summary:     Unpack the Veo AppInfo block from the structure
 *
 ***********************************************************************/
int unpack_VeoAppInfo(struct VeoAppInfo *ai, unsigned char *record, int len)
{
	int 	i;
	unsigned char *start = record;

	i = unpack_CategoryAppInfo( &ai->category, record, len );
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
 * Function:    pack_VeoAppInfo
 *
 * Summary:     Pack the AppInfo block/record back into the structure
 *
 ***********************************************************************/
int
pack_VeoAppInfo(struct VeoAppInfo *vai, unsigned char *record, int len)
{
	int 	i;
	unsigned char *start = record;

	i = pack_CategoryAppInfo(&vai->category, record, len);
	if (!record)
		return i + 4;
	if (!i)
		return 0;
	record += i;
	len -= i;
	if (len < 4)
		return 0;
	set_short(record, vai->dirty);
	set_byte(record + 2, vai->sortByPriority);
	set_byte(record + 3, 0);	/* gapfill */
	record += 4;

	return (record - start);
}
