/* serial.c: Interface layer to serial HotSync connections
 *
 * Copyright (c) 1996, 1997, D. Jeff Dionne & Kenneth Albanowski
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
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
#include "pi-padp.h"
#include "pi-cmp.h"
#include "pi-dlp.h"
#include "pi-syspkt.h"

#ifdef OS2
#include <sys/select.h>
#endif

static int pi_serial_listen(struct pi_socket *ps, int backlog);
static int pi_serial_accept(struct pi_socket *ps, struct sockaddr *addr, int *addrlen);
static int pi_serial_send(struct pi_socket *ps, void *msg, int len, unsigned int flags);
static int pi_serial_recv(struct pi_socket *ps, void *msg, int len, unsigned int flags);
static int pi_serial_tickle(struct pi_socket *ps);
static int pi_serial_close(struct pi_socket * ps);

extern int dlp_trace;

int pi_serial_connect(struct pi_socket *ps, struct sockaddr *addr, int addrlen)
{
  struct cmp c;
  struct pi_sockaddr * pa = (struct pi_sockaddr*)addr;

  if(ps->type == PI_SOCK_STREAM) {
    ps->establishrate = 9600; /* Default PADP connection rate */
    if (getenv("PILOTRATE"))
    	ps->establishrate = atoi(getenv("PILOTRATE"));
    ps->rate = 9600; /* Mandatory CMP connection rate */
  } else if(ps->type == PI_SOCK_RAW) {
    ps->establishrate = ps->rate = 57600; /* Mandatory SysPkt connection rate */
  }

  if ((addr->sa_family == PI_AF_INETSLP) || 
      ((addr->sa_family == PI_AF_SLP) && (pa->pi_device[0] == ':'))) {
    if (pi_inetserial_open(ps, addr, addrlen) == -1) {
      return -1;     /* errno already set */
    }
  }
  else {
    if (pi_serial_open(ps, pa, addrlen) == -1) {
      return -1;     /* errno already set */
    }
  }

  ps->raddr = malloc(addrlen);
  memcpy(ps->raddr, addr, addrlen);
  ps->raddrlen = addrlen;
  ps->laddr = malloc(addrlen);
  memcpy(ps->laddr, addr, addrlen);
  ps->laddrlen = addrlen;
   
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
        ps->serial_changebaud(ps);
      }

    } else if(c.type == 3) {
      /* CMP abort packet -- the other side didn't like us */
      ps->serial_close(ps);

#ifdef DEBUG
      fprintf(stderr,"Received CMP abort from client\n");
#endif
      errno = -EIO;
      return -1;
    }
  }
  ps->connected = 1;

  ps->initiator = 1; /* We initiated the link */
  
  ps->socket_listen = pi_serial_listen;
  ps->socket_accept = pi_serial_accept;
  ps->socket_close = pi_serial_close;
  ps->socket_send = pi_serial_send;
  ps->socket_recv = pi_serial_recv;
  ps->socket_tickle = pi_serial_tickle;
  
  return 0;
}

/* Bind address to a local socket */

int pi_serial_bind(struct pi_socket *ps, struct sockaddr *addr, int addrlen)
{
  struct pi_sockaddr * pa = (struct pi_sockaddr*)addr;
  if(ps->type == PI_SOCK_STREAM) {
    ps->establishrate = 9600; /* Default PADP connection rate */
    if (getenv("PILOTRATE"))
    	ps->establishrate = atoi(getenv("PILOTRATE"));
    ps->rate = 9600; /* Mandatory CMP conncetion rate */
  } else if(ps->type == PI_SOCK_RAW) {
    ps->establishrate = ps->rate = 57600; /* Mandatory SysPkt connection rate */
  }

  if ((addr->sa_family == PI_AF_INETSLP) || 
      ((addr->sa_family == PI_AF_SLP) && (pa->pi_device[0] == ':'))) {
    if (pi_inetserial_open(ps, addr, addrlen) == -1) {
      return -1;     /* errno already set */
    }
  }
  else {
    if (pi_serial_open(ps, pa, addrlen) == -1) {
      return -1;     /* errno already set */
    }
  }
  
  ps->raddr = malloc(addrlen);
  memcpy(ps->raddr, addr, addrlen);
  ps->raddrlen = addrlen;
  ps->laddr = malloc(addrlen);
  memcpy(ps->laddr, addr, addrlen);
  ps->laddrlen = addrlen;

  ps->socket_listen = pi_serial_listen;
  ps->socket_accept = pi_serial_accept;
  ps->socket_close = pi_serial_close;
  ps->socket_send = pi_serial_send;
  ps->socket_recv = pi_serial_recv;
  ps->socket_tickle = pi_serial_tickle;

  return 0;
}

