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
#include "dlp.h"
#include "pdb.h"
#include "pi-socket.h"

static unsigned char User[] = { 0x10, 0};

main(int argc, char *argv[])
{
  struct pi_sockaddr addr;
  int sd;
  int i;
  char *buf;
  int ret;
  struct DBInfo info;

  unsigned char userid[64];

  if (argc < 3) {
    fprintf(stderr,"usage:%s /dev/cua? db.pdb [db.pdb] ...\n",argv[0]);
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

  sd = pi_accept(sd,0,0);
  if(sd == -1) {
    perror("pi_accept");
    exit(1);
  }

  puts("Connected");

  buf = (char *)malloc(4096);  /* some huge (in Pilot terms) working space */

  /* Tell user (via Pilot) that we are starting things up */
  dlp_OpenConduit(sd);

  for (i=2; i<argc; i++) LoadPDB(sd,argv[i], 0);
  
  dlp_EndOfSync(sd, 0);

  pi_close(sd);
  exit(0);
}
