/* dlp.c:  Pilot DLP protocol
 *
 * (c) 1996, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

/*@+matchanyintegral@*/
/*@-predboolint@*/
/*@-boolops@*/ 
 
#include <stdio.h>
#include "pi-socket.h"
#include "dlp.h"

#define get_date(ptr) (dlp_ptohdate((ptr)))                        

#define set_date(ptr,val) (dlp_htopdate((val),(ptr)))                        

#define DLP_BUF_SIZE 0xffff
static /*@checked@*/ unsigned char dlp_buf[DLP_BUF_SIZE];
static /*@checked@*/ unsigned char exec_buf[DLP_BUF_SIZE];

int dlp_exec(int sd, int cmd, int arg, 
             const unsigned char /*@null@*/ *msg, int msglen, 
             unsigned char /*@out@*/ /*@null@*/ *result, int maxlen)
 /*@modifies *result, exec_buf;@*/
 /*@-predboolint -boolops@*/
{
  int i;
  int err;
  
  exec_buf[0] = (unsigned char)cmd;
  if (msg && arg && msglen) {
    memcpy(&exec_buf[6], msg, msglen);
    exec_buf[1] = (unsigned char)1;
    exec_buf[2] = (unsigned char)(arg | 0x80);
    exec_buf[3] = (unsigned char)0;
    set_short(exec_buf+4, msglen);
    i = msglen+6;
  } else {
    exec_buf[1] = (unsigned char)0;
    i = 2;
  }

#ifdef DEBUG
  fprintf(stderr,"Sending dlp command\n");
#endif
  if (pi_write(sd, &exec_buf[0], i)<i) {
    errno = -EIO;
    return -1;
  }

#ifdef DEBUG
  fprintf(stderr,"Reading dlp response\n");
#endif
  i = pi_read(sd, &exec_buf[0], DLP_BUF_SIZE);

  err = get_short(exec_buf+2);

  if (err != 0) {
    errno = -EIO;
    return -err;
  }

  if (exec_buf[0] != (unsigned char)(cmd | 0x80)) { /* received wrong response */
    errno = -ENOMSG;
    return -1;
  }
  
  if ((exec_buf[1] == (unsigned char)0) || (result==0)) /* no return blocks or buffers */
    return 0; 
    
  /* assume only one return block */
  if (exec_buf[4] & 0x80) {
  	i = get_short(exec_buf+6);
  	
  	if (i>maxlen)
  	  i = maxlen;
  	  
  	memcpy(result, &exec_buf[8], i);
  } else {
  	i = (int)exec_buf[5];

  	if (i>maxlen)
  	  i = maxlen;
  	  
  	memcpy(result, &exec_buf[6], i);
  }

  return i;
  
}

/* These conversion functions are strictly for use within the DLP layer.
   This particular date/time format does not occur anywhere else within the
   Pilot or its communications. */
   
static time_t dlp_ptohdate(unsigned const char * data)
{
        struct tm t;
        
        t.tm_sec   = (int)data[6];
        t.tm_min   = (int)data[5];
        t.tm_hour  = (int)data[4];
        t.tm_mday  = (int)data[3];
        t.tm_mon   = (int)data[2]-1;
        t.tm_year  = ((data[0]<<8) | data[1])-1900;
        t.tm_wday  = 0;
        t.tm_yday  = 0;
        t.tm_isdst = -1;
        
        return mktime(&t);
}

static void dlp_htopdate(time_t time, unsigned char * data) /*@+ptrnegate@*/
{
        struct tm * t = localtime(&time);
        int y;

        if (!t)
          abort();
        
        y = t->tm_year+1900;
        
        data[7] = (unsigned char)0; /* packing spacer */
        data[6] = (unsigned char)t->tm_sec;
        data[5] = (unsigned char)t->tm_min;
        data[4] = (unsigned char)t->tm_hour;
        data[3] = (unsigned char)t->tm_mday;
        data[2] = (unsigned char)(t->tm_mon+1);
        data[0] = (unsigned char)((y >> 8) & 0xff);
        data[1] = (unsigned char)((y >> 0) & 0xff);
        
        return;
}

int dlp_GetSysDateTime(int sd, time_t * t)
{
  unsigned char buf[8];
  int result = dlp_exec(sd, 0x13, 0x20, 0, 0, buf, 8);
  
  if(result == 8)
    *t = dlp_ptohdate(buf);
  else
    *t = 0;
  
  return result;
}

