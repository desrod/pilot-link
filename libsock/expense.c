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

void free_Expense(struct Expense * a) {
  if (a->note)
    free(a->note);
  if (a->amount)
    free(a->amount);
  if (a->city)
    free(a->city);
  if (a->vendor)
    free(a->vendor);
  if (a->attendees)
    free(a->attendees);
}

void unpack_Expense(struct Expense * a, unsigned char * buffer, int len)
{
  unsigned long d;
                                                                                                                                                                                                            
  d = (unsigned short int)get_short(buffer);
  a->date.tm_year = (d >> 9) + 4;
  a->date.tm_mon = ((d >> 5) & 15) - 1;
  a->date.tm_mday = d & 31;
  a->date.tm_hour = 0;
  a->date.tm_min = 0;
  a->date.tm_sec = 0;
  a->date.tm_isdst = -1;
  mktime(&a->date);
  
  a->type = get_byte(buffer+2);
  a->payment = get_byte(buffer+3);
  a->currency = get_byte(buffer+4);
  
  buffer += 6;
  
  if (*buffer) {
    a->amount = strdup(buffer);
    buffer += strlen(a->amount);
  } else {
    a->amount = 0;
  }
  buffer++;

  if (*buffer) {
    a->vendor = strdup(buffer);
    buffer += strlen(a->vendor);
  } else {
    a->vendor = 0;
  }
  buffer++;

  if (*buffer) {
    a->city = strdup(buffer);
    buffer += strlen(a->city);
  } else {
    a->city = 0;
  }
  buffer++;


  if (*buffer) {
    a->attendees = strdup(buffer);
    buffer += strlen(a->attendees);
  } else {
    a->attendees = 0;
  }
  buffer++;

  if (*buffer) {
    a->note = strdup(buffer);
    buffer += strlen(a->note);
  } else {
    a->note = 0;
  }
  buffer++;
}

void pack_Expense(struct Expense * a, unsigned char * record, int * len)
{
  unsigned char * buf = record;
  
  set_short(buf, ((a->date.tm_year - 4) << 9) |
                   ((a->date.tm_mon  + 1) << 5) |
                   a->date.tm_mday);
  buf += 2;
  set_byte(buf, a->type);
  set_byte(buf+1, a->payment);
  set_byte(buf+2, a->currency);
  set_byte(buf+3, 0); /*gapfil*/ 
  buf += 4;
  
  if (a->amount) {
    strcpy(buf, a->amount);
    buf += strlen(buf);
  } else {
    set_byte(buf, 0);
  }
  buf++;

  if (a->vendor) {
    strcpy(buf, a->vendor);
    buf += strlen(buf);
  } else {
    set_byte(buf, 0);
  }
  buf++;
  
  if (a->city) {
    strcpy(buf, a->city);
    buf += strlen(buf);
  } else {
    set_byte(buf, 0);
  }
  buf++;
  
  if (a->attendees) {
    strcpy(buf, a->attendees);
    buf += strlen(buf);
  } else {
    set_byte(buf, 0);
  }
  buf++;
  
  if (a->note) {
    strcpy(buf, a->note);
    buf += strlen(buf);
  } else {
    set_byte(buf, 0);
  }
  buf++;
  
  if (len)
    *len = (buf-record);
}

void unpack_ExpenseAppInfo(struct ExpenseAppInfo * ai, unsigned char * record, int len)
{
  int i;
  ai->renamedcategories = get_short(record);
  record+=2;
  for(i=0;i<16;i++) {
    memcpy(ai->CategoryName[i], record, 16);
    record += 16;
  }
  memcpy(ai->CategoryID, record, 16);
  record += 16;
  ai->lastUniqueID = get_byte(record);
  record += 4;
  ai->sortOrder = get_byte(record);
  record += 2;
  for(i=0;i<4;i++) {
    memcpy(ai->currencies[i].name, record, 16);
    record+=16;
    memcpy(ai->currencies[i].symbol, record, 4);
    record+=4;
    memcpy(ai->currencies[i].rate, record, 8);
    record+=8;
  }
}

void pack_ExpenseAppInfo(struct ExpenseAppInfo * ai, unsigned char * record, int * len)
{
  unsigned char * start = record;
  int i;
  set_short(record, ai->renamedcategories);
  record += 2;
  for(i=0;i<16;i++) {
    memcpy(record, ai->CategoryName[i], 16);
    record += 16;
  }
  memcpy(record, ai->CategoryID, 16);
  record += 16;
  set_long(record, 0); /* gapfil */
  set_byte(record, ai->lastUniqueID);
  record += 4;
  set_byte(record, ai->sortOrder);
  record += 2;
  for(i=0;i<4;i++) {
    memcpy(record, ai->currencies[i].name, 16);
    record+=16;
    memcpy(record, ai->currencies[i].symbol, 4);
    record+=4;
    memcpy(record, ai->currencies[i].rate, 8);
    record+=8;
  }
  if (len)
    *len = (record-start);
}

void unpack_ExpensePrefs(struct ExpensePrefs * p, unsigned char * record, int len)
{
  int i;
  p->currentCategory = get_short(record);
  record += 2;
  p->defaultCategory = get_short(record);
  record += 2;
  p->noteFont = get_byte(record);
  record++;
  p->showAllCategories = get_byte(record);
  record++;
  p->showCurrency = get_byte(record);
  record++;
  p->saveBackup = get_byte(record);
  record++;
  p->allowQuickFill = get_byte(record);
  record++;
  p->unitOfDistance = get_byte(record);
  record += 2;
  for(i=0;i<7;i++) {
    p->currencies[i] = get_byte(record);
    record++;
  }
}

void pack_ExpensePrefs(struct ExpensePrefs * p, unsigned char * record, int * len)
{
  int i;
  unsigned char * start = record;
  set_short(record, p->currentCategory);
  record += 2;
  set_short(record, p->defaultCategory);
  record += 2;
  set_byte(record, p->noteFont);
  record++;
  set_short(record, p->showAllCategories);
  record++;
  set_byte(record, p->showCurrency);
  record++;
  set_byte(record, p->saveBackup);
  record++;
  set_byte(record, p->allowQuickFill);
  record++;
  set_byte(record, p->unitOfDistance);
  record++;
  set_byte(record, 0); /* gapfil ?? */
  record++;
  for(i=0;i<7;i++) {
    set_byte(record, p->currencies[i]);
    record++;
  }
  if (len)
    *len = record-start;
}
