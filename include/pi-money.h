#ifndef _PILOT_MONEY_H_
#define _PILOT_MONEY_H_

#include "pi-args.h"
#include "pi-appinfo.h"

#ifdef __cplusplus
extern "C" {
#endif

	struct Transaction {
		int 	amountc,	 /* _cents_ as above                     */
			totalc;

		int 	second, 	 /* Date                                 */
			minute,
			hour,
			day,
			month,
			year,
			wday;

		char 	flags;		 /* 1:cleared, 2:Unflagged               */

		unsigned int checknum;	 /* Check number or 0                    */
		long 	amount;		 /* _integer_ amount and                 */
		long 	total;		 /* the running total after cleared      */



		char 	repeat,		 /* 0:single, 1:weekly, 2: every two     */
					 /* weeks, 3:monthly, 4: monthly end     */
			flags2,		 /* 1:receipt                            */
			type,		 /* Type (Category) index to typeLabels  */
			reserved[2],
			xfer,		 /* Account Xfer (index to categories)   */
			description[19], /* Deescription (Payee)                 */
			note[401];	 /* Note (\0)                            */
	};

	struct MoneyAppInfo {
		struct 	CategoryAppInfo category;
		char 	typeLabels[20][10],
			tranLabels[20][20];
	};

	extern int unpack_Transaction PI_ARGS((struct Transaction *,
					       unsigned char *, int));
	extern int pack_Transaction PI_ARGS((struct Transaction *,
					     unsigned char *, int));
	extern int unpack_MoneyAppInfo PI_ARGS((struct MoneyAppInfo *,
						unsigned char *, int));
	extern int pack_MoneyAppInfo PI_ARGS((struct MoneyAppInfo *,
					      unsigned char *, int));

#ifdef __cplusplus
}
#endif				/* __cplusplus */
#endif				/* _PILOT_MONEY_H_ */
