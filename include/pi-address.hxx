#ifndef _PI_ADDRESS_HXX_
#define _PI_ADDRESS_HXX_

#include "pi-appinfo.hxx"

const int ADDRESS_APP_INFO_SIZE = 638;

typedef char addressLabels_t[22][16];
typedef char addressPhoneLabels_t[8][16];

class addressList_t;	// Forward declaration

class addressAppInfo_t : public appInfo_t
{
	unsigned long _dirtyFieldLabels;
	addressLabels_t _labels;
	addressPhoneLabels_t _phoneLabels;
	int _country;
	int _sortByCompany;
	
public:
	addressAppInfo_t(void *);
	
	const addressLabels_t *labels(void);	
	const addressPhoneLabels_t *phoneLabels(void);
	int country(void);
	int sortByCompany(void);
	
	void *pack(void);
};

class address_t : public baseApp_t
{
	friend class addressList_t;

	int _phoneLabels[5];
	int _whichPhone;
	
	char *_entry[19];
     
	address_t *_next;
	
	void *internalPack(unsigned char *);
     
public:
	enum labelTypes_t {
		lastName, 
		firstName, 
		company, 
		phone1, 
		phone2, 
		phone3, 
		phone4,
		phone5, 
		address, 
		city, 
		state, 
		zip, 
		country, 
		title, 
		custom1,
		custom2, 
		custom3, 
		custom4, 
		note,
		category
	};
	
	address_t(void);	
	address_t(void *buf);
	
	address_t(void *buf, int attr, recordid_t id, int category);
	address_t(const address_t &);
	
	~address_t(void);

	char *entry(labelTypes_t idx);
	int whichPhone(void);
	int phoneLabel(int idx);
	
	void unpack(void *);
	
	void *pack(int *);
	void *pack(void *, int *);
};

class addressList_t 
{
	address_t *_head;
	
public:
	addressList_t(void) : _head(NULL) { }
	~addressList_t();
	
	address_t *first();
	address_t *next(address_t *ptr);
	
	void merge(address_t &);
	void merge(addressList_t &);
};

#endif /* _PI_ADDRESS_HXX */
