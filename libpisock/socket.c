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

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

#include "pi-source.h"
#include "pi-serial.h"
#ifdef HAVE_USB
#include "pi-usb.h"
#endif
#include "pi-inet.h"
#include "pi-slp.h"
#include "pi-sys.h"
#include "pi-padp.h"
#include "pi-cmp.h"
#include "pi-net.h"
#include "pi-dlp.h"
#include "pi-syspkt.h"
#include "pi-debug.h"
#include "pi-error.h"

/* Declare function prototypes */
static pi_socket_list_t *ps_list_append (pi_socket_list_t *list,
	pi_socket_t *ps);
/* knghtbrd: unused
static pi_socket_list_t *ps_list_prepend (pi_socket_list_t *list,
	pi_socket_t *ps); */
static pi_socket_t *ps_list_find (pi_socket_list_t *list,
	int pi_sd);
/* fpillet: unused
static pi_socket_list_t *ps_list_find_elem (pi_socket_list_t *list,
	int pi_sd);  */
static pi_socket_list_t *ps_list_remove (pi_socket_list_t *list,
	int pi_sd); 
static pi_socket_list_t *ps_list_copy (pi_socket_list_t *list);
static void ps_list_free (pi_socket_list_t *list);

static void protocol_queue_add (pi_socket_t *ps, pi_protocol_t *prot);
static void protocol_cmd_queue_add (pi_socket_t *ps, pi_protocol_t *prot);
static pi_protocol_t *protocol_queue_find (pi_socket_t *ps, int level);
static pi_protocol_t *protocol_queue_find_next (pi_socket_t *ps, int level); 

int pi_socket_init(pi_socket_t *ps);

static int is_connected (pi_socket_t *ps);
static int is_listener (pi_socket_t *ps);

/* GLOBALS */
static pi_socket_list_t *psl = NULL;
static pi_socket_list_t *watch_list = NULL;

/* Automated tickling interval */
static unsigned int interval = 0;

/* Indicates that the exit function has already been installed. Made non-static
 * so that library users can choose to not have an exit function installed */
int pi_sock_installedexit = 0;

/* Linked List Code */
/***********************************************************************
 *
 * Function:    ps_list_dump
 *
 * Summary:     internal debugging function
 *
 * Parameters:	pi_socket_list_t *
 *
 * Returns:     void
 *
 ***********************************************************************/
/* fpillet: used only during debug phases
static void
ps_list_dump (pi_socket_list_t *list)
{
	fprintf(stderr, "* Dumping pi_socket_list @ %p:\n",(void*)list);
	while (list != NULL) {
		fprintf(stderr,"  %p: ps=%p, pi_sd=%d\n", (void*)list, (void*)list->ps, list->ps->sd);
		list = list->next;
	}
}
*/

/***********************************************************************
 *
 * Function:    ps_list_append
 *
 * Summary:     creates and appends a new pi_socket_list element to
 *		the (possibly empty) pi_socket_list and	fills in the
 *		pi_socket_t* member to point to the given pi_socket.
 *
 * Parameters:	pi_socket_list_t*, pi_socket_t*
 *
 * Returns:     pi_socket_list_t*, or NULL if operation failed
 *
 ***********************************************************************/
