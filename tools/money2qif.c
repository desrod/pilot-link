/* money2qif.c:  Translate Palm MoneyManager database into QIF format
 *
 * Copyright (c) 1998, Rui Oliveira
 * Copyright (c) 1996, Kenneth Albanowski (original read-todos.c)
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-money.h"
#include "pi-dlp.h"
#include "pi-header.h"

int main(int argc, char *argv[])
{
   struct pi_sockaddr addr;
   int db;
   int sd;
   int i;
   char *nl;
   char *progname = argv[0];
   char *port = argv[1];
   struct PalmUser U;
   int ret;
   unsigned char buffer[0xffff];
   struct MoneyAppInfo mai;

   PalmHeader(progname);

   if (argc < 3) {
      fprintf(stderr, "   Usage: %s %s Account\n\n", argv[0], TTYPrompt);
      exit(2);
   }
   if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP))) {
      perror("pi_socket");
      exit(1);
   }
 
   addr.pi_family = PI_AF_SLP;
   strcpy(addr.pi_device, port);

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

   /* Open the Money database, store access handle in db */
   if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "MoneyDB", &db) < 0) {
      puts("Unable to open MoneyDB");
      dlp_AddSyncLogEntry(sd, "Unable to open MoneyDB.\n");
      exit(1);
   }

   dlp_ReadAppBlock(sd, db, 0, buffer, 0xffff);
   unpack_MoneyAppInfo(&mai, buffer, 0xffff);

   for (i = 0; i < 16; i++)
      if (!strcmp(mai.category.name[i], argv[2]))
	 break;

   if (i < 16) {

      printf("!Type:Bank\n");

      for (i = 0;; i++) {
	 struct Transaction t;
	 int attr, category;

	 int len =

	     dlp_ReadRecordByIndex(sd, db, i, buffer, 0, 0, &attr,
				   &category);

	 if (len < 0)
	    break;

	 /* Skip deleted records */
	 if ((attr & dlpRecAttrDeleted) || (attr & dlpRecAttrArchived))
	    continue;

	 if (strcmp(mai.category.name[category], argv[2]))
	    continue;

	 unpack_Transaction(&t, buffer, len);

	 printf("D%02d/%02d/%2d\n", t.month, t.day, t.year - 1900);
	 if (t.checknum)
	    printf("N%d\n", t.checknum);
	 printf("T%ld.%02d\n", t.amount, t.amountc);
	 printf("P%s\n", t.description);
	 if (t.xfer == category)
	    printf("L%s\n", mai.typeLabels[(int) t.type]);
	 else
	    printf("L[%s]\n", mai.category.name[(int) t.xfer]);
	 if (strcmp(t.note, "")) {
	    while ((nl = strchr(t.note, '\n')))
	       *nl = ' ';
	    printf("M%s\n", t.note);
	 }
	 if (t.flags & 1)
	    printf("CX\n");
	 printf("\n^\n");

      }
   }

   /* Close the database */
   dlp_CloseDB(sd, db);

   dlp_AddSyncLogEntry(sd, "Read MoneyDB from Palm.\n");

   pi_close(sd);
   return 0;
}
