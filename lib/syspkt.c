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

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-syspkt.h"
#include "pi-slp.h"
#include "pi-serial.h"

int sys_RPCerror;

int syspkt_tx(struct pi_socket *ps, unsigned char *msg, int length)
{
  struct pi_skb *nskb;

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
  struct pi_skb *skb;
  int rlen =0;
  
  if (!ps->rxq)
    pi_socket_read(ps, 1);
  
  if (!ps->rxq)
    return 0;

  skb = ps->rxq;
  ps->rxq = skb->next;
  
  rlen = skb->len-12;
  
  buf[0] = ps->laddr.port;
  buf[1] = ps->raddr.port;
  buf[2] = ps->protocol;
  buf[3] = ps->xid;
 
  memcpy(buf+4, &skb->data[10], rlen);
  
  free(skb);
  return rlen+4;

}


int sys_UnpackState(void * buffer, struct Pilot_state * s)
{
  unsigned char * data = buffer;
// printf("\n%s:%x:d84> current state:\n",
//	    		sidename[slp->side], slp->pid);
  s->reset = get_short(data);
  s->exception = get_short(data+2);
  memcpy(s->func_name, data+152, 32);
  s->func_name[32-1] = 0;
  sys_UnpackRegisters(data+4, &s->regs);
  
  return 0;
/*	    	printf("  Just reset: %s, exception: %4.4x\n",
	    		get_short(slp->data+2) ? "yes" : "no", 
	    		get_short(slp->data+4));
	    	for (i=0;i<8;i++) {
	    		printf("  D%d: %8.8X",
	    			i, get_long(slp->data+6+i*4));
	    		if (i<7)
	    			printf("    A%d: %8.8X",
	    				i, get_long(slp->data+38+i*4));
	    		printf("\n");
	    	}
	    	printf("  USP: %8.8X    SSP: %8.8X\n",
	    		get_long(slp->data+66), get_long(slp->data+70));
	    	printf("  PC:  %8.8X    SR:  %4.4X\n",
	    		get_long(slp->data+74), get_short(slp->data+78));
	    	printf("  Instructions at PC:\n");
	    	dumpdata(slp->data+80, 15*2);
	    	printf("  Breakpoints:\n");
	    	for (i=0;i<6;i++) {
	    		printf("    #%d: @ %8.8X, Enabled: %s, installed %s\n",
	    			i,
	    			get_long(slp->data+110+i*6),
	    			get_byte(slp->data+114+i*6) ? "yes" : "no",
	    			get_byte(slp->data+115+i*6) ? "yes" : "no");
	    	}
	    	printf("  Start address of function: %8.8X, end address: %8.8X\n",
	    		get_long(slp->data+146), get_long(slp->data+150));
	    	printf("  Function name '%.32s'\n", slp->data+154);
	    	printf("  Trap table revision: %x\n", get_short(slp->data+186));
	    	break;
*/
}

int sys_UnpackRegisters(void * data, struct Pilot_registers * r)
{
  unsigned char * buffer = data;
  r->D[0] = get_long(buffer+0);
  r->D[1] = get_long(buffer+4);
  r->D[2] = get_long(buffer+8);
  r->D[3] = get_long(buffer+12);
  r->D[4] = get_long(buffer+16);
  r->D[5] = get_long(buffer+20);
  r->D[6] = get_long(buffer+24);
  r->D[7] = get_long(buffer+28);
  r->A[0] = get_long(buffer+32);
  r->A[1] = get_long(buffer+36);
  r->A[2] = get_long(buffer+40);
  r->A[3] = get_long(buffer+44);
  r->A[4] = get_long(buffer+48);
  r->A[5] = get_long(buffer+52);
  r->A[6] = get_long(buffer+56);
  r->USP = get_long(buffer+60);
  r->SSP = get_long(buffer+64);
  r->PC = get_long(buffer+68);
  r->SR = get_short(buffer+72);
  
  return 0;
}

int sys_PackRegisters(void * data, struct Pilot_registers * r)
{
  unsigned char * buffer = data;
  int i;
  for (i=0;i<8;i++)
    set_long(buffer+i*4, r->D[i]);
  for (i=0;i<7;i++)
    set_long(buffer+32+i*4, r->A[i]);
  set_long(buffer+60, r->USP);
  set_long(buffer+64, r->SSP);
  set_long(buffer+68, r->PC);
  set_short(buffer+72, r->SR);
  
  return 0;
}

int sys_Continue(int sd, struct Pilot_continue * c)
{
  char buf[94];
  
  buf[0] = 0;
  buf[1] = 0;
  buf[2] = 0;
  buf[3] = 0;
  
  buf[4] = 0x07;
  buf[5] = 0; /*gapfil*/
  
  sys_PackRegisters(buf+6, &c->regs);
  set_byte(buf+80, c->watch);
  set_byte(buf+81, 0);
  set_long(buf+82, c->watch_address);
  set_long(buf+86, c->watch_length);
  set_long(buf+90, c->watch_checksum);
  
  return pi_write(sd, buf, 94);
}

