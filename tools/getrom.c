/* getrom:  Fetch ROM image for CoPilot or POSE
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 *
 * Written by Kenneth Albanowski, based on code by Greg Hewgill
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "pi-source.h"
#include "pi-serial.h"
#include "pi-header.h"

#include <termios.h>

void Help(char *progname)
{
   fprintf(stderr, "   Usage: %s [-2] %s\n\n", progname, TTYPrompt);
   exit(2);
}

struct record *records = 0;

int main(int argc, char *argv[])
{
   int l, p;
   char buf[0xffff];
   char *progname = argv[0];
   char *port = argv[1];
   int i;
   struct pi_sockaddr addr;
   unsigned char check;
   struct pi_socket ps;
   extern char *optarg;
   extern int optind;
   int rom;
   int version = 1;
   int max;

   PalmHeader(progname);

   if (argc < 2)
      Help(progname);

   if (strcmp(argv[1], "-2") == 0) {
      version = 2;
      argv++;
      argc--;
   }

   addr.pi_family = PI_AF_SLP;
   strcpy(addr.pi_device, port);

   ps.mac = calloc(1, sizeof(struct pi_mac));

   ps.rate = 38400;
   ps.sd = 0;
   if (pi_serial_open(&ps, &addr, sizeof(addr)) == -1) {
      fprintf(stderr, "   Unable to open port %s\n\n", port);
      exit(0);
   }

   rom =
       open((version == 2) ? "pilot2.rom" : "pilot.rom",
	    O_WRONLY | O_CREAT | O_TRUNC, 0666);
   if (rom == -1) {
      perror("Unable to create pilot.rom");
      exit(0);
   }

   printf("
   NOTICE: Use of this program may place you in violation of your license
   agreement with Palm Computing, Inc. Please read the details in your Palm
   handbook (\"Software License Agreement\") before running this program.
   \n\n");

   printf("   Please launch %s on your Palm device.\n",
	  (version == 2) ? "Getrom2.prc" : "Getrom.prc");
   printf("   Port: %s\n\n   Please insert the Palm in the cradle and press the HotSync button.\n", argv[1]);

   max = (version == 2) ? 256 : 128;
   for (i = 0; i < max; i++) {
      do {
	 l = read(ps.mac->fd, buf, 1);
	 if (l < 1)
	    continue;
      } while (buf[0] != '*');
      printf("\r%d/%d", i + 1, max);
      fflush(stdout);

      p = 0;
      do {
	 l = read(ps.mac->fd, buf + p, 4096 - p);
	 if (l < 0) {
	    perror("Unable to read sync byte");
	 }
	 p += l;
      } while (p < 4096);

      check = 0xff;
      if (read(ps.mac->fd, buf + 4096, 1) < 0) {
	 perror("Unable to read checksum byte");
	 goto error;
      }

      for (p = 0; p < 4096; p++)
	 check = ((check << 1) | check >> 7) ^ buf[p];

      if ((unsigned char) buf[4096] != check) {
	 printf("\nChecksum error\n");
	 goto error;
      }

      write(rom, buf, 4096);

      buf[4096] = '+';

      write(ps.mac->fd, buf + 4096, 1);
   }
   printf("\nSuccessful!\n");

 error:
   close(rom);

   ps.serial_close(&ps);

   return 0;
}
