/* install-user.c:  User name
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */
 
/* Note: if you use this program to change the user name on the Pilot, I
 * _highly_ reccomend that you perform a hard reset before HotSyncing with a
 * Windows machine. This is because the user-id information has only been
 * partially altered, and it is not worth trying to predict what the Desktop
 * will do. - KJA
 */
 

#include <stdio.h>
#include <stdlib.h>
#include "pi-socket.h"
#include "dlp.h"

main(int argc, char *argv[])
{
  struct pi_sockaddr addr;
  int sd;
  struct PilotUser U;
  struct SysInfo S;

  if (argc < 2) {
    fprintf(stderr,"usage:%s /dev/tty?? [User name]\n",argv[0]);
    exit(2);
  }

  if (!(sd = pi_socket(AF_SLP, SOCK_STREAM, PF_PADP))) {
    perror("pi_socket");
    exit(1);
  }
    
  addr.sa_family = AF_SLP;
  addr.port = 3;
  strcpy(addr.device,argv[1]);
  
  /*pi_connect(sd, &addr, sizeof(addr));*/
  pi_bind(sd, &addr, sizeof(addr));
  pi_listen(sd, 1);
  sd = pi_accept(sd, 0, 0);
  
  /* Tell user (via Pilot) that we are starting things up */
  dlp_OpenConduit(sd);

  dlp_ReadUserInfo(sd, &U);
  
  dlp_ReadSysInfo(sd, &S);

  if (argc == 2) {
    printf ("Pilot user %s\n",U.username);
  }
  else {
    strcpy(U.username, argv[2]);
    U.lastSyncDate = time( (time_t *)0);
    dlp_WriteUserInfo(sd, &U);
  }
  
  printf( "ROM Version: 0x%8.8X, locale: 0x%8.8X, name: '%s'\n", 
              S.ROMVersion, S.localizationID, S.name);
  
  pi_close(sd);
  exit(0);
}
