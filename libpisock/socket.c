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
#include <signal.h>
#include <fcntl.h>
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-serial.h"
#include "pi-net.h"
#include "pi-padp.h"
#include "pi-cmp.h"
#include "pi-dlp.h"
#include "pi-syspkt.h"

static struct pi_socket *psl = (struct pi_socket *)0;

void installexit(void);

extern int dlp_trace;

/* Automated tickling interval */
int interval=0;
int busy=0;

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

#ifdef OS2
  if((ps->sd = open("NUL", O_RDWR))==-1) {
#else
  if((ps->sd = open("/dev/null", O_RDWR))==-1) {
#endif
    int err = errno; /* Save errno of open */
    free(ps);
    errno = err;
    return -1;
  }
  ps->mac = calloc(1, sizeof(struct pi_mac));
  ps->type = type;
  ps->protocol = protocol;
  ps->connected = 0;
  ps->mac->fd = 0;
  ps->mac->ref = 1;
  ps->xid = 0;
  ps->initiator = 0;
  ps->minorversion = 0;
  ps->majorversion = 0;
  ps->version = 0;
  ps->dlprecord = 0;
  ps->busy = 0;
  
#ifdef OS2
  ps->os2_read_timeout=60;
  ps->os2_write_timeout=60;
#endif

#ifndef NO_SERIAL_TRACE
  ps->debuglog = 0;
  ps->debugfd = 0;
  
  if (getenv("PILOTLOG")) {
    if ((ps->debuglog = getenv("PILOTLOGFILE"))==0)
      ps->debuglog = "PiDebug.log";
  }
#endif

#ifndef NO_DLP_TRACE
  if (getenv("PILOTDLP")) {
    dlp_trace=1;
  }
#endif

  if(type == PI_SOCK_STREAM) {
    ps->establishrate = 9600; /* Default PADP connection rate */
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
  
  return ps->sd;
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

  if (pi_serial_device_open(addr->pi_device, ps) == -1) {
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
        ps->device_changebaud(ps);
      }
      return 0;

    } else if(c.type == 3) {
      /* CMP abort packet -- the other side didn't like us */
      ps->device_close(ps);

#ifdef DEBUG
      fprintf(stderr,"Received CMP abort from client\n");
#endif
      errno = -EIO;
      return -1;
    }
  }
  ps->connected = 1;
  
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
  
  if (addr->pi_device[0] == ':') {
    memmove(addr->pi_device, addr->pi_device+1, strlen(addr->pi_device));
    if (pi_net_device_open(addr->pi_device, ps) == -1) {
      return -1;     /* errno already set */
    }
  }
  else {
    if (pi_serial_device_open(addr->pi_device, ps) == -1) {
      return -1;     /* errno already set */
    }
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
  struct pi_socket *ps, *accept;
  struct cmp c;

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }
  
  accept = malloc(sizeof(struct pi_socket));
  memcpy(accept, ps, sizeof(struct pi_socket));

  if(accept->type == PI_SOCK_STREAM) {
    accept->device_read(accept, 200);
    if(cmp_rx(accept, &c) < 0)
      goto fail; /* Failed to establish connection, errno already set */
    
    if ((c.version & 0xFF00) == 0x0100) {
      if((unsigned long) accept->establishrate > c.baudrate) {
#ifdef DEBUG
        fprintf(stderr,"Rate %d too high, dropping to %ld\n",ps->establishrate,c.baudrate);
#endif
        accept->establishrate = c.baudrate;
      }
      accept->rate = accept->establishrate;
      accept->version = c.version;
      if(cmp_init(accept, accept->rate)<0)
        goto fail;
      pi_socket_flush(accept);
      if(accept->rate != 9600) {
        accept->device_changebaud(accept);
      }
      accept->connected = 1;
      accept->dlprecord = 0;
    }else {
      cmp_abort(ps, 0x80); /* 0x80 means the comm version wasn't compatible*/

      fprintf(stderr, "pi_socket connection failed due to comm version mismatch\n");
      fprintf(stderr, " (expected version 01xx, got %4.4X)\n", c.version);

      errno = ECONNREFUSED;
      goto fail;
    }
  } else {
    accept->connected = 1;
  }
  
  accept->sd = dup(ps->sd);

  if (!psl) psl = accept;
  else {
    struct pi_socket *p;
    for (p = psl; p->next; p=p->next);
      p->next = accept;
  }
  
  accept->mac->ref++; /* Keep mac around even if the bound socket is closed */
  accept->initiator = 0; /* We accepted the link, we did not initiate it */
  
  return accept->sd;
