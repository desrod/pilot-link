#ifndef _PILOT_MEMO_H_
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
	  uchar_t *ret = new uchar_t [MEMO_APP_INFO_SIZE];
	  baseAppInfoPack(ret);
	  return ret;
     }
};

class memo_t
{
   protected:
     char *_text;
     int _size;
     
   public:
     memo_t(void) { _text = NULL; _size = 0; }
     memo_t(void *buf) { unpack(buf, true); }
     void unpack(void *text, bool firstTime = false) {
	  if (firstTime == false && _text)
	       delete _text;
	       
	  _size = strlen((const char *) text) + 1;
	  _text = new char [_size];
	  
	  (void) strcpy(_text, (const char *) text);
     }
     ~memo_t() { delete [] _text; }

     void *pack(int *i) 
     {
	  *i = _size;
	  char *ret = new char [_size];
	  return strcpy(ret, _text);
     }

     void *pack(void *buf, int *i) 
     {
	  if (*i < _size)
	       return NULL;

	  *i = _size;
	  return strcpy((char *) buf, _text);
     }

     const char *text(void) const { return _text; }
};

#endif /*__cplusplus*/

#endif /* _PILOT_MEMO_H_ */
