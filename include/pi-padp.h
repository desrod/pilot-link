#ifndef _PILOT_PADP_H_
#define _PILOT_PADP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "pi-socket.h"

#define PI_PADP_HEADER_LEN  4
#define PI_PADP_MTU         1024

#define PI_PADP_OFFSET_TYPE 0
#define PI_PADP_OFFSET_FLGS 1
#define PI_PADP_OFFSET_SIZE 2

#define padData		0x01
#define padWake		0x101
#define padAck		0x02
#define padTickle	0x04
#define padAbort	0x08	/* PalmOS 2.0 only */

#define FIRST		0x80
#define LAST		0x40
#define MEMERROR	0x20

	typedef struct padp {
		unsigned char type;
		unsigned char flags;
		unsigned short size;
	} padp_t;

	typedef struct pi_padp_data 
	{
		int type;
		int last_type;

		unsigned char txid;
		unsigned next_txid;

		unsigned char last_ack_txid;
		struct padp last_ack_padp;
	} pi_padp_data_t;


	extern pi_protocol_t *padp_protocol
	    PI_ARGS((void));

	extern ssize_t padp_tx
	    PI_ARGS((pi_socket_t *ps, unsigned char *buf, size_t len,
			int flags));

	extern ssize_t padp_rx
	    PI_ARGS((pi_socket_t *ps, pi_buffer_t *buf, size_t expect,
			int flags));

	extern void padp_dump_header
	    PI_ARGS((unsigned char *data, int rxtx));
	extern void padp_dump
	    PI_ARGS((unsigned char *data));

#ifdef __cplusplus
}
#endif
#endif				/* _PILOT_PADP_H_ */

