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
#include <sys/time.h>
#include <stdio.h>
#include "pi-socket.h"
#include "pi-serial.h"

#ifdef bsdi
#include <sys/ioctl_compat.h>
#endif

#if 1
#define POSIX
#endif

#if !defined(linux) && !defined(cfmakeraw)
#define cfmakeraw(ptr) (ptr)->c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR\
					 |IGNCR|ICRNL|IXON);\
                       (ptr)->c_oflag &= ~OPOST;\
                       (ptr)->c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);\
                       (ptr)->c_cflag &= ~(CSIZE|PARENB);\
                       (ptr)->c_cflag |= CS8
#endif

#ifdef OS2
#define INCL_BASE
#define INCL_DOSFILEMGR    /* File System values */
#define INCL_DOSDEVIOCTL   /* DosDevIOCtl values */
#define INCL_DOSDEVICES    /* DosDevice   values */
#include <os2.h>
#endif /* OS2 */

#ifndef OS2

#ifndef N_TTY
#ifndef NTTYDISC
#error "Can't define line discipline"
#else
#define N_TTY NTTYDISC
#endif /* NTTYDISC */
#endif /* N_TTY */

static int calcrate(int baudrate) {
  if(baudrate == 9600)
    return B9600;
#ifdef B19200
  else if(baudrate == 19200)
    return B19200;
#endif
#ifdef B38400
  else if(baudrate == 38400)
    return B38400;
#endif
#ifdef B57600
  else if(baudrate == 57600)
    return B57600;
#endif
#ifdef B115200
  else if(baudrate == 115200)
    return B115200;
#endif
  else {
    printf("Unable to set baud rate %d\n", baudrate);
    abort(); /* invalid baud rate */
  }
}

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

  if (ps->protocol == PF_PADP)
    tcn.c_cflag = CREAD | CLOCAL | calcrate(9600) | CS8 ;
  else
    tcn.c_cflag = CREAD | CLOCAL | calcrate(57600) | CS8 ;

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

int pi_device_changebaud(struct pi_socket *ps)
{
  int i;

  struct termios tcn;

  /* Set the tty to the new speed */
#ifdef POSIX
  tcgetattr(ps->mac.fd,&tcn);
#else
  ioctl(ps->mac.fd,TCGETS,&tcn);
#endif

  tcn.c_cflag &= ~CBAUD;

  tcn.c_cflag |= calcrate(ps->rate);

#ifdef POSIX
  tcsetattr(ps->mac.fd,TCSADRAIN,&tcn);
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
  int result;
#ifdef linux
  /* Something isn't getting flushed somewhere. If this sleep is removed,
     the Pilot never gets the final padp Ack.*/
  sleep(1);
#endif
#ifdef POSIX
  tcsetattr(ps->mac.fd,TCSADRAIN, &ps->tco);
#else
  ioctl(ps->mac.fd,TCSETSW,&ps->tco);
#endif
  result = close(ps->mac.fd);
  ps->mac.fd = 0;
  return result;
}
#endif /* not OS2 */

#ifdef OS2
int pi_device_open(char *tty, struct pi_socket *ps)
{
  int rc;
  HFILE fd;
  unsigned long action, baudrate=9600;
  int filesize=0;
  int param_length;

  /* open the device */
  rc=DosOpen(tty, /* the device */
	     &fd, /* the file descriptor returned */
	     &action, /* the action taken */
	     filesize, /* the size of the file */
	     FILE_NORMAL, /* file permissions mode, not the same as UNIX */
	     OPEN_ACTION_OPEN_IF_EXISTS, /* file open action */
	     OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_READWRITE, /* open mode */
	     0); /* extended attributes */
  if (rc)
    {
      switch (rc)
	{
	case    2:         /* ERROR_FILE_NOT_FOUND  */
	  errno=ENOENT;
	  break;
	case    3:         /* ERROR_PATH_NOT_FOUND  */
	  errno=ENOTDIR;
	  break;
	case    4:         /* ERROR_TOO_MANY_OPEN_FILES  */
	  errno=EMFILE;
	  break;
	case    5:         /* ERROR_ACCESS_DENIED  */
	  errno=EACCES;
	  break;
	case    32:        /* ERROR_SHARING_VIOLATION  */
	  errno=EBUSY;
	  break;
	case    82:        /* ERROR_CANNOT_MAKE  */
	  errno=EEXIST;
	  break;
	case    99:        /* ERROR_DEVICE_IN_USE  */
	  errno=EBUSY;
	  break;
	case    112:       /* ERROR_DISK_FULL  */
	  errno=ENOSPC;
	  break;
	case    87:        /* ERROR_INVALID_PARAMETER  */
	  errno=EINVAL;
	  break;
	default:
	  errno=-ENOMSG;
	  break;
	}
      return(-1);
    }
  /* set it to 9600 baud */
  ps->mac.fd=fd;
  if (ps->protocol == PF_PADP)
    pi_device_changebaud(ps,9600);
  else
    pi_device_changebaud(ps,57600);
  return(fd);  
}

