#ifndef _PILOT_ADDRESS_H_
#define _PILOT_ADDRESS_H_

#include "pi-args.h"
#include "pi-appinfo.h"

#ifdef __cplusplus
extern "C" {
#endif

	enum { entryLastname, entryFirstname, entryCompany,
		entryPhone1, entryPhone2, entryPhone3, entryPhone4,
		    entryPhone5,
		entryAddress, entryCity, entryState, entryZip,
		    entryCountry, entryTitle,
		entryCustom1, entryCustom2, entryCustom3, entryCustom4,
		entryNote
	};

	struct Address {
		int phoneLabel[5];
		int showPhone;

		char *entry[19];
	};

	struct AddressAppInfo {
		struct CategoryAppInfo category;
		char labels[19 + 3][16];		/* Hairy to explain, obvious to look at 		*/
		int labelRenamed[19 + 3];		/* list of booleans showing which labels were modified 	*/
		char phoneLabels[8][16];		/* Duplication of some labels, to greatly reduce hair 	*/
		int country;
		int sortByCompany;
	};

	extern void free_Address PI_ARGS((struct Address *));
	extern int unpack_Address
	    PI_ARGS((struct Address *, unsigned char *record, int len));
	extern int pack_Address
	    PI_ARGS((struct Address *, unsigned char *record, int len));
	extern int unpack_AddressAppInfo
	    PI_ARGS((struct AddressAppInfo *, unsigned char *AppInfo,
		     int len));
	extern int pack_AddressAppInfo
	    PI_ARGS((struct AddressAppInfo *, unsigned char *AppInfo,
		     int len));

#ifdef __cplusplus
}
#include "pi-address.hxx"
#endif				/* __cplusplus */
#endif				/* _PILOT_ADDRESS_H_ */
