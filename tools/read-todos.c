/* todos.c:  Translate Palm ToDo database into generic format
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
#include "pi-todo.h"
#include "pi-dlp.h"
#include "pi-file.h"
#include "pi-header.h"

int main(int argc, char *argv[])
{
   struct pi_sockaddr addr;
   char *filename, *ptr;
   struct pi_file *pif = NULL;
   int db;
   int sd;
   int i;
   struct PilotUser U;
   int ret;
   unsigned char buffer[0xffff];
   struct ToDoAppInfo tai;
   char *progname = argv[0];
   char *port = argv[1];
      
   PalmHeader(progname);

   if (argc < 2) {
      fprintf(stderr, "   Usage:%s /dev/tty<0..n> | -f <ToDoDB.pdb>\n\n", argv[0]);
      exit(2);
   }

   if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP))) {
      perror("pi_socket");
      exit(1);
   }

   if (!strcmp(argv[1], "-f") && (argc > 2)) {
      filename = argv[2];
      port = 0;
   } else {
      port = argv[1];
      filename = 0;
   }

   if (port) {
      addr.pi_family = PI_AF_SLP;
      strcpy(addr.pi_device, port);

      PalmHeader(progname);
   
      ret = pi_bind(sd, (struct sockaddr *) &addr, sizeof(addr));
      if (ret == -1)
      {
         fprintf (stderr, "\n\007   ***** ERROR *****\n   Unable to bind to port '%s'...\n\n   Have you used -p or set $PILOTRATE properly?\n\n", port);
         fprintf (stderr, "   Please see 'man 1 %s' or '%s --help' for information\n   on setting the port.\n\n", progname, progname);
         exit (1);
      }
    
      printf("   Port: %s\n\n   Please insert the Palm in the cradle and press the HotSync button.\n", port);

      ret = pi_listen(sd, 1);
      if (ret == -1) {
	 perror("pi_listen");
	 exit(1);
      }

      sd = pi_accept(sd, 0, 0);
      if (sd == -1) {
	 perror("pi_accept");
	 exit(1);
      }

      /* Ask the pilot who it is. */
      dlp_ReadUserInfo(sd, &U);

      /* Tell user (via Palm) that we are starting things up */
      dlp_OpenConduit(sd);

      /* Open the ToDo database, store access handle in db */
      if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "ToDoDB", &db) < 0) {
	 puts("Unable to open ToDoDB");
	 dlp_AddSyncLogEntry(sd, "Unable to open ToDoDB.\n");
	 exit(1);
      }

      dlp_ReadAppBlock(sd, db, 0, buffer, 0xffff);
   }

   if (filename) {
      int len;

      pif = pi_file_open(filename);
      if (!pif) {
	 perror("pi_file_open");
	 exit(1);
      }

      ret = pi_file_get_app_info(pif, (void *) &ptr, &len);
      if (ret == -1) {
	 perror("pi_file_get_app_info");
	 exit(1);
      }

      memcpy(buffer, ptr, len);
   }

   unpack_ToDoAppInfo(&tai, buffer, 0xffff);

   for (i = 0;; i++) {
      struct ToDo t;
      int attr, category;

      int len;

      if (port) {
	 len =
	     dlp_ReadRecordByIndex(sd, db, i, buffer, 0, 0, &attr,
				   &category);

	 if (len < 0)
	    break;
      }
      if (filename) {
	 if (pi_file_read_record
	     (pif, i, (void *) &ptr, &len, &attr, &category, 0))
	    break;

	 memcpy(buffer, ptr, len);
      }

      /* Skip deleted records */
      if ((attr & dlpRecAttrDeleted) || (attr & dlpRecAttrArchived))
	 continue;

      unpack_ToDo(&t, buffer, len);

      printf("Category: %s\n", tai.category.name[category]);
      printf("Priority: %d\n", t.priority);
      printf("Completed: %s\n", t.complete ? "Yes" : "No");
      if (t.indefinite)
	 puts("Due: No Date");
      else
	 printf("Due: %s", asctime(&t.due));
      if (t.description)
	 printf("Description: %s\n", t.description);
      if (t.note)
	 printf("Note: %s\n", t.note);
      printf("\n");

      free_ToDo(&t);
   }

   if (port) {
      /* Close the database */
      dlp_CloseDB(sd, db);

      dlp_AddSyncLogEntry(sd, "Read todos from Palm.\n");

      pi_close(sd);
   }
   if (filename) {
      pi_file_close(pif);
   }

   return 0;
}
