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
#include "pi-socket.h"
#include "dlp.h"
#include "memo.h"

void free_Memo(struct Memo * a) {
  if(a->text)
    free(a->text);
}

void unpack_Memo(struct Memo * a, unsigned char * buffer, int len) {
  a->text = strdup(buffer);
}

void pack_Memo(struct Memo * a, unsigned char * buffer, int * len) {
  if(a->text)
    strcpy(buffer,a->text);
  else
    buffer[0] = 0;
  *len = strlen(a->text)+1;
}
                  
