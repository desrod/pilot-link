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
  if(a->lastname) free(a->lastname);
  /* FIXME: Etc!! */
}

#define hi(x) (((x) >> 4) & 0x0f)
#define lo(x) ((x) & 0x0f)
#define pair(x,y) (((x) << 4) | (y))

void unpack_Address(struct Address * a, unsigned char * buffer, int len) {
  unsigned long contents;
  unsigned long v;
  
  /*get_byte(buffer);*/
  a->whichphone = hi(get_byte(buffer+1));
  a->phonelabel5 = lo(get_byte(buffer+1));
  a->phonelabel4 = hi(get_byte(buffer+2));
  a->phonelabel3 = lo(get_byte(buffer+2));
  a->phonelabel2 = hi(get_byte(buffer+3));
  a->phonelabel1 = lo(get_byte(buffer+3));
  
  contents = get_long(buffer+4);
  
  /*get_byte(buffer+8)*/
  
  buffer += 9;
  
  /*if(flag & 0x1) { 
     a->lastname = strdup(buffer);
     buffer += strlen(buffer) + 1;
  } else {
    a->lastname = 0;
  }*/
  
  v = 1;
#define extract(name) \
  if(contents & v){ a->name = strdup(buffer); buffer +=strlen(buffer)+1;}else{a->name=0;}v<<=1;
  
  extract(lastname);
  extract(firstname);
  extract(company);
  extract(phone1);
  extract(phone2);
  extract(phone3);
  extract(phone4);
  extract(phone5);
  extract(address);
  extract(city);
  extract(state);
  extract(zip);
  extract(country);
  extract(title);
  extract(custom1);
  extract(custom2);
  extract(custom3);
  extract(custom4);
  extract(note);
}

void pack_Address(struct Address *, unsigned char * record, int * len);
                  
