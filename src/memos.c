/* memos.c:  Translate Pilot Memos into e-mail format
 *
 * Copyright (c) 1996, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pi-socket.h"
#include "memo.h"
#include "dlp.h"

main(int argc, char *argv[])
{
  struct pi_sockaddr addr;
  int db;
  int sd;
  int count;
  int i;
  int l;
  time_t t;
  int memo_size;
  char *memo_buf;
  FILE *f;
  struct PilotUser U;
  int ret;
  char buffer[0xffff];

  if (argc < 2) {
#ifdef linux  
    fprintf(stderr,"usage:%s /dev/cua??\n",argv[0]);
#else
    fprintf(stderr,"usage:%s /dev/tty??\n",argv[0]);
#endif
    exit(2);
  }
  if (!(sd = pi_socket(AF_SLP, SOCK_STREAM, PF_PADP))) {
    perror("pi_socket");
    exit(1);
  }
    
  addr.sa_family = AF_SLP;
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
  
  /* Open the Datebook's database, store access handle in db */
  if(dlp_OpenDB(sd, 0, 0x80|0x40, "MemoDB", &db) < 0) {
    puts("Unable to open MemoDB");
    dlp_AddSyncLogEntry(sd, "Unable to open MemoDB.\n");
    exit(1);
  }

  for (i=0;1;i++) {
  	struct Memo m;
  	int attr;
  	                           
  	int len = dlp_ReadRecordByIndex(sd, db, i, buffer, 0, 0, &attr, 0);
  	if(len<0)
  		break;
  		
  	/* Skip deleted records */
  	if((attr & dlpRecAttrDeleted) || (attr & dlpRecAttrArchived))
  		continue;
  		
	unpack_Memo(&m, buffer, len);
	
	printf("From your.pilot Tue Oct  1 07:56:25 1996\nReceived: Pilot@p by memo Tue Oct  1 07:56:25 1996\nTo: you@y\nDate: Thu, 31 Oct 1996 23:34:38 -0500\nSubject: None\n\n");
	puts(m.text);
  }

  /* Close the database */
  dlp_CloseDB(sd, db);

  dlp_AddSyncLogEntry(sd, "Read memos from Pilot.\n");

  pi_close(sd);  
}

