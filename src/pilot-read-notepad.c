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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "getopt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pi-source.h"
#include "pi-notepad.h"
#include "pi-file.h"
#include "pi-header.h"

#ifdef HAVE_PNG
#include "png.h"
#if (PNG_LIBPNG_VER < 10201)
 #define png_voidp_NULL (png_voidp)NULL
 #define png_error_ptr_NULL (png_error_ptr)NULL
#endif
#endif

/* Declare prototypes */
static void display_help(char *progname);
void print_splash(char *progname);
int pilot_connect(char *port);
char *progname;

int protect_files(char *name, char *extension);
void write_ppm( FILE *f, struct NotePad *n );
void output_picture( int type, struct NotePad n );
void print_note_info( struct NotePad n, struct NotePadAppInfo nai, int category );
void write_png_v2( FILE *f, struct NotePad *n );

#ifdef HAVE_PNG
void write_png( FILE *f, struct NotePad *n );
#endif

struct option options[] = {
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
	{"port",        required_argument, NULL, 'p'},
	{"list",        no_argument,       NULL, 'l'},
	{"type",        required_argument, NULL, 't'},
	{NULL,          0,                 NULL, 0}
};

static const char *optstring = "hvp:lt:";


/***********************************************************************
 *
 * Function:    display_help
 *
 * Summary:     
 *
 * Parameters:  None
 *
 * Return:      Nothing
 *
 ***********************************************************************/
static void display_help(char *progname)
{
	printf("   Syncronize your NotePad database with your desktop machine\n\n");
	printf("   Usage: %s -p /dev/pilot [options]\n\n", progname);
	printf("   Options:\n");
	printf("     -p, --port <port>       Use device file <port> to communicate with Palm\n");
	printf("     -h, --help              Display help information for %s\n", progname);
	printf("     -v, --version           Display %s version information\n\n", progname);
	printf("     -l                      List Notes on device\n");
	printf("     -t                      Specify picture output type\n");
	printf("                             either \"ppm\" or \"png\"\n\n");
	printf("   Examples: %s -p /dev/pilot -l -t png\n\n", progname);

	return;
}


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
 * Function:    protect_files
 *
 * Summary:     
 *
 * Parameters:  None
 *
 * Return:      Nothing
 *
 ***********************************************************************/
int protect_files( char *name, char *extension )
{
   char *save_name, 
	c = 1;
   
   save_name = strdup( name );
   
   if( NULL == save_name )
     {
	printf( "Failed to generate filename %s%s\n", name, extension );
	return( 0 );
     }
	
   sprintf( name, "%s%s", save_name, extension );
   
   while( access( name, F_OK ) == 0 )
     {
	sprintf( name, "%s_%02d%s", save_name, c, extension );

	c++;
	
	if( c == 'z' + 1 )
	  c = 'A';
	
	if( c == 'Z' + 1 )
	  {
	     printf( "Failed to generate filename %s\n", name );
	     return( 0 );
	  }
     }
   
   free( save_name );
   
   return( 1 );
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
        	
   protect_files( fname, extension );
   
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
   else
     fprintf (stderr, "%s: can't write to %s\n", progname, fname);
   
   free_NotePad( &n );
}


int main(int argc, char *argv[])
{
   int	c,	/* switch */
     db,
     i,
     sd	= -1,
     action 	= NOTEPAD_ACTION_OUTPUT;
   
   int type = NOTE_OUT_PPM;
   
   char		*port 		= NULL; 	/* *filename = NULL, *ptr */
   
   struct 	PilotUser User;
   struct 	NotePadAppInfo nai;
   unsigned char buffer[0xffff];

   progname = argv[0];

   while ((c = getopt_long(argc, argv, optstring, options, NULL)) != -1) 
     {
	switch (c) {
	 case 'h':
	   display_help(progname);
	   return 0;
	 case 'v':
	   print_splash(progname);
	   return 0;
	 case 'p':
	   port = optarg;
/* 	   filename = NULL; */
	   break;
	 case 't':
	   if( !strncmp( "png", optarg, 3 ))
	     {
#ifdef HAVE_PNG	     
		type = NOTE_OUT_PNG;
#else
		fprintf( stderr, "read-notepad was built without png support\n" );
#endif
	     }
	   else if( !strncmp( "ppm", optarg, 3 ))
	     {
		type = NOTE_OUT_PPM;
	     }
	   else
	     {
		fprintf( stderr, "Unknown output type defaulting to ppm\n" );
		type = NOTE_OUT_PPM;
	     }
	   
/*	     filename = NULL; */

	   break;
#if 0	   
	 case 'f':
	   filename = optarg;
	   break;
#endif
	 case 'l':
	   action = NOTEPAD_ACTION_LIST;
	   break;
	}
     }
   
   sd = pilot_connect(port);

   if( sd < 0 )
     goto error;   
   
   if (dlp_ReadUserInfo(sd, &User) < 0)
     goto error_close;
   
   /* Open the NotePad database, store access handle in db */
   if( dlp_OpenDB(sd, 0, 0x80 | 0x40, "npadDB", &db ) < 0) 
     {
	puts("Unable to open NotePadDB");
	dlp_AddSyncLogEntry(sd, "Unable to open NotePadDB.\n");
	exit(EXIT_FAILURE);
     }
   
   dlp_ReadAppBlock(sd, db, 0, buffer, 0xffff);
   
   unpack_NotePadAppInfo( &nai, buffer, 0xffff);
   
   for (i = 0;; i++) 
     {
	int 	attr,
		category,
		len = 0;

	struct 	NotePad n;
	
	if( sd ) 
	  {
	     len = dlp_ReadRecordByIndex(sd, db, i, buffer, 0, 0,
				       &attr, &category);
	     
	     if (len < 0)
	       break;
	  }
	
	/* Skip deleted records */
	if ((attr & dlpRecAttrDeleted) || (attr & dlpRecAttrArchived))
	  continue;
	
	unpack_NotePad( &n, buffer, len);

	switch( action )
	  {
	   case NOTEPAD_ACTION_LIST:
	     print_note_info( n, nai, category );
	     printf( "\n" );
	     break;
	     
	   case NOTEPAD_ACTION_OUTPUT:
	     print_note_info( n, nai, category );
	     output_picture( type, n );
	     printf( "\n" );
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

   return 0;
   
error_close:
	pi_close(sd);
   
error:
	return -1;
   
}
