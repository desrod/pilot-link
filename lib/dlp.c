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

char * dlp_errorlist[] = { 
  "No error",
  "General System error",
  "Illegal Function",
  "Out of memory",
  "Invalid parameter",
  "Not found",
  "None Open",
  "Already Open",
  "Too many Open",
  "Already Exists",
  "Cannot Open",
  "Record deleted",
  "Record busy",
  "Operation not supported",
  "-Unused-",
  "Read only",
  "Not enough space",
  "Limit exceeded",
  "Sync cancelled",
  "Bad arg wrapper",
  "Argument missing",
  "Bad argument size"
};

#ifdef DLP_TRACE

#define Trace(name) \
  fprintf(stderr, "DLP %d: %s\n", sd, #name);
#define Expect(count) \
  if (result < count) {  \
    if (result < 0) \
      fprintf(stderr, "Result: Error: %s (%d)\n", dlp_errorlist[-result], result); \
    else { \
      fprintf(stderr, "Result: Read %d bytes, expected at least %d\n", result, count); \
      result = -128; \
    } \
    return result;      \
  } else \
    fprintf(stderr, "Result: No error, %d bytes\n", result);
  
#else

#define Trace(name)
#define Expect(count)   \
  if (result < count) { \
    if (result >= 0)    \
      result = -128;    \
    return result;      \
  }
  
#endif


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

  if (pi_write(sd, &exec_buf[0], i)<i) {
    errno = -EIO;
    return -1;
  }

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
   
/* Notice: These conversion functions apply a possibly incorrect timezone
   correction. They use the local time on the UNIX box, and transmit this
   directly to the Pilot. This assumes that the Pilot has the same local
   time. If the Pilot is communicating from a different timezone, this is
   not necessarily correct.
   
   It would be possible to compare the current time on the Pilot with the
   current time on the UNIX box, and use that as the timezone offset, but
   this would break if the Pilot had the wrong time, or one or the either
   didn't have the proper local (wall) time.
   
   In any case, since the (possibly incorrect) timezone correction is
   applied both way, there is no immediate problem.
                                                                   -- KJA
   */
   
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
  int result;
  
  Trace(GetSysDateTime);
  
  result = dlp_exec(sd, 0x13, 0x20, 0, 0, buf, 8);
  
  Expect(8);
  
  *t = dlp_ptohdate(buf);

#ifdef DLP_TRACE
  fprintf(stderr, "   Read: Time: %s", ctime(t));
#endif
  
  return result;
}

int dlp_SetSysDateTime(int sd, time_t time)
{
  unsigned char buf[8];
  int result;
  dlp_htopdate(time, buf);

  Trace(ReadSysInfo);

#ifdef DLP_TRACE
  fprintf(stderr, "  Wrote: Time: %s", ctime(&time));
#endif
  
  result = dlp_exec(sd, 0x14, 0x20, buf, 8, 0, 0);
  
  Expect(0);
  
  return result;
}

int dlp_ReadStorageInfo(int sd, int cardno, struct CardInfo * c)
{
  int result;
  int len1,len2;
  
  set_byte(dlp_buf, cardno);
  set_byte(dlp_buf+1, 0);
  
  Trace(ReadStorageInfo);
  
#ifdef DLP_TRACE
  fprintf(stderr, " Wrote: Cardno: %d\n", cardno);
#endif  
  
  result = dlp_exec(sd, 0x15, 0x20, dlp_buf, 2, dlp_buf, 256+26);
  
  c->more = 0;
  
  Expect(30);
  
  c->more = get_byte(dlp_buf+1) || (get_byte(dlp_buf+3) > 1);
  
  c->cardno = get_byte(dlp_buf+1+4);
  c->version = get_short(dlp_buf+2+4);
  c->creation = dlp_ptohdate(dlp_buf+4+4);
  c->ROMsize = get_long(dlp_buf+12+4);
  c->RAMsize = get_long(dlp_buf+16+4);
  c->RAMfree = get_long(dlp_buf+20+4);
  
  len1 = get_byte(dlp_buf+24+4);
  memcpy(c->name, dlp_buf+26+4, len1);
  c->name[len1] = '\0';
  
  len2 = get_byte(dlp_buf+25+4);
  memcpy(c->manuf, dlp_buf+26+4+len1, len2);
  c->name[len2] = '\0';
  
#ifdef DLP_TRACE
  fprintf(stderr, "  Read: Cardno: %d, Card Version: %d, Creation time: %s",
    c->cardno, c->version, ctime(&c->creation));
  fprintf(stderr, "        Total ROM: %lu, Total RAM: %lu, Free RAM: %lu\n",
    c->ROMsize, c->RAMsize, c->RAMfree);
  fprintf(stderr, "        Card name: '%s'\n", c->name);
  fprintf(stderr, "        Manufacturer name: '%s'\n", c->manuf);
  fprintf(stderr, "        More: %s\n", c->more ? "Yes" : "No");
#endif
  
  return result;
}


