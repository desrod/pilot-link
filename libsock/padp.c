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

#include <stdio.h>
#include <errno.h>

#include "pi-debug.h"
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-padp.h"
#include "pi-slp.h"
#include "pi-serial.h"

#define xmitTimeout 2*1000
#define xmitRetries 10

static int padp_getsockopt(struct pi_socket *ps, int level, int option_name, 
			   void *option_value, int *option_len);
static int padp_setsockopt(struct pi_socket *ps, int level, int option_name, 
			   const void *option_value, int *option_len);

static struct pi_protocol *padp_protocol_dup (struct pi_protocol *prot)
{
	struct pi_protocol *new_prot;
	struct pi_padp_data *data, *new_data;
	
	new_prot = (struct pi_protocol *)malloc (sizeof (struct pi_protocol));
	new_prot->level = prot->level;
	new_prot->dup = prot->dup;
	new_prot->read = prot->read;
	new_prot->write = prot->write;
	new_prot->getsockopt = prot->getsockopt;
	new_prot->setsockopt = prot->setsockopt;

	new_data = (struct pi_padp_data *)malloc (sizeof (struct pi_padp_data));
	data = (struct pi_padp_data *)prot->data;
	new_data->type = data->type;
	new_data->last_type = data->last_type;
	new_data->txid = data->txid;
	new_data->next_txid = data->next_txid;
	new_prot->data = new_data;

	return new_prot;
}

struct pi_protocol *padp_protocol (void)
{
	struct pi_protocol *prot;
	struct pi_padp_data *data;

	prot = (struct pi_protocol *)malloc (sizeof (struct pi_protocol));	
	prot->level = PI_LEVEL_PADP;
	prot->dup = padp_protocol_dup;
	prot->read = padp_rx;
	prot->write = padp_tx;
	prot->getsockopt = padp_getsockopt;
	prot->setsockopt = padp_setsockopt;

	data = (struct pi_padp_data *)malloc (sizeof (struct pi_padp_data));
	data->type = padData;
	data->last_type = -1;
	data->txid = 0xff;
	data->next_txid = -1;
	prot->data = data;
	
	return prot;
}


/***********************************************************************
 *
 * Function:    padp_tx
 *
 * Summary:     Transmit PADP packets
 *
 * Parmeters:   None
 *
 * Returns:     Number of packets transmitted
 *
 ***********************************************************************/
