/*
 * padp.c:  Pilot PADP protocol
 *
 * (c) 1996, D. Jeff Dionne.
 * Much of this code adapted from Brian J. Swetland <swetland@uiuc.edu>
 *
 * Mostly rewritten by Kenneth Albanowski.  Adjusted timeout values and
 * better error handling by Tilo Christ.
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. *
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
#include "pi-padp.h"
#include "pi-slp.h"
#include "pi-error.h"

#define PI_PADP_TX_TIMEOUT 2*1000
#define PI_PADP_TX_RETRIES 10
#define PI_PADP_RX_BLOCK_TO 30*1000
#define PI_PADP_RX_SEGMENT_TO 30*1000

/* Declare function prototypes */
static int padp_flush(pi_socket_t *ps, int flags);
static int padp_getsockopt(pi_socket_t *ps, int level, int option_name, 
				void *option_value, size_t *option_len);
static int padp_setsockopt(pi_socket_t *ps, int level, int option_name, 
				const void *option_value, size_t *option_len);
static int padp_sendack(struct pi_socket *ps, struct pi_padp_data *data,
				unsigned char txid, struct padp *padp, int flags);


/***********************************************************************
 *
 * Function:    padp_protocol_dup
 *
 * Summary:     clones an existing pi_protocol struct
 *
 * Parameters:  pi_protocol*
 *
 * Returns:     pi_protocol_t* or NULL if operation failed
 *
 ***********************************************************************/
static pi_protocol_t
*padp_protocol_dup (pi_protocol_t *prot)
{
	pi_protocol_t	*new_prot = NULL;
	pi_padp_data_t	*data = NULL,
			*new_data = NULL;
	
	new_prot = (pi_protocol_t *)malloc (sizeof (pi_protocol_t));
	if (new_prot != NULL) {
		new_data = (pi_padp_data_t *)malloc (sizeof (pi_padp_data_t));
		if (new_data == NULL) {
			free(new_prot);
			new_prot = NULL;
		} else {
			new_prot->level = prot->level;
			new_prot->dup 	= prot->dup;
			new_prot->free 	= prot->free;
			new_prot->read 	= prot->read;
			new_prot->write = prot->write;
			new_prot->flush	= prot->flush;
			new_prot->getsockopt = prot->getsockopt;
			new_prot->setsockopt = prot->setsockopt;

			data = (pi_padp_data_t *)prot->data;
			new_data->type	= data->type;
			new_data->last_type = data->last_type;
			new_data->txid 	= data->txid;
			new_data->next_txid = data->next_txid;
			new_prot->data 	= new_data;
		}
	}

	return new_prot;
}


/***********************************************************************
 *
 * Function:    padp_protocol_free
 *
 * Summary:     frees an existing pi_protocol struct
 *
 * Parameters:  pi_protocol*
 *
 * Returns:     void
 *
 ***********************************************************************/
static
void padp_protocol_free (pi_protocol_t *prot)
{
	ASSERT (prot != NULL);

	if (prot != NULL) {
		if (prot->data != NULL)
			free(prot->data);
		free(prot);
	}
}


/***********************************************************************
 *
 * Function:    padp_protocol
 *
 * Summary:     creates and inits pi_protocol struct instance
 *
 * Parameters:  void
 *
 * Returns:     pi_protocol_t* or NULL if operation failed
 *
 ***********************************************************************/
pi_protocol_t
*padp_protocol (void)
{
	pi_protocol_t *prot = NULL;
	pi_padp_data_t *data = NULL;

	prot = (pi_protocol_t *) malloc (sizeof (pi_protocol_t));	
	if (prot != NULL) {
		data = (pi_padp_data_t *) malloc (sizeof (pi_padp_data_t));
		if (data == NULL) {
			free(prot);
			prot = NULL;
		} else {
			prot->level	= PI_LEVEL_PADP;
			prot->dup 	= padp_protocol_dup;
			prot->free 	= padp_protocol_free;
			prot->read 	= padp_rx;
			prot->write 	= padp_tx;
			prot->flush	= padp_flush;
			prot->getsockopt = padp_getsockopt;
			prot->setsockopt = padp_setsockopt;

			data->type 	= padData;
			data->last_type = -1;
			data->txid 	= 0xff;
			data->next_txid = 0xff;
			prot->data 	= data;
		}
	}

	return prot;
}

