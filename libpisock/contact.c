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
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "pi-macros.h"
#include "pi-contact.h"
#include "pi-debug.h"

#define hi(x) (((x) >> 4) & 0x0f)
#define lo(x) ((x) & 0x0f)
#define pair(x,y) (((x) << 4) | (y))

/***********************************************************************
 *
 * Function:	free_Contact
 *
 * Summary:		Frees the allocated contents of a Contact.  Call this
 *				when the structure's data is no longer needed.
 *
 * Parameters:	*c, the Contact to be disposed of
 *
 * Returns:		Nothing
 *
 ***********************************************************************/
void
free_Contact (struct Contact *c)
{
	int				i;

	for (i = 0; i < NUM_CONTACT_ENTRIES; i++)
		if (c->entry[i])
			free(c->entry[i]);

	if (c->picture != NULL)
		pi_buffer_free (c->picture);
}


/***********************************************************************
 *
 * Function:	unpack_Contact
 *
 * Summary:		Unpacks a raw contact data into a common structure for
 *				all known version of palmOne Contacts
 *
 * Parameters:	*c, a Contact structure to fill in
 *				*buf, a pi_buffer_t containing (only) the raw record
 *				type, the type field from ContactAppInfo
 *
 * Returns:     0 on error, the length of the data used from the
 *		buffer otherwise
 *
 ***********************************************************************/
