/* simple server test */

#include <stdio.h>
#include "pi-socket.h"

main(int argc, char *argv[])
{
  struct pi_sockaddr addr;
  int sd;

  if (argc != 2) {
    fprintf(stderr,"usage:%s /dev/tty??\n",argv[0]);
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

}
