/*
 * mail.c:  Translate Pilot mail data formats
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
#include "pi-mail.h"

char *MailSortTypeNames[] = { "Date", "Type", NULL };
char *MailSyncTypeNames[] = { "All", "Send", "Filter", NULL };

/***********************************************************************
 *
 * Function:    free_Mail
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void free_Mail(struct Mail *a)
{
	if (a->from)
		free(a->from);
	if (a->to)
		free(a->to);
	if (a->subject)
		free(a->subject);
	if (a->cc)
		free(a->cc);
	if (a->bcc)
		free(a->bcc);
	if (a->replyTo)
		free(a->replyTo);
	if (a->sentTo)
		free(a->sentTo);
	if (a->body)
		free(a->body);
}

/***********************************************************************
 *
 * Function:    free_MailAppInfo
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void free_MailAppInfo(struct MailAppInfo *a)
{
	/* if (a->signature)
	   free(a->signature); */
}

/***********************************************************************
 *
 * Function:    free_MailSyncPref
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void free_MailSyncPref(struct MailSyncPref *a)
{
	if (a->filterTo);
	free(a->filterTo);
	if (a->filterFrom);
	free(a->filterFrom);
	if (a->filterSubject);
	free(a->filterSubject);
}

/***********************************************************************
 *
 * Function:    free_MailSignaturePref
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void free_MailSignaturePref(struct MailSignaturePref *a)
{
	if (a->signature);
	free(a->signature);
}

/***********************************************************************
 *
 * Function:    unpack_Mail
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int unpack_Mail(struct Mail *a, unsigned char *buffer, int len)
{
	unsigned long d;
	int flags;
	unsigned char *start = buffer;

	if (len < 6)
		return 0;

	d = (unsigned short int) get_short(buffer);
	a->date.tm_year = (d >> 9) + 4;
	a->date.tm_mon = ((d >> 5) & 15) - 1;
	a->date.tm_mday = d & 31;
	a->date.tm_hour = get_byte(buffer + 2);
	a->date.tm_min = get_byte(buffer + 3);
	a->date.tm_sec = 0;
	a->date.tm_isdst = -1;
	mktime(&a->date);

	if (d)
		a->dated = 1;
	else
		a->dated = 0;

	flags = get_byte(buffer + 4);

	a->read = (flags & (1 << 7)) ? 1 : 0;
	a->signature = (flags & (1 << 6)) ? 1 : 0;
	a->confirmRead = (flags & (1 << 5)) ? 1 : 0;
	a->confirmDelivery = (flags & (1 << 4)) ? 1 : 0;
	a->priority = (flags & (3 << 2)) >> 2;
	a->addressing = (flags & 3);

	buffer += 6;
	len -= 6;

	if (len < 1)
		return 0;
	if (get_byte(buffer)) {
		a->subject = strdup(buffer);
		buffer += strlen(buffer);
		len -= strlen(buffer);
	} else
		a->subject = 0;
	buffer++;
	len--;
	if (len < 1)
		return 0;
	if (get_byte(buffer)) {
		a->from = strdup(buffer);
		buffer += strlen(buffer);
		len -= strlen(buffer);
	} else
		a->from = 0;
	buffer++;
	len--;
	if (len < 1)
		return 0;
	if (get_byte(buffer)) {
		a->to = strdup(buffer);
		buffer += strlen(buffer);
		len -= strlen(buffer);
	} else
		a->to = 0;
	buffer++;
	len--;
	if (len < 1)
		return 0;
	if (get_byte(buffer)) {
		a->cc = strdup(buffer);
		buffer += strlen(buffer);
		len -= strlen(buffer);
	} else
		a->cc = 0;
	buffer++;
	len--;
	if (len < 1)
		return 0;
	if (get_byte(buffer)) {
		a->bcc = strdup(buffer);
		buffer += strlen(buffer);
		len -= strlen(buffer);
	} else
		a->bcc = 0;
	buffer++;
	len--;
	if (len < 1)
		return 0;
	if (get_byte(buffer)) {
		a->replyTo = strdup(buffer);
		buffer += strlen(buffer);
		len -= strlen(buffer);
	} else
		a->replyTo = 0;
	buffer++;
	len--;
	if (len < 1)
		return 0;
	if (get_byte(buffer)) {
		a->sentTo = strdup(buffer);
		buffer += strlen(buffer);
		len -= strlen(buffer);
	} else
		a->sentTo = 0;
	buffer++;
	len--;
	if (len < 1)
		return 0;
	if (get_byte(buffer)) {
		a->body = strdup(buffer);
		buffer += strlen(buffer);
		len -= strlen(buffer);
	} else
		a->body = 0;
	buffer++;
	len--;

	return (buffer - start);
}

/***********************************************************************
 *
 * Function:    pack_Mail
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pack_Mail(struct Mail *a, unsigned char *buffer, int len)
{
	unsigned char *start = buffer;
	int destlen = 6 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1;

	if (a->subject)
		destlen += strlen(a->subject);
	if (a->from)
		destlen += strlen(a->from);
	if (a->to)
		destlen += strlen(a->to);
	if (a->cc)
		destlen += strlen(a->cc);
	if (a->bcc)
		destlen += strlen(a->bcc);
	if (a->replyTo)
		destlen += strlen(a->replyTo);
	if (a->sentTo)
		destlen += strlen(a->sentTo);
	if (a->body)
		destlen += strlen(a->body);

	if (!buffer)
		return destlen;
	if (len < destlen)
		return 0;

	set_short(buffer,
		  ((a->date.tm_year - 4) << 9) | ((a->date.tm_mon +
						   1) << 5) | a->date.
		  tm_mday);
	set_byte(buffer + 2, a->date.tm_hour);
	set_byte(buffer + 3, a->date.tm_min);

	if (!a->dated)
		set_long(buffer, 0);

	set_byte(buffer + 4, (a->read ? (1 << 7) : 0) |
		 (a->signature ? (1 << 6) : 0) | (a->
						  confirmRead ? (1 << 5) :
						  0) | (a->
							confirmDelivery
							? (1 << 4) : 0) |
		 ((a->priority & 3) << 2) | (a->addressing & 3));
	set_byte(buffer + 5, 0);

	buffer += 6;

	if (a->subject) {
		strcpy(buffer, a->subject);
		buffer += strlen(buffer);
	} else
		set_byte(buffer, 0);
	buffer++;
	if (a->from) {
		strcpy(buffer, a->from);
		buffer += strlen(buffer);
	} else
		set_byte(buffer, 0);
	buffer++;
	if (a->to) {
		strcpy(buffer, a->to);
		buffer += strlen(buffer);
	} else
		set_byte(buffer, 0);
	buffer++;
	if (a->cc) {
		strcpy(buffer, a->cc);
		buffer += strlen(buffer);
	} else
		set_byte(buffer, 0);
	buffer++;
	if (a->bcc) {
		strcpy(buffer, a->bcc);
		buffer += strlen(buffer);
	} else
		set_byte(buffer, 0);
	buffer++;
	if (a->replyTo) {
		strcpy(buffer, a->replyTo);
		buffer += strlen(buffer);
	} else
		set_byte(buffer, 0);
	buffer++;
	if (a->sentTo) {
		strcpy(buffer, a->sentTo);
		buffer += strlen(buffer);
	} else
		set_byte(buffer, 0);
	buffer++;
	if (a->body) {
		strcpy(buffer, a->body);
		buffer += strlen(buffer);
	} else
		set_byte(buffer, 0);
	buffer++;

	return (buffer - start);
}

/***********************************************************************
 *
 * Function:    unpack_MailAppInfo
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
unpack_MailAppInfo(struct MailAppInfo *ai, unsigned char *record, int len)
{
	int i;
	unsigned char *start = record;

	i = unpack_CategoryAppInfo(&ai->category, record, len);
	if (!i)
		return i;
	record += i;
	len -= i;
	if (len < 11)
		return 0;
	ai->dirty = get_short(record);
	record += 2;
	ai->sortOrder = get_byte(record);
	record += 2;
	ai->unsentMessage = get_long(record);
	record += 4;

/* ai->signature = 0; 			*/
/* strdup(start + get_short(record)); 	*/
	record += 3;

	return (record - start);
}

