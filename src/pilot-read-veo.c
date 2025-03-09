/*
 * $Id$ 
 *
 * pilot-read-veo.c
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
#include "pi-veo.h"
#include "pi-file.h"
#include "pi-header.h"
#include "pi-userland.h"

#ifdef HAVE_PNG
# include "png.h"
#endif

#define pi_mktag(c1,c2,c3,c4) (((c1)<<24)|((c2)<<16)|((c3)<<8)|(c4))

#define VEO_COLOUR_CORRECT 0x01
#define VEO_BIAS           0x12

uint8_t redLUT[256], greenLUT[256], blueLUT[256];

double bias_factor = 0.50;


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
 ***********************************************************************/
static const char *fmt_date (struct Veo *v)
{
   static char buf[24];

   sprintf (buf, "%d-%02d-%02d", v->year, v->month, v->day);
   return buf;

}

/***********************************************************************
 *
 * Function:	Decode
 *
 * Summary:	Decode one record from the Veo database.
 *
 * Parameters:	inP - the compressed record
 *              outP - the uncompressed record
 *              w - the width of the picture
 * Returns:	1 always
 *
 ***********************************************************************/
int Decode (unsigned char *inP, unsigned char *outP, short w)
{
   short shifter = 7, index = 0;
   unsigned short tmp0, tmp3, tmp4, i, j;	/* d3 */
   unsigned char *origOutP = outP;

   shifter = 7;

   for (j = 0; j < 2; j++)
	 {
		if (j == 1)
		  origOutP += w * 2;

		for (i = 0; i < 2; i++)
		  {
			 switch (i)
			   {
				case 0:
				  outP = (unsigned char *) (origOutP + 1);
				  break;
				case 1:
				  outP = (unsigned char *) (origOutP + w);
				  break;
			   }

			 if (shifter != 7)
			   {
				  shifter = 7;
				  inP++;
			   }

			 *outP = *inP++;

			 outP += 2;		/* a3 */
			 index = 2;

			 while (index < w)
			   {
				  /* Top of decompress loop */
				  tmp3 = 1 << shifter;

				  tmp3 &= *inP;

				  if ((shifter & 0xFF) <= 0)
					{
					   shifter = 7;
					   inP++;
					}
				  else
					{
					   shifter--;
					}

				  if ((tmp3 & 0xFF) != 0)
					{
					   *outP = *(outP - 2);
					}
				  else
					{
					   tmp0 = inP[1];
					   tmp4 = inP[0] << 8;

					   tmp4 |= tmp0;

					   tmp4 = tmp4 << (7 - shifter);

					   if (shifter >= 5)
						 {
							shifter -= 5;
						 }
					   else
						 {
							inP++;
							shifter += 3;
						 }

					   tmp4 = tmp4 >> 11;

					   /* if((char)tmp4 <= 0) */
					   if ((char) tmp4 == 0)
						 {
							tmp3 = inP[0] << 8 | inP[1];

							tmp3 = tmp3 << (7 - shifter);

							if (shifter >= 8)
							  {
								 shifter -= 8;
							  }
							else
							  {
								 inP++;
								 shifter &= 0xf;
							  }

							*outP = tmp3 >> 8;
						 }
					   else
						 {
							if (tmp4 & 0x10)
							  {
								 tmp4 &= 0xf;

								 *outP = *(outP - 2) - tmp4;
							  }
							else
							  {
								 tmp4 &= 0xf;

								 *outP = *(outP - 2) + tmp4;
							  }
						 }
					}
				  outP += 2;
				  index += 2;
			   }
			 /* End 'while (index < w)' */
		  }

		outP = origOutP;

		if (shifter != 7)
		  {
			 shifter = 7;
			 inP++;
		  }

		*outP = *inP++;

		if (shifter != 7)
		  {
			 shifter = 7;
			 inP++;
		  }

		outP[w + 1] = *inP++;

		outP += 2;		/* a3 */
		index = 2;

		while (index < w)
		  {
	  /* Top of decompress loop */
			 tmp3 = 1 << shifter;

			 tmp3 &= *inP;

			 if ((shifter & 0xFF) <= 0)
			   {
				  shifter = 7;
				  inP++;
			   }
			 else
			   {
				  shifter--;
			   }

			 if ((tmp3 & 0xFF) != 0)
			   {
				  *outP = outP[w - 1];
			   }
			 else
			   {
				  tmp0 = inP[1];
				  tmp4 = inP[0] << 8;

				  tmp4 |= tmp0;

				  tmp4 = tmp4 << (7 - shifter);

				  if (shifter >= 5)
					{
					   shifter -= 5;
					}
				  else
					{
					   inP++;
					   shifter += 3;
					}

				  tmp4 = tmp4 >> 11;

				  if ((char) tmp4 == 0)
					{
					   tmp3 = inP[0] << 8 | inP[1];

					   tmp3 = tmp3 << (7 - shifter);

					   if (shifter >= 8)
						 {
							shifter -= 8;
						 }
					   else
						 {
							inP++;
							shifter &= 0xf;
						 }

					   *outP = tmp3 >> 8;
					}
				  else
					{
					   if (tmp4 & 0x10)
						 {
							tmp4 &= 0xf;

							*outP = outP[w - 1] - tmp4;
						 }
					   else
						 {
							tmp4 &= 0xf;
							*outP = outP[w - 1] + tmp4;
						 }
					}
			   }

			 tmp3 = 1 << shifter;
			 tmp3 &= *inP;

			 if ((shifter & 0xFF) <= 0)
			   {
				  shifter = 7;
				  inP++;
			   }
			 else
			   {
				  shifter--;
			   }

			 if ((tmp3 & 0xFF) != 0)
			   {
				  outP[w + 1] = *outP;
			   }
			 else
			   {
				  tmp0 = inP[1];
				  tmp4 = inP[0] << 8;

				  tmp4 |= tmp0;

				  tmp4 = tmp4 << (7 - shifter);

				  if (shifter >= 5)
					{
					   shifter -= 5;
					}
				  else
					{
					   inP++;
					   shifter += 3;
					}

				  tmp4 = tmp4 >> 11;

				  /* if((char)tmp4 <= 0) */
				  if ((char) tmp4 == 0)
					{
					   tmp3 = inP[0] << 8 | inP[1];

					   tmp3 = tmp3 << (7 - shifter);

					   if (shifter >= 8)
						 {
							shifter -= 8;
						 }
					   else
						 {
							inP++;
							shifter &= 0xf;
						 }

					   outP[w + 1] = tmp3 >> 8;
					}
				  else
					{
					   if (tmp4 & 0x10)
						 {
							tmp4 &= 0xf;

							outP[w + 1] = *outP - tmp4;
						 }
					   else
						 {
							tmp4 &= 0xf;

							outP[w + 1] = *outP + tmp4;
						 }
					}
			   }
			 outP += 2;
			 index += 2;
		  }
		/* End 'while (index < w)' */

	 }
   /* End 'for (j = 0; j < 2; j++)' */
   return (1);
}

