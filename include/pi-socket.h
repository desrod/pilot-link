#ifndef _PILOT_SOCKET_H_
#define _PILOT_SOCKET_H_

#include <termios.h>

#ifdef __EMX__
#define OS2
#include <sys/param.h> /* for htonl .. */
#define ENOMSG 150
#define strcasecmp stricmp
#define strncasecmp strnicmp

# include <sys/ioctl.h>
# include <sys/time.h>
# include <time.h>
# include <fcntl.h>
# include <unistd.h>
# include <string.h>
# include <stdlib.h>
# include <netinet/in.h>
# include <dirent.h>
# define TTYPrompt "com#"

#else
#include "pi-config.h"
#endif

#define AF_SLP 0x0001        /* arbitrary, for completeness, just in case */

#define PF_SLP    AF_SLP
#define PF_PADP   0x0002
#define PF_LOOP   0x0003

#ifndef SOCK_STREAM
#define SOCK_STREAM    0x0010
#define SOCK_DGRAM     0x0020
#define SOCK_RAW       0x0030
#define SOCK_SEQPACKET 0x0040
#endif

#define PilotSocketDLP       3
#define PilotSocketConsole   1
#define PilotSocketDebugger  0
#define PilotSocketRemoteUI  2

#define SLP_MTU 1038

struct pi_sockaddr {
  unsigned short sa_family;
  unsigned short port;
  unsigned char device[12];
};

struct pi_skb {
  struct pi_skb *next;
  int len;
  unsigned char data[SLP_MTU];
};

struct pi_mac {
  int fd;
  int state;
  int expect;
  struct pi_skb *rxb;
  char *buf;
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
  struct termios tco;
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

int pi_socket(int domain, int type, int protocol);
int pi_connect(int pi_sd, struct pi_sockaddr *remote_addr, int addrlen);
int pi_bind(int pi_sd, struct pi_sockaddr *my_addr, int addrlen);
int pi_listen(int pi_sd, int backlog);
int pi_accept(int pi_sd, struct pi_sockaddr *remote_addr, int *addrlen);

int pi_send(int pi_sd, void *msg, int len, unsigned int flags);
int pi_recv(int pi_sd, void *msg, int len, unsigned int flags);

int pi_read(int pi_sd, void *msg, int len);
int pi_write(int pi_sd, void *msg, int len);

int pi_getsockname(int pi_sd, struct pi_sockaddr * addr, int * namelen);
int pi_getsockpeer(int pi_sd, struct pi_sockaddr * addr, int * namelen);

int pi_tickle(int pi_sd);

int pi_close(int pi_sd);

int pi_sdtofd(int pi_sd);

/* internal functions */

int pi_device_open(char *, struct pi_socket *ps);
struct pi_socket *find_pi_socket(int sd);
int crc16(unsigned char *ptr, int count);
char * printlong (unsigned long val);
void dumpline (const unsigned char *buf, int len, int addr);
void dumpdata (const unsigned char * buf, int len);

/* portable field access */

#define get_long(ptr) (((ptr)[0] << 24) | \
                       ((ptr)[1] << 16) | \
                       ((ptr)[2] << 8)  | \
                       ((ptr)[3] << 0))

#define get_treble(ptr) (((ptr)[0] << 16) | \
                         ((ptr)[1] << 8)  | \
                         ((ptr)[2] << 0))
                       
#define get_short(ptr) (((ptr)[0] << 8)  | \
                        ((ptr)[1] << 0))
                        
#define get_byte(ptr) ((ptr)[0])

#define set_long(ptr,val) (((ptr)[0] = ((val) >> 24) & 0xff), \
		          ((ptr)[1] = ((val) >> 16) & 0xff), \
		          ((ptr)[2] = ((val) >> 8) & 0xff), \
		          ((ptr)[3] = ((val) >> 0) & 0xff))

#define set_treble(ptr,val) (((ptr)[0] = ((val) >> 16) & 0xff), \
		             ((ptr)[1] = ((val) >> 8) & 0xff), \
		             ((ptr)[2] = ((val) >> 0) & 0xff))
                       
#define set_short(ptr,val) (((ptr)[0] = ((val) >> 8) & 0xff), \
		            ((ptr)[1] = ((val) >> 0) & 0xff))

#define set_byte(ptr,val) ((ptr)[0]=(val))

#ifdef TRACE
#define Begin(a) fprintf(stderr,"Begin %s\n",#a)
#define At(a)    fprintf(stderr,"At %s\n",#a)
#define End(a)   fprintf(stderr,"End %s\n",#a)
#else
#define Begin(a)
#define At(a)
#define End(a)
#endif

#endif /* _PILOT_SOCKET_H_ */

