#ifndef _PILOT_USB_H_
#define _PILOT_USB_H_

#include "pi-args.h"
#include "pi-buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PI_USB_DEV     1

	struct pi_usb_data;

	typedef struct pi_usb_impl {
		int (*open) PI_ARGS((pi_socket_t *ps,
			struct pi_sockaddr *addr, size_t addrlen));
		int (*close) PI_ARGS((pi_socket_t *ps));
		ssize_t (*write) PI_ARGS((pi_socket_t *ps,
			unsigned char *buf, size_t len, int flags));
		ssize_t (*read) PI_ARGS((pi_socket_t *ps,
			 pi_buffer_t *buf, size_t expect, int flags));
		int (*flush) PI_ARGS((pi_socket_t *ps, int flags));
		int (*poll) PI_ARGS((pi_socket_t *ps, int timeout));

		int (*control_request) PI_ARGS((struct pi_usb_data *usb_data,
			int request_type, int request, int value, int reqindex,
			void *data, int size, int timeout));
	} pi_usb_impl_t;

#define USB_INIT_NONE		(1<<0)
#define USB_INIT_TAPWAVE	(1<<1)
#define USB_INIT_VISOR		(1<<2)
#define USB_INIT_SONY_CILE	(1<<3)

	typedef struct pi_usb_dev {
		u_int16_t	vendor, product;
		u_int32_t	flags;
		char		*idstr;
	} pi_usb_dev_t;

	typedef struct pi_usb_data {
		struct pi_usb_impl impl;
		struct pi_usb_dev dev;

		unsigned char buf[256];		/* temp. buffer to hold incoming data when peeking at init time*/
		size_t buf_size;
		
		/* I/O options */
		void *ref;

		/* Time out */
		int timeout;
	} pi_usb_data_t;

	extern pi_device_t *pi_usb_device PI_ARGS((int type));
	extern void pi_usb_impl_init PI_ARGS((struct pi_usb_impl *impl));
	extern int USB_check_device PI_ARGS((pi_usb_data_t *dev, u_int16_t vendor, u_int16_t product));
	extern int USB_configure_device PI_ARGS((pi_usb_data_t *dev, u_int8_t *input_pipe, u_int8_t *output_pipe));


	/* Start of the new generic USB pilot init stuff. */

	/*
	 * USB control requests we send to the devices
	 * From linux/drivers/usb/serial/visor.h
	 */
#define GENERIC_REQUEST_BYTES_AVAILABLE         0x01
#define GENERIC_CLOSE_NOTIFICATION              0x02
#define VISOR_GET_CONNECTION_INFORMATION        0x03
#define PALM_GET_EXT_CONNECTION_INFORMATION     0x04

	/*
	 * Reply struct and defines for VISOR_GET_CONNECTION_INFORMATION
	 */
	typedef struct
	{
		u_int16_t num_ports;
		struct
		{
			u_int8_t port_function_id;
			u_int8_t port;
		} connections[2];
	} visor_connection_info_t;
	/* struct visor_connection_info.connection[x].port defines: */
#define VISOR_ENDPOINT_1        0x01
#define VISOR_ENDPOINT_2        0x02

	/* struct visor_connection_info.connection[x].port_function_id defines: */
#define VISOR_FUNCTION_GENERIC              0x00
#define VISOR_FUNCTION_DEBUGGER             0x01
#define VISOR_FUNCTION_HOTSYNC              0x02
#define VISOR_FUNCTION_CONSOLE              0x03
#define VISOR_FUNCTION_REMOTE_FILE_SYS      0x04

	/*
	 * Reply struct for PALM_GET_EXT_CONNECTION_INFORMATION
	 */
	typedef struct
	{
		u_int8_t num_ports;
		u_int8_t endpoint_numbers_different;
		u_int16_t reserved1;
		struct
		{
			char port_function_id[4];
			u_int8_t port;
			u_int8_t endpoint_info;
			u_int16_t reserved;
		} connections[2];
	} palm_ext_connection_info_t;


#ifdef __cplusplus
}
#endif
#endif
