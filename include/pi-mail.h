#ifndef _PILOT_MAIL_H_
#define _PILOT_MAIL_H_

struct Mail {
  int read;
  int signature;
  int confirmRead;
  int confirmDelivery;
  int priority;
  int addressing;
  
  time_t time;
  
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
  unsigned long unsent; /* UniqueID of unsent message */
};

extern void free_Mail(struct Mail *);
extern void unpack_Mail(struct Mail *, unsigned char * record, int len);
extern void pack_Mail(struct Mail *, unsigned char * record, int * len);
extern void unpack_MailAppInfo(struct MailAppInfo *, unsigned char * AppInfo, int len);
extern void pack_MailAppInfo(struct MailAppInfo *, unsigned char * AppInfo, int * len);

#endif /* _PILOT_MAIL_H_ */
