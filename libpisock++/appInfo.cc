#include "pi-appinfo.h"

// Constructor for an app info has to determine the base appinfo fields
appInfo_t::appInfo_t(const void *ap) 
{
     _renamedCategories = get_short(ap);

     uchar_t *ptr = ((uchar_t *) ap) + 2;
     
#if 1
     (void) memcpy(_categoryName, ptr, 256);
     ptr += 256;
#else
     for (short int i = 0; i < 16; i++) {
	  (void) memcpy(_categoryName[i], ptr, 16);
	  ptr += 16;
     }
#endif

     (void) memcpy(_categoryID, ptr, 16);

     ptr += 16;
     _lastUniqueID = get_byte(ptr);
}

// Given an integer location, we return the category name, or NULL if not found
char *appInfo_t::category(const int idx)
{
     if (idx < 0 || idx > 15)
	  return NULL;

     return _categoryName[idx];
}

// Given a category name, we return it's integer location, or -1 if not found
int appInfo_t::categoryIndex(charConst_t category) const 
{
     for (short int i = 0; i < 16; i++)
	  if (!strcmp(_categoryName[i], category))
	       return i;

     return -1;
}

// Given a new category name, we add it to the local info.  This does NOT
// modify the info on the pilot.  To actually change the data on the pilot
// you need to pack this application info and then load it to the pilot
// If false is returned, there are already 16 categories defined

// WARNING - DO NOT USE THIS FUNCTION YET.  IT IS NOT COMPLETELY WRITTEN.
// I STILL NEED TO HAVE IT ASSIGN A NEW UNIQUE ID TO THE CATEGORY
bool appInfo_t::addCategory(charConst_t category)
{
     return false; // To be removed once I finish the function

     for (short int i = 0; i < 16; i++)
	  if (_categoryName[i][0] == '\0') {
	       // We found a free slot
	       (void) strcpy(_categoryName[i], category);
	       return true;
	  }

     return false;
}

// Given an existing category name, we remove it from the local info.  This
// does not modify the info on the pilot.  See comments on addCategory().
// Returns true if the category was removed, false if it was not found

// WARNING - DO NOT USE THIS FUNCTION YET.  IT IS NOT COMPLETELY WRITTEN.
// I STILL NEED TO HAVE IT COMPACT THE CATEGORIES AFTER IT DELETES ONE
bool appInfo_t::removeCategory(charConst_t category) 
{
     return false;  // To be removed once I finish the function
     for (short int i = 0; i < 16; i++)
	  if (!strcmp(_categoryName[i], category)) {
	       // We found their category
	       (void) memset(_categoryName[i], '\0', 16);
	       return true;
	  }

     return false;
}

void appInfo_t::baseAppInfoPack(uchar_t *buffer) 
{
     set_short(buffer, _renamedCategories);

     uchar_t *ptr = ((uchar_t *) buffer) + 2;
#if 1
     (void) memcpy(ptr, _categoryName, 256);
     ptr += 256;
#else
     for (short i = 0; i < 16; i++) {
	  (void) memcpy(ptr, _categoryName[i], 16);
	  ptr += 16;
     }
#endif

     (void) memcpy(ptr, _categoryID, 16);

     ptr += 16;
     set_byte(ptr, _lastUniqueID);
}
