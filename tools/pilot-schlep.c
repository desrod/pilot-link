/* pilot-schlep.c:  Utility to transfer arbitrary data to/from your Palm
 *
 * (c) 1996, Kenneth Albanowski.
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-file.h"
#include "pi-dlp.h"
#include "pi-header.h"

#define pi_mktag(c1,c2,c3,c4) (((c1)<<24)|((c2)<<16)|((c3)<<8)|(c4))

int sd = 0;
char *device;
char *progname;

RETSIGTYPE SigHandler(int signal);

void Connect(void)
{
   struct pi_sockaddr addr;
   int ret;

   if (sd != 0)
      return;

   signal(SIGHUP, SigHandler);
   signal(SIGINT, SigHandler);
   signal(SIGSEGV, SigHandler);

   if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP))) {
      perror("pi_socket");
      exit(1);
   }

   addr.pi_family = PI_AF_SLP;
   strcpy(addr.pi_device, device);

   PalmHeader(progname);

   ret = pi_bind(sd, (struct sockaddr *) &addr, sizeof(addr));
   if (ret == -1)
   {
      fprintf (stderr, "\n\007   ***** ERROR *****\n   Unable to bind to port '%s'...\n\n   Have you used -p or set $PILOTRATE properly?\n\n", device);
      fprintf (stderr, "   Please see 'man 1 %s' or '%s --help' for information\n   on setting the port.\n\n", progname, progname);
      exit (1);
   }
   
   printf("   Port: %s\n\n   Please insert the Palm in the cradle and press the HotSync button.\n", device);

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

   fprintf(stderr, "Connected\n");
}

void Disconnect(void)
{
   if (sd == 0)
      return;

   dlp_EndOfSync(sd, 0);
   pi_close(sd);
   sd = 0;
}

RETSIGTYPE SigHandler(int signal)
{
   fprintf(stderr, "   Abort on signal!\n");
   Disconnect();
   exit(3);
}

void Delete(void)
{
   Connect();

   if (dlp_OpenConduit(sd) < 0) {
      fprintf(stderr, "Exiting on cancel, data not deleted.\n");
      exit(1);
   }

   dlp_DeleteDB(sd, 0, "Schlep");

   fprintf(stderr, "Delete done.\n");
}

int segment = 4096;

void Install(void)
{
   unsigned long len;
   int j;
   int db;
   int l;
   char buffer[0xffff];

   if (isatty(fileno(stdin))) {
      fprintf(stderr,
	      "Cannot install from tty, please redirect from file.\n");
      exit(1);
   }

   Connect();

   if (dlp_OpenConduit(sd) < 0) {
      fprintf(stderr, "Exiting on cancel. Data not installed.\n");
      exit(1);
   }

   dlp_DeleteDB(sd, 0, "Schlep");
   if (dlp_CreateDB
       (sd, pi_mktag('S', 'h', 'l', 'p'), pi_mktag('D', 'A', 'T', 'A'), 0,
	dlpDBFlagResource, 1, "Schlep", &db) < 0)
      return;

   fprintf(stderr, "Please wait, installing data");

   l = 0;
   for (j = 0; (len = read(fileno(stdin), buffer, segment)) > 0; j++) {
      if (dlp_WriteResource
	  (sd, db, pi_mktag('D', 'A', 'T', 'A'), j, buffer, len) < 0)
	 break;
      l += len;
      fprintf(stderr, ".");
   }
   fprintf(stderr, "\n%d bytes written\n", l);

   dlp_CloseDB(sd, db);
}

void Fetch(void)
{
   int db;
   int i;
   int l;
   char buffer[0xffff];

   Connect();

   if (dlp_OpenConduit(sd) < 0) {
      fprintf(stderr, "Exiting on cancel, data not retrieved.\n");
      exit(1);
   }

   if (dlp_OpenDB(sd, 0, dlpOpenRead, "Schlep", &db) < 0)
      return;

   for (i = 0;
	(l =
	 dlp_ReadResourceByType(sd, db, pi_mktag('D', 'A', 'T', 'A'), i,
				buffer, 0, 0)) > 0; i++) {
      write(fileno(stdout), buffer, l);
   }
}

void Help(void)
{
   PalmHeader(progname);

   fprintf(stderr, "   Usage: %s %s command(s)\n", progname, TTYPrompt);
   fprintf(stderr, "
     -i[nstall] < file_to_install
     -f[etch]   > file_to_write_to
     -d[elete]
   
   Examples:

    To install a file on your Palm:\n");    
   fprintf(stderr, "      %s /dev/ttyS0 -i < MyProg.prc\n\n", progname);
   fprintf(stderr, "    To extract a file from your Palm:\n");
   fprintf(stderr, "      %s /dev/ttyS3 -f > ExtractedProg.prc\n\n", progname);
   exit(0);
}

int main(int argc, char *argv[])
{
   int c;
   extern char *optarg;
   extern int optind;

   progname = argv[0];

   if (argc < 3) {
      Help();
   }

   device = argv[1];

   optind = 2;
   while ((c = getopt(argc, argv, "ifdh")) != -1) {
      switch (c) {
      case 'i':
	 Install();
	 break;
      case 'f':
	 Fetch();
	 break;
      case 'd':
	 Delete();
	 break;
      default:
      case 'h':
      case '?':
	 Help();
      }
   }

   Disconnect();

   return 0;
}