int dlp_SetSysDateTime(int sd, time_t time)
{
  unsigned char buf[8];
  dlp_htopdate(time, buf);
  
  return dlp_exec(sd, 0x14, 0x20, buf, 8, 0, 0);
}

int dlp_ReadSysInfo(int sd, struct SysInfo * s)
{
  int result = dlp_exec(sd, 0x12, 0x20, NULL, 0, dlp_buf, 256);
  
  if(result >= 10) {
    s->ROMVersion = get_long(dlp_buf);
    s->localizationID = get_long(dlp_buf+4);
    /* dlp_buf+8 is a filler byte */
    s->namelength = get_byte(dlp_buf+9);
    memcpy(s->name, dlp_buf+10, s->namelength);
    s->name[s->namelength] = '\0';
  }
  
  return result;
}

int dlp_ReadDBList(int sd, int cardno, int flags, int start, struct DBInfo * info)
{
  int result;

  dlp_buf[0] = (unsigned char)flags;
  dlp_buf[1] = (unsigned char)cardno;
  set_short(dlp_buf+2, start);
  
  result = dlp_exec(sd, 0x16, 0x20, dlp_buf, 4, dlp_buf, 48+32);
  
  if(result <= 0)
    return result;
    
  info->more = get_byte(dlp_buf+2);
  info->flags = get_short(dlp_buf+6);
  info->type = get_long(dlp_buf+8);
  info->creator = get_long(dlp_buf+12);
  info->version = get_short(dlp_buf+16);
  info->modnum = get_long(dlp_buf+18);
  info->crdate = get_date(dlp_buf+22);
  info->moddate = get_date(dlp_buf+30);
  info->backupdate = get_date(dlp_buf+38);
  info->index = get_short(dlp_buf+46);
  strncpy(info->name, dlp_buf+48, 32);
  info->name[32] = '\0';
    
  return result;
}

int dlp_FindDBInfo(int sd, int cardno, int start, char * dbname, unsigned long type, unsigned long creator, struct DBInfo * info)
{
  
  int i;
  
  if (start < 0x1000) {
    i=start;
    while(dlp_ReadDBList(sd, cardno, 0x80, i, info)>0) {
      if( 
         ((!dbname) || (strcmp(info->name,dbname)==0)) &&
         ((!type) || (info->type==type)) &&
         ((!creator) || (info->creator==creator))
        )
        goto found;
      i=info->index+1;
    }
    start = 0x1000;
  }
  
  i=start & 0xFFF;
  while(dlp_ReadDBList(sd, cardno, 0x40, i, info)>0) {
    if( 
       ((!dbname) || (strcmp(info->name,dbname)==0)) &&
       ((!type) || (info->type==type)) &&
       ((!creator) || (info->creator==creator))
      ) {
      info->index |= 0x1000;
      goto found;
    }
    i=info->index+1;
  }
  
  return -1;

found:

  return 0;
}

int dlp_OpenDB(int sd, int cardno, int mode, char * name, int * dbhandle)
{
  unsigned char handle;
  int result;

  dlp_buf[0] = (unsigned char)cardno;
  dlp_buf[1] = (unsigned char)mode;
  memcpy(&dlp_buf[2], name, strlen(name)+1); /* NUL is included */
  
  result = dlp_exec(sd, 0x17, 0x20, &dlp_buf[0], strlen(name)+3, &handle, 1);
  
  if(result == 1)
    *dbhandle = (int)handle;
  else
    *dbhandle = 0;
  
  return result;
}

int dlp_DeleteDB(int sd, int card, const char * name)
{
  
  dlp_buf[0] = (unsigned char)card;
  dlp_buf[1] = (unsigned char)0;
  strcpy(dlp_buf+2, name);
  
  return dlp_exec(sd, 0x1A, 0x20, dlp_buf, 2+strlen(name)+1, 0, 0);
}

int dlp_CreateDB(int sd, long creator, long type, int cardno, 
                 int flags, int version, const char * name, int * dbhandle)
{
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
    *dbhandle = (int)handle;
  else
    *dbhandle = 0;

  return result;
}


int dlp_CloseDB(int sd, int dbhandle)
{
  unsigned char handle = (unsigned char)dbhandle;
  
  return dlp_exec(sd, 0x19, 0x20, &handle, 1, 0, 0);
}