/***********************************************************************
 *
 * Function:	GetPicData
 *
 * Summary:     Requests one record of bayer data from the palm
 *              and then decodes it.
 *
 * Parameters:  r - the row we are looking for
 *              v - veo record
 *              row - the bayer data containg our row
 *
 * Returns:     the size of the encoded record
 *
 ***********************************************************************/
static int
  GetPicData (uint32_t flags, int r, struct Veo *v, unsigned char *row)
{
   int attr, category, len;
   pi_buffer_t *tmpRow;

   if (!v->sd)
	 return (-1);

   /* Each record contains four rows of bayer data */
   /* The compressed record can be upto twice as large as the
    * uncompressed record ??? */
   tmpRow = pi_buffer_new (5120);

   len = dlp_ReadRecordByIndex (v->sd, v->db, 1 + r / 4, tmpRow, 0,
								&attr, &category);

   if (len < 0)
	 return 0;

   Decode (tmpRow->data, row, v->width);

   pi_buffer_free(tmpRow);

   return (len);
}

#define max(a,b) (( a > b ) ? a : b )
#define min(a,b) (( a < b ) ? a : b )

/***********************************************************************
 *
 * Function:    Bias
 *
 *              Bias is based on the Fast Alternative to Perlin's Bias
 *              algorithm in Graphics Gems IV by
 *              Christophe Schlick schlick@labri.u-bordeuax.fr
 *
 * Summary:     Lighten or darken the image
 *
 * Parameters:
 *
 * Returns:
 *
 ***********************************************************************/
