/* install-memo.c:  Pilot memo pad installer
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-memo.h"

int usage(char *progname)
{
    fprintf(stderr, "usage: %s [-qrt] [-c category] %s file [file] ...\n",
	    progname, TTYPrompt);
    fprintf(stderr, "       -q = do not prompt for HotSync button press\n");
    fprintf(stderr, "       -r = replace all memos in specified category\n");
    fprintf(stderr, "       -t = use filename as memo title\n");
    exit(2);
}

int main(int argc, char *argv[])
{
  struct pi_sockaddr addr;
  int db;
  int sd;
  int i;
  int l;
  int memo_size;
  char *memo_buf;
  FILE *f;
  struct PilotUser U;
  int ret;
  char buf[0xffff];
  int category;
  struct MemoAppInfo mai;
  char *progname, *category_name, ch;
  int   preamble, quiet, replace_category, add_title;

  progname = argv[0];
  category_name = NULL;
  quiet = replace_category = add_title = 0;

  while ((ch = getopt(argc, argv, "c:qrt")) != -1)
    switch (ch) {
    case 'c':
      category_name = optarg;
      break;
    case 'q':
      quiet++;
      break;
    case 'r':
      replace_category++;
      break;
    case 't':
      add_title++;
      break;
    case '?':
    default:
      usage(progname);
    }

  argc -= optind;
  argv += optind;

  if (argc < 2) {
    fprintf(stderr, "%s: insufficient number of arguments\n", progname);
    usage(progname);
  }

  if (replace_category && !category_name) {
    fprintf(stderr, "%s: memo category required when specifying replace\n",
	    progname);
    usage(progname);
  }

  if (!quiet)
    printf("Insert PalmPilot in cradel and press hotsync button...\n");

  if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP))) {
    perror("pi_socket");
    exit(1);
  }
    
  addr.pi_family = PI_AF_SLP;
  strcpy(addr.pi_device,argv[0]);
  
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
  if(dlp_OpenDB(sd, 0, 0x80|0x40, "MemoDB", &db) < 0) {
    puts("Unable to open MemoDB");
    dlp_AddSyncLogEntry(sd, "Unable to open MemoDB.\n");
    exit(1);
  }
  
  l = dlp_ReadAppBlock(sd, db, 0, (unsigned char *)buf, 0xffff);
  unpack_MemoAppInfo(&mai, (unsigned char *)buf, l);

  if (category_name) {
    category = -1;		/* invalid category */
    for (i = 0; i < 16; i++)
      if (!strcasecmp(mai.category.name[i], category_name)) {
	category = i;
	break;
      }
    if (category < 0) {
      fprintf(stderr, "%s: category %s not found on PalmPilot\n",
	      progname, category_name);
      exit(2);
    }

    if (replace_category)
      dlp_DeleteCategory(sd, db, category);

  } else
    category = 0;		/* unfiled */

  for (i=1; i<argc; i++) {
  
    f = fopen(argv[i], "r");
    if (f == NULL) {
      fprintf(stderr, "%s: cannot open %s (%s), skipping...\n",
	      progname, argv[i], strerror(errno));
      continue;
    }

    fseek(f, 0, SEEK_END);
    memo_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    preamble = add_title ? strlen(argv[i]) + 1 : 0;

    memo_buf = (char*)malloc(memo_size + preamble + 1);
    if (memo_buf == NULL) {
      perror("malloc()");
      exit(1);
    }

    if (preamble)
      sprintf(memo_buf, "%s\n", argv[i]);

    fread(memo_buf + preamble, memo_size, 1, f);

    memo_buf[memo_size + preamble] = '\0';

    dlp_WriteRecord(sd, (unsigned char)db, 0, 0, category,
		    (unsigned char *)memo_buf, -1, 0);
    free(memo_buf);
  }

  /* Close the database */
  dlp_CloseDB(sd, db);

  /* Tell the user who it is, with a different PC id. */
  U.lastSyncPC = 0x00010000;
  U.successfulSyncDate = time(NULL);
  U.lastSyncDate = U.successfulSyncDate;
  dlp_WriteUserInfo(sd,&U);

  dlp_AddSyncLogEntry(sd, "Wrote memo to Pilot.\n");
  
  /* All of the following code is now unnecessary, but harmless */
  
  dlp_EndOfSync(sd,0);
  pi_close(sd);
  
  return 0;
}
