/* appInfo.cc:
 *
 * Copyright (c) 1997, 1998, Scott Grosch
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

#include <string.h>
      
#include "pi-source.h"
#include "pi-appinfo.hxx"

// Constructor for an app info has to determine the base appinfo fields
appInfo_t::appInfo_t(const void *ap) 
{
     _renamedCategories = get_short(ap);

     unsigned char *ptr = ((unsigned char *) ap) + 2;
     
     (void) memcpy(_categoryName, ptr, 256);
     ptr += 256;

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

int appInfo_t::addCategory(charConst_t category)
{
     for (short int i = 0; i < 16; i++)
	  if (_categoryName[i][0] == '\0') {
	       // We found a free slot
	       (void) strcpy(_categoryName[i], category);

	       // Now find an ID to use.  We are allowed between 128 & 255,
	       short int j;
	       unsigned char id_ = 128;
	       for (j = 0; j < 16; j++)
		    if (_categoryName[i][0] != '\0' && _categoryID[j] > id_)
			 id_ = _categoryID[j];

	       if (++id_ >= 255) {
		    id_ = 127;

		    do {
			 id_++;
			 for (j = 0; j < 16; j++)
			      if (_categoryName[i][0] != '\0' &&
				  _categoryID[j] == id_)
				   break;
		    } while (j != 16);
	       }

	       _categoryID[i] = id_;
	       
	       return 1;
	  }

     return 0;
}

// Given an existing category name, we remove it from the local info.  This
// does not modify the info on the pilot.  See comments on addCategory().
// Returns true if the category was removed, false if it was not found
int appInfo_t::removeCategory(charConst_t category) 
{
     for (short int i = 0; i < 16; i++)
	  if (!strcmp(_categoryName[i], category)) {
	       // We found their category
	       _categoryName[i][0] = '\0';
	       return 1;
	  }

     return 0;
}

void appInfo_t::baseAppInfoPack(unsigned char *buffer) 
{
     set_short(buffer, _renamedCategories);

     unsigned char *ptr = ((unsigned char *) buffer) + 2;

     (void) memcpy(ptr, _categoryName, 256);
     ptr += 256;

     (void) memcpy(ptr, _categoryID, 16);

     ptr += 16;
     set_byte(ptr, _lastUniqueID);
}
