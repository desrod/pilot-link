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
#ifdef bsdi
  tcgetattr(ps->mac.fd,&tcn);
#else
  ioctl(ps->mac.fd,TCGETS,&tcn);
#endif

  ps->tco = tcn;

  tcn.c_oflag = 0;
  tcn.c_iflag = IGNBRK | IGNPAR;

  tcn.c_cflag = CREAD | CLOCAL | B9600 | CS8 ;

  tcn.c_lflag = NOFLSH;

#ifdef bsdi
  cfmakeraw(&tcn);
#else
  tcn.c_line = N_TTY;
#endif

  for(i=0;i<16;i++) tcn.c_cc[i]=0;

  tcn.c_cc[VMIN] = 1;
  tcn.c_cc[VTIME] = 0;

#ifdef bsdi
  tcsetattr(ps->mac.fd,TCSANOW,&tcn);
#else
  ioctl(ps->mac.fd,TCSETSW,&tcn);
#endif

  return ps->mac.fd;
}

int pi_device_close(struct pi_socket *ps)
{
#ifdef bsdi
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
      ps->rx_bytes += r;
      buf += r;
      ps->mac.expect -= r;
    }
    slp_rx(ps);
  }
  return 0;
}