/* Wait for an incoming connection */

static int pi_serial_listen(struct pi_socket *ps, int backlog)
{
  return 0;
}

/* Accept an incoming connection */

static int pi_serial_accept(struct pi_socket *ps, struct sockaddr *addr, int *addrlen)
{
  struct pi_socket *accept;
  struct cmp c;

  accept = malloc(sizeof(struct pi_socket));
  memcpy(accept, ps, sizeof(struct pi_socket));

  if(accept->type == PI_SOCK_STREAM) {
    accept->serial_read(accept, 200);
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
      pi_serial_flush(accept);
      if(accept->rate != 9600) {
        accept->serial_changebaud(accept);
      } else {
        /* Apparently the device reconfigures its serial port even if the
           baud rate is unchanged, so we'll need to pause a little so that
           the next transmitted packet won't be lost */
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 50000;
        select(0, 0, 0, 0, &tv);
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

  pi_socket_recognize(accept);
  
  accept->laddr = malloc(ps->laddrlen);
  accept->raddr = malloc(ps->raddrlen);
  memcpy(accept->laddr, ps->laddr, ps->laddrlen);
  memcpy(accept->raddr, ps->raddr, ps->raddrlen);
  
  accept->mac->ref++; /* Keep mac around even if the bound socket is closed */
  accept->initiator = 0; /* We accepted the link, we did not initiate it */
  
  return accept->sd;
fail:
  free(accept);
  return -1;
}

/* Send msg on a connected socket */

static int pi_serial_send(struct pi_socket *ps, void *msg, int len, unsigned int flags)
{
  if (ps->type == PI_SOCK_STREAM)
    return padp_tx(ps, msg, len, padData);
  else
    return syspkt_tx(ps, msg, len);
}

/* Recv msg on a connected socket */

static int pi_serial_recv(struct pi_socket *ps, void *msg, int len, unsigned int flags)
{
  if (ps->type == PI_SOCK_STREAM)
    return padp_rx(ps, msg, len);
  else
    return syspkt_rx(ps, msg, len);
}

static int pi_serial_tickle(struct pi_socket *ps)
{
  if(ps->type == PI_SOCK_STREAM) {
    struct padp pd;
    int ret;
    if (ps->busy || !ps->connected)
      return -1;
    pd.type = padTickle;
    pd.flags = 0x00;
    pd.size = 0x00;
    ret = padp_tx(ps, (void *)&pd, 0, padTickle);
    pi_serial_flush(ps);
    return ret;
  }
  else {
    errno = EOPNOTSUPP;
    return -1;
  }
}

/* Close a connection, destroy the socket */

static int pi_serial_close(struct pi_socket * ps)
{
  if (ps->type == PI_SOCK_STREAM) {
    if (ps->connected & 1) /* If socket is connected */
      if (!(ps->connected & 2)) /* And it wasn't end-of-synced */
        dlp_EndOfSync(ps->sd, 0);  /* then end sync, with clean status */
  }
  
  if(ps->sd && (ps->sd != ps->mac->fd)) /* If device still has a /dev/null handle */
    close(ps->sd); /* Close /dev/null handle */
    
  if(ps->mac->fd) { /* If device was opened */
    if (ps->connected) {
      pi_serial_flush(ps); /* Flush the device, and set baud rate back to the initial setting */
      ps->rate = 9600; 
      ps->serial_changebaud(ps);
    }
    if (--(ps->mac->ref) == 0) { /* If no-one is using the device, close it */
      ps->serial_close(ps);
      free(ps->mac);
    }
  }
  
  if(ps->laddr)
    free(ps->laddr);
  if(ps->raddr)
    free(ps->raddr);

  return 0;
}

int pi_serial_flush(struct pi_socket *ps)
{
  while(ps->serial_write(ps)) ;
  return 0;
}
