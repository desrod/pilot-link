/*
 * socket.c:  Berkeley sockets style interface to Pilot SLP/PADP
 *
 * Copyright (c) 1996, D. Jeff Dionne.
 * Copyright (c) 1997-1999, Kenneth Albanowski
 * Copyright (c) 1999, Tilo Christ
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef WIN32
#include <winsock.h>
#include <io.h>
#else
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-serial.h"
#include "pi-inetserial.h"
#include "pi-inet.h"
#include "pi-padp.h"
#include "pi-cmp.h"
#include "pi-dlp.h"
#include "pi-syspkt.h"

#ifdef WIN32
/* An implementation of alarm for windows*/
#include <process.h>
static long alm_countdown = -1;
static void *alm_tid = 0;

/***********************************************************************
 *
 * Function:    alarm_thread
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void alarm_thread(void *unused)
{
	long av;

	Sleep(1000L);
	av = InterlockedDecrement(&alm_countdown);
	if (av == 0) {
		raise(SIGALRM);
	}
	if (av <= 0) {
		alm_tid = 0;
		ExitThread(0);
	}
}

/***********************************************************************
 *
 * Function:    alarm
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
unsigned alarm(unsigned sec)
{
	long ret = alm_countdown;

	if (sec) {
		alm_countdown = sec;
		if (!alm_tid) {
			unsigned long t;

			//not multi thread safe -- fine if you just call alarm from one thread
			alm_tid =
			    CreateThread(0, 0,
					 (LPTHREAD_START_ROUTINE)
					 alarm_thread, 0, 0, &t);
		}
	} else {
		alm_countdown = -1;
	}
	return ret > 0 ? ret : 0;
}
#endif

static struct pi_socket *psl = (struct pi_socket *) 0;

void installexit(void);

extern int dlp_trace;

static RETSIGTYPE pi_serial_onalarm(int signo);

/* Automated tickling interval */
static int interval = 0;
static int busy = 0;

/***********************************************************************
 *
 * Function:    default_socket_connect
 *
 * Summary:     Connect to the socket passed in the descriptor
 *
 * Parmeters:   None
 *
 * Returns:     -1
 *
 ***********************************************************************/
static int
default_socket_connect(struct pi_socket *ps, struct sockaddr *addr,
		       int flag)
{
	errno = ENOSYS;
	return -1;
}

/***********************************************************************
 *
 * Function:    default_socket_listen
 *
 * Summary:     Listen to the socket passed to the call
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int default_socket_listen(struct pi_socket *ps, int flag)
{
	errno = ENOSYS;
	return -1;
}

/***********************************************************************
 *
 * Function:    default_socket_accept
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int
default_socket_accept(struct pi_socket *ps, struct sockaddr *addr,
		      int *flag)
{
	errno = ENOSYS;
	return -1;
}

/***********************************************************************
 *
 * Function:    default_socket_close
 *
 * Summary:     Close the open socket
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int default_socket_close(struct pi_socket *ps)
{
	return 0;
}

/***********************************************************************
 *
 * Function:    default_socket_tickle
 *
 * Summary:     Keep the socket open with a tickle packet
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int default_socket_tickle(struct pi_socket *ps)
{
	errno = ENOSYS;
	return -1;
}

/***********************************************************************
 *
 * Function:    default_socket_bind
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int
default_socket_bind(struct pi_socket *ps, struct sockaddr *addr, int flag)
{
	errno = ENOSYS;
	return -1;
}

/***********************************************************************
 *
 * Function:    default_socket_send
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int
default_socket_send(struct pi_socket *ps, void *buf, int len,
		    unsigned int flags)
{
	errno = ENOSYS;
	return -1;
}

/***********************************************************************
 *
 * Function:    default_socket_recv
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int
default_socket_recv(struct pi_socket *ps, void *buf, int len,
		    unsigned int flags)
{
	errno = ENOSYS;
	return -1;
}

/***********************************************************************
 *
 * Function:    pi_socket
 *
 * Summary:     Create a local connection endpoint
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_socket(int domain, int type, int protocol)
{
	struct pi_socket *ps;

	if (protocol == 0) {
		if (type == PI_SOCK_STREAM)
			protocol = PI_PF_PADP;
		else if (type == PI_SOCK_RAW)
			protocol = PI_PF_SLP;
	}

	if (((domain != PI_AF_SLP) && (domain != AF_INET))
	    || ((type != PI_SOCK_STREAM) && (type != PI_SOCK_RAW))
	    || ((protocol != PI_PF_PADP) && (protocol != PI_PF_SLP))) {	/* FIXME:  Need to support more */
		errno = EINVAL;
		return -1;
	}

	ps = calloc(sizeof(struct pi_socket), 1);

