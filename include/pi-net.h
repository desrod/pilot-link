#ifndef _PILOT_NET_H_
#define _PILOT_NET_H_

#include "pi-args.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PI_NET_HEADER_LEN  6

#define PI_NET_OFFSET_TYPE 0
#define PI_NET_OFFSET_TXID 1
#define PI_NET_OFFSET_SIZE 2

	struct pi_net_data 
	{
		unsigned char txid;
	};

	extern struct pi_protocol *net_protocol
	    PI_ARGS((void));		
	extern int net_rx_handshake
	    PI_ARGS((struct pi_socket *ps));
	extern int net_tx
	    PI_ARGS((struct pi_socket *ps, unsigned char *buf, int len));
	extern int net_rx
	    PI_ARGS((struct pi_socket *ps, unsigned char *buf, int len));

#ifdef __cplusplus
}
#endif
#endif