/***********************************************************************
 *
 * Function:    pack_MailAppInfo
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pack_MailAppInfo(struct MailAppInfo *ai, unsigned char *record, int len)
{
	int i;
	unsigned char *start = record;

	i = pack_CategoryAppInfo(&ai->category, record, len);
	if (!record)
		return i + 11;
	if (!i)
		return i;
	record += i;
	len -= i;
	if (len < 8)
		return 0;
	set_short(record, ai->dirty);
	record += 2;
	set_short(record, 0);	/* gapfill */
	set_byte(record, ai->sortOrder);
	record += 2;
	set_long(record, ai->unsentMessage);
	record += 4;

	set_short(record, (record - start + 2));
	record += 2;

	/* if (ai->signature)
	   strcpy(record, ai->signature);
	   else
	   set_byte(record, 0);
	   record += strlen(record); */
	set_byte(record, 0);
	record++;

	return (record - start);
}

/***********************************************************************
 *
 * Function:    unpack_MailSyncPref
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
unpack_MailSyncPref(struct MailSyncPref *a, unsigned char *record, int len)
{
	unsigned char *start = record;

	a->syncType = get_byte(record);
	record += 1;
	a->getHigh = get_byte(record);
	record += 1;
	a->getContaining = get_byte(record);
	record += 2;
	a->truncate = get_short(record);
	record += 2;

	if (get_byte(record)) {
		a->filterTo = strdup(record);
		record += strlen(record);
	} else
		a->filterTo = 0;
	record++;
	if (get_byte(record)) {
		a->filterFrom = strdup(record);
		record += strlen(record);
	} else
		a->filterFrom = 0;
	record++;
	if (get_byte(record)) {
		a->filterSubject = strdup(record);
		record += strlen(record);
	} else
		a->filterSubject = 0;
	record++;

	return (record - start);
}

/***********************************************************************
 *
 * Function:    pack_MailSyncPref
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pack_MailSyncPref(struct MailSyncPref *ai, unsigned char *record, int len)
{
	unsigned char *start = record;
	int destlen = 6 + 1 + 1 + 1;

	if (ai->filterTo)
		destlen += strlen(ai->filterTo);
	if (ai->filterSubject)
		destlen += strlen(ai->filterSubject);
	if (ai->filterFrom)
		destlen += strlen(ai->filterFrom);

	if (!record)
		return destlen;
	if (len < destlen)
		return 0;

	set_byte(record, ai->syncType);
	record++;
	set_byte(record, ai->getHigh);
	record++;
	set_byte(record, ai->getContaining);
	record++;
	set_byte(record, 0);
	record++;		/* gapfill */
	set_short(record, ai->truncate);
	record += 2;

	if (ai->filterTo) {
		strcpy(record, ai->filterTo);
		record += strlen(ai->filterTo);
	}
	*record++ = 0;

	if (ai->filterFrom) {
		strcpy(record, ai->filterFrom);
		record += strlen(ai->filterFrom);
	}
	*record++ = 0;

	if (ai->filterSubject) {
		strcpy(record, ai->filterSubject);
		record += strlen(ai->filterSubject);
	}
	*record++ = 0;

	return (record - start);
}

/***********************************************************************
 *
 * Function:    unpack_MailSignaturePref
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
unpack_MailSignaturePref(struct MailSignaturePref *a,
			 unsigned char *record, int len)
{
	unsigned char *start = record;

	if (len < 1)
		return 0;

	a->signature = strdup(record);

	record += strlen(a->signature) + 1;

	return (record - start);
}

/***********************************************************************
 *
 * Function:    pack_MailSignaturePref
 *
 * Summary:     
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pack_MailSignaturePref(struct MailSignaturePref *ai, unsigned char *record,
		       int len)
{
	unsigned char *start = record;
	int destlen = 1;

	if (ai->signature)
		destlen += strlen(ai->signature);

	if (!record)
		return destlen;
	if (len < destlen)
		return 0;
	if (ai->signature) {
		strcpy(record, ai->signature);
		record += strlen(ai->signature);
	}
	*record = 0;
	record++;

	return (record - start);
}
