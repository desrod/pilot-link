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

char * device = "/dev/pilot";

/* Not yet actually used:
 */
void usage(void)
{
      printf("Usage: %s [%s]\n\n","memos",TTYPrompt);
      printf("\n");
      printf("The contents of your Pilot's memo database will be written to");
      printf(" standard\n");
      printf("output as a standard Unix mailbox (mbox-format) file, with each");
      printf(" memo\n");
      printf("as a separate message.  The subject of each message will be the");
      printf(" category.\n");
      printf("\n");
      printf("The serial port to connect to may be specified by the");
      printf(" PILOTPORT\n");
      printf("environment variable instead of the command line. If not");
      printf(" specified\n");
      printf("anywhere, it will default to /dev/pilot.\n");
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
  struct MemoAppInfo mai;
  char *tmp;

  if (argc < 2) {
    tmp = getenv("PILOTPORT");
    if (tmp != NULL) {
      device = tmp;
    }
/*    fprintf(stderr,"usage:%s %s\n",argv[0],TTYPrompt);
    exit(2);
*/
  }
  if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP))) {
    perror("pi_socket");
    exit(1);
  }
    
  addr.pi_family = PI_AF_SLP;
/*  strcpy(addr.pi_device,argv[1]);
*/
  strcpy(addr.pi_device,device);
  
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
  
  /* Open the Datebook's database, store access handle in db */
  if(dlp_OpenDB(sd, 0, 0x80|0x40, "MemoDB", &db) < 0) {
    puts("Unable to open MemoDB");
    dlp_AddSyncLogEntry(sd, "Unable to open MemoDB.\n");
    exit(1);
  }
  
  dlp_ReadAppBlock(sd, db, 0, (unsigned char *)appblock, 0xffff);
  unpack_MemoAppInfo(&mai, (unsigned char *)appblock, 0xffff);

  for (i=0;;i++) {
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
	
	printf("From your.pilot Tue Oct  1 07:56:25 1996\n");
	printf("Received: Pilot@p by memo Tue Oct  1 07:56:25 1996\n");
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

  /* Close the database */
  dlp_CloseDB(sd, db);

  dlp_AddSyncLogEntry(sd, "Read memos from Pilot.\n");

  pi_close(sd);  
  
  return 0;
}


