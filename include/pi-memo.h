#ifndef _PILOT_MEMO_H_		/* -*- C++ -*- */
#define _PILOT_MEMO_H_

#ifdef __cplusplus
extern "C" {
#endif

struct Memo {
  char * text;
};

struct MemoAppInfo {
  unsigned int renamedcategories; /* Bitfield of categories with changed names */
  char CategoryName[16][16]; /* 16 categories of 15 characters+nul each */
  unsigned char CategoryID[16]; 
  unsigned char lastUniqueID; /* Each category gets a unique ID, for sync tracking
                                 purposes. Those from the Pilot are between 0 & 127.
                                 Those from the PC are between 128 & 255. I'm not
                                 sure what role lastUniqueID plays. */
  int sortOrder; /* New for 2.0 memo application, 0 is manual, 1 is alphabetical. */
};

extern void free_Memo(struct Memo *);
extern void unpack_Memo(struct Memo *, unsigned char * record, int len);
extern void pack_Memo(struct Memo *, unsigned char * record, int * len);
extern void unpack_MemoAppInfo(struct MemoAppInfo *, unsigned char * AppInfo, int len);
extern void pack_MemoAppInfo(struct MemoAppInfo *, unsigned char * AppInfo, int * len);

#ifdef __cplusplus
}

#include "pi-appinfo.h"

const int MEMO_APP_INFO_SIZE = BASE_APP_INFO_SIZE;

struct memoAppInfo_t : public appInfo_t 
{
     memoAppInfo_t(void *buffer) : appInfo_t(buffer) { }
     
     void *pack(void) 
     {
	  unsigned char *ret = new unsigned char [MEMO_APP_INFO_SIZE];
	  baseAppInfoPack(ret);
	  return ret;
     }
};

class memoList_t;		// Forward declaration for older compilers

class memo_t : public baseApp_t
{
     friend memoList_t;
     
     char *_text;
     int _size;
     
     memo_t *_next;
	  
     void *internalPack(unsigned char *);

   public:
     memo_t(void) : baseApp_t() { _text = NULL; _size = 0; }
     memo_t(void *buf) : baseApp_t() { unpack(buf, true); }
     memo_t(void *buf, int attr, recordid_t id, int category)
	  : baseApp_t(attr, id, category)
     {
	       unpack(buf, true);
     }
     memo_t(const memo_t &);

     void unpack(void *, bool  = false);
     ~memo_t() { if (_text) delete _text; }

     void *pack(int *i);
     void *pack(void *, int *);

     const char *text(void) const { return _text; }
};

class memoList_t 
{
     memo_t *_head;
     
   public:
     memoList_t(void) : _head(NULL) { }
     ~memoList_t();
     
     memo_t *first() { return _head; }
     memo_t *next(memo_t *ptr) { return ptr->_next; }

     void merge(memo_t &);
     void merge(memoList_t &);
};
     
#endif /*__cplusplus*/

#endif /* _PILOT_MEMO_H_ */