static pi_socket_list_t *
ps_list_append (pi_socket_list_t *list, pi_socket_t *ps) 
{
	pi_socket_list_t *elem, *new_elem;
	
	ASSERT (ps != NULL);

	new_elem = (pi_socket_list_t *) malloc (sizeof(pi_socket_list_t));
	if (new_elem == NULL)
		return list;

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


/***********************************************************************
 *
 * Function:    ps_list_prepend
 *
 * Summary:     creates and prepends a new pi_socket_list element to
 *		the (possibly empty) pi_socket_list and	fills in the
 *		pi_socket_t* member to point to the given pi_socket.
 *
 * Parameters:	pi_socket_list_t *, pi_socket_t *
 *
 * Returns:     pi_socket_list_t *, or NULL if operation failed
 *
 ***********************************************************************/
/* knghtbrd: unused
static pi_socket_list_t *
ps_list_prepend (pi_socket_list_t *list, pi_socket_t *ps) 
{
	pi_socket_list_t *new_elem;
	
	ASSERT (ps != NULL);

	new_elem = malloc(sizeof(pi_socket_list_t));

	if (new_elem != NULL) {
		new_elem->ps 	= ps;
		new_elem->version = 0;
		new_elem->next 	= list;
	}

	return new_elem;
}*/


/***********************************************************************
 *
 * Function:    ps_list_find
 *
 * Summary:     traverse a (possibly empty) pi_socket_list and find
 *		the first list element whose pi_socket_t* member points
 *		to a pi_socket matching the given socket descriptor
 *
 * Parameters:	pi_socket_list_t *, socket descriptor 
 *
 * Returns:     pi_socket_t *, or NULL if no match
 *
 * NOTE:	ps_list_find returns a pointer which points directly to
 *		the socket (pi_socket_t *) whereas ps_find_elem returns a
 *		pointer to the list element (pi_socket_list_t *) which
 *		_contains_ a pointer to the socket
 *
 ***********************************************************************/
static pi_socket_t *
ps_list_find (pi_socket_list_t *list, int pi_sd) 
{
	pi_socket_list_t *elem;
	
	for (elem = list; elem != NULL; elem = elem->next) {
		if (elem->ps != NULL)
			if (elem->ps->sd == pi_sd)
				return elem->ps;
	}

	return NULL;
}


/***********************************************************************
 *
 * Function:    ps_find_elem
 *
 * Summary:     traverse a (possibly empty) pi_socket_list and find
 *		the first list element whose pi_socket* member points
 *		to a pi_socket matching the given socket descriptor
 *
 * Parameters:	pi_socket_list_t*, socket descriptor 
 *
 * Returns:     pi_socket_list_t*, or NULL if not found
 *
 * NOTE:	ps_list_find returns a pointer which points directly to
 *		the socket (pi_socket_t *) whereas ps_find_elem returns a
 *		pointer to the list element (pi_socket_list_t *) which
 *		_contains_ a pointer to the socket
 *
 ***********************************************************************/
/* fpillet: unused
static pi_socket_list_t *
ps_list_find_elem (pi_socket_list_t *list, int pi_sd) 
{
	pi_socket_list_t *elem;
	
	for (elem = list; elem != NULL; elem = elem->next) {
		if (elem->ps != NULL)
			if (elem->ps->sd == pi_sd)
				return elem;
	}

	return NULL;
}
*/

/***********************************************************************
 *
 * Function:    ps_list_remove
 *
 * Summary:     remove first pi_socket_list element pointing to a pi_socket
 *		member matching socket descriptor
 *
 * Parameters:	pi_socket_list_t *, socket descriptor 
 *
 * Returns:     the (possibly NULL) head pi_socket_list_t *
 *
 * NOTE:	only the pi_socket_list element is freed,
 *		_not_ the pi_socket.  Consequently, this function
 *		makes the (risky) assumption that the pi_socket will
 *		be freed elsewhere.
 *
 ***********************************************************************/
static pi_socket_list_t *
ps_list_remove (pi_socket_list_t *list, int pi_sd) 
{
	pi_socket_list_t *elem,
		*new_list = list,
		*prev_elem = NULL;

	for (elem = list; elem != NULL; elem = elem->next) {
		if (elem->ps == NULL)
			continue;
		else if (elem->ps->sd == pi_sd) {
			if (prev_elem == NULL)
				new_list = elem->next;
			else
				prev_elem->next = elem->next;
			free(elem);
			break;
		}
		prev_elem = elem;
	}

	return new_list;
}


/***********************************************************************
 *
 * Function:    ps_list_copy
 *
 * Summary:     copy pi_socket_list
 *
 * Parameters:	pi_socket_list_t *
 *
 * Returns:     pi_socket_list_t* (new list head)
 *
 * NOTE:	pi_list_copy does _not_ copy the pi_socket member, it
 *		copies only the list elements
 *
 ***********************************************************************/
static pi_socket_list_t *
ps_list_copy (pi_socket_list_t *list) 
{
	pi_socket_list_t *l, *new_list = NULL;
	
	for (l = list; l != NULL; l = l->next)
		new_list = ps_list_append (new_list, l->ps);

	return new_list;
}


/***********************************************************************
 *
 * Function:    ps_list_free
 *
 * Summary:     free pi_socket_list elements
 *
 * Parameters:	pi_socket_list_t *
 *
 * Returns:     void
 *
 * NOTE:	only the pi_socket_list elements are freed,
 *		_not_ the pi_sockets.  Consequently, this function
 *		makes the (risky) assumption that the pi_sockets will
 *		be freed elsewhere.
 *
 ***********************************************************************/
static void
ps_list_free (pi_socket_list_t *list)
{
	pi_socket_list_t *l, *next;

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
/***********************************************************************
 *
 * Function:    protocol_queue_add
 *
 * Summary:     adds protocol queue to pi_socket
 *
 * Parameters:	pi_socket_t*, pi_protocol_t*
 *
 * Returns:     void
 *
 ***********************************************************************/
static void
protocol_queue_add (pi_socket_t *ps, pi_protocol_t *prot)
{
	ps->protocol_queue = realloc(ps->protocol_queue,
		(sizeof(pi_protocol_t *)) * (ps->queue_len + 1));
	if (ps->protocol_queue != NULL) {
		ps->protocol_queue[ps->queue_len] = prot;
		ps->queue_len++;
	} else {
		errno = ENOMEM;
		ps->queue_len = 0;
	}
}


/***********************************************************************
 *
 * Function:    cmd_queue_add
 *
 * Summary:     adds command queue to pi_socket
 *
 * Parameters:	pi_socket_t*, pi_protocol*
 *
 * Returns:     void
 *
 ***********************************************************************/
static void
protocol_cmd_queue_add (pi_socket_t *ps, pi_protocol_t *prot)
{
	ps->cmd_queue = realloc(ps->cmd_queue,
		(sizeof(pi_protocol_t *)) * (ps->cmd_len + 1));
	if (ps->cmd_queue != NULL) {
		ps->cmd_queue[ps->cmd_len] = prot;
		ps->cmd_len++;
	} else {
		errno = ENOMEM;
		ps->cmd_len = 0;
	}
}


/***********************************************************************
 *
 * Function:    protocol_queue_find
 *
 * Summary:     find queue entry
 *
 * Parameters:	pi_socket_t*, level
 *
 * Returns:     pi_protocol*, or NULL if queue entry not found
 *
 ***********************************************************************/
static pi_protocol_t*
protocol_queue_find (pi_socket_t *ps, int level) 
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


/***********************************************************************
 *
 * Function:    protocol_queue_find_next
 *
 * Summary:     find next queue entry
 *
 * Parameters:	pi_socket*, level
 *
 * Returns:     pi_protocol_t* or NULL if next queue entry not found
 *
 ***********************************************************************/
static pi_protocol_t*
protocol_queue_find_next (pi_socket_t *ps, int level) 
{
	int 	i;

	if (ps->command && ps->cmd_len == 0)
		return NULL;

	if (!ps->command && ps->queue_len == 0)
		return NULL;

	if (ps->command && level == 0)
		return ps->cmd_queue[0];

	if (!ps->command && level == 0)
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


/***********************************************************************
 *
 * Function:    protocol_queue_build
 *
 * Summary:     build protocol queue
 *
 * Parameters:	pi_socket_t*, autodetect
 *
 * Returns:     void
 *
 ***********************************************************************/
static void
protocol_queue_build (pi_socket_t *ps, int autodetect) 
{
	int 	protocol;
	pi_protocol_t	*prot,
			*dev_prot,
			*dev_cmd_prot;
	unsigned char byte;
	pi_buffer_t byte_buf = { 0 };

	byte_buf.data = &byte;
	byte_buf.allocated = 1;
	byte_buf.used = 0;
	
	/* The device protocol */
	dev_prot 	= ps->device->protocol (ps->device);
	dev_cmd_prot 	= ps->device->protocol (ps->device);

	protocol = ps->protocol;
	
	if (protocol == PI_PF_DLP && autodetect) {
		if (dev_prot->read (ps, &byte_buf, 1, PI_MSG_PEEK) > 0) {
			int found = 0;
			while (!found) {
				LOG((PI_DBG_SOCK, PI_DBG_LVL_INFO,
				    "SOCK Peeked and found 0x%.2x, ", byte));
				
				switch (byte) {
				case PI_SLP_SIG_BYTE1:
					protocol = PI_PF_PADP;
					LOG((PI_DBG_SOCK, PI_DBG_LVL_INFO,
						"PADP/SLP\n"));
					found = 1;
					break;			
				case PI_NET_SIG_BYTE1:
				case 0x01:
					protocol = PI_PF_NET;
					LOG((PI_DBG_SOCK, PI_DBG_LVL_INFO,
						"NET\n"));
					found = 1;
					break;
				default:
					/* here's the trick: the byte we read wasn't the one
					   we were looking for, so we eliminate it with a
					   normal read then do a PEEK again to catch the
					   next one */
					byte_buf.used = 0;
					if (dev_prot->read (ps, &byte_buf, 1, 0) >= 0)
					{
						byte_buf.used = 0;
						if (dev_prot->read (ps, &byte_buf, 1, PI_MSG_PEEK) < 0)
						{
							protocol = PI_PF_PADP;
							LOG((PI_DBG_SOCK, PI_DBG_LVL_INFO,
								"Default\n"));
							found = 1;
						}
					}
					else
					{
						protocol = PI_PF_PADP;
						LOG((PI_DBG_SOCK, PI_DBG_LVL_INFO,
							"Default\n"));
						found = 1;
					}
					break;
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
	case PI_PF_SYS:
		prot = sys_protocol ();
		protocol_queue_add (ps, prot);
		prot = slp_protocol ();
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
	case PI_PF_SYS:
		ps->cmd = PI_CMD_SYS;
		break;
	}

	protocol_queue_add (ps, dev_prot);
  	protocol_cmd_queue_add (ps, dev_cmd_prot);
}


/***********************************************************************
 *
 * Function:    protocol_queue_destroy
 *
 * Summary:     destroy protocol queue
 *
 * Parameters:	pi_socket_t *
 *
 * Returns:     void
 *
 ***********************************************************************/
static void
protocol_queue_destroy (pi_socket_t *ps)
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


/***********************************************************************
 *
 * Function:    pi_protocol
 *
 * Summary:     destroy protocol queue
 *
 * Parameters:	socket descriptor, level
 *
 * Returns:     pi_protocol_t*
 *
 ***********************************************************************/
pi_protocol_t *
pi_protocol (int pi_sd, int level)
{
	pi_socket_t *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return NULL;
	}

	return protocol_queue_find(ps, level);
}

pi_protocol_t *
pi_protocol_next (int pi_sd, int level)
{
	pi_socket_t *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return NULL;
	}

	return protocol_queue_find_next(ps, level);
}


/* Environment Code */
/***********************************************************************
 *
 * Function:    env_check
 *
 * Summary:     configures Pilot-Link debug environment
 *
 * Parameters:	void
 *
 * Returns:     void
 *
 ***********************************************************************/
static void
env_dbgcheck (void) 
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
			else if (!strcmp(b, "ALL"))
				types |= PI_DBG_ALL;
			e++;
			b = e;
		}
		pi_debug_set_types(types);

		free(debug);
	}

	/* PILOT Debug Level */
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
	
	/* log file name */
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
/***********************************************************************
 *
 * Function:    is_connected
 *
 * Summary:     interrogate socket connection state
 *
 * Parameters:	pi_socket*
 *
 * Returns:     1 if socket is connected, 0 otherwise
 *
 ***********************************************************************/
static int
is_connected (pi_socket_t *ps) 
{
	return (ps->state == PI_SOCK_CONIN || ps->state == PI_SOCK_CONAC) ? 1 : 0;
}

/***********************************************************************
 *
 * Function:    is_listener
 *
 * Summary:     interrogate socket listener state
 *
 * Parameters:	pi_socket*
 *
 * Returns:     1 if socket is listener, 0 otherwise
 *
 ***********************************************************************/
static int
is_listener (pi_socket_t *ps) 
{
	return (ps->state == PI_SOCK_LISTN) ? 1 : 0;
}

/* Alarm Handling Code */
static RETSIGTYPE
onalarm(int signo)
{
	pi_socket_list_t *l;

	signal(signo, onalarm);
	
	for (l = watch_list; l != NULL; l = l->next) {
		pi_socket_t *ps = l->ps;

		if (!is_connected(ps))
			continue;

		if (pi_tickle(ps->sd) < 0) {
			LOG((PI_DBG_SOCK, PI_DBG_LVL_INFO, 
				"SOCKET Socket %d is busy during tickle\n",
				ps->sd));
			alarm(1);
		} else {
			LOG((PI_DBG_SOCK, PI_DBG_LVL_INFO,
			    "SOCKET Tickling socket %d\n", ps->sd));
			alarm(interval);
		}
	}
}

/* Exit Handling Code */
/***********************************************************************
 *
 * Function:    onexit
 *
 * Summary:     this function closes and destroys all pi_sockets and
 *		frees the global pi_socket_list
 *
 * Parameters:	void
 *
 * Returns:     void
 *
 ***********************************************************************/
static void
onexit(void)
{
	pi_socket_list_t *l,
			 *list;

	list = ps_list_copy (psl);

	for (l = list; l != NULL; l = l->next)
		pi_close(l->ps->sd);

	ps_list_free (list);
}


/***********************************************************************
 *
 * Function:    installexit
 *
 * Summary:     install exit function using atexit()
 *
 * Parameters:	void
 *
 * Returns:     void
 *
 ***********************************************************************/
static void
installexit(void)
{
	if (!pi_sock_installedexit) {
		atexit(onexit);
		pi_sock_installedexit = 1;
	}
}


/***********************************************************************
 *
 * Function:    pi_socket
 *
 * Summary:     Create a local connection endpoint
 *
 * Parameters:  domain, type, protocol
 *
 * Returns:     socket descriptor, or -1 on error
 *
 ***********************************************************************/
int
pi_socket(int domain, int type, int protocol)
{
	pi_socket_t *ps;
	pi_socket_list_t *list;

	env_dbgcheck ();
	
	if (protocol == 0) {
		if (type == PI_SOCK_STREAM)
			protocol = PI_PF_DLP;
		else if (type == PI_SOCK_RAW)
			protocol = PI_PF_DEV;
	}

	ps = calloc(1, sizeof(pi_socket_t));
	if (ps == NULL) {
		errno = ENOMEM;
		return -1;
	}

	/* Create unique socket descriptor */
	if ((ps->sd = open(NULL_DEVICE, O_RDWR)) == -1) {
		int 	err = errno;	/* Save errno of open */

		free(ps);
		errno = err;
		return -1;
	}

	/* Initialize the rest of the fields (calloc zeroes out
	   all the fields we don't touch) */
	ps->type 	= type;
	ps->protocol 	= protocol;
	ps->state       = PI_SOCK_CLOSE;
	ps->command 	= 1;

#ifdef OS2
	ps->os2_read_timeout 	= 60;
	ps->os2_write_timeout 	= 60;
#endif
 
	/* post the new socket to the list */
	list = pi_socket_recognize(ps);
	if (list == NULL) {
		close (ps->sd);
		free(ps);
		errno = ENOMEM;
		return -1;
	}

	installexit();
	return ps->sd;
}

/***********************************************************************
 *
 * Function:    pi_socket_setsd
 *
 * Summary:	dups a pi_socket descriptor
 *
 * Parameters:  pi_socket_t*, pi_socket descriptor
 *
 * Returns:     0 on success, -1 otherwise
 *
 ***********************************************************************/
int
pi_socket_setsd(pi_socket_t *ps, int pi_sd)
{
	int 	orig;
	
	orig = pi_sd;
	
#ifdef HAVE_DUP2
	pi_sd = dup2(pi_sd, ps->sd);
#else
	#ifdef F_DUPFD
		close(ps->sd);
		pi_sd = fcntl(sd, F_DUPFD, ps->sd);
	#else
		close(ps->sd);
		pi_sd = dup(pi_sd);	/* Unreliable */
	#endif
#endif
	if (pi_sd != orig) {
		close(orig);
		return 0;
	}

	return pi_set_error(ps->sd, PI_ERR_GENERIC_SYSTEM);
}


/***********************************************************************
 *
 * Function:    pi_socket_init
 *
 * Summary:     inits the pi_socket 
 *
 * Parameters:  pi_socket_t*
 *
 * Returns:     0
 *
 ***********************************************************************/
int
pi_socket_init(pi_socket_t *ps)
{
  	protocol_queue_build (ps, 1);

	return 0;
}


/***********************************************************************
 *
 * Function:    pi_socket_recognize
 *
 * Summary:     appends the pi_socket to global list
 *
 * Parameters:  pi_socket*
 *
 * Returns:     pi_socket_list_t *
 *
 ***********************************************************************/
pi_socket_list_t *
pi_socket_recognize(pi_socket_t *ps)
{
	return (psl = ps_list_append (psl, ps));
}

/***********************************************************************
 *
 * Function:    pi_devsocket (static)
 *
 * Summary:     Looks up a socket and creates a new device
 *				FIXME: Decide whether or not to create the socket here
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static pi_socket_t *
pi_devsocket(int pi_sd, const char *port, struct pi_sockaddr *addr)
{
	pi_socket_t *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return NULL;
	}

	if (port == NULL && (port = getenv("PILOTPORT")) == NULL) {
		errno = ENXIO;
		return NULL;
	}

	/* Create the device and sockaddr */
	addr->pi_family = PI_AF_PILOT;
	if (!strncmp (port, "serial:", 4)) {
		strncpy(addr->pi_device, port + 7, sizeof(addr->pi_device));
		ps->device = pi_serial_device (PI_SERIAL_DEV);
#ifdef HAVE_USB	
	} else if (!strncmp (port, "usb:", 4)) {
		strncpy(addr->pi_device, port + 4, sizeof(addr->pi_device));
		ps->device = pi_usb_device (PI_USB_DEV);
#endif
	} else if (!strncmp (port, "net:", 4)) {
		strncpy(addr->pi_device, port + 4, sizeof(addr->pi_device));
		ps->device = pi_inet_device (PI_NET_DEV);
	} else {
		/* No prefix assumed to be serial: (for compatibility) */
		strncpy(addr->pi_device, port, sizeof(addr->pi_device));
		ps->device = pi_serial_device (PI_SERIAL_DEV);
	}

	return ps;
}

/***********************************************************************
 *
 * Function:    pi_connect
 *
 * Summary:     Connect to a remote server
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_connect(int pi_sd, const char *port)
{
	pi_socket_t *ps;
	struct 	pi_sockaddr addr;
	
	ps = pi_devsocket(pi_sd, port, &addr);
	if (!ps)
		return PI_ERR_SOCK_INVALID;

	/* Build the protocol queue */
	protocol_queue_build (ps, 0);
	
	return ps->device->connect (ps, (struct sockaddr *)&addr, sizeof(addr));
}

