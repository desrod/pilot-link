#ifndef _PILOT_NET_H_
#define _PILOT_NET_H_

#include "pi-args.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int pi_net_device_open PI_ARGS((char *tty, struct pi_socket *ps));
extern int pi_net_device_changebaud PI_ARGS((struct pi_socket *ps));
extern int pi_net_device_close PI_ARGS((struct pi_socket *ps));
extern int pi_net_device_write PI_ARGS((struct pi_socket *ps));
extern int pi_net_device_read PI_ARGS((struct pi_socket *ps, int timeout)); /*timeout is in seconds*10*/

#ifdef __cplusplus
}
#endif

#endif
