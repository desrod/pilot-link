#ifndef _PILOT_SOCKET_H_
#define _PILOT_SOCKET_H_

#include "pi-args.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "pi-version.h"

#define PI_AF_PILOT             0x00

#define PI_PF_DEV               0x01
#define PI_PF_SLP		0x02
#define PI_PF_PADP		0x03
#define PI_PF_NET		0x04

#define PI_SOCK_STREAM		0x0010
#define PI_SOCK_RAW		0x0030

#define PI_INIT_CMP 0x01
#define PI_INIT_NET 0x02

enum PiOptLevels {
	PI_LEVEL_SOCKET,
	PI_LEVEL_SLP,
	PI_LEVEL_PADP,
	PI_LEVEL_NET,
	PI_LEVEL_CMP,
	PI_LEVEL_DLP
};

enum PiOptSocket {
	PI_SOCKET_RATE,
	PI_SOCKET_ESTRATE,
	PI_SOCKET_HIGHRATE,
	PI_SOCKET_TIMEOUT
};

enum PiOptSLP {
	PI_SLP_DEST,
	PI_SLP_LASTDEST,
	PI_SLP_SRC,
	PI_SLP_LASTSRC,
	PI_SLP_TYPE,
	PI_SLP_LASTTYPE,
	PI_SLP_TXID,
	PI_SLP_LASTTXID
};

enum PiOptPADP {
	PI_PADP_TYPE,
	PI_PADP_LASTTYPE,
};
	
#define PI_SLP_SPEED		0x0001

#define PI_PilotSocketDLP	3
#define PI_PilotSocketConsole	1
#define PI_PilotSocketDebugger	0
#define PI_PilotSocketRemoteUI	2

#ifdef WIN32
#include "pi-sockaddr-win32.h"
#else
#include "pi-sockaddr.h"
#endif

	struct pi_socket;

	struct sockaddr;

	extern int pi_socket PI_ARGS((int domain, int type, int protocol));
	extern struct pi_socket *pi_socket_copy PI_ARGS((struct pi_socket *ps));

	extern int pi_connect
	    PI_ARGS((int pi_sd, struct sockaddr * remote_addr,
		     int addrlen));
	extern int pi_bind
	    PI_ARGS((int pi_sd, struct sockaddr * my_addr, int addrlen));
	extern int pi_listen PI_ARGS((int pi_sd, int backlog));
	extern int pi_accept
	    PI_ARGS((int pi_sd, struct sockaddr * remote_addr,
		     int *addrlen));

	extern int pi_accept_to
	    PI_ARGS((int pi_sd, struct sockaddr * addr, int *addrlen,
		     int timeout));

	extern int pi_send
	    PI_ARGS((int pi_sd, void *msg, int len, unsigned int flags));
	extern int pi_recv
	    PI_ARGS((int pi_sd, void *msg, int len, unsigned int flags));

	extern int pi_read PI_ARGS((int pi_sd, void *msg, int len));
	extern int pi_write PI_ARGS((int pi_sd, void *msg, int len));

	extern int pi_getsockname
	    PI_ARGS((int pi_sd, struct sockaddr * addr, int *namelen));
	extern int pi_getsockpeer
	    PI_ARGS((int pi_sd, struct sockaddr * addr, int *namelen));

	extern int pi_getsockopt
	    PI_ARGS((int pi_sd, int level, int option_name,
		     void *option_value, int *option_len));
	extern int pi_setsockopt
	    PI_ARGS((int pi_sd, int level, int option_name, 
		     const void *option_value, int *option_len));

	extern int pi_version PI_ARGS((int pi_sd));

	extern int pi_tickle PI_ARGS((int pi_sd));
	extern int pi_watchdog PI_ARGS((int pi_sd, int interval));

	extern int pi_close PI_ARGS((int pi_sd));

	extern struct pi_protocol *pi_protocol
	    PI_ARGS((int pi_sd, int level));
	extern struct pi_protocol *pi_protocol_next
	    PI_ARGS((int pi_sd, int level));
	

#ifdef __cplusplus
}
#endif
#endif				/* _PILOT_SOCKET_H_ */
