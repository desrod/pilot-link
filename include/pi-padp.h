#ifndef _PILOT_PADP_H_
#define _PILOT_PADP_H_

#include "pi-args.h"

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

	struct pi_padp_data 
	{
		int type;
		int last_type;

		unsigned char txid;
		unsigned next_txid;
	};

	struct padp {
		unsigned char type;
		unsigned char flags;
		unsigned short size;
	};

#define SIZEOF_PADP 4

	extern struct pi_protocol *padp_protocol
	    PI_ARGS((void));

	extern int padp_tx
	    PI_ARGS((struct pi_socket *ps, unsigned char *buf, int len, int flags));

	extern int padp_rx
	    PI_ARGS((struct pi_socket *ps, unsigned char *buf, int len, int flags));

	extern void padp_dump_header
	    PI_ARGS((unsigned char *data, int rxtx));
	extern void padp_dump
	    PI_ARGS((unsigned char *data));

#ifdef __cplusplus
}
#endif
#endif				/* _PILOT_PADP_H_ */

