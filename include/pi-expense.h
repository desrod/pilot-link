#ifndef _PILOT_EXPENSE_H_
#define _PILOT_EXPENSE_H_

struct Expense;

struct ExpenseAppInfo;

extern void free_Expense(struct Expense *);
extern void unpack_Expense(struct Expense *, unsigned char * record, int len);
extern void pack_Expense(struct Expense *, unsigned char * record, int * len);
extern void unpack_ExpenseAppInfo(struct ExpenseAppInfo *, unsigned char * AppInfo, int len);
extern void pack_ExpenseAppInfo(struct ExpenseAppInfo *, unsigned char * AppInfo, int * len);

#endif /* _PILOT_EXPENSE_H_ */