/***********************************************************************
 *
 * Function:    padp_tx
 *
 * Summary:     Transmit PADP packets
 *
 * Parameters:  pi_socket_t*, char* to buffer, buffer length, flags
 *
 * Returns:     Number of packets transmitted
 *
 ***********************************************************************/
ssize_t
padp_tx(pi_socket_t *ps, unsigned char *buf, size_t len, int flags)
{
	int 	fl 	= FIRST,
		count 	= 0,
		retries;
	size_t	tlen;
	pi_protocol_t *prot, *next;
	pi_padp_data_t *data;
	pi_buffer_t *padp_buf;
	struct padp padp;

	prot = pi_protocol(ps->sd, PI_LEVEL_PADP);
	if (prot == NULL)
		return pi_set_error(ps->sd, PI_ERR_SOCK_INVALID);

	data = (pi_padp_data_t *)prot->data;
	next = pi_protocol_next(ps->sd, PI_LEVEL_PADP);
	if (next == NULL)
		return pi_set_error(ps->sd, PI_ERR_SOCK_INVALID);

	if (data->type == padWake)
		data->txid = 0xff;

	if (data->txid == 0)
		data->txid = 0x10;	/* some random # */
	else if (data->txid >= 0xfe)
		data->next_txid = 1;	/* wrap */
	else
		data->next_txid = data->txid + 1;

	if (data->type != padAck && ps->state == PI_SOCK_CONAC)
		data->txid = data->next_txid;

	padp_buf = pi_buffer_new (PI_PADP_HEADER_LEN + PI_PADP_MTU);
	if (padp_buf == NULL)
		return pi_set_error(ps->sd, PI_ERR_GENERIC_MEMORY);

	do {
		retries = PI_PADP_TX_RETRIES;
		do {
			int 	type,
				socket,
				timeout;
			size_t	size;

			padp_buf->used = 0;

			type 	= PI_SLP_TYPE_PADP;
			socket 	= PI_SLP_SOCK_DLP;
			timeout = PI_PADP_TX_TIMEOUT;
			size 	= sizeof(type);
			pi_setsockopt(ps->sd, PI_LEVEL_SLP, PI_SLP_TYPE, 
				      &type, &size);
			pi_setsockopt(ps->sd, PI_LEVEL_SLP, PI_SLP_DEST, 
				      &socket, &size);
			pi_setsockopt(ps->sd, PI_LEVEL_SLP, PI_SLP_SRC, 
				      &socket, &size);
			size = sizeof(timeout);
			pi_setsockopt(ps->sd, PI_LEVEL_DEV, PI_DEV_TIMEOUT, 
				      &timeout, &size);
			size = sizeof(data->txid);
			pi_setsockopt(ps->sd, PI_LEVEL_SLP, PI_SLP_TXID, 
				      &data->txid, &size);

			tlen = (len > PI_PADP_MTU) ? PI_PADP_MTU : len;

			padp.type 	= data->type & 0xff;
			padp.flags 	= fl | (len == tlen ? LAST : 0);
			padp.size 	= (fl ? len : (size_t)count);

			/* Construct the packet */
			set_byte(&padp_buf->data[PI_PADP_OFFSET_TYPE], padp.type);
			set_byte(&padp_buf->data[PI_PADP_OFFSET_FLGS], padp.flags);
			set_short(&padp_buf->data[PI_PADP_OFFSET_SIZE], padp.size);
			memcpy(padp_buf->data + PI_PADP_HEADER_LEN, buf, tlen);

			CHECK(PI_DBG_PADP, PI_DBG_LVL_INFO,
				padp_dump_header(padp_buf->data, 1));
			CHECK(PI_DBG_PADP, PI_DBG_LVL_DEBUG,
				padp_dump(padp_buf->data));
			
			/* FIXME: write return code not handled */
			next->write(ps, padp_buf->data, tlen + 4, flags);

			/* Tickles don't get acks */
			if (data->type == padTickle)
				break;

keepwaiting:
			if (next->read(ps, padp_buf,
					PI_PADP_HEADER_LEN + PI_PADP_MTU, flags) > 0) {
				unsigned char txid;
				
				padp.type =
				  get_byte(&padp_buf->data[PI_PADP_OFFSET_TYPE]);
				padp.flags =
				  get_byte(&padp_buf->data[PI_PADP_OFFSET_FLGS]);
				padp.size =
				  get_short(&padp_buf->data[PI_PADP_OFFSET_SIZE]);

				CHECK(PI_DBG_PADP, PI_DBG_LVL_INFO,
					padp_dump_header(padp_buf->data, 0));
				CHECK(PI_DBG_PADP, PI_DBG_LVL_DEBUG,
					padp_dump(padp_buf->data));

				size = sizeof(type);
				pi_getsockopt(ps->sd, PI_LEVEL_SLP, 
					      PI_SLP_LASTTYPE,
					      &type, &size);
				size = sizeof(txid);
				pi_getsockopt(ps->sd, PI_LEVEL_SLP,
					      PI_SLP_LASTTXID,
					      &txid, &size);

				if (type == PI_SLP_TYPE_PADP
					&& padp.type == (unsigned char)padData
					&& txid == data->txid
					&& len == tlen) {
					/* We didn't receive the ack for the
					 last packet, but the incomding data
					 is the response to this transmission.
					 The ack was lost.
					 */
					LOG((PI_DBG_PADP, PI_DBG_LVL_WARN,
					    "PADP TX Missing Ack\n"));
					count += tlen;
					goto done;
				} else if (padp.type == (unsigned char) 4) {
					/* Tickle to avoid timeout */
					goto keepwaiting;
				} else if (type == PI_SLP_TYPE_PADP
							&& padp.type == (unsigned char) padAck
							&& txid == data->txid) {
					if (padp.flags & MEMERROR) {
					/* OS 2.x enjoys sending erroneous 
					memory errors */
						LOG((PI_DBG_PADP,
						 PI_DBG_LVL_WARN,
						    "PADP TX Memory Error\n"));
						/* Mimimum failure:
						 transmission failed due to
						 lack of memory in reciever
						 link layer, but connection is
						 still active. This
						 transmission was lost, but
						 other transmissions will be
						 received. */
						errno = EMSGSIZE;
						count = -1;
						goto done;
					}

					/* Successful Ack */
					buf = buf + tlen;
					len -= tlen;
					count += tlen;
					fl = 0;
					break;
 				} else if (type == PI_SLP_TYPE_PADP
						   && padp.type  == data->last_ack_padp.type
						   && padp.flags == data->last_ack_padp.flags
						   && padp.size  == data->last_ack_padp.size
						   && txid       == data->last_ack_txid) {
					/* A repeat of a packet we already received.  The
					ack got lost, so resend it. */
 					LOG((PI_DBG_PADP, PI_DBG_LVL_WARN,
						 "PADP TX resending lost ack\n"));
					padp_sendack(ps, data, txid, &padp, flags);
 					continue;
				} else {
					LOG((PI_DBG_PADP, PI_DBG_LVL_ERR,
					    "PADP TX Unexpected packet "
					    "possible port speed problem? "
					    "out of sync packet?)\n"));

					padp_dump_header (buf, 1);
					/* Got unknown packet */
					errno = EIO;
					count = -1;
					goto done;
				}

			}
		} while (--retries > 0);

		if (retries == 0) {
			/* Maximum failure: transmission
			   failed, and the connection must be presumed dead */
			LOG((PI_DBG_PADP, PI_DBG_LVL_ERR,
				"PADP TX Timed out"));
			errno = ETIMEDOUT;
			ps->state = PI_SOCK_CONBK;
			pi_buffer_free (padp_buf);
			return pi_set_error(ps->sd, PI_ERR_SOCK_DISCONNECTED);
		}
	} while (len);

done:
	if (data->type != padAck && ps->state == PI_SOCK_CONIN)
		data->txid = data->next_txid;
	pi_buffer_free (padp_buf);
	return count;
}


