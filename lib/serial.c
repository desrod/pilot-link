/* serial.c:  tty line interface code for Pilot networking
 *
 * (c) 1996, D. Jeff Dionne.
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include "pi-socket.h"
#include "pi-serial.h"
#include "slp.h"

#ifdef HAVE_SYS_IOCTL_COMPAT_H
#include <sys/ioctl_compat.h>
#endif

#ifndef HAVE_CFMAKERAW
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

#ifndef HAVE_CFSETSPEED
#if defined(HAVE_CFSETISPEED) && defined(HAVE_CFSETOSPEED)
#define cfsetspeed(t,speed) \
  (cfsetispeed(t,speed) || cfsetospeed(t,speed))
#else
static int cfsetspeed(struct termios * t,int speed) {
#ifdef HAVE_TERMIOS_CSPEED
  t->c_ispeed=speed;
  t->c_ospeed=speed;
#else
  t->c_cflag|=speed;
#endif
  return 0;
}
#endif
#endif

static int calcrate(int baudrate) {
#ifdef B300
  if(baudrate == 300) return B300;
#endif
#ifdef B1200
  if(baudrate == 1200) return B1200;
#endif
#ifdef B2400
  if(baudrate == 2400) return B2400;
#endif
#ifdef B4800
  if(baudrate == 4800) return B4800;
#endif
#ifdef B9600
  if(baudrate == 9600) return B9600;
#endif
#ifdef B19200
  else if(baudrate == 19200) return B19200;
#endif
#ifdef B38400
  else if(baudrate == 38400) return B38400;
#endif
#ifdef B57600
  else if(baudrate == 57600) return B57600;
#endif
#ifdef B115200
  else if(baudrate == 115200) return B115200;
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
  tcgetattr(ps->mac.fd,&tcn);

  ps->tco = tcn;

  tcn.c_oflag = 0;
  tcn.c_iflag = IGNBRK | IGNPAR;

  tcn.c_cflag = CREAD | CLOCAL | CS8;
  
  (void)cfsetspeed(&tcn, calcrate(ps->rate));

  tcn.c_lflag = NOFLSH;

  cfmakeraw(&tcn);

  for(i=0;i<16;i++) tcn.c_cc[i]=0;

  tcn.c_cc[VMIN] = 1;
  tcn.c_cc[VTIME] = 0;
  
#ifdef linux
  i=TIOCM_RTS|TIOCM_DTR;
  i=ioctl(ps->mac.fd,TIOCMSET,&i);
#endif                  

  tcsetattr(ps->mac.fd,TCSANOW,&tcn);

  return ps->mac.fd;
}

int pi_device_changebaud(struct pi_socket *ps)
{
  struct termios tcn;

  /* Set the tty to the new speed */
  tcgetattr(ps->mac.fd,&tcn);

  tcn.c_cflag =  CREAD | CLOCAL | CS8;
  (void)cfsetspeed(&tcn, calcrate(ps->rate));

  tcsetattr(ps->mac.fd,TCSADRAIN,&tcn);

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

  tcsetattr(ps->mac.fd,TCSADRAIN, &ps->tco);

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
  unsigned long action;
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
  ps->mac.fd=fd;
  pi_device_changebaud(ps);
  pi_socket_set_timeout(ps,-1,60);
  return(fd);  
}