/***********************************************************************
 *
 * Function:    pi_bind
 *
 * Summary:     Bind address to a local socket
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_bind(int pi_sd, const char *port)
{
	int	bind_return;
	pi_socket_t *ps;
	struct  pi_sockaddr addr;

	ps = pi_devsocket(pi_sd, port, &addr);
	if (!ps)
		return PI_ERR_SOCK_INVALID;

	bind_return =
		ps->device->bind (ps, (struct sockaddr *)&addr, sizeof(addr));
	if (bind_return < 0) {
		ps->device->free (ps->device);
		ps->device = NULL;

	}
	return bind_return;
}

/***********************************************************************
 *
 * Function:    pi_listen
 *
 * Summary:     Wait for an incoming connection
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_listen(int pi_sd, int backlog)
{
	pi_socket_t *ps;
	
	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return PI_ERR_SOCK_INVALID;
	}

	return ps->device->listen (ps, backlog);
}

/***********************************************************************
 *
 * Function:    pi_accept
 *
 * Summary:     Wait forever for an incoming connection
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_accept(int pi_sd, struct sockaddr *addr, size_t *addrlen)
{
	return pi_accept_to(pi_sd, addr, addrlen, 0);
}

/***********************************************************************
 *
 * Function:    pi_accept_to
 *
 * Summary:     Accept an incoming connection (with timeout)
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_accept_to(int pi_sd, struct sockaddr *addr, size_t *addrlen, int timeout)
{
	pi_socket_t *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return PI_ERR_SOCK_INVALID;
	}

	if (!is_listener (ps))
		return PI_ERR_SOCK_LISTENER;

	ps->accept_to = timeout;

	return ps->device->accept(ps, addr, addrlen);
}

/***********************************************************************
 *
 * Function:    pi_getsockopt
 *
 * Summary:     Get a socket option
 *
 * Parameters:  None
 *
 * Returns:     0 on success, negative value on failure
 *
 ***********************************************************************/
