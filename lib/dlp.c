/* dlp.c:  Pilot DLP protocol
 *
 * (c) 1996, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include <sys/time.h>
#include <errno.h>

#include "pi-socket.h"
#include "dlp.h"

/* These functions are inefficient, but completely portable */

#define get_long(ptr) (((ptr)[0] << 24) | \
                       ((ptr)[1] << 16) | \
                       ((ptr)[2] << 8)  | \
                       ((ptr)[3] << 0))
                       
#define get_short(ptr) (((ptr)[0] << 8)  | \
                        ((ptr)[1] << 0)
                        
#define get_date(ptr) (dlp_ptohdate((ptr)))                        

#define get_byte(ptr) ((ptr)[0])

#define set_long(ptr,val) (((ptr)[0] = ((val) >> 24) & 0xff), \
		          ((ptr)[1] = ((val) >> 16) & 0xff), \
		          ((ptr)[2] = ((val) >> 8) & 0xff), \
		          ((ptr)[3] = ((val) >> 0) & 0xff))
                       
#define set_short(ptr,val) (((ptr)[0] = ((val) >> 8) & 0xff), \
		            ((ptr)[1] = ((val) >> 0) & 0xff))

#define set_date(ptr,val) (dlp_htopdate((val),(ptr)))                        

#define set_byte(ptr,val) ((ptr)[0]=(val))


static unsigned char dlp_buf[0xffff];


int dlp_exec(int sd, int cmd, int arg, void *msg, int msglen, 
             void *result, int maxlen)
{
  int i;
  int err;
  
  if (arg && msg && msglen) {
    /* This arrangement of code and the memmove are intended to let the
       buffer be used as the input buffer to dlp_exec, in addition to the
       working buffer */
    memmove(&dlp_buf[6], msg, msglen);
    dlp_buf[1] = 1;
    dlp_buf[2] = arg | 0x80;
    dlp_buf[3] = 0;
    dlp_buf[4] = msglen >> 8;
    dlp_buf[5] = msglen & 0xff;
    i = msglen+6;
  } else {
    dlp_buf[1] = 0;
    i = 2;
  }
  dlp_buf[0] = cmd;
#ifdef DEBUG
  puts("Sending dlp command");
#endif
  pi_write(sd, &dlp_buf[0], i);

#ifdef DEBUG
  puts("Reading dlp response");
#endif DEBUG
  i = pi_read(sd, &dlp_buf[0], 0xffff);
  
  err = (dlp_buf[2] << 8) | dlp_buf[3];
  
  if (err) {
    errno = -EIO;
    return -err;
  }

  if (dlp_buf[0] != (cmd | 0x80)) { /* received wrong response */
    errno = -ENOMSG;
    return -1;
  }
  
  if ((dlp_buf[1] == 0) || (result==0)) /* no return blocks or buffers */
    return 0; 
    
  /* assume only one return block */
  if (dlp_buf[4] & 0x80) {
  	i = (dlp_buf[6] << 8) | dlp_buf[7];
  	
  	if (i>maxlen)
  	  i = maxlen;
  	  
  	memmove(result, &dlp_buf[8], i);
  } else {
  	i = dlp_buf[5];

  	if (i>maxlen)
  	  i = maxlen;
  	  

  	memmove(result, &dlp_buf[6], i);
  }
  return i;
  
}

/* These conversion functions are strictly for use within the DLP layer.
   This particular date/time format does not occur anywhere else within the
   Pilot or its communications. */
   
static time_t dlp_ptohdate(unsigned const char * data) {
        struct tm t;
        
        t.tm_sec = data[6];
        t.tm_min = data[5];
        t.tm_hour = data[4];
        t.tm_mday = data[3];
        t.tm_mon = data[2]-1;
        t.tm_year = ((data[0]<<8) | data[1])-1900;
        t.tm_isdst = -1;
        
        return mktime(&t);
}

