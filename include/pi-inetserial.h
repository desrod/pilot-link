#ifndef _PILOT_INETSERIAL_H_
#define _PILOT_INETSERIAL_H_

#include "pi-args.h"

#ifdef __cplusplus
extern "C" {
#endif

	extern int pi_inetserial_connect
	    PI_ARGS((struct pi_socket * ps, struct sockaddr * addr,
		     int addrlen));
	extern int pi_inetserial_bind
	    PI_ARGS((struct pi_socket * ps, struct sockaddr * addr,
		     int addrlen));

	extern int pi_inetserial_open
	    PI_ARGS((struct pi_socket * ps, struct sockaddr * addr,
		     int addrlen));

#ifdef __cplusplus
}
#endif
#endif
