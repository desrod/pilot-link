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

#include <stdio.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-net.h"

void nh(unsigned char *d);

static int net_getsockopt(struct pi_socket *ps, int level, int option_name, 
			  void *option_value, int *option_len);
static int net_setsockopt(struct pi_socket *ps, int level, int option_name, 
			  const void *option_value, int *option_len);

static struct pi_protocol *net_protocol_dup (struct pi_protocol *prot)
{
	struct pi_protocol *new_prot;
	struct pi_net_data *data, *new_data;
	
	new_prot = (struct pi_protocol *)malloc (sizeof (struct pi_protocol));
	new_prot->level = prot->level;
	new_prot->dup = prot->dup;
	new_prot->read = prot->read;
	new_prot->write = prot->write;
	new_prot->getsockopt = prot->getsockopt;
	new_prot->setsockopt = prot->setsockopt;

	new_data = (struct pi_net_data *)malloc (sizeof (struct pi_net_data));
	data = (struct pi_net_data *)prot->data;
	new_data->txid = data->txid;
	new_prot->data = new_data;

	return new_prot;
}

struct pi_protocol *net_protocol (void)
{
	struct pi_protocol *prot;
	struct pi_net_data *data;

	prot = (struct pi_protocol *)malloc (sizeof (struct pi_protocol));	
	prot->level = PI_LEVEL_NET;
	prot->dup = net_protocol_dup;
	prot->read = net_rx;
	prot->write = net_tx;
	prot->getsockopt = net_getsockopt;
	prot->setsockopt = net_setsockopt;

	data = (struct pi_net_data *)malloc (sizeof (struct pi_net_data));
	data->txid = 0x00;
	prot->data = data;
	
	return prot;
}

int
net_rx_handshake(struct pi_socket *ps)
{
	char msg1[50] = "\x12\x01\x00\x00\x00\x00\x00\x00\x00\x20\x00\x00\x00"
                        "\x24\xff\xff\xff\xff\x3c\x00\x3c\x00\x00\x00\x00\x00"
                        "\x00\x00\x00\x00\xc0\xa8\xa5\x1f\x04\x27\x00\x00\x00"
                        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
	char msg2[46] = "\x13\x01\x00\x00\x00\x00\x00\x00\x00\x20\x00\x00\x00"
                        "\x20\xff\xff\xff\xff\x00\x3c\x00\x3c\x00\x00\x00\x00"
                        "\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                        "\x00\x00\x00\x00\x00\x00\x00";
	char buffer[200];
	
	if (net_rx(ps, buffer, 200) < 0)
		return -1;
	if (net_tx(ps, msg1, 50) < 0)
		return -1;
	if (net_rx(ps, buffer, 200) < 0)
		return -1;
	if (net_tx(ps, msg2, 46) < 0)
		return -1;
	if (net_rx(ps, buffer, 200) < 0)
		return -1;

	return 0;
}

int
net_tx(struct pi_socket *ps, unsigned char *msg, int len)
{
	struct pi_protocol *prot, *next;
	struct pi_net_data *data;
	unsigned char buf[6];
	int result;
	
	printf ("NET TX\n");

	prot = pi_protocol(ps->sd, PI_LEVEL_NET);
	if (prot == NULL)
		return -1;
	data = (struct pi_net_data *)prot->data;
	next = pi_protocol_next(ps->sd, PI_LEVEL_NET);
	if (next == NULL)
		return -1;

	buf[PI_NET_OFFSET_TYPE] = 0x01;
	buf[PI_NET_OFFSET_TXID] = data->txid;
	set_long(&buf[PI_NET_OFFSET_SIZE], len);
	nh(buf);
	
	result = next->write(ps, buf, PI_NET_HEADER_LEN);
	if (result < 0)
		return result;
	result = next->write(ps, msg, len);
	if (result < 0)
		return result;

#ifndef NO_SERIAL_TRACE
	if (ps->debuglog) {
		buf[0] = 4;
		buf[1] = 0;
		set_long(buf + 2, len);
		write(ps->debugfd, buf, 6);
		write(ps->debugfd, msg, len);
	}
#endif

	return len;
}