#if defined( OS2 ) || defined( WIN32 )
	if ((ps->sd = open("NUL", O_RDWR)) == -1) {
#else
	if ((ps->sd = open("/dev/null", O_RDWR)) == -1) {
#endif
		int err = errno;	/* Save errno of open */

		free(ps);
		errno = err;
		return -1;
	}
	ps->mac = calloc(1, sizeof(struct pi_mac));

	ps->type = type;
	ps->protocol = protocol;
	ps->connected = 0;
	ps->accepted = 0;
	ps->broken = 0;
	ps->mac->fd = 0;
	ps->mac->ref = 1;
	ps->xid = 0xff;
	ps->initiator = 0;
	ps->minorversion = 0;
	ps->majorversion = 0;
	ps->version = 0;
	ps->dlprecord = 0;
	ps->busy = 0;

	ps->establishrate = -1;

	ps->socket_connect = default_socket_connect;
	ps->socket_listen = default_socket_listen;
	ps->socket_accept = default_socket_accept;
	ps->socket_close = default_socket_close;
	ps->socket_tickle = default_socket_tickle;
	ps->socket_bind = default_socket_bind;
	ps->socket_send = default_socket_send;
	ps->socket_recv = default_socket_recv;

#ifdef OS2
	ps->os2_read_timeout = 60;
	ps->os2_write_timeout = 60;
#endif

#ifndef NO_SERIAL_TRACE
	ps->debuglog = 0;
	ps->debugfd = 0;

	if (getenv("PILOTLOG")) {
		if ((ps->debuglog = getenv("PILOTLOGFILE")) == 0)
			ps->debuglog = "PiDebug.log";
	}
#endif

#ifndef NO_DLP_TRACE
	if (getenv("PILOTDLP")) {
		dlp_trace = 1;
	}
#endif

	installexit();

	pi_socket_recognize(ps);

	return ps->sd;
}

