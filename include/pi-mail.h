#ifndef _PILOT_MAIL_H_
#define _PILOT_MAIL_H_

#include "pi-args.h"
#include "pi-appinfo.h"

#ifdef __cplusplus
extern "C" {
#endif
	
/* Much of this is covered in rfc822, just grab
   it from here: http://www.faqs.org/rfcs/rfc822.html
 */

	struct Mail {
		int 	read,
			signature,
			confirmRead,
			confirmDelivery,
			priority,
			addressing,
			dated;
		struct 	tm date;

		char 	*subject,
			*from,
			*to,
			*cc,
			*bcc,
			*replyTo,
			*sentTo,
			*body;
	};

	struct MailAppInfo {
		int 	dirty,				/* boolean */
			sortOrder;
		struct 	CategoryAppInfo category;
		unsigned long unsentMessage;		/* UniqueID of unsent message */

		/* char *signature; not implemented by Palm */
	};

	struct MailSyncPref {
		int 	syncType,
			getHigh,
			getContaining,
			truncate;
		char 	*filterTo,
			*filterFrom,
			*filterSubject;
	};

	struct MailSignaturePref {
		char 	*signature;
	};

	extern char *MailSyncTypeNames[];
	extern char *MailSortTypeNames[];

	typedef enum {
		mailCtgInbox = 0,
		mailCtgOutbox,
		mailCtgDeleted,
		mailCtgFiled,
		mailCtgDraft
	} MailCategory;

	typedef enum {
		mailSyncAll = 0,
		mailSyncSend,
		mailSyncFilter,
		mailSyncUnread = 3
	} MailSyncType;

	typedef enum {
		/* XXX 0? */
		mailPrefLocal = 1,
		mailPrefRemote,
		mailPrefSig
	} MailPrefId;

	extern void free_Mail PI_ARGS((struct Mail *));
	extern void free_MailAppInfo PI_ARGS((struct MailAppInfo *));
	extern void free_MailSyncPref PI_ARGS((struct MailSyncPref *));
	extern void free_MailSignaturePref
	    PI_ARGS((struct MailSignaturePref *));

	extern int unpack_Mail
		PI_ARGS((struct Mail *, unsigned char *record, int len));

	extern int pack_Mail
		PI_ARGS((struct Mail *, unsigned char *record, int len));

	extern int unpack_MailAppInfo
		PI_ARGS((struct MailAppInfo *, unsigned char *AppInfo,
			int len));

	extern int pack_MailAppInfo
		PI_ARGS((struct MailAppInfo *, unsigned char *AppInfo,
			int len));

	extern int unpack_MailSyncPref
		PI_ARGS((struct MailSyncPref *, unsigned char *record,
			int len));

	extern int unpack_MailSignaturePref
		PI_ARGS((struct MailSignaturePref *, unsigned char *record,
			int len));

	extern int pack_MailSyncPref
		PI_ARGS((struct MailSyncPref *, unsigned char *record,
			int len));

	extern int pack_MailSignaturePref
		PI_ARGS((struct MailSignaturePref *, unsigned char *record,
			int len));

#ifdef __cplusplus
}
#endif
#endif				/* _PILOT_MAIL_H_ */
