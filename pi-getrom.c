/* pi-getrom:  Fetch ROM image from Pilot, without using getrom.prc
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 *
 * Copyright (C) 1997, Kenneth Albanowski
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "pi-source.h"
#include "pi-syspkt.h"
#include "pi-dlp.h"

#include <termios.h>

char * progname;

void Help(void)
{
  fprintf(stderr,"usage: %s %s [pilot.rom]\n",progname,TTYPrompt);
  exit(2);
}

int cancel = 0;
RETSIGTYPE sighandler(int signo)
{
  cancel = 1;
}

struct record * records = 0;

int main(int argc, char *argv[])
{
  char name[256];
  char print[256];
  int i, sd;
  struct RPC_params p;
  struct pi_sockaddr addr;
  extern char* optarg;
  extern int optind;
  int err;
  int ret;
  int file;
  unsigned long ROMstart, ROMlength, ROMversion, offset, left;
  int majorVersion, minorVersion, build, state;

  progname = argv[0];

  if (argc < 2)
    Help();
  
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

  ret = pi_listen(sd,1);
  if(ret == -1) {
    perror("pi_listen");
    exit(1);
  }
  
  printf("Warning: Please back up your Pilot (with pilot-xfer) before using this program!\n\n");

  printf("\
NOTICE: Use of this program may place you in violation of your license\n\
        agreement with US Robotics. Please read page 123 of your Pilot\n\
        handbook (\"Software License Agreement\") before running this program.\
\n\n");

  printf("Please start HotSync (not getrom.prc) on your Pilot.\n");
  printf("Waiting for connection on %s...\n", argv[1]);

  sd = pi_accept(sd, 0, 0);
  if(sd == -1) {
    perror("pi_accept");
    exit(1);
  }

  /* Tell user (via Pilot) that we are starting things up */
  dlp_OpenConduit(sd);
  
  PackRPC(&p, 0xA23E, RPC_IntReply, RPC_Long(0xFFFFFFFF), RPC_End);
  err = dlp_RPC(sd, &p, &ROMstart);
  PackRPC(&p, 0xA23E, RPC_IntReply, RPC_Long(ROMstart), RPC_End);
  err = dlp_RPC(sd, &p, &ROMlength);
  
  dlp_ReadFeature(sd, makelong("psys"), 1, &ROMversion);
  
  if (argc<3)
    strcpy(name, "pilot.rom");
  else
    strcpy(name,argv[2]);
  
  majorVersion = (((ROMversion >> 28) & 0xf) * 10)+ ((ROMversion >> 24) & 0xf);
  minorVersion = (((ROMversion >> 20) & 0xf) * 10)+ ((ROMversion >> 16) & 0xf);
  state = ((ROMversion >> 12) & 0xf);
  build = (((ROMversion >> 8) & 0xf) * 10)+(((ROMversion >> 4) & 0xf) * 10)+ (ROMversion  & 0xf);
  
  sprintf(name+strlen(name), "%d.%d", majorVersion, minorVersion);
  if (state!=3)
    sprintf(name+strlen(name), "%s%d", ((state==0) ? "d" : (state == 1) ? "a" : (state==2) ? "b" : "u"), build);
  
  printf("Generating %s\n", name);
  
  file = open(name,O_RDWR|O_CREAT,0666);

  offset = lseek(file, 0, SEEK_END);
  offset &= ~255;
  lseek(file, offset, SEEK_SET);

  PackRPC(&p, 0xA164, RPC_IntReply, RPC_Byte(1), RPC_End);
  err = dlp_RPC(sd, &p, 0);

  sprintf(print, "Downloading byte %ld", offset);
  PackRPC(&p, 0xA220, RPC_IntReply, RPC_Ptr(print, strlen(print)), RPC_Short(strlen(print)), RPC_Short(0), RPC_Short(28), RPC_End);
  err = dlp_RPC(sd, &p, 0);

  signal(SIGINT, sighandler);
  left = ROMlength - offset;
  i=offset;
  while(left>0) {
  	char buffer[256];
  	int len = left;
  	if (len>256)
  		len=256;
  	printf("\r%ld of %ld bytes", offset, ROMlength);
  	fflush(stdout);
  	PackRPC(&p, 0xA026, RPC_IntReply, RPC_Ptr(buffer, len), RPC_Long(offset+ROMstart), RPC_Long(len), RPC_End);
  	err = dlp_RPC(sd, &p, 0);
  	left -= len;
  	write(file, buffer, len);
  	offset += len;
  	if(cancel || !(i++%4))
  		if (cancel || (dlp_OpenConduit(sd)<0)) {
  			printf("\nCancelled!\n");
  			goto cancel;
  		}
  	if(!(i%8)) {
          sprintf(print, "%ld", offset);
          PackRPC(&p, 0xA220, RPC_IntReply, RPC_Ptr(print, strlen(print)), RPC_Short(strlen(print)), RPC_Short(92), RPC_Short(28), RPC_End);
          err = dlp_RPC(sd, &p, 0);
        }
  }
  printf("\r%ld of %ld bytes\n", offset, ROMlength);
  printf("ROM fetch complete\n");

cancel:
  close(file);
  
  pi_close(sd);  
  
  exit(0);
}
