#ifndef __PI_APP_INFO_HXX		/* -*- C++ -*- */
#define __PI_APP_INFO_HXX

#ifdef __cplusplus

#include "pi-macros.h"

const int BASE_APP_INFO_SIZE = 278;	// All apps take up 278 bytes of the same stuff

typedef char category_t[16][16];
typedef const char *const charConst_t;
typedef const unsigned char *const ucharConst_t;

class appInfo_t 
{
   protected:			// Use protected since we will be subclassed
     int _renamedCategories;
     category_t _categoryName;
     unsigned char _categoryID[16];
     unsigned char _lastUniqueID;

     void baseAppInfoPack(unsigned char *);
     
   public:
     appInfo_t(const void *);
     
     char *category(const int);
     int categoryIndex(charConst_t) const;
     int addCategory(charConst_t);
     const category_t &allCategories(void) const { return _categoryName; }
     int removeCategory(charConst_t);
     ucharConst_t categoryID(void) const { return _categoryID; }
     unsigned char lastUniqueID(void) const { return _lastUniqueID; }
     int renamedCategories(void) const { return _renamedCategories; }

     virtual void *pack(void) = 0;
};

class baseApp_t
{
   protected:			// Use protected since we will be subclassed
     int _attrs;		// Attributes on this record
     recordid_t _id;		// The unique ID this record was assigned

     /*
      * This field stores the category this record belongs to.  It will be
      * whatever the pilot said it was.  Note that if you change the categories
      * for the app by calling addCategory or removeCategory in the appInfo_t
      * class, these fields will become invalid.  We have no way to mark them
      * invalid automatically though.  If you're going to change category names
      * in your program, you can't use this field safely.
      */
     int _category;		// The category ID this record belongs to
     
     virtual void *internalPack(unsigned char *) = 0;
     
   public:
     baseApp_t(void) : _attrs(-1), _id(0), _category(-1) {}
     baseApp_t(int a, recordid_t i, int c) : _attrs(a), _id(i), _category(c) {}

     // The destructor does nothing, but we need to provide it since we have
     // virtual functions
     virtual ~baseApp_t(void) {}
     
     virtual void unpack(void *) = 0;
     virtual void *pack(int *) = 0;
     virtual void *pack(void *, int *) = 0;

     int attrs(void) const { return _attrs; }
     int category(void) const { return _category; }
     recordid_t id(void) const { return _id; }
};
     
#endif /*__cplusplus*/

#endif /* __PI_APP_INFO_HXX */