int pi_device_changebaud(struct pi_socket *ps, int baudrate)
{
  int param_length;
  int rc;

  /* set it to baudrate */
  param_length=sizeof(baudrate);
  rc=DosDevIOCtl(ps->mac.fd, /* file decsriptor */
		 IOCTL_ASYNC, /*asyncronous change */
		 ASYNC_SETBAUDRATE, /* set the baudrate */
		 &baudrate, /* pointer the the baudrate */
		 param_length, /* length of the previous parameter */
		 (unsigned long *)&param_length, /* max length of data ret */
		 NULL, /* data to be sent */
		 0, /* length of data */
		 NULL); /* length of data returned */

  if (rc)
    {
      switch (rc)
	{
	case    1:         /* ERROR_INVALID_FUNCTION */
	  errno=ENOTTY;
	  break;
	case    6:         /* ERROR_INVALID_HANDLE */
	  errno=EBADF;
	  break;
	case    87:        /* ERROR_INVALID_PARAMETER */
	  errno=EINVAL;
	  break;
	default:
	  errno=-ENOMSG;
	  break;
	}
      return(-1);
    }
#ifdef OS2
  /* this pause seems necessary under OS2 to let the serial
     port realign itself */
  sleep(1);
#endif
#ifdef OS2_DEBUG
  fprintf(stderr,"set baudrate to %d\n",baudrate);
#endif
  return(0);
}

int pi_device_close(struct pi_socket *ps)
{
  DosClose(ps->mac.fd);
}
#endif /* OS2 */


int pi_socket_send(struct pi_socket *ps)
{
  struct pi_skb *skb;
#ifdef OS2
  int rc,nwrote;
#endif

  if (ps->txq) {

    skb = ps->txq;
    ps->txq = skb->next;

#ifdef OS2
    rc=DosWrite(ps->mac.fd,skb->data,skb->len,(unsigned long *)&nwrote);
#else
    write(ps->mac.fd,skb->data,skb->len);
#endif
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

int pi_socket_read(struct pi_socket *ps, int timeout)
{
  int r;
  char *buf;
#ifdef OS2
  int rc;
#else
  fd_set ready,ready2;
  struct timeval t;
  
  FD_ZERO(&ready);
  FD_SET(ps->mac.fd, &ready);
  t.tv_sec = timeout;
  t.tv_usec = 0;
#endif

  /* FIXME: if timeout == 0, wait forever for packet, otherwise wait till
     timeout seconds -- NOT IMPLEMENTED FOR OS/2! */

  pi_socket_flush(ps);              /* We likely want to be in sync with tx */
  if (!ps->mac.expect) slp_rx(ps);  /* let SLP know we want a packet */

  while (ps->mac.expect) {
    buf = ps->mac.buf;

    while (ps->mac.expect) {
#ifdef OS2
      rc=DosRead(ps->mac.fd,buf,ps->mac.expect,(unsigned long *)&r);
#else
      ready2 = ready;
      select(ps->mac.fd+1,&ready2,0,0,&t);
      /* If data is available in time, read it */
      if(FD_ISSET(ps->mac.fd,&ready2))
        r = read(ps->mac.fd, buf, ps->mac.expect);
      else {
        /* otherwise throw out any current packet and return */
#ifdef DEBUG
        fprintf(stderr, "Serial RX: timeout\n");
#endif
        ps->mac.state = ps->mac.expect = 1;
        ps->mac.buf = ps->mac.rxb->data;
        ps->rx_errors++;
        return 0;
      }
#endif
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
