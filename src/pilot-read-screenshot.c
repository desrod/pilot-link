/*
 * $Id$ 
 *
 * pilot-read-screenshot.c
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
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include "pi-source.h"
#include "pi-file.h"
#include "pi-header.h"
#include "pi-userland.h"

#ifdef HAVE_PNG
# include "png.h"
#endif

#define pi_mktag(c1,c2,c3,c4) (((c1)<<24)|((c2)<<16)|((c3)<<8)|(c4))

#define OUT_PPM	 1
#define OUT_PNG	 2

struct ss_state {
	int w,
	  h,
	  depth;
	unsigned char *pix_map;
};



#define max(a,b) (( a > b ) ? a : b )
#define min(a,b) (( a < b ) ? a : b )

/***********************************************************************
 *
 * Function:	 write_png
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:	  Nothing
 *
 ***********************************************************************/
#ifdef HAVE_PNG
void write_png ( char *fname, struct ss_state *state )
{
	unsigned char *gray_buf;
	int i, j;
	png_structp png_ptr;
	png_infop info_ptr;
	FILE *f;

	if( state->depth < 8 )
		gray_buf = malloc( state->w );

	png_ptr = png_create_write_struct
		(PNG_LIBPNG_VER_STRING, NULL,
		NULL, NULL);

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

	f = fopen (fname, "wb");

	png_init_io (png_ptr, f);

	if( state->depth < 8 )
		png_set_IHDR (png_ptr, info_ptr, state->w, state->h,
		8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	else
	  	png_set_IHDR (png_ptr, info_ptr, state->w, state->h,
		8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);


	png_write_info (png_ptr, info_ptr);

	if( state->depth < 8 )
	{
		for (i = 0; i < state->h; i++)
		{
			for( j=0; j<state->w; j++ )
				gray_buf[j] = state->pix_map[i*3*state->w+j*3];

			png_write_row (png_ptr, gray_buf );
			png_write_flush (png_ptr);
		}
	}
	else
	{
		for (i = 0; i < state->h; i++)
		{
			png_write_row (png_ptr, &state->pix_map[i*3*state->w] );
			png_write_flush (png_ptr);
		}
	}

	png_write_end (png_ptr, info_ptr);
	png_destroy_write_struct (&png_ptr, &info_ptr);

	fclose( f );

	if( state->depth < 8 )
		free( gray_buf );
}
#endif

void write_ppm ( char *fname, struct ss_state *state)
{
	int i;
	FILE *f;

	f = fopen (fname, "wb");

	fprintf (f, "P6\n# ");

	fprintf (f, "%s\n", fname );

	fprintf (f, "%d %d\n", state->w, state->h );

	fprintf (f, "255\n" );

	for( i = 0; i < 3*state->h*state->w; i += 3 )
		fwrite( &state->pix_map[i], 3, 1, f);

	fclose( f );
}


/***********************************************************************
 *
 * Function:	 WritePictures
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
	char extension[8];
	int i, len, idx = 0, recs, imgNum = 0;
//	unsigned char inBuf[61440], *pixelBuf;
	pi_buffer_t *inBuf, *pixelBuf;
	unsigned long clut[256], magic;
	int attr, category, val, mask, j, k;
	struct ss_state state;

	if( type == OUT_PPM )
		sprintf (extension, ".ppm");
	else if( type == OUT_PNG )
		sprintf (extension, ".png");
	else
		return;

	inBuf = pi_buffer_new (61440);

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
			state.w = ( inBuf->data[4] << 8 )+ inBuf->data[5];
			state.h = ( inBuf->data[6] << 8 ) + inBuf->data[7];
			recs = inBuf->data[9];
			state.depth = inBuf->data[8];
			magic = ((unsigned long *)inBuf->data)[0];

			if(  magic != 0xBECEDEFE && magic != 0xDEDEFEFE )
			{
				 /* no magic must version 1 db */
	//			 fprintf( stderr, "No Magic !\n" );

				 state.w = 160;
				 state.h = 160;
				 recs = 1;

				switch( len )
				{
				case 3200:
					state.depth = 1;
					mask = 1;
					break;

				case 6400:
					state.depth = 2;
					mask = 3;
					break;

				case 12800:
					state.depth = 4;
					mask = 0x0f;
					break;

				case 26624:
					state.depth = 8;
					break;

				case 51200:
					state.depth = 16;
					break;

				default:
					/* unknown record */
					/* get next */
					fprintf( stderr, "Unknown record" );
					continue;
				}
			}

			pixelBuf
			= pi_buffer_new (state.h * state.w * state.depth / 8 + 10 + 1024);

	//		pixelBuf = malloc( state.h * state.w * state.depth / 8 + 10 + 1024 );
			state.pix_map = malloc( state.h * state.w * 3 );

			if( !pixelBuf || !state.pix_map )
			{
				fprintf( stderr, "Memory Allocation failed\n" );
				return;
			}

			if( magic == 0xBECEDEFE || magic == 0xDEDEFEFE )
				memcpy( pixelBuf->data, &inBuf->data[10], len - 10 );
			else
				memcpy( pixelBuf->data, inBuf->data, len );

			for( i=1; i< recs; i++ )
			{
				len =
				dlp_ReadRecordByIndex (sd, db, idx, inBuf, 0, &attr, &category);
				memcpy( &pixelBuf->data[i*61440-10], inBuf->data, len );
				idx++;
			}

			sprintf (fname, "ScreenShot%d", ++imgNum );

			if (plu_protect_files (fname, extension, sizeof(fname)) < 1) {
				goto cleanup;
			}

			printf ("Generating %s...\n", fname);
			fprintf( stderr, "height: %d width: %d records: %d bit depth: %d\n"
				, state.h, state.w, recs, state.depth );

			if( state.depth == 8 )
			memcpy( clut, &inBuf->data[len-1024], 1024 );

			switch( state.depth )
			{
				case 1:
				case 2:
				case 4:
					for( i = 0; i < state.h*state.w/(8/state.depth); i++)
					{
						for( j=(8/state.depth-1), k=0; j >= 0; j--, k++ )
						{
							/* get right bits */
							val = ((pixelBuf->data[i] >> (j * state.depth)) & mask);
							/* invert */
							val = mask - val;
							/* stretch */
							val *= (255/mask);

							state.pix_map[3*(i*(8/state.depth)+k)] = val;
							state.pix_map[3*(i*(8/state.depth)+k)+1] = val;
							state.pix_map[3*(i*(8/state.depth)+k)+2] = val;
						}
		 			}
			 	break;

				case 8:
					for( i = 0; i < state.h*state.w; i++)
					{
						state.pix_map[3*i] =
						*(1 + (char *)&clut[pixelBuf->data[i]]);
						state.pix_map[3*i+1] =
						*(2 + (char *)&clut[pixelBuf->data[i]]);
						state.pix_map[3*i+2] =
						*(3 + (char *)&clut[pixelBuf->data[i]]);
					}
				break;

				case 16:
					for( i = 0; i < state.h*state.w; i++)
					{
						state.pix_map[i*3] = pixelBuf->data[i*2] & 0xF8;
						state.pix_map[i*3+1] = ((pixelBuf->data[i*2] & 0x07 ) << 5)
						+ (( pixelBuf->data[i*2+1] & 0xE0 ) >> 3 );
						state.pix_map[i*3+2] = ( pixelBuf->data[i*2+1] & 0x1F ) << 3;
					}
					break;

				default:
					fprintf( stderr, "I'm out of my depth :)\n" );
				break;
			}

			if( type == OUT_PPM )
				write_ppm( fname, &state );
			#ifdef HAVE_PNG
			else
				write_png( fname, &state );
			#endif