int dlp_ReadSysInfo(int sd, struct SysInfo * s)
{
  int result;
  
  Trace(ReadSysInfo);
  
  result = dlp_exec(sd, 0x12, 0x20, NULL, 0, dlp_buf, 256);
  
  Expect(10);
  
  s->ROMVersion = get_long(dlp_buf);
  s->localizationID = get_long(dlp_buf+4);
  /* dlp_buf+8 is a filler byte */
  s->namelength = get_byte(dlp_buf+9);
  memcpy(s->name, dlp_buf+10, s->namelength);
  s->name[s->namelength] = '\0';

#ifdef DLP_TRACE  
  fprintf(stderr, "  Read: ROM Version: 0x%8.8lX, Localization ID: 0x%8.8lX\n",
    (unsigned long)s->ROMVersion, (unsigned long)s->localizationID);
  fprintf(stderr, "        Name '%s'\n", s->name);
#endif
  
  return result;
}

int dlp_ReadDBList(int sd, int cardno, int flags, int start, struct DBInfo * info)
{
  int result;

  dlp_buf[0] = (unsigned char)flags;
  dlp_buf[1] = (unsigned char)cardno;
  set_short(dlp_buf+2, start);

  Trace(ReadDBList);

#ifdef DLP_TRACE  
  fprintf(stderr, " Wrote: Cardno: %d, Flags:", cardno);
  if (flags & dlpDBListROM)
    fprintf(stderr, " ROM");
  if (flags & dlpDBListRAM)
    fprintf(stderr, " RAM");
  if (!flags)
    fprintf(stderr, " None");
  fprintf(stderr, " (0x%2.2X)\n", flags);
#endif
  
  result = dlp_exec(sd, 0x16, 0x20, dlp_buf, 4, dlp_buf, 48+32);
  
  info->more = 0;
  
  Expect(48);
  
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

#ifdef DLP_TRACE  
  fprintf(stderr, "  Read: Name: '%s', Version: %d, More: %s\n",
    info->name, info->version, info->more ? "Yes" : "No");
  fprintf(stderr, "        Creator: '%s'", printlong(info->creator));
  fprintf(stderr, ", Type: '%s', Flags:", printlong(info->type));
  if (info->flags & dlpDBFlagResource)
    fprintf(stderr, " Resource");
  if (info->flags & dlpDBFlagReadOnly)
    fprintf(stderr, " ReadOnly");
  if (info->flags & dlpDBFlagAppInfoDirty)
    fprintf(stderr, " AppInfoDirty");
  if (info->flags & dlpDBFlagBackup)
    fprintf(stderr, " Backup");
  if (info->flags & dlpDBFlagOpen)
    fprintf(stderr, " Open");
  if (!info->flags)
    fprintf(stderr, " None");
  fprintf(stderr, " (0x%2.2X)\n", info->flags);
  fprintf(stderr, "        Modnum: %ld, Index: %d, Creation date: %s",
    info->modnum, info->index, ctime(&info->crdate));
  fprintf(stderr, "        Modification date: %s", ctime(&info->moddate));
  fprintf(stderr, "        Backup date: %s", ctime(&info->backupdate));
#endif
    
  return result;
}

int dlp_FindDBInfo(int sd, int cardno, int start, char * dbname, unsigned long type, unsigned long creator, struct DBInfo * info)
{
  int i;
  
  /* This function does not match any DLP layer function, but is intended as
     a shortcut for programs looking for databases. It uses a fairly
     byzantine mechanism for ordering the RAM databases before the ROM ones.
     You must feed the "index" slot from the returned info in as start the
     next time round. */
  
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
  strcpy(&dlp_buf[2], name);

  Trace(OpenDB);

#ifdef DLP_TRACE  
  fprintf(stderr, " Wrote: Cardno: %d, Name: '%s', Mode:", cardno, name);
  if (mode & dlpOpenRead)
    fprintf(stderr, " Read");
  if (mode & dlpOpenWrite)
    fprintf(stderr, " Write");
  if (mode & dlpOpenExclusive)
    fprintf(stderr, " Exclusive");
  if (mode & dlpOpenSecret)
    fprintf(stderr, " Secret");
  if (!mode)
    fprintf(stderr, " None");
  fprintf(stderr, " (0x%2.2X)\n", mode);
#endif
  
  result = dlp_exec(sd, 0x17, 0x20, &dlp_buf[0], strlen(name)+3, &handle, 1);
  
  Expect(1);
  
  *dbhandle = (int)handle;

#ifdef DLP_TRACE  
  fprintf(stderr, "  Read: Handle: %d\n", (int)handle);
#endif
  
  return result;
}



