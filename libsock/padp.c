/* padp.c:  Pilot PADP protocol
 *
 * (c) 1996, D. Jeff Dionne.
 * Much of this code adapted from Brian J. Swetland <swetland@uiuc.edu>
 *
 * Mostly rewritten by Kenneth Albanowski.
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include <errno.h>
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-padp.h"
#include "pi-slp.h"
#include "pi-serial.h"

#define xmitTimeout 4*10
#define xmitRetries 14*10

/*@+matchanyintegral@*/

int padp_tx(struct pi_socket *ps, void *msg, int len, int type)
/*@-predboolint@*/
{
  int flags = FIRST;
  int tlen;
  int count = 0;

  struct padp padp;

  struct pi_skb* nskb;
  int retries;

#ifdef DEBUG
  fprintf(stderr,"-----------------\n");
#endif

  if (type == padWake) {
    ps->xid = (unsigned char)0xff;
  }

  if(ps->xid == (unsigned char)0)
    ps->xid = (unsigned char)0x11; /* some random # */

  if(ps->initiator) {
    if(ps->xid >= (unsigned char)0xfe)
      ps->nextid = (unsigned char)1; /* wrap */
    else
      ps->nextid = ps->xid+(unsigned char)1;
  } else {
    ps->nextid = ps->xid;
  }
  
  Begin(padp_tx);
  
  do {

    retries = xmitRetries;
    do {

      nskb = (struct pi_skb *)malloc(sizeof(struct pi_skb));
      
      nskb->type = 2;
      nskb->dest = nskb->source = PI_PilotSocketDLP;
      nskb->id = ps->xid;

      tlen = (len > 1024) ? 1024 : len;

      memcpy(&nskb->data[14], msg, tlen);

      padp.type = type &0xff;
      padp.flags = flags | (len == tlen ? LAST : 0);
      padp.size = (flags ? len : count);

      set_byte((unsigned char*)(&nskb->data[10]), padp.type);
      set_byte((unsigned char*)(&nskb->data[11]), padp.flags);
      set_short((unsigned char*)(&nskb->data[12]), padp.size);

      padp_dump(nskb, &padp, 1);
      
      slp_tx(ps, nskb, tlen + 4);
      
      if (type == padTickle) /* Tickles don't get acks */
        break;
      
      At("Reading Ack");
      ps->serial_read(ps, xmitTimeout);

      if(ps->rxq) {
        struct pi_skb *skb;
        struct slp * slp;
        skb = ps->rxq;

        slp = (struct slp*)skb->data;

        padp.type = get_byte((unsigned char*)(&skb->data[10]));
        padp.flags = get_byte((unsigned char*)(&skb->data[11]));
        padp.size = get_short((unsigned char*)(&skb->data[12]));

        padp_dump(skb, &padp, 0);


        if ((slp->type == (unsigned char)2) && (padp.type == (unsigned char)padData) && 
            (slp->id == ps->xid) && (len==0)) {
          fprintf(stderr,"Missing ack\n");
          /* Incoming padData from response to this transmission.
             Maybe the Ack was lost */
          /* Don't consume packet, and return success. */
          return 0;
        } 

        if ((slp->type == (unsigned char)2) && 
            (padp.type == (unsigned char)padAck) && (slp->id == ps->xid)) {
          /* Got correct Ack */
          flags = (unsigned char)padp.flags;

          /* Consume packet */
          ps->rxq = skb->next;
          free(skb);
              
          if (flags & MEMERROR) {
            fprintf(stderr,"Out of memory\n");
            errno = EMSGSIZE;
            return -1; /* Mimimum failure: transmission failed due to lack of
                          memory in reciever link layer, but connection is still
                          active. This transmission was lost, but other
                          transmissions will be received. */
          } else {
            /* Successful Ack */
            msg = ((char *)msg) + tlen;
            len -= tlen;
            count += tlen;
            flags = 0;
            break;
          }
        } else {
          fprintf(stderr,"Weird packet\n");
          /* Got unknown packet */
          /* Don't consume packet */
          errno = EIO;
          return -1; /* Unknown failure: received unknown packet */
        }
      }
    } while(--retries > 0);
    
    if( retries == 0) {
      errno = ETIMEDOUT;
      return -1; /* Maximum failure: transmission failed, and 
                    the connection must be presumed dead */
    }
    
  } while(len);
  
  if( type != padAck) 
    ps->xid = ps->nextid;
    
  End(padp_tx);

  return count;
}

#define recStartTimeout 45*10
#define recSegTimeout 45*10


