/* syspkt.c:  Pilot SysPkt manager
 *
 * (c) 1996, Kenneth Albanowski.
 * Derived from padp.c.
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include "pi-socket.h"
#include "syspkt.h"

int syspkt_tx(struct pi_socket *ps, void *msg, int length)
{
  int i;
  int count = 0;
  struct padp *padp;
  struct pi_skb *nskb;
  char buf[64];

#ifdef DEBUG
  fprintf(stderr,"-----------------\n");
#endif

  ps->xid++;
  ps->xid &= 0xff;
  if ((!ps->xid) || (ps->xid==0xff)) ps->xid = 0x11; /* some random # */
                 
  nskb = (struct pi_skb *)malloc(sizeof(struct pi_skb));
  memcpy(&nskb->data[10], msg, length);
  slp_tx(ps, nskb, length);

  return 0;
}

int syspkt_rx(struct pi_socket *ps, void *buf, int len)
{
  struct padp *padp;
  struct pi_skb *skb;
  int rlen =0;

  if (!ps->rxq) return 0;

  skb = ps->rxq;
  ps->rxq = skb->next;
  
  rlen = skb->len;
 
  memcpy(buf, &skb->data[10], rlen);
  
  free(skb);
  return rlen;

}

#if 0
int padp_dump(struct pi_skb *skb, struct padp *padp, int rxtx)
{
#ifdef DEBUG
  int i;

  fprintf(stderr,"PADP %s %s %c%c len=0x%.4x\n",
	  ((padp->type == padData) ? "DATA" : 
	   (padp->type == padAck) ? "ACK " : 
	   (padp->type == padTickle) ? "TCKL" :
	   "LOOP"),
	  rxtx ? "TX" : "RX" ,
	  (padp->flags & FIRST) ? 'F' : ' ',
	  (padp->flags & LAST) ? 'L' : ' ',
	  ntohs(padp->size));

  if (!(padp->type == padAck)) {
    for (i=0; i < (ntohs(padp->size) & 0x3ff); i += 16) {
      dumpline(&skb->data[14 + i],
	       ((ntohs(padp->size) - i) < 16) ? ntohs(padp->size) - i : 16,
	       i);
    }
  }
#endif
}
#endif

int sys_RemoteEvent(int sd, int penDown, int x, int y, int keypressed, 
                       int keymod, int keyasc, int keycode)
{
  struct pi_sockaddr raddr;
  char buf[16];
  
  buf[0] = 0x0d; /*RemoteEvtCommand*/
  buf[1] = 0; /*gapfil*/
  buf[2] = penDown;
  buf[3] = 0; /*gapfil*/
  buf[4] = x >> 8;
  buf[5] = x & 0xff;
  buf[6] = y >> 8;
  buf[7] = y & 0xff;
  buf[8] = keypressed;
  buf[9] = 0; /*gapfil*/
  buf[10] = keymod >> 8;
  buf[11] = keymod & 0xff;
  buf[12] = keyasc >> 8;
  buf[13] = keyasc & 0xff;
  buf[14] = keycode >> 8;
  buf[15] = keycode & 0xff;
  
  raddr.port = PilotSocketRemoteUI;
  return pi_sendto(sd, buf, 16, 0, &raddr, 0);
}
