/* debugsh.c:  Simple debugging console
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-syspkt.h"
#include "pi-header.h"

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

int done = 0;

void read_user(int sd)
{
   char line[256];
   int l = read(fileno(stdin), line, 256);

   if (l > 0)
      line[l - 1] = 0;

   if (strcmp(line, "apps") == 0) {
      sys_RemoteEvent(sd, 1, 5, 170, 0, 0, 0, 0);	/* Set the pen down */
      sys_RemoteEvent(sd, 0, 5, 170, 0, 0, 0, 0);	/* And then lift it up */
   } else if (strcmp(line, "menu") == 0) {
      sys_RemoteEvent(sd, 1, 5, 200, 0, 0, 0, 0);	/* Set the pen down */
      sys_RemoteEvent(sd, 0, 5, 200, 0, 0, 0, 0);	/* And then lift it up */
   } else if (strcmp(line, "reboot") == 0) {
      RPC(sd, 1, 0xA08C, 2, RPC_End);
   } else if (strcmp(line, "coldboot") == 0) {
      RPC(sd, 1, 0xA08B, 2, RPC_Long(0), RPC_Long(0), RPC_Long(0),
	  RPC_Long(0), RPC_End);
   } else if (strcmp(line, "numdb") == 0) {
      printf("Number of databases on card 0: %d\n",
	     RPC(sd, 1, 0xA043, 0, RPC_Short(0), RPC_End)
	  );
   } else if (strcmp(line, "dbinfo") == 0) {
      long creator, type, appInfo, sortInfo, modnum, backdate, moddate,
	  crdate, version, attr;
      char name[32];

      int id = RPC(sd, 1, 0xA044, 0, RPC_Short(0), RPC_Short(0), RPC_End);

      RPC(sd, 1, 0xA046, 0, RPC_Short(0), RPC_Long(id),
	  RPC_Ptr(name, 32),
	  RPC_ShortRef(attr), RPC_ShortRef(version), RPC_LongRef(crdate),
	  RPC_LongRef(moddate), RPC_LongRef(backdate), RPC_LongRef(modnum),
	  RPC_LongRef(appInfo), RPC_LongRef(sortInfo), RPC_LongRef(type),
	  RPC_LongRef(creator), RPC_End);

      printf("The name of db 0 (LocalID %x) is %s\n", id, name);

   } else if (strcmp(line, "quit") == 0) {
      done = 1;
   } else if (l > 1) {
      printf
	  ("unknown command '%s' (try 'apps', 'menu', 'coldboot', 'reboot', 'dbinfo', or 'quit')\n",
	   line);
   }

   if (!done) {
      printf("debugsh>");
      fflush(stdout);
   }
   if (l == 0)
      done = 1;
}

void read_pilot(int sd)
{
   char buf[4096];
   int l = pi_read(sd, buf, 4096);

   puts("From Palm:");
   dumpdata((unsigned char *) buf, l);

   if (buf[2] == 0) {		/* SysPkt command */
      if (buf[0] == 1) {	/* Console */
	 if (buf[4] == 0x7f) {	/* Message from Palm */
	    int i;

	    for (i = 6; i < l; i++)
	       if (buf[i] == '\r')
		  buf[i] = '\n';
	    printf("%s", buf + 6);
	 }
      }
   }

   if (!done) {
      printf("debugsh>");
      fflush(stdout);
   }

}

void sig(int signal)
{
   done = 1;
}

int main(int argc, char *argv[])
{
   struct pi_sockaddr laddr;
   int sd;
   fd_set r, rin;
   int max;
   char *progname = argv[0];

   PalmHeader(progname);

   if (argc < 2) {
      fprintf(stderr, "   Usage: %s %s\n\n", argv[0], TTYPrompt);
      exit(2);
   }

   if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_RAW, PI_PF_SLP))) {
      perror("pi_socket");
      exit(1);
   }

   laddr.pi_family = PI_AF_SLP;
   strcpy(laddr.pi_device, argv[1]);

   pi_bind(sd, (struct sockaddr *) &laddr, sizeof(laddr));

   /* Now we can read and write packets: to get the Palm to send a packet,
      write a ".2" shortcut, which starts the debugging mode. (Make sure to
      reset your Palm after finishing this example!) */

   FD_ZERO(&r);
   FD_SET(sd, &r);
   FD_SET(fileno(stdin), &r);

   max = sd;
   if (fileno(stdin) > max)
      max = fileno(stdin);

   printf("debugsh>");
   fflush(stdout);

   signal(SIGINT, sig);

   while (!done) {
      rin = r;
      if (select(max + 1, &rin, 0, 0, 0) >= 0) {
	 if (FD_ISSET(fileno(stdin), &rin)) {
	    read_user(sd);
	 } else if (FD_ISSET(sd, &rin)) {
	    read_pilot(sd);
	 }
      } else {
	 break;
      }
   }

   printf("\n   Exiting...\n");
   pi_close(sd);

   return 0;
}