int pi_device_changebaud(struct pi_socket *ps)
{
  int param_length;
  int rc, baudrate;
  
  baudrate = ps->rate;

  param_length=sizeof(baudrate);
  rc=DosDevIOCtl(ps->mac.fd, /* file decsriptor */
		 IOCTL_ASYNC, /*asyncronous change */
		 ASYNC_SETBAUDRATE, /* set the baudrate */
		 &baudrate, /* pointer to the baudrate */
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
  /* this pause seems necessary under OS2 to let the serial
     port realign itself */
  sleep(1);
#ifdef OS2_DEBUG
  fprintf(stderr,"set baudrate to %d\n",baudrate);
#endif
  return(0);
}

int pi_device_close(struct pi_socket *ps)
{
  DosClose(ps->mac.fd);
}


/* 
 * values for read_timeout and write_timeout 
 * 0           = infinite timeout
 * 1 to 65535  = timeout in seconds
 * -1          = dont change timeout
 */
int pi_socket_set_timeout(struct pi_socket *ps, int read_timeout, 
			  int write_timeout)
{
  int param_length, ret_len;
  int rc;
  int newtimeout;
  DCBINFO devinfo;

  if ((ps->os2_read_timeout==read_timeout || read_timeout==-1) && 
      (ps->os2_write_timeout==write_timeout || write_timeout==-1))
    return(0);

  ret_len=sizeof(DCBINFO);
  rc=DosDevIOCtl(ps->mac.fd, /* file decsriptor */
		 IOCTL_ASYNC, /*asyncronous change */
		 ASYNC_GETDCBINFO, /* get device control block info */
		 NULL, /*  */
		 0, /* length of the previous parameter */
		 NULL, /* max length of data ret */
		 &devinfo, /* data to be recieved */
		 ret_len, /* length of data */
		 (unsigned long *)&ret_len); /* length of data returned */
  if (rc)
    goto error;

  if (read_timeout!=-1)
    {
      if (read_timeout==0)
	{
      devinfo.usReadTimeout=65535;
    }
      else
	{
	  newtimeout=read_timeout * 10 -0.1;
	  if (newtimeout>65535)
	    newtimeout=65535;
	  devinfo.usReadTimeout=newtimeout;
	}
    }
  if (write_timeout==-1)
    {
      if (write_timeout==0)
	{
	  devinfo.fbTimeout |=0x01;
	}
      else
	{
	  devinfo.fbTimeout &= 0xFE;
	  newtimeout=write_timeout*10;
	  if (newtimeout>65535)
	    newtimeout=65535;
	  devinfo.usWriteTimeout=newtimeout;
	}
    }
  param_length=sizeof(DCBINFO);
  rc=DosDevIOCtl(ps->mac.fd, /* file decsriptor */
		 IOCTL_ASYNC, /*asyncronous change */
		 ASYNC_SETDCBINFO, /* get device control block info */
		 &devinfo, /* parameters to set  */
		 param_length, /* length of the previous parameter */
		 (unsigned long *)&param_length, /* max length of parameters */
		 NULL, /* data to be recieved */
		 0, /* length of data */
		 NULL); /* length of data returned */


       
error:
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
  if (read_timeout!=-1)
    ps->os2_read_timeout=read_timeout;
  if (write_timeout!=-1)
    ps->os2_write_timeout=write_timeout;
#ifdef OS2_DEBUG
  fprintf(stderr,"set read_timeout to %d\n",read_timeout);
  fprintf(stderr,"set write_timeout to %d\n",write_timeout);
#endif
  return(0);
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
#endif

  /* FIXME: if timeout == 0, wait forever for packet, otherwise wait till
     timeout seconds */

#ifdef OS2
  /* for OS2, timeout of 0 is almost forever, only 1.8 hours */
  /* if no timeout is set at all, the timeout defaults to 1 minute */
  rc=pi_socket_set_timeout(ps,timeout,-1);
  if (rc==-1)
    {
      fprintf(stderr,"error setting timeout, old timeout used\n");
    }
#endif /* #ifdef OS2 */

  pi_socket_flush(ps);              /* We likely want to be in sync with tx */
  if (!ps->mac.expect) slp_rx(ps);  /* let SLP know we want a packet */

  while (ps->mac.expect) {
    buf = ps->mac.buf;

    while (ps->mac.expect) {
#ifdef OS2
      rc=DosRead(ps->mac.fd,buf,ps->mac.expect,(unsigned long *)&r);
      if (rc)
#else /* #ifdef OS2 */
      ready2 = ready;
      t.tv_sec = timeout;
      t.tv_usec = 0;
      select(ps->mac.fd+1,&ready2,0,0,&t);
      /* If data is available in time, read it */
      if(FD_ISSET(ps->mac.fd,&ready2))
        r = read(ps->mac.fd, buf, ps->mac.expect);
      else
#endif /* #ifdef OS2 */
      {
        /* otherwise throw out any current packet and return */
#ifdef DEBUG
        fprintf(stderr, "Serial RX: timeout\n");
#endif
        ps->mac.state = ps->mac.expect = 1;
        ps->mac.buf = ps->mac.rxb->data;
        ps->rx_errors++;
        return 0;
      }
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