int dlp_DeleteDB(int sd, int card, const char * name)
{
  int result;
  
  dlp_buf[0] = (unsigned char)card;
  dlp_buf[1] = (unsigned char)0;
  strcpy(dlp_buf+2, name);

  Trace(DeleteDB);

#ifdef DLP_TRACE  
  fprintf(stderr, " Wrote: Cardno: %d, Name: '%s'\n", card, name);
#endif
  
  result = dlp_exec(sd, 0x1A, 0x20, dlp_buf, 2+strlen(name)+1, 0, 0);
  
  Expect(0);
  
  return result;
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
  
  Trace(CreateDB);

#ifdef DLP_TRACE  
  fprintf(stderr, " Wrote: Card: %d, version: %d, name '%s'\n", cardno, version, name);
  fprintf(stderr, "        DB Flags:");
  if (flags & dlpDBFlagResource)
    fprintf(stderr, " Resource");
  if (flags & dlpDBFlagReadOnly)
    fprintf(stderr, " ReadOnly");
  if (flags & dlpDBFlagAppInfoDirty)
    fprintf(stderr, " AppInfoDirty");
  if (flags & dlpDBFlagBackup)
    fprintf(stderr, " Backup");
  if (flags & dlpDBFlagOpen)
    fprintf(stderr, " Open");
  if (!flags)
    fprintf(stderr, " None");
  fprintf(stderr, " (0x%2.2X), Creator: '%s'", flags, printlong(creator));
  fprintf(stderr, ", Type: '%s'\n", printlong(type));
#endif  

  result = dlp_exec(sd, 0x18, 0x20, dlp_buf, 14+strlen(name)+1, &handle, 1);
  
  Expect(1);
  
  if (dbhandle)
    *dbhandle = (int)handle;

#ifdef DLP_TRACE
  fprintf(stderr, "  Read: Handle: %d\n", (int)handle);
#endif

  return result;
}


int dlp_CloseDB(int sd, int dbhandle)
{
  unsigned char handle = (unsigned char)dbhandle;
  int result;

  Trace(CloseDB);

#ifdef DLP_TRACE  
  fprintf(stderr, " Wrote: Handle: %d\n", dbhandle);
#endif
  
  result = dlp_exec(sd, 0x19, 0x20, &handle, 1, 0, 0);

  Expect(0);
  
  return result;
}

int dlp_CloseDB_All(int sd)
{
  int result;

  Trace(CloseDB_all);
  
  result = dlp_exec(sd, 0x19, 0x21, 0, 0, 0, 0);
  
  Expect(0);
  
  return result;
}

int dlp_CallApplication(int sd, unsigned long creator, int action, 
                        int length, unsigned char * data,
                        int * resultptr,
                        int * retlen, unsigned char * retdata)
{
  int result;
  
  set_long(dlp_buf+0, creator);
  set_short(dlp_buf+4, action);
  set_short(dlp_buf+6, length);
  
  Trace(CallApplication);
  
#ifdef DLP_TRACE  
  fprintf(stderr, " Wrote: Creator: '%s', Action code: %d, and %d bytes of data:\n",
    printlong(creator), action, length);
  dumpdata(data, length);
#endif
  
  result = dlp_exec(sd, 0x28, 0x21, dlp_buf, 8, dlp_buf, 0xffff);
  
  Expect(6);
  
  if (resultptr)
    *resultptr = get_short(dlp_buf+2);
    
  result -= 6;
  
  if (retlen && retdata)
    memcpy(retdata, dlp_buf+6, result > *retlen ? *retlen : result);
  
#ifdef DLP_TRACE  
  fprintf(stderr, "  Read: Action: %d, Result: %d (0x%4.4X), and %d bytes:\n",
    get_short(dlp_buf), get_short(dlp_buf+2), get_short(dlp_buf+2), result);
  dumpdata(dlp_buf, result);
#endif
  
  return result;
}

