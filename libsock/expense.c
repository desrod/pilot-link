/*
 * expense.c:  Translate Pilot expense tracker data formats
 *
 * Copyright (c) 1997, Kenneth Albanowski
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-expense.h"

char *ExpenseSortNames[] = { "Date", "Type", NULL };
char *ExpenseDistanceNames[] = { "Miles", "Kilometers", NULL };
char *ExpensePaymentNames[] =
    { "AmEx", "Cash", "Check", "CreditCard", "MasterCard", "Prepaid",
"VISA",
	"Unfiled"
};
char *ExpenseTypeNames[] =
    { "Airfare", "Breakfast", "Bus", "Business Meals", "Car Rental", "Dinner",
	"Entertainment", "Fax", "Gas", "Gifts", "Hotel", "Incidentals", "Laundry",
	"Limo", "Lodging", "Lunch", "Mileage", "Other", "Parking", "Postage",
	"Snack", "Subway", "Supplies", "Taxi", "Telephone", "Tips", "Tolls", "Train"
};

/***********************************************************************
 *
 * Function:    free_Expense
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void free_Expense(struct Expense *a)
{
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

/***********************************************************************
 *
 * Function:    unpack_Expense
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int unpack_Expense(struct Expense *a, unsigned char *buffer, int len)
{
	unsigned long d;
	unsigned char *start = buffer;

	if (len < 6)
		return 0;

	d = (unsigned short int) get_short(buffer);
	a->date.tm_year = (d >> 9) + 4;
	a->date.tm_mon = ((d >> 5) & 15) - 1;
	a->date.tm_mday = d & 31;
	a->date.tm_hour = 0;
	a->date.tm_min = 0;
	a->date.tm_sec = 0;
	a->date.tm_isdst = -1;
	mktime(&a->date);

	a->type = (enum ExpenseType) get_byte(buffer + 2);
	a->payment = (enum ExpensePayment) get_byte(buffer + 3);
	a->currency = get_byte(buffer + 4);

	buffer += 6;
	len -= 6;

	if (len < 1)
		return 0;

	if (*buffer) {
		a->amount = strdup(buffer);
		buffer += strlen(a->amount);
		len -= strlen(a->amount);
	} else {
		a->amount = 0;
	}
	buffer++;
	len--;

	if (len < 1)
		return 0;

	if (*buffer) {
		a->vendor = strdup(buffer);
		buffer += strlen(a->vendor);
		len -= strlen(a->vendor);
	} else {
		a->vendor = 0;
	}
	buffer++;
	len--;

	if (len < 1)
		return 0;

	if (*buffer) {
		a->city = strdup(buffer);
		buffer += strlen(a->city);
		len -= strlen(a->city);
	} else {
		a->city = 0;
	}
	buffer++;
	len--;

	if (len < 1)
		return 0;

	if (*buffer) {
		a->attendees = strdup(buffer);
		buffer += strlen(a->attendees);
		len -= strlen(a->attendees);
	} else {
		a->attendees = 0;
	}
	buffer++;
	len--;

	if (len < 1)
		return 0;

	if (*buffer) {
		a->note = strdup(buffer);
		buffer += strlen(a->note);
		len -= strlen(a->note);
	} else {
		a->note = 0;
	}

	buffer++;
	len--;

	return (buffer - start);
}

/***********************************************************************
 *
 * Function:    pack_Expense
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pack_Expense(struct Expense *a, unsigned char *record, int len)
{
	unsigned char *buf = record;
	int destlen = 6 + 1 + 1 + 1 + 1 + 1;

	if (a->amount)
		destlen += strlen(a->amount);
	if (a->vendor)
		destlen += strlen(a->vendor);
	if (a->city)
		destlen += strlen(a->city);
	if (a->attendees)
		destlen += strlen(a->attendees);
	if (a->note)
		destlen += strlen(a->note);

	if (!record)
		return destlen;
	if (len < destlen)
		return 0;

	set_short(buf,
		  ((a->date.tm_year - 4) << 9) | ((a->date.tm_mon +
						   1) << 5) | a->date.
		  tm_mday);
	buf += 2;
	set_byte(buf, a->type);
	set_byte(buf + 1, a->payment);
	set_byte(buf + 2, a->currency);
	set_byte(buf + 3, 0);	/* gapfill */
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

	return (buf - record);
}

/***********************************************************************
 *
 * Function:    unpack_ExpenseAppInfo
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
unpack_ExpenseAppInfo(struct ExpenseAppInfo *ai, unsigned char *record,
		      int len)
{
	int i;
	unsigned char *start = record;

	i = unpack_CategoryAppInfo(&ai->category, record, len);
	if (!i)
		return 0;
	record += i;
	len -= i;
	if (len < 2 + (16 + 4 + 8) * 4);
	ai->sortOrder = (enum ExpenseSort) get_byte(record);
	record += 2;
	for (i = 0; i < 4; i++) {
		memcpy(ai->currencies[i].name, record, 16);
		record += 16;
		memcpy(ai->currencies[i].symbol, record, 4);
		record += 4;
		memcpy(ai->currencies[i].rate, record, 8);
		record += 8;
	}
	return (record - start);
}

/***********************************************************************
 *
 * Function:    pack_ExpenseAppInfo
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pack_ExpenseAppInfo(struct ExpenseAppInfo *ai, unsigned char *record,
		    int len)
{
	unsigned char *start = record;
	int i;
	int destlen = 2 + (16 + 4 + 8) * 4;

	i = pack_CategoryAppInfo(&ai->category, record, len);
	if (!record)
		return i + destlen;
	if (!i)
		return i;
	record += i;
	len -= i;
	if (len < destlen)
		return 0;
	set_byte(record, ai->sortOrder);
	set_byte(record + 1, 0);	/* gapfill */
	record += 2;
	for (i = 0; i < 4; i++) {
		memcpy(record, ai->currencies[i].name, 16);
		record += 16;
		memcpy(record, ai->currencies[i].symbol, 4);
		record += 4;
		memcpy(record, ai->currencies[i].rate, 8);
		record += 8;
	}

	return (record - start);
}

/***********************************************************************
 *
 * Function:    unpack_ExpensePref
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
unpack_ExpensePref(struct ExpensePref *p, unsigned char *record, int len)
{
	int i;
	unsigned char *start = record;

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
	p->unitOfDistance = (enum ExpenseDistance) get_byte(record);
	record += 2;
	for (i = 0; i < 7; i++) {
		p->currencies[i] = get_byte(record);
		record++;
	}
	return (record - start);
}

/***********************************************************************
 *
 * Function:    pack_ExpensePref
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pack_ExpensePref(struct ExpensePref *p, unsigned char *record, int len)
{
	int i;
	unsigned char *start = record;

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
	set_byte(record, 0);	/* gapfill ?? */
	record++;
	for (i = 0; i < 7; i++) {
		set_byte(record, p->currencies[i]);
		record++;
	}
	return record - start;
}
