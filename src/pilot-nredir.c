/* pi-nredir.c: Redirect a connection over the network
 *
 * Copyright (C) 1997, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */
 

#include <stdio.h>
#include <stdlib.h>
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"

int main(int argc, char *argv[])
{
  struct sockaddr_in addr2;
  struct pi_sockaddr addr;
  int sd, sd2;
  struct NetSyncInfo N;
  char buffer[0xffff];
  int len;
  int ret;

  if (argc < 2) {
    fprintf(stderr,"usage:%s %s\n",argv[0],TTYPrompt);
    exit(2);
  }

  if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP))) {
    perror("pi_socket");
    exit(1);
  }
    
  addr.pi_family = PI_AF_SLP;
  strcpy(addr.pi_device,argv[1]);
  
  ret = pi_bind(sd, (struct sockaddr*)&addr, sizeof(addr));
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
  
  if (dlp_ReadNetSyncInfo(sd, &N) < 0) {
    fprintf(stderr, "Unable to read network information, cancelling sync.\n");
    exit(1);
  }
    
  if (!N.lanSync) {
    fprintf(stderr, "LAN Sync not enabled on your PalmPilot, cancelling sync.\n");
    exit(1);
  }

  putenv("PILOTLOGFILE=PiDebugNet.log");
  
  sd2 = pi_socket(AF_INET, PI_SOCK_STREAM, 0);
  if (sd2 <0 ) {
    perror("Unable to get socket 2");
    exit(1);
  }
  printf("Got socket 2\n");
  
  memset(&addr2, 0, sizeof(addr2));
  addr2.sin_family = AF_INET;
  addr2.sin_port = htons(14238);

  if ((addr2.sin_addr.s_addr = inet_addr(N.hostAddress))==-1) {
    fprintf(stderr, "Unable to parse PC address '%s'\n", N.hostAddress);
    exit(1);
  }
  
  ret = pi_connect(sd2, (struct sockaddr*)&addr2, sizeof(addr2));

  if (ret<0) {
    perror("Unable to connect to PC");
    exit(1);
  }
  
  while ((len = pi_read(sd2, buffer, 0xffff))>0) {
    pi_write(sd, buffer, len);
    len = pi_read(sd, buffer, 0xffff);
    if (len < 0)
      break;
    pi_write(sd2, buffer, len);
  }
  
  dlp_AbortSync(sd);
  close(sd2);

  return 0;
}