int
pi_getsockopt(int pi_sd, int level, int option_name, 
	      void *option_value, size_t *option_len)
{
	pi_socket_t *ps;
	pi_protocol_t *prot;
	
	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return PI_ERR_SOCK_INVALID;
	}

	if (level == PI_LEVEL_SOCK) {
		if (option_name == PI_SOCK_STATE) {
			if (*option_len != sizeof (ps->state)) {
				errno = EINVAL;
				return PI_ERR_GENERIC_ARGUMENT;
			}
			memcpy (option_value, &ps->state, sizeof (ps->state));
			*option_len = sizeof (ps->state);
		}
		return 0;
	}

	prot = protocol_queue_find (ps, level);
	
	if (prot == NULL || prot->level != level) {
		errno = EINVAL;
		return PI_ERR_SOCK_INVALID;
	}

	return prot->getsockopt (ps, level, option_name,
		option_value, option_len);
}


/***********************************************************************
 *
 * Function:    pi_setsockopt
 *
 * Summary:     Set a socket option
 *
 * Parameters:  None
 *
 * Returns:     0 on success, negative value on failure
 *
 ***********************************************************************/
int
pi_setsockopt(int pi_sd, int level, int option_name, 
	      const void *option_value, size_t *option_len) 
{
	pi_socket_t *ps;
	pi_protocol_t *prot;
	
	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return PI_ERR_SOCK_INVALID;
	}

	if (level == PI_LEVEL_SOCK) {
		if (option_name == PI_SOCK_STATE) {
			if (*option_len != sizeof (ps->state)) {
				errno = EINVAL;
				return PI_ERR_GENERIC_ARGUMENT;
			}
			memcpy (&ps->state, option_value, sizeof (ps->state));
		}
		return 0;
	}
		
	prot = protocol_queue_find (ps, level);

	if (prot == NULL || prot->level != level) {
		errno = EINVAL;
		return PI_ERR_SOCK_INVALID;
	}

	return prot->setsockopt (ps, level, option_name,
		option_value, option_len);
}

