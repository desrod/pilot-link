#ifndef _PILOT_SYSPKT_H
#define _PILOT_SYSPKT_H

#include "pi-args.h"
#include "pi-buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PI_SYS_HEADER_LEN  0

	typedef struct pi_sys_data 
	{
		unsigned char txid;
	} pi_sys_data_t;

	extern pi_protocol_t *sys_protocol
	    PI_ARGS((void));

	extern ssize_t sys_tx
	  PI_ARGS((pi_socket_t *ps, unsigned char *buf,
		 size_t len, int flags));
	extern ssize_t sys_rx
	  PI_ARGS((pi_socket_t *ps, pi_buffer_t *buf,
		 size_t len, int flags));

	extern void sys_dump_header
	    PI_ARGS((unsigned char *data, int rxtx));
	extern void sys_dump
	    PI_ARGS((unsigned char *data, size_t len));

#ifdef __cplusplus
}
#endif
#endif				/*_PILOT_SYSPKT_H_*/