/***********************************************************************
 *
 * Function:    pi_socket_recognize
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void pi_socket_recognize(struct pi_socket *ps)
{
	struct pi_socket *p;

	if (!psl)
		psl = ps;
	else {
		for (p = psl; p->next; p = p->next);

		p->next = ps;
	}
}

/***********************************************************************
 *
 * Function:    pi_connect
 *
 * Summary:     Connect to a remote server
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_connect(int pi_sd, struct sockaddr *addr, int addrlen)
{
	struct pi_socket *ps;
	struct pi_sockaddr *pa = (struct pi_sockaddr *) addr;
	enum { inet, serial } conn;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}

	conn = serial;

	if (addr->sa_family == PI_AF_SLP) {
		if (pa->pi_device[0] == '.')
			conn = inet;
		else
			conn = serial;
	} else if (addr->sa_family == AF_INET)
		conn = inet;
	else if (addr->sa_family == PI_AF_INETSLP)
		conn = inet;

	if (conn == serial)
		return pi_serial_connect(ps, addr, addrlen);
	else if (conn == inet)
#ifdef _PILOT_INET_H_
		return pi_inet_connect(ps, addr, addrlen);
#else
		return -1;
#endif

	return -1;
}

/***********************************************************************
 *
 * Function:    pi_bind
 *
 * Summary:     Bind address to a local socket
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_bind(int pi_sd, struct sockaddr *addr, int addrlen)
{
	struct pi_socket *ps;
	struct pi_sockaddr *pa = (struct pi_sockaddr *) addr;
	enum { inet, serial } conn;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}

	conn = serial;

	if (addr->sa_family == PI_AF_SLP) {
		if (pa->pi_device[0] == '.')
			conn = inet;
		else
			conn = serial;
	} else if (addr->sa_family == AF_INET)
		conn = inet;
	else if (addr->sa_family == PI_AF_INETSLP)
		conn = serial;

	if (conn == serial)
		return pi_serial_bind(ps, addr, addrlen);
	else if (conn == inet)
#ifdef _PILOT_INET_H_
		return pi_inet_bind(ps, addr, addrlen);
#else
		return -1;
#endif

	return -1;
}

/***********************************************************************
 *
 * Function:    pi_listen
 *
 * Summary:     Wait for an incoming connection
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_listen(int pi_sd, int backlog)
{
	struct pi_socket *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}

	return ps->socket_listen(ps, backlog);
}

/***********************************************************************
 *
 * Function:    pi_accept
 *
 * Summary:     Accept an incoming connection
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_accept(int pi_sd, struct sockaddr *addr, int *addrlen)
{
	struct pi_socket *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}

	ps->accept_to = 0;

	return ps->socket_accept(ps, addr, addrlen);
}

/***********************************************************************
 *
 * Function:    pi_accept_to
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_accept_to(int pi_sd, struct sockaddr *addr, int *addrlen, int timeout)
{
	struct pi_socket *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}

	ps->accept_to = timeout;

	return ps->socket_accept(ps, addr, addrlen);
}

/***********************************************************************
 *
 * Function:    pi_setmaxspeed
 *
 * Summary:     Set the maximum connection speed of the socket
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_setmaxspeed(int pi_sd, int speed, int overclock)
{
	struct pi_socket *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}

	if (ps->connected) {
		errno = EBUSY;	/* EISCONN might be better, but is not available on all OSes. */
		return -1;
	}

	ps->establishrate = speed;
	ps->establishhighrate = overclock;

	return 0;
}

