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
		struct pi_skb *next;
		int len;
		unsigned char source, dest, type, id;
		unsigned char data[PI_SLP_MTU];
	};

	struct pi_mac {
		int fd;
		int state;
		int expect;
		int ref;
		struct pi_skb *rxb;
		unsigned char *buf;
	};

	struct sockaddr;

	struct pi_socket {
		struct sockaddr *laddr;
		int laddrlen;
		struct sockaddr *raddr;
		int raddrlen;
		int type;
		int protocol;
		unsigned char xid;
		unsigned char nextid;
		int sd;
		int initiator;
		struct pi_mac *mac;
#ifndef WIN32
#ifndef OS2
# ifndef SGTTY
		struct termios tco;
# else
		struct sgttyb tco;
# endif
#endif
#endif
		struct pi_skb *txq;
		struct pi_skb *rxq;
		struct pi_socket *next;
		int rate;		/* Current port baud rate                                               */
		int establishrate;	/* Baud rate to use after link is established                           */
		int establishhighrate;	/* Boolean: try to establish rate higher than the device publishes      */
		int connected;		/* true on connected or accepted socket                                 */
		int accepted;		/* only true on accepted socket                                         */
		int broken;		/* sth. went wrong so badly we cannot use this socket anymore           */
		int accept_to;		/* timeout value for call to accept()                                   */
		int majorversion;
		int minorversion;
		int tickle;
		int busy;
		int version;		/* In form of 0xAABB where AA is major version and BB is minor version  */
		int dlprecord;		/* Index used for some DLP functions */
		int tx_packets;
		int rx_packets;
		int tx_bytes;
		int rx_bytes;
		int tx_errors;
		int rx_errors;
		char last_tid;
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
		char *debuglog;
		int debugfd;
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