static void Bias( double bias, int width, int height, uint8_t *data )
{
   int i;
   double num, denom, t;

   for( i=0; i<width*height; i++ )
     {
	t = (double)data[i]/256.0;
	num = t;
	denom = (1.0/bias - 2) * (1.0 - t) + 1;
	data[i] = num/denom * 256.0;
     }
}

int ColourCorrect (struct Veo *v, uint8_t *red, uint8_t *green, uint8_t *blue, long flags )
{
	uint8_t *tmpRow;
	uint8_t gMin, gMax, rMin, rMax, bMin, bMax;
	float gInc, rInc, bInc, gCur, rCur, bCur;
	float rMean = 0, gMean = 0, bMean = 0, maxMean;
	uint16_t	width = v->width;
	uint16_t	height = v->height;
	int i;
   	float redCeiling = 254;
   	float greenCeiling = 252;
   	float blueCeiling = 255;

	memset( red, 0, 256 * sizeof( uint8_t ));
	memset( green, 0, 256 * sizeof( uint8_t ));
	memset( blue, 0, 256 * sizeof( uint8_t ));

	gMin = rMin = bMin = 255;
	gMax = rMax = bMax = 0;

	tmpRow = malloc( 2560 );

	GetPicData( 0, 0, v, tmpRow );

	for( i=0; i<width; i += 2 )
	{
		gMin = min( gMin, tmpRow[i] );
		rMin = min( rMin, tmpRow[i+width] );
		bMin = min( bMin, tmpRow[i+1] );
		gMin = min( gMin, tmpRow[i+width+1] );
		gMax = max( gMax, tmpRow[i] );
		rMax = max( rMax, tmpRow[i+width] );
		bMax = max( bMax, tmpRow[i+1] );
		gMax = max( gMax, tmpRow[i+width+1] );

		rMean += tmpRow[i+width];
		gMean += tmpRow[i];
		gMean += tmpRow[i+width+1];
		bMean += tmpRow[i+1];
	}


	if( width == 640 )
	{
		for( i=width*2; i<width*3; i += 2 )
		{
			gMin = min( gMin, tmpRow[i] );
			rMin = min( rMin, tmpRow[i+width] );
			bMin = min( bMin, tmpRow[i+1] );
			gMin = min( gMin, tmpRow[i+width+1] );
			gMax = max( gMax, tmpRow[i] );
			rMax = max( rMax, tmpRow[i+width] );
			bMax = max( bMax, tmpRow[i+1] );
			gMax = max( gMax, tmpRow[i+width+1] );

			rMean += tmpRow[i+width];
			gMean += tmpRow[i];
			gMean += tmpRow[i+width+1];
			bMean += tmpRow[i+1];
		}
	}

	GetPicData( 0, height/2, v, tmpRow );

	for( i=0; i<width; i += 2 )
	{
		gMin = min( gMin, tmpRow[i] );
		rMin = min( rMin, tmpRow[i+width] );
		bMin = min( bMin, tmpRow[i+1] );
		gMin = min( gMin, tmpRow[i+width+1] );
		gMax = max( gMax, tmpRow[i] );
		rMax = max( rMax, tmpRow[i+width] );
		bMax = max( bMax, tmpRow[i+1] );
		gMax = max( gMax, tmpRow[i+width+1] );

		rMean += tmpRow[i+width];
		gMean += tmpRow[i];
		gMean += tmpRow[i+width+1];
		bMean += tmpRow[i+1];
	}

	if( width == 640 )
	{
		for( i=width*2; i<width*3; i += 2 )
		{
			gMin = min( gMin, tmpRow[i] );
			rMin = min( rMin, tmpRow[i+width] );
			bMin = min( bMin, tmpRow[i+1] );
			gMin = min( gMin, tmpRow[i+width+1] );
			gMax = max( gMax, tmpRow[i] );
			rMax = max( rMax, tmpRow[i+width] );
			bMax = max( bMax, tmpRow[i+1] );
			gMax = max( gMax, tmpRow[i+width+1] );

			rMean += tmpRow[i+width];
			gMean += tmpRow[i];
			gMean += tmpRow[i+width+1];
			bMean += tmpRow[i+1];
		}
	}

	GetPicData( 0, height-1, v, tmpRow );

	for( i=0; i<width; i += 2 )
	{
		gMin = min( gMin, tmpRow[i] );
		rMin = min( rMin, tmpRow[i+width] );
		bMin = min( bMin, tmpRow[i+1] );
		gMin = min( gMin, tmpRow[i+width+1] );
		gMax = max( gMax, tmpRow[i] );
		rMax = max( rMax, tmpRow[i+width] );
		bMax = max( bMax, tmpRow[i+1] );
		gMax = max( gMax, tmpRow[i+width+1] );

		rMean += tmpRow[i+width];
		gMean += tmpRow[i];
		gMean += tmpRow[i+width+1];
		bMean += tmpRow[i+1];
	}

	if( width == 640 )
	{
		for( i=width*2; i<width*3; i += 2 )
		{
			gMin = min( gMin, tmpRow[i] );
			rMin = min( rMin, tmpRow[i+width] );
			bMin = min( bMin, tmpRow[i+1] );
			gMin = min( gMin, tmpRow[i+width+1] );
			gMax = max( gMax, tmpRow[i] );
			rMax = max( rMax, tmpRow[i+width] );
			bMax = max( bMax, tmpRow[i+1] );
			gMax = max( gMax, tmpRow[i+width+1] );

			rMean += tmpRow[i+width];
			gMean += tmpRow[i];
			gMean += tmpRow[i+width+1];
			bMean += tmpRow[i+1];
		}
	}

	rMean = rMean / ( 640 * 3 );
	gMean = gMean / ( 640 * 6 );
	bMean = bMean / ( 640 * 3 );

	maxMean = max( gMean-gMin, max( bMean-bMin, rMean-rMin ));
//	maxMean = max( gMean, max( bMean, rMean ));

	rInc = maxMean / (rMean-rMin);
   	gInc = maxMean / (gMean-gMin);
   	bInc = maxMean / (bMean-bMin);

   rCur = 0;
   gCur = 0;
   bCur = 0;

   for (i = 0; i<256; i++)
	 {
	 	if( i < rMin )
		 	red[i] = 0;
		else
		{
			if( rCur < redCeiling )
			  red[i] = rCur;
			else
			  red[i] = redCeiling;

			rCur += rInc;
		}

		if( i < gMin )
			green[i] = 0;
		else
		{
			if( gCur < greenCeiling )
			  green[i] = gCur;
			else
			  green[i] = greenCeiling;

			gCur += gInc;
		}

		if( i < bMin )
			blue[i] = 0;
		else
		{
			if( bCur < blueCeiling )
			  blue[i] = bCur;
			else
			  blue[i] = blueCeiling;

			bCur += bInc;
		}
	 }

	free( tmpRow );

	return( 1 );
}

