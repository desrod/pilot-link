#ifndef _PILOT_SERIAL_H_
#define _PILOT_SERIAL_H_

#include "pi-args.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PI_SERIAL_DEV     1

	struct pi_serial_impl {
		int (*open) PI_ARGS((struct pi_socket *ps, struct pi_sockaddr *addr, int addrlen));
		int (*close) PI_ARGS((struct pi_socket *ps));
		int (*changebaud) PI_ARGS((struct pi_socket *ps));
		int (*write) PI_ARGS((struct pi_socket *ps, unsigned char *buf, int len, int flags));
		int (*read) PI_ARGS((struct pi_socket *ps, unsigned char *buf, int len, int flags));
		int (*poll) PI_ARGS((struct pi_socket *ps, int timeout));
	};

	struct pi_serial_data {
		struct pi_serial_impl impl;

		unsigned char buf[256];
		int buf_size;
		
		/* I/O options */
		int *ref;
		
#ifndef WIN32
#ifndef OS2
# ifndef SGTTY
		struct termios tco;
# else
		struct sgttyb tco;
# endif
#endif
#endif

		/* Baud rate info */
		long rate;		/* Current port baud rate                         */
		int establishrate;	/* Baud rate to use after link is established                     */
		int establishhighrate;	/* Boolean: try to establish rate higher than the device publishes*/

		/* Time out */
		int timeout;
		
		/* Statistics */
		int rx_bytes;
		int rx_errors;

		int tx_bytes;
		int tx_errors;
	};

	extern struct pi_device *pi_serial_device
            PI_ARGS((int type));

	extern void pi_serial_impl_init
	    PI_ARGS((struct pi_serial_impl *impl));

#ifdef __cplusplus
}
#endif
#endif
