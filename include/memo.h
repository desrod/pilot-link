#ifndef _PILOT_MEMO_H_
#define _PILOT_MEMO_H_

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

void free_Memo(struct Memo *);
void unpack_Memo(struct Memo *, unsigned char * record, int len);
void pack_Memo(struct Memo *, unsigned char * record, int * len);
void unpack_MemoAppInfo(struct MemoAppInfo *, unsigned char * AppInfo, int len);
void pack_MemoAppInfo(struct MemoAppInfo *, unsigned char * AppInfo, int * len);

#endif /* _PILOT_MEMO_H_ */