int
net_rx(struct pi_socket *ps, unsigned char *msg, int len)
{
	struct pi_protocol *prot, *next;
	struct pi_net_data *data;
	int n, l;
	unsigned char buf[6];
	int rlen;

	printf ("NET RX\n");
	
	prot = pi_protocol(ps->sd, PI_LEVEL_NET);
	if (prot == NULL)
		return -1;
	data = (struct pi_net_data *)prot->data;
	next = pi_protocol_next(ps->sd, PI_LEVEL_NET);
	if (next == NULL)
		return -1;

	l = 0;
	if (data->txid == 0) {		
		/* Peek to see if it is a headerless packet */
		n = next->read(ps, buf, 1);
		if (n > 0) {
			if (buf[0] == 0x90) {
				l = PI_NET_HEADER_LEN + 1;
				buf[PI_NET_OFFSET_TYPE] = 0x01;
				buf[PI_NET_OFFSET_TXID] = 0x01;
				set_long (&buf[PI_NET_OFFSET_SIZE], 21);
			} else {
				l += n;
			}
		}
		if (n <= 0)
			return n;
	}
	
	while (l < PI_NET_HEADER_LEN) {
		n = next->read(ps, buf + l, PI_NET_HEADER_LEN - l);
		if (n > 0)
			l += n;
		if (n <= 0)
			return n;
	}
	if (l == PI_NET_HEADER_LEN + 1)
		printf ("NET RX: Headerless packet\n");
	nh(buf);
	
	rlen = get_long(&buf[PI_NET_OFFSET_SIZE]);
	if (len > rlen)
		len = rlen;

	l = 0;
	while (l < len) {
		n = next->read(ps, msg, len - l);
		if (n > 0)
			l += n;
		if (n < 0)
			return n;
		if (n == 0) {
			len = l;
			break;
		}
	}

	if (l < rlen) {
		char discard;

		while (l < rlen) {
			n = read(ps->sd, &discard, 1);
			if (n > 0)
				l += n;
			if (n < 0)
				return n;
			if (n == 0)
				break;
		}
	}

	if (ps->initiator)
		data->txid = buf[1];
	else {
		data->txid++;
		if (data->txid == 0xff)
			data->txid = 1;
	}

#ifndef NO_SERIAL_TRACE
	if (ps->debuglog) {
		buf[0] = 3;
		buf[1] = 0;
		set_long(buf + 2, len);
		write(ps->debugfd, buf, 6);
		write(ps->debugfd, msg, len);
	}
#endif

	return len;
}

static int
net_getsockopt(struct pi_socket *ps, int level, int option_name, 
	       void *option_value, int *option_len)
{
	struct pi_protocol *prot;
	struct pi_net_data *data;

	prot = pi_protocol(ps->sd, PI_LEVEL_NET);
	if (prot == NULL)
		return -1;
	data = (struct pi_net_data *)prot->data;

	return 0;
	
 error:
	errno = EINVAL;
	return -1;
}

static int
net_setsockopt(struct pi_socket *ps, int level, int option_name, 
	       const void *option_value, int *option_len)
{
	struct pi_protocol *prot;
	struct pi_net_data *data;

	prot = pi_protocol(ps->sd, PI_LEVEL_NET);
	if (prot == NULL)
		return -1;
	data = (struct pi_net_data *)prot->data;

	return 0;
	
 error:
	errno = EINVAL;
	return -1;
}

void nh(unsigned char *d)
{
#ifdef DEBUG
	int i;

	fprintf(stderr, "NET HDR [");
	for (i = 0; i < 6; i++)
		fprintf(stderr, " 0x%.2x", 0xff & d[i]);
	fprintf(stderr, "]\n");
#endif
}
