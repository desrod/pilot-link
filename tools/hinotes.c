/* hinotes.c:  Translate Palm Hi-Note Text Memos into e-mail format
 *
 * Copyright (c) 1996, Kenneth Albanowski
 * Based on code by Bill Goodman, modified by Michael Bravo
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-hinote.h"
#include "pi-dlp.h"
#include "pi-header.h"

/*
 * SHOULD INCLUDE HEADERS FOR mkdir()
 */

#define PILOTPORT "/dev/pilot"

/*
 * constants to determine how to produce memos
 */

#define MEMO_MBOX_STDOUT 0
#define MEMO_DIRECTORY 1

#define MAXDIRNAMELEN 1024

char *progname;

/*
 * function prototypes
 */

void Help(char *progname);
int main(int argc, char *argv[]);
void write_memo_mbox(struct HiNoteNote m, struct HiNoteAppInfo mai,
		     int category);

void write_memo_in_directory(char *dirname, struct HiNoteNote m,
			     struct HiNoteAppInfo mai, int category);

void Help(char *progname)
{
   fprintf(stderr, "   Usage: %s [options]\n", progname);
   fprintf(stderr, "
   Options:
     -q          = do not prompt for HotSync button press
     -p port     = use device file <port> to communicate with Palm
     -d dir      = save memos in <dir> instead of writing to STDOUT
     -h|-?       = print this help summary

   By default, the contents of your Palm's memo database will be written
   to standard output as a standard Unix mailbox (mbox-format) file, with
   each memo as a separate message.  The subject of each message will be the
   category.

   If `-d' is specified, than instead of being written to standard output,
   will be saved in subdirectories of <dir>.  Each subdirectory will be the
   name of a category on the Palm, and will contain the memos in that
   category.  Each memo's filename will be the first line (up to the first
   40 characters) of the memo.  Control characters, slashes, and equal
   signs that would otherwise appear in filenames are converted after the
   fashion of MIME's quoted-printable encoding.  
   
   Note that if you have two memos in the same category whose first lines 
   are identical, one of them will be overwritten.

   The serial port to connect to may be specified by the $PILOTPORT environment
   variable instead of by `-p' on the command line. If not specified anywhere
   it will default to /dev/pilot.\n\n");
   exit(0);
}

int main(int argc, char *argv[])
{
   struct pi_sockaddr addr;
   int db;
   int sd;
   int i;
   struct PilotUser U;
   int ret;
   unsigned char buffer[0xffff];
   char appblock[0xffff];
   struct HiNoteAppInfo mai;

   int quiet = 0;
   int mode = MEMO_MBOX_STDOUT;
   char dirname[MAXDIRNAMELEN] = "";
   int c;
   extern char *optarg;
   extern int optind;

   char *progname = argv[0];

   PalmHeader(progname);

   if (getenv("PILOTPORT")) {
      strcpy(addr.pi_device, getenv("PILOTPORT"));
   } else {
      strcpy(addr.pi_device, PILOTPORT);
   }

   while (((c = getopt(argc, argv, "qp:d:h?")) != -1)) {
      switch (c) {
      case 'q':
	 quiet = 1;
	 break;
      case 'p':
	 /* optarg is name of port to use instead of $PILOTPORT or /dev/pilot */
	 strcpy(addr.pi_device, optarg);
	 break;
      case 'd':
	 /* optarg is name of directory to create and store memos in */
	 strcpy(dirname, optarg);
	 mode = MEMO_DIRECTORY;
	 break;
      case 'h':
      case '?':
	 Help(progname);
      }
   }

   if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP))) {
      perror("pi_socket");
      exit(1);
   }

   addr.pi_family = PI_AF_SLP;

   ret = pi_bind(sd, (struct sockaddr *) &addr, sizeof(addr));
   if (ret == -1)
   {
      fprintf (stderr, "\n\007   ***** ERROR *****\n   Unable to bind to port '%s'...\n\n   Have you used -p or set $PILOTRATE properly?\n\n", addr.pi_device);
      fprintf (stderr, "   Please see 'man 1 %s' or '%s --help' for information\n   on setting the port.\n\n", progname, progname);
      exit (1);
   }
   
   printf("   Port: %s\n\n   Please insert the Palm in the cradle and press the HotSync button.\n", addr.pi_device);

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

   /* Ask the palm who it is. */
   dlp_ReadUserInfo(sd, &U);

   /* Tell user (via Palm) that we are starting things up */
   dlp_OpenConduit(sd);

   /* Open the Memo Pad's database, store access handle in db */
   if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "Hi-NoteDB", &db) < 0) {
      puts("Unable to open Hi-NoteDB");
      dlp_AddSyncLogEntry(sd, "Unable to open Hi-NoteDB.\n");
      exit(1);
   }

   dlp_ReadAppBlock(sd, db, 0, (unsigned char *) appblock, 0xffff);
   unpack_HiNoteAppInfo(&mai, (unsigned char *) appblock, 0xffff);

   for (i = 0;; i++) {
      struct HiNoteNote m;
      int attr;
      int category;

      int len =

	  dlp_ReadRecordByIndex(sd, db, i, buffer, 0, 0, &attr, &category);
      if (len < 0)
	 break;

      /* Skip deleted records */
      if ((attr & dlpRecAttrDeleted) || (attr & dlpRecAttrArchived))
	 continue;

      unpack_HiNoteNote(&m, buffer, len);
      switch (mode) {
      case MEMO_MBOX_STDOUT:
	 write_memo_mbox(m, mai, category);
	 break;
      case MEMO_DIRECTORY:
	 write_memo_in_directory(dirname, m, mai, category);
	 break;
      }
   }

   /* Close the database */
   dlp_CloseDB(sd, db);

   dlp_AddSyncLogEntry(sd, "Read Hi-Notes from Palm.\n");

   pi_close(sd);

   return 0;
}

