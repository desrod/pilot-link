#ifndef _PILOT_SERIAL_H_
#define _PILOT_SERIAL_H_

#include "pi-args.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int pi_serial_connect(struct pi_socket *ps, struct sockaddr *addr, int addrlen);
extern int pi_serial_bind(struct pi_socket *ps, struct sockaddr *addr, int addrlen);

extern int pi_serial_open(struct pi_socket *ps, struct pi_sockaddr * addr, int addrlen);

extern int pi_serial_flush(struct pi_socket *ps);


#ifdef __cplusplus
}
#endif

#endif
