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

struct Contact {
	int phoneLabel[NUM_CONTACT_PHONES];
	int addressLabel[3];
	int IMLabel[2];
	int showPhone;

	char *entry[NUM_CONTACT_ENTRIES];

	int birthdayFlag;
	struct tm birthday;

	int reminder;

	pi_buffer_t *picture;
};

struct ContactAppInfo {
	contactsType type;
	struct CategoryAppInfo category;
	char internal[26];			/* Palm has not documented what this is */
	int numLabels;
	char labels[53][16];		/* Hairy to explain, obvious to look at 		*/
	char phoneLabels[8][16];	/* Duplication of some labels, to greatly reduce hair 	*/
	char addrLabels[3][16];		/* Duplication of some labels, to greatly reduce hair 	*/
	char IMLabels[5][16];		/* Duplication of some labels, to greatly reduce hair 	*/
	char customLabels[9][16];

	int country;
	int sortByCompany;
};

extern void free_Contact PI_ARGS((struct Contact *));
extern int unpack_Contact
		PI_ARGS((struct Contact *, pi_buffer_t *buf, contactsType type));
extern int pack_Contact
		PI_ARGS((struct Contact *, pi_buffer_t *buf, contactsType type));
extern int unpack_ContactAppInfo
		PI_ARGS((struct ContactAppInfo *, pi_buffer_t *buf));
extern int pack_ContactAppInfo
		PI_ARGS((struct ContactAppInfo *, pi_buffer_t *buf));

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _PILOT_CONTACT_H_ */

/* vi: set ts=4 sw=4 sts=4 noexpandtab: cin */
