/* getrom:  Fetch ROM image for CoPilot
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 *
 * Written by Kenneth Albanowski, based on code by Greg Hewgill
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "pi-source.h"
#include "pi-serial.h"

char * progname;

void Help(void)
{
  fprintf(stderr,"usage: %s %s\n",progname,TTYPrompt);
  exit(2);
}

struct record * records = 0;

int main(int argc, char *argv[])
{
  int l,p;
  char buf[0xffff];
  int i;
  unsigned char check;
  struct pi_socket ps;
  extern char* optarg;
  extern int optind;
  int rom;

  progname = argv[0];

  if (argc < 2)
    Help();
  
  ps.mac = calloc(1, sizeof(struct pi_mac));
  ps.rate = 38400;
  ps.sd = 0;
  if (pi_device_open(argv[1], &ps) == -1) {
    perror("Unable to open port");
    exit(0);
  }

  rom = open("pilot.rom",O_WRONLY|O_CREAT|O_TRUNC,0666);
  if (rom == -1) {
    perror("Unable to create pilot.rom");
    exit(0);
  }
  
  printf("\
NOTICE: Use of this program may place you in violation of your license\n\
        agreement with US Robotics. Please read page 123 of your Pilot\n\
        handbook (\"Software License Agreement\") before running this program.\
\n\n");
                      
  printf("Please start Getrom.prc on your Pilot.\n");
  printf("Waiting for connection on %s...\n", argv[1]);
  
  for(i=0;i < 128;i++) {
        do {
          l = read(ps.mac->fd, buf, 1);
          if (l<1)
            continue;
  	} while (buf[0] != '*');
  	printf("\r%d/128", i+1);
        fflush(stdout);
  	
  	p = 0;
  	do {
  	  l = read(ps.mac->fd, buf+p, 4096-p);
  	  if (l<0) {
  	    perror("Unable to read sync byte");
  	  }
  	  p += l;
  	} while( p<4096);
  	
  	check = 0xff;
  	if (read(ps.mac->fd, buf+4096, 1)<0) {
  	  perror("Unable to read checksum byte");
  	  goto error;
  	}
  	
  	for (p=0;p<4096;p++)
  	  check = ((check << 1) | check >> 7) ^ buf[p];
  	
  	if ((unsigned char)buf[4096] != check) {
  		printf("\nChecksum error\n");
  		goto error;
  	}
  	
  	write(rom, buf, 4096);
  	
  	buf[4096] = '+';
  	
  	write(ps.mac->fd, buf+4096, 1);
  }
  printf("\nSuccessful!\n");

error:
  close(rom);
          
  pi_device_close(&ps);
  
  exit(0);
}