int dlp_CloseDB_All(int sd)
{
  
  return dlp_exec(sd, 0x19, 0x21, 0, 0, 0, 0);
}

int dlp_ResetSystem(int sd)
{

  return dlp_exec(sd, 0x29, 0, 0, 0, 0, 0);
}

int dlp_AddSyncLogEntry(int sd, char * entry)
{
  return dlp_exec(sd, 0x2A, 0x20, (unsigned char*)entry, strlen(entry), 0, 0);
}

int dlp_ReadOpenDBInfo(int sd, int dbhandle, int * records)
{
  unsigned char buf[2];
  int result;
  
  set_byte(dlp_buf, (unsigned char)dbhandle);
  result = dlp_exec(sd, 0x2B, 0x20, dlp_buf, 1, buf, 2);
  
  if (records)
    if (result == 2)
      *records = get_short(buf);
    else
      *records = 0;
  
  return result;
}

int dlp_OpenConduit(int sd)
{
  return dlp_exec(sd, 0x2E, 0, 0, 0, 0, 0);
}

int dlp_EndOfSync(int sd, int status)
{
  int result;
  struct pi_socket * ps;
  
  set_short(dlp_buf, status);
  
  result = dlp_exec(sd, 0x2F, 0x20, dlp_buf, 2, 0, 0);
  
  /* Messy code to set end-of-sync flag on socket 
     so pi_close won't do it for us */
  if (result == 0)
    if ( (ps = find_pi_socket(sd)) )
      ps->connected |= 2;
  
  return result;
}

int dlp_AbortSync(int sd) {
  struct pi_socket * ps;
  
  /* Set end-of-sync flag on socket so pi_close won't do a dlp_EndOfSync */
  if ( (ps = find_pi_socket(sd)) )
    ps->connected |= 2;

  return pi_close(sd);
}

int dlp_WriteUserInfo(int sd, struct PilotUser *User)
{
  set_long(dlp_buf, User->userID);
  set_long(dlp_buf+4, User->viewerID);
  set_long(dlp_buf+8, User->lastSyncPC);
  set_date(dlp_buf+12, User->lastSyncDate);
  set_byte(dlp_buf+20, 0xff);
  set_byte(dlp_buf+21, strlen(User->username)+1);
  strcpy(dlp_buf+22, User->username);

  return dlp_exec(sd, 0x11, 0x20, dlp_buf, 22+strlen(User->username)+1, NULL, 0);
}
                        
int dlp_ReadUserInfo(int sd, struct PilotUser* User)
{
  int result;

  result = dlp_exec(sd, 0x10, 0x00, NULL, 0, dlp_buf, DLP_BUF_SIZE);

  if (result >= 30) {
    int userlen = get_byte(dlp_buf+28);

    User->userID = get_long(dlp_buf);
    User->viewerID = get_long(dlp_buf+4);
    User->lastSyncPC = get_long(dlp_buf+8);
    User->succSyncDate = get_date(dlp_buf+12);
    User->lastSyncDate = get_date(dlp_buf+20);
    User->passwordLen = get_byte(dlp_buf+29);
    memcpy(User->username, dlp_buf+30, userlen);
    User->username[userlen] = '\0';
    memcpy(User->password, dlp_buf+30+userlen, User->passwordLen);
  }

  return result;
}

int dlp_ReadRecordIDList(int sd, int dbhandle, int sort, 
                         int start, int max, unsigned long * IDs)
{
  int result;
  
  set_byte(dlp_buf,    dbhandle);
  set_byte(dlp_buf+1,  sort?0x80:0);
  set_short(dlp_buf+2, start);
  set_short(dlp_buf+4, max);
  
  result = dlp_exec(sd, 0x20, 0x20, dlp_buf, 6, (unsigned char*)IDs, max*4);
  
  if(result>=0)
    return result/4;
  else
    return result;
}

