/* 
 * memos.c:  Translate Palm Memos into e-mail format
 *
 * Copyright (c) 1996, Kenneth Albanowski
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-memo.h"
#include "pi-dlp.h"
#include "pi-file.h"
#include "pi-header.h"

/* constants to determine how to produce memos */
#define MEMO_MBOX_STDOUT 0
#define MEMO_DIRECTORY 1
#define MAXDIRNAMELEN 1024

int verbose = 0;
char *progname;

void Help(void);
int main(int argc, char *argv[]);
void write_memo_mbox(struct Memo m, struct MemoAppInfo mai, int category);
void write_memo_in_directory(char *dirname, struct Memo m,
			     struct MemoAppInfo mai, int category,
			     int verbose);

void Help(void)
{
   PalmHeader(progname);

   fprintf(stderr, "   Usage: %s [options]", progname);
   fprintf(stderr, "\n\n   Options:\n");
   fprintf(stderr, "    -q          = do not prompt for HotSync button press\n");
   fprintf(stderr, "    -v          = with -d, print each filename as it's written\n");
   fprintf(stderr, "    -Q          = do not announce which files are opened\n");
   fprintf(stderr, "    -p port     = use device file <port> to communicate with Palm\n");
   fprintf(stderr, "    -f file     = use <file> as memo database file (rather than hotsyncing)\n");
   fprintf(stderr, "    -d dir      = save memos in <dir> instead of writing to STDOUT\n");
   fprintf(stderr, "    -h|-?       = print this usage summary\n\n");

   fprintf(stderr, "   By default, the contents of your Palm's memo database will be written\n");
   fprintf(stderr, "   to standard output as a standard Unix mailbox (mbox-format) file, with\n");
   fprintf(stderr, "   each memo as a separate message.  The subject of each message will be the\n");
   fprintf(stderr, "   category.\n\n");

   fprintf(stderr, "   If `-d' is specified, than instead of being written to standard output,\n");
   fprintf(stderr, "   will be saved in subdirectories of <dir>.  Each subdirectory will be the\n");
   fprintf(stderr, "   name of a category on the Palm, and will contain the memos in that\n");
   fprintf(stderr, "   category.  Each memo's filename will be the first line (up to the first\n");
   fprintf(stderr, "   40 characters) of the memo.  Control characters, slashes, and equal\n");
   fprintf(stderr, "   signs that would otherwise appear in filenames are converted after the\n");
   fprintf(stderr, "   fashion of MIME's quoted-printable encoding.  Note that if you have two\n");
   fprintf(stderr, "   memos in the same category whose first lines are identical, one of them\n");
   fprintf(stderr, "   will be overwritten.\n\n");

   fprintf(stderr, "   The serial port to connect to may be specified by the PILOTPORT environment\n");
   fprintf(stderr, "   variable instead of by `-p' on the command line. If not specified anywhere,\n");
   fprintf(stderr, "   it will default to /dev/pilot.\n\n");

   fprintf(stderr, "   If -f' is specified, the specified file will be treated as a memo database\n");
   fprintf(stderr, "   from which to read memos, rather than HotSyncing from the Palm.\n\n");

   exit(0);
}

