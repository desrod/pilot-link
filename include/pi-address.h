#ifndef _PILOT_ADDRESS_H_
#define _PILOT_ADDRESS_H_

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

extern void free_Address(struct Address *);
extern void unpack_Address(struct Address *, unsigned char * record, int len);
extern void pack_Address(struct Address *, unsigned char * record, int * len);
extern void unpack_AddressAppInfo(struct AddressAppInfo *, unsigned char * AppInfo, int len);
extern void pack_AddressAppInfo(struct AddressAppInfo *, unsigned char * AppInfo, int * len);

#ifdef __cplusplus
}

#include "pi-appinfo.h"

const int ADDRESS_APP_INFO_SIZE = 638;

typedef char addressLabels_t[22][16];
typedef char addressPhoneLabels_t[8][16];

class addressAppInfo_t : public appInfo_t
{
     unsigned long _dirtyFieldLabels;
     addressLabels_t _labels;
     addressPhoneLabels_t _phoneLabels;
     int _country;
     int _sortByCompany;
     
   public:
     addressAppInfo_t(void *);

     void *pack(void);

     const addressLabels_t &labels(void) const { return _labels; }
     const addressPhoneLabels_t &phoneLabels(void) const { return _phoneLabels; }
     int country(void) const { return _country; }
     int sortByCompany(void) const { return _sortByCompany; }
};

class addressList_t;	// Forward declaration

class address_t : public baseApp_t
{
     int _phoneLabels[5];
     int _whichPhone;

     char *_entry[19];
     
     friend addressList_t;
     
     address_t *_next;

     void *internalPack(unsigned char *);
     
   public:
     enum labelTypes_t {
	  lastName, firstName, company, phone1, phone2, phone3, phone4,
	  phone5, address, city, state, zip, country, title, custom1,
	  custom2, custom3, custom4, note
     };

     address_t(void *buf) { unpack(buf, true); }
     address_t(void) { memset(this, '\0', sizeof(address_t)); }
     address_t(void *buf, int attr, recordid_t id, int category)
	  : baseApp_t(attr, id, category)
	  {
	       unpack(buf, true);
	  }
     address_t(const address_t &);
     
     ~address_t(void);

     char *entry(labelTypes_t idx) { return _entry[idx]; }
     int whichPhone(void) const { return _whichPhone; }
     
     void unpack(void *, bool = false);

     void *pack(int *);
     void *pack(void *, int *);
};

class addressList_t 
{
     address_t *_head;
     
   public:
     addressList_t(void) : _head(NULL) { }
     ~addressList_t();
     
     address_t *first() { return _head; }
     address_t *next(address_t *ptr) { return ptr->_next; }

     void merge(address_t &);
     void merge(addressList_t &);
};

#endif /* __cplusplus */

#endif /* _PILOT_ADDRESS_H_ */
