#ifndef _PILOT_APPINFO_H_	/* -*- C++ -*- */
#define _PILOT_APPINFO_H_

#include "pi-args.h"

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct CategoryAppInfo {
		unsigned int renamed[16];	/* Boolean array of categories
						 with changed names */
		char name[16][16];		/* 16 categories of 15
						 characters+nul each */
		unsigned char ID[16];
		unsigned char lastUniqueID;	/* Each category gets a unique
						 ID, for sync tracking
						 purposes. Those from the
						 Palm are between 0 & 127.
						 Those from the PC are between
						 128 & 255. I'm not sure what
						 role lastUniqueID plays.
						 */
	} CategoryAppInfo_t;

	extern int unpack_CategoryAppInfo
	    PI_ARGS((CategoryAppInfo_t *, unsigned char *AppInfo, size_t len));
	extern int pack_CategoryAppInfo
	    PI_ARGS((CategoryAppInfo_t *, unsigned char *AppInfo, size_t len));

#ifdef __cplusplus
}
#include "pi-appinfo.hxx"
#endif				/*__cplusplus*/
#endif				/* _PILOT_APPINFO_H_ */
