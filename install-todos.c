/* install-todolist.c:  Pilot todo list installer
 *
 * Copyright 1996, Robert A. Kaplan
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-todo.h"

int main(int argc, char *argv[])
{
  struct pi_sockaddr addr;
  int db;
  int sd;
  int i;
  int ToDo_size;
  unsigned char ToDo_buf[0xffff];
  struct ToDo todo;
  FILE *f;
  struct PilotUser U;
  int ret;
  int filelen;
  char note_text[] = "";
  char *cPtr;
  char *begPtr;
  char *file_text;
  int cLen;
  int count;

#ifndef NOTHAPPENING
  if (argc < 3) {
    fprintf(stderr,"usage:%s %s file [file] ...\n",argv[0],TTYPrompt);
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
  
  /* Open the ToDo Pad's database, store access handle in db */
  if(dlp_OpenDB(sd, 0, 0x80|0x40, "ToDoDB", &db) < 0) {
    puts("Unable to open ToDoDB");
    dlp_AddSyncLogEntry(sd, "Unable to open ToDoDB.\n");
    exit(1);
  }
#endif
  for (i=2; i<argc; i++) {

    f = fopen(argv[i], "r");
    if (f == NULL) {
      perror("fopen");
      exit(1);
    }

    fseek(f, 0, SEEK_END);
    filelen = ftell(f);
    fseek(f, 0, SEEK_SET);

    file_text = (char*)malloc(filelen+1);
    if (file_text == NULL) {
      perror("malloc()");
      exit(1);
    }

    fread(file_text, filelen, 1, f);

    /*printf("file: %s\n",file_text);*/

    cPtr = file_text;
    begPtr = cPtr;
    cLen = 0;
    count = 0;
    while(count < filelen) {
      count++;
       /*printf("c:%c.\n",*cPtr); */
      if (*cPtr == '\n') {
	todo.description = begPtr;
	/* replace cr with terminator */
	*cPtr = '\0';

	todo.priority = 4;
	todo.complete = 1;
	todo.indefinite = 1;
	/*now = time(0);
	todo.due = *localtime(&now);*/
	todo.note = note_text;
	pack_ToDo(&todo, ToDo_buf,&ToDo_size);
printf("desc: %s\n",todo.description); 
/* printf("todobuf: %s\n",ToDo_buf);       */
	dlp_WriteRecord(sd, db, 0, 0, 0, ToDo_buf, ToDo_size, 0);
	cPtr++;
	begPtr = cPtr;
	cLen = 0;
      } else {
	cLen++;
	cPtr++;
      }
    }
  }

  /* Close the database */
  dlp_CloseDB(sd, db);

  /* Tell the user who it is, with a different PC id. */
  U.lastSyncPC = 0x00010000;
  U.succSyncDate = time(NULL);
  U.lastSyncDate = U.succSyncDate;
  dlp_WriteUserInfo(sd,&U);

  dlp_AddSyncLogEntry(sd, "Wrote ToDo to Pilot.\n");
  
  /* All of the following code is now unnecessary, but harmless */
  
  dlp_EndOfSync(sd,0);
  pi_close(sd);
  exit(0);
}
