/*
 * slp.c:  Pilot SLP protocol
 *
 * (c) 1996, D. Jeff Dionne.
 * Much of this code adapted from Brian J. Swetland <swetland@uiuc.edu>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. *
 */

#ifdef WIN32
#include <winsock.h>
#endif
#include <stdio.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-slp.h"

/***********************************************************************
 *
 * Function:    slp_tx
 *
 * Summary:     Build and queue up an SLP packet to be transmitted
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int slp_tx(struct pi_socket *ps, struct pi_skb *nskb, int len)
{
	struct pi_skb *skb;
	struct slp *slp;

	unsigned int i;
	unsigned int n;

	slp = (struct slp *) nskb->data;

	slp->_be 	= 0xbe;
	slp->_ef 	= 0xef;
	slp->_ed 	= 0xed;
	slp->dest 	= nskb->dest;
	slp->src 	= nskb->source;
	slp->type 	= nskb->type;
	slp->dlen 	= htons(len);
	slp->id 	= nskb->id;

	for (n = i = 0; i < 9; i++)
		n += nskb->data[i];
	slp->csum = 0xff & n;

	set_short(&nskb->data[len + 10], crc16(nskb->data, len + 10));

	nskb->len = len + 12;
	nskb->next = (struct pi_skb *) 0;

	ps->busy++;
	if (!ps->txq)
		ps->txq = nskb;
	else {
		for (skb = ps->txq; skb->next; skb = skb->next);
		skb->next = nskb;
	}
	ps->busy--;

	dph(nskb->data);
	slp_dump(nskb, 1);

	ps->tx_packets++;
	return 0;
}

/* Sigh.  SLP is a really broken protocol.  It has no proper framing, so it
   makes a proper "device driver" layer impossible.  There ought to be a
   layer below SLP that reads frames off the wire and passes them up. 
   Insted, all we can do is have the device driver give us bytes and SLP has
   to keep a pile of status info while it builds frames for itself.  So
   here's the code that does that. */

/***********************************************************************
 *
 * Function:    slp_rx
 *
 * Summary:     Accept SLP packets on the wire
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int slp_rx(struct pi_socket *ps)
{
	int i;
	int v;
	struct pi_skb *skb;

	if (!ps->mac->state) {
		ps->mac->expect = 1;
		ps->mac->state = 1;
		ps->mac->rxb =
		    (struct pi_skb *) malloc(sizeof(struct pi_skb));

		ps->mac->rxb->next = (struct pi_skb *) 0;
		ps->mac->buf = ps->mac->rxb->data;
		return 0;
	}

	v = 0xff & (int) *ps->mac->buf;

	switch (ps->mac->state) {

	case 1:
		if (v == 0xbe) {
			ps->mac->state++;
			ps->mac->expect = 1;
			ps->mac->buf++;
		} else
			ps->mac->expect = 1;
		break;

	case 2:
		if (v == 0xef) {
			ps->mac->state++;
			ps->mac->expect = 1;
			ps->mac->buf++;
		}
		break;

	case 3:
		if (v == 0xed) {
			/* OK.  we think we're sync'ed, so go for the rest
			   of the header */
			ps->mac->state++;
			ps->mac->expect = 7;
			ps->mac->buf++;
		}
		break;

	case 4:
		/* read in the whole SLP header. */
		for (v = i = 0; i < 9; i++)
			v += ps->mac->rxb->data[i];

		dph(ps->mac->rxb->data);

		if ((v & 0xff) == ps->mac->rxb->data[9]) {
			ps->mac->state++;
			ps->mac->rxb->len =
			    12 + get_short(&ps->mac->rxb->data[6]);
			ps->mac->expect = ps->mac->rxb->len - 10;
			ps->mac->buf += 7;
		}
		break;

	case 5:
		/* that should be the whole packet. */
		v = crc16(ps->mac->rxb->data, ps->mac->rxb->len - 2);

		if ((v ==
		     get_short(&ps->mac->rxb->data[ps->mac->rxb->len - 2]))
		    || (ps->mac->rxb->data[5] == 3 /* PI_PF_LOOP */ )
		    /* we'll ignore LOOP packets anyway, so we'll disregard
		       CRC errors for them -- working around a problem where
		       the tenth LOOP packet carries an incorrect CRC value */
#if 0
		    || (0xbeef ==
			get_short(&ps->mac->rxb->
				  data[ps->mac->rxb->len - 2]))
#endif
		    ) {

			ps->mac->rxb->dest = ps->mac->rxb->data[3];
			ps->mac->rxb->source = ps->mac->rxb->data[4];
			ps->mac->rxb->type = ps->mac->rxb->data[5];
			ps->mac->rxb->id = ps->mac->rxb->data[8];
			/* ps->xid = ps->mac->rxb->data[8];
			   ps->laddr.pi_port = ps->mac->rxb->data[3];
			   ps->raddr.pi_port = ps->mac->rxb->data[4]; 
			   ps->protocol = ps->mac->rxb->data[5]; XXX */

			/* hack to ignore LOOP packets... */

			if (ps->mac->rxb->data[5] == 3 /* PI_PF_LOOP */ ) {
				ps->mac->expect = 1;
				ps->mac->state = 1;
				ps->mac->rxb->next = (struct pi_skb *) 0;
				ps->mac->buf = ps->mac->rxb->data;
			} else {
				if (!ps->rxq)
					ps->rxq = ps->mac->rxb;
				else {

					for (skb = ps->rxq; skb->next;
					     skb = skb->next);
					skb->next = ps->mac->rxb;
				}
				ps->mac->state = 0;
			}
			ps->rx_packets++;
		} else {
#ifdef DEBUG
			fprintf(stderr, "my crc=0x%.4x your crc=0x%.4x\n",
				v,
				get_short((&ps->mac->rxb->
					   data[ps->mac->rxb->len - 2])));
#endif
		}
		slp_dump(ps->mac->rxb, 0);
		break;

	default:
		break;
	}

	if (ps->mac->state && (!ps->mac->expect)) {

#ifdef DEBUG
		fprintf(stderr, "SLP RX: error, state %d \n",
			ps->mac->state);
#endif

		ps->mac->state = ps->mac->expect = 1;
		ps->mac->buf = ps->mac->rxb->data;
		ps->rx_errors++;
	}

	return 0;
}

/***********************************************************************
 *
 * Function:    slp_dump
 *
 * Summary:     Dump the contents of the SPL frame
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void slp_dump(struct pi_skb *skb, int rxtx)
{
#ifdef DEBUG
	fprintf(stderr, "SLP %s %d->%d len=0x%.4x Prot=%d ID=0x%.2x\n",
		rxtx ? "TX" : "RX", skb->data[4], skb->data[3],
		get_short(&skb->data[6]), skb->data[5], skb->data[8]);
#endif
}

/***********************************************************************
 *
 * Function:    dph
 *
 * Summary:     Dump the raw data to stderr
 *
 * Parmeters:   None
 *
 * Returns:     Nothing 
 *
 ***********************************************************************/
void dph(unsigned char *d)
{
#ifdef DEBUG
	int i;

	fprintf(stderr, "SLP HDR [");
	for (i = 0; i < 10; i++)
		fprintf(stderr, " 0x%.2x", 0xff & d[i]);
	fprintf(stderr, "]\n");
#endif
}
