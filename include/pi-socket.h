#ifndef _PILOT_SOCKET_H_
#define _PILOT_SOCKET_H_

#include <unistd.h>

#include "pi-args.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "pi-version.h"
#include "pi-sockaddr.h"
#include "pi-buffer.h"

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

#define PI_MSG_PEEK 	0x01
#define	PI_MSG_REALLOC	0x02

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
	PI_NET_TYPE,
	PI_NET_SPLIT_WRITES,	/* if set, write separately the NET header and data */
	PI_NET_WRITE_CHUNKSIZE	/* size of data chunks if PI_NET_SPLIT_WRITES is set. 0 for no chunking of data */
};

enum PiOptSock {
	PI_SOCK_STATE
};

struct	pi_protocol;	/* forward declaration */

typedef struct pi_socket {
	int sd;

	int type;
	int protocol;
	int cmd;

	struct sockaddr *laddr;
	size_t laddrlen;
	struct sockaddr *raddr;
	size_t raddrlen;

	struct pi_protocol **protocol_queue;
	int queue_len;
	struct pi_protocol **cmd_queue;
	int cmd_len;
	struct pi_device *device;

	int state;
	int command;	/* true when socket in command state  */
	int accept_to;	/* timeout value for call to accept() */
	int dlprecord;	/* Index used for some DLP functions */

	int dlpversion;	/* version of the DLP protocol running on the device */
	unsigned long maxrecsize;/* max record size on the device */

	int last_error;		/* error code returned by the last dlp_* command */
	int palmos_error;	/* Palm OS error code returned by the last transaction with the handheld */

#ifdef OS2
	unsigned short os2_read_timeout;
	unsigned short os2_write_timeout;
#endif
} pi_socket_t;

typedef struct pi_socket_list
{
	pi_socket_t *ps;
	struct pi_socket_list *next;
} pi_socket_list_t;

	extern int pi_socket PI_ARGS((int domain, int type, int protocol));
	extern int pi_socket_setsd PI_ARGS((pi_socket_t *ps, int pi_sd));

	extern int pi_connect
	    PI_ARGS((int pi_sd, const char *port));
	extern int pi_bind
	    PI_ARGS((int pi_sd, const char *port));
	extern int pi_listen PI_ARGS((int pi_sd, int backlog));
	extern int pi_accept
	    PI_ARGS((int pi_sd, struct sockaddr * remote_addr,
		     size_t *addrlen));

	extern int pi_accept_to
	    PI_ARGS((int pi_sd, struct sockaddr * addr, size_t *addrlen,
		     int timeout));

	extern int pi_send
	    PI_ARGS((int pi_sd, void *msg, size_t len, int flags));
	extern ssize_t pi_recv
	    PI_ARGS((int pi_sd, pi_buffer_t *msg, size_t len, int flags));

	extern ssize_t pi_read PI_ARGS((int pi_sd, pi_buffer_t *msg, size_t len));
	extern ssize_t pi_write PI_ARGS((int pi_sd, void *msg, size_t len));
	extern void pi_flush PI_ARGS((int pi_sd, int flags));

	extern int pi_getsockname
	    PI_ARGS((int pi_sd, struct sockaddr * addr, size_t *namelen));
	extern int pi_getsockpeer
	    PI_ARGS((int pi_sd, struct sockaddr * addr, size_t *namelen));

	extern int pi_getsockopt
	    PI_ARGS((int pi_sd, int level, int option_name,
		     void *option_value, size_t *option_len));
	extern int pi_setsockopt
	    PI_ARGS((int pi_sd, int level, int option_name, 
		     const void *option_value, size_t *option_len));

	extern int pi_socket_connected
		PI_ARGS((int pi_sd));

	extern int pi_version PI_ARGS((int pi_sd));
	extern unsigned long pi_maxrecsize PI_ARGS((int pi_sd));

	extern int pi_tickle PI_ARGS((int pi_sd));
	extern int pi_watchdog PI_ARGS((int pi_sd, int interval));

	extern int pi_close PI_ARGS((int pi_sd));

	extern struct pi_protocol *pi_protocol
	    PI_ARGS((int pi_sd, int level));
	extern struct pi_protocol *pi_protocol_next
	    PI_ARGS((int pi_sd, int level));
	
	extern int pi_error
		PI_ARGS((int pi_sd));
	extern int pi_set_error
		PI_ARGS((int pi_sd, int error_code));
	extern int pi_palmos_error
		PI_ARGS((int pi_sd));
	extern int pi_set_palmos_error
		PI_ARGS((int pi_sd, int error_code));
	extern void pi_reset_errors
		PI_ARGS((int sd));

#ifdef __cplusplus
}
#endif
#endif				/* _PILOT_SOCKET_H_ */