int padp_tx(struct pi_socket *ps, unsigned char *buf, int len)
{
	struct pi_protocol *prot, *next;
	struct pi_padp_data *data;
	int flags = FIRST;
	int tlen;
	int count = 0;
	struct padp padp;
	int retries;

	if (ps->broken)		/* Don't use an unavailable connection */
		return -1;

	prot = pi_protocol(ps->sd, PI_LEVEL_PADP);
	if (prot == NULL)
		return -1;
	data = (struct pi_padp_data *)prot->data;
	next = pi_protocol_next(ps->sd, PI_LEVEL_PADP);
	if (next == NULL)
		return -1;

	if (data->type == padWake) {
		data->txid = (unsigned char) 0xff;
	}

	if (data->txid == (unsigned char) 0)
		data->txid = (unsigned char) 0x10;	/* some random # */

	if (data->txid >= (unsigned char) 0xfe)
		data->next_txid = (unsigned char) 1;	/* wrap */
	else
		data->next_txid = data->txid + (unsigned char) 1;

	if ((data->type != padAck) && !ps->initiator)
		data->txid = data->next_txid;

	Begin(padp_tx);

	do {

		retries = xmitRetries;
		do {
			unsigned char padp_buf[PI_PADP_MTU];
			int type, socket, size;

			type = PI_SLP_TYPE_PADP;
			socket = PI_PilotSocketDLP;
			size = sizeof(type);
			pi_setsockopt(ps->sd, PI_LEVEL_SLP, PI_SLP_TYPE, 
				      &type, &size);
			pi_setsockopt(ps->sd, PI_LEVEL_SLP, PI_SLP_DEST, 
				      &socket, &size);
			pi_setsockopt(ps->sd, PI_LEVEL_SLP, PI_SLP_SRC, 
				      &socket, &size);
			size = sizeof(data->txid);
			pi_setsockopt(ps->sd, PI_LEVEL_SLP, PI_SLP_TXID, 
				      &data->txid, &size);

			tlen = (len > PI_PADP_MTU - PI_PADP_HEADER_LEN) 
				? PI_PADP_MTU - PI_PADP_HEADER_LEN : len;

			padp.type = data->type & 0xff;
			padp.flags = flags | (len == tlen ? LAST : 0);
			padp.size = (flags ? len : count);

			/* Construct the packet */
			set_byte(&padp_buf[PI_PADP_OFFSET_TYPE], padp.type);
			set_byte(&padp_buf[PI_PADP_OFFSET_FLGS], padp.flags);
			set_short(&padp_buf[PI_PADP_OFFSET_SIZE], padp.size);
			memcpy(padp_buf + PI_PADP_HEADER_LEN, buf, tlen);

			CHECK(PI_DBG_PADP, PI_DBG_LVL_INFO, padp_dump_header(padp_buf, 1));
			CHECK(PI_DBG_PADP, PI_DBG_LVL_DEBUG, padp_dump(padp_buf));
			
			next->write(ps, padp_buf, tlen + 4);

			if (data->type == padTickle)	/* Tickles don't get acks */
				break;

		      keepwaiting:
			At("Reading Ack");
			
			if (next->read(ps, padp_buf, PI_SLP_MTU) > 0) {
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
				
				if (padp.flags & MEMERROR) {
					if (txid == data->txid) {
						/* OS 2.x enjoys sending erroneous memory errors */

						fprintf(stderr,
							"Out of memory\n");
						errno = EMSGSIZE;
						count = -1;
						goto done;
						return -1;	/* Mimimum failure: transmission failed due to lack of
								   memory in reciever link layer, but connection is still
								   active. This transmission was lost, but other
								   transmissions will be received. */
					} else
						goto keepwaiting;
				} else if ((type == PI_SLP_TYPE_PADP)
					   && (padp.type == (unsigned char) padData)
					   && (txid == data->txid)
					   && (len == 0)) {
					fprintf(stderr, "Missing ack\n");
					/* Incoming padData from response to
					   this transmission.  Maybe the Ack
					   was lost */
					/* Don't consume packet, and return success. */
					count = 0;
					goto done;
					return 0;
				} else if (padp.type == (unsigned char) 4) {
					/* Tickle to avoid timeout */

					goto keepwaiting;
				} else if ((type == PI_SLP_TYPE_PADP)
					   && (padp.type == (unsigned char) padAck)
					   && (txid == data->txid)) {
					/* Got correct Ack */
					flags = (unsigned char) padp.flags;

					/* Successful Ack */
					buf = ((char *) buf) + tlen;
					len -= tlen;
					count += tlen;
					flags = 0;
					break;
				} else {
					fprintf(stderr, "\n   Weird packet (port speed problem?)\n");
					/* Got unknown packet */
					/* Don't consume packet */
					errno = EIO;
					count = -1;
					goto done;
					return -1;	/* Unknown failure: received unknown packet */
				}
			}
		} while (--retries > 0);

		if (retries == 0) {
			errno = ETIMEDOUT;
			ps->broken = -1;
			return -1;	/* Maximum failure: transmission
					   failed, and the connection must
					   be presumed dead */
		}

	} while (len);

      done:
	if ((data->type != padAck) && ps->initiator)
		data->txid = data->next_txid;

	End(padp_tx);

	return count;
}

#define recStartTimeout 30*1000
#define recSegTimeout 30*1000