/***********************************************************************
 *
 * Function:    pi_send
 *
 * Summary:     Send message on a connected socket
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_send(int pi_sd, void *msg, size_t len, int flags)
{
	pi_socket_t *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return PI_ERR_SOCK_INVALID;
	}

	if (!is_connected (ps))
		return PI_ERR_SOCK_DISCONNECTED;
	
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
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
ssize_t
pi_recv(int pi_sd, pi_buffer_t *msg, size_t len, int flags)
{
	pi_socket_t *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return PI_ERR_SOCK_INVALID;
	}

	if (!is_connected (ps))
		return PI_ERR_SOCK_DISCONNECTED;

	return ps->protocol_queue[0]->read (ps, msg, len, flags);
}

/***********************************************************************
 *
 * Function:    pi_read
 *
 * Summary:     Wrapper for receive
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
ssize_t
pi_read(int pi_sd, pi_buffer_t *msg, size_t len)
{
	return pi_recv(pi_sd, msg, len, 0);
}

/***********************************************************************
 *
 * Function:    pi_write
 *
 * Summary:     Wrapper for send
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
ssize_t
pi_write(int pi_sd, void *msg, size_t len)
{
	return pi_send(pi_sd, msg, len, 0);
}

/***********************************************************************
 *
 * Function:    pi_flush
 *
 * Summary:     flush input and output buffers
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void
pi_flush(int pi_sd, int flags)
{
	pi_socket_t *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return;
	}

	if (!is_connected (ps))
		return;
	
	ps->protocol_queue[0]->flush (ps, flags);
}

/***********************************************************************
 *
 * Function:    pi_tickle
 *
 * Summary:     Tickle a stream connection
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_tickle(int pi_sd)
{
	int 	result,
		type,
		oldtype;
	size_t	len = 0,
		size;
	unsigned char 	msg[1];
	pi_socket_t *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return PI_ERR_SOCK_INVALID;
	}

	if (!is_connected (ps))
		return PI_ERR_SOCK_DISCONNECTED;

	LOG((PI_DBG_SOCK, PI_DBG_LVL_INFO,
			"SOCKET Tickling socket %d\n", pi_sd));
	
	switch (ps->cmd) {
		case PI_CMD_CMP:
			/* save previous packet type */
			size = sizeof(type);
			pi_getsockopt(ps->sd, PI_LEVEL_PADP, PI_PADP_TYPE, &oldtype, &size);
			
			/* set packet type to "tickle" */
			type = padTickle;
			size = sizeof(type);
			pi_setsockopt(ps->sd, PI_LEVEL_PADP, PI_PADP_TYPE, &type, &size);

			/* send packet */
			result = ps->protocol_queue[0]->write (ps, msg, len, 0);
			
			/* restore previous packet type */
			size = sizeof(type);
			pi_setsockopt(ps->sd, PI_LEVEL_PADP, PI_PADP_TYPE, &oldtype, &size);
			break;

		case PI_CMD_NET:
			/* Enter command state */
			ps->command = 1;

			/* Set the type to "tickle" */
			type = PI_NET_TYPE_TCKL;
			size = sizeof(type);
			pi_setsockopt(ps->sd, PI_LEVEL_NET, PI_NET_TYPE, &type, &size);
			
			/* send packet */
			result = ps->cmd_queue[0]->write (ps, msg, len, 0);

			/* Exit command state */
			ps->command = 0;
			break;
	}

	return result;
}


