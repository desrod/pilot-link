/* -*- Mode: C; tab-width: 2 -*- */
#ifndef _PILOT_VERSAMAIL_H_
#define _PILOT_VERSAMAIL_H_

#include <time.h>
#include "pi-args.h"
#include "pi-appinfo.h"

#ifdef __cplusplus
extern "C" {
#endif

	struct VersaMail {
		unsigned long imapuid;
		struct tm date;
		unsigned int category;
		unsigned int accountNo;
		unsigned int unknown1;
		unsigned int download;
		unsigned int mark;
		unsigned int unknown2;		
		unsigned int reserved1;		
		unsigned int reserved2;
		unsigned int read;		
		unsigned int msgSize;
		unsigned int attachmentCount;
		char *messageUID;
		char *to;
		char *from;
		char *cc;
		char *bcc;
		char *subject;
		char *dateString;
		char *body;
		char *replyTo;
		void *unknown3;
		unsigned int unknown3length;
	};

	struct VersaMailAppInfo {
		struct CategoryAppInfo category;
	};

	extern int unpack_VersaMail
	    PI_ARGS((struct VersaMail *, unsigned char *record, int len));
	extern int pack_VersaMail
	    PI_ARGS((struct VersaMail *a, unsigned char *buffer, int len));
	
	extern void free_VersaMail PI_ARGS((struct VersaMail *));

	extern void free_VersaMailAppInfo PI_ARGS((struct VersaMailAppInfo *));
	extern int unpack_VersaMailAppInfo PI_ARGS((struct VersaMailAppInfo *,
																							unsigned char *AppInfo, int len));
	
#ifdef __cplusplus
}
#endif
#endif				/* _PILOT_VERSAMAIL_H_ */