/***********************************************************************
 *
 * Function:    padp_rx
 *
 * Summary:     Receive PADP packets
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int padp_rx(struct pi_socket *ps, unsigned char *buf, int len)
{
	struct pi_protocol *next, *prot;
	struct pi_padp_data *data;
	struct padp padp, npadp;
	int data_len;
	int offset = 0;
	int ouroffset = 0;
	time_t endtime;
	unsigned char padp_buf[PI_PADP_MTU];

	endtime = time(NULL) + recStartTimeout / 1000;

	if (ps->broken)		/* Don't use a broken connection */
		return -1;

	prot = pi_protocol(ps->sd, PI_LEVEL_PADP);
	if (prot == NULL)
		return -1;
	data = (struct pi_padp_data *)prot->data;
	next = pi_protocol_next(ps->sd, PI_LEVEL_PADP);
	if (next == NULL)
		return -1;

	if (!ps->initiator) {
		if (data->txid >= 0xfe)
			data->next_txid = 1;	/* wrap */
		else
			data->next_txid = data->txid + 1;
	} else {
		data->next_txid = data->txid;
	}
	
	Begin(padp_rx);

	for (;;) {
		int type, size;
		unsigned char txid;
		
		if (time(NULL) > endtime) {
			/* Start timeout, return error */
			errno = ETIMEDOUT;
			ouroffset = -1;
			ps->broken = -1;	/* Bad timeout breaks connection */
			goto done;
			return -1;
		}

		data_len = next->read(ps, padp_buf, PI_PADP_MTU);
		
		padp.type = get_byte(&padp_buf[PI_PADP_OFFSET_TYPE]);
		padp.flags = get_byte(&padp_buf[PI_PADP_OFFSET_FLGS]);
		padp.size = get_short(&padp_buf[PI_PADP_OFFSET_SIZE]);

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
				fprintf(stderr, "Out of memory\n");
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

			endtime = time(NULL) + recStartTimeout / 1000;
			fprintf(stderr, "Got tickled\n");

			continue;
		} else if ((type != PI_SLP_TYPE_PADP) || (padp.type != padData)
			   || (txid != data->txid)
			   || !(padp.flags & FIRST)) {
			if (padp.type == padTickle) {
				endtime =
				    time(NULL) + recStartTimeout / 1000;
				fprintf(stderr, "Got tickled\n");
			}
			fprintf(stderr, "\n   Wrong packet type on queue (port speed problem?)\n");
			continue;
		}
		break;
	}

	/* OK, we got the expected begin-of-data packet */
	endtime = time(NULL) + recSegTimeout / 1000;

	for (;;) {
		int type, socket, size;
		unsigned char npadp_buf[PI_PADP_HEADER_LEN];
		At(got data);

		CHECK(PI_DBG_PADP, PI_DBG_LVL_INFO, padp_dump_header(padp_buf, 0));
		CHECK(PI_DBG_PADP, PI_DBG_LVL_DEBUG, padp_dump(padp_buf));

		/* Ack the packet */
		type = 2;
		socket = PI_PilotSocketDLP;
		size = sizeof(type);
		pi_setsockopt(ps->sd, PI_LEVEL_SLP, PI_SLP_TYPE, 
			      &type, &size);
		pi_setsockopt(ps->sd, PI_LEVEL_SLP, PI_SLP_DEST, 
			      &socket, &size);
		pi_setsockopt(ps->sd, PI_LEVEL_SLP, PI_SLP_SRC, 
			      &socket, &size);
		size = sizeof(data->txid);
		pi_setsockopt(ps->sd, PI_LEVEL_SLP, PI_SLP_TXID, 
			      &data->txid, &size);
		
		npadp.type = padAck;
		npadp.flags = padp.flags;
		npadp.size = padp.size;

		set_byte(&npadp_buf[PI_PADP_OFFSET_TYPE], npadp.type);
		set_byte(&npadp_buf[PI_PADP_OFFSET_FLGS], npadp.flags);
		set_short(&npadp_buf[PI_PADP_OFFSET_SIZE], npadp.size);

		CHECK(PI_DBG_PADP, PI_DBG_LVL_INFO, padp_dump_header(npadp_buf, 1));
		CHECK(PI_DBG_PADP, PI_DBG_LVL_DEBUG, padp_dump(npadp_buf));

		next->write(ps, npadp_buf, PI_PADP_HEADER_LEN);
		At(sent Ack);

		/* calculate length and offset - remove  */
		offset = ((padp.flags & FIRST) ? 0 : padp.size);
		data_len -= PI_PADP_HEADER_LEN;

		/* If packet was out of order, ignore it */

		if (offset == ouroffset) {
			At(storing block);
			memcpy((unsigned char *) buf + ouroffset,
			       &padp_buf[PI_PADP_HEADER_LEN], data_len);

			ouroffset += data_len;
		}

		if (padp.flags & LAST) {
			break;
		} else {
			endtime = time(NULL) + recSegTimeout / 1000;

			for (;;) {
				int type, size;
				unsigned char txid;
				
				if (time(NULL) > endtime) {
					fprintf(stderr,
						"segment timeout\n");
					/* Segment timeout, return error */
					errno = ETIMEDOUT;
					ouroffset = -1;
					ps->broken = -1;	/* Bad timeout breaks connection */
					goto done;
					return -1;
				}

				data_len = next->read(ps, padp_buf, PI_SLP_MTU);

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
						fprintf(stderr,
							"Out of memory\n");
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

					endtime =
					    time(NULL) +
					    recStartTimeout / 1000;
					fprintf(stderr, "Got tickled\n");

					continue;
				} else
				    if ((type != PI_SLP_TYPE_PADP)
					|| (padp.type != padData)
					|| (txid != data->txid)
					|| (padp.flags & FIRST)) {
					if (padp.type == padTickle) {
						endtime =
						    time(NULL) +
						    recSegTimeout / 1000;
						fprintf(stderr,
							"Got tickled\n");
					}
					fprintf(stderr,
						"Wrong packet type on queue\n");
					continue;
				}
				At(got good packet);
				break;
			}
		}
	}

      done:
	data->txid = data->next_txid;

	End(padp_rx);

	return ouroffset;
}