/***********************************************************************
 *
 * Function:    padp_rx
 *
 * Summary:     Receive PADP packets
 *
 * Parameters:  pi_socket_t*, char* to buffer, expected length, flags
 *
 * Returns:     number of bytes received or negative on error
 *
 ***********************************************************************/
ssize_t
padp_rx(pi_socket_t *ps, pi_buffer_t *buf, size_t expect, int flags)
{
	int 	bytes,
		offset 		= 0,
		ouroffset 	= 0;

	size_t	total_bytes;
	pi_protocol_t *next, *prot;
	pi_padp_data_t *data;
	struct 	padp padp;
	pi_buffer_t *padp_buf;
	time_t endtime;

	prot = pi_protocol(ps->sd, PI_LEVEL_PADP);
	if (prot == NULL)
		return pi_set_error(ps->sd, PI_ERR_SOCK_INVALID);

	data = (pi_padp_data_t *)prot->data;
	next = pi_protocol_next(ps->sd, PI_LEVEL_PADP);
	if (next == NULL)
		return pi_set_error(ps->sd, PI_ERR_SOCK_INVALID);

	padp_buf = pi_buffer_new (PI_PADP_HEADER_LEN + PI_PADP_MTU);
	if (padp_buf == NULL) {
		errno = ENOMEM;
		return pi_set_error(ps->sd, PI_ERR_GENERIC_MEMORY);
	}

	if (ps->state == PI_SOCK_CONAC) {
		if (data->txid >= 0xfe)
			data->next_txid = 1;	/* wrap */
		else
			data->next_txid = data->txid + 1;
	} else {
		data->next_txid = data->txid;
	}
	
	endtime = time(NULL) + PI_PADP_RX_BLOCK_TO / 1000;

	for (;;) {
		int 	type,
			timeout;
		size_t	size;
		unsigned char txid;
		
		if (time(NULL) > endtime) {
			LOG((PI_DBG_PADP, PI_DBG_LVL_ERR,
				"PADP RX Timed out"));
			/* Bad timeout breaks connection */
			errno 		= ETIMEDOUT;
			ps->state 	= PI_SOCK_CONBK;
			pi_buffer_free (padp_buf);
			return pi_set_error(ps->sd, PI_ERR_SOCK_DISCONNECTED);
		}

		timeout = PI_PADP_RX_BLOCK_TO + 2000;
		size = sizeof(timeout);
		pi_setsockopt(ps->sd, PI_LEVEL_DEV, PI_DEV_TIMEOUT, 
			      &timeout, &size);

		total_bytes = 0;
		while (total_bytes < PI_PADP_HEADER_LEN) {
			bytes = next->read(ps, padp_buf, 
				(size_t)PI_PADP_HEADER_LEN + PI_PADP_MTU - total_bytes, flags);
			if (bytes < 0) {
				LOG((PI_DBG_PADP, PI_DBG_LVL_ERR,
					"PADP RX Read Error\n"));
				pi_buffer_free (padp_buf);
				return bytes;
			}
			total_bytes += bytes;
		}
		
		padp.type = get_byte(&padp_buf->data[PI_PADP_OFFSET_TYPE]);
		padp.flags = get_byte(&padp_buf->data[PI_PADP_OFFSET_FLGS]);
		padp.size = get_short(&padp_buf->data[PI_PADP_OFFSET_SIZE]);

		size = sizeof(type);
		pi_getsockopt(ps->sd, PI_LEVEL_SLP, 
			      PI_SLP_LASTTYPE,
			      &type, &size);
		size = sizeof(txid);
		pi_getsockopt(ps->sd, PI_LEVEL_SLP,
			      PI_SLP_LASTTXID,
			      &txid, &size);

		if (padp.flags & MEMERROR) {
			if (txid == data->txid) {
				LOG((PI_DBG_PADP, PI_DBG_LVL_WARN,
				    "PADP RX Memory Error\n"));
				errno = EMSGSIZE;
				ouroffset = -1;
				goto done;      /* Mimimum failure:
						 transmission failed due to
						 lack of memory in reciever
						 link layer, but connection is
						 still active. This
						 transmission was lost, but
						 other transmissions will be
						 received. */
			}
			continue;
		} else if (padp.type == (unsigned char) 4) {
			/* Tickle to avoid timeout */
			LOG((PI_DBG_PADP, PI_DBG_LVL_WARN,
			    "PADP RX Got Tickled\n"));
			endtime = time(NULL) + PI_PADP_RX_BLOCK_TO / 1000;
			continue;
		} else if ((type != PI_SLP_TYPE_PADP) || (padp.type != padData)
			   || (txid != data->txid)
			   || !(padp.flags & FIRST)) {
			LOG((PI_DBG_PADP, PI_DBG_LVL_ERR,
			    "PADP RX Wrong packet type on queue"
			    "(possible port speed problem? (loc1))\n"));
			continue;
		}
		break;
	}

	/* OK, we got the expected begin-of-data packet */
	endtime = time(NULL) + PI_PADP_RX_SEGMENT_TO / 1000;

	for (;;) {
		CHECK(PI_DBG_PADP, PI_DBG_LVL_INFO,
			 padp_dump_header(padp_buf->data, 0));
		CHECK(PI_DBG_PADP, PI_DBG_LVL_DEBUG,
			 padp_dump(padp_buf->data));

		/* Ack the packet */
		padp_sendack(ps, data, data->txid, &padp, flags);

		/* calculate length and offset - remove  */
		offset = ((padp.flags & FIRST) ? 0 : padp.size);
		total_bytes -= PI_PADP_HEADER_LEN;

		/* If packet was out of order, ignore it */

		if (offset == ouroffset) {
			if (pi_buffer_append (buf, &padp_buf->data[PI_PADP_HEADER_LEN], total_bytes) == NULL) {
				errno = ENOMEM;
				return pi_set_error(ps->sd, PI_ERR_GENERIC_MEMORY);
			}
			ouroffset += total_bytes;
		}

		if (padp.flags & LAST)
			break;

		endtime = time(NULL) + PI_PADP_RX_SEGMENT_TO / 1000;

		for (;;) {
			int type, timeout;
			size_t size;
			unsigned char txid;
			
			if (time(NULL) > endtime) {
				LOG((PI_DBG_PADP, PI_DBG_LVL_ERR,
					"PADP RX Segment Timeout"));

				/* Segment timeout, return error */
				errno = ETIMEDOUT;
				ouroffset = -1;
				/* Bad timeout breaks connection */
				ps->state = PI_SOCK_CONBK;
				pi_buffer_free (padp_buf);
				return pi_set_error(ps->sd, PI_ERR_SOCK_DISCONNECTED);
			}

			timeout = PI_PADP_RX_SEGMENT_TO + 2000;
			size = sizeof(timeout);
			pi_setsockopt(ps->sd, PI_LEVEL_DEV,
				 PI_DEV_TIMEOUT, &timeout, &size);

			total_bytes = 0;
			while (total_bytes < PI_PADP_HEADER_LEN) {
				padp_buf->used = total_bytes;
				bytes = next->read(ps, padp_buf, 
					PI_PADP_HEADER_LEN + PI_PADP_MTU - total_bytes,  flags);
				if (bytes < 0) {
					LOG((PI_DBG_PADP,
						PI_DBG_LVL_ERR, "PADP RX Read Error"));
					pi_buffer_free (padp_buf);
					return bytes;
				}
				total_bytes += bytes;
			}				

			padp.type =
				get_byte(&padp_buf->data[PI_PADP_OFFSET_TYPE]);
			padp.flags =
				get_byte(&padp_buf->data[PI_PADP_OFFSET_FLGS]);
			padp.size =
				get_short(&padp_buf->data[PI_PADP_OFFSET_SIZE]);

			CHECK(PI_DBG_PADP, PI_DBG_LVL_INFO,
				 padp_dump_header(padp_buf->data, 0));
			CHECK(PI_DBG_PADP, PI_DBG_LVL_DEBUG,
				 padp_dump(padp_buf->data));

			size = sizeof(type);
			pi_getsockopt(ps->sd, PI_LEVEL_SLP, 
					  PI_SLP_LASTTYPE,
					  &type, &size);
			size = sizeof(txid);
			pi_getsockopt(ps->sd, PI_LEVEL_SLP,
					  PI_SLP_LASTTXID,
					  &txid, &size);

			if (padp.flags & MEMERROR) {
				if (txid == data->txid) {
					LOG((PI_DBG_PADP,
					  PI_DBG_LVL_WARN,
					  "PADP RX Memory Error"));
					errno = EMSGSIZE;
					ouroffset = -1;
					goto done;
					/* Mimimum failure:
					 transmission failed due
					 to lack of memory in reciever
					 link layer, but connection is
					 still active. This
					 transmission was lost, but
					 other transmissions will be
					 received. */
				}
				continue;
			}
			
			if (padp.type == (unsigned char) 4) {
				/* Tickle to avoid timeout */

				endtime = time(NULL) +
					PI_PADP_RX_BLOCK_TO / 1000;
				LOG((PI_DBG_PADP, PI_DBG_LVL_WARN,
					"PADP RX Got Tickled"));
				continue;
			}

			if ((type != PI_SLP_TYPE_PADP)
				|| (padp.type != padData)
				|| (txid != data->txid)
				|| (padp.flags & FIRST)) {
					LOG((PI_DBG_PADP, PI_DBG_LVL_ERR,
				"PADP RX Wrong packet type on queue"
				"(possible port speed problem? (loc2))\n"));
				continue;
			}
			break;
		}
	}

done:
	data->txid = data->next_txid;

	pi_buffer_free (padp_buf);

	return ouroffset;
}

