/* address.c:  Translate Pilot address book data formats
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
#include "address.h"

void free_Address(struct Address * a) {
  int i;
  for(i=0;i<19;i++)
    if(a->entry[i])
      free(a->entry[i]);
}

#define hi(x) (((x) >> 4) & 0x0f)
#define lo(x) ((x) & 0x0f)
#define pair(x,y) (((x) << 4) | (y))

void unpack_Address(struct Address * a, unsigned char * buffer, int len) {
  unsigned long contents;
  unsigned long v;
  
  /*get_byte(buffer); gapfil*/
  a->whichphone = hi(get_byte(buffer+1));
  a->phonelabel5 = lo(get_byte(buffer+1));
  a->phonelabel4 = hi(get_byte(buffer+2));
  a->phonelabel3 = lo(get_byte(buffer+2));
  a->phonelabel2 = hi(get_byte(buffer+3));
  a->phonelabel1 = lo(get_byte(buffer+3));
  
  contents = get_long(buffer+4);
  
  /*get_byte(buffer+8) offset*/
  
  buffer += 9;
  
  /*if(flag & 0x1) { 
     a->lastname = strdup(buffer);
     buffer += strlen(buffer) + 1;
  } else {
    a->lastname = 0;
  }*/
  
  for(v=0;v<19;v++) {
    if(contents & (1 << v)) {
      a->entry[v] = strdup(buffer);
      buffer += strlen(buffer)+1;
    } else {
      a->entry[v] = 0;
    }
  }
}

void pack_Address(struct Address * a, unsigned char * record, int * len) {
  unsigned char *buffer;
  unsigned long contents;
  unsigned long v;
  unsigned long phoneflag;
  unsigned char offset;
  int l;

  *len = 9;
  buffer = record + 9;

  phoneflag = 0;
  contents = 0;
  offset = 0; /* FIXME: Check to see if this really should default to zero */

  for(v=0;v<19;v++) {
    if(a->entry[v]) {
      if(v==entryCompany)
        offset = (unsigned char)(*len - 8);
      contents |= (1 << v);
      l = strlen(a->entry[v])+1;
      memcpy(buffer,a->entry[v],l);
      *len+=l;
      buffer+=l;
    }
  }

  phoneflag  = ((unsigned long)a->phonelabel1) << 0;
  phoneflag |= ((unsigned long)a->phonelabel2) << 4;
  phoneflag |= ((unsigned long)a->phonelabel3) << 8;
  phoneflag |= ((unsigned long)a->phonelabel4) << 12;
  phoneflag |= ((unsigned long)a->phonelabel5) << 16;
  phoneflag |= ((unsigned long)a->whichphone)  << 20;

  set_long(record, phoneflag);
  set_long(record+4, contents);
  set_byte(record+8, offset);
}

void unpack_AddressAppInfo(struct AddressAppInfo * ai, unsigned char * record, int len) {
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
  record += 4;
  ai->dirtyfieldlabels = get_long(record);
  record += 4;
  memcpy(ai->labels,record, 16*22);
  record += 16*22;
  ai->country = get_short(record);
  record+=2;
  ai->sortByCompany = get_byte(record);
}

void pack_AddressAppInfo(struct AddressAppInfo *, unsigned char * AppInfo, int * len);
