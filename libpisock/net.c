/*
 * net.c: Protocl for NetSync connections
 *
 * Copyright (c) 1997, Kenneth Albanowski
 * Copyright (c) 1999, Tilo Christ
 * Copyright (c) 1999, John Franks
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "pi-debug.h"
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-net.h"

#define PI_NET_TIMEOUT 10*1000

static int net_getsockopt(struct pi_socket *ps, int level, int option_name, 
			  void *option_value, int *option_len);
static int net_setsockopt(struct pi_socket *ps, int level, int option_name, 
			  const void *option_value, int *option_len);

static struct pi_protocol *net_protocol_dup (struct pi_protocol *prot)
{
	struct pi_protocol *new_prot;
	struct pi_net_data *data, *new_data;
	
	new_prot 		= (struct pi_protocol *)malloc (sizeof (struct pi_protocol));
	new_prot->level 	= prot->level;
	new_prot->dup 		= prot->dup;
	new_prot->free 		= prot->free;
	new_prot->read 		= prot->read;
	new_prot->write 	= prot->write;
	new_prot->getsockopt 	= prot->getsockopt;
	new_prot->setsockopt 	= prot->setsockopt;

	new_data 		= (struct pi_net_data *)malloc (sizeof (struct pi_net_data));
	data 			= (struct pi_net_data *)prot->data;
	new_data->type 		= data->type;
	new_data->txid 		= data->txid;
	new_prot->data 		= new_data;

	return new_prot;
}

static void net_protocol_free (struct pi_protocol *prot)
{
	free(prot->data);
	free(prot);
}

struct pi_protocol *net_protocol (void)
{
	struct 	pi_protocol *prot;
	struct 	pi_net_data *data;

	prot 			= (struct pi_protocol *)malloc (sizeof (struct pi_protocol));	
	prot->level 		= PI_LEVEL_NET;
	prot->dup 		= net_protocol_dup;
	prot->free 		= net_protocol_free;
	prot->read 		= net_rx;
	prot->write 		= net_tx;
	prot->getsockopt 	= net_getsockopt;
	prot->setsockopt 	= net_setsockopt;

	data 			= (struct pi_net_data *)malloc (sizeof (struct pi_net_data));
	data->type 		= PI_NET_TYPE_DATA;
	data->txid 		= 0x00;
	prot->data 		= data;
	
	return prot;
}

int
net_rx_handshake(struct pi_socket *ps)
{
	unsigned char msg1[50] = 
		"\x12\x01\x00\x00\x00\x00\x00\x00\x00\x20\x00\x00\x00"
		"\x24\xff\xff\xff\xff\x3c\x00\x3c\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\xc0\xa8\xa5\x1f\x04\x27\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
	unsigned char msg2[46] = 
		"\x13\x01\x00\x00\x00\x00\x00\x00\x00\x20\x00\x00\x00"
		"\x20\xff\xff\xff\xff\x00\x3c\x00\x3c\x00\x00\x00\x00"
		"\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00";
	unsigned char buffer[200];

	
	if (net_rx(ps, buffer, 22, 0) < 0)
		return -1;
	if (net_tx(ps, msg1, 50, 0) < 0)
		return -1;
	if (net_rx(ps, buffer, 50, 0) < 0)
		return -1;
	if (net_tx(ps, msg2, 46, 0) < 0)
		return -1;
	if (net_rx(ps, buffer, 8, 0) < 0)
		return -1;

	return 0;
}

int
net_tx_handshake(struct pi_socket *ps)
{
	unsigned char msg1[22] = 
		"\x90\x01\x00\x00\x00\x00\x00\x00\x00\x20\x00\x00\x00"
		"\x08\x01\x00\x00\x00\x00\x00\x00\x00";
	unsigned char msg2[50] = 
		"\x92\x01\x00\x00\x00\x00\x00\x00\x00\x20\x00\x00\x00"
		"\x24\xff\xff\xff\xff\x00\x3c\x00\x3c\x40\x00\x00\x00"
		"\x01\x00\x00\x00\xc0\xa8\xa5\x1e\x04\x01\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
	unsigned char msg3[8]  = 
		"\x93\x00\x00\x00\x00\x00\x00\x00";
	unsigned char buffer[200];
	
	if (net_tx(ps, msg1, 22, 0) < 0)
		return -1;
	if (net_rx(ps, buffer, 50, 0) < 0)
		return -1;
	if (net_tx(ps, msg2, 50, 0) < 0)
		return -1;
	if (net_rx(ps, buffer, 46, 0) < 0)
		return -1;
	if (net_tx(ps, msg3, 8, 0) < 0)
		return -1;

	return 0;
}

int
net_tx(struct pi_socket *ps, unsigned char *msg, int len, int flags)
{
	int 	bytes;
	struct 	pi_protocol *prot, *next;
	struct 	pi_net_data *data;
	unsigned char buf[PI_NET_HEADER_LEN];

	prot = pi_protocol(ps->sd, PI_LEVEL_NET);
	if (prot == NULL)
		return -1;
	data = (struct pi_net_data *)prot->data;
	next = pi_protocol_next(ps->sd, PI_LEVEL_NET);
	if (next == NULL)
		return -1;

	/* Create the header */
	buf[PI_NET_OFFSET_TYPE] = data->type;
	if (data->type == PI_NET_TYPE_TCKL)
		buf[PI_NET_OFFSET_TXID] = 0xff;
	else
		buf[PI_NET_OFFSET_TXID] = data->txid;
	set_long(&buf[PI_NET_OFFSET_SIZE], len);

	/* Write the header and body */
	bytes = next->write(ps, buf, PI_NET_HEADER_LEN, flags);
	if (bytes < PI_NET_HEADER_LEN)
		return bytes;
	bytes = next->write(ps, msg, len, flags);
	if (bytes < len)
		return bytes;

	CHECK(PI_DBG_NET, PI_DBG_LVL_INFO, net_dump_header(buf, 1));
	CHECK(PI_DBG_NET, PI_DBG_LVL_DEBUG, dumpdata(msg, len));
	
	return len;
}

