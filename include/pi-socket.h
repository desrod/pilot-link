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
#define PI_PF_SYS		0x03
#define PI_PF_PADP		0x04
#define PI_PF_NET		0x05
#define PI_PF_DLP		0x06

#define PI_SOCK_STREAM		0x0010
#define PI_SOCK_RAW		0x0030

#define PI_CMD_CMP 0x01
#define PI_CMD_NET 0x02
#define PI_CMD_SYS 0x03

#define PI_MSG_PEEK 0x01

enum PiOptLevels {
	PI_LEVEL_DEV,
	PI_LEVEL_SLP,
	PI_LEVEL_PADP,
	PI_LEVEL_NET,
	PI_LEVEL_SYS,
	PI_LEVEL_CMP,
	PI_LEVEL_DLP,
	PI_LEVEL_SOCK
};

enum PiOptDevice {
	PI_DEV_RATE,
	PI_DEV_ESTRATE,
	PI_DEV_HIGHRATE,
	PI_DEV_TIMEOUT
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
	PI_PADP_LASTTYPE
};

enum PiOptCMP {
	PI_CMP_TYPE,
	PI_CMP_FLAGS,
	PI_CMP_VERS,
	PI_CMP_BAUD
};

enum PiOptNet {
	PI_NET_TYPE
};

enum PiOptSock {
	PI_SOCK_STATE
};

#include "pi-sockaddr.h"


	struct pi_socket;

	struct sockaddr;

	extern int pi_socket PI_ARGS((int domain, int type, int protocol));
	extern struct pi_socket *pi_socket_copy PI_ARGS((struct pi_socket *ps));
	extern int pi_socket_setsd PI_ARGS((struct pi_socket *ps, int sd));

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
