/*
 * socket.c:  Berkeley sockets style interface to Pilot
 *
 * Copyright (c) 1996, D. Jeff Dionne.
 * Copyright (c) 1997-1999, Kenneth Albanowski
 * Copyright (c) 1999, Tilo Christ
 * Copyright (c) 2000-2001, JP Rosevear
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
 *
 * -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef WIN32
#include <winsock.h>
#include <io.h>
#else
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-serial.h"
#include "pi-inet.h"
#include "pi-slp.h"
#include "pi-padp.h"
#include "pi-cmp.h"
#include "pi-net.h"
#include "pi-dlp.h"
#include "pi-syspkt.h"
#include "pi-debug.h"

/* Linked List of Sockets */
struct pi_socket_list
{
	struct pi_socket *ps;
	struct pi_socket_list *next;
};

static struct pi_socket_list *psl = NULL;
static struct pi_socket_list *watch_list = NULL;

/* Automated tickling interval */
static int interval = 0;


/* Linked List Code */
static struct pi_socket_list *
ps_list_append (struct pi_socket_list *list, struct pi_socket *ps) 
{
	struct pi_socket_list *elem, *new_elem;
	
	ASSERT (ps != NULL)

	new_elem 	= malloc(sizeof(struct pi_socket_list));
	new_elem->ps 	= ps;
	new_elem->next 	= NULL;

	if (list == NULL)
		return new_elem;
	
	elem = list;
	while (elem->next != NULL)
		elem = elem->next;
	
	elem->next = new_elem;

	return list;
}

static struct pi_socket_list *
ps_list_prepend (struct pi_socket_list *list, struct pi_socket *ps) 
{
	struct pi_socket_list *new_elem;
	
	ASSERT (ps != NULL)

	new_elem 	= malloc(sizeof(struct pi_socket_list));
	new_elem->ps 	= ps;
	new_elem->next 	= list;

	return new_elem;
}

static struct pi_socket *
ps_list_find (struct pi_socket_list *list, int sd) 
{
	struct pi_socket_list *elem;
	
	for (elem = list; elem != NULL; elem = elem->next) {
		if (elem->ps->sd == sd)
			return elem->ps;
	}

	return NULL;
}

static struct pi_socket_list *
ps_list_remove (struct pi_socket_list *list, int sd) 
{
	struct pi_socket_list *elem, *new_list = list, *prev_elem;

	prev_elem = NULL;
	for (elem = list; elem != NULL; elem = elem->next) {
		if (elem->ps->sd == sd) {
			if (prev_elem == NULL)
				new_list = elem->next;
			else
				prev_elem->next = elem->next;
			free(elem);
			break;
		}
	}

	return new_list;
}

static struct pi_socket_list *
ps_list_copy (struct pi_socket_list *list) 
{
	struct pi_socket_list *l, *new_list = NULL;
	
	for (l = list; l != NULL; l = l->next)
		new_list = ps_list_append (new_list, l->ps);

	return new_list;
}

static void
ps_list_free (struct pi_socket_list *list)
{
	struct pi_socket_list *l, *next;

	if (list == NULL)
		return;
	
	l = list;
	do {
		next = l->next;
		free(l);
		l = next;
	} while (l != NULL);
}
	      
/* Protocol Queue */
static void
protocol_queue_add (struct pi_socket *ps, struct pi_protocol *prot)
{
	ps->protocol_queue = realloc(ps->protocol_queue, (sizeof(struct pi_protocol *)) * ps->queue_len + 1);
	ps->protocol_queue[ps->queue_len] = prot;
	ps->queue_len++;
}

static void
protocol_cmd_queue_add (struct pi_socket *ps, struct pi_protocol *prot)
{
	ps->cmd_queue = realloc(ps->cmd_queue, (sizeof(struct pi_protocol *)) * ps->cmd_len + 1);
	ps->cmd_queue[ps->cmd_len] = prot;
	ps->cmd_len++;
}

