/* ex: set tabstop=4 expandtab: */
/*
 * read-veo.c
 *
 * Copyright (c) 2003-2004, Angus Ainslie
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
# include <config.h>
#endif

#include "getopt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "pi-source.h"
//#include "pi-veo.h"
#include "pi-file.h"
#include "pi-header.h"

#ifdef HAVE_PNG
# include "png.h"
# if (PNG_LIBPNG_VER < 10201)
#  define png_voidp_NULL (png_voidp)NULL
#  define png_error_ptr_NULL (png_error_ptr)NULL
# endif
#endif

#define pi_mktag(c1,c2,c3,c4) (((c1)<<24)|((c2)<<16)|((c3)<<8)|(c4))

#define OUT_PPM    1
#define OUT_PNG    2

struct option options[] =
{
	 {"help", no_argument, NULL, 'h'},
	 {"version", no_argument, NULL, 'v'},
	 {"port", required_argument, NULL, 'p'},
	 {"type", required_argument, NULL, 't'},
	 {NULL, 0, NULL, 0}
};

static const char *optstring = "hvp:t:";

/***********************************************************************
 *
 * Function:    display_help
 *
 * Summary:     Print out the --help options and arguments
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void display_help (char *progname)
{
   printf("   Syncronize your Veo Traveler databases with your desktop machine\n\n");
   printf("   Usage: %s -p /dev/pilot [options]\n\n", progname);
   printf("   Options:\n");
   printf("     -p, --port <port>       Use device file <port> to communicate with Palm\n");
   printf("     -h, --help              Display help information for %s\n", progname);
   printf("     -v, --version           Display %s version information\n\n", progname);
#ifdef HAVE_PNG
   printf("     -t, --type [type],      Specify picture output type (ppm or png)\n");
#endif
   printf("   Examples: %s -p /dev/pilot -l\n\n", progname);

   exit (0);
}

/***********************************************************************
 *
 * Function:    fmt_date
 *
 * Summary:     Format the output date on the images
 *
 * Parameters:
 *
 * Returns:
 *
static const char *fmt_date (struct Veo *v)
{
   static char buf[24];

   sprintf (buf, "%d-%02d-%02d", v->year, v->month, v->day);
   return buf;

}
 ***********************************************************************/

/***********************************************************************
 *
 * Function:	protect_files
 *
 * Summary:     Adjust output file name so as to not overwrite an exsisting 
 *              file
 *
 * Parameters:  filename and file extension
 *
 * Returns:     1 file name protected
 *              0 no alernate name found
 *
 ***********************************************************************/
int protect_files (char *name, char *extension)
{
   char *save_name, c = 1;

   save_name = strdup (name);

   if (NULL == save_name)
	 {
		printf ("Failed to generate filename %s%s\n", name, extension);
		return (0);
	 }

   sprintf (name, "%s%s", save_name, extension);

   while (access (name, F_OK) == 0)
	 {
		sprintf (name, "%s_%02d%s", save_name, c, extension);

		c++;

		if (c == 'z' + 1)
		  c = 'A';

		if (c == 'Z' + 1)
		  {
			 printf ("Failed to generate filename %s\n", name);
			 return (0);
		  }
	 }

   free (save_name);

   return (1);
}

#define max(a,b) (( a > b ) ? a : b )
#define min(a,b) (( a < b ) ? a : b )

/***********************************************************************
 *
 * Function:    write_png
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
#ifdef HAVE_PNG
void write_png (FILE * f, struct Veo *v, long flags)
{
   unsigned char outBuf[2560];
   int i;
   png_structp png_ptr;
   png_infop info_ptr;

   png_ptr = png_create_write_struct
	 (PNG_LIBPNG_VER_STRING, png_voidp_NULL,
	  png_error_ptr_NULL, png_error_ptr_NULL);

   if (!png_ptr)
	 return;

   info_ptr = png_create_info_struct (png_ptr);
   if (!info_ptr)
	 {
		png_destroy_write_struct (&png_ptr, (png_infopp) NULL);
		return;
	 }

   if (setjmp (png_jmpbuf (png_ptr)))
	 {
		png_destroy_write_struct (&png_ptr, &info_ptr);
		fclose (f);
		return;
	 }

   png_init_io (png_ptr, f);

   png_set_IHDR (png_ptr, info_ptr, v->width, v->height,
				 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
				 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

   png_write_info (png_ptr, info_ptr);

   for (i = 0; i < v->height; i++)
	 {
		Gen24bitRow (flags, i, v, outBuf);
		png_write_row (png_ptr, outBuf);
		png_write_flush (png_ptr);
	 }

   png_write_end (png_ptr, info_ptr);
   png_destroy_write_struct (&png_ptr, &info_ptr);

}
#endif
 ***********************************************************************/

/***********************************************************************
 *
 * Function:    WritePictures
 *
 * Summary:	FIXME
 *
 * Parameters:
 *
 * Returns:
 *
 ***********************************************************************/
