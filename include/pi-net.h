#ifndef _PILOT_NET_H_
#define _PILOT_NET_H_

#include "pi-args.h"
#include "pi-buffer.h"

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

	typedef struct pi_net_data 
	{
		int type;
		int split_writes;				/* set to 0 or <> 0 (see net_tx() function) */
		size_t write_chunksize;			/* set to 0 or a chunk size value (i.e. 4096) (see net_tx() function) */
		unsigned char txid;
	} pi_net_data_t;

	extern pi_protocol_t *net_protocol
	    PI_ARGS((void));

	extern int net_rx_handshake
	    PI_ARGS((pi_socket_t *ps));
	extern int net_tx_handshake
	    PI_ARGS((pi_socket_t *ps));
	extern ssize_t net_tx
	    PI_ARGS((pi_socket_t *ps, unsigned char *buf, size_t len,
		 int flags));
	extern ssize_t net_rx
	    PI_ARGS((pi_socket_t *ps, pi_buffer_t *buf, size_t expect,
		 int flags));

	extern void net_dump_header
	    PI_ARGS((unsigned char *data, int rxtx, int sd));
	extern void net_dump
	    PI_ARGS((unsigned char *header, unsigned char *data));

#ifdef __cplusplus
}
#endif
#endif
