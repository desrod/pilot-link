#ifndef _PILOT_PADP_SLP_H_
#define _PILOT_PADP_SLP_H_

#include "pi-args.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SLP_RDCP_T 0
#define SLP_PADP_T 2
#define SLP_LOOP_T 3

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

	extern int slp_tx
	    PI_ARGS((struct pi_socket * ps, struct pi_skb * nskb,
		     int len));
	extern int slp_rx PI_ARGS((struct pi_socket * ps));

	extern void slp_dump PI_ARGS((struct pi_skb * skb, int rxtx));
	extern void dph PI_ARGS((unsigned char *d));

#ifdef __cplusplus
}
#endif
#endif				/* _PILOT_PADP_SLP_H_ */
