#ifndef _PILOT_SOURCE_H_
#define _PILOT_SOURCE_H_

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

# define TTYPrompt "com#"
# define RETSIGTYPE void
# define HAVE_SIGACTION
# define HAVE_DUP2
# define HAVE_SYS_SELECT_H
# define HAVE_STRDUP
#else
#endif

#ifdef SGTTY
# include <sgtty.h>
#else
# include <termios.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "pi-socket.h"
#include "pi-macros.h"
#include "pi-buffer.h"

#define PI_SOCK_LISTN  0x01  /* Listener */
#define PI_SOCK_CONAC  0x02  /* Connected by accepting */
#define PI_SOCK_CONIN  0x04  /* Connected by initiating */
#define PI_SOCK_CONBK  0x08  /* Connected but broken */
#define PI_SOCK_CONEN  0x10  /* Connected but end */
#define PI_SOCK_CLOSE  0x20  /* Closed */

#define PI_FLUSH_INPUT	0x01	/* for flush() */
#define	PI_FLUSH_OUTPUT	0x02	/* for flush() */

	typedef struct pi_protocol {
		int level;
		struct pi_protocol *(*dup)
			PI_ARGS((struct pi_protocol *));
		void (*free)
			PI_ARGS((struct pi_protocol *));
		ssize_t	(*read)
			PI_ARGS((pi_socket_t *ps, pi_buffer_t *buf,
				size_t expect, int flags));
		ssize_t	(*write)
			PI_ARGS((pi_socket_t *ps, unsigned char *buf,
				size_t len, int flags));
		int (*flush)
			PI_ARGS((pi_socket_t *ps, int flags));
	 	int (*getsockopt)
			PI_ARGS((pi_socket_t *ps, int level,
				int option_name, void *option_value,
					size_t *option_len));
		int (*setsockopt)
			PI_ARGS((pi_socket_t *ps, int level,
				int option_name, const void *option_value,
					size_t *option_len));
		void *data;
	} pi_protocol_t;

	typedef struct pi_device {
		void (*free)
			PI_ARGS((struct pi_device *dev));
		struct pi_protocol *(*protocol)
			PI_ARGS((struct pi_device *dev));
		int (*bind)
			PI_ARGS((pi_socket_t *ps,
				struct sockaddr *addr, size_t addrlen));
		int (*listen)
			PI_ARGS((pi_socket_t *ps, int backlog));
		int (*accept)
			PI_ARGS((pi_socket_t *ps, struct sockaddr *addr,
				size_t *addrlen));
		int (*connect)
			PI_ARGS((pi_socket_t *ps, struct sockaddr *addr,
				size_t addrlen));
		int (*close)
			PI_ARGS((pi_socket_t *ps));
		void *data;
	} pi_device_t;
	
	/* internal functions */
	extern pi_socket_list_t *pi_socket_recognize PI_ARGS((pi_socket_t *));
	extern pi_socket_t *find_pi_socket PI_ARGS((int sd));
	extern int crc16 PI_ARGS((unsigned char *ptr, int count));
	extern char *printlong PI_ARGS((unsigned long val));
	extern unsigned long makelong PI_ARGS((char *c));

	extern void dumpline
	    PI_ARGS((const char *buf, size_t len, unsigned int addr));
	extern void dumpdata
	    PI_ARGS((const char *buf, size_t len));

#ifdef __cplusplus
}
#endif
#endif				/* _PILOT_SOURCE_H_ */
