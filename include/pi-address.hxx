#include "pi-appinfo.hxx"

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

     address_t(void *buf) { unpack(buf); }
     address_t(void) { memset(this, '\0', sizeof(address_t)); }
     address_t(void *buf, int attr, recordid_t id, int category)
	  : baseApp_t(attr, id, category)
	  {
	       unpack(buf);
	  }
     address_t(const address_t &);
     
     ~address_t(void);

     char *entry(labelTypes_t idx) { return _entry[idx]; }
     int whichPhone(void) const { return _whichPhone; }
     int phoneLabel(int idx) { return _phoneLabels[idx]; }
     
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
     
     address_t *first() { return _head; }
     address_t *next(address_t *ptr) { return ptr->_next; }

     void merge(address_t &);
     void merge(addressList_t &);
};