/***********************************************************************
 *
 * Function:    padp_flush
 *
 * Summary:     Flush input and output buffers
 *
 * Parameters:  pi_socket_t*, flags
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
static int
padp_flush(pi_socket_t *ps, int flags)
{
	pi_protocol_t	*prot,
			*next;

	prot = pi_protocol(ps->sd, PI_LEVEL_PADP);
	if (prot == NULL)
		return pi_set_error(ps->sd, PI_ERR_SOCK_INVALID);

	next = pi_protocol_next(ps->sd, PI_LEVEL_PADP);
	if (next == NULL)
		return pi_set_error(ps->sd, PI_ERR_SOCK_INVALID);

	return next->flush(ps, flags);
}

/***********************************************************************
 *
 * Function:    padp_getsockopt
 *
 * Summary:     get options on socket
 *
 * Parameters:  pi_socket*, level, option name, option value, option length
 *
 * Returns:     0 for success, negative otherwise
 *
 ***********************************************************************/
static int
padp_getsockopt(pi_socket_t *ps, int level, int option_name, 
	       void *option_value, size_t *option_len)
{
	pi_protocol_t *prot;
	pi_padp_data_t *data;

	prot = pi_protocol(ps->sd, PI_LEVEL_PADP);
	if (prot == NULL)
		return pi_set_error(ps->sd, PI_ERR_SOCK_INVALID);
	data = (pi_padp_data_t *)prot->data;

	switch (option_name) {
		case PI_PADP_TYPE:
			if (*option_len < sizeof (data->type))
				goto error;
			memcpy (option_value, &data->type,
					sizeof (data->type));
			*option_len = sizeof (data->type);
			break;
		case PI_PADP_LASTTYPE:
			if (*option_len < sizeof (data->last_type))
				goto error;
			memcpy (option_value, &data->last_type,
					sizeof (data->last_type));
			*option_len = sizeof (data->last_type);
			break;
	}

	return 0;
	
 error:
	errno = EINVAL;
	return pi_set_error(ps->sd, PI_ERR_GENERIC_ARGUMENT);
}


