/*
 * cmp.c:  Pilot CMP protocol
 *
 * Copyright (c) 1996, Kenneth Albanowski.
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

#include <stdio.h>

#include "pi-debug.h"
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-padp.h"
#include "pi-cmp.h"
#include "pi-serial.h"

static int cmp_getsockopt(struct pi_socket *ps, int level, int option_name, 
			  void *option_value, int *option_len);
static int cmp_setsockopt(struct pi_socket *ps, int level, int option_name, 
			  const void *option_value, int *option_len);

static struct pi_protocol *cmp_protocol_dup (struct pi_protocol *prot)
{
	struct pi_protocol *new_prot;
	struct pi_cmp_data *data, *new_data;
	
	new_prot = (struct pi_protocol *)malloc (sizeof (struct pi_protocol));
	new_prot->level = prot->level;
	new_prot->dup = prot->dup;
	new_prot->read = prot->read;
	new_prot->write = prot->write;
	new_prot->getsockopt = prot->getsockopt;
	new_prot->setsockopt = prot->setsockopt;

	new_data = (struct pi_cmp_data *)malloc (sizeof (struct pi_cmp_data));
	data = (struct pi_cmp_data *)prot->data;
	new_data->type = data->type;
	new_data->flags = data->type;
	new_data->version = data->type;
	new_data->baudrate = data->type;
	new_prot->data = new_data;

	return new_prot;
}

struct pi_protocol *cmp_protocol (void)
{
	struct pi_protocol *prot;
	struct pi_cmp_data *data;

	prot = (struct pi_protocol *)malloc (sizeof (struct pi_protocol));	
	prot->level = PI_LEVEL_CMP;
	prot->dup = cmp_protocol_dup;
	prot->read = cmp_rx;
	prot->write = cmp_tx;
	prot->getsockopt = cmp_getsockopt;
	prot->setsockopt = cmp_setsockopt;

	data = (struct pi_cmp_data *)malloc (sizeof (struct pi_cmp_data));
	data->type = 0;
	data->flags = 0;
	data->version = 0;
	data->baudrate = 0;
	prot->data = data;
	
	return prot;
}

int
cmp_rx_handshake(struct pi_socket *ps, unsigned long establishrate, int establishhighrate) 
{
	struct pi_protocol *prot;
	struct pi_cmp_data *data;
	unsigned char buf[PI_CMP_HEADER_LEN];

	prot = pi_protocol(ps->sd, PI_LEVEL_CMP);
	if (prot == NULL)
		return -1;
	data = (struct pi_cmp_data *)prot->data;

	/* Check for a proper cmp connection */
	if (cmp_rx(ps, buf, PI_CMP_HEADER_LEN) < 0)
		return -1;	/* Failed to establish connection, errno already set */

	if ((data->version & 0xFF00) == 0x0100) {
		if (establishrate > data->baudrate) {
			if (establishhighrate) {
				LOG(PI_DBG_CMP, PI_DBG_LVL_NONE, 
				    "CMP Establishing higher rate %ul (%ul)\n",
				    establishrate, data->baudrate);
				data->baudrate = establishrate;
			}
		} else {
			data->baudrate = establishrate;
		}
		
		if (cmp_init(ps, data->baudrate) < 0)
			return -1;
	} else {
		/* 0x80 means the comm version wasn't compatible */
		cmp_abort(ps, 0x80);
		errno = ECONNREFUSED;
		return -1;
	}

	return 0;
}

int
cmp_tx_handshake(struct pi_socket *ps) 
{
	struct pi_protocol *prot;
	struct pi_cmp_data *data;

	prot = pi_protocol(ps->sd, PI_LEVEL_CMP);
	if (prot == NULL)
		return -1;
	data = (struct pi_cmp_data *)prot->data;

	if (cmp_wakeup(ps, 38400) < 0)	/* Assume this box can't go over 38400 */
		return -1;

	if (cmp_rx(ps, NULL, 0) < 0)
		return -1;	/* failed to read, errno already set */

	switch (data->type) {
	case PI_CMP_TYPE_INIT:
		return 0;
	case PI_CMP_TYPE_ABRT:
		LOG(PI_DBG_CMP, PI_DBG_LVL_NONE, "CMP Aborted by other end\n");
		errno = -EIO;
		return -1;
	}

	return -1;

}

