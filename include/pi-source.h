#ifndef _PILOT_SOURCE_H_
#define _PILOT_SOURCE_H_

#ifdef NeXT
# include <sys/types.h>
# include <sys/socket.h>
#endif

#ifdef __EMX__
# define OS2
# include <sys/param.h> /* for htonl .. */
# define ENOMSG 150
# define strcasecmp stricmp
# define strncasecmp strnicmp

# include <sys/ioctl.h>
# include <sys/time.h>
# include <sys/types.h>
# include <sys/errno.h>
# include <time.h>
# include <fcntl.h>
# include <unistd.h>
# include <string.h>
# include <stdlib.h>
/*# include <netinet/in.h>*/
# include <dirent.h>
# include <errno.h>
# define TTYPrompt "com#"
# define RETSIGTYPE void

#else
# include "pi-config.h"
#endif

#ifdef SGTTY
# include <sgtty.h>
#else
# include <termios.h>
#endif

#include "pi-socket.h"
#include "pi-macros.h"

#define PI_SLP_MTU 1038

struct pi_skb {
  struct pi_skb *next;
  int len;
  unsigned char data[PI_SLP_MTU];
};

struct pi_mac {
  int fd;
  int state;
  int expect;
  struct pi_skb *rxb;
  unsigned char *buf;
};

struct pi_socket {
  struct pi_sockaddr laddr;
  struct pi_sockaddr raddr;
  int type;
  int protocol;
  unsigned char xid;
  unsigned char nextid;
  int sd;
  int initiator;
  struct pi_mac mac;
#ifndef OS2
# ifndef SGTTY
   struct termios tco;
# else
   struct sgttyb tco;
# endif
#endif
  struct pi_skb *txq;
  struct pi_skb *rxq;
  struct pi_socket *next;
  int rate;          /* Current port baud rate */
  int establishrate; /* Baud rate to use after link is established */
  int connected;
  int tx_packets;
  int rx_packets;
  int tx_bytes;
  int rx_bytes;
  int tx_errors;
  int rx_errors;
  char last_tid;
#ifdef OS2
  unsigned short os2_read_timeout;
  unsigned short os2_write_timeout;
#endif
};

/* internal functions */

extern int pi_device_open(char *, struct pi_socket *ps);
extern struct pi_socket *find_pi_socket(int sd);
extern int crc16(unsigned char *ptr, int count);
extern char * printlong (unsigned long val);
extern void dumpline (const unsigned char *buf, int len, int addr);
extern void dumpdata (const unsigned char * buf, int len);

#if defined(PADP_TRACE)
#define Begin(a) fprintf(stderr,"Begin %s\n",#a)
#define At(a)    fprintf(stderr,"At %s\n",#a)
#define End(a)   fprintf(stderr,"End %s\n",#a)
#else
#define Begin(a)
#define At(a)
#define End(a)
#endif

#endif /* _PILOT_SOCKET_H_ */

