#ifndef _IAMBICEXPENSE_H        /* -*- C++ -*- */
#define _IAMBICEXPENSE_H

#include "pi-args.h"
#include "pi-appinfo.h"

#ifdef __cplusplus
extern "C" {
#endif

        struct iambicExpense {
                struct tm date;
                double amount;
                double milesStart, milesEnd;

                int currency;
                char *type;
                char *paidBy;
                char *payee;
                char *note;
        };

        struct iambicExpenseAppInfo {
                struct CategoryAppInfo category;
        };

        extern void free_iambicExpense PI_ARGS((struct iambicExpense *));
        extern int unpack_iambicExpense
            PI_ARGS((struct iambicExpense *, unsigned char *record,
                     int len));
        extern int pack_iambicExpense
            PI_ARGS((struct iambicExpense *, unsigned char *record,
                     int len));
        extern int unpack_iambicExpenseAppInfo
            PI_ARGS((struct iambicExpenseAppInfo *, unsigned char *AppInfo,
                     int len));
        extern int pack_iambicExpenseAppInfo
            PI_ARGS((struct iambicExpenseAppInfo *, unsigned char *AppInfo,
                     int len));

#ifdef __cplusplus
}
# include "pi-iambicExpense.hxx"
#endif
#endif                          /* _IAMBICEXPENSE_H */
