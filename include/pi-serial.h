#ifndef _PILOT_SERIAL_H_
#define _PILOT_SERIAL_H_

#include "pi-args.h"

#ifdef __cplusplus
extern "C" {
#endif

	extern int pi_serial_connect
	    PI_ARGS((struct pi_socket * ps, struct sockaddr * addr,
		     int addrlen));
	extern int pi_serial_bind
	    PI_ARGS((struct pi_socket * ps, struct sockaddr * addr,
		     int addrlen));

	extern int pi_serial_open
	    PI_ARGS((struct pi_socket * ps, struct pi_sockaddr * addr,
		     int addrlen));

	extern int pi_serial_flush PI_ARGS((struct pi_socket * ps));

#ifdef __cplusplus
}
#endif
#endif
