/* utils.c:  misc. stuff for dealing with packets.
 *
 * (c) 1996, D. Jeff Dionne.
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include "pi-socket.h"

/* this routine ruthlessly stolen verbatim from Brian J. Swetland */

int crc16(unsigned char *ptr, int count)
{
  int crc, i;

  crc = 0;
  while(--count >= 0) {
    crc = crc ^ (int)*ptr++ << 8;
    for(i = 0; i < 8; ++i)
      if(crc & 0x8000)
	crc = crc << 1 ^ 0x1021;
      else
	crc = crc << 1;
  }
  return (crc & 0xFFFF);
}

#ifdef DEBUG
dumpline (char *buf, int len, int addr)
{
  int i;

  fprintf (stderr,"%.4x  ",addr);

  for (i=0;i<16;i++) {

    if (i<len) fprintf (stderr,"%.2x ",0xff & (unsigned int)buf[i]);
    else fprintf (stderr,"   ");
  }

  fprintf (stderr,"  ");

  for (i=0;i<len;i++) {
    if (isprint(buf[i])) fprintf (stderr,"%c",buf[i]);
    else fprintf(stderr,".");
  }
  fprintf(stderr,"\n");
}
#endif
