/* socket.c:  Berkeley sockets style interface to Pilot SLP/PADP
 *
 * (c) 1996, D. Jeff Dionne.
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-serial.h"
#include "pi-padp.h"
#include "pi-cmp.h"
#include "pi-dlp.h"
#include "pi-syspkt.h"

static struct pi_socket *psl = (struct pi_socket *)0;
static int pi_next_socket = 3;            /* FIXME: This should be a real fd */

void installexit(void);

/* Create a local connection endpoint */

int pi_socket(int domain, int type, int protocol)
{
  struct pi_socket *p;
  struct pi_socket *ps;

  if ((domain != PI_AF_SLP) ||
      ((type  != PI_SOCK_STREAM) &&
      (type   != PI_SOCK_RAW)) ||
      ((protocol != PI_PF_PADP) &&
       (protocol != PI_PF_SLP))) {  /* FIXME:  Need to support more */
    errno = EINVAL;
    return -1;
  }

  ps = malloc(sizeof(struct pi_socket));
  memset(ps,0,sizeof(struct pi_socket));

  ps->type = type;
  ps->protocol = protocol;
  ps->connected = 0;
  ps->mac.fd = 0;
  ps->xid = 0;
  ps->initiator = 0;
  
#ifdef OS2
  ps->os2_read_timeout=60;
  ps->os2_write_timeout=60;
#endif

  if(type == PI_SOCK_STREAM) {
#ifdef __sgi
    ps->establishrate = 9600; /* Default PADP connection rate */
#else
    ps->establishrate = 19200; /* Default PADP connection rate */
#endif
    if (getenv("PILOTRATE"))
    	ps->establishrate = atoi(getenv("PILOTRATE"));
    ps->rate = 9600; /* Mandatory CMP conncetion rate */
  } else if(type == PI_SOCK_RAW) {
    ps->establishrate = ps->rate = 57600; /* Mandatory SysPkt connection rate */
  }
  
  installexit();

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

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }

  if (pi_device_open(addr->device, ps) == -1) {
    return -1;     /* errno already set */
  }

  ps->raddr = *addr;
  ps->laddr = *addr;
   
  if(ps->type == PI_SOCK_STREAM) {

    if(cmp_wakeup(ps, 38400)<0) /* Assume this box can't go over 38400 */
      return -1;

    if(cmp_rx(ps, &c) < 0)
      return -1; /* failed to read, errno already set */

    if(c.type == 2) {
      /* CMP init packet */

      if(c.flags & 0x80) {
        /* Change baud rate */
      ps->rate = c.baudrate;
      pi_device_changebaud(ps);
      }
      return 0;

    } else if(c.type == 3) {
      /* CMP abort packet -- the other side didn't like us */
      pi_device_close(ps);

#ifdef DEBUG
      fprintf(stderr,"Received CMP abort from client\n");
#endif
      errno = -EIO;
      return -1;
    }
  }
  
  ps->initiator = 1; /* We initiated the link */
  
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
  
  ps->laddr = *addr;
  ps->raddr = *addr;

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
  return 0;
}

/* Accept an incoming connection */

int pi_accept(int pi_sd, struct pi_sockaddr *addr, int *addrlen)
{
  struct pi_socket *ps;
  struct cmp c;

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }

  if(ps->type == PI_SOCK_STREAM) {

    pi_socket_read(ps, 20);
    if(cmp_rx(ps, &c) < 0)
      return -1; /* Failed to establish connection, errno already set */

    if (c.commversion == OurCommVersion) {
      if(ps->establishrate > c.baudrate) {
#ifdef DEBUG
        fprintf(stderr,"Rate %d too high, dropping to %ld\n",ps->establishrate,c.baudrate);
#endif
        ps->establishrate = c.baudrate;
      }
      ps->rate = ps->establishrate;
      if(cmp_init(ps, ps->rate)<0)
        return -1;
      if(ps->rate != 9600) {
        pi_socket_flush(ps);
        pi_device_changebaud(ps);
      }
      ps->connected = 1;
    }else {
      cmp_abort(ps, 0x80); /* 0x80 means the comm version wasn't compatible*/
      pi_device_close(ps);

      fprintf(stderr, "pi_socket connection failed due to comm version mismatch\n");
      fprintf(stderr, " (expected 0x%lx, got 0x%lx)\n", OurCommVersion, c.commversion);

      errno = ECONNREFUSED;
      return -1;
    }
  }
  
  ps->initiator = 0; /* We accepted the link, we did not initiate it */
  
  return pi_sd; /* FIXME: return different socket */
}

/* Send msg on a connected socket */

int pi_send(int pi_sd, void *msg, int len, unsigned int flags)
{
  struct pi_socket *ps;

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }

  if(ps->type == PI_SOCK_STREAM) {
    return padp_tx(ps,msg,len,padData);
  } else {
    return syspkt_tx(ps, msg, len);
  }
}

/* Recv msg on a connected socket */

int pi_recv(int pi_sd, void *msg, int len, unsigned int flags)
{
  struct pi_socket *ps;

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }

  if(ps->type == PI_SOCK_STREAM) {
    return padp_rx(ps,msg,len);
  } else {
    return syspkt_rx(ps,msg,len);
  }
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

/* Tickle a stream connection */

int pi_tickle(int pi_sd)
{
  struct pi_socket *ps;

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }
  
  if(ps->type == PI_SOCK_STREAM)
    return padp_tx(ps,0,0,padTickle);
  else {
    errno = EOPNOTSUPP;
    return -1;
  }
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
  
  if (ps->type == PI_SOCK_STREAM) {
    if (ps->connected & 1) /* If socket is connected */
      if (!(ps->connected & 2)) /* And it wasn't end-of-synced */
        dlp_EndOfSync(pi_sd, 0);  /* Then do it now, with clean status */
  }
  
  if(ps->mac.fd) { /* If device was opened */
    pi_socket_flush(ps);
    pi_device_close(ps);
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

void pi_onexit(void)
{
  struct pi_socket *p, *n;

  for (p=psl; p; p=n ) {
    n = p->next;
    pi_close(p->sd);
  }
  
}

void installexit(void)
{
  static installedexit = 0;
  
  if (!installedexit)
    atexit(pi_onexit);
    
  installedexit = 1;
}

/* Get the local address for a socket */

int pi_getsockname(int pi_sd, struct pi_sockaddr * addr, int * namelen)
{
  struct pi_socket *ps;

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }
  
  if(addr)
    *addr = ps->laddr;
  if(namelen)
    *namelen = sizeof(struct pi_sockaddr);
    
  return 0;
}

/* Get the remote address for a socket */

int pi_getsockpeer(int pi_sd, struct pi_sockaddr * addr, int * namelen)
{
  struct pi_socket *ps;

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }
  
  if(addr)
    *addr = ps->raddr;
  if(namelen)
    *namelen = sizeof(struct pi_sockaddr);
    
  return 0;
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
    if (p->sd == sd) return p;
  }

  return 0;
}