int padp_rx(struct pi_socket *ps, void *buf, int len)
{
  struct pi_skb *skb;
  struct padp padp;

  struct pi_skb *nskb;
  struct padp npadp;

  struct slp * slp;
  int data_len;
  int offset = 0;
  int ouroffset = 0;
  time_t endtime;
  endtime = time(NULL)+recStartTimeout;

  if(!ps->initiator) {
    if(ps->xid >= 0xfe)
      ps->nextid = 1; /* wrap */
    else
      ps->nextid = ps->xid+1;
  } else {
    ps->nextid = ps->xid;
  }
  
  Begin(padp_rx);

  for(;;) {
    if(time(NULL)>endtime) {
      /* Start timeout, return error */
      errno = ETIMEDOUT;
      return -1;
    }
  
    if (!ps->rxq) {
      ps->serial_read(ps, recStartTimeout);
      continue;
    }

    skb = ps->rxq;
    ps->rxq = skb->next;

    slp = (struct slp*)(skb->data);

    padp.type = get_byte((unsigned char*)(&skb->data[10]));
    padp.flags = get_byte((unsigned char*)(&skb->data[11]));
    padp.size = get_short((unsigned char*)(&skb->data[12]));
    
    if ((slp->type != 2) || (padp.type != padData) || 
        (slp->id != ps->xid) || !(padp.flags & FIRST)) {
      if(padp.type == padTickle) {
        endtime = time(NULL)+recStartTimeout;
        fprintf(stderr,"Got tickled\n");
      }
      fprintf(stderr,"Wrong packet type on queue\n");
      free(skb);
      ps->serial_read(ps, recStartTimeout);
      continue;
    }
    break;
  }
  
  /* OK, we got the expected begin-of-data packet */
  
  endtime = time(NULL) + recSegTimeout;
  
  for(;;) {
  
    At(got data);

    padp_dump(skb, &padp, 0);

    /* Ack the packet */
    
    nskb = (struct pi_skb *)malloc(sizeof(struct pi_skb));

    nskb->type = 2;
    nskb->dest = nskb->source = PI_PilotSocketDLP;
    nskb->id = ps->xid;

    npadp.type = padAck;
    npadp.flags = padp.flags;
    npadp.size = padp.size;

    set_byte((unsigned char*)(&nskb->data[10]), npadp.type);
    set_byte((unsigned char*)(&nskb->data[11]), npadp.flags);
    set_short((unsigned char*)(&nskb->data[12]), npadp.size);

    padp_dump(nskb, &npadp, 1);
  
    slp_tx(ps, nskb, 4);
    pi_serial_flush(ps); /* It's an Ack, so flush it already */
    At(sent Ack);
    
    /* calculate length and offset */
    
    offset = ((padp.flags & FIRST) ? 0 : padp.size);
    data_len = get_short(&skb->data[6])-4;
    
    /* If packet was out of order, ignore it */
    
    if(offset == ouroffset) {
      At(storing block);
      memcpy((unsigned char*)buf + ouroffset, &skb->data[14], data_len);
    	
      ouroffset += data_len;
      free(skb);
    }
    
    if (padp.flags & LAST) {
      break;
    } else  {
      endtime = time(NULL) + recSegTimeout;
      
      for(;;) {
        if(time(NULL)>endtime) {
          fprintf(stderr,"segment timeout\n");
          /* Segment timeout, return error */
          errno = ETIMEDOUT;
          return -1;
        }
        
        if(!ps->rxq) {
          ps->serial_read(ps, recSegTimeout);
          continue;
        }
        
        skb = ps->rxq;
        ps->rxq = skb->next;
      
        slp = (struct slp*)(skb->data);

        padp.type = get_byte((unsigned char*)(&skb->data[10]));
        padp.flags = get_byte((unsigned char*)(&skb->data[11]));
        padp.size = get_short((unsigned char*)(&skb->data[12]));

        if ((slp->type != 2) || (padp.type != padData) || 
            (slp->id != ps->xid) || (padp.flags & FIRST)) {
          if(padp.type == padTickle) {
            endtime = time(NULL)+recSegTimeout;
            fprintf(stderr,"Got tickled\n");
          }
          fprintf(stderr,"Wrong packet type on queue\n");
          free(skb);
          ps->serial_read(ps, recSegTimeout);
          continue;
        }
        At(got good packet);
        break;
      }
    }
  }

  ps->xid = ps->nextid;
  
  End(padp_rx);

  return ouroffset;
}

void padp_dump(struct pi_skb *skb, struct padp* padp, int rxtx)
{
#ifdef DEBUG
  int i;
  int s;
  char *stype;

  switch(padp->type) {
  case padData:
    stype = "DATA"; break;
  case padAck:
    stype = "ACK"; break;
  case padTickle:
    stype = "TICKLE"; break;
  case padWake:
    stype = "WAKE"; break;
  case padAbort:
    stype = "ABORT"; break;
  default:
    stype = "LOOP"; break;
  }

  fprintf(stderr,"PADP %s %s %c%c%c len=0x%.4x\n",
	  stype,
	  rxtx ? "TX" : "RX" ,
	  (padp->flags & FIRST) ? 'F' : ' ',
	  (padp->flags & LAST) ? 'L' : ' ',
          (padp->flags & MEMERROR) ? 'M' : ' ',
	  padp->size);

  s = padp->size;
  if(s>1024) s=1024;
  if (!(padp->type == padAck)) {
    for (i=0; i < s; i += 16) {
      dumpline(&skb->data[14 + i],
	       ((padp->size - i) < 16) ? padp->size - i : 16,
	       i);
    }
  }
#endif
}
