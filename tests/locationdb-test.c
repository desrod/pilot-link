/*
 * $Id$
 *
 * calendardb-test.c: Test parsing the PalmOne timezone databases
 * loclLDefLocationDB and loclCusLocationDB.
 
 * (c) 2008, Jon Schewe
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
*/

#ifdef TIME_WITH_SYS_TIME 	 
# include <sys/time.h> 	 
# include <time.h> 	 
#else 	 
# ifdef HAVE_SYS_TIME_H 	 
#  include <sys/time.h> 	 
# else 	 
#  include <time.h> 	 
# endif 	 
#endif

#include "pi-source.h"
#include "pi-file.h"
#include "pi-macros.h"
#include "pi-location.h"

/***********************************************************************
 *
 * Function:    dump
 *
 * Summary:     Dump data as requested by other functions
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void dump(void *buf, int n)
{
  int 	ch,
    i,
    j;

  for (i = 0; i < n; i += 16) {
    printf("%04x: ", i);
    for (j = 0; j < 16; j++) {
      if (i + j < n)
        printf("%02x ",
               ((unsigned char *) buf)[i + j]);
      else
        printf("   ");
    }
    printf("  ");
    for (j = 0; j < 16 && i + j < n; j++) {
      ch = ((unsigned char *) buf)[i + j] & 0x7f;
      if (ch < ' ' || ch >= 0x7f)
        putchar('.');
      else
        putchar(ch);
    }
    printf("\n");
  }
}

/**
 * Test parsing database.
 */
void parse(pi_file_t *pf)
{
  int result;
  int nentries;
  char *buf;
  int attrs, cat;
  size_t size;
  recordid_t uid;
  Location_t loc;
  int entnum;
  pi_buffer_t *pi_buf;
  pi_buffer_t *test;
  
  pi_file_get_entries(pf, &nentries);
  printf("Number of entries: %d\n", nentries);
  
  for (entnum = 0; entnum < nentries; entnum++) {
    if (pi_file_read_record(pf, entnum, (void **) &buf, &size,
                            &attrs, &cat, &uid) < 0) {
      printf("Error reading record number %d\n", entnum);
      return;
    }

    /* Skip deleted records */
    if ((attrs & dlpRecAttrDeleted)
        || (attrs & dlpRecAttrArchived)) {
      continue;
    }

    printf("original record %d\n", entnum);
    dump(buf, size);

    pi_buf = pi_buffer_new(size);
    pi_buffer_append(pi_buf, buf, size);
      
    result = unpack_Location(&loc, pi_buf);
    if(result == -1) {
      printf("Error unpacking record %d!\n", entnum);
    } else {
      printf("Timezone name: %s\n", loc.tz.name);
      printf("\tOffset from GMT: %d minutes\n", loc.tz.offset);
      printf("\tDST observed: %d\n", loc.tz.dstObserved);
      printf("\tNote is: %s\n", loc.note);

      printf("\tlat: %d degrees %d minutes\n", loc.latitude.degrees, loc.latitude.minutes);
      printf("\tlon: %d degrees %d minutes\n", loc.longitude.degrees, loc.longitude.minutes);
      
    }


    /* now try and pack the record and see if we get the same data */
    test = pi_buffer_new(0);
    result = pack_Location(&loc, test);
    if(result == -1) {
      printf("Error packing record %d!\n", entnum);
    } else {
      printf("packed record\n");
      dump(test->data, test->used);
      
      if(pi_buf->used != test->used) {
        int i;
        printf("Error: Different record sizes unpack: %d pack: %d last byte unpack: 0x%02X pack: 0x%02X\n", pi_buf->used, test->used, pi_buf->data[pi_buf->used-1], test->data[test->used-1]);
        for(i=0; i<pi_buf->used; ++i) {
          if(pi_buf->data[i] != test->data[i]) {
            printf("Error: Byte %d is different unpack: 0x%02X pack: 0x%02X\n", i, pi_buf->data[i], test->data[i]);
          }
        }
      }
    }
    
      
    pi_buffer_free(test);
    
    pi_buffer_free(pi_buf);
    free_Location(&loc);
  }
}

int main(int argc, char **argv)
{
  pi_file_t *pf;
  struct DBInfo info;
        
  if (argc != 2) {
    fprintf(stderr, "Usage: %s [.pdb file]\n", *argv);
    return 1;
  }


  if ((pf = pi_file_open(*(argv + 1))) == NULL) {
    perror("pi_file_open");
    return 1;
  }

  pi_file_get_info(pf, &info);

  parse(pf);

  pi_file_close(pf);

  return 0;
}
