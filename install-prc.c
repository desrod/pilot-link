/* install-prc.c:  Pilot Application installer
 *
 * (c) 1996, D. Jeff Dionne.
 * Much of this code adapted from Brian J. Swetland <swetland@uiuc.edu>
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include "pi-socket.h"

static unsigned char Ident[] = { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static unsigned char User[] = { 0x10, 0};

static unsigned char SyncClose[] = { 0x29, 0 };
static unsigned char EOS[] = { 0x2f, 0x01, 0x20, 0x02, 0, 0};

main(int argc, char *argv[])
{
  struct pi_sockaddr addr;
  int sd;
  int i;
  char *buf;

  unsigned char userid[64];

  if (argc < 3) {
    fprintf(stderr,"usage:%s /dev/tty?? app.prc [app.prc] ...\n",argv[0]);
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

  pi_listen(sd,0);

  buf = (char *)malloc(4096);  /* some huge (in Pilot terms) working space */

  pi_write(sd,Ident,sizeof(Ident));
  pi_write(sd,User,sizeof(User));
  pi_read(sd,userid,64);

  for (i=2; i<argc; i++) LoadPRC(sd,argv[i]);

  pi_write(sd, SyncClose, sizeof(SyncClose));
  pi_read(sd, buf, 64);
  
  /* I think this is End Of Session */

  pi_write(sd,EOS,sizeof(EOS));
  pi_read(sd, buf, 64);

  /* wait a second, for things to close */
  sleep(1);
}
