/* cmp.c:  Pilot CMP protocol
 *
 * (c) 1996, Kenneth Albanowski.
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include "pi-socket.h"
#include "padp.h"
#include "cmp.h"

int cmp_rx(struct pi_socket *ps, struct cmp * c)
{
  int l;
  unsigned char cmpbuf[10];
  
  Begin(cmp_rx);
  
  if(!ps->rxq)
    pi_socket_read(ps, 20);
  l = padp_rx(ps, cmpbuf, 10);
  
  if( l < 10)
    return -1;
    
  cmp_dump(cmpbuf,0);
            
  c->type = get_byte(cmpbuf);
  c->flags = get_byte(cmpbuf+1);
  c->commversion = get_long(cmpbuf+2);
  c->baudrate = get_long(cmpbuf+6);

  End(cmp_rx);  
}

int cmp_init(struct pi_socket *ps, int baudrate)
{
  char cmpbuf[10];
  struct cmp c;

  set_byte(cmpbuf+0, 2);
  set_long(cmpbuf+2, 0);
  set_long(cmpbuf+6, baudrate);

  if(baudrate != 9600) 
        set_byte(cmpbuf+1, 0x80);
  else
        set_byte(cmpbuf+1, 0);
        
  cmp_dump(cmpbuf, 1);
  
  return padp_tx(ps, cmpbuf, 10, padData);
}

int cmp_abort(struct pi_socket *ps, int reason)
{
  char cmpbuf[10];
  
  set_byte(cmpbuf+0, 3);
  set_byte(cmpbuf+1, reason);
  set_long(cmpbuf+2, 0);
  set_long(cmpbuf+6, 0);

  cmp_dump(cmpbuf, 1);
  
  return padp_tx(ps, cmpbuf, 10, padData);
}

int cmp_wakeup(struct pi_socket *ps, int maxbaud)
{
  char cmpbuf[200];
  
  set_byte(cmpbuf+0, 1);
  set_byte(cmpbuf+1, 0);
  set_long(cmpbuf+2, OurCommVersion);
  set_long(cmpbuf+6, maxbaud);

  cmp_dump(cmpbuf, 1);
  
  return padp_tx(ps, cmpbuf, 10, padWake);
}

int cmp_dump(unsigned char * cmp, int rxtx)
{
#ifdef DEBUG
  int i;
  

  fprintf(stderr,"CMP %s %s",
	  rxtx ? "TX" : "RX",
	  (get_byte(cmp) == 1) ? "WAKE" :
	  (get_byte(cmp) == 2) ? "INIT" :
	  (get_byte(cmp) == 3) ? "ABRT" : 
	  "");
  if((get_byte(cmp) < 1) || (get_byte(cmp) > 3))
  	fprintf(stderr, "UNK %d", get_byte(cmp));
  fprintf(stderr,"  Flags: %2.2X Version: %8.8X Baud: %8.8X (%d)\n",
	  get_byte(cmp+1),
	  get_long(cmp+2),
	  get_long(cmp+6),
	  get_long(cmp+6));

#endif
}
