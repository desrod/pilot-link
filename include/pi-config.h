/* include/pi-config.h.  Generated automatically by configure.  */
/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Define if your <sys/time.h> declares struct tm.  */
/* #undef TM_IN_SYS_TIME */

/* Define if you have the strtoul function.  */
#define HAVE_STRTOUL 1

/* Define if you have the memcpy function.  */
#define HAVE_MEMCPY 1

/* Define if you have the memcpy function.  */
#define HAVE_MEMMOVE 1

/* Define if you have the strchr function.  */
#define HAVE_STRCHR 1

/* Define if you have the strdup function.  */
#define HAVE_STRDUP 1

/* Define if you have the atexit function.  */
#define HAVE_ATEXIT 1

/* Define if you have the dup2 function.  */
#define HAVE_DUP2 1

/* Define if you have the cfmakeraw function. */
#define HAVE_CFMAKERAW 1

/* Define if you have the cfsetspeed function. */
/* #undef HAVE_CFSETSPEED */

/* Define if you have the cfsetispeed function. */
#define HAVE_CFSETISPEED 1

/* Define if you have the cfsetospeed function. */
#define HAVE_CFSETOSPEED 1

/* Define if you have the sigaction function. */
#define HAVE_SIGACTION 1

/* Define if you have the putenv function. */
#define HAVE_PUTENV 1

/* Define if you have the cispeed and cospeed members of struct termios */
/* #undef TERMIOS_CSPEED */

/* Define if you have the sa_len member of struct sockaddr */
/* #undef HAVE_SA_LEN */

/* Define if you have the inet_aton function */
#define HAVE_INET_ATON 1

/* Define if you have the gethostname function */
#define HAVE_GETHOSTNAME 1

/* Define if you have the uname function */
#define HAVE_UNAME 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <malloc.h> header file.  */
#define HAVE_MALLOC_H 1

/* Define if you have the <memory.h> header file.  */
#define HAVE_MEMORY_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <strings.h> header file.  */
#define HAVE_STRINGS_H 1

/* Define if you have the <stdlib.h> header file.  */
#define HAVE_STDLIB_H 1

/* Define if you have the <sys/ioctl.h> header file.  */
#define HAVE_SYS_IOCTL_H 1

/* Define if you have the <sys/select.h> header file.  */
/* #undef HAVE_SYS_SELECT_H */

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <sys/ioctl_compat.h> header file. */
/* #undef HAVE_SYS_IOCTL_COMPAT_H */

/* Define if you have the <netinet/in.h> header file. */
#define HAVE_NETINET_IN_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the <dirent.h> header file.  */
#define HAVE_DIRENT_H 1

/* Define if you have the <sys/ndir.h> header file.  */
/* #undef HAVE_SYS_NDIR_H */

/* Define if you have the <sys/dir.h> header file.  */
#define HAVE_SYS_DIR_H 1

/* Define if you have the <ndir.h> header file.  */
/* #undef HAVE_NDIR_H */

/* Define if you have the <sockio.h> header file.  */
/* #undef HAVE_SOCKIO_H */

/* Define if you have the <netdb.h> header file.  */
#define HAVE_NETDB_H 1

/* Define if you have the <sys/utsname.h> header file.  */
#define HAVE_SYS_UTSNAME_H 1

#ifdef NeXT
# include <libc.h>
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_MEMORY_H
# include <memory.h>
#endif

#ifdef HAVE_MALLOC_H
# include <malloc.h>
#endif

#ifdef HAVE_STRING_H
# include <string.h>
#endif

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#ifdef HAVE_SOCKIO_H
# include <sockio.h>
#endif

#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif

#ifdef HAVE_SYS_UTSNAME_H
# include <sys/utsname.h>
#endif

#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif

#if defined(HAVE_DIRENT_H) && !defined(NeXT)
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

#ifndef HAVE_STRTOUL
# define strtoul(str,ptr,base) (unsigned long)strtol((str),(ptr),(base))
#endif

#ifndef HAVE_STRCHR
# define strchr index
# define strrchr rindex
#endif
#ifndef HAVE_MEMCPY
# define memcpy(d, s, n) bcopy ((s), (d), (n))
#endif
#ifndef HAVE_MEMMOVE
# define memmove(d, s, n) bcopy ((s), (d), (n))
#endif

#ifndef HAVE_ATEXIT
# define atexit(x) on_exit((x),NULL)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HAVE_STRDUP
char * strdup(const char *string);
# ifndef bzero
#  define bzero(b,len) memset(b,0,len)
# endif
#endif

#ifndef HAVE_PUTENV
int putenv(const char *string);
#endif

#ifdef __cplusplus
}
#endif

#include <errno.h>

#ifndef ENOMSG
# define ENOMSG 1024
#endif

#if defined(linux) || defined(__FreeBSD__)
# define TTYPrompt "/dev/cua??"
#else
# define TTYPrompt "/dev/tty??"
#endif

#ifdef NeXT
# define SGTTY
#endif

/* Include internet headers */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifndef NeXT
#include <netdb.h>
#endif
