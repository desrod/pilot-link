/* install-user.c:  User name
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
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
  FILE *f;

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
  
  pi_connect(sd, &addr, sizeof(addr));
  
  /* Tell user (via Pilot) that we are starting things up */
  dlp_OpenConduit(sd);

  dlp_ReadUser(sd, &U);

  if (argc == 2) {
    printf ("Pilot user %s\n",U.username);
  }
  else {
    strcpy(U.username, argv[2]);
    dlp_WriteUser(sd, &U, time( (time_t *)0));
  }
  /* All of the following code is now unnecessary, but harmless */
  
  dlp_EndOfSync(sd,0);
  pi_close(sd);
}