/***********************************************************************
 *
 * Function:    pi_getsockopt
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_getsockopt(int pi_sd, int level, int option_name, void *option_value,
	      int *option_len)
{
	struct pi_socket *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}

	if (level != PI_PF_SLP) {
		errno = EINVAL;
		return -1;
	}

	if (option_name == PI_SLP_SPEED) {
		int speed = ps->rate;
		memcpy(option_value, &speed,
		       (*option_len <
			sizeof(int)) ? *option_len : sizeof(int));
		if (*option_len > sizeof(int))
			*option_len = sizeof(int);

		return 0;
	}

	errno = EINVAL;
	return -1;
}

/***********************************************************************
 *
 * Function:    pi_send
 *
 * Summary:     Send message on a connected socket
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_send(int pi_sd, void *msg, int len, unsigned int flags)
{
	struct pi_socket *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}

	if (interval)
		alarm(interval);

	return ps->socket_send(ps, msg, len, flags);
}

/***********************************************************************
 *
 * Function:    pi_recv
 *
 * Summary:     Receive msg on a connected socket
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_recv(int pi_sd, void *msg, int len, unsigned int flags)
{
	struct pi_socket *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}

	return ps->socket_recv(ps, msg, len, flags);
}

/***********************************************************************
 *
 * Function:    pi_read
 *
 * Summary:     Wrapper for receive
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_read(int pi_sd, void *msg, int len)
{
	return pi_recv(pi_sd, msg, len, 0);
}


/***********************************************************************
 *
 * Function:    pi_write
 *
 * Summary:     Wrapper for send
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_write(int pi_sd, void *msg, int len)
{
	return pi_send(pi_sd, msg, len, 0);
}

/***********************************************************************
 *
 * Function:    pi_tickle
 *
 * Summary:     Tickle a stream connection
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_tickle(int pi_sd)
{
	struct pi_socket *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}

	return ps->socket_tickle(ps);
}

/***********************************************************************
 *
 * Function:    pi_close
 *
 * Summary:     Close a connection, destroy the socket
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_close(int pi_sd)
{
	int result;
	struct pi_socket *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}

	busy++;
	result = ps->socket_close(ps);
	busy--;

	if (result == 0) {
		if (ps == psl) {
			psl = psl->next;
		} else {
			struct pi_socket *p;

			for (p = psl; p; p = p->next) {
				if (ps == p->next) {
					p->next = p->next->next;
					break;
				}
			}
		}
		free(ps);
	}

	return result;
}

/***********************************************************************
 *
 * Function:    pi_onexit
 *
 * Summary:     Install an atexit handler that closes open sockets
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void pi_onexit(void)
{
	struct pi_socket *p, *n;

	for (p = psl; p; p = n) {
		n = p->next;
		if (p->socket_close) {
			pi_close(p->sd);
		}
	}

}

/***********************************************************************
 *
 * Function:    installexit
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void installexit(void)
{
	static int installedexit = 0;

	if (!installedexit)
		atexit(pi_onexit);

	installedexit = 1;
}

/***********************************************************************
 *
 * Function:    pi_getsockname
 *
 * Summary:     Get the local address for a socket
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_getsockname(int pi_sd, struct sockaddr *addr, int *namelen)
{
	struct pi_socket *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}

	if (*namelen > ps->laddrlen)
		*namelen = ps->laddrlen;
	memcpy(addr, &ps->laddr, *namelen);

	return 0;
}

/***********************************************************************
 *
 * Function:    pi_getsockpeer
 *
 * Summary:     Get the remote address for a socket
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_getsockpeer(int pi_sd, struct sockaddr *addr, int *namelen)
{
	struct pi_socket *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}

	if (*namelen > ps->raddrlen)
		*namelen = ps->raddrlen;
	memcpy(addr, &ps->raddr, *namelen);

	return 0;
}

/***********************************************************************
 *
 * Function:    pi_version
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_version(int pi_sd)
{
	struct pi_socket *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}

	return ps->version;
}

/***********************************************************************
 *
 * Function:    pi_socket
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
struct pi_socket *find_pi_socket(int sd)
{
	struct pi_socket *p;

	for (p = psl; p; p = p->next) {
		if (p->sd == sd)
			return p;
	}

	return 0;
}

/***********************************************************************
 *
 * Function:    pi_watchdog
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_watchdog(int pi_sd, int newinterval)
{
	struct pi_socket *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}

	ps->tickle = 1;
	signal(SIGALRM, pi_serial_onalarm);
	interval = newinterval;
	alarm(interval);
	return 0;
}

/***********************************************************************
 *
 * Function:    pi_serial_onalarm
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static RETSIGTYPE pi_serial_onalarm(int signo)
{
	struct pi_socket *p, *n;

	signal(SIGALRM, pi_serial_onalarm);

	if (busy) {
#ifdef DEBUG
		fprintf(stderr, "world is busy. Rescheduling.\n");
#endif
		alarm(1);
	} else
		for (p = psl; p; p = n) {
			n = p->next;
			if (p->connected) {
#ifdef DEBUG
				fprintf(stderr, "Tickling socket %d\n",
					p->sd);
#endif
				if (pi_tickle(p->sd) == -1) {
#ifdef DEBUG
					fprintf(stderr,
						" but socket is busy. Rescheduling.\n");
#endif
					alarm(1);
				} else
					alarm(interval);
			}
		}
}
