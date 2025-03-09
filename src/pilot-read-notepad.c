/*
 * $Id$ 
 *
 * pilot-read-notepad.c:  Translate Palm NotePad database into generic
 *                        picture format
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
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pi-source.h"
#include "pi-notepad.h"
#include "pi-file.h"
#include "pi-header.h"
#include "pi-userland.h"

#ifdef HAVE_PNG
#include "png.h"
#endif

const char *progname;

#ifdef HAVE_PNG
void write_png( FILE *f, struct NotePad *n );
#endif




/***********************************************************************
 *
 * Function:    fmt_date
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Return:      Nothing
 *
 ***********************************************************************/
static const char *fmt_date ( noteDate_t d )
{

   static char buf[24];
   sprintf (buf, "%d-%02d-%02d %02d:%02d:%02d",
	    d.year, d.month, d.day, d.hour, d.min, d.sec);
   return buf;

}


/***********************************************************************
 *
 * Function:    write_ppm
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Return:      Nothing
 *
 ***********************************************************************/
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

   /* NotePad says that the width is only 152 but it encodes 160 bits */
   /* of data! - AA */
   fprintf( f, "%ld %ld\n255\n",
	    n->body.width+8, n->body.height );

   if( n->body.dataType == NOTEPAD_DATA_BITS )
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
   else
     for( i=0; i<n->body.dataLen/2; i++ )
       {
	  for( k=0; k<8; k++ )
	    {
	       if( n->data[i].repeat & 1<<(7-k) )
		 fwrite( &black, 3, 1, f );
	       else
		 fwrite( &white, 3, 1, f );
	    }
	  for( k=0; k<8; k++ )
	    {
	       if( n->data[i].data & 1<<(7-k) )
		 fwrite( &black, 3, 1, f );
	       else
		 fwrite( &white, 3, 1, f );
	    }
       }

}


/***********************************************************************
 *
 * Function:    write_png
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Return:      Nothing
 *
 ***********************************************************************/