int
net_rx(struct pi_socket *ps, unsigned char *msg, int len, int flags)
{
	int 	bytes, 
		total_bytes, 
		packet_len,
		timeout,
		size;
	
	struct 	pi_protocol *prot, *next;
	struct 	pi_net_data *data;
		
	unsigned char *cur;

	prot = pi_protocol(ps->sd, PI_LEVEL_NET);
	if (prot == NULL)
		return -1;
	data = (struct pi_net_data *)prot->data;
	next = pi_protocol_next(ps->sd, PI_LEVEL_NET);
	if (next == NULL)
		return -1;

	timeout = PI_NET_TIMEOUT;
	size = sizeof(timeout);
	pi_setsockopt(ps->sd, PI_LEVEL_DEV, PI_DEV_TIMEOUT, 
		      &timeout, &size);

	total_bytes = 0;
	if (data->txid == 0) {	
		/* Peek to see if it is a headerless packet */
		bytes = next->read(ps, msg, 1, flags);
		if (bytes > 0) {
			LOG ((PI_DBG_NET, PI_DBG_LVL_INFO,
			     "NET RX: Checking for headerless packet %d\n", msg[0]));

			if (msg[0] == 0x90) {
				/* Cause the header bytes to be skipped */
				LOG ((PI_DBG_NET, PI_DBG_LVL_INFO,
				     "NET RX: Headerless packet\n"));
				total_bytes = PI_NET_HEADER_LEN;
				msg[PI_NET_OFFSET_TYPE] = PI_NET_TYPE_DATA;
				msg[PI_NET_OFFSET_TXID] = 0x01;
				set_long (&msg[PI_NET_OFFSET_SIZE], 21);
			} else {
				total_bytes += bytes;
			}
		} else {
			return bytes;
		}
	}
	
	/* Bytes in what's left of the header */
	while (total_bytes < PI_NET_HEADER_LEN) {
		bytes = next->read(ps, msg + total_bytes, PI_NET_HEADER_LEN - total_bytes, flags);
		if (bytes <= 0)
			return bytes;
		total_bytes += bytes;
	}

	/* Bytes in the rest of the packet */
	packet_len = get_long(&msg[PI_NET_OFFSET_SIZE]);
	while (total_bytes < PI_NET_HEADER_LEN + packet_len) {
		bytes = next->read(ps, msg + total_bytes, 
				  PI_NET_HEADER_LEN + packet_len - total_bytes, flags);
		if (bytes <= 0)
			return bytes;
		else if (bytes > 0)
			total_bytes += bytes;
	}

	CHECK(PI_DBG_NET, PI_DBG_LVL_INFO, net_dump_header(msg, 0));
	CHECK(PI_DBG_NET, PI_DBG_LVL_DEBUG, net_dump(msg));

	/* Update the transaction id */
	if (ps->state == PI_SOCK_CONIN || ps->command == 1)
		data->txid = msg[PI_NET_OFFSET_TXID];
	else {
		data->txid++;
		if (data->txid == 0xff)
			data->txid = 1;
	}

	/* Remove the header */
	cur = msg + PI_NET_HEADER_LEN;
	memmove (msg, cur, packet_len);	

	return packet_len;
}

static int
net_getsockopt(struct pi_socket *ps, int level, int option_name, 
	       void *option_value, int *option_len)
{
	struct 	pi_protocol *prot;
	struct 	pi_net_data *data;

	prot = pi_protocol(ps->sd, PI_LEVEL_NET);
	if (prot == NULL)
		return -1;
	data = (struct pi_net_data *)prot->data;

	switch (option_name) {
	case PI_NET_TYPE:
		if (*option_len < sizeof (data->type))
			goto error;
		memcpy (option_value, &data->type,
			sizeof (data->type));
		*option_len = sizeof (data->type);
		break;
	}
	
	return 0;
	
 error:
	errno = EINVAL;
	return -1;
}

static int
net_setsockopt(struct pi_socket *ps, int level, int option_name, 
	       const void *option_value, int *option_len)
{
	struct 	pi_protocol *prot;
	struct 	pi_net_data *data;

	prot = pi_protocol(ps->sd, PI_LEVEL_NET);
	if (prot == NULL)
		return -1;
	data = (struct pi_net_data *)prot->data;

	switch (option_name) {
	case PI_NET_TYPE:
		if (*option_len != sizeof (data->type))
			goto error;
		memcpy (&data->type, option_value,
			sizeof (data->type));
		*option_len = sizeof (data->type);
		break;
	}

	return 0;
	
 error:
	errno = EINVAL;
	return -1;
}

void net_dump_header(unsigned char *data, int rxtx)
{
	LOG((PI_DBG_NET, PI_DBG_LVL_NONE,
	    "NET %s type=%d txid=0x%.2x len=0x%.4x\n",
	    rxtx ? "TX" : "RX",
	    get_byte(&data[PI_NET_OFFSET_TYPE]),
	    get_byte(&data[PI_NET_OFFSET_TXID]),
	    get_long(&data[PI_NET_OFFSET_SIZE])));
}

void net_dump(unsigned char *data)
{
	int 	size;

	size = get_long(&data[PI_NET_OFFSET_SIZE]);
	dumpdata(&data[PI_NET_HEADER_LEN], size);
}
