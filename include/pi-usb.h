#ifndef _PILOT_USB_H_
#define _PILOT_USB_H_

#include "pi-args.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PI_USB_DEV     1

	struct pi_usb_impl {
		int (*open) PI_ARGS((struct pi_socket *ps, struct pi_sockaddr *addr, int addrlen));
		int (*close) PI_ARGS((struct pi_socket *ps));
		int (*write) PI_ARGS((struct pi_socket *ps, unsigned char *buf, int len, int flags));
		int (*read) PI_ARGS((struct pi_socket *ps, unsigned char *buf, int len, int flags));
		int (*poll) PI_ARGS((struct pi_socket *ps, int timeout));
	};

	struct pi_usb_data {
		struct pi_usb_impl impl;

		unsigned char buf[256];
		unsigned char *pos;
		int buf_size;
		
		/* I/O options */
		int *ref;

		/* Time out */
		int timeout;
	};

	extern struct pi_device *pi_usb_device
            PI_ARGS((int type));

	extern void pi_usb_impl_init
	    PI_ARGS((struct pi_usb_impl *impl));

#ifdef __cplusplus
}
#endif
#endif
