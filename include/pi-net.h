#ifndef _PILOT_NET_H_
#define _PILOT_NET_H_

#include "pi-args.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PI_NET_HEADER_LEN  6
#define PI_NET_MTU         0xffff

#define PI_NET_SIG_BYTE1   0x90

#define PI_NET_OFFSET_TYPE 0
#define PI_NET_OFFSET_TXID 1
#define PI_NET_OFFSET_SIZE 2

#define PI_NET_TYPE_DATA 0x01
#define PI_NET_TYPE_TCKL 0x02

	struct pi_net_data 
	{
		int type;

		unsigned char txid;
	};

	extern struct pi_protocol *net_protocol
	    PI_ARGS((void));

	extern int net_rx_handshake
	    PI_ARGS((struct pi_socket *ps));
	extern int net_tx_handshake
	    PI_ARGS((struct pi_socket *ps));
	extern int net_tx
	    PI_ARGS((struct pi_socket *ps, unsigned char *buf, int len, int flags));
	extern int net_rx
	    PI_ARGS((struct pi_socket *ps, unsigned char *buf, int len, int flags));

	extern void net_dump_header
	    PI_ARGS((unsigned char *data, int rxtx));
	extern void net_dump
	    PI_ARGS((unsigned char *data));

#ifdef __cplusplus
}
#endif
#endif
