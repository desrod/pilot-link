/* money.c:  Translate Pilot MoneyManager data formats
 *
 * Copyright (c) 1998, Rui Oliveira
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
#include "pi-money.h"

int unpack_Transaction(struct Transaction *a,
		       unsigned char *buffer, int len)
{

   unsigned char *p;

   if (len < 46)
      return 0;

   p = buffer;
   a->flags = get_byte(p);
   p += 2;			/* gap */
   a->checknum = get_short(p);
   p += 2;
   a->amount = get_slong(p);
   p += 4;
   a->total = get_slong(p);
   p += 4;
   a->amountc = get_sshort(p);
   p += 2;
   a->totalc = get_sshort(p);
   p += 2;

   a->second = get_sshort(p);
   p += 2;
   a->minute = get_sshort(p);
   p += 2;
   a->hour = get_sshort(p);
   p += 2;
   a->day = get_sshort(p);
   p += 2;
   a->month = get_sshort(p);
   p += 2;
   a->year = get_sshort(p);
   p += 2;
   a->wday = get_sshort(p);
   p += 2;

   a->repeat = get_byte(p);
   p += 1;
   a->flags2 = get_byte(p);
   p += 1;
   a->type = get_byte(p);
   p += 1;

   memcpy(a->reserved, p, 2);
   p += 2;

   a->xfer = get_byte(p);
   p += 1;

   strcpy(a->description, p);
   p += 19;
   strcpy(a->note, p);
   p += strlen(p) + 1;

   return (p - buffer);
}

int pack_Transaction(struct Transaction *a, unsigned char *buffer, int len)
{
   unsigned char *p;

   int destlen = 46 + strlen(a->note) + 1;

   if (!buffer)
      return destlen;
   if (len < destlen)
      return 0;

   p = buffer;
   set_byte(p, a->flags);
   p += 1;
   set_byte(p, 0);
   p += 1;			/* gap fill */
   set_short(p, a->checknum);
   p += 2;
   set_slong(p, a->amount);
   p += 4;
   set_slong(p, a->total);
   p += 4;
   set_sshort(p, a->amountc);
   p += 2;
   set_sshort(p, a->totalc);
   p += 2;

   set_sshort(p, a->second);
   p += 2;
   set_sshort(p, a->minute);
   p += 2;
   set_sshort(p, a->hour);
   p += 2;
   set_sshort(p, a->day);
   p += 2;
   set_sshort(p, a->month);
   p += 2;
   set_sshort(p, a->year);
   p += 2;
   set_sshort(p, a->wday);
   p += 2;

   set_byte(p, a->repeat);
   p += 1;
   set_byte(p, a->flags2);
   p += 1;
   set_byte(p, a->type);
   p += 1;

   /* gap fill */
   set_short(p, 0);
   p += 2;

   set_byte(p, a->xfer);
   p += 1;

   strcpy(p, a->description);
   p += 19;
   strcpy(p, a->note);
   p += strlen(p) + 1;

   return (p - buffer);
}


int unpack_MoneyAppInfo(struct MoneyAppInfo *a,
			unsigned char *buffer, int len)
{
   int i, j;
   unsigned char *p;

   i = unpack_CategoryAppInfo(&a->category, buffer, len);
   if (!i)
      return 0;

   p = (unsigned char *) (buffer + i);

   len -= i;
   if (len < 603)
      return 0;

   for (j = 0; j < 20; j++) {
      memcpy(a->typeLabels[j], p, 10);
      p += 10;
   }

   for (j = 0; j < 20; j++) {
      memcpy(a->tranLabels[j], p, 20);
      p += 20;
   }

   return i + 603;
}

int pack_MoneyAppInfo(struct MoneyAppInfo *a,
		      unsigned char *buffer, int len)
{
   int i, j;
   unsigned char *p;

   i = pack_CategoryAppInfo(&a->category, buffer, len);

   if (!buffer)
      return i + 603;
   if (!i)
      return i;

   p = (unsigned char *) (buffer + i);
   len -= i;
   if (i < 603)
      return 0;

   for (j = 0; j < 20; j++) {
      memcpy(p, a->typeLabels[j], 10);
      p += 10;
   }

   for (j = 0; j < 20; j++) {
      memcpy(p, a->tranLabels[j], 20);
      p += 20;
   }

   return (i + 603);
}
