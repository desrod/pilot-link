#ifndef _PILOT_MAIL_H_
#define _PILOT_MAIL_H_

#include "pi-args.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Mail {
  int read;
  int signature;
  int confirmRead;
  int confirmDelivery;
  int priority;
  int addressing;
  
  int dated;
  struct tm date;
  
  char * subject;
  char * from;
  char * to;
  char * cc;
  char * bcc;
  char * replyTo;
  char * sentTo;
  char * body;
};

struct MailAppInfo {
  unsigned int renamedcategories; /* Bitfield of categories with changed names */
  char CategoryName[16][16]; /* 16 categories of 15 characters+nul each */
  unsigned char CategoryID[16]; 
  unsigned char lastUniqueID; /* Each category gets a unique ID, for sync tracking
                                 purposes. Those from the Pilot are between 0 & 127.
                                 Those from the PC are between 128 & 255. I'm not
                                 sure what role lastUniqueID plays. */
  unsigned long dirtyfieldlabels; /* bitfield of same */
  int sortOrder;
  unsigned long unsentMessage; /* UniqueID of unsent message */
  
  char * signature;
};

struct MailPref1 {
  int synctype;
  int gethigh;
  int getcontaining;
  int truncate;
  char * filterto;
  char * filterfrom;
  char * filtersubject;
};

enum MailSyncType { mailSyncAll, mailSyncSend, mailSyncFilter };

extern void free_Mail PI_ARGS((struct Mail *));
extern void free_MailAppInfo PI_ARGS((struct MailAppInfo *));
extern void free_MailPref1 PI_ARGS((struct MailPref1 *));
extern void unpack_Mail PI_ARGS((struct Mail *, unsigned char * record, int len));
extern void pack_Mail PI_ARGS((struct Mail *, unsigned char * record, int * len));
extern void unpack_MailAppInfo PI_ARGS((struct MailAppInfo *, unsigned char * AppInfo, int len));
extern void pack_MailAppInfo PI_ARGS((struct MailAppInfo *, unsigned char * AppInfo, int * len));
extern void unpack_MailPref1 PI_ARGS((struct MailPref1 *, unsigned char * record, int len));

#ifdef __cplusplus
}
#endif

#endif /* _PILOT_MAIL_H_ */