void write_memo_mbox(struct HiNoteNote m, struct HiNoteAppInfo mai,
		     int category)
{
   int j;

   printf("From Your.Palm Tue Oct  1 07:56:25 1996\n");
   printf("Received: Palm@p by hi-note Tue Oct  1 07:56:25 1996\n");
   printf("To: you@y\n");
   printf("Date: Thu, 31 Oct 1996 23:34:38 -0500\n");
   printf("Subject: ");
   /*
    * print category name in brackets in subject field
    */
   printf("[%s] ", mai.category.name[category]);
   /*
    * print (at least part of) first line as part of subject:
    */
   for (j = 0; j < 40; j++) {
      if ((!m.text[j]) || (m.text[j] == '\n'))
	 break;
      printf("%c", m.text[j]);
   }
   if (j == 40)
      printf("...\n");
   else
      printf("\n");
   puts("");
   puts(m.text);
}

void write_memo_in_directory(char *dirname, struct HiNoteNote m,
			     struct HiNoteAppInfo mai, int category)
{
   int j;
   char pathbuffer[MAXDIRNAMELEN + (128 * 3)] = "";
   char tmp[5] = "";
   FILE *fd;

   /* SHOULD CHECK IF DIRNAME EXISTS AND IS A DIRECTORY */
   mkdir(dirname, 0700);
   /* SHOULD CHECK IF THERE WERE PROBLEMS CREATING DIRECTORY */

   /* create a directory for the category */
   strncat(pathbuffer, dirname, MAXDIRNAMELEN);
   strcat(pathbuffer, "/");
   /* SHOULD MAKE SURE CATEGORY DOESN'T HAVE SLASHES IN IT */
   strncat(pathbuffer, mai.category.name[category], 60);
   /* SHOULD CHECK IF DIRNAME EXISTS AND IS A DIRECTORY */
   mkdir(pathbuffer, 0700);
   /* SHOULD CHECK IF THERE WERE PROBLEMS CREATING DIRECTORY */

   /* open the actual file to write */
   strcat(pathbuffer, "/");
   for (j = 0; j < 40; j++) {
      if ((!m.text[j]) || (m.text[j] == '\n'))
	 break;
      if (m.text[j] == '/') {
	 strcat(pathbuffer, "=2F");
	 continue;
      }
      if (m.text[j] == '=') {
	 strcat(pathbuffer, "=3D");
	 continue;
      }
      /* escape if it's an ISO8859 control character (note: some are printable on the Palm) */
      if ((m.text[j] | 0x7f) < ' ') {
	 tmp[0] = '\0';
	 sprintf(tmp, "=%2X", (unsigned char) m.text[j]);
      } else {
	 tmp[0] = m.text[j];
	 tmp[1] = '\0';
      }
      strcat(pathbuffer, tmp);
   }

   fprintf(stderr, "Opening file %s\n", pathbuffer);
   if (!(fd = fopen(pathbuffer, "w"))) {
      fprintf(stderr, "%s: can't open file \"%s\" for writing\n", progname,
	      pathbuffer);
      exit(1);
   }
   fputs(m.text, fd);
   fclose(fd);
}
