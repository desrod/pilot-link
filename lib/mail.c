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

void unpack_Mail(struct Mail * a, unsigned char * buffer, int len);

void pack_Mail(struct Mail * a, unsigned char * record, int * len);

void unpack_MailAppInfo(struct MailAppInfo * ai, unsigned char * record, int len);

void pack_MailAppInfo(struct MailAppInfo * ai, unsigned char * record, int * len);