static struct pi_protocol *
protocol_queue_find (struct pi_socket *ps, int level) 
{
	int 	i;

	if (ps->command) {
		for (i = 0; i < ps->cmd_len; i++) {
			if (ps->cmd_queue[i]->level == level)
				return ps->cmd_queue[i];
		}
	} else {
		for (i = 0; i < ps->queue_len; i++) {
			if (ps->protocol_queue[i]->level == level)
				return ps->protocol_queue[i];
		}
	}

	return NULL;
}

static struct pi_protocol *
protocol_queue_find_next (struct pi_socket *ps, int level) 
{
	int 	i;

	if (ps->command && ps->cmd_len == 0)
		return NULL;
	else if (!ps->command && ps->queue_len == 0)
		return NULL;
	if (ps->command && level == 0)
		return ps->cmd_queue[0];
	else if (!ps->command && level == 0)
		return ps->protocol_queue[0];
	
	if (ps->command) {
		for (i = 0; i < ps->cmd_len - 1; i++) {
			if (ps->cmd_queue[i]->level == level)
				return ps->cmd_queue[i + 1];
		}
	} else {
		for (i = 0; i < ps->queue_len - 1; i++) {
			if (ps->protocol_queue[i]->level == level)
				return ps->protocol_queue[i + 1];
		}
	}

	return NULL;
}

static void
protocol_queue_build (struct pi_socket *ps, int autodetect) 
{
	int 	protocol;
	struct 	pi_protocol *prot, *dev_prot, *dev_cmd_prot;
	unsigned char byte;
	
	/* The device protocol */
	dev_prot 	= ps->device->protocol (ps->device);
	dev_cmd_prot 	= ps->device->protocol (ps->device);

	protocol = ps->protocol;
	
	if (protocol == PI_PF_DLP && autodetect) {
		if (dev_prot->read (ps, &byte, 1, PI_MSG_PEEK) > 0) {
			int found = 0;

			while (!found) {
				LOG(PI_DBG_SOCK, PI_DBG_LVL_INFO,
				    "SOCK Peeked and found 0x%.2x, ", byte);
				
				switch (byte) {
				case PI_SLP_SIG_BYTE1:
					protocol = PI_PF_PADP;
					LOG(PI_DBG_SOCK, PI_DBG_LVL_INFO, "PADP/SLP\n");
					found = 1;
					break;			
				case PI_NET_SIG_BYTE1:
				case 0x01:
					protocol = PI_PF_NET;
					LOG(PI_DBG_SOCK, PI_DBG_LVL_INFO, "NET\n");
					found = 1;
					break;
				default:
					if (dev_prot->read (ps, &byte, 1, PI_MSG_PEEK) < 0) {
						protocol = PI_PF_PADP;
						LOG(PI_DBG_SOCK, PI_DBG_LVL_INFO, "Default\n");
						found = 1;
					}
				}
			}
		}
	} else if (protocol == PI_PF_DLP) {
		protocol = PI_PF_PADP;
	}
	
	/* The connected protocol queue */
	switch (protocol) {
	case PI_PF_PADP:
		prot = padp_protocol ();
		protocol_queue_add (ps, prot);
	case PI_PF_SLP:
		prot = slp_protocol ();
		protocol_queue_add (ps, prot);
		break;
	case PI_PF_NET:
		prot = net_protocol ();
		protocol_queue_add (ps, prot);
		break;
	}

	/* The command protocol queue */
	switch (protocol) {
	case PI_PF_PADP:
	case PI_PF_SLP:
		prot 	= cmp_protocol ();
		protocol_cmd_queue_add (ps, prot);
	
		prot 	= padp_protocol ();
		protocol_cmd_queue_add (ps, prot);
	
		prot 	= slp_protocol ();
		protocol_cmd_queue_add (ps, prot);
	
		ps->cmd = PI_CMD_CMP;
		break;
	case PI_PF_NET:
		prot 	= net_protocol ();
		protocol_cmd_queue_add (ps, prot);
		ps->cmd = PI_CMD_NET;
		break;
	}

	protocol_queue_add (ps, dev_prot);
	protocol_cmd_queue_add (ps, dev_cmd_prot);
}