int dlp_WriteRecord(int sd, int dbhandle, int flags,
                 unsigned long recID, int catID, unsigned char* data, int length, long* NewID)
{
  unsigned char buf[4];
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

int dlp_DeleteRecord(int sd, int dbhandle, int all, unsigned long recID)
{
  int result;
  int flags = all ? 0x80 : 0;

  set_byte(dlp_buf, dbhandle);
  set_byte(dlp_buf+1, flags);
  set_long(dlp_buf+2, recID);
  
  result = dlp_exec(sd, 0x22, 0x20, dlp_buf, 6, 0, 0);
  
  return result;
}


int dlp_ReadResourceByType(int sd, int fHandle, unsigned long type, int id, unsigned char* buffer, 
                          int* index, int* size)
{
  int result;
  
  set_byte(dlp_buf, fHandle);
  set_byte(dlp_buf+1, 0x00);
  set_long(dlp_buf+2, type);
  set_short(dlp_buf+6, id);
  set_short(dlp_buf+8, 0); /* Offset into record */
  set_short(dlp_buf+10, DLP_BUF_SIZE); /* length to return */

  result = dlp_exec(sd, 0x23, 0x21, dlp_buf, 12, dlp_buf, DLP_BUF_SIZE);
  
  if (result >= 10) {
    if (index)
      *index = get_short(dlp_buf+6);
    if (size)
      *size = get_short(dlp_buf+8);
    if (buffer)
      memcpy(buffer, dlp_buf+10, result-10);

    return result-10;
  }
  else
    return result;
}

int dlp_ReadResourceByIndex(int sd, int fHandle, int index, unsigned char* buffer,
                          unsigned long* type, int * id, int* size)
{
  int result;
  
  set_byte(dlp_buf, fHandle);
  set_byte(dlp_buf+1, 0x00);
  set_short(dlp_buf+2, index);
  set_short(dlp_buf+4, 0); /* Offset into record */
  set_short(dlp_buf+6, DLP_BUF_SIZE); /* length to return */

  result = dlp_exec(sd, 0x23, 0x20, dlp_buf, 8, dlp_buf, DLP_BUF_SIZE);
  
  if (result >= 10) {
    if (type) 
      *type = get_long(dlp_buf);
    if (id)
      *id = get_short(dlp_buf+4);
    if (size)
      *size = get_short(dlp_buf+8);
    if (buffer)
      memcpy(buffer, dlp_buf+10, result-10);
    
    return result-10;
  }
  else
    return result;
}

int dlp_WriteResource(int sd, int dbhandle, unsigned long type, int id,
                 const unsigned char* data, int length)
{
  set_byte(dlp_buf, dbhandle);
  set_byte(dlp_buf+1, 0);
  set_long(dlp_buf+2, type);
  set_short(dlp_buf+6, id);
  set_short(dlp_buf+8, length);
  memcpy(dlp_buf+10, data, length);

  return dlp_exec(sd, 0x24, 0x20, dlp_buf, 10+length, NULL, 0);
}

int dlp_DeleteResource(int sd, int dbhandle, int all, unsigned long restype, int resID)
{
  int result;
  int flags = all ? 0x80 : 0;

  set_byte(dlp_buf, dbhandle);
  set_byte(dlp_buf+1, flags);
  set_long(dlp_buf+2, restype);
  set_short(dlp_buf+6, resID);
  
  result = dlp_exec(sd, 0x25, 0x20, dlp_buf, 8, NULL, 0);
  
  return result;
}

int dlp_ReadAppBlock(int sd, int fHandle, int offset,
                           unsigned char* dbuf, int dlen)
{
  int result;
  
  set_byte(dlp_buf, fHandle);
  set_byte(dlp_buf+1, 0x00);
  set_short(dlp_buf+2, offset);
  set_short(dlp_buf+4, dlen);
  
  result = dlp_exec(sd, 0x1b, 0x20, dlp_buf, 6, dlp_buf, DLP_BUF_SIZE);
  
  if(result >= 2) {
    if (dbuf)
      memcpy(dbuf, dlp_buf+2, result-2);
    return result-2;
  }
  else
    return result;
}

int dlp_WriteAppBlock(int sd, int fHandle, const /*@unique@*/ unsigned char* data, int length)
{
  set_byte(dlp_buf, fHandle);
  set_byte(dlp_buf+1, 0x00);
  set_short(dlp_buf+2, length);
  memcpy(dlp_buf+4, data, length);

  return dlp_exec(sd, 0x1c, 0x20, dlp_buf, length+4, NULL, 0);
}

int dlp_ReadSortBlock(int sd, int fHandle, int offset,
                           unsigned char* dbuf, int dlen)
{
  int result;
  
  set_byte(dlp_buf, fHandle);
  set_byte(dlp_buf+1, 0x00);
  set_short(dlp_buf+2, offset);
  set_short(dlp_buf+4, dlen);
  
  result = dlp_exec(sd, 0x1d, 0x20, dlp_buf, 6, dlp_buf, DLP_BUF_SIZE);
  
  if(result >= 2) {
    if (dbuf)
      memcpy(dbuf, dlp_buf+2, result-2);
    return result-2;
  }
  else
    return result;
}

int dlp_WriteSortBlock(int sd, int fHandle, const /*@unique@*/ unsigned char* data, int length)
{
  set_byte(dlp_buf, fHandle);
  set_byte(dlp_buf+1, 0x00);
  set_short(dlp_buf+2, length);
  memcpy(dlp_buf+4, data, length);

  return dlp_exec(sd, 0x1e, 0x20, dlp_buf, length+4, NULL, 0);
}

int dlp_CleanUpDatabase(int sd, int fHandle)
{
  unsigned char handle = fHandle;
  return dlp_exec(sd, 0x26, 0x20, &handle, 1, NULL, 0);
}

int dlp_ResetSyncFlags(int sd, int fHandle)
{
  unsigned char handle = fHandle;
  return dlp_exec(sd, 0x27, 0x20, &handle, 1, NULL, 0);
}

int dlp_ReadNextModifiedRec(int sd, int fHandle, unsigned char* buffer,
                          unsigned long* id, int* index, int* size, int* attr, int* category)
{
  unsigned char handle = fHandle;
  int result = dlp_exec(sd, 0x1f, 0x20, &handle, 1, dlp_buf, DLP_BUF_SIZE);

  if (result >= 10) {
    if (id)
      *id = get_long(dlp_buf);
    if (index)
      *index = get_short(dlp_buf+4);
    if (size)
      *size = get_short(dlp_buf+6);
    if (attr)
      *attr = get_byte(dlp_buf+8);
    if (category)
      *category = get_byte(dlp_buf+9);
      
    memcpy(buffer, dlp_buf+10, result-10);
    
    return result-10;
  }
  else
    return result;
}

int dlp_ReadRecordById(int sd, int fHandle, unsigned long id, unsigned char* buffer, 
                          int* index, int* size, int* attr, int* category)
{
  int result;
  
  set_byte(dlp_buf, fHandle);
  set_byte(dlp_buf+1, 0x00);
  set_long(dlp_buf+2, id);
  set_short(dlp_buf+6, 0); /* Offset into record */
  set_short(dlp_buf+8, DLP_BUF_SIZE); /* length to return */

  result = dlp_exec(sd, 0x20, 0x20, dlp_buf, 10, dlp_buf, DLP_BUF_SIZE);
  
  if (result >= 10) {
    /*id = get_long(dlp_buf);*/
    if (index)
      *index = get_short(dlp_buf+4);
    if (size)
      *size = get_short(dlp_buf+6);
    if (attr)
      *attr = get_byte(dlp_buf+8);
    if (category)
      *category = get_byte(dlp_buf+9);
    if (buffer)
      memcpy(buffer, dlp_buf+10, result-10);

    return result-10;
  }
  else
    return result;
}

int dlp_ReadRecordByIndex(int sd, int fHandle, int index, unsigned char* buffer,
                          long* id, int* size, int* attr, int* category)
{
  int result;
  
  set_byte(dlp_buf, fHandle);
  set_byte(dlp_buf+1, 0x00);
  set_short(dlp_buf+2, index);
  set_short(dlp_buf+4, 0); /* Offset into record */
  set_short(dlp_buf+6, DLP_BUF_SIZE); /* length to return */

  result = dlp_exec(sd, 0x20, 0x21, dlp_buf, 8, dlp_buf, DLP_BUF_SIZE);
  
  if (result >= 10) {
    if (id)
      *id = get_long(dlp_buf);
    /*get_short(dlp_buf+4) == index*/
    if (size)
      *size = get_short(dlp_buf+6);
    if (attr)
      *attr = get_byte(dlp_buf+8);
    if (category)
      *category = get_byte(dlp_buf+9);
    if (buffer)
      memcpy(buffer, dlp_buf+10, result-10);
    
    return result-10;
  }
  else
    return result;
}
