/* hinote.c:  Translate Hi-Note data formats
 *
 * Copyright 1997 Bill Goodman
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-hinote.h"

void free_HiNoteNote(struct HiNoteNote *a)
{
   if (a->text)
      free(a->text);
}

int unpack_HiNoteNote(struct HiNoteNote *a, unsigned char *buffer, int len)
{
   if (len < 3)
      return 0;
   a->flags = buffer[0];
   a->level = buffer[1];
   a->text = strdup((char *) &buffer[2]);
   return strlen((char *) &buffer[2]) + 3;
}

int pack_HiNoteNote(struct HiNoteNote *a, unsigned char *buffer, int len)
{
   int destlen;

   destlen = 3;
   if (a->text)
      destlen += strlen(a->text);

   if (!buffer)
      return destlen;
   if (len < destlen)
      return 0;

   buffer[0] = a->flags;
   buffer[1] = a->level;


   if (a->text)
      strcpy((char *) &buffer[2], a->text);
   else {
      buffer[2] = 0;
   }
   return destlen;
}

int unpack_HiNoteAppInfo(struct HiNoteAppInfo *ai, unsigned char *record,
			 int len)
{
   unsigned char *start;
   int i;
   int index;

   start = record;
   i = unpack_CategoryAppInfo(&ai->category, record, len);
   if (!i)
      return i;
   record += i;
   len -= i;
   if (len < 48)
      return 0;
   for (index = 0; i < 48; i++)
      ai->reserved[i] = *record++;
   return (record - start);
}

int pack_HiNoteAppInfo(struct HiNoteAppInfo *ai, unsigned char *record,
		       int len)
{
   int i;
   int index;
   unsigned char *start = record;

   i = pack_CategoryAppInfo(&ai->category, record, len);
   if (i == 0)			/* category pack failed */
      return 0;
   if (!record)
      return i + 48;
   record += i;
   len -= i;
   if (len < 48)
      return (record - start);
   for (index = 0; i < 48; i++)
      *record++ = ai->reserved[i];

   return (record - start);
}