/***********************************************************************
 *
 * Function:    pi_close
 *
 * Summary:     Close a connection, destroy the socket
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_close(int pi_sd)
{
	int 		result = 0;
	pi_socket_t	*ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return PI_ERR_SOCK_INVALID;
	}

	if (ps->type == PI_SOCK_STREAM && ps->cmd != PI_CMD_SYS) {
		if (is_connected (ps)) {
				ps->command = 1;
				
				/* then end sync, with clean status */
				result = dlp_EndOfSync(ps->sd, 0);

				ps->command = 0;
		}
	}

	if (result == 0) {
		/* we need to remove the entry from the list prior to
		 * closing it, because closing it will reset the pi_sd */
		psl = ps_list_remove (psl, pi_sd);
		watch_list = ps_list_remove (watch_list, pi_sd);

		if (ps->device != NULL)
			result = ps->device->close (ps);

		protocol_queue_destroy(ps);

		if (ps->device != NULL)
			ps->device->free(ps->device);

		free(ps);
	}

	return result;
}


/***********************************************************************
 *
 * Function:    pi_getsockname
 *
 * Summary:     Get the local address for a socket
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_getsockname(int pi_sd, struct sockaddr *addr, size_t *namelen)
{
	pi_socket_t *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return PI_ERR_SOCK_INVALID;
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
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_getsockpeer(int pi_sd, struct sockaddr *addr, size_t *namelen)
{
	pi_socket_t *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return PI_ERR_SOCK_INVALID;
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
 * Summary:     return the device's DLP version as a unsigned short
 *		(0xMMmm with MM=major, mm=minor)
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_version(int pi_sd)
{
	size_t 	size;
	pi_socket_t *ps;
	struct  SysInfo si;
	
	/* FIXME This is an ugly hack for versions because cmp doesn't
	 * go beyond 1.1 in its versioning because ReadSysInfo
	 * provides the dlp version in dlp 1.2 and higher */

	if (!(ps = ps_list_find (psl, pi_sd))) {
		errno = ESRCH;
		return PI_ERR_SOCK_INVALID;
	}
	
	if (ps->dlpversion)
		return ps->dlpversion;
	
	if (dlp_ReadSysInfo (ps->sd, &si) < 0)
		return 0x0000;

	if (si.dlpMajorVersion != 0) {
		ps->dlpversion = (si.dlpMajorVersion << 8) | si.dlpMinorVersion;
		ps->maxrecsize = si.maxRecSize;		
		return ps->dlpversion;
	}
	
	/* Enter command state */
	ps->command = 1;

	/* Get the version */
	if (ps->cmd == PI_CMD_CMP) {
		size = sizeof(ps->dlpversion);
		pi_getsockopt(ps->sd, PI_LEVEL_CMP,
			PI_CMP_VERS, &ps->dlpversion, &size);
		ps->maxrecsize = DLP_BUF_SIZE;
	}

	/* Exit command state */
	ps->command = 0;

	return ps->dlpversion;
}

