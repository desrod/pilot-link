/* 
 * pi-contact.h: Support for palmOne's Contacts
 *
 * Copyright 2004  Joseph Carter
 * portions of this code are Copyright 2004 Judd Montgomery
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

#ifndef _PILOT_CONTACT_H_
#define _PILOT_CONTACT_H_

#include <time.h>

#include "pi-args.h"
#include "pi-appinfo.h"
#include "pi-buffer.h"

#define NUM_CONTACT_PHONES 7
#define NUM_CONTACT_CUSTOMS 9
#define NUM_CONTACT_ENTRIES 39
#define NUM_CONTACT_LABELS 53

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	contacts_v10,
	contacts_v11
} contactsType;

typedef enum {
	cpic_none,
	cpic_jpeg
} contactsPicType;

enum {
	contLastname,
	contFirstname,
	contCompany,
	contTitle,
	contPhone1,
	contPhone2,
	contPhone3,
	contPhone4,
	contPhone5,
	contPhone6,
	contPhone7,
	contIM1,
	contIM2,
	contWebsite,
	contCustom1,
	contCustom2,
	contCustom3,
	contCustom4,
	contCustom5,
	contCustom6,
	contCustom7,
	contCustom8,
	contCustom9,
	contAddress1,
	contCity1,
	contState1,
	contZip1,
	contCountry1,
	contAddress2,
	contCity2,
	contState2,
	contZip2,
	contCountry2,
	contAddress3,
	contCity3,
	contState3,
	contZip3,
	contCountry3,
	contNote
};

typedef struct Contact {
	int phoneLabel[NUM_CONTACT_PHONES];
	int addressLabel[3];
	int IMLabel[2];
	int showPhone;

	char *entry[NUM_CONTACT_ENTRIES];

	int birthdayFlag;
	struct tm birthday;

	int reminder;

	contactsPicType pictype;
	pi_buffer_t *picture;
} Contact_t;

typedef struct ContactAppInfo {
	contactsType type;
	struct CategoryAppInfo category;

	/* These fields should be considered opaque */
	pi_buffer_t *internal;
	pi_buffer_t *labels;

	int numCustoms;
	char customLabels[9][16];

	int country;
	int sortByCompany;
} ContactAppInfo_t;


/***********************************************************************
 *
 * Function:	free_Contact
 *
 * Summary:		Frees (only) the allocated contents of a Contact.  Call
 *				this when the structure's data is no longer needed.
 *
 * Parameters:	+ *c, the Contact_t to be disposed of
 *
 * Returns:		Nothing
 *
 ***********************************************************************/
extern void free_Contact PI_ARGS((Contact_t *c));


/***********************************************************************
 *
 * Function:	unpack_Contact
 *
 * Summary:		Unpacks raw contact data into a common structure for
 *				all known versions of palmOne Contacts
 *
 * Parameters:	+ *c, a Contact_t to fill in
 *				- *buf, a pi_buffer_t containing (only) the raw record
 *				- type, the type field from ContactAppInfo
 *
 * Returns:     0 on success
 *				-1 on error
 *
 ***********************************************************************/
extern int unpack_Contact
		PI_ARGS((Contact_t *c, pi_buffer_t *buf, contactsType type));


/***********************************************************************
 *
 * Function:    pack_Contact
 *
 * Summary:     Packs a Contact structure into raw data corresponding to
 *				that of a given version of palmOne Contacts
 *
 *				NOTE: Byte-for-byte reproduction of the original raw
 *				data is not guaranteed.  Moreover, if the target type
 *				does not support a given field in the Contact structure,
 *				that field is simply ignored to allow forward and
 *				backward compatibility.
 *
 * Parameters:  - *c, a Contact_t containing the data to pack
 *				+ *buf, a pi_buffer_t to write to
 *				- type, the desired format of the output
 *
 * Returns:     0 on success
 *				-1 on error
 *
 ***********************************************************************/
extern int pack_Contact
		PI_ARGS((Contact_t *c, pi_buffer_t *buf, contactsType type));


/***********************************************************************
 *
 * Function:    free_ContactAppInfo
 *
 * Summary:		Frees (only) the allocated fields of a ContactAppInfo
 *				structure.
 *
 * Parameters:	+ *ai, the ContactsAppInfo_t to dispose of
 *
 * Returns:		Nothing
 *
 ***********************************************************************/
extern void free_ContactAppInfo PI_ARGS((ContactAppInfo_t *ai));


/***********************************************************************
 *
 * Function:    unpack_ContactAppInfo
 *
 * Summary:     Unpacks a raw AppInfo into a usable structure.  palmOne
 *				Contacts 1.0 and 1.1/1.2 are currently supported.
 *
 * Parameters:  + *ai, the ContactAppInfo_t to fill in
 *				- *buf, a pi_buffer_t containing (only) the raw AppInfo
 *
 * Returns:     0 on success
 *				-1 on error
 *
 ***********************************************************************/
extern int unpack_ContactAppInfo
		PI_ARGS((ContactAppInfo_t *ai, pi_buffer_t *buf));


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
 * Parameters:	- *ai, a ContactAppInfo of the type to pack
 *				+ *buf, a pi_buffer_t to store the raw AppInfo into
 *
 * Returns:		0 on success
 *				-1 on error
 *
 ***********************************************************************/
extern int pack_ContactAppInfo
		PI_ARGS((ContactAppInfo_t *ai, pi_buffer_t *buf));


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _PILOT_CONTACT_H_ */

/* vi: set ft=c tw=78 ts=4 sw=4 sts=4 noexpandtab: cin */
