/*
 * notepad.c:  Translate Palm NotePad application data formats
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
#include "pi-notepad.h"

/***********************************************************************
 *
 * Function:    free_NotePad
 *
 * Summary:     Free the memory and filehandle from the record alloc. 
 *
 ***********************************************************************/
void free_NotePad( struct NotePad *a )
{
   if( a->flags & NOTEPAD_FLAG_NAME )
     {
/*	fprintf( stderr, "Freeing name: %s\n", a->name ); */
	free(a->name);
     }
   
   if( a->flags & NOTEPAD_FLAG_BODY )
     {
/*	fprintf( stderr, "Freeing data\n" ); */
	free(a->data);
     }
   
}

/***********************************************************************
 *
 * Function:    unpack_NotePad
 *
 * Summary:     Unpack the NotePad structure into records we can chew on
 *
 ***********************************************************************/
int unpack_NotePad(struct NotePad *a, unsigned char *buffer, int len)
{
   unsigned char *start = buffer;
   
   a->createDate.sec = (unsigned short int) get_short(buffer);
   buffer += 2;
   a->createDate.min = (unsigned short int) get_short(buffer);
   buffer += 2;
   a->createDate.hour = (unsigned short int) get_short(buffer);
   buffer += 2;
   a->createDate.day = (unsigned short int) get_short(buffer);
   buffer += 2;
   a->createDate.month = (unsigned short int) get_short(buffer);
   buffer += 2;
   a->createDate.year = (unsigned short int) get_short(buffer);
   buffer += 2;

   a->createDate.s = (unsigned short int) get_short(buffer);
   buffer += 2;

   a->changeDate.sec = (unsigned short int) get_short(buffer);
   buffer += 2;
   a->changeDate.min = (unsigned short int) get_short(buffer);
   buffer += 2;
   a->changeDate.hour = (unsigned short int) get_short(buffer);
   buffer += 2;
   a->changeDate.day = (unsigned short int) get_short(buffer);
   buffer += 2;
   a->changeDate.month = (unsigned short int) get_short(buffer);
   buffer += 2;
   a->changeDate.year = (unsigned short int) get_short(buffer);
   buffer += 2;

   a->changeDate.s = (unsigned short int) get_short(buffer);
   buffer += 2;

   a->flags = (unsigned short int) get_short(buffer);
   buffer += 2;

/*   fprintf( stderr, "flags: 0x%x\n", a->flags ); */
   
   if( a->flags & NOTEPAD_FLAG_ALARM )
     {
/*	fprintf( stderr, "Getting Alarm\n" ); */
	a->alarmDate.sec = (unsigned short int) get_short(buffer);
	buffer += 2;
	a->alarmDate.min = (unsigned short int) get_short(buffer);
	buffer += 2;
	a->alarmDate.hour = (unsigned short int) get_short(buffer);
	buffer += 2;
	a->alarmDate.day = (unsigned short int) get_short(buffer);
	buffer += 2;
	a->alarmDate.month = (unsigned short int) get_short(buffer);
	buffer += 2;
	a->alarmDate.year = (unsigned short int) get_short(buffer);
	buffer += 2;

	a->alarmDate.s = (unsigned short int) get_short(buffer);
	buffer += 2;
     }
  
   if( a->flags & NOTEPAD_FLAG_NAME )
     {
/*	fprintf( stderr, "Getting Name\n" ); */
	a->name = strdup((char *) buffer);
   
	buffer += strlen( a->name ) + 1;
	
	if( (strlen( a->name ) + 1)%2 == 1)
	  buffer++;
	
     }
   else 
     {
	a->name = NULL;
     }
   

   if( a->flags & NOTEPAD_FLAG_BODY )
     {
/*	fprintf( stderr, "Getting Body\n" ); */
	a->body.bodyLen = get_long( buffer );
	buffer += 4;
   
	a->body.width = get_long( buffer );
	buffer += 4;
   
	a->body.height = get_long( buffer );
	buffer += 4;
   
	a->body.l1 = get_long( buffer );
	buffer += 4;
   
	a->body.dataType = get_long( buffer );
	buffer += 4;

	a->body.dataLen = get_long( buffer );
	buffer += 4;
   
	a->data = malloc( a->body.dataLen );

	if( a->data == NULL )
	  {
	     fprintf( stderr, "Body data alloc failed\n" );
	     return( 0 );
	  }
	     
	memcpy( a->data, buffer, a->body.dataLen );

     }
   
   return ( buffer - start );	/* FIXME: return real length */
}

/***********************************************************************
 *
 * Function:    pack_NotePad
 *
 * Summary:     Pack the NotePad records into a structure
 *
 ***********************************************************************/
int pack_NotePad(struct NotePad *a, unsigned char *buf, int len)
{
   return( 0 );
}

/***********************************************************************
 *
 * Function:    unpack_NotePadAppInfo
 *
 * Summary:     Unpack the NotePad AppInfo block from the structure
 *
 ***********************************************************************/
int unpack_NotePadAppInfo(struct NotePadAppInfo *ai, unsigned char *record, int len)
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
 * Function:    pack_NotePadAppInfo
 *
 * Summary:     Pack the AppInfo block/record back into the structure
 *
 ***********************************************************************/
int
pack_NotePadAppInfo(struct NotePadAppInfo *ai, unsigned char *record, int len)
{
	int 	i;
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
