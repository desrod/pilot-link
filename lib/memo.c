/* memo.c:  Translate Pilot memopad data formats
 *
 * Copyright (c) 1996, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-memo.h"

void free_Memo(struct Memo * a) {
  if(a->text)
    free(a->text);
}

void unpack_Memo(struct Memo * a, unsigned char * buffer, int len) {
  a->text = strdup(buffer);
}

void pack_Memo(struct Memo * a, unsigned char * buffer, int * len) {
  if(a->text) {
    strcpy(buffer,a->text);
    *len = strlen(a->text)+1;
  } else {
    buffer[0] = 0;
    *len = 1;
  }
}
                  
void unpack_MemoAppInfo(struct MemoAppInfo * ai, unsigned char * record, int len) {
  int i;
  ai->renamedcategories = get_short(record);
  record+=2;
  for(i=0;i<16;i++) {
    memcpy(ai->CategoryName[i], record, 16);
    record += 16;
  }
  memcpy(ai->CategoryID, record, 16);
  record += 16;
  ai->lastUniqueID = get_byte(record);
}

void pack_MemoAppInfo(struct MemoAppInfo * ai, unsigned char * record, int * len) {
  int i;
  set_short(record, ai->renamedcategories);
  record += 2;
  for(i=0;i<16;i++) {
    memcpy(record, ai->CategoryName[i], 16);
    record += 16;
  }
  memcpy(record, ai->CategoryID, 16);
  record += 16;
  set_byte(record, ai->lastUniqueID);
  record ++;
  set_byte(record, 0); /* gapfil */
  set_short(record+1, 0); /* gapfil */
}
