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
  
  Begin(cmp_rx);
  
  if(!ps->rxq)
    pi_socket_read(ps, 20);
  l = padp_rx(ps, c, sizeof(struct cmp));
  
  if( l <= 0)
    return -1;

  cmp_dump(c,0);

  c->baudrate = ntohl(c->baudrate); /* This baud rate is the _maximum_, not the default */
  c->commversion = ntohl(c->commversion);

  End(cmp_rx);  
}

int cmp_init(struct pi_socket *ps, int baudrate)
{
  char buf[200];
  struct cmp c;

  c.type = 2;
  c.flags = 0;
  c.commversion = 0;

  if(baudrate != 9600) {
  	c.baudrate = baudrate;
  	c.flags |= 0x80;
  } else
  	c.baudrate = 0;
  c.baudrate = htonl(c.baudrate);
  
  cmp_dump(&c,1);
  
  return padp_tx(ps, &c, sizeof(struct cmp), padData);
}

int cmp_abort(struct pi_socket *ps, int reason)
{
  char buf[200];
  struct cmp c;
  
  c.type = 3;
  c.flags = reason;
  c.commversion = 0;
  c.baudrate = 0;
  
  cmp_dump(&c,1);
  
  return padp_tx(ps, &c, sizeof(struct cmp), padData);
}

int cmp_wakeup(struct pi_socket *ps, int maxbaud)
{
  char buf[200];
  struct cmp c;
  
  c.type = 1;
  c.flags = 0;
  c.commversion = htonl(OurCommVersion);
  c.baudrate = htonl(maxbaud);
  
  cmp_dump(&c,1);
  
  return padp_tx(ps, &c, sizeof(struct cmp), padWake);
}

int cmp_dump(struct cmp * cmp, int rxtx)
{
#ifdef DEBUG
  int i;

  fprintf(stderr,"CMP %s ",
	  rxtx ? "TX" : "RX");

 dumpline(cmp, sizeof(struct cmp), 0);
#endif
}