void WritePictures (int sd, int db, int type )
{
   char fname[FILENAME_MAX];
   FILE *f;
   char extension[8];
   int i, len, idx = 0, w, h, recs, depth, imgNum = 0;
   pi_buffer_t *inBuf;

   char *pixelBuf;

//   unsigned char inBufB[320*320];
   unsigned long clut[256], magic;
   int attr, category, val;

   inBuf = pi_buffer_new (61440);

   sprintf (extension, ".ppm");

   if (sd)
     while( 1 )
       {
	  len =
	    dlp_ReadRecordByIndex (sd, db, idx, inBuf, 0, &attr, &category);
	  
	  if( len <= 0 )
	    {
	       /* EOF */
	       break;
	    }
	  
	  idx++;
	  h = ( inBuf[4] << 8 )+ inBuf[5];
	  w = ( inBuf[6] << 8 ) + inBuf[7];
	  recs = inBuf[9];
	  depth = inBuf[8];
	  magic = ((unsigned long *)inBuf)[0];
	  
	  if(  magic != 0xBECEDEFE )
	    {
	       /* no magic something bad happened */
	       fprintf( stderr, "No Magic !\n" );

	       exit( EXIT_FAILURE );
	    }
	  
	  pixelBuf = malloc( h * w * depth / 8 + 10 + 1024 );
	  
	  if( !pixelBuf )
	    {
	       fprintf( stderr, "Memory Allocation failed\n" );
	       return;
	    }
	  
	  memcpy( pixelBuf, &inBuf[10], len - 10 );
		  
	  for( i=1; i< recs; i++ )
	    {
	       len =
		 dlp_ReadRecordByIndex (sd, db, idx, inBuf, 0, 0, &attr, &category);
	       
	       memcpy( &pixelBuf[i*61440-10], inBuf, len );	  
	       
	       idx++;
	    }

	  sprintf (fname, "ScreenShot%d", ++imgNum );
	  
	  protect_files (fname, extension);
	  
	  printf ("Generating %s...\n", fname);
  	  fprintf( stderr, "h: %d w: %d recs: %d bit depth: %d\n", h, w, recs, depth );

	  f = fopen (fname, "wb");
	  
	  if( depth < 16 )
	    memcpy( clut, &inBuf[len-1024], 1024 );
	  
	  fprintf (f, "P6\n# ");
	  
//	       fprintf (f, "%s (created on %s)\n", name, fmt_date ( time( NULL )));
	  fprintf (f, "%s\n", fname );
	  
	  fprintf (f, "%d %d\n255\n", w, h );
	  
	  switch( depth )
	    {
	     case 8:
	       for( i = 0; i < h*w; i++)
		 {
		    fwrite( 1 + (char *)&clut[pixelBuf[i]], 1, 1, f);
		    fwrite( 2 + (char *)&clut[pixelBuf[i]], 1, 1, f);
		    fwrite( 3 + (char *)&clut[pixelBuf[i]], 1, 1, f);
		 }
	       break;
	       
	     case 16:
	       for( i = 0; i < h*w; i++)
		 {
		    val = pixelBuf[i*2] & 0xF8;
		    fwrite( &val, 1, 1, f);
		    val = (( pixelBuf[i*2] & 0x07 ) << 5 ) 
		      + (( pixelBuf[i*2+1] & 0xE0 ) >> 3 );
		    fwrite( &val, 1, 1, f);
		    val = ( pixelBuf[i*2+1] & 0x1F ) << 3;
		    fwrite( &val, 1, 1, f);
		 }
	       break;
	       
	     default:
	       fprintf( stderr, "I'm out of my depth :)\n" );
	       break;
	    }
	  
	  free( pixelBuf );
	  
	  fclose (f);
       }
   else
     return;
    
}

int main (int argc, char *argv[])
{
   int c,			/* switch */
	 db,
	 sd = -1, 
	 dbcount = 0,
     type = OUT_PPM;

   char *progname = argv[0], *port = NULL;

   struct PilotUser User;
   unsigned char buffer[0xffff];

   while ((c = getopt_long (argc, argv, optstring, options, NULL)) != -1)
	 {
		switch (c)
		  {
		   case 'h':
			 display_help (progname);
			 return 0;
		   case 'v':
			 print_splash (progname);
			 return 0;
		   case 'p':
			 free (port);
			 port = strdup (optarg);
			 break;
		   case 't':
			 if (!strncmp ("png", optarg, 3))
			   {
#ifdef HAVE_PNG
				  type = OUT_PNG;
#else
				  fprintf (stderr, "read-veo was built without png support\n");
#endif
			   }
			 else if (!strncmp ("ppm", optarg, 3))
			   {
				  type = OUT_PPM;
			   }
			 else
			   {
				  fprintf (stderr, "Unknown output type defaulting to ppm\n");
				  type = OUT_PPM;
			   }
			 break;
		  }
	 }

   sd = pilot_connect (port);

   if (sd < 0)
	 goto error;

   if (dlp_ReadUserInfo (sd, &User) < 0)
	 goto error_close;

   if (dlp_OpenDB (sd, 0, dlpOpenRead, "ScreenShotDB", &db) < 0)
     {
	puts ("Unable to open Screen Shot database");
	dlp_AddSyncLogEntry (sd, "Unable to open Screen Shot database.\n");
	exit (EXIT_FAILURE);
     }
   
   dlp_ReadAppBlock (sd, db, 0, buffer, 0xffff);
   
   WritePictures (sd, db, type );
   
   if (sd)
     {
	/* Close the database */
	dlp_CloseDB (sd, db);
     }
   
   if (sd)
	 {
		dlp_AddSyncLogEntry (sd,
							 "Successfully read Veo photos from Palm.\n"
							 "Thank you for using pilot-link.");
		dlp_EndOfSync (sd, 0);
		pi_close (sd);
	 }

   printf ("\nList complete. %d files found.\n", dbcount);

   return 0;

error_close:
   pi_close (sd);

error:
   return -1;

}
