/* utils.c:  misc. stuff for dealing with packets.
 *
 * (c) 1996, D. Jeff Dionne.
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include <ctype.h>

#ifdef NeXT
# include <stdlib.h>
# include <string.h>
# include <assert.h>
#endif

#include "pi-source.h"
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

#ifdef NeXT
char * strdup(const char *string)
{
    size_t length;
    char *result;

    assert(string != NULL);

    length = strlen(string) + 1;
    result = malloc(length);

    if (result == NULL)
	return NULL;

    memcpy(result, string, length);

    return result;
}
#endif

char * printlong (unsigned long val)
{
  static char buf[5];
  set_long(buf, val);
  buf[4] = 0;
  return buf;
}

unsigned long makelong (char * c)
{
  return get_long(c);
}

void dumpline (const unsigned char *buf, int len, int addr)
{
  int i;

  fprintf (stderr,"%.4x  ",addr);

  for (i=0;i<16;i++) {

    if (i<len) fprintf (stderr,"%.2x ",0xff & (unsigned int)buf[i]);
    else fprintf (stderr,"   ");
  }

  fprintf (stderr,"  ");

  for (i=0;i<len;i++) {
    if (isprint(buf[i]) && (buf[i]>=32) && (buf[i]<=126)) fprintf (stderr,"%c",buf[i]);
    else fprintf(stderr,".");
  }
  fprintf(stderr,"\n");
}

void dumpdata (const unsigned char * buf, int len) {
  int i;
  for(i=0;i<len;i+=16) {
    dumpline(buf+i, ((len-i)>16) ? 16 : len-i, i);
  }
}
