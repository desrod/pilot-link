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
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"

int main(int argc, char *argv[])
{
  struct pi_sockaddr addr;
  int sd;
  struct PilotUser U;
  struct SysInfo S;
  struct CardInfo C;
  unsigned long romversion;
  int ret;

  if (argc < 2) {
    fprintf(stderr,"usage:%s %s [User name [User ID]]\n",argv[0],TTYPrompt);
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

  ret = pi_listen(sd, 1);
  if(ret == -1) {
    perror("pi_listen");
    exit(1);
  }

  sd = pi_accept(sd, 0, 0);
  if(sd < 0) {
    perror("pi_accept");
    exit(1);
  }
  
  /* Tell user (via Pilot) that we are starting things up */
  dlp_OpenConduit(sd);

  dlp_ReadUserInfo(sd, &U);
  
  dlp_ReadSysInfo(sd, &S);
  
  C.cardno = -1;
  C.more = 1;
  while(C.more) {
    if(dlp_ReadStorageInfo(sd, C.cardno+1, &C)<0)
      break;
    
    printf(" Card #%d has %lu bytes of ROM, and %lu bytes of RAM (%lu of that is free)\n",
       C.cardno, C.ROMsize, C.RAMsize, C.RAMfree);
    printf(" It is called '%s', and was made by '%s'.\n", C.name, C.manuf);
  }
  

  if (argc == 2) {
    printf ("Pilot user %s\n",U.username);
    printf("UserID %ld \n", U.userID );
  }
  else {
    strcpy(U.username, argv[2]);
    if (argc == 4) { U.userID = atoi(argv[3]); }
    U.lastSyncDate = time( (time_t *)0);
    dlp_WriteUserInfo(sd, &U);
  }
  
  printf( "Through ReadSysInfo: ROM Version: 0x%8.8lX, locale: 0x%8.8lX, name: '%s'\n", 
              S.ROMVersion, S.localizationID, S.name);

  dlp_ReadFeature(sd, makelong("psys"), 1, &romversion);
  
  printf( "ROM Version through ReadFeature: 0x%8.8lX\n", romversion);
  
  pi_close(sd);
  exit(0);
}
