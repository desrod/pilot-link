/* todos.c:  Translate Pilot ToDo database into generic format
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
#include "pi-todo.h"
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
  struct ToDoAppInfo tai;

  if (argc < 2) {
    fprintf(stderr,"usage:%s %s\n",argv[0],TTYPrompt);
    exit(2);
  }
  if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP))) {
    perror("pi_socket");
    exit(1);
  }
    
  addr.pi_family = PI_AF_SLP;
  addr.pi_port = 3;
  strcpy(addr.pi_device,argv[1]);
  
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
  
  /* Open the ToDo database, store access handle in db */
  if(dlp_OpenDB(sd, 0, 0x80|0x40, "ToDoDB", &db) < 0) {
    puts("Unable to open ToDoDB");
    dlp_AddSyncLogEntry(sd, "Unable to open ToDoDB.\n");
    exit(1);
  }
  
  dlp_ReadAppBlock(sd, db, 0, buffer, 0xffff);
  unpack_ToDoAppInfo(&tai, buffer, 0);
  
  for (i=0;1;i++) {
  	struct ToDo t;
  	int attr, category;
  	                           
  	int len = dlp_ReadRecordByIndex(sd, db, i, buffer, 0, 0, &attr, &category);
  	if(len<0)
  		break;
  		
  	/* Skip deleted records */
  	if((attr & dlpRecAttrDeleted) || (attr & dlpRecAttrArchived))
  		continue;
  		
	unpack_ToDo(&t, buffer, len);
	
	printf("Category: %s\n", tai.CategoryName[category]);
	printf("Priority: %d\n", t.priority);
	printf("Completed: %s\n", t.complete ? "Yes" : "No");
	if(t.indefinite)
	  puts("Due: No Date");
	else
	  printf("Due: %s", asctime(&t.due));
	if(t.description)
	  printf("Description: %s\n", t.description);
	if(t.note)
	  printf("Note: %s\n", t.note );
	printf("\n");

	free_ToDo(&t);
  }

  /* Close the database */
  dlp_CloseDB(sd, db);

  dlp_AddSyncLogEntry(sd, "Read todos from Pilot.\n");

  pi_close(sd);  
  exit(0);
}

