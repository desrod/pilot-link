/* install-prc.c:  Pilot Application installer
 *
 * (c) 1996, D. Jeff Dionne.
 * Much of this code adapted from Brian J. Swetland <swetland@uiuc.edu>
 * Changed to use DLP layer, Kenneth Albanowski, 1996
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include "pi-socket.h"

static unsigned char User[] = { 0x10, 0};

main(int argc, char *argv[])
{
  struct pi_sockaddr addr;
  int sd;
  int i;
  char *buf;

  unsigned char userid[64];

  if (argc < 3) {
    fprintf(stderr,"usage:%s /dev/cua? app.prc [app.prc] ...\n",argv[0]);
    exit(2);
  }

  if (!(sd = pi_socket(AF_SLP, SOCK_STREAM, PF_PADP))) {
    perror("pi_socket");
    exit(1);
  }

  addr.sa_family = AF_SLP;
  addr.port = 3;
  strcpy(addr.device,argv[1]);

  pi_bind(sd, &addr, sizeof(addr));
  pi_listen(sd,1);
  sd = pi_accept(sd,0,0);
  
  puts("Connected");

  buf = (char *)malloc(4096);  /* some huge (in Pilot terms) working space */

  /*pi_write(sd,User,sizeof(User));
  pi_read(sd,userid,64);*/

  for (i=2; i<argc; i++) LoadPRC(sd,argv[i]);

  dlp_ResetSystem(sd,0);
  
  dlp_EndOfSync(sd, 0);

  pi_close(sd);
  exit(0);
}