#ifdef HAVE_PNG
void write_png( FILE *f, struct NotePad *n )
{
   int i,j,k = 0, width;
   png_structp png_ptr;
   png_infop info_ptr;
   png_bytep row;

   width = n->body.width + 8;

   png_ptr = png_create_write_struct
     ( PNG_LIBPNG_VER_STRING, NULL,
       NULL, NULL);

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

   if( n->body.dataType == NOTEPAD_DATA_BITS )
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
   else
     for( i=0, k=0; i<n->body.dataLen/2; i++ )
       {
	  row[k] = n->data[i].repeat ^ 0xFF;
	  row[k+1] = n->data[i].data ^ 0xFF;
	  k += 2;
	  if( k >= width/8 )
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


/***********************************************************************
 *
 * Function:    write_png_v2
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Return:      Nothing
 *
 ***********************************************************************/
void write_png_v2( FILE *f, struct NotePad *n )
{
   if( n->body.dataType != NOTEPAD_DATA_PNG )
     {
	fprintf( stderr, "Bad data Type" );
	return;
     }

   fwrite( n->data, n->body.dataLen, 1, f );
   fflush( f );
}


/***********************************************************************
 *
 * Function:    print_note_info
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Return:      Nothing
 *
 ***********************************************************************/
void print_note_info( struct NotePad n, struct NotePadAppInfo nai, int category )
{
   if( n.flags & NOTEPAD_FLAG_NAME )
     printf( "Name: %s\n", n.name );

   printf( "Category: %s\n", nai.category.name[category] );
   printf( "Created: %s\n", fmt_date( n.createDate ));
   printf( "Changed: %s\n", fmt_date( n.changeDate ));

   if( n.flags & NOTEPAD_FLAG_ALARM )
     printf( "Alarm set for: %s\n", fmt_date( n.alarmDate ));
   else
     printf( "Alarm set for: none\n" );

   printf( "Picture: " );

   if( n.flags & NOTEPAD_FLAG_BODY )
     printf( "yes\n" );
   else
     printf( "no\n" );

   printf( "Picture version: %ld\n", n.body.dataType );

}

/***********************************************************************
 *
 * Function:    output_picture
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Return:      Nothing
 *
 ***********************************************************************/
void output_picture( int type, struct NotePad n )
{
   char fname[FILENAME_MAX];
   FILE *f;
   char extension[8];
   static int i = 1;

   if( n.body.dataType == NOTEPAD_DATA_PNG )
     type = NOTE_OUT_PNG;

   if( n.flags & NOTEPAD_FLAG_NAME )
     {
	if( type == NOTE_OUT_PNG )
	  sprintf( extension, ".png" );
	else if( type == NOTE_OUT_PPM )
	  sprintf( extension, ".ppm" );

	sprintf( fname, "%s", n.name );
     }
   else
     {
	if( type == NOTE_OUT_PNG )
	  sprintf( extension, "_np.png" );
	else if( type == NOTE_OUT_PPM )
	  sprintf( extension, "_np.ppm" );

	sprintf( fname, "%4.4d", i++ );
     }

	if (plu_protect_files( fname, extension, sizeof(fname) ) < 1) {
		goto cleanup;
	}

   printf ("Generating %s...\n", fname);

   f = fopen (fname, "wb");
   if( f )
     {
	switch( n.body.dataType )
	  {
	   case NOTEPAD_DATA_PNG:
	     fprintf( stderr, "Notepad data version 2 defaulting to png output.\n" );
	     write_png_v2( f, &n );
	     break;

	   case NOTEPAD_DATA_BITS:
	   case NOTEPAD_DATA_UNCOMPRESSED:
	     switch( type )
	       {
		case NOTE_OUT_PPM:
		  write_ppm( f, &n );
		  break;

		case NOTE_OUT_PNG:
#ifdef HAVE_PNG
		  write_png( f, &n );
#else
		  fprintf( stderr, "read-notepad was built without png support\n" );
#endif
		  break;
	       }
	     break;

	   default:
	     fprintf( stderr, "Picture version is unknown - unable to convert\n" );
	  }

	fclose (f);

     }
	else {
		fprintf (stderr, "Can't write to %s\n", fname);
	}

cleanup:
	free_NotePad( &n );
}


int main(int argc, const char *argv[])
{
   int	c,	/* switch */
     db,
     i,
     sd	= -1,
     action 	= NOTEPAD_ACTION_OUTPUT;

   int type = NOTE_OUT_PPM;

   const char
	*typename	= 
#ifdef HAVE_PNG
	"png"
#else
	"ppm"
#endif
	;

   struct 	PilotUser User;
   struct 	NotePadAppInfo nai;
   pi_buffer_t *buffer;

   poptContext pc;

   struct poptOption options[] = {
   	USERLAND_RESERVED_OPTIONS
        {"list", 'l', POPT_ARG_VAL, &action, NOTEPAD_ACTION_LIST, "List Notes on device", NULL},
        {"type", 't', POPT_ARG_STRING, &typename, 0, "Specify picture output type, either \"ppm\" or \"png\"", "type"},
        POPT_TABLEEND
   };

	pc = poptGetContext("read-notepad", argc, argv, options, 0);
	poptSetOtherOptionHelp(pc,"\n\n"
		"   Copy or list your NotePad database.\n\n");


	if (argc<2) {
		poptPrintUsage(pc,stderr,0);
		return 1;
	}
	while ((c = poptGetNextOpt(pc)) >= 0) {
		fprintf(stderr,"   ERROR: Unhandled option %d.\n",c);
		return 1;
	}
	if (c < -1) {
		plu_badoption(pc,c);
	}

	if( !strncmp( "png", typename, 3 ))
	{
#ifdef HAVE_PNG
		type = NOTE_OUT_PNG;
#else
		fprintf( stderr, "   WARNING: read-notepad was built without png support,\n"
			"            using ppm instead.\n" );
		type = NOTE_OUT_PPM;
#endif
	}
	else if( !strncmp( "ppm", typename, 3 ))
	{
		type = NOTE_OUT_PPM;
	}
	else
	{
		fprintf( stderr, "   WARNING: Unknown output type defaulting to ppm\n" );
		type = NOTE_OUT_PPM;
	}

	sd = plu_connect();

   if( sd < 0 )
     goto error;

   if (dlp_ReadUserInfo(sd, &User) < 0)
     goto error_close;

   /* Open the NotePad database, store access handle in db */
   if( dlp_OpenDB(sd, 0, 0x80 | 0x40, "npadDB", &db ) < 0)
     {
	fprintf(stderr,"   ERROR: Unable to open NotePadDB on Palm.\n");
	dlp_AddSyncLogEntry(sd, "Unable to open NotePadDB.\n");
	exit(EXIT_FAILURE);
     }

   buffer = pi_buffer_new (0xffff);

   dlp_ReadAppBlock(sd, db, 0, 0xffff, buffer);
   unpack_NotePadAppInfo( &nai, buffer->data, buffer->used);

   for (i = 0;; i++)
     {
	int 	attr,
		category,
		len = 0;

	struct 	NotePad n;

	if( sd )
	  {
	     len = dlp_ReadRecordByIndex(sd, db, i, buffer, 0,
				       &attr, &category);

	     if (len < 0)
	       break;
	  }
	else
		pi_buffer_clear(buffer);

	/* Skip deleted records */
	if ((attr & dlpRecAttrDeleted) || (attr & dlpRecAttrArchived))
	  continue;

	unpack_NotePad( &n, buffer->data, buffer->used);

	switch( action )
	  {
	   case NOTEPAD_ACTION_LIST:
	     print_note_info( n, nai, category );
	     printf( "\n" );
	     break;

	   case NOTEPAD_ACTION_OUTPUT:
	     if (!plu_quiet) {
	        print_note_info( n, nai, category );
	     }
	     output_picture( type, n );
	     if (!plu_quiet) {
	        printf( "\n" );
	     }
	     break;
	  }
     }


   if( sd ) {
	/* Close the database */
	dlp_CloseDB( sd, db );
	dlp_AddSyncLogEntry( sd, "Successfully read NotePad from Palm.\n\n"
			    "Thank you for using pilot-link.");
	dlp_EndOfSync( sd, 0 );
	pi_close(sd);
     }

   pi_buffer_free (buffer);

   return 0;

error_close:
	pi_close(sd);

error:
	return -1;

}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
/* ex: set tabstop=4 expandtab: */
/* Local Variables: */
/* indent-tabs-mode: t */
/* c-basic-offset: 8 */
/* End: */

