/* expense.c:  Translate Pilot expense tracker data formats
 *
 * Copyright (c) 1997, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-expense.h"
                        
void free_Expense(struct Expense * a);

void unpack_Expense(struct Expense * a, unsigned char * buffer, int len);

void pack_Expense(struct Expense * a, unsigned char * record, int * len);

void unpack_ExpenseAppInfo(struct ExpenseAppInfo * ai, unsigned char * record, int len);

void pack_ExpenseAppInfo(struct ExpenseAppInfo * ai, unsigned char * record, int * len);
