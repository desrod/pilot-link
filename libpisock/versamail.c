/* -*- Mode: C; c-basic-indent: 4; indent-tabs-mode: nil -*-
 * versamail.c:  Translate VersaMail data formats
 *
 * Copyright (c) 2004, Nick Piper
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

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include "pi-macros.h"
#include "pi-versamail.h"


/*

Todo: What is rfu1 and 2?
      What is reserved 1 and 2?
      What is download ? (It's non zero, when there is more to d/l,
                          and appears to be an ID, allocated
                          by VersaMail. Maybe an index into another db?)
      How do we do attachments ? They are not kept in-line, but in another db.
      Why is mark = 2, when there is no mark set, and 3 when it is?

*/

void free_VersaMail(struct VersaMail *a) {
  if (a->messageUID) free(a->messageUID);
  if (a->to) free(a->to);
  if (a->from) free(a->from);
  if (a->cc) free(a->cc);
  if (a->bcc) free(a->bcc);
  if (a->subject) free(a->subject);
  if (a->dateString) free(a->dateString);
  if (a->body) free(a->body);
  if (a->replyTo) free(a->replyTo);
}



#define SHUFFLE_BUFFER(shuffle_distance) {          \
    len    -= shuffle_distance;                     \
    buffer += shuffle_distance;                     \
  }

#define GET_BYTE_INTO(attrib) {                                  \
    attrib = get_byte(buffer);                                   \
    SHUFFLE_BUFFER(1);                                           \
  }

#define GET_SHORT_INTO(attrib) {                                 \
    attrib = (unsigned int) get_short(buffer);                   \
    SHUFFLE_BUFFER(2);                                           \
  }

#define GET_STR_INTO(attrib) {                  \
    if (get_byte(buffer)) {                     \
      attrib = strdup(buffer);                  \
      SHUFFLE_BUFFER(strlen(buffer));           \
    } else {                                    \
      attrib = NULL;                            \
    };                                          \
    buffer++;                                   \
    len--;                                      \
  }

#define PUT_STR_FROM(attrib) {                  \
    if (attrib) {                               \
      strcpy(buffer, attrib);                   \
      SHUFFLE_BUFFER(strlen(buffer));           \
    } else                                      \
      set_byte(buffer, 0);                      \
    buffer++;                                   \
    len--;                                      \
  }


#define ADD_LENGTH(str, destlen) {              \
    if (str) {                                  \
      destlen += 1+strlen(str);                 \
    } else {                                    \
      destlen++;                                \
    };                                          \
  }                                             \


#define CONVERT_TIME_T_PALM_TO_UNIX(t) (t=t-2082844800)
#define CONVERT_TIME_T_UNIX_TO_PALM(t) (t=t+2082844800)

int unpack_VersaMail(struct VersaMail *a, unsigned char *buffer, int len) {
  unsigned long d;
  time_t date_t;
  struct tm *date_tm;
  unsigned char *start = buffer;

  a->imapuid = (unsigned long) get_long(buffer);
  SHUFFLE_BUFFER(4);

  /* This is different to the other databases, because Palm didn't
     write VersaMail :-) */
  date_t = (time_t) get_long(buffer);
  CONVERT_TIME_T_PALM_TO_UNIX(date_t);
  date_tm = localtime(&date_t);
  memcpy(&(a->date), date_tm, sizeof(struct tm));
  SHUFFLE_BUFFER(4);

  GET_SHORT_INTO(a->category);
  GET_SHORT_INTO(a->accountNo);
  GET_SHORT_INTO(a->rfu1);
  GET_BYTE_INTO(a->download);

  a->mark = (get_byte(buffer));
  SHUFFLE_BUFFER(1);

  GET_SHORT_INTO(a->rfu2);

  a->reserved1 = (get_byte(buffer  )     );
  a->reserved2 = (get_byte(buffer+1) >> 1);
  a->read      = (get_byte(buffer+1) && 1);
  SHUFFLE_BUFFER(2);

  /* This is the size, as provided by the imap server:
     * 1 FETCH (UID 12779 BODY[] {3377} .... )
     Size==3377
     I guess VersaMail uses it to determine how much
     more there might be to download. I don't think it uses it
     to know there IS more to download though, because
     the palm doesn't track most of the headers, so it'll never
     be able to calculate if there is more or not, exactly.
  */

  a->msgSize = get_long(buffer);
  SHUFFLE_BUFFER(4);

  GET_STR_INTO(a->messageUID);
  GET_STR_INTO(a->to);
  GET_STR_INTO(a->from);
  GET_STR_INTO(a->cc);
  GET_STR_INTO(a->bcc);
  GET_STR_INTO(a->subject);
  GET_STR_INTO(a->dateString);
  GET_STR_INTO(a->body);
  GET_STR_INTO(a->replyTo);

  return (buffer - start);
}