int
main(int argc, char *argv[])
{
   struct pi_sockaddr addr;
   int db;
   int sd = 0;
   int i;
   struct PilotUser U;
   int ret;
   unsigned char buffer[0xffff];
   char appblock[0xffff];
   struct MemoAppInfo mai;

   int quiet = 0;
   int verbose = 0;
   int mode = MEMO_MBOX_STDOUT;
   char dirname[MAXDIRNAMELEN] = "";
   int c;
   extern char *optarg;
   extern int optind;
   char filename[MAXDIRNAMELEN + 1], *ptr;
   struct pi_file *pif = NULL;
   char *device = argv[1];
   progname = argv[0];
 
   if (argc < 2) {
      Help();
   }

   while (((c = getopt(argc, argv, "vqp:d:h?")) != -1)) {

      switch (c) {
      case 'v':
	 verbose = 1;
	 break;
      case 'q':
	 quiet = 1;
	 break;
      case 'Q':
	 verbose = 0;
	 break;
      case 'p':
	 /* optarg is name of port to use instead of $PILOTPORT or /dev/pilot */
	 strcpy(addr.pi_device, optarg);
	 break;
      case 'f':
	 /* optarg is name of file to use instead of hotsyncing */
	 strncpy(filename, optarg, MAXDIRNAMELEN);
	 filename[MAXDIRNAMELEN] = '\0';
	 break;
      case 'd':
	 /* optarg is name of directory to create and store memos in */
	 strcpy(dirname, optarg);
	 mode = MEMO_DIRECTORY;
	 break;
      case 'h':
      case '?':
	 Help();
      }
   }

   PalmHeader(progname);

   addr.pi_family = PI_AF_SLP;
   strcpy(addr.pi_device, argv[1]);

   if (filename[0] == '\0') {

      if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP))) {
	 perror("pi_socket");
	 exit(1);
      }

      ret = pi_bind(sd, (struct sockaddr *) &addr, sizeof(addr));
      if (ret == -1) {
	 fprintf(stderr, "\n   Unable to bind to port %s\n", device);
	 perror("   pi_bind");
	 fprintf(stderr, "\n");
	 exit(1);
      }

      printf("   Port: %s\n\n   Please press the HotSync button now...\n", device);

      ret = pi_listen(sd, 1);
      if (ret == -1) {
	 fprintf(stderr, "\n   Error listening on %s\n", device);
	 perror("   pi_listen");
	 fprintf(stderr, "\n");
	 exit(1);
      }

      sd = pi_accept(sd, 0, 0);
      if (sd == -1) {
	 fprintf(stderr, "\n   Error accepting data on %s\n", device);
	 perror("   pi_accept");
	 fprintf(stderr, "\n");
	 exit(1);
      }

      fprintf(stderr, "Connected...\n");

      /* Ask the pilot who it is. */
      dlp_ReadUserInfo(sd, &U);

      /* Tell user (via Palm) that we are starting things up */
      dlp_OpenConduit(sd);

      /* Open the Memo Pad's database, store access handle in db */
      if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "MemoDB", &db) < 0) {
	 puts("Unable to open MemoDB");
	 dlp_AddSyncLogEntry(sd, "Unable to open MemoDB.\n");
	 exit(1);
      }

      dlp_ReadAppBlock(sd, db, 0, (unsigned char *) appblock, 0xffff);

   }
   else {
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

      memcpy(appblock, ptr, len);
   }

   unpack_MemoAppInfo(&mai, (unsigned char *) appblock, 0xffff);

   for (i = 0;; i++) {
      struct Memo m;
      int attr;
      int category;

      int len;

      if (filename[0] == '\0') {
	 len =
	    dlp_ReadRecordByIndex(sd, db, i, buffer, 0, 0, &attr, &category);
	 if (len < 0)
	    break;
      }
      else {
	 if (pi_file_read_record
	     (pif, i, (void *) &ptr, &len, &attr, &category, 0))
	    break;
	 memcpy(buffer, ptr, len);
      }

      /* Skip deleted records */
      if ((attr & dlpRecAttrDeleted) || (attr & dlpRecAttrArchived))
	 continue;

      unpack_Memo(&m, buffer, len);
      switch (mode) {
      case MEMO_MBOX_STDOUT:
	 write_memo_mbox(m, mai, category);
	 break;
      case MEMO_DIRECTORY:
	 write_memo_in_directory(dirname, m, mai, category, verbose);
	 break;
      }
   }

   if (filename[0] == '\0') {
      /* Close the database */
      dlp_CloseDB(sd, db);

      dlp_AddSyncLogEntry(sd, "Read memos from Palm.\n");

      pi_close(sd);
   }
   else {
      pi_file_close(pif);
   }
   return 0;
}

void
write_memo_mbox(struct Memo m, struct MemoAppInfo mai, int category)
{
   int j;

   printf("From Your.Palm Tue Oct  1 07:56:25 1996\n");
   printf("Received: Palm@p by memo Tue Oct  1 07:56:25 1996\n");
   printf("To: you@y\n");
   printf("Date: Thu, 31 Oct 1996 23:34:38 -0500\n");
   printf("Subject: ");

   /* print category name in brackets in subject field */
   printf("[%s] ", mai.category.name[category]);

   /* print (at least part of) first line as part of subject: */
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

void
write_memo_in_directory(char *dirname, struct Memo m, struct MemoAppInfo mai, int category, int verbose)
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
      }
      else {
	 tmp[0] = m.text[j];
	 tmp[1] = '\0';
      }
      strcat(pathbuffer, tmp);
   }

   if (verbose) {
      fprintf(stdout, "Writing %s\n", pathbuffer);
   }

   if (!(fd = fopen(pathbuffer, "w"))) {
      fprintf(stderr, "%s: can't open file \"%s\" for writing\n", progname, pathbuffer);
      exit(1);
   }
   fputs(m.text, fd);
   fclose(fd);
}