/***********************************************************************
 *
 * Function:	Gen24bitRow
 *
 * Summary:     It requests some decoded bayer pattern data from the palm
 *              and then interpolates one RGB row from that data.
 *
 * Parameters:  r - the row to be interpolated
 *              v - veo record
 *              row - the returned RGB data
 * Returns:     1 success
 *              -1 failure
 *
 ***********************************************************************/
int Gen24bitRow (long flags, int r, struct Veo *v, unsigned char *row)
{
   int i, rawW, rawH, modR = r % 4;

   unsigned char rowA[2560], rowB[2560];
   unsigned char *rAP, *rBP, *rCP;

   rawW = v->width / 2;
   rawH = v->height / 2;

   if (r == 0)
	 {
		if (-1 == GetPicData (flags, r, v, rowB))
		  return (-1);
		rAP = rBP = rowB;
		rCP = rowB + v->width;

	 }
   else if (r == (v->height - 1))
	 {
		if (-1 == GetPicData (flags, r, v, rowA))
		  return (-1);
		rAP = rowA + v->width * 2;
		rCP = rBP = rowA + v->width * 3;

	 }
   else if (modR == 0)
	 {
		if (-1 == GetPicData (flags, r - 1, v, rowA))
		  return (-1);
		rAP = rowA + v->width * 3;
		if (-1 == GetPicData (flags, r, v, rowB))
		  return (-1);
		rBP = rowB;
		rCP = rowB + v->width;

	 }
   else if (modR == 3)
	 {
		if (-1 == GetPicData (flags, r, v, rowA))
		  return (-1);
		rAP = rowA + v->width * 2;
		rBP = rowA + v->width * 3;
		if (-1 == GetPicData (flags, r + 1, v, rowB))
		  return (-1);
		rCP = rowB;

	 }
   else
	 {
		if (-1 == GetPicData (flags, r, v, rowA))
		  return (-1);
		rAP = rowA + v->width * (modR - 1);
		rBP = rowA + v->width * modR;
		rCP = rowA + v->width * (modR + 1);
	 }

   /* Bayer Pattern
    * GBGB
    * RGRG
    * GBGB
    * RGRG */
   if (r % 2 == 0)
	 {
      /* green blue center */
		row[0] = (rAP[0] + rCP[0]) >> 1;
		row[1] = rBP[0];
		row[2] = rBP[1];

      /* blue center */
		row[3] = (rAP[0] + rCP[0] + rAP[2] + rCP[2]) >> 2;
		row[4] = (rAP[1] + rBP[0] + rBP[2] + rCP[1]) >> 2;
		row[5] = rBP[1];

		for (i = 1; i < rawW - 1; i++)
		  {
	  /* green blue center */
			 row[i * 6] = (rAP[i * 2] + rCP[i * 2]) >> 1;
			 row[i * 6 + 1] = rBP[i * 2];
			 row[i * 6 + 2] = (rBP[i * 2 - 1] + rBP[i * 2 + 1]) >> 1;

	  /* blue center */
			 row[i * 6 + 3] =
			   (rAP[i * 2] + rCP[i * 2] + rAP[i * 2 + 2] + rCP[i * 2 + 2]) >> 2;
			 row[i * 6 + 4] =
			   (rAP[i * 2 + 1] + rBP[i * 2] + rBP[i * 2 + 2] +
				rCP[i * 2 + 1]) >> 2;
			 row[i * 6 + 5] = rBP[i * 2 + 1];
		  }

		i = rawW - 1;
      /* green blue center */
		row[i * 6] = (rAP[i * 2] + rCP[i * 2]) >> 1;
		row[i * 6 + 1] = rBP[i * 2];
		row[i * 6 + 2] = (rBP[i * 2 - 1] + rBP[i * 2 + 1]) >> 1;

      /* blue center */
		row[i * 6 + 3] = (rAP[i * 2] + rCP[i * 2]) >> 1;
		row[i * 6 + 4] = (rAP[i * 2 + 1] + rBP[i * 2] + rCP[i * 2 + 1]) / 3;
		row[i * 6 + 5] = rBP[i * 2 + 1];

	 }
   else
	 {
      /* red center */
		row[0] = rBP[0];
		row[1] = (rAP[0] + rBP[1] + rCP[0]) / 3;
		row[2] = (rAP[1] + rCP[1]) >> 1;

      /* green red center */
		row[3] = (rBP[0] + rBP[2]) >> 1;
		row[4] = rBP[1];
		row[5] = (rAP[1] + rCP[1]) >> 1;

		for (i = 1; i < rawW - 1; i++)
		  {
	  /* red center */
			 row[i * 6] = rBP[i * 2];
			 row[i * 6 + 1] =
			   (rAP[i * 2] + rBP[i * 2 - 1] + rBP[i * 2 + 1] + rCP[i * 2]) >> 2;
			 row[i * 6 + 2] =
			   (rAP[i * 2 - 1] + rAP[i * 2 + 1] + rCP[i * 2 - 1] +
				rCP[i * 2 + 1]) >> 2;

	  /* green red center */
			 row[i * 6 + 3] = (rBP[i * 2] + rBP[i * 2 + 2]) >> 1;
			 row[i * 6 + 4] = rBP[i * 2 + 1];
			 row[i * 6 + 5] = (rAP[i * 2 + 1] + rCP[i * 2 + 1]) >> 1;
		  }

		i = rawW - 1;

      /* red center */
		row[i * 6] = rBP[i * 2];
		row[i * 6 + 1] =
		  (rAP[i * 2] + rBP[i * 2 - 1] + rBP[i * 2 + 1] + rCP[i * 2]) >> 2;
		row[i * 6 + 2] =
		  (rAP[i * 2 - 1] + rAP[i * 2 + 1] + rCP[i * 2 - 1] +
		   rCP[i * 2 + 1]) >> 2;

      /* green red center */
		row[i * 6 + 3] = rBP[i * 2];
		row[i * 6 + 4] = rBP[i * 2 + 1];
		row[i * 6 + 5] = (rAP[i * 2 + 1] + rCP[i * 2 + 1]) >> 1;
	 }

   if (flags & VEO_COLOUR_CORRECT)
	 {
		for (i = 0; i < v->width * 3; i += 3)
		  {
			 row[i] = redLUT[row[i]];
			 row[i + 1] = greenLUT[row[i + 1]];
			 row[i + 2] = blueLUT[row[i + 2]];
		  }
	 }

   if (flags & VEO_BIAS)
	 Bias( bias_factor, v->width*3, 1, row );

   return (1);
}

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
 ***********************************************************************/
