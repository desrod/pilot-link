/* install-user.c:  This will push a valid Username to your Palm
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

/* Note: if you use this program to change the user name on the Palm, I
 * _highly_ reccomend that you perform a hard reset before HotSyncing with a
 * Windows machine. This is because the user-id information has only been
 * partially altered, and it is not worth trying to predict what the Desktop
 * will do. - KJA
 */

#include <stdio.h>
#include <stdlib.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-header.h"

int main(int argc, char *argv[])
{
   struct pi_sockaddr addr;
   int sd;
   struct PilotUser U;
   struct SysInfo S;
   struct CardInfo C;
   struct NetSyncInfo N;
   unsigned long romversion;
   int ret;
   char *progname = argv[0];
   char *port = argv[1];

   PalmHeader(progname);

   if (argc < 3) {
      fprintf(stderr, "   Assigns your Palm device a user name and unique userid\n\n");
      fprintf(stderr, "   Usage: %s %s [User name] <userid>\n\n", argv[0], TTYPrompt);
      fprintf(stderr, "   Example: %s /dev/ttyS0 \"John Q. Public\" 12345\n\n", progname);
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
   if (sd < 0) {
      perror("pi_accept");
      exit(1);
   }

   /* Tell user (via Palm) that we are starting things up */
   dlp_OpenConduit(sd);

   dlp_ReadUserInfo(sd, &U);

   dlp_ReadSysInfo(sd, &S);

   C.card = -1;
   C.more = 1;
   while (C.more) {
      if (dlp_ReadStorageInfo(sd, C.card + 1, &C) < 0)
	 break;

      printf
	  (" Card #%d has %lu bytes of ROM, and %lu bytes of RAM (%lu of that is free)\n",
	   C.card, C.romSize, C.ramSize, C.ramFree);
      printf(" It is called '%s', and was made by '%s'.\n", C.name,
	     C.manufacturer);
   }

   if (argc == 2) {
      printf("Palm user %s\n", U.username);
      printf("UserID %ld \n", U.userID);
   } else {
      strcpy(U.username, argv[2]);
      if (argc == 4) {
	 U.userID = atoi(argv[3]);
      }
      U.lastSyncDate = time((time_t *) 0);
      dlp_WriteUserInfo(sd, &U);
   }

   printf
       ("Through ReadSysInfo: ROM Version: 0x%8.8lX, locale: 0x%8.8lX, name: '%s'\n",
	S.romVersion, S.locale, S.name);

   dlp_ReadFeature(sd, makelong("psys"), 1, &romversion);

   printf("ROM Version through ReadFeature: 0x%8.8lX\n", romversion);

   if (dlp_ReadNetSyncInfo(sd, &N) >= 0) {
      printf
	  ("NetSync: LAN sync = %d, Host name = '%s', address = '%s', netmask ='%s'\n",
	   N.lanSync, N.hostName, N.hostAddress, N.hostSubnetMask);
   }

   pi_close(sd);
   exit(0);
}