int cmp_tx(struct pi_socket *ps, unsigned char *buf, int len)
{
	struct pi_protocol *prot, *next;
	struct pi_cmp_data *data;
	unsigned char cmp_buf[PI_CMP_HEADER_LEN];
	int bytes, type, size;

	prot = pi_protocol(ps->sd, PI_LEVEL_CMP);
	if (prot == NULL)
		return -1;
	data = (struct pi_cmp_data *)prot->data;
	next = pi_protocol_next(ps->sd, PI_LEVEL_CMP);
	if (next == NULL)
		return -1;

	type = padData;
	size = sizeof(type);
	pi_setsockopt(ps->sd, PI_LEVEL_PADP, PI_PADP_TYPE, 
		      &type, &size);
	
	set_byte(&cmp_buf[PI_CMP_OFFSET_TYPE], data->type);
	set_byte(&cmp_buf[PI_CMP_OFFSET_FLGS], data->flags);
	set_short(&cmp_buf[PI_CMP_OFFSET_VERS], 0);
	set_short(&cmp_buf[PI_CMP_OFFSET_RESV], 0);
	set_long(&cmp_buf[PI_CMP_OFFSET_BAUD], data->baudrate);

	CHECK(PI_DBG_CMP, PI_DBG_LVL_INFO, cmp_dump(cmp_buf, 1));

	bytes = next->write(ps, cmp_buf, PI_CMP_HEADER_LEN);
	if (bytes < 10)
		return -1;

	return 0;
}

/***********************************************************************
 *
 * Function:    cmp_rx
 *
 * Summary:     Receive CMP packets
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int cmp_rx(struct pi_socket *ps, unsigned char *msg, int len)
{
	struct pi_protocol *prot, *next;
	struct pi_cmp_data *data;
	int bytes;

	prot = pi_protocol(ps->sd, PI_LEVEL_CMP);
	if (prot == NULL)
		return -1;
	data = (struct pi_cmp_data *)prot->data;
	next = pi_protocol_next(ps->sd, PI_LEVEL_CMP);
	if (next == NULL)
		return -1;

	bytes = next->read(ps, msg, len);
	if (bytes < 10)
		return -1;

	CHECK(PI_DBG_CMP, PI_DBG_LVL_INFO, cmp_dump(msg, 0));

	data->type = get_byte(&msg[PI_CMP_OFFSET_TYPE]);
	data->flags = get_byte(&msg[PI_CMP_OFFSET_FLGS]);
	data->version = get_short(&msg[PI_CMP_OFFSET_VERS]);
	data->baudrate = get_long(&msg[PI_CMP_OFFSET_BAUD]);

	return 0;
}

/***********************************************************************
 *
 * Function:    cmp_init
 *
 * Summary:     Initialize the socket for CMP transmission
 *
 * Parmeters:   None
 *
 * Returns:     Number of packets transmitted
 *
 ***********************************************************************/
int cmp_init(struct pi_socket *ps, int baudrate)
{	
	struct pi_protocol *prot;
	struct pi_cmp_data *data;
	
	prot = pi_protocol(ps->sd, PI_LEVEL_CMP);
	if (prot == NULL)
		return -1;
	data = (struct pi_cmp_data *)prot->data;

	data->type = PI_CMP_TYPE_INIT;
	if (baudrate != data->baudrate)
		data->flags = 0x80;
	else
		data->flags = 0x00;
	data->baudrate = baudrate;

	return cmp_tx(ps, NULL, 0);
}

/***********************************************************************
 *
 * Function:    cmp_abort
 *
 * Summary:     Abort a CMP session in progress
 *
 * Parmeters:   None
 *
 * Returns:     Number of PADP packets transmitted
 *
 ***********************************************************************/
int cmp_abort(struct pi_socket *ps, int reason)
{
	struct pi_protocol *prot;
	struct pi_cmp_data *data;
	
	prot = pi_protocol(ps->sd, PI_LEVEL_CMP);
	if (prot == NULL)
		return -1;
	data = (struct pi_cmp_data *)prot->data;

	data->type = PI_CMP_TYPE_ABRT;
	data->flags = reason;

	LOG(PI_DBG_CMP, PI_DBG_LVL_NONE, "CMP ABORT\n");

	return cmp_tx (ps, NULL, 0);
}

