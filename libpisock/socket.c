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
#include "pi-inetserial.h"
#include "pi-inet.h"
#include "pi-padp.h"
#include "pi-cmp.h"
#include "pi-dlp.h"
#include "pi-syspkt.h"

static struct pi_socket *psl = (struct pi_socket *)0;

void installexit(void);

extern int dlp_trace;

static RETSIGTYPE pi_serial_onalarm(int signo);

/* Automated tickling interval */
static int interval=0;
static int busy=0;


static int default_socket_connect(struct pi_socket * ps, struct sockaddr * addr, int flag)
{
	errno = ENOSYS;
	return -1;
}
static int default_socket_listen(struct pi_socket * ps, int flag)
{
	errno = ENOSYS;
	return -1;
}
static int default_socket_accept(struct pi_socket * ps, struct sockaddr * addr, int *flag)
{
	errno = ENOSYS;
	return -1;
}
static int default_socket_close(struct pi_socket * ps)
{
	return 0;
}
static int default_socket_tickle(struct pi_socket * ps)
{
	errno = ENOSYS;
	return -1;
}
static int default_socket_bind(struct pi_socket * ps, struct sockaddr * addr, int flag)
{
	errno = ENOSYS;
	return -1;
}
static int default_socket_send(struct pi_socket * ps, void * buf, int len, unsigned int flags)
{
	errno = ENOSYS;
	return -1;
}
static int default_socket_recv(struct pi_socket * ps, void * buf, int len, unsigned int flags)
{
	errno = ENOSYS;
	return -1;
}


/* Create a local connection endpoint */

int pi_socket(int domain, int type, int protocol)
{
  struct pi_socket *ps;
  
  if (protocol == 0) {
    if (type == PI_SOCK_STREAM)
      protocol = PI_PF_PADP;
    else if (type == PI_SOCK_RAW)
      protocol = PI_PF_SLP;
  }

  if (((domain != PI_AF_SLP) && 
       (domain != AF_INET)) ||
      ((type  != PI_SOCK_STREAM) &&
      (type   != PI_SOCK_RAW)) ||
      ((protocol != PI_PF_PADP) &&
       (protocol != PI_PF_SLP))) {  /* FIXME:  Need to support more */
    errno = EINVAL;
    return -1;
  }

  ps = calloc(sizeof(struct pi_socket), 1);

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
  ps->xid = 0xff;
  ps->initiator = 0;
  ps->minorversion = 0;
  ps->majorversion = 0;
  ps->version = 0;
  ps->dlprecord = 0;
  ps->busy = 0;
  
  ps->socket_connect = default_socket_connect;
  ps->socket_listen = default_socket_listen;
  ps->socket_accept = default_socket_accept;
  ps->socket_close = default_socket_close;
  ps->socket_tickle = default_socket_tickle;
  ps->socket_bind = default_socket_bind;
  ps->socket_send = default_socket_send;
  ps->socket_recv = default_socket_recv;
  

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

  installexit();

  pi_socket_recognize(ps);
  
  return ps->sd;
}

void pi_socket_recognize(struct pi_socket *ps)
{
  struct pi_socket *p;
  if (!psl) psl = ps;
  else {
    for (p = psl; p->next; p=p->next);

    p->next = ps;
  }
}

/* Connect to a remote server */

int pi_connect(int pi_sd, struct sockaddr *addr, int addrlen)
{
  struct pi_socket *ps;
  struct pi_sockaddr * pa = (struct pi_sockaddr*)addr;
  enum {inet, serial} conn;

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }
  
  conn = serial;
  
  if (addr->sa_family == PI_AF_SLP) {
    if (pa->pi_device[0] == '.')
      conn = inet;
    else
      conn = serial;
  }
  else if (addr->sa_family == AF_INET)
    conn = inet;
  else if (addr->sa_family == PI_AF_INETSLP)
    conn = inet;
    
  if (conn == serial)
    return pi_serial_connect(ps, addr, addrlen);
  else if (conn == inet)
    return pi_inet_connect(ps, addr, addrlen);
    
  return -1;
}

/* Bind address to a local socket */

int pi_bind(int pi_sd, struct sockaddr *addr, int addrlen)
{
  struct pi_socket *ps;
  struct pi_sockaddr * pa = (struct pi_sockaddr*)addr;
  enum {inet, serial} conn;

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }

  conn = serial;
  
  if (addr->sa_family == PI_AF_SLP) {
    if (pa->pi_device[0] == '.')
      conn = inet;
    else
      conn = serial;
  }
  else if (addr->sa_family == AF_INET)
    conn = inet;
  else if (addr->sa_family == PI_AF_INETSLP)
    conn = serial;
    
  if (conn == serial)
    return pi_serial_bind(ps, addr, addrlen);
  else if (conn == inet)
    return pi_inet_bind(ps, addr, addrlen);

  return -1;
}

/* Wait for an incoming connection */

int pi_listen(int pi_sd, int backlog)
{
  struct pi_socket *ps;

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }
  
  return ps->socket_listen(ps, backlog);
}

/* Accept an incoming connection */

int pi_accept(int pi_sd, struct sockaddr *addr, int *addrlen)
{
  struct pi_socket *ps;

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }
  
  return ps->socket_accept(ps, addr, addrlen);
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
  
  return ps->socket_send(ps, msg, len, flags);
}

/* Recv msg on a connected socket */

int pi_recv(int pi_sd, void *msg, int len, unsigned int flags)
{
  struct pi_socket *ps;

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }

  return ps->socket_recv(ps, msg, len, flags);
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
  
  return ps->socket_tickle(ps);
}

/* Close a connection, destroy the socket */

int pi_close(int pi_sd)
{
  int result;
  struct pi_socket *ps;
  
  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }

  busy++;
  result = ps->socket_close(ps);
  busy--;

  if (result == 0) {
    if (ps == psl) {
      psl = psl->next;
    } else {
      struct pi_socket * p;
      for (p=psl; p; p=p->next) {
        if (ps == p->next) {
          p->next = p->next->next;
          break;
        }
      }
    }
    free(ps);
  }
  
  return result;
}

/* Install an atexit handler that closes open sockets */

void pi_onexit(void)
{
  struct pi_socket *p, *n;
  
  for (p=psl; p; p=n ) {
    n = p->next;
    if (p->socket_close) {
      pi_close(p->sd);
    }
  }
  
}

void installexit(void)
{
  static int installedexit = 0;
  
  if (!installedexit)
    atexit(pi_onexit);
    
  installedexit = 1;
}

/* Get the local address for a socket */

int pi_getsockname(int pi_sd, struct sockaddr * addr, int * namelen)
{
  struct pi_socket *ps;

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }
  
  if (*namelen > ps->laddrlen)
    *namelen = ps->laddrlen;
  memcpy(addr, &ps->laddr, *namelen);
    
  return 0;
}

/* Get the remote address for a socket */

int pi_getsockpeer(int pi_sd, struct sockaddr * addr, int * namelen)
{
  struct pi_socket *ps;

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }
  
  if (*namelen > ps->raddrlen)
    *namelen = ps->raddrlen;
  memcpy(addr, &ps->raddr, *namelen);
    
  return 0;
}

int pi_version(int pi_sd)
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


int pi_watchdog(int pi_sd, int newinterval)
{
  struct pi_socket *ps;

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }
  
  ps->tickle = 1;
  signal(SIGALRM, pi_serial_onalarm);
  interval = newinterval;
  alarm(interval);
  return 0;
}

static RETSIGTYPE pi_serial_onalarm(int signo)
{
  struct pi_socket *p, *n;
  
  signal(SIGALRM,pi_serial_onalarm);
  
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