cleanup:
			pi_buffer_free (pixelBuf);
			//	  free( pixelBuf );
			free( state.pix_map );

			//	  fclose (f);
		}
	else
		return;

	pi_buffer_free (inBuf);
}

int main (int argc, const char *argv[])
{
	int c,			/* switch */
	 db,
	 sd = -1,
	 dbcount = 0,
	  type = OUT_PPM;

	const char
                *pformat;

	struct PilotUser User;

	poptContext po;

	struct poptOption options[] = {
		USERLAND_RESERVED_OPTIONS
		{"format", 	'f', POPT_ARG_STRING, &pformat, 0, "Specify picture output type (ppm or png)"},
		POPT_TABLEEND
	};

	po = poptGetContext("pilot-read-screenshot", argc, argv, options, 0);
	poptSetOtherOptionHelp(po,"\n\n");

	if (argc<2) {
		poptPrintUsage(po,stderr,0);
		return 1;
	}

	while ((c = poptGetNextOpt(po)) >= 0) {
		fprintf(stderr,"   ERROR: Unhandled option %d.\n",c);
		return 1;
	}
	if (c<-1) {
		plu_badoption(po,c);
	}

	if (!strncmp ("png", pformat, 3))
	{
#ifdef HAVE_PNG
		type = OUT_PNG;
#else
		fprintf (stderr, "   ERROR: pilot-read-screenshot was built without png support.\n");
#endif
	}
	else if (!strncmp ("ppm", pformat, 3))
	{
		type = OUT_PPM;
	}
	else
	{
		fprintf (stderr, "   ERROR: Unknown output type, defaulting to ppm\n");
		type = OUT_PPM;
	}

	sd = plu_connect ();

	if (sd < 0)
	 	goto error;

	if (dlp_ReadUserInfo (sd, &User) < 0)
	 	goto error_close;

	if (dlp_OpenDB (sd, 0, dlpOpenRead, "ScreenShotDB", &db) < 0)
	{
		fprintf (stderr,"   ERROR: Unable to open Screen Shot database on Palm.\n");
		dlp_AddSyncLogEntry (sd, "Unable to open Screen Shot database.\n");
		goto error_close;
	}

	WritePictures (sd, db, type );

	if (sd)
	{
		/* Close the database */
		dlp_CloseDB (sd, db);
	 }

	if (sd)
	{
		dlp_AddSyncLogEntry (sd,
							 "Successfully read screenshots from Palm.\n"
							 "Thank you for using pilot-link.");
		dlp_EndOfSync (sd, 0);
		pi_close (sd);
	}

	if (!plu_quiet) {
		printf ("\nList complete. %d files found.\n", dbcount);
	}

	return 0;

error_close:
	pi_close (sd);

error:
	return -1;
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
/* ex: set tabstop=4 expandtab: */
/* Local Variables: */
/* indent-tabs-mode: t */
/* c-basic-offset: 8 */
/* End: */
