#ifndef _PILOT_USB_H_
#define _PILOT_USB_H_

#include "pi-args.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PI_USB_DEV     1

	typedef struct pi_usb_impl {
		int (*open) PI_ARGS((pi_socket_t *ps,
			struct pi_sockaddr *addr, size_t addrlen));
		int (*close) PI_ARGS((pi_socket_t *ps));
		int (*write) PI_ARGS((pi_socket_t *ps,
			unsigned char *buf, size_t len, int flags));
		int (*read) PI_ARGS((pi_socket_t *ps,
			 unsigned char *buf, size_t len, int flags));
		int (*poll) PI_ARGS((pi_socket_t *ps, int timeout));
	} pi_usb_impl_t;

	typedef struct pi_usb_data {
		struct pi_usb_impl impl;

		unsigned char buf[256];
		unsigned char *pos;
		size_t buf_size;
		
		/* I/O options */
		int *ref;

		/* Time out */
		int timeout;
	} pi_usb_data_t;

	extern pi_device_t *pi_usb_device PI_ARGS((int type));
	extern void pi_usb_impl_init PI_ARGS((struct pi_usb_impl *impl));

#ifdef __cplusplus
}
#endif
#endif
