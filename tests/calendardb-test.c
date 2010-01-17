/*
 * $Id$
 *
 * calendardb-test.c: Test code for calendar database
 
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
#include "pi-calendar.h"

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

  if(NULL == buf) {
    printf("Null buf\n");
    return;
  }
  
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
  void *app_info;
  size_t app_info_size;
  int i;
  CalendarAppInfo_t cab;
  int result;
  int nentries;
  unsigned char *buf;
  int attrs, cat;
  size_t size;
  recordid_t uid;
  CalendarEvent_t appt;
  int entnum;
  pi_buffer_t *pi_buf;
  pi_buffer_t *test;
  char timeString[256];
  
  pi_file_get_app_info(pf, &app_info, &app_info_size);
  if (app_info == NULL) {
    fprintf(stderr, "Unable to get app info\n");
    return;
  }
  printf("Size of appinfo block: %d\n", (int)app_info_size);

  // dump the appinfo
  dump(app_info, app_info_size);

  pi_buf = pi_buffer_new(0);
  pi_buf->data = app_info;
  pi_buf->used = app_info_size;
  pi_buf->allocated = app_info_size;
   
  result = unpack_CalendarAppInfo(&cab, pi_buf);
  printf("unpack_CalendarAppInfo returned %d\n", result);

  pi_buf->data=NULL;
  pi_buf->used = 0;
  pi_buf->allocated = 0;

  result = pack_CalendarAppInfo(&cab, pi_buf);
  printf("pack_CalendarAppInfo returned %d\n", result);

  // dump the appinfo
  dump(pi_buf->data, pi_buf->used);

  pi_buffer_free(pi_buf);

  /* print out the standard app info stuff and see if it's right */
  /* not really useful for Calendar, all names are custom
  for(i=0; i<16; ++i) {
    printf("renamed[%d] = %d\n", i, cab.category.renamed[i]);
  }
  */
        
  for(i=0; i<16; ++i) {
    printf("Category %d is %s\n", i, cab.category.name[i]);
  }

  printf("Start of week is %d\n", cab.startOfWeek);

  printf("Internal:\n");
  dump(cab.internal, 18);

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

    printf("original record %d:\n", entnum);
    dump(buf, size);

    pi_buf = pi_buffer_new(size);
    pi_buffer_append(pi_buf, buf, size);
    
    result = unpack_CalendarEvent(&appt, pi_buf, calendar_v1);
    if(-1 == result) {
      printf("Error unpacking record %d!\n", entnum);
    }

    printf("\tdescription: %s\n", appt.description);
    if(NULL != appt.location) {
      printf("\tLocation: %s\n", appt.location);
    }

    printf("\tis event: %d\n", appt.event);
    
    result = strftime(timeString, 256, "%F %T %z", &(appt.begin));
    if(0 == result) {
      printf("Error begin time to string!");
    } else {
      printf("\tBegin: %s\n", timeString);
    }
    result = strftime(timeString, 256, "%F %T %z", &(appt.end));
    if(0 == result) {
      printf("Error end time to string!");
    } else {
      printf("\tEnd: %s\n", timeString);
    }
    
    printf("\tnote: %s\n", appt.note);
    
    if(NULL != appt.tz) {
      printf("\tTimezone name: %s\n", appt.tz->name);
      printf("\t\tOffset from GMT: %d minutes\n", appt.tz->offset);
      printf("\t\tDST observed: %d\n", appt.tz->dstObserved);
      printf("\t\tDST start:\n");
      printf("\t\t\tdayOfWeek: %d\n", appt.tz->dstStart.dayOfWeek);
      printf("\t\t\tweekOfMonth: %d\n", appt.tz->dstStart.weekOfMonth);
      printf("\t\t\tmonth: %d\n", appt.tz->dstStart.month);
      printf("\t\tDST end:\n");
      printf("\t\t\tdayOfWeek: %d\n", appt.tz->dstEnd.dayOfWeek);
      printf("\t\t\tweekOfMonth: %d\n", appt.tz->dstEnd.weekOfMonth);
      printf("\t\t\tmonth: %d\n", appt.tz->dstEnd.month);
    }

    /* now try and pack the record and see if we get the same data */
    test = pi_buffer_new(0);
    result = pack_CalendarEvent(&appt, test, calendar_v1);
    if(result == -1) {
      printf("Error packing record %d!\n", entnum);
    } else {
      printf("packed record\n");
      dump(test->data, test->used);
      
      if(pi_buf->used != test->used) {
        int i;
        printf("Error: Different record sizes unpack: %ld pack: %ld last byte unpack: 0x%02X pack: 0x%02X\n", pi_buf->used, test->used, pi_buf->data[pi_buf->used-1], test->data[test->used-1]);
        for(i=0; i<pi_buf->used; ++i) {
          if(pi_buf->data[i] != test->data[i]) {
            printf("Error: Byte %d is different unpack: 0x%02X pack: 0x%02X\n", i, pi_buf->data[i], test->data[i]);
          }
        }
      }
    }
    
    /*printf("event: %d\n", appt.event);*/

    /*
      if (appt.untimed() == false) {
      cout << "Begin Time:  " << asctime(appt.
      beginTime());
      cout << "End Time:    " << asctime(appt.endTime());
      } else {
      cout << "Untimed event" << endl;
      }

      if (appt.hasAlarm()) {
      cout << "The alarm is set to go off " << appt.
      advance() << " ";

      switch (appt.advanceUnits()) {
      case appointment_t::minutes:
      cout << "minutes";
      break;
      case appointment_t::hours:
      cout << "hours";
      break;
      case appointment_t::days:
      cout << "days";
      default:
      cout << "(internal error)";
      }

      cout << " before the event" << endl;
      } else
      cout << "There is not an alarm set for this event"
      << endl;

      if (appt.repeatType() != appointment_t::none)
      prettyPrintRepeat(&appt);
      else
      cout << "Event does not repeat" << endl;

      if ((timePtr = appt.exceptions())) {
      size = appt.numExceptions();
      cout << "I seem to have " << size << " exceptions:"
      << endl;
      for (int i = 0; i < size; i++)
      cout << "\t" << asctime(&timePtr[i]);
      }

      cout << "Description: " << appt.description() << endl;

      if (appt.note()) {
      cout << "Note: " << appt.note() << endl;
      }

      cout << endl;
    */

    pi_buffer_free(pi_buf);
    free_CalendarEvent(&appt);
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
