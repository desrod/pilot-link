/* socket.c:  Berkeley sockets style interface to Pilot SLP/PADP
 *
 * (c) 1996, D. Jeff Dionne.
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <errno.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "pi-socket.h"
#include "padp.h"

static struct pi_socket *psl = (struct pi_socket *)0;
static int pi_next_socket = 3;            /* FIXME: This should be a real fd */

static unsigned char wakeup[] = {1, 0, 1, 0, 0, 0, 0, 0, 0xe1, 0};

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
  char buf[5];

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }

  if (pi_device_open(addr->device, ps) == -1) {
    return -1;     /* errno already set */
  }

  ps->raddr = *addr;
  ps->laddr = *addr;     /* FIXME: This is not always true! */

  /* Now we send some magic to the other end.... */

  padp_tx(ps,wakeup,sizeof(wakeup),padWake);
  pi_socket_read(ps);
  padp_rx(ps, buf, 0);

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
  ps->raddr = *addr;     /* FIXME: This is not always true! */

  return 0;
}

/* Wait for an incoming connection */

int pi_listen(int pi_sd, int backlog)
{
  struct pi_socket *ps;
  char buf[200];

  if (!(ps = find_pi_socket(pi_sd))) {
    errno = ESRCH;
    return -1;
  }

  pi_socket_read(ps);
  padp_rx(ps,buf,200);
  pi_socket_flush(ps);
}

/* Accept an incoming connection */

int pi_accept(int pi_sd, struct pi_sockaddr *addr, int *addrlen)
{
  errno = ENOSYS;
  return -1;
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
  errno = ENOSYS;
  return -1;
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