static int
padp_getsockopt(struct pi_socket *ps, int level, int option_name, 
	       void *option_value, int *option_len)
{
	struct pi_protocol *prot;
	struct pi_padp_data *data;

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

void padp_dump_header(unsigned char *data, int rxtx)
{
	int s;
	unsigned char type, flags;
	char *stype;

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
		stype = "LOOP";
		break;
	}

	flags = get_byte(&data[PI_PADP_OFFSET_FLGS]);
	s = get_short(&data[PI_PADP_OFFSET_SIZE]);

	LOG(PI_DBG_PADP, PI_DBG_LVL_NONE, 
	    "PADP %s %c%c%c type=%s len=0x%.4x\n", 
	    rxtx ? "TX" : "RX",
	    (flags & FIRST) ? 'F' : ' ',
	    (flags & LAST) ? 'L' : ' ',
	    (flags & MEMERROR) ? 'M' : ' ',
	    stype, s);
}

/***********************************************************************
 *
 * Function:    padp_dump
 *
 * Summary:     Dump PADP packets 
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void padp_dump(unsigned char *data)
{
	int s;
	unsigned char type;

	type = get_byte (&data[PI_PADP_OFFSET_TYPE]);
	s = get_short(&data[PI_PADP_OFFSET_SIZE]);

	if (s > PI_PADP_MTU - PI_PADP_HEADER_LEN)
		s = PI_PADP_MTU - PI_PADP_HEADER_LEN;
	if (type != padAck)
		dumpdata(PI_DBG_PADP, &data[PI_PADP_HEADER_LEN], s);
}
