/* test-acceptor.c: Test a unix-to-unix connection
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */
 
/* "Server" and "client" are fuzzy concepts with the Pilot. The Pilot
 * initiates conversation, and the desktop sits passively and listens, so by
 * unix networking standards at least, the desktop is the server and the
 * Pilot is the client. However, once a connection is established the
 * desktop initiates all further actions while the Pilot sits, passively
 * allowing the desktop to rummage through its databases. Thus the desktop
 * could be called a client.
 *
 * All normal Pilot hot-sync programs should be "acceptors" that sit around
 * waiting for a pilot to connect, using the Unix bind/listen/accept
 * sequence. The test-connector companion to this file is effectively a
 * "fake Pilot" that uses connect to simulate a Pilot connecting. Note that
 * either side can transmit and receive data, as long as they agree on which
 * one is going to be talking.
 *
 * This pair of programs use a pty to simulate the first stages of a Pilot
 * communcation -- CMP exchanges, and baud rate matching. Note that a pty is
 * used to let the two programs talk together, and that no attempt has been
 * made to properly allocate a pty.
 *
 * Also note that dlp_AbortSync is used, instead of EndOfSync. That is
 * because the normal EndOfSync proceedings involves sending a padp packet,
 * which would not work as neither side is expecting such a packet.
 *
 */ 

#include <stdio.h>
#include <stdlib.h>
#include "pi-socket.h"
#include "dlp.h"

int main(int argc, char *argv[])
{
  struct pi_sockaddr addr;
  int sd;
  char buffer[64];
  int ret;

  if (!(sd = pi_socket(AF_SLP, SOCK_STREAM, PF_PADP))) {
    perror("pi_socket");
    exit(1);
  }
    
  addr.sa_family = AF_SLP;
  addr.port = 3;
  strcpy(addr.device,"/dev/ptyp9"); /* Bogus PTY allocation */
  
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
  
  pi_write(sd, "Sent from server", 17 );
  pi_read(sd, buffer, 64);
  puts(buffer);
  
  dlp_AbortSync(sd);
  exit(0);
}