static void
protocol_queue_destroy (struct pi_socket *ps)
{
	int 	i;
	
	for (i = 0; i < ps->queue_len; i++)
		ps->protocol_queue[i]->free(ps->protocol_queue[i]);
	for (i = 0; i < ps->cmd_len; i++)
		ps->cmd_queue[i]->free(ps->cmd_queue[i]);

	if (ps->queue_len > 0)
		free(ps->protocol_queue);
	if (ps->cmd_len > 0)
		free(ps->cmd_queue);
}

struct pi_protocol *
pi_protocol (int pi_sd, int level)
{
	struct 	pi_socket *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return NULL;
	}

	return protocol_queue_find(ps, level);
}

struct pi_protocol *
pi_protocol_next (int pi_sd, int level)
{
	struct 	pi_socket *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return NULL;
	}

	return protocol_queue_find_next(ps, level);
}


/* Environment Code */
static void
env_check (void) 
{
	if (getenv("PILOT_DEBUG")) {
		int 	types = 0,
			done;
		char 	*debug,
			*b,
			*e;
		
		debug = strdup(getenv("PILOT_DEBUG"));

		b 	= debug;
		done 	= 0;
		while (!done) {
			e = strchr(b, ' ');
			if (e)
				*e = '\0';
			else
				done = 1;

			if (!strcmp(b, "SYS"))
				types |= PI_DBG_SYS;
			else if (!strcmp(b, "DEV"))
				types |= PI_DBG_DEV;
			else if (!strcmp(b, "SLP"))
				types |= PI_DBG_SLP;
			else if (!strcmp(b, "PADP"))
				types |= PI_DBG_PADP;
			else if (!strcmp(b, "DLP"))
				types |= PI_DBG_DLP;
			else if (!strcmp(b, "NET"))
				types |= PI_DBG_NET;
			else if (!strcmp(b, "CMP"))
				types |= PI_DBG_CMP;
			else if (!strcmp(b, "SOCK"))
				types |= PI_DBG_SOCK;
			else if (!strcmp(b, "API"))
				types |= PI_DBG_API;
			else if (!strcmp(b, "USER"))
				types |= PI_DBG_USER;
			e++;
			b = e;
		}
		pi_debug_set_types(types);

		free(debug);
	}
	if (getenv("PILOT_DEBUG_LEVEL")) {
		int 	level = 0;
		const char *debug;


		debug = getenv("PILOT_DEBUG_LEVEL");
		if (!strcmp(debug, "NONE"))
			level |= PI_DBG_LVL_NONE;
		else if (!strcmp(debug, "ERR"))
			level |= PI_DBG_LVL_ERR;
		else if (!strcmp(debug, "WARN"))
			level |= PI_DBG_LVL_WARN;
		else if (!strcmp(debug, "INFO"))
			level |= PI_DBG_LVL_INFO;
		else if (!strcmp(debug, "DEBUG"))
			level |= PI_DBG_LVL_DEBUG;

		pi_debug_set_level (level);
	}
	
	if (getenv("PILOT_LOG")) {
		const char *logfile;
		
		logfile = getenv("PILOT_LOGFILE");
		if (logfile == NULL)
			pi_debug_set_file("PiDebug.log");
		else
			pi_debug_set_file(logfile);
	}
}

/* Util functions */
static int
is_connected (struct pi_socket *ps) 
{
	if (ps->state == PI_SOCK_CONIN || ps->state == PI_SOCK_CONAC)
		return 1;
	
	return 0;
}

static int
is_listener (struct pi_socket *ps) 
{
	if (ps->state == PI_SOCK_LISTN)
		return 1;
	
	return 0;
}

/* Alarm Handling Code */

#ifdef WIN32
/* An implementation of alarm for windows*/
#include <process.h>
static long alm_countdown = -1;
static void *alm_tid = 0;

