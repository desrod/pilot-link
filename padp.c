/* padp.c:  Pilot PADP protocol
 *
 * (c) 1996, D. Jeff Dionne.
 * Much of this code adapted from Brian J. Swetland <swetland@uiuc.edu>
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include "pi-socket.h"
#include "padp.h"

int padp_tx(struct pi_socket *ps, void *msg, int len, int type)
{
  int i;
  int flags = FIRST;
  int tlen;
  int count = 0;
  struct padp *padp;
  struct pi_skb *nskb;
  char buf[64];

#ifdef DEBUG
  fprintf(stderr,"-----------------\n");
#endif

  if (type == padData || type == padTickle) {
    ps->xid++;
    ps->xid &= 0xff;
    if ((!ps->xid) || (ps->xid==0xff)) ps->xid = 0x11; /* some random # */
  }

  if (type == padWake) ps->xid = 0xff;

  do {

    if (!flags) {
      pi_socket_read(ps);
      padp_rx(ps, buf, 0);
    }

    nskb = (struct pi_skb *)malloc(sizeof(struct pi_skb));

    tlen = (len > 1024) ? 1024 : len;

    memcpy(&nskb->data[14], msg, tlen);
    msg += tlen;

    padp = (struct padp *)(&nskb->data[10]);
    padp->type = type & 0xff;

    padp->size = htons(flags ? len : count);
    len -= tlen;
    count += tlen;

    padp->flags = flags | (!len ? LAST : 0);
    flags = 0;

    padp_dump(nskb,padp,1);

    slp_tx(ps, nskb, tlen + 4);
  } while (len);

  return 0;
}

int padp_rx(struct pi_socket *ps, void *buf, int len)
{
  struct padp *padp;
  struct pi_skb *skb;
  int rlen =0;

  if (!ps->rxq) return 0;

  skb = ps->rxq;
  ps->rxq = skb->next;

  padp = (struct padp *)(&skb->data[10]);

  padp_dump(skb, padp, 0);

  if (padp->type == padData) {

    padp_tx(ps,(void *)padp,0,padAck);
    rlen = (ntohs(padp->size) <= len) ? ntohs(padp->size) : len;
    memcpy(buf,&skb->data[14],rlen);
  }

  free (skb);
  return rlen;
}

int padp_dump(struct pi_skb *skb, struct padp *padp, int rxtx)
{
#ifdef DEBUG
  int i;

  fprintf(stderr,"PADP %s %s %c%c len=0x%.4x\n",
	  (padp->type == padData) ? "DATA" : ((padp->type == padAck) ? "ACK " : "LOOP"),
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