int
unpack_Contact (struct Contact *c, pi_buffer_t *buf, contactsType type)
{
	int				i;
	uint32_t		contents1,
					contents2;
	size_t			ofs;
	uint16_t		packed_date;

	if (c == NULL)
		return -1;

	/* just in case... */
	for (i = 0; i < NUM_CONTACT_ENTRIES; i++)
		c->entry[i] = NULL;
	c->picture = NULL;

	if (buf == NULL || buf->data == NULL || buf->used < 17)
		return -1;

	if (type != contacts_v10
			&& type != contacts_v11)
		/* Don't support anything else yet */
		return -1;

	c->showPhone     = hi (get_byte (buf->data));
	c->phoneLabel[6] = lo (get_byte (buf->data));
	c->phoneLabel[5] = hi (get_byte (buf->data + 1));
	c->phoneLabel[4] = lo (get_byte (buf->data + 1));
	c->phoneLabel[3] = hi (get_byte (buf->data + 2));
	c->phoneLabel[2] = lo (get_byte (buf->data + 2));
	c->phoneLabel[1] = hi (get_byte (buf->data + 3));
	c->phoneLabel[0] = lo (get_byte (buf->data + 3));

	/* hi(get_byte (buf->data + 4)) unused */
	c->addressLabel[2] = lo (get_byte(buf->data + 4));
	c->addressLabel[1] = hi (get_byte(buf->data + 5));
	c->addressLabel[0] = lo (get_byte(buf->data + 5));

	/* get_byte (buf->data + 6) unused */
	c->IMLabel[1] = hi(get_byte(buf->data + 7));
	c->IMLabel[0] = lo(get_byte(buf->data + 7));

	contents1 = get_long(buf->data + 8);
	contents2 = get_long(buf->data + 12);

	/* get_byte (buf->data + 16) is an offset to the Company field */

	ofs = 17;

	for (i = 0; i < 28; i++) {
		if ((contents1 & (1 << i)) != 0) {
			if (ofs <= buf->used)
				c->entry[i] = strdup (buf->data + ofs);
			else
				return -1;

			while (ofs < buf->used)
				if (buf->data[ofs++] == '\0')
					break;

			contents1 ^= (1 << i);
		}
	}

	for (i = 0; i < 11; i++) {
		if ((contents2 & (1 << i)) != 0) {
			if (ofs <= buf->used)
				c->entry[i + 28] = strdup (buf->data + ofs);
			else
				return -1;

			while (ofs < buf->used)
				if (buf->data[ofs++] == '\0')
					break;

			contents2 ^= (1 << i);
		}
	}

	/* Both bits are set if the birthday field is present */
	if (contents2 & 0x1800) {
		/* Two bytes of padding */
		if (ofs - buf->used < 4)
			return -1;

		c->birthdayFlag = 1;

		packed_date = get_short(buf->data + ofs);
		c->birthday.tm_year = ((packed_date & 0xFE00) >> 9) + 4;
		c->birthday.tm_mon  = ((packed_date & 0x01E0) >> 5) - 1;
		c->birthday.tm_mday = (packed_date & 0x001F);
		c->birthday.tm_hour = 0;
		c->birthday.tm_min  = 0;
		c->birthday.tm_sec  = 0;
		c->birthday.tm_isdst= -1;
		mktime(&c->birthday);

		ofs += 4;

		if (contents2 & 0x2000) {
			if (ofs - buf->used < 1)
				return -1;

			c->reminder = get_byte(buf->data + ofs++);
			contents2 ^= 0x2000;
		} else
			c->reminder = -1;

		contents2 ^= 0x1800;
	} else {
		c->birthdayFlag = 0;
		c->reminder = -1;
	}

	if (contents1 != 0 || contents2 != 0) {
		/* We want to know if this happens! */
		LOG((PI_DBG_API, PI_DBG_LVL_ERR,
					"Contact has remaining fields 0x%08x%08x",
					contents2, contents1));
	}

	if (ofs < buf->used) {
		if (type == contacts_v11) {
			if ((c->picture = pi_buffer_new (buf->used - ofs)) == NULL)
				return -1;

			pi_buffer_append (c->picture, buf->data + ofs, buf->used - ofs);
		} else
			return -1;
	}

	return 0;
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
int
pack_Contact (struct Contact *c, pi_buffer_t *buf, contactsType type)
{
	int 	l,
		destlen = 17;

	size_t ofs;
	unsigned long contents1, contents2;
	unsigned long v;
	unsigned int  field_i;
	unsigned long phoneflag;
	unsigned long typesflag;
	unsigned short packed_date;
	int companyOffset = 0;

	if (c == NULL)
		return -1;

	if (type != contacts_v10
			&& type != contacts_v11)
		/* Don't support anything else yet */
		return -1;
	
	for (v = 0; v < NUM_CONTACT_ENTRIES; v++) {
		if (c->entry[v]) {
			destlen += (strlen(c->entry[v]) + 1);
		}
	}
	if (c->birthdayFlag)
		destlen += 3;
	if (c->reminder != -1)
		destlen += 2;
	if (c->picture != NULL) {
		destlen += c->picture->used;
	}

	if (buf == NULL || buf->data == NULL)
		return -1;
	
	pi_buffer_expect (buf, destlen);

	ofs = 17;
	phoneflag = 0;
	contents1 = 0;
	contents2 = 0;

	field_i = 0;
	for (v = 0; v < 28; v++, field_i++) {
		if (c->entry[field_i] && strlen (c->entry[field_i])) {
			contents1 |= (1 << v);
			l = strlen (c->entry[field_i]) + 1;
			memcpy (buf->data + ofs, c->entry[field_i], l);
			ofs += l;
		}
	}
	for (v = 0; v < 11; v++, field_i++) {
		if (c->entry[field_i] && strlen (c->entry[field_i])) {
			contents2 |= (1 << v);
			l = strlen (c->entry[field_i]) + 1;
			memcpy (buf->data + ofs, c->entry[field_i], l);
			ofs += l;
		}
	}

	if (c->birthdayFlag) {
		contents2 |= 0x1800;
		packed_date = (((c->birthday.tm_year - 4) << 9) & 0xFE00) |
                        (((c->birthday.tm_mon+1) << 5) & 0x01E0) |
			(c->birthday.tm_mday & 0x001F);
		set_short (buf->data + ofs, packed_date);
		ofs += 2;
		set_byte(buf->data + ofs, 0);
		ofs += 1;
		if (c->reminder != -1) {
			contents2 |= 0x2000;
			/* reminder in days */
			set_byte(buf->data + ofs++, 1);
			set_byte(buf->data + ofs++, c->reminder);
		} else
			/* no reminder */
			set_byte(buf->data + ofs++, 0);
	}
	if (type == contacts_v11 && c->picture != NULL) {
		memcpy (buf->data + ofs, c->picture->data, c->picture->used);
		ofs += c->picture->used;
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

	set_long(buf->data, phoneflag);
	set_long(buf->data + 4, typesflag);
	set_long(buf->data + 8, contents1);
	set_long(buf->data + 12, contents2);
	/* companyOffset is the offset from itself to the company field,
	 * or zero if no company field.  Its not useful to us at all.
	 */
	if (c->entry[2]) {
               companyOffset++;
               if (c->entry[0]) companyOffset += strlen(c->entry[0]) + 1;
               if (c->entry[1]) companyOffset += strlen(c->entry[1]) + 1;
	}
	set_byte (buf->data + 16, companyOffset);
	buf->used = ofs;

	return 0;
}


/***********************************************************************
 *
 * Function:    unpack_ContactAppInfo
 *
 * Summary:     Unpacks a raw AppInfo into a usable structure.  palmOne
 *				Contacts 1.0 and 1.1/1.2 are currently supported.
 *
 * Parameters:  *ai, the ContactAppInfo structure to write
 *				*buf, a pi_buffer_t containing (only) the raw AppInfo
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
 * Function:	pack_ContactAppInfo
 *
 * Summary:		Build a raw AppInfo structure from a previously unpacked
 *				CategoryAppInfo structure.
 *
 *				NOTE: ContactAppInfo contains data necessary to produce
 *				only the raw AppInfo it was unpacked from.  Raw AppInfos
 *				can only be written to the devices they were read from.
 *				Use a read-modify-update on devices and database files.
 *				FIXME: Add a note about translate_ContactAppInfo once it
 *				exists.
 *
 * Parameters:	*ai, an unpacked ContactAppInfo structure
 *				*buf, a pi_buffer_t to store the raw AppInfo into
 *
 * Returns:		0 on success
 *				-1 on error
 *
 ***********************************************************************/
int
pack_ContactAppInfo (struct ContactAppInfo *ai, pi_buffer_t *buf)
{
	int				destlen;

	if (buf == NULL || buf->data == NULL)
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
