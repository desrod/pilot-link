#ifndef _PILOT_CONTACT_H_
#define _PILOT_CONTACT_H_

#include <time.h>

#include <pi-args.h>
#include <pi-appinfo.h>

/*
 * This code works on the Tungsten E and T3, but breaks other devices (not
 * just breaks on other devices!)  You've been warned.  --KB
 */

#define NUM_CONTACT_PHONES 7
#define NUM_CONTACT_CUSTOMS 9
#define NUM_CONTACT_ENTRIES 39
#define NUM_CONTACT_LABELS 49

#ifdef __cplusplus
extern "C" {
#endif

	enum {  contLastname, 
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
	        int birthdayFlag;
		/* int alarm; */
		int reminderFlag;
		int advance;
		int advanceUnits;
	        struct tm birthday;
		char *entry[NUM_CONTACT_ENTRIES];
	};

	struct ContactAppInfo {
		struct CategoryAppInfo category;
		char unknown1[26];			/* Palm has not documented what this is */
		char labels[49][16];		/* Hairy to explain, obvious to look at 		*/
		/* I haven't figured how to derive this next field from the data yet, JBM */
		/*int labelRenamed[48];*/		/* list of booleans showing which labels were modified 	*/
		int country;
		int sortByCompany;
		char phoneLabels[8][16];		/* Duplication of some labels, to greatly reduce hair 	*/
		char addrLabels[3][16];		/* Duplication of some labels, to greatly reduce hair 	*/
		char IMLabels[5][16];		/* Duplication of some labels, to greatly reduce hair 	*/
	};

	extern void free_Contact PI_ARGS((struct Contact *));
	extern int unpack_Contact
	    PI_ARGS((struct Contact *, unsigned char *record, int len));
	extern int pack_Contact
	    PI_ARGS((struct Contact *, unsigned char *record, int len));
	extern int unpack_ContactAppInfo
	    PI_ARGS((struct ContactAppInfo *, unsigned char *AppInfo,
		     int len));
	extern int pack_ContactAppInfo
	    PI_ARGS((struct ContactAppInfo *, unsigned char *AppInfo,
		     int len));

#ifdef __cplusplus
}
#include "pi-contact.hxx"
#endif				/* __cplusplus */
#endif				/* _PILOT_CONTACT_H_ */
