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

	struct sockaddr;

	struct pi_protocol {
		int level;
		struct pi_protocol *(*dup) PI_ARGS((struct pi_protocol *));

		int (*read) PI_ARGS((struct pi_socket *, unsigned char *, int));
		int (*write) PI_ARGS((struct pi_socket *, unsigned char *, int));
	 	int (*getsockopt) PI_ARGS((struct pi_socket *, int, int, void *, int *));
		int (*setsockopt) PI_ARGS((struct pi_socket *, int, int, const void *, int *));

		void *data;
	};

	struct pi_device {
		struct pi_device *(*dup) PI_ARGS((struct pi_device *dev));
		struct pi_protocol *(*protocol) PI_ARGS((struct pi_device *dev));

		int (*bind) PI_ARGS((struct pi_socket *ps, struct sockaddr *addr, int addrlen));
		int (*listen) PI_ARGS((struct pi_socket *ps, int backlog));
		int (*accept) PI_ARGS((struct pi_socket *ps, struct sockaddr *addr, int *addrlen));
		int (*connect) PI_ARGS((struct pi_socket *ps, struct sockaddr *addr, int addrlen));
		int (*close) PI_ARGS((struct pi_socket *ps));

		void *data;
	};
	
	struct pi_socket {
		struct sockaddr *laddr;
		int laddrlen;
		struct sockaddr *raddr;
		int raddrlen;
		int type;
		int protocol;
		int cmd;
		int sd;
		int initiator;

		struct pi_protocol **protocol_queue;
		int queue_len;
		struct pi_protocol **cmd_queue;
		int cmd_len;

		struct pi_device *device;

		struct pi_skb *txq;
		struct pi_skb *rxq;

		int connected;		/* true on connected or accepted socket                            */
		int command;		/* true when socket in command state                               */
		int broken;		/* sth. went wrong so badly we cannot use this socket anymore      */
		int accept_to;		/* timeout value for call to accept()                              */
		int dlprecord;		/* Index used for some DLP functions */

#ifdef OS2
		unsigned short os2_read_timeout;
		unsigned short os2_write_timeout;
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
	    PI_ARGS((int type, const unsigned char *buf, int len, int addr));
	extern void dumpdata
	    PI_ARGS((int type, const unsigned char *buf, int len));

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
