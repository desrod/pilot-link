/*
 * notepad.c:  Translate Palm NotePad database into generic picture format
 *
 * Copyright (c) 2002, Angus Ainslie
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include "getopt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-notepad.h"
#include "pi-dlp.h"
#include "pi-file.h"
#include "pi-header.h"

#ifdef HAVE_PNG
#include "png.h"
#if (PNG_LIBPNG_VER < 10201)
 #define png_voidp_NULL (png_voidp)NULL
 #define png_error_ptr_NULL (png_error_ptr)NULL
#endif
#endif


int pilot_connect(const char *port);
static void print_help(char *progname);
void write_ppm( FILE *f, struct NotePad *n );

#ifdef HAVE_PNG
void write_png( FILE *f, struct NotePad *n );
#endif

struct option options[] = {
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
	{"port",        required_argument, NULL, 'p'},
	{"list",        no_argument,       NULL, 'l'},
	{NULL,          0,                 NULL, 0}
};

static const char *optstring = "hvp:l";

static void print_help(char *progname)
{
	printf
	    ("   Syncronize your NotePad database with your desktop or server machine\n"
	     "   Usage: %s -p /dev/pilot [options]\n\n" "   Options:\n"
	     "     -p <port>      Use device file <port> to communicate with Palm\n"
	     "     -l             List Notes on device\n" 
	     "     -h             Display this information\n\n"
	     "   Examples: %s -p /dev/pilot\n\n"
	     , progname, progname);
	return;
}

static const char *fmt_date ( noteDate_t d )
{
   
   static char buf[24];
   sprintf (buf, "%d-%02d-%02d %02d:%02d:%02d",
	    d.year, d.month, d.day, d.hour, d.min, d.sec);
   return buf;
   
}

void write_ppm( FILE *f, struct NotePad *n )
{
   int i,j,k,datapoints = 0;
   unsigned long black = 0;
   unsigned long white = 0xFFFFFFFF;
   
   fprintf( f, "P6\n# " );
   
   if( n->name != NULL )
     fprintf( f, "%s (created on %s)\n", n->name, fmt_date( n->createDate ));
   else
     fprintf( f, "%s\n", fmt_date( n->createDate ));

   // NotePad says that the width is only 152 but it encodes 160 bits 
   // of data! - AA
   fprintf( f, "%ld %ld\n255\n",
	    n->body.width+8, n->body.height );

   for( i=0; i<n->body.dataLen/2; i++ )
     {
	datapoints += n->data[i].repeat;
	
	for( j=0; j<n->data[i].repeat; j++ )
	  {
	  
	     for( k=0; k<8; k++ )
	       {
		  if( n->data[i].data & 1<<(7-k) )
		    fwrite( &black, 3, 1, f );
		  else
		    fwrite( &white, 3, 1, f );
	       }
	  }
     }
   
}

#ifdef HAVE_PNG
void write_png( FILE *f, struct NotePad *n )
{
   int i,j,k = 0, width;
   png_structp png_ptr;
   png_infop info_ptr;
   png_bytep row;
   
   width = n->body.width + 8;
   
   png_ptr = png_create_write_struct
     ( PNG_LIBPNG_VER_STRING, png_voidp_NULL,  
       png_error_ptr_NULL, png_error_ptr_NULL);

   if(!png_ptr)
     return;
   
   info_ptr = png_create_info_struct(png_ptr);
   if( !info_ptr )
     {
	png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	return;
     }
   
   if( setjmp( png_jmpbuf( png_ptr )))
     {
	png_destroy_write_struct( &png_ptr, &info_ptr );
	fclose( f );
	return;
     }
   
   png_init_io( png_ptr, f );
   
   png_set_IHDR( png_ptr, info_ptr, width, n->body.height,
		1, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,  PNG_FILTER_TYPE_DEFAULT );

   png_write_info( png_ptr, info_ptr );

   row = (png_bytep)malloc( width/8 * sizeof( png_byte ));
   
   if( NULL == row )
     return;
   
   for( i=0, k=0; i<n->body.dataLen/2; i++ )
     for( j=0; j<n->data[i].repeat; j++ )
       {
	  row[k] = n->data[i].data ^ 0xFF;

	  if( ++k >= width/8 )
	    {
	       png_write_row( png_ptr, row );
	       k = 0;
	       png_write_flush(png_ptr);	  
	    }
       }
   
   png_write_end(png_ptr, info_ptr);
   
   free( row );

   row = NULL;

   png_destroy_write_struct( &png_ptr, &info_ptr );
   
}
#endif

int main(int argc, char *argv[])
{
	int	c,	/* switch */
		db,
		i,
		sd	= -1,
		action 	= NOTEPAD_ACTION_OUTPUT;
   
#ifdef HAVE_PNG
   int type = NOTE_OUT_PNG;
#else
   int type = NOTE_OUT_PPM;
