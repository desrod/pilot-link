#ifndef _PILOT_SERIAL_H_
#define _PILOT_SERIAL_H_

#include "pi-args.h"
#include "pi-buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PI_SERIAL_DEV     1

	struct pi_serial_impl {
		int (*open) PI_ARGS((pi_socket_t *ps,
			struct pi_sockaddr *addr, size_t addrlen));
		int (*close) PI_ARGS((pi_socket_t *ps));
		int (*changebaud) PI_ARGS((pi_socket_t *ps));
		ssize_t (*write) PI_ARGS((pi_socket_t *ps,
			unsigned char *buf, size_t len, int flags));
		ssize_t (*read) PI_ARGS((pi_socket_t *ps,
			pi_buffer_t *buf, size_t expect, int flags));
		int (*flush) PI_ARGS((pi_socket_t *ps, int flags));
		int (*poll) PI_ARGS((pi_socket_t *ps, int timeout));
	};

	struct pi_serial_data {
		struct pi_serial_impl impl;

		unsigned char buf[256];
		size_t buf_size;
		
		/* I/O options */
		/*int *ref;*/
		
#ifndef OS2
# ifndef SGTTY
		struct termios tco;
# else
		struct sgttyb tco;
# endif
#endif

		/* Baud rate info */
		speed_t rate;	/* Current port baud rate */
		speed_t establishrate; /* Baud rate to use after link
					 is established                     */

		int establishhighrate;	/* Boolean: try to establish
					 rate higher than the device
					 publishes*/

		/* Time out */
		int timeout;
		
		/* Statistics */
		int rx_bytes;
		int rx_errors;

		int tx_bytes;
		int tx_errors;
	};

	extern pi_device_t *pi_serial_device
            PI_ARGS((int type));

	extern void pi_serial_impl_init
	    PI_ARGS((struct pi_serial_impl *impl));

#ifdef __cplusplus
}
#endif
#endif
