#ifndef _IAMBICEXPENSE_HXX	/* -*- C++ -*- */
#define _IAMBICEXPENSE_HXX

#include "pi-appinfo.hxx"

#define reimburse 0x01
#define receipt 0x02

const int IAMBIC_EXPENSE_APP_INFO_SIZE = 512;

class iambicExpenseList_t;		// Forward declaration

class iambicExpenseAppInfo_t : public appInfo_t 
{
	category_t _conversionNames;

public:
	iambicExpenseAppInfo_t(void *);
	
	const category_t *conversionNames(void);
	
	// You can't install an expense entry yet, but we have to provide this
	// function or the compiler will choke, as the parent defines this as
	// a pure virtual function.
	void *pack(void);
};

class iambicExpense_t : public baseApp_t
{
	friend iambicExpenseList_t;

	short _flags;
	char *_type;
	char *_paidby;
	char *_payee;
	char *_note;
	double _amount;
	double _milesStart, _milesEnd;
	double _exchangeRate;
	tm _date;
	
	iambicExpense_t *_next;
	
	// Will never get called, but we need the name
	void *internalPack(unsigned char *a);
	
public:
	iambicExpense_t(void);
	iambicExpense_t(void *buf);
	iambicExpense_t(void *buf, int attr, recordid_t id, int category);
	iambicExpense_t(const iambicExpense_t &);
     
	~iambicExpense_t();

	const char *type(void);
	const char *paidBy(void);
	const char *paidby(void);
	const char *payee(void);
	const char *note(void);
	double amount(void);
	double milesStart(void);
	double milesEnd(void);
	double exchangeRate(void);
	const tm *date(void);

	void unpack(void *);
	
	// We don't let you pack one of these, but we must provide the name
	void *pack(int *a);
	void *pack(void *a, int *b);
};

class iambicExpenseList_t 
{
	iambicExpense_t *_head;
     
public:
	iambicExpenseList_t(void) : _head(NULL) {}
	~iambicExpenseList_t();
	
	iambicExpense_t *first();
	iambicExpense_t *next(iambicExpense_t *ptr);
	
	void merge(iambicExpense_t &);
	void merge(iambicExpenseList_t &);
};

#endif // _IAMBICEXPENSE_HXX
