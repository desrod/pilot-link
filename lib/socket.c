/* socket.c:  Berkeley sockets style interface to Pilot SLP/PADP
 *
 * (c) 1996, D. Jeff Dionne.
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <errno.h>
#ifndef bsdi
#include <malloc.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "pi-socket.h"
#include "padp.h"
#include "cmp.h"
#include "dlp.h"

static struct pi_socket *psl = (struct pi_socket *)0;
static int pi_next_socket = 3;            /* FIXME: This should be a real fd */

/* Create a local connection endpoint */

int pi_socket(int domain, int type, int protocol)
{
  struct pi_socket *p;
  struct pi_socket *ps;

  if ((domain != AF_SLP) ||
      (type   != SOCK_STREAM)) {  /* FIXME:  Need to support more */
    errno = EINVAL;
    return -1;
  }

  ps = malloc(sizeof(struct pi_socket));
  memset(ps,0,sizeof(struct pi_socket));

  ps->protocol = protocol;
  ps->connected = 0;

  if (!psl) psl = ps;
  else {
    for (p = psl; p->next; p=p->next);

    p->next = ps;
  }

  return (ps->sd = pi_next_socket++);
}

/* Connect to a remote server */

int pi_connect(int pi_sd, struct pi_sockaddr *addr, int addrlen)
{
  struct pi_socket *ps;
  struct cmp c;
  char buf[5];

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }

  if (pi_device_open(addr->device, ps) == -1) {
    return -1;     /* errno already set */
  }

  ps->rate = 9600;
  ps->raddr = *addr;
  ps->laddr = *addr;     /* FIXME: This is not always true! */

  cmp_wakeup(ps, 38400); /* Assume this box can't go over 38400 */

  pi_socket_read(ps);
  cmp_rx(ps, &c); /* Accept incoming CMP response */
  
  /* FIXME: if that wasn't a CMP response packet, fail or loop */
  
  if(c.type == 2) {
  	/* CMP init packet */
  	
  	if(c.flags & 0x80) {
  		/* Change baud rate */
  		ps->rate = c.baudrate;
  		pi_device_changebaud(addr->device, ps->rate);
  	}
  	return 0;
  	
  } else if(c.type == 3) {
  	/* CMP abort packet -- the other side didn't like us */
  	pi_device_close(addr->device, ps);
  	puts("Received CMP abort from client");
  	
  	return -1;
  }

  return 0;
}

/* Bind address to a local socket */

int pi_bind(int pi_sd, struct pi_sockaddr *addr, int addrlen)
{
  struct pi_socket *ps;

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }

  if (pi_device_open(addr->device, ps) == -1) {
    return -1;     /* errno already set */
  }


  ps->rate = 19200;
  ps->laddr = *addr;
  ps->raddr = *addr;     /* FIXME: This is not always true! */

  return 0;
}

/* Wait for an incoming connection */

int pi_listen(int pi_sd, int backlog)
{
  struct pi_socket *ps;

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }

  pi_socket_read(ps);
  return 0;
}

/* Accept an incoming connection */

int pi_accept(int pi_sd, struct pi_sockaddr *addr, int *addrlen)
{
  struct pi_socket *ps;
  struct cmp c;
  char buf[5];

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }

  cmp_rx(ps, &c); /* Accept incoming CMP wakeup packet */
  
  /* FIXME: if that wasn't a CMP wakeup packet, fail or loop */

  if(c.commversion == OurCommVersion) {
    if(ps->rate > c.baudrate) {
#ifdef DEBUG
      printf("Rate %d too high, dropping to %d\n",ps->rate,c.baudrate);
#endif      
      ps->rate = c.baudrate;
    }
    cmp_init(ps, ps->rate);
    if(ps->rate != 9600) {
      pi_socket_flush(ps);
      pi_device_changebaud(ps, ps->rate);
    }
    ps->connected = 1;

    /* Now that we've made a successful connection,
       make sure it goes away as needed */
    
    installexit();

  } else {
    cmp_abort(ps, 0x80); /* 0x80 means the comm version wasn't compatible*/
    pi_device_close(addr->device, ps);
    
    puts("pi_socket connection failed due to comm version mismatch");
    printf(" (expected 0x%x, got 0x%x)\n", OurCommVersion, c.commversion);

    /* FIXME: set errno to something useful */
    return -1;
  }

  return pi_sd; /* FIXME: return different socket */
}

/* Send msg on a connected socket */

int pi_send(int pi_sd, void *msg, int len, unsigned int flags)
{
  struct pi_socket *ps;
  char buf[200];

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }

  padp_tx(ps,msg,len,padData);
  pi_socket_read(ps);
  padp_rx(ps, buf, 0);
  return len;
}

/* Recv msg on a connected socket */

int pi_recv(int pi_sd, void *msg, int len, unsigned int flags)
{
  struct pi_socket *ps;
  char buf[200];

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }

  pi_socket_read(ps);
  return padp_rx(ps,msg,len,padData);
}

/* Wrapper for recv */

int pi_read(int pi_sd, void *msg, int len)
{
  return pi_recv(pi_sd, msg, len, 0);
}

/* Wrapper for send */

int pi_write(int pi_sd, void *msg, int len)
{
  return pi_send(pi_sd, msg, len, 0);
}

/* Close a connection, destroy the socket */

int pi_close(int pi_sd)
{
  struct pi_socket *ps;
  struct pi_socket *p;

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }
  
  if (ps->connected & 1) { /* If socket is connected */
    if (!(ps->connected & 2)) /* And it wasn't end-of-synced */
      dlp_EndOfSync(pi_sd, 0);  /* Then do it now, with clean status */
  
    pi_socket_flush(ps);
  
    ps->connected = 0;
  
    pi_device_close(ps->raddr.device, ps); /* FIXME: raddr or laddr? */
  }

  if (ps == psl) {
    psl = psl->next;
  } else {
    for (p=psl; p; p=p->next) {
      if (ps == p->next) {
        p->next = p->next->next;
	break;
      }
    }
  }

  free(ps);
  return 0;
}

/* Install an atexit handler that closes open sockets */

int pi_onexit(void)
{
  struct pi_socket *p, *n;

  for (p=psl; p; p=n ) {
    n = p->next;
    pi_close(p->sd);
  }
}

int installexit(void)
{
  static installedexit = 0;
  
  if (!installedexit)
    atexit(pi_onexit);
    
  installedexit = 1;
}


/* Sigh.  Connect can't return a real fd since we don't know the device yet.
   Therefore, we need this uglyness so that ppl can use select() and friends */

int pi_sdtofd(int pi_sd)
{
  struct pi_socket *ps;

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }

  return ps->mac.fd;
}

struct pi_socket *find_pi_socket(int sd)
{
  struct pi_socket *p;

  for (p=psl; p; p=p->next) {
    if (p->sd = sd) return p;
  }

  return 0;
}