/***********************************************************************
 *
 * Function:    pi_maxrecsize
 *
 * Summary:     return the device's maximum record size, usually 0xffff
 *		but it goes much higher with Palm OS 5+ devices
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
unsigned long
pi_maxrecsize(int pi_sd)
{
	pi_socket_t *ps;

	if (!(ps = ps_list_find (psl, pi_sd))) {
		errno = ESRCH;
		return 0;
	}

	/* pi_version will read necessary info from device */
	if (pi_version(pi_sd) == 0x0000)
		return DLP_BUF_SIZE;

	return ps->maxrecsize;
}

/***********************************************************************
 *
 * Function:    find_pi_socket
 *
 * Summary:     Wrapper for ps_list_find
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
pi_socket_t *
find_pi_socket(int pi_sd)
{
	return ps_list_find (psl, pi_sd);
}

/***********************************************************************
 *
 * Function:    pi_watchdog
 *
 * Summary:     
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_watchdog(int pi_sd, int newinterval)
{
	pi_socket_t *ps;

	if (!(ps = find_pi_socket(pi_sd))) {
		errno = ESRCH;
		return PI_ERR_SOCK_INVALID;
	}

	watch_list = ps_list_append (watch_list, ps);
	signal(SIGALRM, onalarm);
	interval = newinterval;
	alarm(interval);

	return 0;
}

/***********************************************************************
 *
 * Function:    pi_error
 *
 * Summary:     return the last error that occured
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_error(int pi_sd)
{
	pi_socket_t *ps;

	if ((ps = find_pi_socket(pi_sd)) == NULL) {
		errno = ESRCH;
		return PI_ERR_SOCK_INVALID;
	}
	return ps->last_error;
}

/***********************************************************************
 *
 * Function:    pi_set_error
 *
 * Summary:     set the last error code
 *
 * Parameters:  None
 *
 * Returns:     The error code that was just set
 *
 ***********************************************************************/
