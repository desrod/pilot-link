/*
 * contact.c:  Translate Palm contact data formats
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
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "pi-macros.h"
#include "pi-contact.h"

/***********************************************************************
 *
 * Function:    free_Contact
 *
 * Summary:	Free the members of an contact structure
 *
 * Parameters:  struct Contact *c
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void free_Contact(struct Contact *c)
{
	int 	i;

	for (i = 0; i < NUM_CONTACT_ENTRIES; i++)
		if (c->entry[i])
			free(c->entry[i]);
	if (c->picture != NULL)
		pi_buffer_free (c->picture);
}

#define hi(x) (((x) >> 4) & 0x0f)
#define lo(x) ((x) & 0x0f)
#define pair(x,y) (((x) << 4) | (y))

/***********************************************************************
 *
 * Function:    unpack_Contact
 *
 * Summary:     Fill in the contact structure based on the raw record 
 *		data
 *
 * Parameters:  struct Contact *c, unsigned char *buffer, int len
 *
 * Returns:     0 on error, the length of the data used from the
 *		buffer otherwise
 *
 ***********************************************************************/
int unpack_Contact(struct Contact *c, unsigned char *buffer, int len)
{
	unsigned long contents;
	unsigned long v;
	unsigned char *start = buffer;
	int i, max_bit, field_num;
	unsigned int packed_date;
	if (len < 17)
		return 0;

	c->showPhone     = hi(get_byte(buffer));
	c->phoneLabel[6] = lo(get_byte(buffer));
	c->phoneLabel[5] = hi(get_byte(buffer + 1));
	c->phoneLabel[4] = lo(get_byte(buffer + 1));
	c->phoneLabel[3] = hi(get_byte(buffer + 2));
	c->phoneLabel[2] = lo(get_byte(buffer + 2));
	c->phoneLabel[1] = hi(get_byte(buffer + 3));
	c->phoneLabel[0] = lo(get_byte(buffer + 3));

	c->addressLabel[2] = lo(get_byte(buffer + 4));
	c->addressLabel[1] = hi(get_byte(buffer + 5));
	c->addressLabel[0] = lo(get_byte(buffer + 5));

	c->IMLabel[1] = hi(get_byte(buffer + 7));
	c->IMLabel[0] = lo(get_byte(buffer + 7));

	contents = get_long(start + 8);

	/* We calculate the new offset in pack_Contact(),
           and we don't need to store this anywhere */
	/* c->companyOffset = get_byte(start + 16); */

	buffer 	+= 17;
	len 	-= 17;

	field_num=0;
	for (i=0; i<2; i++) {
		if (i==0) {
			max_bit=28;
		} else {
			max_bit=11;
			contents = get_long(start + 12);
		}
		for (v = 0; v < max_bit; v++, field_num++) {
			if (contents & (1 << v)) {
				if (len < 1)
					return 0;
				c->entry[field_num] = strdup((char *) buffer);
				buffer += strlen((char *) buffer) + 1;
				len -= strlen(c->entry[field_num]) + 1;
			} else {
				c->entry[field_num] = 0;
			}
		}
	}

	/* Both of these are set if the birthday field is set.  Previously
	 * this comment suspected one may be an alarm, but I see no evidence
	 * of that.  --KB
	 */
	if ((contents & 0x0800) || (contents & 0x1000)) {
                c->birthdayFlag = 1;
		if (len < 1)
			return 0;
		packed_date = get_short(buffer);
		c->birthday.tm_year = ((packed_date & 0xFE00) >> 9) + 4;
		c->birthday.tm_mon  = ((packed_date & 0x01E0) >> 5) - 1;
		c->birthday.tm_mday = (packed_date & 0x001F);
		c->birthday.tm_hour = 0;
		c->birthday.tm_min  = 0;
		c->birthday.tm_sec  = 0;
		c->birthday.tm_isdst= -1;
		mktime(&c->birthday);
		/* 1 byte containing a zero (padding) */
		len -= 3;
		buffer += 3;
	} else {
		c->birthdayFlag = 0;
	}

	if (contents & 0x2000) {
		c->reminderFlag = 1;
		if (len < 2)
			return 0;
		c->advanceUnits = get_byte(buffer);
		c->advance = get_byte(buffer+1);
		len -= 2;
		buffer += 2;
	} else {
		c->reminderFlag = 0;
	}

	if (len > (size_t)(buffer - start)) {
		if ((c->picture = pi_buffer_new (len)) == NULL)
			return -1;

		pi_buffer_append (c->picture, buffer, len);
	} else
		c->picture = NULL;

	return (buffer - start);
}

