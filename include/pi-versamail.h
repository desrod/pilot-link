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
		unsigned int rfu1;
		unsigned int download;
		unsigned int mark;
		unsigned int rfu2;		
		unsigned int reserved1;		
		unsigned int reserved2;
		unsigned int read;		
		unsigned int msgSize;
		char *messageUID;
		char *to;
		char *from;
		char *cc;
		char *bcc;
		char *subject;
		char *dateString;
		char *body;
		char *replyTo;
	};


	extern int unpack_VersaMail
	    PI_ARGS((struct VersaMail *, unsigned char *record, int len));
	
	extern void free_VersaMail PI_ARGS((struct VersaMail *));

#ifdef __cplusplus
}
#endif
#endif				/* _PILOT_VERSAMAIL_H_ */