/***********************************************************************
 *
 * Function:    padp_setsockopt
 *
 * Summary:     get options on socket
 *
 * Parameters:  pi_socket*, level, option name, option value, option length
 *
 * Returns:     0 for success, negative otherwise
 *
 ***********************************************************************/
static int
padp_setsockopt(pi_socket_t *ps, int level, int option_name, 
		const void *option_value, size_t *option_len)
{
	pi_protocol_t *prot;
	pi_padp_data_t *data;

	prot = pi_protocol(ps->sd, PI_LEVEL_PADP);
	if (prot == NULL)
		return pi_set_error(ps->sd, PI_ERR_SOCK_INVALID);
	data = (pi_padp_data_t *)prot->data;

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
	return pi_set_error(ps->sd, PI_ERR_GENERIC_ARGUMENT);
}

/***********************************************************************
*
* Function:    padp_sendack
*
* Summary:     Acknowledge receipt of a packet
*
* Parameters:  
*
* Returns:     
*
***********************************************************************/
static int
padp_sendack(struct pi_socket *ps,
		struct pi_padp_data *data, /* padp state, will be modified */
		unsigned char txid,        /* txid of the packet being acked */
		struct padp *padp,         /* padp header of the packet being acked */
		int flags)
{
	int type, socket;
	size_t size;
	unsigned char npadp_buf[PI_PADP_HEADER_LEN];
	struct padp npadp;
	int result;
	struct pi_protocol *next;
	
	next = pi_protocol_next(ps->sd, PI_LEVEL_PADP);
	if (next == NULL)
 	    return pi_set_error(ps->sd, PI_ERR_SOCK_INVALID);

	type 	= 2;
	socket 	= PI_SLP_SOCK_DLP;
	size 	= sizeof(type);
	pi_setsockopt(ps->sd, PI_LEVEL_SLP, PI_SLP_TYPE, 
				  &type, &size);
	pi_setsockopt(ps->sd, PI_LEVEL_SLP, PI_SLP_DEST, 
				  &socket, &size);
	pi_setsockopt(ps->sd, PI_LEVEL_SLP, PI_SLP_SRC, 
				  &socket, &size);
	size = sizeof(txid);
	pi_setsockopt(ps->sd, PI_LEVEL_SLP, PI_SLP_TXID, 
				  &txid, &size);
	
	npadp.type 	= padAck;
	npadp.flags	= padp->flags;
	npadp.size 	= padp->size;
	
	set_byte(&npadp_buf[PI_PADP_OFFSET_TYPE], npadp.type);
	set_byte(&npadp_buf[PI_PADP_OFFSET_FLGS], npadp.flags);
	set_short(&npadp_buf[PI_PADP_OFFSET_SIZE], npadp.size);
	
	CHECK(PI_DBG_PADP, PI_DBG_LVL_INFO, padp_dump_header(npadp_buf, 1));
	CHECK(PI_DBG_PADP, PI_DBG_LVL_DEBUG, padp_dump(npadp_buf));
	
	result = next->write(ps, npadp_buf, PI_PADP_HEADER_LEN, flags);
	
	if (result >= 0) {
		data->last_ack_txid = txid;
		data->last_ack_padp.type  = padp->type;
		data->last_ack_padp.flags = padp->flags;
		data->last_ack_padp.size  = padp->size;
	}
	
	return result;
}