/***********************************************************************
 *
 * Function:    pack_Contact
 *
 * Summary:     Fill in the raw contact record data based on the 
 *		contact structure
 *
 * Parameters:  struct Contact *c, unsigned char *record, int len
 *
 * Returns:     The length of the buffer required if record is NULL,
 *		or 0 on error, the length of the data used from the 
 *		buffer otherwise
 *
 ***********************************************************************/
int pack_Contact(struct Contact *c, unsigned char *record, int len)
{
	int 	l,
		destlen = 17;

	unsigned char *start = record;
	unsigned char *buffer;
	unsigned long contents1, contents2;
	unsigned long v;
	unsigned int  field_i;
	unsigned long phoneflag;
	unsigned long typesflag;
	unsigned short packed_date;
	int companyOffset = 0;

	for (v = 0; v < NUM_CONTACT_ENTRIES; v++) {
		if (c->entry[v]) {
			destlen += (strlen(c->entry[v]) + 1);
		}
	}
	if (c->birthdayFlag)
		destlen += 3;
	if (c->reminderFlag)
		destlen += 2;
	if (c->picture != NULL) {
		destlen += c->picture->used;
	}

	if (!record)
		return destlen;
	if (len < destlen)
		return 0;

	buffer = record + 17;
	phoneflag = 0;
	contents1 = contents2 = 0;

	field_i = 0;
	for (v = 0; v < 28; v++, field_i++) {
		if (c->entry[field_i] && strlen(c->entry[field_i])) {
			contents1 |= (1 << v);
			l = strlen(c->entry[field_i]) + 1;
			memcpy(buffer, c->entry[field_i], l);
			buffer += l;
		}
	}
	for (v = 0; v < 11; v++, field_i++) {
		if (c->entry[field_i] && strlen(c->entry[field_i])) {
			contents2 |= (1 << v);
			l = strlen(c->entry[field_i]) + 1;
			memcpy(buffer, c->entry[field_i], l);
			buffer += l;
		}
	}

	if (c->birthdayFlag) {
		contents2 |= 0x1800;
		packed_date = (((c->birthday.tm_year - 4) << 9) & 0xFE00) |
                        (((c->birthday.tm_mon+1) << 5) & 0x01E0) |
			(c->birthday.tm_mday & 0x001F);
		set_short(buffer, packed_date);
		buffer += 2;
		set_byte(buffer, 0);
		buffer += 1;
	}
	if (c->reminderFlag) {
		contents2 |= 0x2000;
		set_byte(buffer, c->advanceUnits);
		buffer += 1;
		set_byte(buffer, c->advance);
		buffer += 1;
	}
	if (c->picture != NULL) {
		memcpy (buffer, c->picture->data, c->picture->used);
		buffer += c->picture->used;
	}

	phoneflag  = (((unsigned long) c->phoneLabel[0]) & 0xF) << 0;
	phoneflag |= (((unsigned long) c->phoneLabel[1]) & 0xF) << 4;
	phoneflag |= (((unsigned long) c->phoneLabel[2]) & 0xF) << 8;
	phoneflag |= (((unsigned long) c->phoneLabel[3]) & 0xF) << 12;
	phoneflag |= (((unsigned long) c->phoneLabel[4]) & 0xF) << 16;
	phoneflag |= (((unsigned long) c->phoneLabel[5]) & 0xF) << 20;
	phoneflag |= (((unsigned long) c->phoneLabel[6]) & 0xF) << 24;
	phoneflag |= (((unsigned long) c->showPhone) & 0xF) << 28;

	typesflag   = (((unsigned long) c->IMLabel[0]) & 0xF) << 0;
	typesflag  |= (((unsigned long) c->IMLabel[1]) & 0xF) << 4;
	typesflag  |= (((unsigned long) c->addressLabel[0]) & 0xF) << 16;
	typesflag  |= (((unsigned long) c->addressLabel[1]) & 0xF) << 20;
	typesflag  |= (((unsigned long) c->addressLabel[2]) & 0xF) << 24;

	set_long(record, phoneflag);
	set_long(record + 4, typesflag);
	set_long(record + 8, contents1);
	set_long(record + 12, contents2);
	/* companyOffset is the offset from itself to the company field,
	 * or zero if no company field.  Its not useful to us at all.
	 */
	if (c->entry[2]) {
               companyOffset++;
               if (c->entry[0]) companyOffset += strlen(c->entry[0]) + 1;
               if (c->entry[1]) companyOffset += strlen(c->entry[1]) + 1;
	}
	set_byte(record + 16, companyOffset);

	return (buffer - start);
}

