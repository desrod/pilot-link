/* hinotes.c:  Translate Pilot Hi-Note Text Memos into e-mail format
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

char * progname;

/*
 * function prototypes
 */

void usage(void);
int main(int argc, char *argv[]);
void write_memo_mbox(struct HiNoteNote m, struct HiNoteAppInfo mai, int category);
void write_memo_in_directory(char * dirname, struct HiNoteNote m, struct HiNoteAppInfo mai, int category);

/* Not yet actually used:
 */
void usage(void)
{
  fprintf(stderr, "Usage: %s [options]\n", progname);
  fprintf(stderr, "\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -q          = do not prompt for HotSync button press\n");
  fprintf(stderr, "  -p port     = use device file <port> to communicate with Pilot\n");
  fprintf(stderr, "  -d dir      = save memos in <dir> instead of writing to stdout\n");
  fprintf(stderr, "  -h|-?       = print this help\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "By default, the contents of your Pilot's memo database will be written\n");
  fprintf(stderr, "to standard output as a standard Unix mailbox (mbox-format) file, with\n");
  fprintf(stderr, "each memo as a separate message.  The subject of each message will be the\n");
  fprintf(stderr, "category.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "If `-d' is specified, than instead of being written to standard output,\n");
  fprintf(stderr, "will be saved in subdirectories of <dir>.  Each subdirectory will be the\n");
  fprintf(stderr, "name of a category on the Pilot, and will contain the memos in that\n");
  fprintf(stderr, "category.  Each memo's filename will be the first line (up to the first\n");
  fprintf(stderr, "40 characters) of the memo.  Control characters, slashes, and equal\n");
  fprintf(stderr, "signs that would otherwise appear in filenames are converted after the\n");
  fprintf(stderr, "fashion of MIME's quoted-printable encoding.  Note that if you have two\n");
  fprintf(stderr, "memos in the same category whose first lines are identical, one of them\n");
  fprintf(stderr, "will be overwritten.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "The serial port to connect to may be specified by the PILOTPORT environment\n");
  fprintf(stderr, "variable instead of by `-p' on the command line. If not specified anywhere,\n");
  fprintf(stderr, "it will default to /dev/pilot.\n");
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
  extern char* optarg;
  extern int optind;
  
  progname = argv[0];
  
  if (getenv("PILOTPORT")) {
    strcpy(addr.pi_device,getenv("PILOTPORT"));
  } else {
    strcpy(addr.pi_device,PILOTPORT);
  }
  
  while (((c = getopt(argc, argv, "qp:d:h?")) != -1)) {
    switch (c) {
    case 'q':
      quiet = 1;
      break;
    case 'p':
      /* optarg is name of port to use instead of $PILOTPORT or /dev/pilot */
      strcpy(addr.pi_device,optarg);
      break;
    case 'd':
      /* optarg is name of directory to create and store memos in */
      strcpy(dirname,optarg);
      mode = MEMO_DIRECTORY;
      break;
    case 'h': case '?':
      usage();
    }
  }
  
  if (!quiet) {
    fprintf(stderr,
      "Please insert Pilot in cradle on %s and press HotSync button.\n",
      addr.pi_device);
  }
  
  if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP))) {
    perror("pi_socket");
    exit(1);
  }
    
  addr.pi_family = PI_AF_SLP;
  
  ret = pi_bind(sd, (struct sockaddr*)&addr, sizeof(addr));
  if(ret == -1) {
    perror("pi_bind");
    exit(1);
  }

  ret = pi_listen(sd,1);
  if(ret == -1) {
    perror("pi_listen");
    exit(1);
  }

  sd = pi_accept(sd, 0, 0);
  if(sd == -1) {
    perror("pi_accept");
    exit(1);
  }

  /* Ask the pilot who it is. */
  dlp_ReadUserInfo(sd,&U);
  
  /* Tell user (via Pilot) that we are starting things up */
  dlp_OpenConduit(sd);
  
  /* Open the Memo Pad's database, store access handle in db */
  if(dlp_OpenDB(sd, 0, 0x80|0x40, "Hi-NoteDB", &db) < 0) {
    puts("Unable to open Hi-NoteDB");
    dlp_AddSyncLogEntry(sd, "Unable to open Hi-NoteDB.\n");
    exit(1);
  }
  
  dlp_ReadAppBlock(sd, db, 0, (unsigned char *)appblock, 0xffff);
  unpack_HiNoteAppInfo(&mai, (unsigned char *)appblock, 0xffff);

  for (i=0;;i++) {
    struct HiNoteNote m;
    int attr;
    int category;
                               
    int len = dlp_ReadRecordByIndex(sd, db, i, buffer, 0, 0, &attr, &category);
    if(len<0)
    	break;
    	
    /* Skip deleted records */
    if((attr & dlpRecAttrDeleted) || (attr & dlpRecAttrArchived))
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

  dlp_AddSyncLogEntry(sd, "Read Hi-Notes from Pilot.\n");

  pi_close(sd);  
  
  return 0;
}

void write_memo_mbox(struct HiNoteNote m, struct HiNoteAppInfo mai, int category)
{
  int j;

  printf("From your.pilot Tue Oct  1 07:56:25 1996\n");
  printf("Received: Pilot@p by hi-note Tue Oct  1 07:56:25 1996\n");
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
  for(j=0;j<40;j++) {
    if((!m.text[j]) || (m.text[j] == '\n'))
      break;
    printf("%c",m.text[j]);
  }
  if(j==40)
    printf("...\n");
  else
    printf("\n");
  puts("");
  puts(m.text);
}

void write_memo_in_directory(char * dirname, struct HiNoteNote m, struct HiNoteAppInfo mai, int category)
{
  int j;
  char pathbuffer[MAXDIRNAMELEN+(128*3)] = "";
  char tmp[5]="";
  FILE * fd;
  
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
  for (j=0;j<40;j++) {
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
    /* escape if it's an ISO8859 control character (note: some are printable on the Pilot) */
    if ((m.text[j]|0x7f) < ' ') {
      tmp[0]='\0';
      sprintf(tmp, "=%2X", (unsigned char)m.text[j]);
    } else {
      tmp[0]=m.text[j];
      tmp[1]='\0';
    }
    strcat(pathbuffer, tmp);
  }
  
  fprintf(stderr, "Opening file %s\n", pathbuffer);
  if (!(fd = fopen(pathbuffer, "w"))) {
    fprintf(stderr, "%s: can't open file \"%s\" for writing\n", progname, pathbuffer);
    exit(1);
  }
  fputs(m.text, fd);
  fclose(fd);
}
