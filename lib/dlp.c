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
  	  
  	memcpy(result, &dlp_buf[8], i);
  } else {
  	i = dlp_buf[5];

  	if (i>maxlen)
  	  i = maxlen;
  	  

  	memcpy(result, &dlp_buf[6], i);
  }
  return i;
  
}

/* This structure and the accompanying conversion functions
   are strictly for use within the DLP layer. This particular
   date/time format does not occur anywhere else within
   the Pilot or its communications. */
   
typedef struct {
  unsigned char data[8];
} dlp_PilotDateTime;

static time_t dlp_ptohdate(dlp_PilotDateTime d) {
        struct tm t;
        
        t.tm_sec = d.data[6];
        t.tm_min = d.data[5];
        t.tm_hour = d.data[4];
        t.tm_mday = d.data[3];
        t.tm_mon = d.data[2]-1;
        t.tm_year = ((d.data[0]<<8) | d.data[1])-1900;
        t.tm_isdst = -1;
        
        return mktime(&t);
}

static dlp_PilotDateTime dlp_htopdate(time_t time) {
        struct tm * t = localtime(&time);
        dlp_PilotDateTime d;
        
        int y = t->tm_year+1900;
        
        d.data[7] = 0; /* packing spacer */
        d.data[6] = t->tm_sec;
        d.data[5] = t->tm_min;
        d.data[4] = t->tm_hour;
        d.data[3] = t->tm_mday;
        d.data[2] = t->tm_mon+1;
        d.data[0] = (y >> 8) & 0xff;
        d.data[1] = (y >> 0) & 0xff;
        
        return d;
}

int dlp_GetSysDateTime(int sd, time_t * t) {
  dlp_PilotDateTime d;
  int result = dlp_exec(sd, 0x13, 0x20, 0, 0, &d, sizeof(d));
  
  if(result == sizeof(d))
    *t = dlp_ptohdate(d);
  else
    *t = 0;
  
  return result;
}

int dlp_SetSysDateTime(int sd, time_t time) {
  dlp_PilotDateTime d = dlp_htopdate(time);
  
  return dlp_exec(sd, 0x14, 0x20, &d, sizeof(d), 0, 0);
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

int dlp_WriteUser(int sd, struct PilotUser *User, time_t tm) {
  dlp_PilotDateTime d;

  *((long *)(dlp_buf)) = htonl(User->id1);
  *((long *)(dlp_buf+4)) = htonl(User->id2);
  *((long *)(dlp_buf+8)) = htonl(User->id3);

  d = dlp_htopdate(tm);
  memcpy(dlp_buf+12,&d,sizeof(d));

  dlp_buf[20] = 0xff;
  dlp_buf[21] = strlen(User->username)+1;
  strcpy(dlp_buf+22,User->username);

  return dlp_exec(sd, 0x11, 0x20, dlp_buf, strlen(User->username)+23, NULL, 0);
}

int dlp_ReadUser(int sd, struct PilotUser *User) {

  int result;

  result = dlp_exec(sd, 0x10, 0x00, NULL, 0, dlp_buf, 0xffff);

  User->id1 = ntohl(*((long *)(dlp_buf)));
  User->id2 = ntohl(*((long *)(dlp_buf+4)));
  User->id3 = ntohl(*((long *)(dlp_buf+8)));

  strcpy(User->username, dlp_buf+30);

  return result;
}

int dlp_WriteRec(int sd, unsigned char dbhandle, unsigned char flags,
                 long recID, short catID, char *text) {

  dlp_buf[0] = dbhandle;
  dlp_buf[1] = flags;
  *((long *)(dlp_buf+2)) = htonl(recID);
  *((short *)(dlp_buf+6)) = htons(catID);
  strcpy(dlp_buf+8,text);

  return dlp_exec(sd, 0x21, 0x20, dlp_buf, 9+strlen(text), NULL, 0);
}
