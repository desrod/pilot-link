/* net.c:
 *
 * (c) 1996, D. Jeff Dionne.
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-net.h"
#include "pi-slp.h"

int pi_net_device_open(char *host, struct pi_socket *ps)
{
  struct sockaddr_in serv_addr;
  ps->mac->fd = socket(AF_INET, SOCK_STREAM, 0);
  
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(host);
  serv_addr.sin_port = htons(4386);

  connect(ps->mac->fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

  if (ps->sd) {
    int orig = ps->mac->fd;
#ifdef HAVE_DUP2
    ps->mac->fd = dup2(ps->mac->fd, ps->sd);
#else
#ifdef F_DUPFD
    close(ps->sd);
    ps->mac->fd = fcntl(ps->mac->fd, F_DUPFD, ps->sd);
#else
    close(ps->sd);
    ps->mac->fd = dup(ps->mac->fd); /* Unreliable */
#endif
#endif
    if (ps->mac->fd != orig)
      close(orig);
  }

#ifndef NO_SERIAL_TRACE
  if(ps->debuglog) {
    ps->debugfd = open(ps->debuglog,O_WRONLY|O_CREAT|O_APPEND,0666);
    /* This sequence is magic used by my trace analyzer - kja */
    write(ps->debugfd, "\0\1\0\0\0\0\0\0\0\0", 10);
  }
#endif

  ps->device_read = pi_net_device_read;
  ps->device_write = pi_net_device_write;
  ps->device_close = pi_net_device_close;
  ps->device_changebaud = pi_net_device_changebaud;

  return ps->mac->fd;
}

int pi_net_device_changebaud(struct pi_socket *ps)
{
  char buffer[20];
  
  set_short(buffer, 1);
  set_short(buffer+2, 4);
  set_long(buffer+4, ps->rate);
  
  write(ps->mac->fd, buffer, 8);

  return 0;
}

int pi_net_device_close(struct pi_socket *ps)
{
  int result;
  char buffer[4];
  
  set_short(buffer, 2);
  set_short(buffer+2, 0);

  write(ps->mac->fd, buffer, 4);

  result = close(ps->mac->fd);
  ps->mac->fd = 0;

#ifndef NO_SERIAL_TRACE
  if (ps->debugfd)
    close(ps->debugfd);  
#endif

  return result;
}

int pi_net_device_write(struct pi_socket *ps)
{
  struct pi_skb *skb;
  int nwrote, len;
  char buffer[4];
#ifndef NO_SERIAL_TRACE
  int i;
#endif
#ifdef OS2
  int rc;
#endif

  if (ps->txq) {
  
    ps->busy++;

    skb = ps->txq;
    ps->txq = skb->next;
    
    set_short(buffer, 0);
    set_short(buffer+2, skb->len);
    write(ps->mac->fd, buffer, 4);
    
    len = 0;
    while (len<skb->len) {
      nwrote = 0;
      nwrote=write(ps->mac->fd,skb->data,skb->len);
      if (nwrote<=0)
        break; /* transmission failure */
      len += nwrote;
    }
#ifndef NO_SERIAL_TRACE
    if (ps->debuglog)
      for (i=0;i<skb->len;i++) {
        write(ps->debugfd, "2", 1);
        write(ps->debugfd, skb->data+i, 1);
      }
#endif
    ps->tx_bytes += skb->len;
    free(skb);

    ps->busy--;
    
    return 1;
  }
  return 0;
}

int pi_net_device_read(struct pi_socket *ps, int timeout)
{
  int r;
  unsigned char *buf;
#ifndef NO_SERIAL_TRACE
  int i;
#endif
  fd_set ready,ready2;
  struct timeval t;
  
  FD_ZERO(&ready);
  FD_SET(ps->mac->fd, &ready);

  pi_socket_flush(ps);              /* We likely want to be in sync with tx */
  
  if (!ps->mac->expect) slp_rx(ps);  /* let SLP know we want a packet */

  while (ps->mac->expect) {
    buf = ps->mac->buf;

    while (ps->mac->expect) {
      ready2 = ready;
      t.tv_sec = timeout/10;
      t.tv_usec = (timeout % 10) * 100000;
      select(ps->mac->fd+1,&ready2,0,0,&t);
      /* If data is available in time, read it */
      if(FD_ISSET(ps->mac->fd,&ready2))
        r = read(ps->mac->fd, buf, ps->mac->expect);
      else
      {
        /* otherwise throw out any current packet and return */
#ifdef DEBUG
        fprintf(stderr, "Serial RX: timeout\n");
#endif
        ps->mac->state = ps->mac->expect = 1;
        ps->mac->buf = ps->mac->rxb->data;
        ps->rx_errors++;
        return 0;
      }
#ifndef NO_SERIAL_TRACE
      if (ps->debuglog)
        for (i=0;i<r;i++) {
          write(ps->debugfd, "1", 1);
          write(ps->debugfd, buf+i, 1);
        }
#endif
      ps->rx_bytes += r;
      buf += r;
      ps->mac->expect -= r;
    }
    slp_rx(ps);
  }
  return 0;
}

