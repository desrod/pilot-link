#ifndef _PILOT_MEMO_H_		/* -*- C++ -*- */
#define _PILOT_MEMO_H_

#include "pi-args.h"

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

extern void free_Memo PI_ARGS((struct Memo *));
extern void unpack_Memo PI_ARGS((struct Memo *, unsigned char * record, int len));
extern void pack_Memo PI_ARGS((struct Memo *, unsigned char * record, int * len));
extern void unpack_MemoAppInfo PI_ARGS((struct MemoAppInfo *, unsigned char * AppInfo, int len));
extern void pack_MemoAppInfo PI_ARGS((struct MemoAppInfo *, unsigned char * AppInfo, int * len));

#ifdef __cplusplus
}

#include "pi-memo.hxx"

#endif /*__cplusplus*/

#endif /* _PILOT_MEMO_H_ */
