/*
 * slp.c:  Pilot SLP protocol
 *
 * (c) 1996, D. Jeff Dionne.
 * Much of this code adapted from Brian J. Swetland <swetland@uiuc.edu>
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

#ifdef WIN32
#include <winsock.h>
#endif
#include <stdio.h>

#include "pi-debug.h"
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-serial.h"
#include "pi-slp.h"

static int slp_getsockopt(struct pi_socket *ps, int level, int option_name, 
			  void *option_value, int *option_len);
static int slp_setsockopt(struct pi_socket *ps, int level, int option_name, 
			  const void *option_value, int *option_len);

static struct pi_protocol *slp_protocol_dup (struct pi_protocol *prot)
{
	struct pi_protocol *new_prot;
	struct pi_slp_data *data, *new_data;
	
	new_prot = (struct pi_protocol *)malloc (sizeof (struct pi_protocol));
	new_prot->level = prot->level;
	new_prot->dup = prot->dup;
	new_prot->read = prot->read;
	new_prot->write = prot->write;
	new_prot->getsockopt = prot->getsockopt;
	new_prot->setsockopt = prot->setsockopt;

	new_data = (struct pi_slp_data *)malloc (sizeof (struct pi_slp_data));
	data = (struct pi_slp_data *)prot->data;
	new_data->dest = data->dest;
	new_data->last_dest = data->last_dest;	
	new_data->src = data->src;
	new_data->last_src = data->last_src;
	new_data->type = data->type;
	new_data->last_type = data->last_type;
	new_data->txid = data->txid;
	new_data->last_txid = data->last_txid;
	new_prot->data = new_data;

	return new_prot;
}

struct pi_protocol *slp_protocol (void)
{
	struct pi_protocol *prot;
	struct pi_slp_data *data;

	prot = (struct pi_protocol *)malloc (sizeof (struct pi_protocol));
	prot->level = PI_LEVEL_SLP;
	prot->dup = slp_protocol_dup;
	prot->read = slp_rx;
	prot->write = slp_tx;
	prot->getsockopt = slp_getsockopt;
	prot->setsockopt = slp_setsockopt;

	data = (struct pi_slp_data *)malloc (sizeof (struct pi_slp_data));
	data->dest = PI_PilotSocketDLP;
	data->last_dest = -1;	
	data->src = PI_PilotSocketDLP;
	data->last_src = -1;
	data->type = PI_SLP_TYPE_PADP;
	data->last_type = -1;
	data->txid = 0xfe;
	data->last_txid = -1;
	prot->data = data;
	
	return prot;
}

/***********************************************************************
 *
 * Function:    slp_tx
 *
 * Summary:     Build and queue up an SLP packet to be transmitted
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int slp_tx(struct pi_socket *ps, unsigned char *buf, int len)
{
	struct pi_protocol *prot, *next;
	struct pi_slp_data *data;
	struct slp *slp;
	unsigned char slp_buf[PI_SLP_MTU];
	unsigned int i;
	unsigned int n;
	int bytes;

	prot = pi_protocol(ps->sd, PI_LEVEL_SLP);
	if (prot == NULL)
		return -1;
	data = (struct pi_slp_data *)prot->data;
	next = pi_protocol_next(ps->sd, PI_LEVEL_SLP);
	if (next == NULL)
		return -1;
	
	slp = (struct slp *) slp_buf;

	/* Header values */
	slp->_be 	= 0xbe;
	slp->_ef 	= 0xef;
	slp->_ed 	= 0xed;
	slp->dest 	= data->dest;
	slp->src 	= data->src;
	slp->type 	= data->type;
	slp->dlen 	= htons(len);
	slp->id 	= data->txid;

	for (n = i = 0; i < 9; i++)
		n += slp_buf[i];
	slp->csum = 0xff & n;

	/* Copy in the packet data */
	memcpy (slp_buf + PI_SLP_HEADER_LEN, buf, len);

	/* CRC value */
	set_short(&slp_buf[PI_SLP_HEADER_LEN + len], crc16(slp_buf, PI_SLP_HEADER_LEN + len));

	/* Write out the data */
	bytes = next->write(ps, slp_buf, PI_SLP_HEADER_LEN + len + PI_SLP_FOOTER_LEN);

	CHECK(PI_DBG_SLP, PI_DBG_LVL_INFO, slp_dump_header(slp_buf, 1));
	CHECK(PI_DBG_SLP, PI_DBG_LVL_DEBUG, slp_dump(slp_buf));

	return bytes;
}

/* Sigh.  SLP is a really broken protocol.  It has no proper framing, so it
   makes a proper "device driver" layer impossible.  There ought to be a
   layer below SLP that reads frames off the wire and passes them up. 
   Insted, all we can do is have the device driver give us bytes and SLP has
   to keep a pile of status info while it builds frames for itself.  So
   here's the code that does that. */

