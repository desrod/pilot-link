#ifndef _PILOT_MONEY_H_
#define _PILOT_MONEY_H_

#include "pi-appinfo.h"

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct Transaction {
		char flags;		/* 1:cleared, 2:Unflagged  */
		unsigned int checknum;	/* Check number or 0       */
		long amount;		/* _integer_ amount and    */
		long total;		/* the running total after cleared  */
		int amountc;		/* _cents_ as above        */
		int totalc;

		int second;		/* Date                     */
		int minute;
		int hour;
		int day;
		int month;
		int year;
		int wday;

		char repeat;	/* 0:single, 1:weekly, 2: every two */
				/* weeks, 3:monthly, 4: monthly end */
		char flags2;	/* 1:receipt                        */
		char type;	/* Type (Category) index to typeLabels */
		char reserved[2];
		char xfer;	/* Account Xfer (index to categories) */
		char description[19];	/* Deescription (Payee)       */
		char note[401];		/* Note (\0)                  */
	} Transaction_t;

	typedef struct MoneyAppInfo {
		struct CategoryAppInfo category;
		char typeLabels[20][10];
		char tranLabels[20][20];
	} MoneyAppInfo_t;

	extern int unpack_Transaction PI_ARGS((Transaction_t *,
					       unsigned char *, size_t));
	extern int pack_Transaction PI_ARGS((Transaction_t *,
					     unsigned char *, size_t));
	extern int unpack_MoneyAppInfo PI_ARGS((MoneyAppInfo_t *,
						unsigned char *, size_t));
	extern int pack_MoneyAppInfo PI_ARGS((MoneyAppInfo_t *,
					      unsigned char *, size_t));

#ifdef __cplusplus
}
#endif				/* __cplusplus */
#endif				/* _PILOT_MONEY_H_ */