#ifdef HAVE_PNG
void write_png (FILE * f, struct Veo *v, long flags)
{
   unsigned char outBuf[2560];
   int i;
   png_structp png_ptr;
   png_infop info_ptr;

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

/***********************************************************************
 *
 * Function:    write_ppm
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void write_ppm (FILE * f, struct Veo *v, long flags)
{
   unsigned char outBuf[2560];
   int i;

   fprintf (f, "P6\n# ");

   if (v->name != NULL)
	 fprintf (f, "%s (created on %s)\n", v->name, fmt_date (v));

   fprintf (f, "%d %d\n255\n", v->width, v->height);

   for (i = 0; i < v->height; i++)
	 {
		Gen24bitRow (flags, i, v, outBuf);

		fwrite (outBuf, v->width * 3, 1, f);
	 }
}

/***********************************************************************
 *
 * Function:    WritePicture
 *
 * Summary:	FIXME
 *
 * Parameters:
 *
 * Returns:
 *
 ***********************************************************************/
void WritePicture (int sd, int db, int type, char *name, const char *progname, long flags)
{
   char fname[FILENAME_MAX];
   FILE *f;
   char extension[8];
   static int len;
   struct Veo v;
   pi_buffer_t *inBuf;
   int attr, category;

   if (type == VEO_OUT_PNG)
	 sprintf (extension, ".png");
   else if (type == VEO_OUT_PPM)
	 sprintf (extension, ".ppm");

   sprintf (fname, "%s", name);
   strcpy (v.name, name);

	if (plu_protect_files (fname, extension, sizeof(fname) ) < 1) {
		/* no suitable filename could be found. */
		return;
	}

   printf ("Generating %s...\n", fname);

   f = fopen (fname, "wb");

   if (f)
	 {
		if (sd)
		  {
             inBuf = pi_buffer_new (2560);
			 len =
			   dlp_ReadRecordByIndex (sd, db, 0, inBuf, 0, &attr, &category);
			 unpack_Veo (&v, inBuf->data, inBuf->used);
             pi_buffer_free (inBuf);
			 v.sd = sd;
			 v.db = db;
		  }
		else
		  return;

		ColourCorrect (&v, redLUT, greenLUT, blueLUT, flags);

		if (type == VEO_OUT_PPM)
		  write_ppm (f, &v, flags);
#ifdef HAVE_PNG
		else if (type == VEO_OUT_PNG)
		  write_png (f, &v, flags);
#endif

		fclose (f);
	 }
   else
	 {
		fprintf (stderr, "%s: can't write to %s\n", progname, fname);
	 }

}

int main (int argc, const char *argv[])
{
   int  c,			/* switch */
	db,
	i = 0,
	sd = -1,
	action = VEO_ACTION_OUTPUT,
	dbcount = 0,
	type = VEO_OUT_PPM,
	bias = 50;
	long flags = 0;
	struct DBInfo info;
	pi_buffer_t *buf;

	const char
                *picname = NULL;

	char *imgtype = NULL;

	struct PilotUser User;

	poptContext po;

	struct poptOption options[] = {
		USERLAND_RESERVED_OPTIONS
		{"name", 'n', POPT_ARG_STRING, &picname, 'n',
		 "Specify output picture by name", "name"},
		{"list", 'l', POPT_ARG_VAL, &action, VEO_ACTION_LIST,
		 "List Photos on device", NULL},
		{"bias", 'b', POPT_ARG_INT, &bias, 'b',
		 "lighten or darken the image (0..50 darken, 50..100 lighten)", "bias"},
		{"colour", 'c', POPT_ARG_VAL | POPT_ARGFLAG_OR, &flags,
		 VEO_COLOUR_CORRECT,
		 "colour correct the output colours", NULL},
		{"type", 't', POPT_ARG_STRING, &imgtype, 't',
		 "Specify picture output type (ppm or png)", "[ppm|png]"},
		POPT_TABLEEND
	};

	po = poptGetContext("read-veo", argc, argv, options, 0);
	poptSetOtherOptionHelp(po,"\n\n"
		"   Synchronize your Veo Traveler databases with your desktop machine.\n"
		"   Output defaults to ppm.\n\n");

	if (argc<2) {
		poptPrintUsage(po,stderr,0);
		return 1;
	}

	while ((c = poptGetNextOpt(po)) >= 0) {
		switch (c) {
		   case 'n':
			 action = VEO_ACTION_OUTPUT_ONE;
			 break;
		   case 'b':
			if( bias > 100 || bias < 0 ) {
  				  fprintf (stderr, "   ERROR: Bad bias value %d, using 50.\n",bias);
				  bias = 50;
			   }

			 bias_factor = (double)bias / 100.0;
			 flags |= VEO_BIAS;
			 break;
		   case 't':
			if (!strncmp ("png", imgtype, 3)) {
#ifdef HAVE_PNG
				  type = VEO_OUT_PNG;
#else
				  fprintf (stderr, "   ERROR: read-veo was built without png support\n");
				  type = VEO_OUT_PPM;
#endif
			   }
			else if (!strncmp ("ppm", imgtype, 3)) {
				  type = VEO_OUT_PPM;
			} else {
				  fprintf (stderr, "   ERROR: Unknown output type, defaulting to ppm\n");
				  type = VEO_OUT_PPM;
			   }
			 break;
		  }
	 }

	if (c < -1) {
		plu_badoption(po,c);
	}

	sd = plu_connect ();

   if (sd < 0)
	 goto error;

   if (dlp_ReadUserInfo (sd, &User) < 0)
	 goto error_close;

   buf = pi_buffer_new (sizeof (struct DBInfo));
	for (;;) {
		if (dlp_ReadDBList (sd, 0, 0x80, i, buf) < 0)
		  break;
        memcpy (&info, buf->data, sizeof(struct DBInfo));
		i = info.index + 1;
		if (info.type == pi_mktag ('E', 'Z', 'V', 'I')
		    && info.creator == pi_mktag ('O', 'D', 'I', '2')) {
			 dbcount++;
			switch (action) {
				case VEO_ACTION_LIST:
				  printf ("%s\n", info.name);
				  break;

				case VEO_ACTION_OUTPUT_ONE:
				  if (strcmp (info.name, picname))
					break;

				case VEO_ACTION_OUTPUT:
				if (dlp_OpenDB (sd, 0, 0x80 | 0x40, info.name, &db) < 0) {
					   fprintf (stderr,"   ERROR:Unable to open Veo database on Palm.\n");
					   dlp_AddSyncLogEntry (sd, "Unable to open Veo database.\n");
					   goto error_close;
					}

				  WritePicture(sd, db, type, info.name, "read-veo", flags);


				if (sd) {
					   /* Close the database */
					   dlp_CloseDB (sd, db);
					}

				  break;
			   }
		  }
	 }
    pi_buffer_free(buf);
	if (sd) {
		dlp_AddSyncLogEntry (sd,
							 "Successfully read Veo photos from Palm.\n"
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
