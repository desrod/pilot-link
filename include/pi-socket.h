#ifndef _PILOT_SOCKET_H_
#define _PILOT_SOCKET_H_

#ifdef bsdi
#include <termios.h>
#else
#include <termio.h>
#endif

#define AF_SLP 0x0001        /* arbitrary, for completeness, just in case */

#define PF_SLP  AF_SLP
#define PF_PADP 0x0002

#define SOCK_STREAM    0x0010
#define SOCK_DGRAM     0x0020
#define SOCK_RAW       0x0030
#define SOCK_SEQPACKET 0x0040

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
  int protocol;
  int xid;
  int sd;
  struct pi_mac mac;
  struct termios tco;
  struct pi_skb *txq;
  struct pi_skb *rxq;
  struct pi_socket *next;
  int rate;
  int connected;
  int tx_packets;
  int rx_packets;
  int tx_bytes;
  int rx_bytes;
  int tx_errors;
  int rx_errors;
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

int pi_close(int pi_sd);

int pi_sdtofd(int pi_sd);

/* internal functions */

int pi_device_open(char *, struct pi_socket *ps);
struct pi_socket *find_pi_socket(int sd);
int crc16(unsigned char *ptr, int count);

#endif /* _PILOT_SOCKET_H_ */

