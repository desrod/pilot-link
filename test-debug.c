/* test-debug.c:  Demonstrate debug packets
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include "pi-socket.h"
#include "dlp.h"
#include "syspkt.h"

main(int argc, char *argv[])
{
  struct pi_sockaddr laddr,raddr;
  int sd;
  char buf[64];
  struct PilotUser U;
  struct SysInfo S;
  FILE *f;

  if (argc < 2) {
    fprintf(stderr,"usage:%s /dev/tty?? [User name]\n",argv[0]);
    exit(2);
  }

  if (!(sd = pi_socket(AF_SLP, SOCK_DGRAM, PF_SYS))) {
    perror("pi_socket");
    exit(1);
  }
    
  laddr.sa_family = AF_SLP;
  laddr.port = PilotSocketRemoteUI;
  strcpy(laddr.device,argv[1]);
  
  /* First, we bind the socket to the device. Since we are using a
     PF_SYS/SOCK_DGRAM socket, this only locks down the device name, not the
     port */
     
  pi_bind(sd, &laddr, sizeof(laddr));
  
  /* Now we can read and write packets: to get the Pilot to send a packet,
     write a ".2" shortcut, which starts the debugging mode. (Make sure to
     reset your Pilot after finishing this example!) */
  
  pi_recvfrom(sd, buf, 64, 0, &raddr, 0);
#ifdef DEBUG
  dumpline(buf, 16, 0);
#endif

  /* recvfrom returns the port that the packet was sent from.
     getsockname will return the port that the packet was sent _to_. */
  
  pi_getsockname(sd, &laddr, 0);
  
  printf("Read from port %d, destined for port %d\n", raddr.port, laddr.port);
  
  
  /* The Pilot is a little grumpy after starting debug mode, so we let it calm down */
  
  sleep(1);
  
  /* And now we can send packets */
  
#if 0  
  buf[0] = 0x0d; //RemoteEvtCommand
  buf[1] = 0; //gapfil
  buf[2] = 1; //pendown
  buf[3] = 0; //gapfil
  buf[4] = 0;
  buf[5] = 5; //x
  buf[6] = 0;
  buf[7] = 170; // y
  buf[8] = 0;//KeyPress
  buf[9] = 0;//gapfil
  buf[10] = buf[11] = buf[12] = buf[13] = buf[14] = buf[15] = 0;
  
  /* Sendto takes the target port as its argument, and uses the
     current (bound) local port as the source */
  
  raddr.port = PilotSocketRemoteUI;
  pi_sendto(sd, buf, 16, 0, &raddr, 0);
#endif

  /* (These two packets happen to place the pen over the Apps button, and
     then release it.) */

  sys_RemoteEvent(sd, 1, 5,170, 0, 0,0,0); /* Set the pen down */
  sys_RemoteEvent(sd, 0, 5,170, 0, 0,0,0); /* And then lift it up */

}
