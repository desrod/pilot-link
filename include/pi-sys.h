#ifndef _PILOT_SYSPKT_H
#define _PILOT_SYSPKT_H

#include "pi-args.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PI_SYS_HEADER_LEN  0

	struct pi_sys_data 
	{
		unsigned char txid;
	};

	extern struct pi_protocol *sys_protocol
	    PI_ARGS((void));

	extern int sys_tx
	    PI_ARGS((struct pi_socket *ps, unsigned char *buf, int len, int flags));
	extern int sys_rx
	    PI_ARGS((struct pi_socket *ps, unsigned char *buf, int len, int flags));

	extern void sys_dump_header
	    PI_ARGS((unsigned char *data, int rxtx));
	extern void sys_dump
	    PI_ARGS((unsigned char *data, int len));

#ifdef __cplusplus
}
#endif
#endif				/*_PILOT_SYSPKT_H_*/