int
pi_set_error(int pi_sd, int error_code)
{
	pi_socket_t *ps;

	if ((ps = find_pi_socket(pi_sd)))
		ps->last_error = error_code;
	else
		errno = ESRCH;

	/* also update errno if makes sense */
	if (error_code == PI_ERR_GENERIC_MEMORY)
		errno = ENOMEM;

	return error_code;
}

/***********************************************************************
 *
 * Function:    pi_palmos_error
 *
 * Summary:     return the last error returned by the handheld
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_palmos_error(int pi_sd)
{
	pi_socket_t *ps;

	if ((ps = find_pi_socket(pi_sd)) == NULL) {
		errno = ESRCH;
		return PI_ERR_SOCK_INVALID;
	}
	return ps->palmos_error;
}

/***********************************************************************
 *
 * Function:    pi_set_palmos_error
 *
 * Summary:     set the last Palm OS error code
 *
 * Parameters:  sd	-->			socket
 *				erorr_code	-->	Palm OS error code
 *
 * Returns:     the error code that was just set
 *
 ***********************************************************************/
int
pi_set_palmos_error(int pi_sd, int error_code)
{
	pi_socket_t *ps;

	if ((ps = find_pi_socket(pi_sd)))
		ps->palmos_error = error_code;
	else
		errno = ESRCH;
	return error_code;
}

/***********************************************************************
 *
 * Function:    pi_reset_errors
 *
 * Summary:     reset the last error and the last Palm OS error
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void
pi_reset_errors(int pi_sd)
{
	pi_socket_t *ps;

	if ((ps = find_pi_socket(pi_sd))) {
		ps->last_error = 0;
		ps->palmos_error = 0;
	} else
		errno = ESRCH;
}

/***********************************************************************
 *
 * Function:    pi_socket_connected
 *
 * Summary:     returns != 0 if the socket is connected
 *
 * Parameters:  None
 *
 * Returns:     0 for not connected, != 0 otherwise
 *
 ***********************************************************************/
int
pi_socket_connected(int pi_sd)
{
	pi_socket_t *ps;

	if ((ps = find_pi_socket(pi_sd)))
		return is_connected(ps);
	errno = ESRCH;
	return 0;
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
