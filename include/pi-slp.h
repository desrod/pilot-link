#ifndef _PILOT_PADP_SLP_H_
#define _PILOT_PADP_SLP_H_

#include "pi-args.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PI_SLP_HEADER_LEN  10
#define PI_SLP_FOOTER_LEN  2

#define PI_SLP_SIG_BYTE1 0xbe
#define PI_SLP_SIG_BYTE2 0xef
#define PI_SLP_SIG_BYTE3 0xed

#define PI_SLP_OFFSET_SIG1 0
#define PI_SLP_OFFSET_SIG2 1
#define PI_SLP_OFFSET_SIG3 2
#define PI_SLP_OFFSET_DEST 3
#define PI_SLP_OFFSET_SRC  4
#define PI_SLP_OFFSET_TYPE 5
#define PI_SLP_OFFSET_SIZE 6
#define PI_SLP_OFFSET_TXID 8
#define PI_SLP_OFFSET_SUM  9

#define PI_SLP_TYPE_RDCP 0x00
#define PI_SLP_TYPE_PADP 0x02
#define PI_SLP_TYPE_LOOP 0x03

	struct pi_slp_data 
	{
		int dest;
		int last_dest;
		int src;
		int last_src;
		
		int type;
		int last_type;
		
		unsigned char txid;
		unsigned char last_txid;
	};
	
	struct slp {
		unsigned char _be;
		unsigned char _ef;
		unsigned char _ed;
		unsigned char dest;
		unsigned char src;
		unsigned char type;
		unsigned short dlen;
		unsigned char id;
		unsigned char csum;
	};

	extern struct pi_protocol *slp_protocol
	    PI_ARGS((void));

	extern int slp_tx
	    PI_ARGS((struct pi_socket * ps, unsigned char *buf, int len));
	extern int slp_rx
	    PI_ARGS((struct pi_socket *ps, unsigned char *buf, int len));

	extern void slp_dump PI_ARGS((unsigned char * skb, int rxtx));
	extern void dph PI_ARGS((unsigned char *d));

#ifdef __cplusplus
}
#endif
#endif				/* _PILOT_PADP_SLP_H_ */
