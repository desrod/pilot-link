#ifndef _PILOT_SERIAL_H_
#define _PILOT_SERIAL_H_

#include "pi-args.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int pi_device_open PI_ARGS((char *tty, struct pi_socket *ps));
extern int pi_device_changebaud PI_ARGS((struct pi_socket *ps));
extern int pi_device_close PI_ARGS((struct pi_socket *ps));
extern int pi_socket_send PI_ARGS((struct pi_socket *ps));
extern int pi_socket_flush PI_ARGS((struct pi_socket *ps));
extern int pi_socket_read PI_ARGS((struct pi_socket *ps, int timeout)); /*timeout is in seconds*10*/
#ifdef OS2
extern int pi_socket_set_timeout PI_ARGS((struct pi_socket *ps, int read_timeout, int write_timeout));
#endif

#ifdef __cplusplus
}
#endif

#endif
