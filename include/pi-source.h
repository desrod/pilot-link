#ifndef _PILOT_SOURCE_H_
#define _PILOT_SOURCE_H_

#ifdef NeXT
# include <sys/types.h>
# include <sys/socket.h>
#endif

#ifdef __EMX__
# define OS2
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/param.h>		/* for htonl .. */
# define ENOMSG 150
# define strcasecmp stricmp
# define strncasecmp strnicmp

# include <sys/ioctl.h>
# include <sys/time.h>
# include <sys/errno.h>
# include <time.h>
# include <fcntl.h>
# include <unistd.h>
# include <string.h>
# include <stdlib.h>
# include <dirent.h>
# include <errno.h>
# include <assert.h>

# define TTYPrompt "com#"
# define RETSIGTYPE void
# define HAVE_SIGACTION
# define HAVE_DUP2
# define HAVE_SYS_SELECT_H
# define HAVE_STRDUP
#else
#ifdef WIN32
# include <time.h>
# include <string.h>
# include <stdlib.h>
# include <errno.h>
# define RETSIGTYPE void
# define SIGALRM 14
# define ENOMSG 1024
# define EMSGSIZE 1025
# define ETIMEDOUT 1026
# define ECONNREFUSED 1027
# define EOPNOTSUPP 1028
#define HAVE_DUP2

#else
#include "pi-config.h"
#endif
#endif

#ifndef WIN32
#ifdef SGTTY
# include <sgtty.h>
#else
# include <termios.h>
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "pi-socket.h"
#include "pi-macros.h"

#define PI_SLP_MTU 1038

	struct pi_skb {
		int 	len;
		struct 	pi_skb *next;
		unsigned char source, dest, type, id;
		unsigned char data[PI_SLP_MTU];
	};

	struct pi_mac {
		int 	fd,
			state,
			expect,
			ref;
		struct 	pi_skb *rxb;
		unsigned char *buf;
	};

	struct sockaddr;

	struct pi_socket {
		int 	laddrlen,
			raddrlen,
			type,
			protocol,
			sd,
			initiator;
		unsigned char xid;
		unsigned char nextid;
		struct 	sockaddr *laddr;
		struct 	sockaddr *raddr;
		struct 	pi_mac *mac;
#ifndef WIN32
#ifndef OS2
# ifndef SGTTY
		struct 	termios tco;
# else
		struct 	sgttyb tco;
# endif
#endif
#endif
		struct 	pi_skb *txq;
		struct 	pi_skb *rxq;
		struct 	pi_socket *next;
		int 	rate, 		   /* Current port baud rate */
			establishrate,	   /* Baud rate to use after link is established */
			establishhighrate, /* Boolean: try to establish rate higher than the device publishes */
			connected, 	   /* true on connected or accepted socket */
			accepted,	   /* only true on accepted socket */
			broken,		   /* sth. went wrong so badly we cannot use this socket anymore */
			accept_to,	   /* timeout value for call to accept() */
			majorversion,
			minorversion,
			tickle,
			busy,
			version, 	   /* In form of 0xAABB where AA is major version and BB is minor version  */
			dlprecord,	   /* Index used for some DLP functions */
			tx_packets,
			rx_packets,
			tx_bytes,
			rx_bytes,
			tx_errors,
			rx_errors;
		char 	last_tid;

		int (*socket_connect)
			PI_ARGS((struct pi_socket *, struct sockaddr *, int));
		int (*socket_listen) PI_ARGS((struct pi_socket *, int));
		int (*socket_accept)
			PI_ARGS((struct pi_socket *, struct sockaddr *, int *));
		int (*socket_close) PI_ARGS((struct pi_socket *));
		int (*socket_tickle) PI_ARGS((struct pi_socket *));
		int (*socket_bind)
			PI_ARGS((struct pi_socket *, struct sockaddr *, int));
		int (*socket_send)
			PI_ARGS((struct pi_socket *, void *buf, int len, 
				unsigned int flags));
		int (*socket_recv)
			PI_ARGS((struct pi_socket *, void *buf, int len,
				unsigned int flags));
		int (*serial_close) PI_ARGS((struct pi_socket *));
		int (*serial_changebaud) PI_ARGS((struct pi_socket *));
		int (*serial_write) PI_ARGS((struct pi_socket *));
		int (*serial_read) PI_ARGS((struct pi_socket *, int));
#ifdef OS2
		unsigned short os2_read_timeout;
		unsigned short os2_write_timeout;
#endif
#ifndef NO_SERIAL_TRACE
		int 	debugfd;
		char 	*debuglog;

#endif
	};

	/* internal functions */

#include "pi-args.h"

	extern void pi_socket_recognize PI_ARGS((struct pi_socket *));
	extern struct pi_socket *find_pi_socket PI_ARGS((int sd));
	extern int crc16 PI_ARGS((unsigned char *ptr, int count));
	extern char *printlong PI_ARGS((unsigned long val));
	extern unsigned long makelong PI_ARGS((char *c));
	extern void dumpline
		PI_ARGS((const unsigned char *buf, int len, int addr));
	extern void dumpdata PI_ARGS((const unsigned char *buf, int len));

#if defined(PADP_TRACE)
#define Begin(a) fprintf(stderr,"Begin %s\n",#a)
#define At(a)    fprintf(stderr,"At %s\n",#a)
#define End(a)   fprintf(stderr,"End %s\n",#a)
#else
#define Begin(a)
#define At(a)
#define End(a)
#endif

#ifdef __cplusplus
}
#endif
#endif				/* _PILOT_SOCKET_H_ */