void
alarm_thread(void *unused)
{
	long 	av;

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

unsigned
alarm(unsigned sec)
{
	long 	ret = alm_countdown;

	if (sec) {
		alm_countdown = sec;
		if (!alm_tid) {
			unsigned long t;

			/* not multi thread safe -- fine if you just
                           call alarm from one thread */
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

static void
onalarm(int signo)
{
	struct pi_socket_list *l;

	signal(SIGALRM, onalarm);
	
	for (l = watch_list; l != NULL; l = l->next) {
		struct pi_socket *ps = l->ps;

		if (!is_connected(ps))
			continue;

		if (pi_tickle(ps->sd) < 0) {
			LOG(PI_DBG_SOCK, PI_DBG_LVL_INFO, 
			    "SOCKET Socket %d is busy during tickle\n", ps->sd);
			alarm(1);
		} else {
			LOG(PI_DBG_SOCK, PI_DBG_LVL_INFO,
			    "SOCKET Tickling socket %d\n", ps->sd);
			alarm(interval);
		}
	}
}

/* Exit Handling Code */
static void
onexit(void)
{
	struct pi_socket_list *l, *list;

	list = ps_list_copy (psl);
	for (l = list; l != NULL; l = l->next)
		pi_close(l->ps->sd);

	ps_list_free (list);
}

static void
installexit(void)
{
	static int installedexit = 0;

	if (!installedexit)
		atexit(onexit);

	installedexit = 1;
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
	struct 	pi_socket *ps;

	env_check ();
	
	if (protocol == 0) {
		if (type == PI_SOCK_STREAM)
			protocol = PI_PF_DLP;
		else if (type == PI_SOCK_RAW)
			protocol = PI_PF_DEV;
	}

	ps = calloc(sizeof(struct pi_socket), 1);

	/* Create unique socket descriptor */
#if defined( OS2 ) || defined( WIN32 )
	if ((ps->sd = open("NUL", O_RDWR)) == -1) {
#else
	if ((ps->sd = open("/dev/null", O_RDWR)) == -1) {
#endif
		int 	err = errno;	/* Save errno of open */

		free(ps);
		errno = err;
		return -1;
	}

	/* Initialize the rest of the fields */
	ps->type 	= type;
	ps->protocol 	= protocol;
	ps->cmd 	= 0;

	ps->laddr 	= NULL;
	ps->laddrlen 	= 0;
	ps->raddr 	= NULL;
	ps->raddrlen 	= 0;

	ps->protocol_queue = NULL;
	ps->queue_len   = 0;
	ps->cmd_queue   = NULL;
	ps->cmd_len     = 0;
	ps->device      = NULL;

	ps->state       = PI_SOCK_CLOSE;
	ps->command 	= 1;

	ps->accept_to 	= 0;
	ps->dlprecord 	= 0;

#ifdef OS2
	ps->os2_read_timeout 	= 60;
	ps->os2_write_timeout 	= 60;
#endif

	installexit();

	pi_socket_recognize(ps);

	return ps->sd;
}


/***********************************************************************
 *
 * Function:    pi_socket_copy
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     New socket
 *
 ***********************************************************************/
struct pi_socket *pi_socket_copy(struct pi_socket *ps)
{
	int 	i;
	struct pi_socket *new_ps;
	
	new_ps = malloc(sizeof(struct pi_socket));
	memcpy(new_ps, ps, sizeof(struct pi_socket));

	new_ps->laddr = malloc(ps->laddrlen);
	new_ps->raddr = malloc(ps->raddrlen);
	memcpy(new_ps->laddr, ps->laddr, ps->laddrlen);
	memcpy(new_ps->raddr, ps->raddr, ps->raddrlen);

	new_ps->sd = dup(ps->sd);
	
	new_ps->protocol_queue = NULL;
	new_ps->queue_len = 0;
	for (i = 0; i < ps->queue_len; i++) {
		struct pi_protocol *prot;
		
		prot = ps->protocol_queue[i]->dup (ps->protocol_queue[i]);
		protocol_queue_add(new_ps, prot);
	}
	new_ps->cmd_queue = NULL;
	new_ps->cmd_len = 0;
	for (i = 0; i < ps->cmd_len; i++) {
		struct pi_protocol *prot;
		
		prot = ps->cmd_queue[i]->dup (ps->cmd_queue[i]);
		protocol_cmd_queue_add(new_ps, prot);
	}
	new_ps->device = ps->device->dup (ps->device);
	
	pi_socket_recognize(new_ps);

	return new_ps;
}

int pi_socket_setsd(struct pi_socket *ps, int sd)
{
	int 	orig;
	
	orig = sd;
	
#ifdef HAVE_DUP2
	sd = dup2(sd, ps->sd);
#else
#ifdef F_DUPFD
	close(ps->sd);
	sd = fcntl(sd, F_DUPFD, ps->sd);
#else
	close(ps->sd);
	sd = dup(sd);	/* Unreliable */
#endif
#endif
	if (sd != orig)
		close(orig);
	else
		return -1;

	return 0;
}

int pi_socket_init(struct pi_socket *ps)
{
	protocol_queue_build (ps, 1);

	return 0;
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
	psl = ps_list_append (psl, ps);
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
	int 	paddrlen = addrlen;
	struct 	pi_socket *ps;
	struct 	pi_sockaddr *paddr = (struct pi_sockaddr *) addr;
	struct 	pi_sockaddr eaddr;
	
	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}

	if (paddr == NULL && getenv("PILOTPORT")) {
		eaddr.pi_family = PI_AF_PILOT;
		strncpy(eaddr.pi_device, getenv("PILOTPORT"),
			sizeof(eaddr.pi_device));
		paddr = &eaddr;
		addrlen = sizeof(struct pi_sockaddr);
	} else if (paddr == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* Determine the device type */
	if (strlen (paddr->pi_device) < 4)
		ps->device = pi_serial_device (PI_SERIAL_DEV);
	else if (!strncmp (paddr->pi_device, "ser:", 4))
		ps->device = pi_serial_device (PI_SERIAL_DEV);
	else if (!strncmp (paddr->pi_device, "net:", 4))
		ps->device = pi_inet_device (PI_NET_DEV);
	else
		ps->device = pi_serial_device (PI_SERIAL_DEV);

	/* Build the protocol queue */
	protocol_queue_build (ps, 0);
	
	return ps->device->connect (ps, (struct sockaddr *)paddr, paddrlen);
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
	int 	paddrlen = addrlen;
	struct 	pi_socket *ps;
	struct 	pi_sockaddr *paddr = (struct pi_sockaddr *) addr;
	struct 	pi_sockaddr eaddr;
	
	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}

	if (paddr == NULL && getenv("PILOTPORT")) {
		eaddr.pi_family = PI_AF_PILOT;
		strncpy(eaddr.pi_device, getenv("PILOTPORT"),
			sizeof(eaddr.pi_device));
		paddr = &eaddr;
		addrlen = sizeof(struct pi_sockaddr);
	} else if (paddr == NULL) {
		errno = EINVAL;
		return -1;
	}
	
	/* Determine the device type */
	if (strlen (paddr->pi_device) < 4)
		ps->device = pi_serial_device (PI_SERIAL_DEV);
	else if (!strncmp (paddr->pi_device, "ser:", 4))
		ps->device = pi_serial_device (PI_SERIAL_DEV);
	else if (!strncmp (paddr->pi_device, "net:", 4))
		ps->device = pi_inet_device (PI_NET_DEV);
	else
		ps->device = pi_serial_device (PI_SERIAL_DEV);

	return ps->device->bind (ps, (struct sockaddr *)paddr, paddrlen);
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
	struct 	pi_socket *ps;
	
	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}

	return ps->device->listen (ps, backlog);
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
	struct 	pi_socket *ps;
		
	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}

	if (!is_listener (ps))
		return -1;
	
	ps->accept_to = 0;

	return ps->device->accept(ps, addr, addrlen);
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
	struct 	pi_socket *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}

	if (!is_listener (ps))
		return -1;

	ps->accept_to = timeout;

	return ps->device->accept(ps, addr, addrlen);
}

/***********************************************************************
 *
 * Function:    pi_getsockopt
 *
 * Summary:     Get a socket option
 *
 * Parmeters:   None
 *
 * Returns:     0 on success, negative value on failure
 *
 ***********************************************************************/
int
pi_getsockopt(int pi_sd, int level, int option_name, 
	      void *option_value, int *option_len)
{
	struct 	pi_socket *ps;
	struct 	pi_protocol *prot;
	
	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}

	prot = protocol_queue_find (ps, level);
	
	if (prot == NULL || prot->level != level) {
		errno = EINVAL;
		return -1;
	}

	return prot->getsockopt (ps, level, option_name, option_value, option_len);
}


