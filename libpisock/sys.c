/*
 * sys.c:  Pilot System Protocol
 *
 * (c) 1996, Kenneth Albanowski.
 * Derived from padp.c.
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "pi-debug.h"
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-slp.h"
#include "pi-sys.h"

static int sys_getsockopt(struct pi_socket *ps, int level, int option_name, 
			  void *option_value, int *option_len);
static int sys_setsockopt(struct pi_socket *ps, int level, int option_name, 
			  const void *option_value, int *option_len);

static struct pi_protocol *sys_protocol_dup (struct pi_protocol *prot)
{
	struct pi_protocol *new_prot;
	struct pi_sys_data *data, *new_data;
	
	new_prot 		= (struct pi_protocol *)malloc (sizeof (struct pi_protocol));
	new_prot->level 	= prot->level;
	new_prot->dup 		= prot->dup;
	new_prot->free 		= prot->free;
	new_prot->read 		= prot->read;
	new_prot->write 	= prot->write;
	new_prot->getsockopt 	= prot->getsockopt;
	new_prot->setsockopt 	= prot->setsockopt;

	new_data 		= (struct pi_sys_data *)malloc (sizeof (struct pi_sys_data));
	data 			= (struct pi_sys_data *)prot->data;
	new_data->txid 		= data->txid;
	new_prot->data 		= new_data;

	return new_prot;
}

static void sys_protocol_free (struct pi_protocol *prot)
{
	free(prot->data);
	free(prot);
}

struct pi_protocol *sys_protocol (void)
{
	struct 	pi_protocol *prot;
	struct 	pi_sys_data *data;

	prot 			= (struct pi_protocol *)malloc (sizeof (struct pi_protocol));	
	prot->level 		= PI_LEVEL_SYS;
	prot->dup 		= sys_protocol_dup;
	prot->free 		= sys_protocol_free;
	prot->read 		= sys_rx;
	prot->write 		= sys_tx;
	prot->getsockopt 	= sys_getsockopt;
	prot->setsockopt 	= sys_setsockopt;

	data 			= (struct pi_sys_data *)malloc (sizeof (struct pi_sys_data));
	data->txid 		= 0x00;
	prot->data 		= data;
	
	return prot;
}

/***********************************************************************
 *
 * Function:    sys_tx
 *
 * Summary:     Send a system message
 *
 * Parmeters:   None
 *
 * Returns:     0 if success, nonzero otherwise
 *
 ***********************************************************************/
int sys_tx(struct pi_socket *ps, unsigned char *buf, int len, int flags)
{
	struct 	pi_protocol *prot, *next;
	struct 	pi_sys_data *data;
	int 	type,
		socket,
		size;

	prot = pi_protocol(ps->sd, PI_LEVEL_SYS);
	if (prot == NULL)
		return -1;
	data = (struct pi_sys_data *)prot->data;
	next = pi_protocol_next(ps->sd, PI_LEVEL_SYS);
	if (next == NULL)
		return -1;

	if ((!data->txid) || (data->txid == 0xff))
		data->txid = 0x11;	/* some random # */
	data->txid++;
	if ((!data->txid) || (data->txid == 0xff))
		data->txid = 0x11;	/* some random # */

	type 	= PI_SLP_TYPE_RDCP;
	/* Fix me, allow socket type */
	socket 	= PI_SLP_SOCK_CON;
	size 	= sizeof(type);
	pi_setsockopt(ps->sd, PI_LEVEL_SLP, PI_SLP_TYPE, 
		      &type, &size);
	pi_setsockopt(ps->sd, PI_LEVEL_SLP, PI_SLP_DEST, 
		      &socket, &size);
	pi_setsockopt(ps->sd, PI_LEVEL_SLP, PI_SLP_SRC, 
		      &socket, &size);
	size = sizeof(data->txid);
	pi_setsockopt(ps->sd, PI_LEVEL_SLP, PI_SLP_TXID, 
		      &data->txid, &size);
	
	next->write(ps, buf, len, flags);

	CHECK(PI_DBG_SYS, PI_DBG_LVL_INFO, sys_dump_header(buf, 1));
	CHECK(PI_DBG_SYS, PI_DBG_LVL_DEBUG, sys_dump(buf, len));

	return 0;
}

/***********************************************************************
 *
 * Function:    sys_rx
 *
 * Summary:     Receive system message
 *
 * Parmeters:   None
 *
 * Returns:     Length of read
 *
 ***********************************************************************/
int sys_rx(struct pi_socket *ps, unsigned char *buf, int len, int flags)
{
	struct 	pi_protocol *next, *prot;
	struct 	pi_sys_data *data;
	int 	data_len;

	prot = pi_protocol(ps->sd, PI_LEVEL_SYS);
	if (prot == NULL)
		return -1;
	data = (struct pi_sys_data *)prot->data;
	next = pi_protocol_next(ps->sd, PI_LEVEL_SYS);
	if (next == NULL)
		return -1;

	data_len = next->read(ps, buf, len, flags);

	CHECK(PI_DBG_SYS, PI_DBG_LVL_INFO, sys_dump_header(buf, 0));
	CHECK(PI_DBG_SYS, PI_DBG_LVL_DEBUG, sys_dump(buf, data_len));

	return data_len;
}

static int
sys_getsockopt(struct pi_socket *ps, int level, int option_name, 
	       void *option_value, int *option_len)
{
	struct 	pi_protocol *prot;
	struct 	pi_sys_data *data;

	prot = pi_protocol(ps->sd, PI_LEVEL_SYS);
	if (prot == NULL)
		return -1;
	data = (struct pi_sys_data *)prot->data;

	switch (option_name) {

	}
	
	return 0;
	
 error:
	errno = EINVAL;
	return -1;
}

static int
sys_setsockopt(struct pi_socket *ps, int level, int option_name, 
	       const void *option_value, int *option_len)
{
	struct 	pi_protocol *prot;
	struct 	pi_sys_data *data;

	prot = pi_protocol(ps->sd, PI_LEVEL_SYS);
	if (prot == NULL)
		return -1;
	data = (struct pi_sys_data *)prot->data;

	switch (option_name) {

	}

	return 0;
	
 error:
	errno = EINVAL;
	return -1;
}

void sys_dump_header(unsigned char *data, int rxtx)
{
	LOG(PI_DBG_SYS, PI_DBG_LVL_NONE,
	    "SYS %s\n", rxtx ? "TX" : "RX");
}

void sys_dump(unsigned char *data, int len)
{
	dumpdata(&data[PI_SYS_HEADER_LEN], len);
}
