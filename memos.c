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
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-memo.h"
#include "pi-dlp.h"

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
  struct MemoAppInfo mai;

  if (argc < 2) {
    fprintf(stderr,"usage:%s %s\n",argv[0],TTYPrompt);
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
  
  /* Open the Datebook's database, store access handle in db */
  if(dlp_OpenDB(sd, 0, 0x80|0x40, "MemoDB", &db) < 0) {
    puts("Unable to open MemoDB");
    dlp_AddSyncLogEntry(sd, "Unable to open MemoDB.\n");
    exit(1);
  }
  
  dlp_ReadAppBlock(sd, db, 0, appblock, 0xffff);
  unpack_MemoAppInfo(&mai, appblock, 0);

  for (i=0;1;i++) {
  	struct Memo m;
  	int attr;
  	int category;
  	int j;
  	                           
  	int len = dlp_ReadRecordByIndex(sd, db, i, buffer, 0, 0, &attr, &category);
  	if(len<0)
  		break;
  		
  	/* Skip deleted records */
  	if((attr & dlpRecAttrDeleted) || (attr & dlpRecAttrArchived))
  		continue;
  		
	unpack_Memo(&m, buffer, len);
	
	printf("From your.pilot Tue Oct  1 07:56:25 1996\nReceived: Pilot@p by memo Tue Oct  1 07:56:25 1996\nTo: you@y\nDate: Thu, 31 Oct 1996 23:34:38 -0500\n");
	printf("Subject: ");
	printf("[%s] ", mai.CategoryName[category]);
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

  /* Close the database */
  dlp_CloseDB(sd, db);

  dlp_AddSyncLogEntry(sd, "Read memos from Pilot.\n");

  pi_close(sd);  
  
  exit(0);
}

