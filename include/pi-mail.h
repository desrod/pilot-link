#ifndef _PILOT_MAIL_H_
#define _PILOT_MAIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/time.h>

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

struct MailPrefs {
  int synctype;
  int gethigh;
  int getcontaining;
  int truncate;
  char * filterto;
  char * filterfrom;
  char * filtersubject;
};

enum { mailSyncAll, mailSyncSend, mailSyncFilter } MailSyncType;

extern void free_Mail(struct Mail *);
extern void free_MailAppInfo(struct MailAppInfo *);
extern void free_MailPrefs(struct MailPrefs *);
extern void unpack_Mail(struct Mail *, unsigned char * record, int len);
extern void pack_Mail(struct Mail *, unsigned char * record, int * len);
extern void unpack_MailAppInfo(struct MailAppInfo *, unsigned char * AppInfo, int len);
extern void pack_MailAppInfo(struct MailAppInfo *, unsigned char * AppInfo, int * len);
extern void unpack_MailPrefs(struct MailPrefs *, unsigned char * record, int len);

#ifdef __cplusplus
}
#endif

#endif /* _PILOT_MAIL_H_ */