/***********************************************************************
 *
 * Function:    pi_setsockopt
 *
 * Summary:     Set a socket option
 *
 * Parmeters:   None
 *
 * Returns:     0 on success, negative value on failure
 *
 ***********************************************************************/
int
pi_setsockopt(int pi_sd, int level, int option_name, 
	      const void *option_value, int *option_len) 
{
	struct 	pi_socket *ps;
	struct 	pi_protocol *prot;
	
	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}

	prot = protocol_queue_find (ps, level);
	
	if (prot == NULL || prot->level != level) {
		errno = EINVAL;
		return -1;
	}

	return prot->setsockopt (ps, level, option_name, option_value, option_len);
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
	struct 	pi_socket *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}

	if (!is_connected (ps))
		return -1;
	
	if (interval)
		alarm(interval);

	return ps->protocol_queue[0]->write (ps, msg, len, flags);
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
	struct 	pi_socket *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}

	if (!is_connected (ps))
		return -1;

	return ps->protocol_queue[0]->read (ps, msg, len, flags);
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
	int 	result,
		type, 
		size, 
		len = 0;
	char 	msg[1];
	struct pi_socket *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}

	if (!is_connected (ps))
		return -1;

	LOG(PI_DBG_SOCK, PI_DBG_LVL_INFO, "SOCKET Tickling socket %d\n", pi_sd);

	/* Enter command state */
	ps->command = 1;
	
	/* Set the type to "tickle" */
	switch (ps->cmd) {
	case PI_CMD_CMP:
		type = padTickle;
		size = sizeof(type);
		pi_setsockopt(ps->sd, PI_LEVEL_PADP, PI_PADP_TYPE, &type, &size);
		break;
	case PI_CMD_NET:
		type = PI_NET_TYPE_TCKL;
		size = sizeof(type);
		pi_setsockopt(ps->sd, PI_LEVEL_NET, PI_NET_TYPE, &type, &size);
		break;
	}

	result = ps->cmd_queue[0]->write (ps, msg, len, 0);

	/* Exit command state */
	ps->command = 0;

	return result;
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
	int 	result = 0;
	struct 	pi_socket *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}

	if (ps->type == PI_SOCK_STREAM) {
		if (is_connected (ps)) {
				ps->command = 1;
				
				/* then end sync, with clean status */
				dlp_EndOfSync(ps->sd, 0);

				ps->command = 0;
		}
	}

	if (ps->device != NULL) {
		result = ps->device->close (ps);
		if (result < 0)
			return result;
	}
	
	psl = ps_list_remove (psl, pi_sd);
	watch_list = ps_list_remove (watch_list, pi_sd);

	protocol_queue_destroy(ps);
	if (ps->device != NULL)
		ps->device->free(ps->device);
	free(ps);

	return result;
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
	struct 	pi_socket *ps;

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
	struct 	pi_socket *ps;

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
	int 	size, 
		vers = 0x0000;
	struct 	pi_socket *ps;

	
	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}
	
	/* Enter command state */
	ps->command = 1;

	/* Get the version */
	switch (ps->cmd) {
	case PI_CMD_CMP:
		size = sizeof(vers);
		pi_getsockopt(ps->sd, PI_LEVEL_CMP, PI_CMP_VERS, &vers, &size);
		break;
	case PI_CMD_NET:
		vers = 0x0101;
		break;
	}

	/* Exit command state */
	ps->command = 0;

	return vers;
}

struct pi_socket *find_pi_socket(int sd)
{
	return ps_list_find (psl, sd);
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
	struct 	pi_socket *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return -1;
	}

	watch_list 	= ps_list_append (watch_list, ps);
	signal(SIGALRM, onalarm);
	interval 	= newinterval;
	alarm(interval);

	return 0;
}
