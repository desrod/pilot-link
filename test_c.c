/* simple server test */

#include <stdio.h>
#include "pi-socket.h"
#include "dlp.h"

main(int argc, char *argv[])
{
  struct pi_sockaddr addr;
  int db;
  int sd;
  int count;
  time_t t;
  struct PilotUser U;

  if (argc != 2) {
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
  puts("Got socket");
    
  addr.sa_family = AF_SLP;
  addr.port = 3;
  strcpy(addr.device,argv[1]);
  
  pi_connect(sd, &addr, sizeof(addr));
  puts("Connected");
  
#if 1
  /* Ask the pilot who it is. */
  dlp_ReadUser(sd,&U);
  printf("Username: %s\nUserid: %08lx %08lx %08lx\n",U.username, U.id1,
         U.id2, U.id3);
#endif

  /* Tell user (via Pilot) that we are starting things up */
  dlp_OpenConduit(sd);
  
  /* Open the Memo Pad's database, store access handle in db */
  if(dlp_OpenDB(sd, 0, 0x80|0x40, "MemoDB", &db)>=0) { /* less then zero means error */
  
    dlp_ReadOpenDBInfo(sd, db, &count);
  
    printf("There are %d entries in the MemoDB database\n", count);

    /* dlp_exec(sd, 0x26, 0x20, &db, 1, NULL, 0); */
    dlp_WriteRec(sd, (unsigned char)db, 0x80, 0x00000000, 0x0000,
                 "This is brian's neato spiffy notepad\nentry.\n");
  
    /* Close the database */
    dlp_CloseDB(sd, db);
  } else {
    puts("Unable to open MemoDB");
  }
  
  dlp_GetSysDateTime(sd, &t);
  
  printf("Your Pilot thinks the time is %s\n", ctime(&t));
  
  /* Set pilot one hour forward */
  /*** dlp_SetSysDateTime(sd, t+3600); ***/
  /* Set pilot one hour backward */
  /*** dlp_SetSysDateTime(sd, t-3600); ***/
  
#if 1
  /* Tell the Pilot who it is. */
  dlp_WriteUser(sd,&U, time(NULL));
#endif

  puts("Adding log message");
  dlp_AddSyncLogEntry(sd, "Pilot-link\n was here!");

  puts("Signing off");
  
  /* All of the following code is now unnecessary, but harmless */
  
  dlp_EndOfSync(sd,0);
  puts("Done");
  pi_close(sd);
  
}