int sys_SetBreakpoints(int sd, struct Pilot_breakpoint * b)
{
  char buf[94];
  int i;
  
  buf[0] = 0;
  buf[1] = 0;
  buf[2] = 0;
  buf[3] = 0;
  
  buf[4] = 0x0c;
  buf[5] = 0; /*gapfil*/
  
  for (i=0;i<6;i++) {
    set_long(buf+6+i*6, b[i].address);
    set_byte(buf+7+i*6, b[i].enabled);
    set_byte(buf+8+i*6, b[i].installed);
  }
  
  return pi_write(sd, buf, 42);
}

int sys_QueryState(int sd)
{
  char buf[6];
  
  buf[0] = 0;
  buf[1] = 0;
  buf[2] = 0;
  buf[3] = 0;
  
  buf[4] = 0;
  buf[5] = 0; /*gapfil*/
  
  return pi_write(sd, buf, 6);
}


int sys_RemoteEvent(int sd, int penDown, int x, int y, int keypressed, 
                       int keymod, int keyasc, int keycode)
{
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

int sys_RPC(int sd, int socket, int trap, long * D0, long * A0, int params, struct RPC_param * param, int reply)
{
  unsigned char buf[4096];
  int i;
  unsigned char * c;
  
  buf[0] = socket; /* 0 for debug, 1 for console */
  buf[1] = socket;
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
  
  if (socket == 3)
    set_short(buf+4, c-buf - 6);
  
  pi_write(sd, buf, c-buf);
  
  if(reply) {
    int l = pi_read(sd, buf, 4096);
  
    if (l < 0)
      return l;
    if (l < 6)
      return -1;
    if (buf[4] != 0x8a)
      return -2;
    
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
  return 0;
}

int RPC(int sd, int socket, int trap, int reply, ...)
{
  va_list ap;
  struct RPC_param p[20];
  int RPC_arg[20];
  int i=0,j;
  long D0=0,A0=0;

  va_start(ap, reply);
  while(1) {
    int type = va_arg(ap, int);
    if(type == 0)
      break;
    if(type < 0) {
      p[i].byRef = 0;
      p[i].size = -type;
      RPC_arg[i] = va_arg(ap,int);
      p[i].data = &RPC_arg[i];
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
  
  sys_RPCerror = sys_RPC(sd, socket, trap, &D0, &A0, i, p, reply != 2);
  
  for(j=0;j<i;j++) {
      if(p[j].invert) {
        void * c = p[j].data;
        if(p[j].size == 2) {
          int * s = c;
          *s = htons(*s);
        } else {
          int * l = c;
          *l = htonl(*l);
        }
      }
  }
  
  if(reply)
    return A0;
  else
    return D0;
}

int PackRPC(struct RPC_params * p, int trap, int reply, ...)
{
  va_list ap;
  int i=0;
  
  p->trap = trap;
  p->reply = reply;

  va_start(ap, reply);
  while(1) {
    int type = (int)va_arg(ap, int);
    if(type == 0)
      break;
    if(type < 0) {
      p->param[i].byRef = 0;
      p->param[i].size = -type;
      p->param[i].arg = (int)va_arg(ap, int);
      p->param[i].data = &p->param[i].arg;
      p->param[i].invert = 0;
    } else {
      void * c = (void*)va_arg(ap,void*);
      p->param[i].byRef = 1;
      p->param[i].size = type;
      p->param[i].data = c;
      p->param[i].invert = (int)va_arg(ap,int);
      if(p->param[i].invert) {
        if(p->param[i].size == 2) {
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
  p->args = i;
  va_end(ap);
  
  return 0;
}

unsigned long DoRPC(int sd, int socket, struct RPC_params * p, int * error)
{
  int j;
  int err;
  unsigned long D0=0,A0=0;

  err = sys_RPC(sd, socket, p->trap, &D0, &A0, p->args, &p->param[0], p->reply);
  
  if (error)
    *error = err;
  
  for(j=0;j<p->args;j++) {
      if(p->param[j].invert) {
        void * c = p->param[j].data;
        if(p->param[j].size == 2) {
          int * s = c;
          *s = htons(*s);
        } else {
          int * l = c;
          *l = htonl(*l);
        }
      }
  }
  
  if(p->reply==RPC_PtrReply)
    return A0;
  else if (p->reply==RPC_IntReply)
    return D0;
  else
    return err;
}

int RPC_Int_Void(int sd, int trap) {
  return RPC(sd, 1, trap, 0, RPC_End);
}

int RPC_Ptr_Void(int sd, int trap) {
  return RPC(sd, 1, trap, 1, RPC_End);
}

/* Untested complex RPC example
int RPC_MemCardInfo(int sd, int cardno, char * cardname, char * manufname,
                    int * version, long * date, long * romsize, long * ramsize,
                    long * freeram) {
  return RPC(sd, 1, 0xA004, 0, RPC_Short(cardno), RPC_Ptr(cardname, 32), 
                               RPC_Ptr(manufname, 32), RPC_ShortPtr(version),
                               RPC_LongPtr(date), RPC_LongPtr(romsize),
                               RPC_LongPtr(ramsize), RPC_LongPtr(freeram));
}                    
*/
