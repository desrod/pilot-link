/* mail.c:  Translate Pilot mail data formats
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
#include "pi-mail.h"

void free_Mail(struct Mail * a) {
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

void free_MailAppInfo(struct MailAppInfo * a) {
  if (a->signature)
    free(a->signature);
}

void free_MailPrefs(struct MailPrefs * a) {
  if (a->filterto);
    free(a->filterto);
  if (a->filterfrom);
    free(a->filterfrom);
  if (a->filtersubject);
    free(a->filtersubject);
}

void unpack_Mail(struct Mail * a, unsigned char * buffer, int len)
{
  unsigned long d;
  int flags;

  d = (unsigned short int)get_short(buffer);
  a->date.tm_year = (d >> 9) + 4;
  a->date.tm_mon = ((d >> 5) & 15) - 1;
  a->date.tm_mday = d & 31;
  a->date.tm_hour = get_byte(buffer+2);
  a->date.tm_min = get_byte(buffer+3);
  a->date.tm_sec = 0;
  a->date.tm_isdst = -1;
  mktime(&a->date);
  
  if (d)
    a->dated = 1;
  else
    a->dated = 0;
  
  flags = get_byte(buffer+4);
  
  a->read = (flags & (1 << 7)) ? 1 : 0;
  a->signature = (flags & (1 << 6)) ? 1 : 0;
  a->confirmRead = (flags & (1 << 5)) ? 1 : 0;
  a->confirmDelivery = (flags & (1 << 4)) ? 1 : 0;
  a->priority = (flags & (3 << 2)) >> 2;
  a->addressing = (flags & 3);
  
  buffer += 6;
  
  if (get_byte(buffer)) {
    a->subject = strdup(buffer);
    buffer += strlen(buffer);
  } else
    a->subject = 0;
  buffer++;
  if (get_byte(buffer)) {
    a->from = strdup(buffer);
    buffer += strlen(buffer);
  } else
    a->from = 0;
  buffer++;
  if (get_byte(buffer)) {
    a->to = strdup(buffer);
    buffer += strlen(buffer);
  } else
    a->to = 0;
  buffer++;
  if (get_byte(buffer)) {
    a->cc = strdup(buffer);
    buffer += strlen(buffer);
  } else
    a->cc = 0;
  buffer++;
  if (get_byte(buffer)) {
    a->bcc = strdup(buffer);
    buffer += strlen(buffer);
  } else
    a->bcc = 0;
  buffer++;
  if (get_byte(buffer)) {
    a->replyTo = strdup(buffer);
    buffer += strlen(buffer);
  } else
    a->replyTo = 0;
  buffer++;
  if (get_byte(buffer)) {
    a->sentTo = strdup(buffer);
    buffer += strlen(buffer);
  } else
    a->sentTo = 0;
  buffer++;
  if (get_byte(buffer)) {
    a->body = strdup(buffer);
    buffer += strlen(buffer);
  } else
    a->body = 0;
  buffer++;
}

void pack_Mail(struct Mail * a, unsigned char * buffer, int * len)
{
  unsigned char * start = buffer;

  set_short(buffer, ((a->date.tm_year - 4) << 9) |
                    ((a->date.tm_mon  + 1) << 5) |
                    a->date.tm_mday);
  set_byte(buffer+2, a->date.tm_hour);
  set_byte(buffer+3, a->date.tm_min);
  
  if (!a->dated)
    set_long(buffer, 0);
  
  set_byte(buffer+4, (a->read ? (1 << 7) : 0) |
                     (a->signature ? (1 << 6) : 0) |
                     (a->confirmRead ? (1 << 5) : 0) |
                     (a->confirmDelivery ? (1 << 4) : 0) |
                     ((a->priority & 3) << 2) |
                     (a->addressing & 3)
           );
  set_byte(buffer+5, 0);
  
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
  
  if (len)
    *len = buffer - start;
}


void unpack_MailAppInfo(struct MailAppInfo * ai, unsigned char * record, int len)
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
  ai->dirtyfieldlabels = get_short(record);
  record += 2;
  ai->sortOrder = get_byte(record);
  record += 2;
  ai->unsentMessage = get_long(record);
  record += 4;
  
  ai->signature = 0; /*strdup(start + get_short(record));*/
  
  return;
}

void pack_MailAppInfo(struct MailAppInfo * ai, unsigned char * record, int * len)
{
  int i;
  unsigned char * start = record;
  
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
  set_short(record, ai->dirtyfieldlabels);
  record += 2;
  set_short(record, 0); /* gapfil */
  set_byte(record, ai->sortOrder);
  record += 2;
  set_long(record, ai->unsentMessage);
  record += 4;
  
  set_short(record, (record-start+2));
  record += 2;
  
  if (ai->signature)
    strcpy(record, ai->signature);
  else
    set_byte(record, 0);
  record += strlen(record);

  if (len)
    *len = record-start;
}

void unpack_MailPrefs(struct MailPrefs * a, unsigned char * record, int len) {
  a->synctype = get_byte(record);
  record += 1;
  a->gethigh = get_byte(record);
  record += 1;
  a->getcontaining = get_byte(record);
  record += 2;
  a->truncate = get_short(record);
  record += 2;
  
  if (get_byte(record)) {
    a->filterto = strdup(record);
    record += strlen(record);
  } else
    a->filterto = 0;
  record++;
  if (get_byte(record)) {
    a->filterfrom = strdup(record);
    record += strlen(record);
  } else
    a->filterfrom = 0;
  record++;
  if (get_byte(record)) {
    a->filtersubject = strdup(record);
    record += strlen(record);
  } else
    a->filtersubject = 0;
  record++;  
}
