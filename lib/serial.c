/* serial.c:  tty line interface code for Pilot networking
 *
 * (c) 1996, D. Jeff Dionne.
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "pi-socket.h"

#ifdef bsdi
#include <sys/ioctl_compat.h>
#endif

#if defined(linux) || defined(bsdi)
#define POSIX
#endif

#ifndef N_TTY
#ifndef NTTYDISC
#error "Can't define line discipline"
#else
#define N_TTY NTTYDISC
#endif /* NTTYDISC */
#endif /* N_TTY */

int pi_device_open(char *tty, struct pi_socket *ps)
{
  int i;
  struct termios tcn;

  if ((ps->mac.fd = open(tty, O_RDWR )) == -1) {
    return -1;     /* errno already set */
  }

  if (!isatty(ps->mac.fd)) {
    close(ps->mac.fd);
    errno = EINVAL;
    return -1;
  }

  /* Set the tty to raw and to the correct speed */
#ifdef POSIX
  tcgetattr(ps->mac.fd,&tcn);
#else
  ioctl(ps->mac.fd,TCGETS,&tcn);
#endif

  ps->tco = tcn;

  tcn.c_oflag = 0;
  tcn.c_iflag = IGNBRK | IGNPAR;

  /* Always start a connection at 9600 baud */
  tcn.c_cflag = CREAD | CLOCAL | B9600 | CS8 ;

  tcn.c_lflag = NOFLSH;

#ifdef POSIX
  cfmakeraw(&tcn);
#else
  tcn.c_line = N_TTY;
#endif

  for(i=0;i<16;i++) tcn.c_cc[i]=0;

  tcn.c_cc[VMIN] = 1;
  tcn.c_cc[VTIME] = 0;
  
#ifdef linux
  i=TIOCM_RTS|TIOCM_DTR;
  i=ioctl(ps->mac.fd,TIOCMSET,&i);
#endif                  

#ifdef POSIX
  tcsetattr(ps->mac.fd,TCSANOW,&tcn);
#else
  ioctl(ps->mac.fd,TCSETSW,&tcn);
#endif

  return ps->mac.fd;
}

int pi_device_changebaud(struct pi_socket *ps, int baudrate)
{
  int i;
  struct termios tcn;

  /* Set the tty to the new speed */
#ifdef POSIX
  tcgetattr(ps->mac.fd,&tcn);
#else
  ioctl(ps->mac.fd,TCGETS,&tcn);
#endif

  ps->tco = tcn;

  tcn.c_cflag &= ~CBAUD;

  if(baudrate == 9600)
    baudrate = B9600;
#ifdef B19200
  else if(baudrate == 19200)
    baudrate = B19200;
#endif
#ifdef B38400
  else if(baudrate == 38400)
    baudrate = B38400;
#endif
#ifdef B57600
  else if(baudrate == 57600)
    baudrate = B57600;
#endif
#ifdef B115200
  else if(baudrate == 115200)
    baudrate = B115200;
#endif
  else
    abort(); /* invalid baud rate */
                                                     
  tcn.c_cflag |= baudrate;

#ifdef POSIX
  tcsetattr(ps->mac.fd,TCSANOW,&tcn);
#else
  ioctl(ps->mac.fd,TCSETSW,&tcn);
#endif


#ifdef linux
  /* this pause seems necessary under Linux to let the serial
     port realign itself */
  sleep(1);
#endif

  return 0;
}

int pi_device_close(struct pi_socket *ps)
{
#ifdef POSIX
  tcsetattr(ps->mac.fd,TCSANOW, &ps->tco);
#else
  ioctl(ps->mac.fd,TCSETSW,&ps->tco);
#endif
  return close(ps->mac.fd);
}

int pi_socket_send(struct pi_socket *ps)
{
  struct pi_skb *skb;

  if (ps->txq) {

    skb = ps->txq;
    ps->txq = skb->next;

    write(ps->mac.fd,skb->data,skb->len);
#ifdef DEBUG
    write(6,skb->data,skb->len);
#endif
    ps->tx_bytes += skb->len;
    free(skb);

    return 1;
  }
  return 0;
}

int pi_socket_flush(struct pi_socket *ps)
{
  while (pi_socket_send(ps));
  return 0;
}

int pi_socket_read(struct pi_socket *ps)
{
  int r;
  char *buf;

  if (!ps->mac.expect) slp_rx(ps);  /* let SLP know we want a packet */
  pi_socket_flush(ps);              /* We likely want to be in sync with tx */

  while (ps->mac.expect) {
    buf = ps->mac.buf;

    while (ps->mac.expect) {
      r = read(ps->mac.fd, buf, ps->mac.expect);
#ifdef DEBUG
      write(7, buf, r);
#endif
      ps->rx_bytes += r;
      buf += r;
      ps->mac.expect -= r;
    }
    slp_rx(ps);
  }
  return 0;
}
