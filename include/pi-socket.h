#ifndef _PILOT_SOCKET_H_
#define _PILOT_SOCKET_H_

#include "pi-args.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PILOT_LINK_VERSION 0
#define PILOT_LINK_MAJOR 7
#define PILOT_LINK_MINOR 1

#define PI_AF_SLP 0x0051        /* arbitrary, for completeness, just in case */
#define PI_AF_INETSLP 0x0054    

#define PI_PF_SLP    PI_AF_SLP
#define PI_PF_PADP   0x0052
#define PI_PF_LOOP   0x0053

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
  char pi_device[12];
};

struct pi_skb;

struct pi_mac;

struct pi_socket;

extern int pi_socket PI_ARGS((int domain, int type, int protocol));
extern int pi_connect PI_ARGS((int pi_sd, struct sockaddr *remote_addr, int addrlen));
extern int pi_bind PI_ARGS((int pi_sd, struct sockaddr *my_addr, int addrlen));
extern int pi_listen PI_ARGS((int pi_sd, int backlog));
extern int pi_accept PI_ARGS((int pi_sd, struct sockaddr *remote_addr, int *addrlen));

extern int pi_send PI_ARGS((int pi_sd, void *msg, int len, unsigned int flags));
extern int pi_recv PI_ARGS((int pi_sd, void *msg, int len, unsigned int flags));

extern int pi_read PI_ARGS((int pi_sd, void *msg, int len));
extern int pi_write PI_ARGS((int pi_sd, void *msg, int len));

extern int pi_getsockname PI_ARGS((int pi_sd, struct sockaddr * addr, int * namelen));
extern int pi_getsockpeer PI_ARGS((int pi_sd, struct sockaddr * addr, int * namelen));

extern unsigned int pi_version PI_ARGS((int pi_sd));

extern int pi_tickle PI_ARGS((int pi_sd));
extern int pi_watchdog PI_ARGS((int pi_sd, int interval));

extern int pi_close PI_ARGS((int pi_sd));

#ifdef __cplusplus
}
#endif

#endif /* _PILOT_SOCKET_H_ */
