/* serial.c:  tty line interface code for Pilot networking
 *
 * (c) 1996, D. Jeff Dionne.
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-serial.h"
#include "pi-slp.h"

/* if this is running on a NeXT system... */
#ifdef NeXT
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/file.h>
#include <fcntl.h>
#endif

#ifdef HAVE_SYS_IOCTL_COMPAT_H
#include <sys/ioctl_compat.h>
#endif

#ifdef OS2
#define INCL_BASE
#define INCL_DOSFILEMGR    /* File System values */
#define INCL_DOSDEVIOCTL   /* DosDevIOCtl values */
#define INCL_DOSDEVICES    /* DosDevice   values */
#include <os2.h>
#endif /* OS2 */

#ifdef SERIAL_TRACE
int debugh=0;
#endif

#ifndef OS2

#ifndef SGTTY

#ifndef HAVE_CFMAKERAW
#define cfmakeraw(ptr) (ptr)->c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR\
					 |IGNCR|ICRNL|IXON);\
                       (ptr)->c_oflag &= ~OPOST;\
                       (ptr)->c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);\
                       (ptr)->c_cflag &= ~(CSIZE|PARENB);\
                       (ptr)->c_cflag |= CS8
#endif

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

#endif /*SGTTY*/

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

#ifndef SGTTY
  int i;
  struct termios tcn;
#else
  struct sgttyb tcn;
#endif

  if ((ps->mac.fd = open(tty, O_RDWR )) == -1) {
    return -1;     /* errno already set */
  }

  if (!isatty(ps->mac.fd)) {
    close(ps->mac.fd);
    errno = EINVAL;
    return -1;
  }

#ifndef SGTTY
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
  
  tcsetattr(ps->mac.fd,TCSANOW,&tcn);
#else
  /* Set the tty to raw and to the correct speed */
  ioctl(ps->mac.fd, TIOCGETP, &tcn);

  ps->tco = tcn;

  tcn.sg_flags = RAW;
  tcn.sg_ispeed = calcrate(ps->rate);
  tcn.sg_ospeed = calcrate(ps->rate);
  
  ioctl(ps->mac.fd, TIOCSETN, &tcn);
#endif

#ifdef SERIAL_TRACE
  if(debugh==0) {
    debugh = open("PiDebug.log",O_WRONLY|O_CREAT|O_APPEND,0666);
    write(debugh, "\0\1\0\0\0\0\0\0\0\0", 10);
  }
#endif

  return ps->mac.fd;
}

int pi_device_changebaud(struct pi_socket *ps)
{
#ifndef SGTTY
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

#else
  struct sgttyb tcn;

  ioctl(ps->mac.fd, TIOCGETP, &tcn);

  tcn.sg_ispeed = calcrate(ps->rate);
  tcn.sg_ospeed = calcrate(ps->rate);
  
  ioctl(ps->mac.fd, TIOCSETN, &tcn);
#endif

  return 0;
}

int pi_device_close(struct pi_socket *ps)
{
  int result;
#ifndef SGTTY

#if 1 /*defined(linux)*/
  /* Something isn't getting flushed somewhere. If this sleep is removed,
     the Pilot never gets the final padp Ack.*/
  /* And now it turns out NetBSD requires this pause too. I'm turning
     it on for everyone now, until I find a proper solution. */
  sleep(1);
#endif

  tcsetattr(ps->mac.fd,TCSADRAIN, &ps->tco);
#else

  ioctl(ps->mac.fd, TIOCSETP, &ps->tco);

#endif

  result = close(ps->mac.fd);
  ps->mac.fd = 0;

#ifdef SERIAL_TRACE
  if (debugh)
    close(debugh);  
#endif

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
  pi_socket_set_timeout(ps,-1,600);
  
#ifdef SERIAL_TRACE
  if (debugh==0) {
    debugh = open("PiDebug.log",O_WRONLY|O_CREAT,0666);
    write(debugh, "\0\1\0\0\0\0\0\0\0\0", 10);
  }
#endif
  return(fd);  
}

int pi_device_changebaud(struct pi_socket *ps)
{
  int param_length;
  int rc, baudrate;
  unsigned char linctrl[3] = {8,0,0};

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

  /* also set the port to 8N1 as OS/2 defaults to some braindead values */
  if (!rc)   /* but only if the previous operation succeeded */
    {
      param_length = 3; /* 3 bytes for line control */
      rc=DosDevIOCtl(ps->mac.fd, /* file decsriptor */
		     IOCTL_ASYNC, /*asyncronous change */
		     ASYNC_SETLINECTRL, /* set the line controls */
		     linctrl, /* pointer to the configuration */
		     param_length, /* length of the previous parameter */
		     (unsigned long *)&param_length, /* max length of params */
		     NULL, /* data to be returned */
		     0, /* length of data */
		     NULL); /* length of data returned */
    }


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
#ifdef SERIAL_TRACE
  if (debugh)
    close(debugh);  
#endif

  DosClose(ps->mac.fd);
  return(0);
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
	  newtimeout=read_timeout -0.1;
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
	  newtimeout=write_timeout;
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
#ifdef SERIAL_TRACE
  int i;
#endif
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
#ifdef SERIAL_TRACE
    /*write(6,skb->data,skb->len);*/
    for (i=0;i<skb->len;i++) {
      write(debugh, "2", 1);
      write(debugh, skb->data+i, 1);
    }
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
  unsigned char *buf;
#ifdef SERIAL_TRACE
  int i;
#endif
#ifdef OS2
  int rc;
#else
  fd_set ready,ready2;
  struct timeval t;
  
  FD_ZERO(&ready);
  FD_SET(ps->mac.fd, &ready);
#endif

  /* FIXME: if timeout == 0, wait forever for packet, otherwise wait till
     timeout tenth-of-seconds */

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
      t.tv_sec = timeout/10;
      t.tv_usec = (timeout % 10) * 100000;
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
#ifdef SERIAL_TRACE
      /*write(7, buf, r);*/
      for (i=0;i<r;i++) {
        write(debugh, "1", 1);
        write(debugh, buf+i, 1);
      }
#endif
      ps->rx_bytes += r;
      buf += r;
      ps->mac.expect -= r;
    }
    slp_rx(ps);
  }
  return 0;
}

