/* syspkt.c:  Pilot SysPkt manager
 *
 * (c) 1996, Kenneth Albanowski.
 * Derived from padp.c.
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include <stdarg.h>

#include "pi-socket.h"
#include "syspkt.h"

int syspkt_tx(struct pi_socket *ps, unsigned char *msg, int length)
{
  int i;
  int count = 0;
  struct padp *padp;
  struct pi_skb *nskb;
  char buf[64];

#ifdef DEBUG
  fprintf(stderr,"-----------------\n");
#endif

  ps->laddr.port = msg[0];
  ps->raddr.port = msg[1];
  ps->protocol = msg[2];
  /*ps->xid = msg[3];*/
  
  ps->xid++;
  ps->xid &= 0xff;
  if ((!ps->xid) || (ps->xid==0xff)) ps->xid = 0x11; /* some random # */
                 
  nskb = (struct pi_skb *)malloc(sizeof(struct pi_skb));
  memcpy(&nskb->data[10], msg+4, length-4);
  slp_tx(ps, nskb, length-4);
  
  pi_socket_flush(ps);

  return 0;
}

int syspkt_rx(struct pi_socket *ps, unsigned char *buf, int len)
{
  struct padp *padp;
  struct pi_skb *skb;
  int rlen =0;
  
  if (!ps->rxq)
    pi_socket_read(ps, 10);

  skb = ps->rxq;
  ps->rxq = skb->next;
  
  rlen = skb->len;
  
  buf[0] = ps->laddr.port;
  buf[1] = ps->raddr.port;
  buf[2] = ps->protocol;
  buf[3] = ps->xid;
 
  memcpy(buf+4, &skb->data[10], rlen);
  
  free(skb);
  return rlen+4;

}

int sys_RemoteEvent(int sd, int penDown, int x, int y, int keypressed, 
                       int keymod, int keyasc, int keycode)
{
  struct pi_sockaddr raddr;
  char buf[20];
  
  buf[0] = 2;
  buf[1] = 2;
  buf[2] = 0;
  buf[3] = 0x11;
  
  buf[4] = 0x0d; /*RemoteEvtCommand*/
  buf[5] = 0; /*gapfil*/
  buf[6] = penDown;
  buf[7] = 0; /*gapfil*/
  buf[8] = x >> 8;
  buf[9] = x & 0xff;
  buf[10] = y >> 8;
  buf[11] = y & 0xff;
  buf[12] = keypressed;
  buf[13] = 0; /*gapfil*/
  buf[14] = keymod >> 8;
  buf[15] = keymod & 0xff;
  buf[16] = keyasc >> 8;
  buf[17] = keyasc & 0xff;
  buf[18] = keycode >> 8;
  buf[19] = keycode & 0xff;
  
  return pi_write(sd, buf, 16+4);
}

int sys_RPC(int sd, int trap, long * D0, long * A0, int params, struct RPC_param * param, int rep)
{
  unsigned char buf[4096];
  int i;
  unsigned char * c;
  
  buf[0] = 1;
  buf[1] = 1;
  buf[2] = 0;
  
  buf[4] = 0x0a;
  buf[5] = 0;
  set_short(buf+6, trap);
  set_long(buf+8, *D0);
  set_long(buf+12, *A0);
  set_short(buf+16, params);
  
  c = buf+18;
  for(i=params-1;i>=0;i--) {
    set_byte(c, param[i].byRef); c++;
    set_byte(c, param[i].size); c++;
    if(param[i].data)
      memcpy(c, param[i].data, param[i].size);
    c += param[i].size;
  }
  
  pi_write(sd, buf, c-buf);
  
  if(rep) {
    pi_read(sd, buf, 4096);
  
    /* FIXME: Check to make sure incoming packet is response */
  
    *D0 = get_long(buf+8);
    *A0 = get_long(buf+12);
    c = buf+18;
    for(i=params-1;i>=0;i--) {
      c++;
      param[i].size = get_byte(c); c++;
      if(param[i].byRef && param[i].data)
        memcpy(param[i].data, c, param[i].size);
      c += param[i].size;
    }
  }
}

int RPC(int sd, int trap, int ret, ...)
{
  va_list ap;
  struct RPC_param p[20];
  int i=0,j;
  long D0=0,A0=0;

  va_start(ap, ret);
  while(1) {
    int type = va_arg(ap, int);
    if(type == 0)
      break;
    if(type < 0) {
      p[i].byRef = 0;
      p[i].size = -type;
      p[i].data = &va_arg(ap,int);
      p[i].invert = 0;
    } else {
      void * c = va_arg(ap,void*);
      p[i].byRef = 1;
      p[i].size = type;
      p[i].data = c;
      p[i].invert = va_arg(ap,int);
      if(p[i].invert) {
        if(p[i].size == 2) {
          int * s = c;
          *s = htons(*s);
        } else {
          int * l = c;
          *l = htonl(*l);
        }
      }
    }
    i++;
  }
  va_end(ap);
  
  sys_RPC(sd, trap, &D0, &A0, i, p, ret != 2);
  
  for(j=0;j<i;j++) {
      if(p[i].invert) {
        void * c = p[i].data;
        if(p[i].size == 2) {
          int * s = c;
          *s = htons(*s);
        } else {
          int * l = c;
          *l = htonl(*l);
        }
      }
  }
  
  if(ret)
    return A0;
  else
    return D0;
}

int RPC_Int_Void(int sd, int trap) {
  return RPC(sd, trap, 0, RPC_End);
}

int RPC_Ptr_Void(int sd, int trap) {
  return RPC(sd, trap, 1, RPC_End);
}


