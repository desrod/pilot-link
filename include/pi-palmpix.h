#ifndef PILOT_PALMPIX_H
#define PILOT_PALMPIX_H

#include "pi-args.h"

#ifdef __cplusplus
extern "C" 
{
#endif

#define PalmPix_Creator (makelong ("COCO"))
#define PalmPix_DB "ArchImage"

/* flags values */
#define PALMPIX_COLOUR_CORRECTION    1
#define PALMPIX_HISTOGRAM_STRETCH    2

struct PalmPixState {
   /* This callback should read record #RECNO into BUFFER and BUFSIZE,
    * and return 0 when successful, just like pi_file_read_record().  */
   int (*getrecord) PI_ARGS ((struct PalmPixState *self, int recno,
   void **buffer, size_t *bufsize));

   /* This will be filled in by pixName.  */
   char pixname[33];

   /* After unpack_PalmPix, this will be the last record index which is
    * +   part of the current picture.  */
   int highest_recno;
   
   /* Set these to some permutation of 0,1,2 before using pixPixmap.  */
   int offset_r, offset_g, offset_b;

   /* This specifies the png or ppm output */
   int output_type;
   
   /* This will be filled in by pixPixmap.  */
   unsigned char *pixmap;

   /* The output brightness adjustment */
   int bias;
   
   /* this controls colour correction and histogram stretch */
   int flags;
};
   
enum {
   pixChannelGR, pixChannelR, pixChannelB, pixChannelGB
};
   
struct PalmPixHeader {
   int w, h, resolution, zoom, num;
   int year, month, day, hour, min, sec;
   int numRec, thumbLen;
   int chansize[4];
};
   
enum {
   pixName = 0x01, pixThumbnail = 0x02, pixPixmap = 0x04
};
   
/* picture output types */
#define PALMPIX_OUT_PPM    1
#define PALMPIX_OUT_PNG    2
   
/* Returns the number of bytes from the buffer that were consumed, or 0 on
 * error (generally the record not in fact being a PalmPixHeader).  */
extern int unpack_PalmPixHeader
     PI_ARGS ((struct PalmPixHeader *h, const unsigned char *p, int len));
   
extern int unpack_PalmPix
     PI_ARGS ((struct PalmPixState *state,
	       const struct PalmPixHeader *h, int recno, int wanted));
   
extern int free_PalmPix_data
     PI_ARGS ((struct PalmPixState *state));
   
#ifdef __cplusplus 
}
#endif

#endif
