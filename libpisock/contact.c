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

	c->IMLabel[1] = hi(get_byte(buffer + 7)) - 1; /* First (AIM) = 1, not 0 on HH. */
	c->IMLabel[0] = lo(get_byte(buffer + 7)) - 1;

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

	/* I think one of these is a birthday flag and one is an alarm flag.
	 * Since both are always set there is no way to know which is which.
	 * It could be something like a flag for advanceUnits also.
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
		if (c->birthdayFlag) destlen += 3;
		if (c->reminderFlag) destlen += 2;
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

	phoneflag  = (((unsigned long) c->phoneLabel[0]) & 0xF) << 0;
	phoneflag |= (((unsigned long) c->phoneLabel[1]) & 0xF) << 4;
	phoneflag |= (((unsigned long) c->phoneLabel[2]) & 0xF) << 8;
	phoneflag |= (((unsigned long) c->phoneLabel[3]) & 0xF) << 12;
	phoneflag |= (((unsigned long) c->phoneLabel[4]) & 0xF) << 16;
	phoneflag |= (((unsigned long) c->phoneLabel[5]) & 0xF) << 20;
	phoneflag |= (((unsigned long) c->phoneLabel[6]) & 0xF) << 24;
	phoneflag |= (((unsigned long) c->showPhone) & 0xF) << 28;

	typesflag   = (((unsigned long) c->IMLabel[0]+1) & 0xF) << 0; /* First = 1, not 0 on HH. */
	typesflag  |= (((unsigned long) c->IMLabel[1]+1) & 0xF) << 4;
	typesflag  |= (((unsigned long) c->addressLabel[0]) & 0xF) << 16;
	typesflag  |= (((unsigned long) c->addressLabel[1]) & 0xF) << 20;
	typesflag  |= (((unsigned long) c->addressLabel[2]) & 0xF) << 24;

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
 *		info data
 *
 * Parameters:  struct ContactAppInfo *ai, unsigned char *record, int len
 *
 * Returns:     The necessary length of the buffer if record is NULL,
 *		or 0 on error, the length of the data used from the 
 *		buffer otherwise
 *
 ***********************************************************************/
int
unpack_ContactAppInfo(struct ContactAppInfo *ai, unsigned char *record,
		      int len)
{
	int 	i, j,
		destlen = 4 + 16 * 48 + 2 + 2;

	unsigned char *start = record;
	/* unsigned long r; */

	i = unpack_CategoryAppInfo(&ai->category, record, len);
	if (!record)
		return i + destlen;
	if (!i)
		return i;
	record += i;
	len -= i;

	if (len < destlen)
		return 0;

/*	r = get_long(record);
	for (i = 0; i < 22; i++)
		ai->labelRenamed[i] = !!(r & (1 << i));
	record += 4;
*/
	memcpy(ai->unknown1, record, 26);
	record += 26;
	memcpy(ai->labels, record, 16 * 49);
	record += 16 * 49;
	ai->country = get_byte(record);
	record += 2;
	ai->sortByCompany = get_byte(record);
	record += 2;

	/* These are the fields that go in drop down menus */
	for (i = 4, j = 0; i < 11; i++, j++) {
		strcpy(ai->phoneLabels[j], ai->labels[i]);
	}
	strcpy(ai->phoneLabels[j], ai->labels[40]);

	strcpy(ai->addrLabels[0], ai->labels[23]);
	strcpy(ai->addrLabels[1], ai->labels[28]);
	strcpy(ai->addrLabels[2], ai->labels[33]);

	strcpy(ai->IMLabels[0], ai->labels[42]);
	strcpy(ai->IMLabels[1], ai->labels[43]);
	strcpy(ai->IMLabels[2], ai->labels[44]);
	strcpy(ai->IMLabels[3], ai->labels[45]);
	strcpy(ai->IMLabels[4], ai->labels[41]);

	return (record - start);
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
pack_ContactAppInfo(struct ContactAppInfo *ai, unsigned char *record,
		    int len)
{
	int 	i,
		destlen = 4 + 16 * 48 + 2 + 2;
	unsigned char *pos = record;

	i = pack_CategoryAppInfo(&ai->category, record, len);
	if (!record)
		return destlen + i;
	if (!i)
		return i;

	pos += i;
	len -= i;

	memcpy(pos, ai->unknown1, 26);
	pos += 26;

	memcpy(pos, ai->labels, 16 * 49);
	pos += 16 * 49;

	set_byte(pos++, ai->country);
	set_byte(pos++, 0);

	set_byte(pos++, ai->sortByCompany);
	set_byte(pos++, 0);

	/* r = 0;
	for (i = 0; i < 22; i++)
		if (ai->labelRenamed[i])
			r |= (1 << i);
	set_long(pos, r);
	pos += 4;*/

	return (pos - record);
}
