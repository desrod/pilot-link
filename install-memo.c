/* install a memo to the pilot */

#include <stdio.h>
#include <stdlib.h>
#include "pi-socket.h"
#include "dlp.h"

main(int argc, char *argv[])
{
  struct pi_sockaddr addr;
  int db;
  int sd;
  int count;
  int l;
  time_t t;
  int memo_size;
  char *memo_buf;
  FILE *f;

  if (argc != 3) {
#ifdef linux  
    fprintf(stderr,"usage:%s /dev/cua?? file\n",argv[0]);
#else
    fprintf(stderr,"usage:%s /dev/tty?? file\n",argv[0]);
#endif
    exit(2);
  }

  f = fopen(argv[2], "r");
  if (f == NULL) {
    perror("fopen");
    exit(1);
  }

  fseek(f, 0, SEEK_END);
  memo_size = ftell(f);
  fseek(f, 0, SEEK_SET);

  l = strlen(argv[2]);

  memo_buf = malloc(memo_size + l + 2);
  if (memo_buf == NULL) {
    perror("malloc()");
    exit(1);
  }

  strcpy(memo_buf, argv[2]);
  memo_buf[l] = '\n';

  fread(memo_buf + l + 1, memo_size, 1, f);
  memo_buf[l + 1 + memo_size] = '\0';

  if (!(sd = pi_socket(AF_SLP, SOCK_STREAM, PF_PADP))) {
    perror("pi_socket");
    exit(1);
  }
    
  addr.sa_family = AF_SLP;
  addr.port = 3;
  strcpy(addr.device,argv[1]);
  
  pi_connect(sd, &addr, sizeof(addr));
  
  /* Tell user (via Pilot) that we are starting things up */
  dlp_OpenConduit(sd);
  
  /* Open the Memo Pad's database, store access handle in db */
  if(dlp_OpenDB(sd, 0, 0x80|0x40, "MemoDB", &db)>=0) { /* less then zero means error */
    /* dlp_exec(sd, 0x26, 0x20, &db, 1, NULL, 0); */
    dlp_WriteRec(sd, (unsigned char)db, 0x80, 0x00000000, 0x0000, memo_buf);
  
    /* Close the database */
    dlp_CloseDB(sd, db);
    dlp_AddSyncLogEntry(sd, "Wrote memo to Pilot.\n");
  } else {
    puts("Unable to open MemoDB");
    dlp_AddSyncLogEntry(sd, "Unable to open MemoDB.\n");
  }
  
  /* All of the following code is now unnecessary, but harmless */
  
  free(memo_buf);
  dlp_EndOfSync(sd,0);
  pi_close(sd);
}
