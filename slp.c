/* slp.c:  Pilot SLP protocol
 *
 * (c) 1996, D. Jeff Dionne.
 * Much of this code adapted from Brian J. Swetland <swetland@uiuc.edu>
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#ifndef bsdi
#include <malloc.h>
#endif
#include "pi-socket.h"
#include "slp.h"

/* Build and queue up an SLP packet to be transmitted */

int slp_tx(struct pi_socket *ps, struct pi_skb *nskb, int len)
{
  struct pi_skb *skb;
  struct slp *slp;

  unsigned int i;
  unsigned int n;

  slp = (struct slp *)nskb->data;

  slp->_be  = 0xbe;
  slp->_ef  = 0xef;
  slp->_ed  = 0xed;
  slp->dest = ps->raddr.port;
  slp->src  = ps->laddr.port;
  slp->type = ps->protocol;
  slp->dlen = htons(len);
  slp->id   = ps->xid;

  for (n = i = 0; i<9; i++) n += nskb->data[i];
  slp->csum = 0xff & n;

  *(unsigned short *)(&nskb->data[len+10]) = htons(crc16(nskb->data, len+10));
  nskb->len = len+12;
  nskb->next = (struct pi_skb *)0;

  if (!ps->txq) ps->txq = nskb;
  else {
    for (skb = ps->txq; skb->next; skb=skb->next);
    skb->next = nskb;
  }

  dph(nskb->data);
  slp_dump(nskb,1);
  ps->tx_packets++;
  return 0;
}

/* Sigh.  SLP is a really broken protocol.  It has no proper framing, so
   it makes a proper "device driver" layer impossible.  There ought to be
   a layer below SLP that reads frames off the wire and passes them up.
   Insted, all we can do is have the device driver give us bytes and SLP
   has to keep a pile of status info while it builds frames for itself.
   So here's the code that does that. */

int slp_rx(struct pi_socket *ps)
{
  int i;
  int v;
  struct pi_skb *skb;

  if (!ps->mac.state) {
    ps->mac.expect = 1;
    ps->mac.state = 1;
    ps->mac.rxb = (struct pi_skb *)malloc(sizeof(struct pi_skb));
    ps->mac.rxb->next = (struct pi_skb *)0;
    ps->mac.buf = ps->mac.rxb->data;
    return 0;
  }

  v = 0xff & (int) *ps->mac.buf;

  switch(ps->mac.state) {

  case 1:
    if (v == 0xbe) { 
      ps->mac.state++;
      ps->mac.expect = 1;
      ps->mac.buf++;
    }
    else ps->mac.expect = 1;
    break;

  case 2:
    if (v == 0xef) { 
      ps->mac.state++;
      ps->mac.expect = 1;
      ps->mac.buf++;
    }
    break;

  case 3:
    if (v == 0xed) { 
    /* OK.  we think we're sync'ed, so go for the rest of the header */
      ps->mac.state++;
      ps->mac.expect = 7;
      ps->mac.buf++;
    }
    break;

  case 4:
    /* read in the whole SLP header. */
    for (v = i = 0; i<9; i++) v += ps->mac.rxb->data[i];

    dph(ps->mac.rxb->data);

    if ((v & 0xff) == ps->mac.rxb->data[9]) {
      ps->mac.state++;
      ps->mac.rxb->len = 12+ ntohs(*(unsigned short *)(&ps->mac.rxb->data[6]));
      ps->mac.expect = ps->mac.rxb->len - 10;
      ps->mac.buf += 7;
    }
    break;

  case 5:
    /* that should be the whole packet. */
    v = crc16(ps->mac.rxb->data, ps->mac.rxb->len - 2);

    if ((v == 
	ntohs(*(unsigned short *)(&ps->mac.rxb->data[ps->mac.rxb->len - 2])))
#if 0
	|| (0xbeef == 
	ntohs(*(unsigned short *)(&ps->mac.rxb->data[ps->mac.rxb->len - 2])))
#endif
	) {

      ps->xid = ps->mac.rxb->data[8];

      /* hack to ignore LOOP packets... */

      if (ps->mac.rxb->data[5] == 2) {

	if (!ps->rxq) ps->rxq = ps->mac.rxb;
	else {

	  for (skb = ps->rxq; skb->next; skb=skb->next);
	  skb->next = ps->mac.rxb;
	}
	ps->mac.state = 0;
      } else {
	ps->mac.expect = 1;
	ps->mac.state = 1;
	ps->mac.rxb->next = (struct pi_skb *)0;
	ps->mac.buf = ps->mac.rxb->data;
      }
      ps->rx_packets++;
    } else {
#ifdef DEBUG
      fprintf(stderr,"my crc=0x%.4x your crc=0x%.4x\n", v, ntohs(*(unsigned short *)(&ps->mac.rxb->data[ps->mac.rxb->len - 2])));
#endif
    }
    slp_dump(ps->mac.rxb,0);
    break;

  default:
  }

  if (ps->mac.state && (!ps->mac.expect)) {

#ifdef DEBUG
    fprintf(stderr, "SLP RX: error, state %d \n",ps->mac.state);
#endif

    ps->mac.state = ps->mac.expect = 1;
    ps->mac.buf = ps->mac.rxb->data;
    ps->rx_errors++;
  }

  return 0;
}

int slp_dump(struct pi_skb *skb, int rxtx)
{
#ifdef DEBUG
  int i;

  fprintf(stderr,"SLP %s %d->%d len=0x%.4x Prot=%d ID=0x%.2x\n",
	  rxtx ? "TX" : "RX" ,
	  skb->data[4],skb->data[3],
	  ntohs(*(unsigned short *)(&skb->data[6])),
	  skb->data[5],skb->data[8]);
#endif
}


dph(unsigned char *d)
{
#ifdef DEBUG
  int i;

  fprintf(stderr,"SLP HDR [");
  for (i=0;i<10;i++) fprintf (stderr," 0x%.2x",0xff & d[i]);
  fprintf(stderr,"]\n");
#endif
}
