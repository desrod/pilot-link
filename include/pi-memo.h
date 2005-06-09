#ifndef _PILOT_MEMO_H_		/* -*- C++ -*- */
#define _PILOT_MEMO_H_

#include "pi-appinfo.h"
#include "pi-buffer.h"

	typedef enum {
		memo_v1,
	} memoType;
  
	typedef struct Memo {
		char *text;
	} Memo_t;

	typedef struct MemoAppInfo {
		memoType type;
		struct CategoryAppInfo category;
		/* New for 2.0 memo application, 0 is manual, 1 is
		   alphabetical. 
		 */
		int sortByAlpha;	

	} MemoAppInfo_t;

	extern void free_Memo PI_ARGS((struct Memo *));
	extern int unpack_Memo
	    PI_ARGS((struct Memo *, pi_buffer_t *record, memoType type));
	extern int pack_Memo
	    PI_ARGS((struct Memo *, pi_buffer_t *record, memoType type));
	extern int unpack_MemoAppInfo
	    PI_ARGS((struct MemoAppInfo *, unsigned char *AppInfo,
		     size_t len));
	extern int pack_MemoAppInfo
	    PI_ARGS((struct MemoAppInfo *, unsigned char *AppInfo,
		     size_t len));

#endif				/* _PILOT_MEMO_H_ */
