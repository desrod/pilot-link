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
#include "pi-socket.h"
#include "pi-padp.h"
#include "pi-slp.h"

#define PI_PADP_TX_TIMEOUT 2*1000
#define PI_PADP_TX_RETRIES 10
#define PI_PADP_RX_BLOCK_TO 30*1000
#define PI_PADP_RX_SEGMENT_TO 30*1000

static int padp_getsockopt(struct pi_socket *ps, int level, int option_name, 
			   void *option_value, int *option_len);
static int padp_setsockopt(struct pi_socket *ps, int level, int option_name, 
			   const void *option_value, int *option_len);

static struct pi_protocol *padp_protocol_dup (struct pi_protocol *prot)
{
	struct 	pi_protocol *new_prot;
	struct 	pi_padp_data *data, *new_data;
	
	new_prot 		= (struct pi_protocol *)malloc (sizeof (struct pi_protocol));
	new_prot->level 	= prot->level;
	new_prot->dup 		= prot->dup;
	new_prot->free 		= prot->free;
	new_prot->read 		= prot->read;
	new_prot->write 	= prot->write;
	new_prot->getsockopt 	= prot->getsockopt;
	new_prot->setsockopt 	= prot->setsockopt;

	new_data 		= (struct pi_padp_data *)malloc (sizeof (struct pi_padp_data));
	data 			= (struct pi_padp_data *)prot->data;
	new_data->type 		= data->type;
	new_data->last_type 	= data->last_type;
	new_data->txid 		= data->txid;
	new_data->next_txid 	= data->next_txid;
	new_prot->data 		= new_data;

	return new_prot;
}

static void padp_protocol_free (struct pi_protocol *prot)
{
	free(prot->data);
	free(prot);
}

struct pi_protocol *padp_protocol (void)
{
	struct 	pi_protocol *prot;
	struct 	pi_padp_data *data;

	prot 			= (struct pi_protocol *)malloc (sizeof (struct pi_protocol));	
	prot->level 		= PI_LEVEL_PADP;
	prot->dup 		= padp_protocol_dup;
	prot->free 		= padp_protocol_free;
	prot->read 		= padp_rx;
	prot->write 		= padp_tx;
	prot->getsockopt 	= padp_getsockopt;
	prot->setsockopt 	= padp_setsockopt;

	data 			= (struct pi_padp_data *)malloc (sizeof (struct pi_padp_data));
	data->type 		= padData;
	data->last_type 	= -1;
	data->txid 		= 0xff;
	data->next_txid 	= -1;
	prot->data 		= data;
	
	return prot;
}

/***********************************************************************
 *
 * Function:    padp_tx
 *
 * Summary:     Transmit PADP packets
 *
 * Parameters:  None
 *
 * Returns:     Number of packets transmitted
 *
 ***********************************************************************/
