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

  if (argc < 3) {
    fprintf(stderr,"usage:%s %s [-c category] file [file] ...\n",argv[0],TTYPrompt);
    exit(2);
  }
  if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP))) {
    perror("pi_socket");
    exit(1);
  }
    
  addr.sa_family = PI_AF_SLP;
  addr.port = 3;
  strcpy(addr.device,argv[1]);
  
  ret = pi_bind(sd, &addr, sizeof(addr));
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

  category = 0;
  
  for (i=2; i<argc; i++) {
  
    if (strcmp(argv[i],"-c")==0) {
      for (l=0; l<16; l++)
        if (strcasecmp(mai.CategoryName[l], argv[i+1]) == 0) {
          category = l;
          break;
        }
      if (l==16)
        category = atoi(argv[i+1]);
      i++;
      continue;
    }

    f = fopen(argv[i], "r");
    if (f == NULL) {
      perror("fopen");
      exit(1);
    }

    fseek(f, 0, SEEK_END);
    memo_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    l = strlen(argv[i]);

    memo_buf = (char*)malloc(memo_size + l + 2);
    if (memo_buf == NULL) {
      perror("malloc()");
      exit(1);
    }

    strcpy(memo_buf, argv[i]);
    memo_buf[l] = '\n';

    fread(memo_buf + l + 1, memo_size, 1, f);
    memo_buf[l + 1 + memo_size] = '\0';

    /* dlp_exec(sd, 0x26, 0x20, &db, 1, NULL, 0); */
    dlp_WriteRecord(sd, (unsigned char)db, 0, 0, category,
		    (unsigned char *)memo_buf, -1, 0);
    free(memo_buf);
  }

  /* Close the database */
  dlp_CloseDB(sd, db);

  /* Tell the user who it is, with a different PC id. */
  U.lastSyncPC = 0x00010000;
  U.succSyncDate = time(NULL);
  U.lastSyncDate = U.succSyncDate;
  dlp_WriteUserInfo(sd,&U);

  dlp_AddSyncLogEntry(sd, "Wrote memo to Pilot.\n");
  
  /* All of the following code is now unnecessary, but harmless */
  
  dlp_EndOfSync(sd,0);
  pi_close(sd);
  exit(0);
}
