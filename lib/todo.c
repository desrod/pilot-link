/* todo.c:  Translate Pilot ToDo application data formats
 *
 * Copyright (c) 1996, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pi-socket.h"
#include "dlp.h"
#include "todo.h"

void free_ToDo(struct ToDo * a) {
  if(a->description)
    free(a->description);
  if(a->note)
    free(a->note);
}

void unpack_ToDo(struct ToDo * a, unsigned char * buffer, int len) {
  int iflags;
  char * p2;
  unsigned long d;
  int j;
  
  a->due.tm_hour = 0;
  a->due.tm_min = 0;
  a->due.tm_sec = 0;
  d = (unsigned short int)get_short(buffer);
  a->due.tm_year = (d >> 9) + 4;
  a->due.tm_mon = ((d >> 5) & 15) - 1;
  a->due.tm_mday = d & 31;

  a->priority = get_byte(buffer+2);
  if(a->priority & 0x80) {
    a->complete = 1;
    a->priority &= 0x7f;
  }
  
  a->description = strdup(buffer+3);
  a->note = strdup(buffer+3+strlen(buffer+3)+1);
}

void pack_ToDo(struct ToDo *, unsigned char * record, int * len);
                  