int padp_tx(struct pi_socket *ps, unsigned char *buf, int len, int flags)
{
	int 	fl 	= FIRST,
		tlen,
		count 	= 0,
		retries;
	
	struct 	pi_protocol *prot, *next;
	struct 	pi_padp_data *data;

	struct padp padp;


	prot = pi_protocol(ps->sd, PI_LEVEL_PADP);
	if (prot == NULL)
		return -1;
	data = (struct pi_padp_data *)prot->data;
	next = pi_protocol_next(ps->sd, PI_LEVEL_PADP);
	if (next == NULL)
		return -1;

	if (data->type == padWake) {
		data->txid = 0xff;
	}

	if (data->txid == 0)
		data->txid = 0x10;	/* some random # */

	if (data->txid >= 0xfe)
		data->next_txid = 1;	/* wrap */
	else
		data->next_txid = data->txid + 1;

	if ((data->type != padAck) && ps->state == PI_SOCK_CONAC)
		data->txid = data->next_txid;

	do {

		retries = PI_PADP_TX_RETRIES;
		do {
			unsigned char padp_buf[PI_PADP_HEADER_LEN + PI_PADP_MTU];
			int 	type,
				socket,
				timeout,
				size;

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
			padp.size 	= (fl ? len : count);

			/* Construct the packet */
			set_byte(&padp_buf[PI_PADP_OFFSET_TYPE], padp.type);
			set_byte(&padp_buf[PI_PADP_OFFSET_FLGS], padp.flags);
			set_short(&padp_buf[PI_PADP_OFFSET_SIZE], padp.size);
			memcpy(padp_buf + PI_PADP_HEADER_LEN, buf, tlen);

			CHECK(PI_DBG_PADP, PI_DBG_LVL_INFO, padp_dump_header(padp_buf, 1));
			CHECK(PI_DBG_PADP, PI_DBG_LVL_DEBUG, padp_dump(padp_buf));
			
			next->write(ps, padp_buf, tlen + 4, flags);

			if (data->type == padTickle)	/* Tickles don't get acks */
				break;

		keepwaiting:

			if (next->read(ps, padp_buf, PI_PADP_HEADER_LEN + PI_PADP_MTU, flags) > 0) {
				int type, size;
				unsigned char txid;
				
				padp.type =
					get_byte(&padp_buf[PI_PADP_OFFSET_TYPE]);
				padp.flags =
				    get_byte(&padp_buf[PI_PADP_OFFSET_FLGS]);
				padp.size =
				    get_short(&padp_buf[PI_PADP_OFFSET_SIZE]);


				CHECK(PI_DBG_PADP, PI_DBG_LVL_INFO, padp_dump_header(padp_buf, 0));
				CHECK(PI_DBG_PADP, PI_DBG_LVL_DEBUG, padp_dump(padp_buf));

				/* FIXME: error checking */
				size = sizeof(type);
				pi_getsockopt(ps->sd, PI_LEVEL_SLP, 
					      PI_SLP_LASTTYPE,
					      &type, &size);
				size = sizeof(txid);
				pi_getsockopt(ps->sd, PI_LEVEL_SLP,
					      PI_SLP_LASTTXID,
					      &txid, &size);

				if ((type == PI_SLP_TYPE_PADP)
					   && (padp.type == (unsigned char) padData)
					   && (txid == data->txid)
					   && (len == tlen)) {

					/* We didn't receive the ack for the last packet,
					   but the incomding data is the response to
					   this transmission.  The ack was lost.
					 */
					LOG((PI_DBG_PADP, PI_DBG_LVL_WARN,
					    "PADP TX Missing Ack\n"));
					count += tlen;
					goto done;
				} else if (padp.type == (unsigned char) 4) {
					/* Tickle to avoid timeout */

					goto keepwaiting;
				} else if ((type == PI_SLP_TYPE_PADP)
					   && (padp.type == (unsigned char) padAck)
					   && (txid == data->txid)) {
					if (padp.flags & MEMERROR) {
						/* OS 2.x enjoys sending erroneous memory errors */
						LOG((PI_DBG_PADP, PI_DBG_LVL_WARN,
						    "PADP TX Memory Error\n"));
						/* Mimimum failure: transmission failed due to lack of
						   memory in reciever link layer, but connection is still
						   active. This transmission was lost, but other
						   transmissions will be received. */
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
				} else {
					LOG((PI_DBG_PADP, PI_DBG_LVL_ERR,
					    "PADP TX Unexpected packet "
					    "possible port speed problem? "
					    "out of sync packet?)\n"));

					fprintf(stderr, "PADP TX Unexpected packet, "
						"possible port speed problem?\n\n");

					padp_dump_header (buf, 1);
					/* Got unknown packet */
					errno = EIO;
					count = -1;
					goto done;
				}

			}
		} while (--retries > 0);

		if (retries == 0) {
			LOG((PI_DBG_PADP, PI_DBG_LVL_ERR, "PADP TX Timed out"));
			errno = ETIMEDOUT;
			ps->state = PI_SOCK_CONBK;
			return -1;	/* Maximum failure: transmission
					   failed, and the connection must
					   be presumed dead */		}

	} while (len);

      done:
	if ((data->type != padAck) && ps->state == PI_SOCK_CONIN)
		data->txid = data->next_txid;

	return count;
}

/***********************************************************************
 *
 * Function:    padp_rx
 *
 * Summary:     Receive PADP packets
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int padp_rx(struct pi_socket *ps, unsigned char *buf, int len, int flags)
{
	int 	bytes,
		total_bytes,
		offset 		= 0,
		ouroffset 	= 0;
	struct 	pi_protocol *next, *prot;
	struct 	pi_padp_data *data;
	struct 	padp padp, npadp;

	time_t endtime;
	unsigned char padp_buf[PI_PADP_HEADER_LEN + PI_PADP_MTU];

	endtime = time(NULL) + PI_PADP_RX_BLOCK_TO / 1000;

	prot = pi_protocol(ps->sd, PI_LEVEL_PADP);
	if (prot == NULL)
		return -1;
	data = (struct pi_padp_data *)prot->data;
	next = pi_protocol_next(ps->sd, PI_LEVEL_PADP);
	if (next == NULL)
		return -1;

	if (ps->state == PI_SOCK_CONAC) {
		if (data->txid >= 0xfe)
			data->next_txid = 1;	/* wrap */
		else
			data->next_txid = data->txid + 1;
	} else {
		data->next_txid = data->txid;
	}
	
	for (;;) {
		int 	type,
			timeout,
			size;
		unsigned char txid;
		
		if (time(NULL) > endtime) {
			LOG((PI_DBG_PADP, PI_DBG_LVL_ERR, "PADP RX Timed out"));
			/* Bad timeout breaks connection */
			errno 		= ETIMEDOUT;
			ps->state 	= PI_SOCK_CONBK;	
			return -1;
		}

		timeout = PI_PADP_RX_BLOCK_TO + 2000;
		size = sizeof(timeout);
		pi_setsockopt(ps->sd, PI_LEVEL_DEV, PI_DEV_TIMEOUT, 
			      &timeout, &size);

		total_bytes = 0;
		while (total_bytes < PI_PADP_HEADER_LEN) {
			bytes = next->read(ps, padp_buf + total_bytes, 
					   PI_PADP_HEADER_LEN + PI_PADP_MTU - total_bytes, flags);
			if (bytes < 0) {
				LOG((PI_DBG_PADP, PI_DBG_LVL_ERR, "PADP RX Read Error\n"));
				return -1;
			}
			total_bytes += bytes;
		}
		
		padp.type 	= get_byte(&padp_buf[PI_PADP_OFFSET_TYPE]);
		padp.flags 	= get_byte(&padp_buf[PI_PADP_OFFSET_FLGS]);
		padp.size 	= get_short(&padp_buf[PI_PADP_OFFSET_SIZE]);

		size = sizeof(type);
		/* FIXME: error checking */
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
				goto done;
				return -1;	/* Mimimum failure: transmission failed due to lack of
						   memory in reciever link layer, but connection is still
						   active. This transmission was lost, but other
						   transmissions will be received. */
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
			    "(possible port speed problem?)\n"));
			continue;
		}
		break;
	}

	/* OK, we got the expected begin-of-data packet */
	endtime = time(NULL) + PI_PADP_RX_SEGMENT_TO / 1000;

	for (;;) {
		int type, socket, size;
		unsigned char npadp_buf[PI_PADP_HEADER_LEN];

		CHECK(PI_DBG_PADP, PI_DBG_LVL_INFO, padp_dump_header(padp_buf, 0));
		CHECK(PI_DBG_PADP, PI_DBG_LVL_DEBUG, padp_dump(padp_buf));

		/* Ack the packet */
		type 	= 2;
		socket 	= PI_SLP_SOCK_DLP;
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
		
		npadp.type 	= padAck;
		npadp.flags 	= padp.flags;
		npadp.size 	= padp.size;

		set_byte(&npadp_buf[PI_PADP_OFFSET_TYPE], npadp.type);
		set_byte(&npadp_buf[PI_PADP_OFFSET_FLGS], npadp.flags);
		set_short(&npadp_buf[PI_PADP_OFFSET_SIZE], npadp.size);

		CHECK(PI_DBG_PADP, PI_DBG_LVL_INFO, padp_dump_header(npadp_buf, 1));
		CHECK(PI_DBG_PADP, PI_DBG_LVL_DEBUG, padp_dump(npadp_buf));

		next->write(ps, npadp_buf, PI_PADP_HEADER_LEN, flags);

		/* calculate length and offset - remove  */
		offset = ((padp.flags & FIRST) ? 0 : padp.size);
		total_bytes -= PI_PADP_HEADER_LEN;

		/* If packet was out of order, ignore it */

		if (offset == ouroffset) {
			memcpy((unsigned char *) buf + ouroffset,
			       &padp_buf[PI_PADP_HEADER_LEN], total_bytes);

			ouroffset += total_bytes;
		}

		if (padp.flags & LAST) {
			break;
		} else {
			endtime = time(NULL) + PI_PADP_RX_SEGMENT_TO / 1000;

			for (;;) {
				int type, timeout, size;
				unsigned char txid;
				
				if (time(NULL) > endtime) {
					LOG((PI_DBG_PADP, PI_DBG_LVL_ERR,
					    "PADP RX Segment Timeout"));

					/* Segment timeout, return error */
					errno = ETIMEDOUT;
					ouroffset = -1;
					ps->state = PI_SOCK_CONBK;	/* Bad timeout breaks connection */
					return -1;
				}

				timeout = PI_PADP_RX_SEGMENT_TO + 2000;
				size = sizeof(timeout);
				pi_setsockopt(ps->sd, PI_LEVEL_DEV, PI_DEV_TIMEOUT, 
					      &timeout, &size);

				total_bytes = 0;
				while (total_bytes < PI_PADP_HEADER_LEN) {
					bytes = next->read(ps, padp_buf + total_bytes, 
							   PI_PADP_HEADER_LEN + PI_PADP_MTU - total_bytes, 
							   flags);
					if (bytes < 0) {
						LOG((PI_DBG_PADP, PI_DBG_LVL_ERR, "PADP RX Read Error"));
						return -1;
					}
					total_bytes += bytes;
				}				

				padp.type =
				    get_byte(&padp_buf[PI_PADP_OFFSET_TYPE]);
				padp.flags =
				    get_byte(&padp_buf[PI_PADP_OFFSET_FLGS]);
				padp.size =
				    get_short(&padp_buf[PI_PADP_OFFSET_SIZE]);

				CHECK(PI_DBG_PADP, PI_DBG_LVL_INFO, padp_dump_header(padp_buf, 0));
				CHECK(PI_DBG_PADP, PI_DBG_LVL_DEBUG, padp_dump(padp_buf));

				size = sizeof(type);
				/* FIXME: error checking */
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
						    "PADP RX Memory Error"));
						errno = EMSGSIZE;
						ouroffset = -1;
						goto done;
						return -1;	/* Mimimum failure: transmission failed due to lack of
								   memory in reciever link layer, but connection is still
								   active. This transmission was lost, but other
								   transmissions will be received. */
					} else
						continue;
				} else if (padp.type == (unsigned char) 4) {
					/* Tickle to avoid timeout */

					endtime = time(NULL) +
						PI_PADP_RX_BLOCK_TO / 1000;
					LOG((PI_DBG_PADP, PI_DBG_LVL_WARN,
					    "PADP RX Got Tickled"));
					continue;
				} else
				    if ((type != PI_SLP_TYPE_PADP)
					|| (padp.type != padData)
					|| (txid != data->txid)
					|| (padp.flags & FIRST)) {
					    LOG((PI_DBG_PADP, PI_DBG_LVL_ERR,
						"PADP RX Wrong packet type on queue"
						"(possible port speed problem?)\n"));
					continue;
				}
				break;
			}
		}
	}

      done:
	data->txid = data->next_txid;

	return ouroffset;
}

static int
padp_getsockopt(struct pi_socket *ps, int level, int option_name, 
	       void *option_value, int *option_len)
{
	struct 	pi_protocol *prot;
	struct 	pi_padp_data *data;

	prot = pi_protocol(ps->sd, PI_LEVEL_PADP);
	if (prot == NULL)
		return -1;
	data = (struct pi_padp_data *)prot->data;

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
	return -1;
}

static int
padp_setsockopt(struct pi_socket *ps, int level, int option_name, 
		const void *option_value, int *option_len)
{
	struct 	pi_protocol *prot;
	struct 	pi_padp_data *data;

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

void padp_dump_header(unsigned char *data, int rxtx)
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
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void padp_dump(unsigned char *data)
{
	int 	size;
	unsigned char type;

	type 	= get_byte (&data[PI_PADP_OFFSET_TYPE]);
	size 	= get_short(&data[PI_PADP_OFFSET_SIZE]);

	if (size > PI_PADP_MTU)
		size = PI_PADP_MTU;
	if (type != padAck)
		dumpdata(&data[PI_PADP_HEADER_LEN], size);
}
