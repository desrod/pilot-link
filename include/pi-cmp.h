#ifndef _PILOT_CMP_H_
#define _PILOT_CMP_H_

#include "pi-args.h"
#include "pi-buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PI_CMP_HEADER_LEN 10
#define PI_CMP_MTU        10

#define PI_CMP_OFFSET_TYPE 0
#define PI_CMP_OFFSET_FLGS 1
#define PI_CMP_OFFSET_VERS 2
#define PI_CMP_OFFSET_RESV 4
#define PI_CMP_OFFSET_BAUD 6

#define PI_CMP_TYPE_WAKE 0x01
#define PI_CMP_TYPE_INIT 0x02
#define PI_CMP_TYPE_ABRT 0x03

#define PI_CMP_VERS_1_0 0x0100L
#define PI_CMP_VERS_1_1 0x0101L
#define PI_CMP_VERS_1_2 0x0102L

	struct pi_cmp_data {
		unsigned char type;
		unsigned char flags;
		unsigned int version;
		speed_t baudrate;
	};

	extern pi_protocol_t *cmp_protocol
	    PI_ARGS((void));

	extern int cmp_rx_handshake
	  PI_ARGS((pi_socket_t *ps, unsigned long establishrate,
		int establishhighrate));
	extern int cmp_tx_handshake
	  PI_ARGS((pi_socket_t *ps));
	extern ssize_t cmp_tx
	  PI_ARGS((pi_socket_t *ps, unsigned char *buf,
		size_t len, int flags));
	extern ssize_t cmp_rx
	  PI_ARGS((pi_socket_t *ps, pi_buffer_t *msg,
		size_t expect, int flags));

	extern int cmp_init
	  PI_ARGS((pi_socket_t *ps, speed_t baudrate));
	extern int cmp_abort
	  PI_ARGS((pi_socket_t *ps, int reason));
	extern int cmp_wakeup
	  PI_ARGS((pi_socket_t *ps, speed_t maxbaud));

	extern void cmp_dump
	  PI_ARGS((unsigned char *cmp, int rxtx));

#ifdef __cplusplus
}
#endif
#endif				/* _PILOT_CMP_H_ */