static void dlp_htopdate(time_t time, unsigned char * data) {
        struct tm * t = localtime(&time);
        
        int y = t->tm_year+1900;
        
        data[7] = 0; /* packing spacer */
        data[6] = t->tm_sec;
        data[5] = t->tm_min;
        data[4] = t->tm_hour;
        data[3] = t->tm_mday;
        data[2] = t->tm_mon+1;
        data[0] = (y >> 8) & 0xff;
        data[1] = (y >> 0) & 0xff;
        
        return;
}

int dlp_GetSysDateTime(int sd, time_t * t) {
  char buf[8];
  int result = dlp_exec(sd, 0x13, 0x20, 0, 0, buf, 8);
  
  if(result == 8)
    *t = dlp_ptohdate(buf);
  else
    *t = 0;
  
  return result;
}

int dlp_SetSysDateTime(int sd, time_t time) {
  char buf[8];
  dlp_htopdate(time, buf);
  
  return dlp_exec(sd, 0x14, 0x20, buf, 8, 0, 0);
}

int dlp_ReadSysInfo(int sd, struct SysInfo * s) {
  int result = dlp_exec(sd, 0x12, 0x20, NULL, 0, dlp_buf, 256);
  
  if(result >= 10) {
    s->ROMVersion = get_long(dlp_buf);
    s->localizationID = get_long(dlp_buf+4);
    /* dlp_buf+8 is a filler byte */
    s->namelength = get_byte(dlp_buf+9);
    memcpy(s->name, dlp_buf+10, s->namelength);
    s->name[s->namelength] = 0;
  }
  
  return result;
}

int dlp_OpenDB(int sd, int cardno, int mode, char * name, int * dbhandle) {
  unsigned char handle;
  int result;
  
  dlp_buf[0] = (unsigned char)cardno;
  dlp_buf[1] = (unsigned char)mode;
  memmove(&dlp_buf[2], name, strlen(name)+1); /* NUL is included */
  
  result = dlp_exec(sd, 0x17, 0x20, &dlp_buf[0], strlen(name)+3, &handle, 1);
  
  if(result == 1)
    *dbhandle = handle;
  else
    *dbhandle = 0;
  
  return result;
}

int dlp_DeleteDB(int sd, int card, const char * name) {
  
  dlp_buf[0] = card;
  dlp_buf[1] = 0;
  strcpy(dlp_buf+2, name);
  
  return dlp_exec(sd, 0x1A, 0x20, dlp_buf, 2+strlen(name)+1, 0, 0);
}

int dlp_CreateDB(int sd, long creator, long type, int cardno, 
                 int flags, int version, const char * name, int * dbhandle) {
  unsigned char handle;
  int result;
  
  set_long(dlp_buf, creator);
  set_long(dlp_buf+4, type);
  set_byte(dlp_buf+8, cardno);
  set_byte(dlp_buf+9, 0);
  set_short(dlp_buf+10, flags);
  set_short(dlp_buf+12, version);
  strcpy(dlp_buf+14, name);
  
  result = dlp_exec(sd, 0x18, 0x20, dlp_buf, 14+strlen(name)+1, &handle, 1);
  
  if(result==1)
    *dbhandle = handle;
  else
    *dbhandle = 0;
  
  return result;
}


int dlp_CloseDB(int sd, int dbhandle) {
  unsigned char handle = dbhandle;
  
  return dlp_exec(sd, 0x19, 0x20, &handle, 1, 0, 0);
}

int dlp_CloseDB_All(int sd) {
  
  return dlp_exec(sd, 0x19, 0x21, 0, 0, 0, 0);
}

int dlp_ResetSystem(int sd) {

  return dlp_exec(sd, 0x29, 0, 0, 0, 0, 0);
}

int dlp_AddSyncLogEntry(int sd, char * entry) {

  return dlp_exec(sd, 0x2A, 0x20, entry, strlen(entry), 0, 0);
}

