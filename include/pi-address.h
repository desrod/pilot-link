#ifndef _PILOT_ADDRESS_H_
#define _PILOT_ADDRESS_H_

#include "pi-args.h"

#ifdef __cplusplus
extern "C" {
#endif

enum { entryLastname, entryFirstname, entryCompany, 
       entryPhone1, entryPhone2, entryPhone3, entryPhone4, entryPhone5,
       entryAddress, entryCity, entryState, entryZip, entryCountry, entryTitle,
       entryCustom1, entryCustom2, entryCustom3, entryCustom4,
       entryNote
};

struct Address {
  int phonelabel[5];
  int whichphone;
  
  char *entry[19];
};

struct AddressAppInfo {
  unsigned int renamedcategories; /* Bitfield of categories with changed names */
  char CategoryName[16][16]; /* 16 categories of 15 characters+nul each */
  unsigned char CategoryID[16]; 
  unsigned char lastUniqueID; /* Each category gets a unique ID, for sync tracking
                                 purposes. Those from the Pilot are between 0 & 127.
                                 Those from the PC are between 128 & 255. I'm not
                                 sure what role lastUniqueID plays. */
  unsigned long dirtyfieldlabels; /* bitfield of same */
  char labels[19+3][16]; /* Hairy to explain, obvious to look at */
  char phonelabels[8][16]; /* Duplication of some labels, to
                              greatly reduce hair */
  int country;
  int sortByCompany;
};

extern void free_Address PI_ARGS((struct Address *));
extern void unpack_Address PI_ARGS((struct Address *, unsigned char * record, int len));
extern void pack_Address PI_ARGS((struct Address *, unsigned char * record, int * len));
extern void unpack_AddressAppInfo PI_ARGS((struct AddressAppInfo *, unsigned char * AppInfo, int len));
extern void pack_AddressAppInfo PI_ARGS((struct AddressAppInfo *, unsigned char * AppInfo, int * len));

#ifdef __cplusplus
}

#include "pi-address.hxx"

#endif /* __cplusplus */

#endif /* _PILOT_ADDRESS_H_ */