#endif
   
	char 	*progname 	= argv[0],
		*port 		= NULL,
		*filename 	= NULL,
		*ptr,
		extension[8];

   struct 	PilotUser User;
   struct 	pi_file *pif 	= NULL;
   struct 	NotePadAppInfo nai;
   unsigned char buffer[0xffff];
   
   while ((c = getopt_long(argc, argv, optstring, options, NULL)) != -1) 
     {
	switch (c) {
	   case 'h':
	     print_help(progname);
	     exit(0);
	   case 'v':
	     print_splash(progname);
	     return 0;
	   case 'p':
	     port = optarg;
	     filename = NULL;
	     break;
//	   case 'f':
//	     filename = optarg;
//	     break;
	   case 'l':
	     action = NOTEPAD_ACTION_LIST;
	     break;
	  }
     }
   
   sd = pilot_connect(port);
   if (sd < 0)
     goto error;   
   
   if (dlp_ReadUserInfo(sd, &User) < 0)
     goto error_close;
   
   /* Open the NotePad database, store access handle in db */
   if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "npadDB", &db) < 0) 
     {
	puts("Unable to open NotePadDB");
	dlp_AddSyncLogEntry(sd, "Unable to open NotePadDB.\n");
	exit(1);
     }
   
   dlp_ReadAppBlock(sd, db, 0, buffer, 0xffff);
   
#if 0
   // Not going use this right now but it might come back 
   // later - AA
   if (filename) 
     {
	int 	len;
	
	pif = pi_file_open(filename);
	if (!pif) 
	  {
	     perror("pi_file_open");
	     exit(1);
	  }
	
	sd = pi_file_get_app_info(pif, (void *) &ptr, &len);
	if (sd == -1) 
	  {
	     perror("pi_file_get_app_info");
	     exit(1);
	  }
	
	memcpy(buffer, ptr, len);
     }
#endif
   
   unpack_NotePadAppInfo( &nai, buffer, 0xffff);
   
   for (i = 0;; i++) 
     {
	char fname[FILENAME_MAX];
	FILE *f;
	int 	attr,
	  category,
	  len;
	struct 	NotePad n;
	
	if (port) 
	  {
	     len = dlp_ReadRecordByIndex(sd, db, i, buffer, 0, 0,
				       &attr, &category);

	     if (len < 0)
	       break;
	  }
	
	if (filename) 
	  {
	     if (pi_file_read_record
		 (pif, i, (void *) &ptr, &len, &attr, &category, 0))
	       break;
	     
	     memcpy(buffer, ptr, len);
	  }
	
	/* Skip deleted records */
	if ((attr & dlpRecAttrDeleted) || (attr & dlpRecAttrArchived))
	  continue;
	
	unpack_NotePad( &n, buffer, len);

	switch( action )
	  {
	   case NOTEPAD_ACTION_LIST:
	     if( n.flags & NOTEPAD_FLAG_NAME )
	       printf( "Name: %s\n", n.name );

	     printf( "Category: %s\n", nai.category.name[category] );
	     printf( "Created: %s\n", fmt_date( n.createDate ));
	     printf( "Changed: %s\n", fmt_date( n.changeDate ));
	       
	     if( n.flags & NOTEPAD_FLAG_ALARM )
	       printf( "Alarm set for: %s\n", fmt_date( n.alarmDate ));
	     else
	       printf( "Alarm set for: none\n" );
	       
	     printf( "Pictue: " );
	     
	     if( n.flags & NOTEPAD_FLAG_BODY )
	       printf( "yes\n\n" );
	     else
	       printf( "no\n\n" );
	       
	     break;
	     
	   case NOTEPAD_ACTION_OUTPUT:

	     if( type == NOTE_OUT_PNG )
	       sprintf( extension, "png" );
	     else if( type == NOTE_OUT_PPM )
	       sprintf( extension, "ppm" );
	     
	     if( n.flags & NOTEPAD_FLAG_NAME )
	       sprintf( fname, "%s.%s", n.name, extension );
	     else
	       sprintf( fname, "%d.%s", i, extension );
	     
	     printf ("Generating %s...\n", fname);
	     
	     f = fopen (fname, "wb");
	     if( f ) 
	       {
		  switch( type )
		    {
		     case NOTE_OUT_PPM:
		       write_ppm( f, &n );
		       break;
		       
		     case NOTE_OUT_PNG:
		       write_png( f, &n );
		       break;
		    }
		  		       
		  fclose (f);
		  
	       }
	     else
	       fprintf (stderr, "%s: can't write to %s\n", progname, fname);
	     
	     free_NotePad( &n );

	  }
     }
   

   if (port) 
     {
	/* Close the database */
	dlp_CloseDB(sd, db);
	dlp_AddSyncLogEntry(sd, "Successfully read NotePad from Palm.\n"
			    "Thank you for using pilot-link.");
	dlp_EndOfSync(sd, 0);
	pi_close(sd);
     } 
   else if (filename) 
     {
	pi_file_close(pif);
     }
   
   return 0;
   
   error_close:
   pi_close(sd);
   
error:
   return -1;
   
}