int dlp_ReadOpenDBInfo(int sd, int dbhandle, int * records) {
  char buf[2];
  unsigned char handle = dbhandle;
  int result = dlp_exec(sd, 0x2B, 0x20, &dbhandle, 1, &buf[0], 2);
  
  if (records)
    if (result == 2)
      *records = (buf[0] << 8) | buf[1];
    else
      *records = 0;
  
  return result;
}

int dlp_OpenConduit(int sd) {

  return dlp_exec(sd, 0x2E, 0, 0, 0, 0, 0);
}

int dlp_EndOfSync(int sd, int status) {
  unsigned short int code = htons(status);
  int result;
  struct pi_socket * ps;
  
  result = dlp_exec(sd, 0x2F, 0x20, &code, 2, 0, 0);
  
  /* Messy code to set end-of-sync flag on socket 
     so pi_close won't do it for us */
  if (result == 0)
    if (ps = find_pi_socket(sd))
      ps->connected |= 2;
  
  return result;
}

int dlp_AbortSync(int sd) {
  struct pi_socket * ps;
  
  /* Set end-of-sync flag on socket so pi_close won't do a dlp_EndOfSync */
  if (ps = find_pi_socket(sd))
    ps->connected |= 2;

  pi_close(sd);
  
  return 0;
}

int dlp_WriteUserInfo(int sd, struct PilotUser *User) {

  set_long(dlp_buf, User->userID);
  set_long(dlp_buf+4, User->viewerID);
  set_long(dlp_buf+8, User->lastSyncPC);
  set_date(dlp_buf+12, User->lastSyncDate);
  set_byte(dlp_buf+20, 0xff);
  set_byte(dlp_buf+21, strlen(User->username)+1);
  strcpy(dlp_buf+22, User->username);

  return dlp_exec(sd, 0x11, 0x20, dlp_buf, 22+strlen(User->username)+1, NULL, 0);
}
                        
int dlp_ReadUserInfo(int sd, struct PilotUser *User) {

  int result;

  result = dlp_exec(sd, 0x10, 0x00, NULL, 0, dlp_buf, 0xffff);
  
  if (result >= 30) {
  
    User->userID = get_long(dlp_buf);
    User->viewerID = get_long(dlp_buf+4);
    User->lastSyncPC = get_long(dlp_buf+8);
    User->succSyncDate = get_date(dlp_buf+12);
    User->lastSyncDate = get_date(dlp_buf+20);
    User->passwordLen = get_byte(dlp_buf+29);
    memcpy(User->username, dlp_buf+30, get_byte(dlp_buf+28));
    User->username[get_byte(dlp_buf+28)] = 0;
    memcpy(User->password, dlp_buf+30+get_byte(dlp_buf+28), User->passwordLen);

  }

  return result;
}

int dlp_WriteRecord(int sd, unsigned char dbhandle, unsigned char flags,
                 long recID, short catID, char *data, int length, long * NewID) {
  char buf[4];
  int result;

  set_byte(dlp_buf, dbhandle);
  set_byte(dlp_buf+1, 0);
  set_long(dlp_buf+2, recID);
  set_byte(dlp_buf+6, flags);
  set_byte(dlp_buf+7, catID);
  
  if(length == -1)
  	length = strlen(data)+1;
  	
  memcpy(dlp_buf+8, data, length);

  result = dlp_exec(sd, 0x21, 0x20, dlp_buf, 8+length, buf, 4);
  
  if(NewID)
    if(result == 4)
      *NewID = get_long(buf); /* New record ID */
    else
      *NewID = 0;
      
  return result;
}

int dlp_WriteResource(int sd, unsigned char dbhandle, long type, int id,
                 const char *data, int length) {

  set_byte(dlp_buf, dbhandle);
  set_byte(dlp_buf+1, 0);
  set_long(dlp_buf+2, type);
  set_short(dlp_buf+6, id);
  set_short(dlp_buf+8, length);
  memcpy(dlp_buf+10, data, length);

  return dlp_exec(sd, 0x24, 0x20, dlp_buf, 10+length, NULL, 0);
}
