#ifndef _PILOT_ADDRESS_H_
#define _PILOT_ADDRESS_H_

enum { entryLastname, entryFirstname, entryCompany, 
       entryPhone1, entryPhone2, entryPhone3, entryPhone4, entryPhone5,
       entryAddress, entryCity, entryState, entryZip, entryCountry, entryTitle,
       entryCustom1, entryCustom2, entryCustom3, entryCustom4,
       entryNote
};

struct Address {
  int phonelabel1, phonelabel2, phonelabel3, phonelabel4, phonelabel5;
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
  int country;
  int sortByCompany;
};

void free_Address(struct Address *);
void unpack_Address(struct Address *, unsigned char * record, int len);
void pack_Address(struct Address *, unsigned char * record, int * len);
void unpack_AddressAppInfo(struct AddressAppInfo *, unsigned char * AppInfo, int len);
void pack_AddressAppInfo(struct AddressAppInfo *, unsigned char * AppInfo, int * len);

#endif /* _PILOT_ADDRESS_H_ */