int dlp_ResetSystem(int sd)
{
  int result;

  Trace(ResetSystem);
  
  result = dlp_exec(sd, 0x29, 0, 0, 0, 0, 0);
  
  Expect(0);

  return result;
}

int dlp_AddSyncLogEntry(int sd, char * entry)
{ 
  int result;

  Trace(AddSyncLogEntry);
  
#ifdef DLP_TRACE  
  fprintf(stderr," Wrote: Entry:\n");
  dumpdata(entry,strlen(entry));
#endif

  result = dlp_exec(sd, 0x2A, 0x20, (unsigned char*)entry, strlen(entry), 0, 0);

  Expect(0);
  
  return result;
}

int dlp_ReadOpenDBInfo(int sd, int dbhandle, int * records)
{
  unsigned char buf[2];
  int result;

  Trace(ReadOpenDBInfo);

#ifdef DLP_TRACE
  fprintf(stderr, " Wrote: Handle: %d\n", dbhandle);
#endif
  
  set_byte(dlp_buf, (unsigned char)dbhandle);
  result = dlp_exec(sd, 0x2B, 0x20, dlp_buf, 1, buf, 2);

  Expect(2);
  
  if (records)
    *records = get_short(buf);
      
#ifdef DLP_TRACE
  fprintf(stderr, "  Read: %d records\n", get_short(buf));
#endif
  
  return result;
}

int dlp_MoveCategory(int sd, int handle, int fromcat, int tocat)
{
  int result;
  
  set_byte(dlp_buf+0, handle);
  set_byte(dlp_buf+1, fromcat);
  set_byte(dlp_buf+2, tocat);
  set_byte(dlp_buf+3, 0);

  Trace(MoveCategory);

#ifdef DLP_TRACE  
  fprintf(stderr, " Wrote: Handle: %d, From: %d, To: %d\n", handle, fromcat, tocat);
#endif
  
  result = dlp_exec(sd, 0x2C, 0x20, dlp_buf, 4, 0, 0);
  
  Expect(0);
  
  return result;
}


int dlp_OpenConduit(int sd)
{
  int result;

  Trace(OpenConduit);

  result = dlp_exec(sd, 0x2E, 0, 0, 0, 0, 0);

  Expect(0);
  
  return result;
}

int dlp_EndOfSync(int sd, int status)
{
  int result;
  struct pi_socket * ps;
  
  set_short(dlp_buf, status);

  Trace(EndOfSync);
  
  result = dlp_exec(sd, 0x2F, 0x20, dlp_buf, 2, 0, 0);

  Expect(0);
  
  /* Messy code to set end-of-sync flag on socket 
     so pi_close won't do it for us */
  if (result == 0)
    if ( (ps = find_pi_socket(sd)) )
      ps->connected |= 2;
  
  return result;
}

int dlp_AbortSync(int sd) {
  struct pi_socket * ps;

#ifdef DLP_TRACE  
  fprintf(stderr, "DLP %d: AbortSync\nResult: Whatever\n", sd);
#endif
  
  /* Set end-of-sync flag on socket so pi_close won't do a dlp_EndOfSync */
  if ( (ps = find_pi_socket(sd)) )
    ps->connected |= 2;

  return pi_close(sd);
}

int dlp_WriteUserInfo(int sd, struct PilotUser *User)
{
  int result;

  Trace(WriteUserInfo);

#ifdef DLP_TRACE
  fprintf(stderr, " Wrote: UID: 0x%8.8lX, VID: 0x%8.8lX, PCID: 0x%8.8lX\n", 
    User->userID, User->viewerID, User->lastSyncPC);
  fprintf(stderr, "        Last sync date: %s", ctime(&User->lastSyncDate));
  fprintf(stderr, "        Successful sync date: %s", ctime(&User->succSyncDate));
  fprintf(stderr, "        User name '%s'\n", User->username);
#endif
  
  set_long(dlp_buf, User->userID);
  set_long(dlp_buf+4, User->viewerID);
  set_long(dlp_buf+8, User->lastSyncPC);
  set_date(dlp_buf+12, User->lastSyncDate);
  set_byte(dlp_buf+20, 0xff);
  set_byte(dlp_buf+21, strlen(User->username)+1);
  strcpy(dlp_buf+22, User->username);

  result = dlp_exec(sd, 0x11, 0x20, dlp_buf, 22+strlen(User->username)+1, NULL, 0);
  
  Expect(0);
  
  return result;
}
                        