/***********************************************************************
 *
 * Function:    unpack_ContactAppInfo
 *
 * Summary:     Fill in the app info structure based on the raw app 
 *				info data
 *
 * Parameters:  struct ContactAppInfo *ai, unsigned char *record, int len
 *
 * Returns:     0 on success
 *				-1 on error
 *
 ***********************************************************************/
int
unpack_ContactAppInfo (struct ContactAppInfo *ai, pi_buffer_t *buf)
{
	int				i,
					j,
					destlen;
	ptrdiff_t		ofs = 0;

	if (buf == NULL || buf->data == NULL || ai == NULL)
		return -1;

	switch (buf->used) {
		case 1092:
			ai->type = contacts_v10;
			ai->numLabels = 49;
			break;
		case 1156:
			ai->type = contacts_v11;
			ai->numLabels = 53;
			break;
		default:
			/* Unknown version */
			return -1;
	}
	destlen = 278						/* categories */
			+ 26						/* internal */
			+ ai->numLabels * 16		/* a bunch of strings */
			+ 2							/* country */
			+ 2;						/* sorting */

	if (buf->used < destlen)
		return -1;

	i = unpack_CategoryAppInfo(&ai->category, buf->data, buf->used);
	if (!i)
		return -1;
	ofs += i;

	memcpy(ai->internal, buf->data + ofs, 26);
	ofs += 26;
	memcpy(ai->labels, buf->data + ofs, 16 * ai->numLabels);
	ofs += 16 * ai->numLabels;
	ai->country = get_byte(buf->data + ofs);
	ofs += 2;
	ai->sortByCompany = get_byte(buf->data + ofs);
	ofs += 2;

	if (ofs != buf->used)
		/* Should never happen */
		return -1;

	/* These are the fields that go in drop down menus */
	for (i = 4, j = 0; i < 11; i++, j++) {
		strcpy(ai->phoneLabels[j], ai->labels[i]);
	}
	strcpy(ai->phoneLabels[j], ai->labels[40]);

	strcpy(ai->addrLabels[0], ai->labels[23]);
	strcpy(ai->addrLabels[1], ai->labels[28]);
	strcpy(ai->addrLabels[2], ai->labels[33]);

	strcpy(ai->IMLabels[0], ai->labels[41]);
	strcpy(ai->IMLabels[1], ai->labels[42]);
	strcpy(ai->IMLabels[2], ai->labels[43]);
	strcpy(ai->IMLabels[3], ai->labels[44]);
	strcpy(ai->IMLabels[4], ai->labels[45]);

	strcpy(ai->customLabels[0], ai->labels[14]);
	strcpy(ai->customLabels[1], ai->labels[15]);
	strcpy(ai->customLabels[2], ai->labels[16]);
	strcpy(ai->customLabels[3], ai->labels[17]);
	strcpy(ai->customLabels[4], ai->labels[18]);
	strcpy(ai->customLabels[5], ai->labels[19]);
	strcpy(ai->customLabels[6], ai->labels[20]);
	strcpy(ai->customLabels[7], ai->labels[21]);
	strcpy(ai->customLabels[8], ai->labels[22]);

	return 0;
}

/***********************************************************************
 *
 * Function:    pack_ContactAppInfo
 *
 * Summary:     Fill in the raw app info record data based on the
 *		ContactAppInfo structure
 *
 * Parameters:  struct ContactAppInfo *ai, unsigned char *record, int len
 *
 * Returns:     The length of the buffer required if record is NULL,
 *		or 0 on error, the length of the data used from the
 *		buffer otherwise
 *
 ***********************************************************************/
int
pack_ContactAppInfo(struct ContactAppInfo *ai, pi_buffer_t *buf)
{
	int				destlen;


	if (buf == NULL)
		return -1;

	destlen = 278						/* categories */
			+ 26						/* internal */
			+ ai->numLabels * 16		/* a bunch of strings */
			+ 2							/* country */
			+ 2;						/* sorting */

	pi_buffer_expect (buf, destlen);

	buf->used = pack_CategoryAppInfo(&ai->category, buf->data, buf->allocated);

	if (buf->used != 278)
		return -1;

	pi_buffer_append (buf, ai->internal, 26);

	pi_buffer_append (buf, ai->labels, 16 * ai->numLabels);

	set_byte (buf->data + buf->used++, ai->country);
	set_byte (buf->data + buf->used++, 0);
	set_byte (buf->data + buf->used++, ai->sortByCompany);
	set_byte (buf->data + buf->used++, 0);

	return 0;
}

/* vi: set ts=4 sw=4 sts=4 noexpandtab: cin */
