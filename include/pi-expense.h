#ifndef _PILOT_EXPENSE_H_
#define _PILOT_EXPENSE_H_

#include <time.h>
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

	typedef struct ExpenseCustomCurrency {
		char name[16];
		char symbol[4];
		char rate[8];
	} ExpenseCustomCurrency_t;

	typedef struct Expense {
		struct tm date;
		enum ExpenseType type;
		enum ExpensePayment payment;
		int currency;
		char *amount;
		char *vendor;
		char *city;
		char *attendees;
		char *note;
	} Expense_t;

	typedef struct ExpenseAppInfo {
		struct CategoryAppInfo category;
		enum ExpenseSort sortOrder;
		struct ExpenseCustomCurrency currencies[4];
	} ExpenseAppInfo_t;

#define Expense_Pref 1

	typedef struct ExpensePref {
		int currentCategory;
		int defaultCurrency;
		int attendeeFont;
		int showAllCategories;
		int showCurrency;
		int saveBackup;
		int allowQuickFill;
		enum ExpenseDistance unitOfDistance;
		int currencies[5];
		int unknown[2];
		int noteFont;
	} ExpensePref_t;

	extern void free_Expense
	  PI_ARGS((struct Expense *));
	extern int unpack_Expense
	  PI_ARGS((struct Expense *, unsigned char *record, int len));
	extern int pack_Expense
	  PI_ARGS((struct Expense *, unsigned char *record, int len));
	extern int unpack_ExpensePref
	  PI_ARGS((struct ExpensePref *, unsigned char *record, int len));
	extern int pack_ExpensePref
	  PI_ARGS((struct ExpensePref *, unsigned char *record, int len));
	extern int unpack_ExpenseAppInfo
	  PI_ARGS((struct ExpenseAppInfo *, unsigned char *AppInfo, int len));
	extern int pack_ExpenseAppInfo
	  PI_ARGS((struct ExpenseAppInfo *, unsigned char *AppInfo, int len));

#ifdef __cplusplus
}
#endif
#endif				/* _PILOT_EXPENSE_H_ */
