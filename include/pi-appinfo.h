#ifndef __PI_APP_INFO_H
#define __PI_APP_INFO_H

#ifdef __cplusplus

#include <memory.h>		// For memset on some architectures
#include <string.h>
#include <sys/types.h>
#include "pi-macros.h"

const BASE_APP_INFO_SIZE = 278;	// All apps take up 278 bytes of the same stuff

typedef char category_t[16][16];
typedef const char *const charConst_t;
typedef const unsigned char *const ucharConst_t;

class appInfo_t 
{
   protected:			// Use protected since we will be subclassed
     int _renamedCategories;
     category_t _categoryName;
     uchar_t _categoryID[16];
     uchar_t _lastUniqueID;

     void baseAppInfoPack(uchar_t *);
     
   public:
     appInfo_t(const void *);
     
     char *category(const int);
     int categoryIndex(charConst_t) const;
     bool addCategory(charConst_t);
     const category_t &allCategories(void) const { return _categoryName; }
     bool removeCategory(charConst_t);
     ucharConst_t categoryID(void) const { return _categoryID; }
     uchar_t lastUniqueID(void) const { return _lastUniqueID; }
     int renamedCategories(void) const { return _renamedCategories; }

     virtual void *pack(void) = 0;
};

#endif /*__cplusplus*/

#endif /* __PI_APP_INFO_H */