/***********************************************************************
 *
 * Function:    slp_rx
 *
 * Summary:     Accept SLP packets on the wire
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int slp_rx(struct pi_socket *ps, unsigned char *buf, int len)
{
	struct pi_protocol *prot, *next;
	struct pi_slp_data *data;
	unsigned char slp_buf[PI_SLP_MTU];
	int i, checksum, b1, b2, b3;
	int state, expect, packet_len, bytes, total_bytes;
	unsigned char *cur;

	prot = pi_protocol(ps->sd, PI_LEVEL_SLP);
	if (prot == NULL)
		return -1;
	data = (struct pi_slp_data *)prot->data;
	next = pi_protocol_next(ps->sd, PI_LEVEL_SLP);
	if (next == NULL)
		return -1;

	state = 0;
	packet_len = 0;
	total_bytes = 0;
	cur = slp_buf;
	for (;;) {
		switch (state) {		
		case 0:
			expect = 3;
			state++;
			break;

		case 1:
			b1 = (0xff & (int)slp_buf[PI_SLP_OFFSET_SIG1]);
			b2 = (0xff & (int)slp_buf[PI_SLP_OFFSET_SIG2]);
			b3 = (0xff & (int)slp_buf[PI_SLP_OFFSET_SIG3]);
			if (b1 == PI_SLP_SIG_BYTE1
			    && b2 == PI_SLP_SIG_BYTE2
			    && b3 == PI_SLP_SIG_BYTE3) {
				state++;
				expect = PI_SLP_HEADER_LEN - 3;
			} else {
				slp_buf[PI_SLP_OFFSET_SIG1] = slp_buf[PI_SLP_OFFSET_SIG2];
				slp_buf[PI_SLP_OFFSET_SIG2] = slp_buf[PI_SLP_OFFSET_SIG3];
				expect = 1;
				cur--;
				LOG(PI_DBG_SLP, PI_DBG_LVL_WARN,
				    "SLP RX Unexpected signature 0x%.2x 0x%.2x 0x%.2x\n",
				    b1, b2, b3);
			}
			break;

		case 2:
			/* Addition check sum for header */
			for (checksum = i = 0; i < 9; i++)
				checksum += slp_buf[i];

			/* read in the whole SLP header. */
			if ((checksum & 0xff) == slp_buf[PI_SLP_OFFSET_SUM]) {
				state++;
				packet_len = get_short(&slp_buf[PI_SLP_OFFSET_SIZE]);
				expect = packet_len;
			} else {
				LOG(PI_DBG_SLP, PI_DBG_LVL_WARN, "SLP RX Header Checksum failed\n");
				return -1;
			}
			break;
		case 3:
			state++;
			expect = PI_SLP_FOOTER_LEN;
			break;
		case 4:
			/* that should be the whole packet. */
			checksum = crc16(slp_buf, PI_SLP_HEADER_LEN + packet_len);
			if (checksum != get_short(&slp_buf[PI_SLP_HEADER_LEN + packet_len])) {
				LOG(PI_DBG_SLP, PI_DBG_LVL_ERR,
				    "CRC16 check failed: computed=0x%.4x received=0x%.4x\n", 
				    checksum, get_short(&slp_buf[PI_SLP_HEADER_LEN + packet_len]));
				return -1;
			}
			
			/* FIXME: Handle LOOP packets */

			/* Track the info so getsockopt will work */
			data->last_dest = get_byte(&slp_buf[PI_SLP_OFFSET_DEST]);
			data->last_src = get_byte(&slp_buf[PI_SLP_OFFSET_SRC]);
			data->last_type = get_byte(&slp_buf[PI_SLP_OFFSET_TYPE]);
			data->last_txid = get_byte(&slp_buf[PI_SLP_OFFSET_TXID]);

			CHECK(PI_DBG_SLP, PI_DBG_LVL_INFO, slp_dump_header(slp_buf, 0));
			CHECK(PI_DBG_SLP, PI_DBG_LVL_DEBUG, slp_dump(slp_buf));

			memcpy(buf, &slp_buf[PI_SLP_HEADER_LEN], packet_len);
			goto done;
			break;

		default:
			break;
		}

		if (total_bytes + expect > len + PI_SLP_HEADER_LEN + PI_SLP_FOOTER_LEN)
			return -1;
		do {
			bytes = next->read(ps, cur, expect);
			total_bytes += bytes;
			expect -= bytes;
			cur += bytes;
		} while (expect > 0);
	}

 done:

	return packet_len;
}