/***********************************************************************
 *
 * Function:    cmp_wakeup
 *
 * Summary:     Wakeup the CMP listener process
 *
 * Parmeters:   None
 *
 * Returns:     Number of PADP packets transmitted
 *
 ***********************************************************************/
int cmp_wakeup(struct pi_socket *ps, int maxbaud)
{
	struct pi_protocol *prot;
	struct pi_cmp_data *data;
	
	prot = pi_protocol(ps->sd, PI_LEVEL_CMP);
	if (prot == NULL)
		return -1;
	data = (struct pi_cmp_data *)prot->data;

	data->type = PI_CMP_TYPE_WAKE;
	data->flags = 0;
	data->version = CommVersion_1_0;
	data->baudrate = maxbaud;

	return cmp_tx(ps, NULL, 0);
}

static int
cmp_getsockopt(struct pi_socket *ps, int level, int option_name, 
	       void *option_value, int *option_len)
{
	struct pi_protocol *prot;
	struct pi_cmp_data *data;

	prot = pi_protocol(ps->sd, PI_LEVEL_CMP);
	if (prot == NULL)
		return -1;
	data = (struct pi_cmp_data *)prot->data;

	switch (option_name) {
	case PI_CMP_TYPE:
		if (*option_len < sizeof (data->type))
			goto error;
		memcpy (option_value, &data->type,
			sizeof (data->type));
		*option_len = sizeof (data->type);
		break;
	case PI_CMP_FLAGS:
		if (*option_len < sizeof (data->flags))
			goto error;
		memcpy (option_value, &data->flags,
			sizeof (data->flags));
		*option_len = sizeof (data->flags);
		break;
	case PI_CMP_VERS:
		if (*option_len < sizeof (data->version))
			goto error;
		memcpy (option_value, &data->version,
			sizeof (data->version));
		*option_len = sizeof (data->version);
		break;
	case PI_CMP_BAUD:
		if (*option_len < sizeof (data->baudrate))
			goto error;
		memcpy (option_value, &data->baudrate,
			sizeof (data->baudrate));
		*option_len = sizeof (data->baudrate);
		break;
	}

	return 0;
	
 error:
	errno = EINVAL;
	return -1;
}

static int
cmp_setsockopt(struct pi_socket *ps, int level, int option_name, 
	       const void *option_value, int *option_len)
{
	struct pi_protocol *prot;
	struct pi_padp_data *data;

	prot = pi_protocol(ps->sd, PI_LEVEL_PADP);
	if (prot == NULL)
		return -1;
	data = (struct pi_padp_data *)prot->data;

	switch (option_name) {
	case PI_PADP_TYPE:
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

/***********************************************************************
 *
 * Function:    cmp_dump
 *
 * Summary:     Dump the CMP packet frames
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void cmp_dump(unsigned char *cmp, int rxtx)
{
	char *type;
	
	switch (get_byte(&cmp[PI_CMP_OFFSET_TYPE])) {
	case PI_CMP_TYPE_WAKE:
		type = "WAKE";
		break;
	case PI_CMP_TYPE_INIT:
		type = "INIT";
		break;
	case PI_CMP_TYPE_ABRT:
		type = "ABRT";
		break;
	default:
		type = "UNK";
		break;
	}
	
	LOG(PI_DBG_CMP, PI_DBG_LVL_NONE,
	    "CMP %s %s", rxtx ? "TX" : "RX", type);
	LOG(PI_DBG_CMP, PI_DBG_LVL_NONE,
	    "  Type: %2.2X Flags: %2.2X Version: %8.8lX Baud: %8.8lX (%ld)\n",
	    get_byte(&cmp[PI_CMP_OFFSET_TYPE]), 
	    get_byte(&cmp[PI_CMP_OFFSET_FLGS]),
	    get_long(&cmp[PI_CMP_OFFSET_VERS]),
	    get_long(&cmp[PI_CMP_OFFSET_BAUD]),
	    get_long(&cmp[PI_CMP_OFFSET_BAUD]));
}
