#ifndef _PILOT_MEMO_H_		/* -*- C++ -*- */
#define _PILOT_MEMO_H_

#include "pi-args.h"
#include "pi-appinfo.h"

#ifdef __cplusplus
extern "C" {
#endif

	struct Memo {
		char *text;
	};

	struct MemoAppInfo {
		struct CategoryAppInfo category;
		/* New for 2.0 memo application, 0 is manual, 1 is
		   alphabetical. 
		 */
		int sortByAlpha;	

	};

	extern void free_Memo PI_ARGS((struct Memo *));
	extern int unpack_Memo
	    PI_ARGS((struct Memo *, unsigned char *record, int len));
	extern int pack_Memo
	    PI_ARGS((struct Memo *, unsigned char *record, int len));
	extern int unpack_MemoAppInfo
	    PI_ARGS((struct MemoAppInfo *, unsigned char *AppInfo,
		     int len));
	extern int pack_MemoAppInfo
	    PI_ARGS((struct MemoAppInfo *, unsigned char *AppInfo,
		     int len));

#ifdef __cplusplus
}
#include "pi-memo.hxx"
#endif				/*__cplusplus*/
#endif				/* _PILOT_MEMO_H_ */
