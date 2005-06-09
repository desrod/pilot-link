#ifndef _PILOT_ADDRESS_H_
#define _PILOT_ADDRESS_H_

#include "pi-appinfo.h"
#include "pi-buffer.h"

	typedef enum {
		address_v1,
	} addressType;
  
	enum {  entryLastname, 
		entryFirstname, 
		entryCompany, 
		entryPhone1, 
		entryPhone2,
		entryPhone3,
		entryPhone4,
		entryPhone5,
		entryAddress,
		entryCity,
		entryState,
		entryZip,
		entryCountry,
		entryTitle,
		entryCustom1,
		entryCustom2,
		entryCustom3,
		entryCustom4,
		entryNote,
		entryCategory
	};

	typedef struct Address {
		int phoneLabel[5];
		int showPhone;

		char *entry[19];
	} Address_t;

	typedef struct AddressAppInfo {
		addressType type;
		struct CategoryAppInfo category;
		char labels[19 + 3][16]; /* Hairy explain, obvious to look */
		int labelRenamed[19 + 3]; /* booleans show labels modified */
		char phoneLabels[8][16]; /* Dup some labels, reduce hair */
		int country;
		int sortByCompany;
	} AddressAppInfo_t;

	extern void free_Address
	  PI_ARGS((Address_t *));
	extern int unpack_Address
	  PI_ARGS((Address_t *, pi_buffer_t *buf, addressType type));
	extern int pack_Address
	  PI_ARGS((Address_t *, pi_buffer_t *buf, addressType type));
	extern int unpack_AddressAppInfo
	  PI_ARGS((AddressAppInfo_t *, unsigned char *AppInfo, size_t len));
	extern int pack_AddressAppInfo
	  PI_ARGS((AddressAppInfo_t *, unsigned char *AppInfo, size_t len));

#endif				/* _PILOT_ADDRESS_H_ */
