/* test-connector.c:  Test a unix-to-unix connection
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */
 
/* See test-acceptor.c for more information */

#include <stdio.h>
#include <stdlib.h>
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"

int main(int argc, char *argv[])
{
  struct pi_sockaddr addr;
  int sd;
  char buffer[64];
  int ret;

  if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP))) {
    perror("pi_socket");
    exit(1);
  }
    
  addr.sa_family = PI_AF_SLP;
  addr.port = 3;
  strcpy(addr.device,"/dev/ttyp9");
  
  ret = pi_connect(sd, &addr, sizeof(addr));
  if(ret == -1) {
    perror("pi_connect");
    exit(1);
  }
  
  pi_read(sd, buffer, 64);
  puts(buffer);
  pi_write(sd, "Sent from client", 17);

  dlp_AbortSync(sd);
  exit(0);
}