int pack_VersaMail(struct VersaMail *a, unsigned char *buffer, int len) {
  time_t date_t;
  unsigned int destlen;
  unsigned char *start = buffer;

  destlen = 4+4+2+2+2+2+2+2+4;

  ADD_LENGTH(a->messageUID, destlen);
  ADD_LENGTH(a->to, destlen);
  ADD_LENGTH(a->from, destlen);
  ADD_LENGTH(a->cc, destlen);
  ADD_LENGTH(a->bcc, destlen);
  ADD_LENGTH(a->subject, destlen);
  ADD_LENGTH(a->dateString, destlen);
  ADD_LENGTH(a->body, destlen);
  ADD_LENGTH(a->replyTo, destlen);

  if (!buffer)
    return destlen;
  if (len < destlen)
    return 0;

  set_long(buffer, a->imapuid);
  SHUFFLE_BUFFER(4);
  
  date_t = mktime(&(a->date));
  CONVERT_TIME_T_UNIX_TO_PALM(date_t);
  set_long(buffer, (unsigned long) date_t);
  SHUFFLE_BUFFER(4);

  set_short(buffer, a->category);
  SHUFFLE_BUFFER(2);
  set_short(buffer, a->accountNo);
  SHUFFLE_BUFFER(2);
  set_short(buffer, a->rfu1);
  SHUFFLE_BUFFER(2);
  set_byte(buffer, a->download);
  SHUFFLE_BUFFER(1);
  set_byte(buffer, a->mark);
  SHUFFLE_BUFFER(1);
  set_short(buffer, a->rfu2);
  SHUFFLE_BUFFER(2);

  set_byte(buffer, a->reserved1);
  set_byte(buffer+1, ((a->reserved2 << 1) || a->read));
  SHUFFLE_BUFFER(2);

  set_long(buffer, a->msgSize);
  SHUFFLE_BUFFER(4);

  PUT_STR_FROM(a->messageUID);
  PUT_STR_FROM(a->to);
  PUT_STR_FROM(a->from);
  PUT_STR_FROM(a->cc);
  PUT_STR_FROM(a->bcc);
  PUT_STR_FROM(a->subject);
  PUT_STR_FROM(a->dateString);
  PUT_STR_FROM(a->body);
  PUT_STR_FROM(a->replyTo);  

  return (buffer - start);
}

/* 

Message provided to the maintainer of pilot-mailsync:
(the above implementation does not fully agree, BTW)

-------- Original Message --------
Subject: Re: QueueSync2.0
Date: Sat, 31 Jan 2004 03:19:28 +0900
From: Masaru matsumoto <matumoto@queuesoft.jp>
To: Jochen Garcke <jochen@garcke.de>
References: <401A7AE7.5090109@garcke.de>
   
Hi,

The definition of the database is to refer to the following.
But, because I analyzed it, it has the possibility to be wrong.
Write only information on VersaMail. Because there are many problems in
ClieMail.


typedef struct
{
        UInt16                          mu_reserved1                    : 8;
        UInt16                          mu_reserved2                    : 7;
        UInt16                          mu_read                         : 1;
} mu_MailFlagsType;

typedef struct {
      UInt32                          mu_version;
      UInt32                          mu_datetime;
      UInt16                          mu_category;
      UInt16                          mu_account_no;
      UInt16                          mu_rfu1;
      UInt16                          mu_mark;
      UInt16                          mu_rfu2;
      mu_MailFlagsType                mu_flags;
      UInt32                          mu_msgSize;
        char                                    mu_firstField;
} mu_MailPackedDBRecordType;

typedef mu_MailPackedDBRecordType * mu_MailPackedDBRecordPtr;


mu_msgSize =
strlen(subject)+strlen(contents)+strlen(dateString)+strlen(replyto)+4;

Field sequence :
messageUID
to  
from
cc
bcc
subject   
dateString
body
replayTo

*/