int dlp_ReadUserInfo(int sd, struct PilotUser* User)
{
  int result;
  int userlen;

  Trace(ReadUserInfo);

  result = dlp_exec(sd, 0x10, 0x00, NULL, 0, dlp_buf, DLP_BUF_SIZE);

  Expect(30);

  userlen = get_byte(dlp_buf+28);

  User->userID = get_long(dlp_buf);
  User->viewerID = get_long(dlp_buf+4);
  User->lastSyncPC = get_long(dlp_buf+8);
  User->succSyncDate = get_date(dlp_buf+12);
  User->lastSyncDate = get_date(dlp_buf+20);
  User->passwordLen = get_byte(dlp_buf+29);
  memcpy(User->username, dlp_buf+30, userlen);
  User->username[userlen] = '\0';
  memcpy(User->password, dlp_buf+30+userlen, User->passwordLen);

#ifdef DLP_TRACE
  fprintf(stderr, "  Read: UID: 0x%8.8lX, VID: 0x%8.8lX, PCID: 0x%8.8lX\n", 
    User->userID, User->viewerID, User->lastSyncPC);
  fprintf(stderr, "        Last sync date: %s", ctime(&User->lastSyncDate));
  fprintf(stderr, "        Successful sync date: %s", ctime(&User->succSyncDate));
  fprintf(stderr, "        User name '%s'", User->username);
  if (User->passwordLen) {
    fprintf(stderr, ", Password of %d bytes:\n", User->passwordLen);
    dumpdata(User->password,User->passwordLen);
  }
  else
    fprintf(stderr, ", No password\n");
#endif

  return result;
}

int dlp_ResetDBIndex(int sd, int dbhandle)
{
  int result;
  
  set_byte(dlp_buf,    dbhandle);

  Trace(ResetDBIndex);

#ifdef DLP_TRACE
  fprintf(stderr, " Wrote: Handle: %d\n", dbhandle);
#endif
  
  result = dlp_exec(sd, 0x30, 0x20, dlp_buf, 1, NULL, 0);
  
  Expect(0);

  return result;
}


int dlp_ReadRecordIDList(int sd, int dbhandle, int sort, 
                         int start, int max, unsigned long * IDs)
{
  int result, i, ret;
  unsigned int nbytes;
  unsigned char * p; 
  
  set_byte(dlp_buf,    dbhandle);
  set_byte(dlp_buf+1,  sort?0x80:0);
  set_short(dlp_buf+2, start);
  set_short(dlp_buf+4, max);

  Trace(ReadRecordIDList);

#ifdef DLP_TRACE
  fprintf(stderr, " Wrote: Handle: %d, Sort: %s, Start: %d, Max: %d\n",
    dbhandle, sort ? "Yes" : "No", start, max);
#endif

  nbytes = max * 4 + 2;
  if (nbytes > DLP_BUF_SIZE)
    nbytes = DLP_BUF_SIZE;
  
  result = dlp_exec(sd, 0x31, 0x20, dlp_buf, 6, dlp_buf, nbytes);
  
  Expect(2);
  
  ret = get_short(dlp_buf);

#ifdef DLP_TRACE
  fprintf(stderr, " Read: %d IDs:\n", ret);
  dumpdata(dlp_buf+2, ret*4);
#endif

  for (i = 0, p = dlp_buf+2; i < ret; i++, p+=4)
    IDs[i] = get_long(p);
  
  return nbytes;
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

  Trace(WriteRecord);

#ifdef DLP_TRACE
  fprintf(stderr, " Wrote: Handle: %d, RecordID: 0x%8.8lX, Category: %d\n",
    dbhandle, (unsigned long)recID, catID);
  fprintf(stderr, "        Flags:");
  if (flags & dlpRecAttrDeleted)
    fprintf(stderr, " Deleted");
  if (flags & dlpRecAttrDirty)
    fprintf(stderr, " Dirty");
  if (flags & dlpRecAttrBusy)
    fprintf(stderr, " Busy");
  if (flags & dlpRecAttrSecret)
    fprintf(stderr, " Secret");
  if (flags & dlpRecAttrArchived)
    fprintf(stderr, " Archive");
  if (!flags)
    fprintf(stderr, " None");
  fprintf(stderr, " (0x%2.2X), and %d bytes of data: \n", flags, length);
  dumpdata(data, length);
#endif

  result = dlp_exec(sd, 0x21, 0x20, dlp_buf, 8+length, buf, 4);
  
  Expect(4);
  
  if(NewID)
    if(result == 4)
      *NewID = get_long(buf); /* New record ID */
    else
      *NewID = 0;

#ifdef DLP_TRACE
  fprintf(stderr, "  Read: Record ID: 0x%8.8lX\n", (unsigned long)get_long(buf));
#endif
      
  return result;
}

