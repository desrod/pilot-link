#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef NeXT
# include <libc.h>
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
char 	*strdup(const char *string);
# ifndef bzero
#  define bzero(b,len) memset(b,0,len)
# endif
#endif

#ifndef HAVE_PUTENV
int 	putenv(const char *string);
#endif

#ifdef __cplusplus
}
#endif

#include <errno.h>

#ifndef ENOMSG
# define ENOMSG 1024
#endif

#if defined(__FreeBSD__)
# define TTYPrompt "/dev/cua[<0..n>]"
#else
# define TTYPrompt "/dev/tty[<0..n>]"
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