fail:
  free(accept);
  return -1;
}

/* Send msg on a connected socket */

int pi_send(int pi_sd, void *msg, int len, unsigned int flags)
{
  struct pi_socket *ps;

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }
  
  if (interval)
    alarm(interval);

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
  
  if(ps->type == PI_SOCK_STREAM) {
    struct padp pd;
    int ret;
    if (ps->busy || !ps->connected)
      return -1;
    pd.type = padTickle;
    pd.flags = 0x00;
    pd.size = 0x00;
    ret = padp_tx(ps, (void *)&pd, 0, padTickle);
    pi_socket_flush(ps);
    return ret;
  }
  else {
    errno = EOPNOTSUPP;
    return -1;
  }
}

RETSIGTYPE pi_onalarm(int);

int pi_watchdog(int pi_sd, int newinterval)
{
  struct pi_socket *ps;

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }
  
  if(ps->type == PI_SOCK_STREAM) {
    ps->tickle = 1;
    signal(SIGALRM, pi_onalarm);
    interval = newinterval;
    alarm(interval);
    return 0;
  } else {
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
  
  busy++;

  if (ps->type == PI_SOCK_STREAM) {
    if (ps->connected & 1) /* If socket is connected */
      if (!(ps->connected & 2)) /* And it wasn't end-of-synced */
        dlp_EndOfSync(pi_sd, 0);  /* then end sync, with clean status */
  }
  
  if(ps->sd && (ps->sd != ps->mac->fd)) /* If device still has a /dev/null handle */
    close(ps->sd); /* Close /dev/null handle */
    
  if(ps->mac->fd) { /* If device was opened */
    if (ps->connected) {
      pi_socket_flush(ps); /* Flush the device, and set baud rate back to the initial setting */
      ps->rate = 9600; 
      ps->device_changebaud(ps);
    }
    if (--(ps->mac->ref) == 0) { /* If no-one is using the device, close it */
      ps->device_close(ps);
      free(ps->mac);
    }
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
  
  busy--;

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

RETSIGTYPE pi_onalarm(int signo)
{
  struct pi_socket *p, *n;
  
  signal(SIGALRM,pi_onalarm);
  
  if (busy) {
#ifdef DEBUG
    fprintf(stderr, "world is busy. Rescheduling.\n");
#endif
    alarm(1);
  } else
    for (p=psl; p; p=n ) {
      n = p->next;
      if (p->connected) {
#ifdef DEBUG
        fprintf(stderr, "Tickling socket %d\n", p->sd);
#endif
        if (pi_tickle(p->sd)==-1) {
#ifdef DEBUG
          fprintf(stderr, " but socket is busy. Rescheduling.\n");
#endif
          alarm(1);
        } else
          alarm(interval);
      }
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

unsigned int pi_version(int pi_sd)
{
  struct pi_socket *ps;

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }
  
  return ps->version;
}

struct pi_socket *find_pi_socket(int sd)
{
  struct pi_socket *p;

  for (p=psl; p; p=p->next) {
    if (p->sd == sd) return p;
  }

  return 0;
}

int pi_socket_flush(struct pi_socket *ps)
{
  while (ps->device_write(ps));
  return 0;
}
