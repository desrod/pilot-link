/* addresses.c:  Translate Pilot address book into a generic format
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
#include "address.h"
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
  struct AddressAppInfo aai;

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
  
  /* Open the Address book's database, store access handle in db */
  if(dlp_OpenDB(sd, 0, 0x80|0x40, "AddressDB", &db) < 0) {
    puts("Unable to open AddressDB");
    dlp_AddSyncLogEntry(sd, "Unable to open AddressDB.\n");
    exit(1);
  }
  
  dlp_ReadAppBlock(sd, db, 0, buffer, 0xffff);
  unpack_AddressAppInfo(&aai, buffer, 0);
  
  for (i=0;1;i++) {
  	struct Address a;
  	int attr, category;
  	int j;
  	                           
  	int len = dlp_ReadRecordByIndex(sd, db, i, buffer, 0, 0, &attr, &category);
  	if(len<0)
  		break;
  		
  	/* Skip deleted records */
  	if((attr & dlpRecAttrDeleted) || (attr & dlpRecAttrArchived))
  		continue;
  		
	unpack_Address(&a, buffer, len);
	
	printf("Category: %s\n", aai.CategoryName[category]);
	for(j=0;j<19;j++) {
	  if(a.entry[j]) {
	    int l = j;
	    if(l == entryPhone1) {
	       l = entryPhone1 + a.phonelabel1;
	       if(a.phonelabel1 >= 5) 
	         l += 16 - 5;
	    }
	    if(l == entryPhone2) {
	       l = entryPhone1 + a.phonelabel2;
	       if(a.phonelabel2 >= 5) 
	         l += 16 - 5;
	    }
	    if(l == entryPhone3) {
	       l = entryPhone1 + a.phonelabel3;
	       if(a.phonelabel3 >= 5) 
	         l += 16 - 5;
	    }
	    if(l == entryPhone4) {
	       l = entryPhone1 + a.phonelabel4;
	       if(a.phonelabel4 >= 5)
	         l += 16 - 5;
	    }
	    if(l == entryPhone5) {
	       l = entryPhone1 + a.phonelabel5;
	       if(a.phonelabel5 >= 5) 
	         l += 16 - 5;
	    }
	    printf("%s: %s\n", aai.labels[l], a.entry[j]);
	  }
	}
	printf("\n");

	free_Address(&a);
  }

  /* Close the database */
  dlp_CloseDB(sd, db);

  dlp_AddSyncLogEntry(sd, "Read addresses from Pilot.\n");

  pi_close(sd);  
}

