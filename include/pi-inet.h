#ifndef _PILOT_INET_H_
#define _PILOT_INET_H_

#include "pi-args.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int pi_inet_connect(struct pi_socket *ps, struct sockaddr *addr, int addrlen);
extern int pi_inet_bind(struct pi_socket *ps, struct sockaddr *addr, int addrlen);

#ifdef __cplusplus
}
#endif

#endif
