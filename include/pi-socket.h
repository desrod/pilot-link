#ifndef _PILOT_SOCKET_H_
#define _PILOT_SOCKET_H_

#ifdef __cplusplus
extern "C" {
#endif

#define PILOT_LINK_VERSION 0
#define PILOT_LINK_MAJOR 5
#define PILOT_LINK_MINOR 7

#define PI_AF_SLP 0x0001        /* arbitrary, for completeness, just in case */

#define PI_PF_SLP    PI_AF_SLP
#define PI_PF_PADP   0x0002
#define PI_PF_LOOP   0x0003

#define PI_SOCK_STREAM    0x0010
#define PI_SOCK_DGRAM     0x0020
#define PI_SOCK_RAW       0x0030
#define PI_SOCK_SEQPACKET 0x0040

#define PI_PilotSocketDLP       3
#define PI_PilotSocketConsole   1
#define PI_PilotSocketDebugger  0
#define PI_PilotSocketRemoteUI  2

struct pi_sockaddr {
  unsigned short pi_family;
  unsigned short pi_port;
  char pi_device[12];
};

struct pi_skb;

struct pi_mac;

struct pi_socket;

extern int pi_socket(int domain, int type, int protocol);
extern int pi_connect(int pi_sd, struct pi_sockaddr *remote_addr, int addrlen);
extern int pi_bind(int pi_sd, struct pi_sockaddr *my_addr, int addrlen);
extern int pi_listen(int pi_sd, int backlog);
extern int pi_accept(int pi_sd, struct pi_sockaddr *remote_addr, int *addrlen);

extern int pi_send(int pi_sd, void *msg, int len, unsigned int flags);
extern int pi_recv(int pi_sd, void *msg, int len, unsigned int flags);

extern int pi_read(int pi_sd, void *msg, int len);
extern int pi_write(int pi_sd, void *msg, int len);

extern int pi_getsockname(int pi_sd, struct pi_sockaddr * addr, int * namelen);
extern int pi_getsockpeer(int pi_sd, struct pi_sockaddr * addr, int * namelen);

extern unsigned int pi_version(int pi_sd);

extern int pi_tickle(int pi_sd);

extern int pi_close(int pi_sd);

#ifdef __cplusplus
}
#endif

#endif /* _PILOT_SOCKET_H_ */
