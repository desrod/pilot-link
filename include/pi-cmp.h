#ifndef _PILOT_CMP_H_
#define _PILOT_CMP_H_

#include "pi-args.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CommVersion_1_0 0x0100L
#define CommVersion_2_0 0x0101L

	struct cmp {
		unsigned char type;
		unsigned char flags;
		unsigned int version;
		int reserved;
		unsigned long baudrate;
	};

	extern int cmp_rx PI_ARGS((struct pi_socket * ps, struct cmp * c));
	extern int cmp_init PI_ARGS((struct pi_socket * ps, int baudrate));
	extern int cmp_abort PI_ARGS((struct pi_socket * ps, int reason));

	extern int cmp_wakeup
	    PI_ARGS((struct pi_socket * ps, int maxbaud));

	extern void cmp_dump PI_ARGS((unsigned char *cmp, int rxtx));

#ifdef __cplusplus
}
#endif
#endif				/* _PILOT_CMP_H_ */