int dlp_DeleteRecord(int sd, int dbhandle, int all, unsigned long recID)
{
  int result;
  int flags = all ? 0x80 : 0;

  set_byte(dlp_buf, dbhandle);
  set_byte(dlp_buf+1, flags);
  set_long(dlp_buf+2, recID);
  
  Trace(DeleteRecord);

#ifdef DLP_TRACE
  fprintf(stderr, " Wrote: Handle: %d, RecordID: %8.8lX, All: %s\n",
    dbhandle, (unsigned long)recID, all ? "Yes" : "No");
#endif
  
  result = dlp_exec(sd, 0x22, 0x20, dlp_buf, 6, 0, 0);
  
  Expect(0);
  
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

  Trace(ReadResourceByType);

#ifdef DLP_TRACE
  fprintf(stderr, " Wrote: Handle: %d, Type: '%s', ID: %d\n",
    fHandle, printlong(type), id);
#endif

  result = dlp_exec(sd, 0x23, 0x21, dlp_buf, 12, dlp_buf, DLP_BUF_SIZE);
  
  Expect(10);

#ifdef DLP_TRACE
  fprintf(stderr, "  Read: Type: '%s', ID: %d, Index: %d, and %d bytes:\n",
    printlong(type), id, get_short(dlp_buf+6), result-10);
  dumpdata(dlp_buf+10, result-10);
#endif
  
  if (index)
    *index = get_short(dlp_buf+6);
  if (size)
    *size = get_short(dlp_buf+8);
  if (buffer)
    memcpy(buffer, dlp_buf+10, result-10);

  return result-10;
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

  Trace(ReadResourceByIndex);

#ifdef DLP_TRACE
  fprintf(stderr, " Wrote: Handle: %d, Index: %d\n",
    fHandle, index);
#endif

  result = dlp_exec(sd, 0x23, 0x20, dlp_buf, 8, dlp_buf, DLP_BUF_SIZE);
  
  Expect(10);

#ifdef DLP_TRACE
  fprintf(stderr, "  Read: Type: '%s', ID: %d, Index: %d, and %d bytes:\n",
    printlong(get_long(dlp_buf)), get_short(dlp_buf+4), index, result-10);
  dumpdata(dlp_buf+10, result-10);
#endif

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

int dlp_WriteResource(int sd, int dbhandle, unsigned long type, int id,
                 const unsigned char* data, int length)
{
  int result;
  
  set_byte(dlp_buf, dbhandle);
  set_byte(dlp_buf+1, 0);
  set_long(dlp_buf+2, type);
  set_short(dlp_buf+6, id);
  set_short(dlp_buf+8, length);
  memcpy(dlp_buf+10, data, length);
  
  Trace(WriteResource);

#ifdef DLP_TRACE
  fprintf(stderr, " Wrote: Type: '%s', ID: %d, and %d bytes:\n",
    printlong(type), id, length);
  dumpdata(data, length);
#endif

  result = dlp_exec(sd, 0x24, 0x20, dlp_buf, 10+length, NULL, 0);
  
  Expect(0);
  
  return result;
}

int dlp_DeleteResource(int sd, int dbhandle, int all, unsigned long restype, int resID)
{
  int result;
  int flags = all ? 0x80 : 0;

  set_byte(dlp_buf, dbhandle);
  set_byte(dlp_buf+1, flags);
  set_long(dlp_buf+2, restype);
  set_short(dlp_buf+6, resID);

  Trace(DeleteResource);

#ifdef DLP_TRACE
  fprintf(stderr, " Wrote: Type: '%s', ID: %d, All: %s\n",
    printlong(restype), resID, all ? "Yes" : "No");
#endif
  
  result = dlp_exec(sd, 0x25, 0x20, dlp_buf, 8, NULL, 0);
  
  Expect(0);
  
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
  
  Trace(ReadAppBlock);

#ifdef DLP_TRACE
  fprintf(stderr, " Wrote: Handle: %d, Offset: %d, Max Length: %d\n",
    fHandle, offset, dlen);
#endif
  
  result = dlp_exec(sd, 0x1b, 0x20, dlp_buf, 6, dlp_buf, DLP_BUF_SIZE);
  
  Expect(2);
  
  if (dbuf)
    memcpy(dbuf, dlp_buf+2, result-2);
  
#ifdef DLP_TRACE
  fprintf(stderr, "  Read: %d bytes:\n",
   result-2);
  dumpdata(dlp_buf+2, result-2);
#endif
  
  return result-2;
}

int dlp_WriteAppBlock(int sd, int fHandle, const /*@unique@*/ unsigned char* data, int length)
{
  int result;
  
  set_byte(dlp_buf, fHandle);
  set_byte(dlp_buf+1, 0x00);
  set_short(dlp_buf+2, length);
  memcpy(dlp_buf+4, data, length);

  Trace(WriteAppBlock);

#ifdef DLP_TRACE
  fprintf(stderr, " Wrote: Handle: %d, %d bytes:\n",
    fHandle, length);
  dumpdata(data, length);
#endif

  result = dlp_exec(sd, 0x1c, 0x20, dlp_buf, length+4, NULL, 0);
  
  Expect(0);
  
  return result;
}

int dlp_ReadSortBlock(int sd, int fHandle, int offset,
                           unsigned char* dbuf, int dlen)
{
  int result;
  
  set_byte(dlp_buf, fHandle);
  set_byte(dlp_buf+1, 0x00);
  set_short(dlp_buf+2, offset);
  set_short(dlp_buf+4, dlen);

  Trace(ReadSortBlock);

#ifdef DLP_TRACE
  fprintf(stderr, " Wrote: Handle: %d, Offset: %d, Max Length: %d\n",
    fHandle, offset, dlen);
#endif
  
  result = dlp_exec(sd, 0x1d, 0x20, dlp_buf, 6, dlp_buf, DLP_BUF_SIZE);

#ifdef DLP_TRACE
  fprintf(stderr, "  Read: %d bytes:\n",
   result-2);
  dumpdata(dlp_buf+2, result-2);
#endif
  
  if (dbuf)
    memcpy(dbuf, dlp_buf+2, result-2);
  return result-2;
}

int dlp_WriteSortBlock(int sd, int fHandle, const /*@unique@*/ unsigned char* data, int length)
{
  int result;
  set_byte(dlp_buf, fHandle);
  set_byte(dlp_buf+1, 0x00);
  set_short(dlp_buf+2, length);
  memcpy(dlp_buf+4, data, length);

  Trace(WriteSortBlock);

#ifdef DLP_TRACE
  fprintf(stderr, " Wrote: Handle: %d, %d bytes:\n",
    fHandle, length);
  dumpdata(data, length);
#endif

  result = dlp_exec(sd, 0x1e, 0x20, dlp_buf, length+4, NULL, 0);

  Expect(0);
  
  return result;
}

int dlp_CleanUpDatabase(int sd, int fHandle)
{
  int result;
  unsigned char handle = fHandle;

  Trace(CleanUpDatabase);

#ifdef DLP_TRACE
  fprintf(stderr, " Wrote: Handle: %d\n", fHandle);
#endif

  result = dlp_exec(sd, 0x26, 0x20, &handle, 1, NULL, 0);
  
  Expect(0);
  
  return result;
}

int dlp_ResetSyncFlags(int sd, int fHandle)
{
  int result;
  unsigned char handle = fHandle;

  Trace(ResetSyncFlags);

#ifdef DLP_TRACE
  fprintf(stderr, " Wrote: Handle: %d\n", fHandle);
#endif
  
  result = dlp_exec(sd, 0x27, 0x20, &handle, 1, NULL, 0);
  
  Expect(0);
  
  return result;
}

int dlp_ReadNextModifiedRec(int sd, int fHandle, unsigned char* buffer,
                          unsigned long* id, int* index, int* size, int* attr, int* category)
{
  unsigned char handle = fHandle;
  int result;
#ifdef DLP_TRACE
  int flags;
#endif

  Trace(ReadNextModifiedRec);

#ifdef DLP_TRACE
  fprintf(stderr, " Wrote: Handle: %d\n", fHandle);
#endif
  
  result = dlp_exec(sd, 0x1f, 0x20, &handle, 1, dlp_buf, DLP_BUF_SIZE);
  
  Expect(10);

#ifdef DLP_TRACE
  flags = get_byte(dlp_buf+8);
  fprintf(stderr, "  Read: ID: 0x%8.8lX, Index: %d, Category: %d\n        Flags:",
    (unsigned long)get_long(dlp_buf), get_short(dlp_buf+4), (int)get_byte(dlp_buf+9));
  if (flags & dlpRecAttrDeleted)
    fprintf(stderr, " Deleted");
  if (flags & dlpRecAttrDirty)
    fprintf(stderr, " Dirty");
  if (flags & dlpRecAttrBusy)
    fprintf(stderr, " Busy");
  if (flags & dlpRecAttrSecret)
    fprintf(stderr, " Secret");
  if (flags & dlpRecAttrArchived)
    fprintf(stderr, " Archive");
  if (!flags)
    fprintf(stderr, " None");
  fprintf(stderr, " (0x%2.2X), and %d bytes:\n", flags, result-10);
  dumpdata(dlp_buf+10, result-10);
#endif

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
  if (buffer)    
    memcpy(buffer, dlp_buf+10, result-10);
    
  return result-10;
}

int dlp_ReadRecordById(int sd, int fHandle, unsigned long id, unsigned char* buffer, 
                          int* index, int* size, int* attr, int* category)
{
  int result;
#ifdef DLP_TRACE
  int flags;
#endif
  
  set_byte(dlp_buf, fHandle);
  set_byte(dlp_buf+1, 0x00);
  set_long(dlp_buf+2, id);
  set_short(dlp_buf+6, 0); /* Offset into record */
  set_short(dlp_buf+8, DLP_BUF_SIZE); /* length to return */

  Trace(ReadRecordById);

#ifdef DLP_TRACE
  fprintf(stderr, " Wrote: Handle: %d, Record ID: 0x%8.8lX\n", fHandle, id);
#endif
  
  result = dlp_exec(sd, 0x20, 0x20, dlp_buf, 10, dlp_buf, DLP_BUF_SIZE);

  Expect(10);

#ifdef DLP_TRACE
  flags = get_byte(dlp_buf+8);
  fprintf(stderr, "  Read: ID: 0x%8.8lX, Index: %d, Category: %d\n        Flags:",
    (unsigned long)get_long(dlp_buf), get_short(dlp_buf+4), (int)get_byte(dlp_buf+9));
  if (flags & dlpRecAttrDeleted)
    fprintf(stderr, " Deleted");
  if (flags & dlpRecAttrDirty)
    fprintf(stderr, " Dirty");
  if (flags & dlpRecAttrBusy)
    fprintf(stderr, " Busy");
  if (flags & dlpRecAttrSecret)
    fprintf(stderr, " Secret");
  if (flags & dlpRecAttrArchived)
    fprintf(stderr, " Archive");
  if (!flags)
    fprintf(stderr, " None");
  fprintf(stderr, " (0x%2.2X), and %d bytes:\n", flags, result-10);
  dumpdata(dlp_buf+10, result-10);
#endif
  
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

int dlp_ReadRecordByIndex(int sd, int fHandle, int index, unsigned char* buffer,
                          long* id, int* size, int* attr, int* category)
{
  int result;
#ifdef DLP_TRACE
  int flags;
#endif
  
  set_byte(dlp_buf, fHandle);
  set_byte(dlp_buf+1, 0x00);
  set_short(dlp_buf+2, index);
  set_short(dlp_buf+4, 0); /* Offset into record */
  set_short(dlp_buf+6, DLP_BUF_SIZE); /* length to return */

  Trace(ReadRecordByIndex);

#ifdef DLP_TRACE
  fprintf(stderr, " Wrote: Handle: %d, Index: %d\n", fHandle, index);
#endif  

  result = dlp_exec(sd, 0x20, 0x21, dlp_buf, 8, dlp_buf, DLP_BUF_SIZE);
  
  Expect(10);
  
#ifdef DLP_TRACE
  flags = get_byte(dlp_buf+8);
  fprintf(stderr, "  Read: ID: 0x%8.8lX, Index: %d, Category: %d\n        Flags:",
    (unsigned long)get_long(dlp_buf), get_short(dlp_buf+4), (int)get_byte(dlp_buf+9));
  if (flags & dlpRecAttrDeleted)
    fprintf(stderr, " Deleted");
  if (flags & dlpRecAttrDirty)
    fprintf(stderr, " Dirty");
  if (flags & dlpRecAttrBusy)
    fprintf(stderr, " Busy");
  if (flags & dlpRecAttrSecret)
    fprintf(stderr, " Secret");
  if (flags & dlpRecAttrArchived)
    fprintf(stderr, " Archive");
  if (!flags)
    fprintf(stderr, " None");
  fprintf(stderr, " (0x%2.2X), and %d bytes:\n", flags, result-10);
  dumpdata(dlp_buf+10, result-10);
#endif

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
