#ifndef _PILOT_EXPENSE_H_
#define _PILOT_EXPENSE_H_

#include "pi-args.h"
#include "pi-appinfo.h"

#ifdef __cplusplus
extern "C" {
#endif

#define Expense_Creator makelong("exps")
#define Expense_DB "ExpenseDB"

	extern char *ExpenseSortNames[];
	extern char *ExpenseDistanceNames[];
	extern char *ExpensePaymentNames[];
	extern char *ExpenseTypeNames[];

	enum ExpenseSort {
		esDate, esType
	};

	enum ExpenseDistance {
		edMiles, edKilometers
	};

	enum ExpensePayment {
		epAmEx, epCash, epCheck, epCreditCard, epMasterCard,
		epPrepaid, epVISA,
		epUnfiled
	};

	enum ExpenseType {
		etAirfare, etBreakfast, etBus, etBusinessMeals,
		etCarRental, etDinner,
		etEntertainment, etFax, etGas, etGifts, etHotel,
		etIncidentals,
		etLaundry,
		etLimo, etLodging, etLunch, etMileage, etOther, etParking,
		etPostage,
		etSnack, etSubway, etSupplies, etTaxi, etTelephone, etTips,
		etTolls,
		etTrain
	};

	struct ExpenseCustomCurrency {
		char 	name[16],
			symbol[4],
			rate[8];
	};

	struct Expense {
		int 	currency;
		char 	*amount,
			*vendor,
			*city,
			*attendees,
			*note;
		struct 	tm date;
		enum 	ExpenseType type;
		enum 	ExpensePayment payment;
	};

	struct ExpenseAppInfo {
		struct 	CategoryAppInfo category;
		enum 	ExpenseSort sortOrder;
		struct 	ExpenseCustomCurrency currencies[4];
	};

#define Expense_Pref 1

	struct ExpensePref {
		int 	currentCategory,
			defaultCurrency,
			attendeeFont,
			showAllCategories,
			showCurrency,
			saveBackup,
			allowQuickFill;
		enum 	ExpenseDistance unitOfDistance;
		int     currencies[5],
			unknown[2],
			noteFont;

	};

	extern void free_Expense PI_ARGS((struct Expense *));
	extern int unpack_Expense
		PI_ARGS((struct Expense *, unsigned char *record, int len));
	extern int pack_Expense
		PI_ARGS((struct Expense *, unsigned char *record, int len));
	extern int unpack_ExpensePref
		PI_ARGS((struct ExpensePref *, unsigned char *record,
			int len));
	extern int pack_ExpensePref
		PI_ARGS((struct ExpensePref *, unsigned char *record,
			int len));
	extern int unpack_ExpenseAppInfo
		PI_ARGS((struct ExpenseAppInfo *, unsigned char *AppInfo,
			int len));
	extern int pack_ExpenseAppInfo
		PI_ARGS((struct ExpenseAppInfo *, unsigned char *AppInfo,
			int len));

#ifdef __cplusplus
}
#endif
#endif				/* _PILOT_EXPENSE_H_ */