static int
slp_getsockopt(struct pi_socket *ps, int level, int option_name, 
	       void *option_value, int *option_len)
{
	struct pi_protocol *prot;
	struct pi_slp_data *data;

	prot = pi_protocol(ps->sd, PI_LEVEL_SLP);
	if (prot == NULL)
		return -1;
	data = (struct pi_slp_data *)prot->data;

	switch (option_name) {
	case PI_SLP_DEST:
		if (*option_len < sizeof (data->dest))
			goto error;
		memcpy (option_value, &data->dest, sizeof (data->dest));
		*option_len = sizeof (data->dest);
		break;
	case PI_SLP_LASTDEST:
		if (*option_len < sizeof (data->dest))
			goto error;
		memcpy (option_value, &data->last_dest, sizeof (data->last_dest));
		*option_len = sizeof (data->last_dest);
		break;
	case PI_SLP_SRC:
		if (*option_len < sizeof (data->src))
			goto error;
		memcpy (option_value, &data->src, 
			sizeof (data->src));
		*option_len = sizeof (data->src);
		break;
	case PI_SLP_LASTSRC:
		if (*option_len < sizeof (data->last_src))
			goto error;
		memcpy (option_value, &data->last_src, 
			sizeof (data->last_src));
		*option_len = sizeof (data->last_src);
		break;
	case PI_SLP_TYPE:
		if (*option_len < sizeof (data->type))
			goto error;
		memcpy (option_value, &data->type,
			sizeof (data->type));
		*option_len = sizeof (data->type);
		break;
	case PI_SLP_LASTTYPE:
		if (*option_len < sizeof (data->last_type))
			goto error;
		memcpy (option_value, &data->last_type,
			sizeof (data->last_type));
		*option_len = sizeof (data->last_type);
		break;
	case PI_SLP_TXID:
		if (*option_len < sizeof (data->txid))
			goto error;
		memcpy (option_value, &data->txid,
			sizeof (data->txid));
		*option_len = sizeof (data->txid);
		break;
	case PI_SLP_LASTTXID:
		if (*option_len < sizeof (data->last_txid))
			goto error;
		memcpy (option_value, &data->last_txid,
			sizeof (data->last_txid));
		*option_len = sizeof (data->last_txid);
		break;
	}

	return 0;
	
 error:
	errno = EINVAL;
	return -1;
}

static int
slp_setsockopt(struct pi_socket *ps, int level, int option_name, 
	       const void *option_value, int *option_len)
{
	struct pi_protocol *prot;
	struct pi_slp_data *data;

	prot = pi_protocol(ps->sd, PI_LEVEL_SLP);
	if (prot == NULL)
		return -1;
	data = (struct pi_slp_data *)prot->data;

	switch (option_name) {
	case PI_SLP_DEST:
		if (*option_len != sizeof (data->dest))
			goto error;
		memcpy (&data->dest, option_value,
			sizeof (data->dest));
		*option_len = sizeof (data->dest);
		break;
	case PI_SLP_SRC:
		if (*option_len != sizeof (data->src))
			goto error;
		memcpy (&data->src, option_value,
			sizeof (data->src));
		*option_len = sizeof (data->src);
		break;
	case PI_SLP_TYPE:
		if (*option_len != sizeof (data->type))
			goto error;
		memcpy (&data->type, option_value,
			sizeof (data->type));
		*option_len = sizeof (data->type);
		break;
	case PI_SLP_TXID:
		if (*option_len != sizeof (data->txid))
			goto error;
		memcpy (&data->txid, option_value,
			sizeof (data->txid));
		*option_len = sizeof (data->txid);
		break;
	}

	return 0;
	
 error:
	errno = EINVAL;
	return -1;
}

/***********************************************************************
 *
 * Function:    slp_dump
 *
 * Summary:     Dump the contents of the SPL frame
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void slp_dump_header(unsigned char *data, int rxtx)
{	
	LOG(PI_DBG_SLP, PI_DBG_LVL_NONE,
	    "SLP %s %d->%d type=%d txid=0x%.2x len=0x%.4x\n",
	    rxtx ? "TX" : "RX",
	    get_byte(&data[PI_SLP_OFFSET_DEST]),
	    get_byte(&data[PI_SLP_OFFSET_SRC]),
	    get_byte(&data[PI_SLP_OFFSET_TYPE]),
	    get_byte(&data[PI_SLP_OFFSET_TXID]),
	    get_short(&data[PI_SLP_OFFSET_SIZE]));
}

void slp_dump(unsigned char *data)
{
	int s;

	s = get_short(&data[PI_SLP_OFFSET_SIZE]);
	dumpdata(PI_DBG_SLP, &data[PI_SLP_HEADER_LEN], s);
}