/***********************************************************************
 *
 * Function:    padp_dump_header
 *
 * Summary:     Dump PADP packet header
 *
 * Parameters:  char* to data, RXTX boolean
 *
 * Returns:     void
 *
 ***********************************************************************/
void
padp_dump_header(unsigned char *data, int rxtx)
{
	int 	s;
	char 	*stype;
	unsigned char type, flags;

	type = get_byte (&data[PI_PADP_OFFSET_TYPE]);
	switch (type) {
	case padData:
		stype = "DATA";
		break;
	case padAck:
		stype = "ACK";
		break;
	case padTickle:
		stype = "TICKLE";
		break;
	case padWake:
		stype = "WAKE";
		break;
	case padAbort:
		stype = "ABORT";
		break;
	default:
		stype = "UNK";
		break;
	}

	flags = get_byte(&data[PI_PADP_OFFSET_FLGS]);
	s = get_short(&data[PI_PADP_OFFSET_SIZE]);

	LOG((PI_DBG_PADP, PI_DBG_LVL_NONE, 
	    "PADP %s %c%c%c type=%s len=0x%.4x\n", 
	    rxtx ? "TX" : "RX",
	    (flags & FIRST) ? 'F' : ' ',
	    (flags & LAST) ? 'L' : ' ',
	    (flags & MEMERROR) ? 'M' : ' ',
	    stype, s));
}


/***********************************************************************
 *
 * Function:    padp_dump
 *
 * Summary:     Dump PADP packets 
 *
 * Parameters:  char* to data
 *
 * Returns:     void
 *
 ***********************************************************************/
void
padp_dump(unsigned char *data)
{
	size_t 	size;
	unsigned char type;

	type 	= get_byte (&data[PI_PADP_OFFSET_TYPE]);
	size 	= get_short(&data[PI_PADP_OFFSET_SIZE]);

	if (size > PI_PADP_MTU)
		size = PI_PADP_MTU;
	if (type != padAck)
		dumpdata((char *)&data[PI_PADP_HEADER_LEN], size);
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
