
#ifndef _PILOT_DLP_H_
#define _PILOT_DLP_H_

#include <sys/time.h>

/* Note: All of these functions return an integer that if greater then zero
   is the number of bytes in the result, zero if there was no result, or
   less then zero if an error occured. Any return fields will be set to zero
   if an error occurs. All calls to dlp_* functions should check for a 
   return value less then zero. */
   
struct PilotUser {
  unsigned long userID, viewerID, lastSyncPC;
  time_t succSyncDate, lastSyncDate;
  char username[128];
  int passwordLen;
  char password[128];
};

struct SysInfo {
  unsigned long ROMVersion;
  unsigned long localizationID;
  int namelength;
  char name[128];
};

struct DBInfo {
  int more;
  unsigned int flags;
  unsigned long type,creator;
  unsigned int version;
  unsigned long modnum;
  time_t crdate,moddate,backupdate;
  unsigned int index;
  char name[34];
};

struct CardInfo {
	int cardno;
	int version;
	time_t creation;
	unsigned long ROMsize, RAMsize, RAMfree;
	char name[128];
	char manuf[128];
	
	int more;
};

enum dlpDBFlags {
	dlpDBFlagResource = 0x0001, /* Resource DB, instead of record DB */
	dlpDBFlagReadOnly = 0x0002, /* DB is read only */
	dlpDBFlagAppInfoDirty = 0x0004, /* AppInfo data has been modified */
	dlpDBFlagBackup = 0x0008, /* DB is tagged for generic backup */
	dlpDBFlagOpen = 0x8000 /* DB is currently open */
};

enum dlpRecAttributes {
	dlpRecAttrDeleted = 0x80, /* tagged for deletion during next sync */
	dlpRecAttrDirty   = 0x40, /* record modified */
	dlpRecAttrBusy    = 0x20, /* record locked  */
	dlpRecAttrSecret  = 0x10, /* record is secret*/
	dlpRecAttrArchived= 0x08 /* tagged for archival during next sync*/
};

enum dlpOpenFlags {
        dlpOpenRead = 0x80,
        dlpOpenWrite = 0x40,
        dlpOpenExclusive = 0x20,
        dlpOpenSecret = 0x10,
        dlpOpenReadWrite = 0xC0
};

enum dlpEndStatus {
        dlpEndCodeNormal = 0,  /* Normal */
        dlpEndCodeOutOfMemory, /* End due to low memory on Pilot */
        dlpEndCodeUserCan,     /* Cancelled by user */
        dlpEndCodeOther        /* dlpEndCodeOther and higher mean "Anything else" */
};

enum dlpDBList {
	dlpDBListRAM = 0x80,
	dlpDBListROM = 0x40
};

enum dlpErrors { 
  dlpErrNoError = -1,
  dlpErrSystem  = -2,
  dlpErrMemory  = -3,
  dlpErrParam   = -4,
  dlpErrNotFound = -5,
  dlpErrNoneOpen = -6,
  dlpErrAlreadyOpen = -7,
  dlpErrTooManyOpen = -8,
  dlpErrExists = -9,
  dlpErrOpen = -10,
  dlpErrDeleted = -11,
  dlpErrBusy = -12,
  dlpErrNotSupp = -13,
  dlpErrUnused1 = -14,
  dlpErrReadOnly = -15,
  dlpErrSpace = -16,
  dlpErrLimit = -17,
  dlpErrSync = -18,
  dlpErrWrapper = -19,
  dlpErrArgument = -20,
  dlpErrSize = -21,
  dlpErrUnknown = -128
};

extern int dlp_GetSysDateTime(int sd, time_t * t);

  /* Get the time on the Pilot and return it as a local time_t value. */
  
extern int dlp_SetSysDateTime(int sd, time_t time);

  /* Set the time on the Pilot using a local time_t value. */
  
extern int dlp_ReadStorageInfo(int sd, int cardno, struct CardInfo * c);


extern int dlp_ReadSysInfo(int sd, struct SysInfo * s);

  /* Read the system information block. */

extern int dlp_ReadDBList(int sd, int cardno, int flags, int start, struct DBInfo * info);

  /* flags must contain dlpDBListRAM and/or dlpDBListROM */

extern int dlp_FindDBInfo(int sd, int cardno, int start, char * dbname, unsigned long type,
                               unsigned long creator, struct DBInfo * info);
  
extern int dlp_OpenDB(int sd, int cardno, int mode, char * name, int * dbhandle);

  /* Open a database on the Pilot. cardno is the target memory card (always
     use zero for now), mode is the access mode, and name is the ASCII name
     of the database.
     
     Mode can contain any and all of these values:  Read = 0x80
                                                    Write = 0x40
                                                    Exclusive = 0x20
                                                    ShowSecret = 0x10
  */
  
extern int dlp_CloseDB(int sd, int dbhandle);

  /* Close an opened database using the handle returned by OpenDB. */

extern int dlp_CloseDB_All(int sd);

  /* Variant of CloseDB that closes all opened databases. */

extern int dlp_DeleteDB(int sd, int cardno, const char * name);

  /* Delete a database.
       cardno: zero for now
       name: ascii name of DB. */
  
extern int dlp_CreateDB(int sd, long creator, long type, int cardno,
                 int flags, int version, const char * name, int * dbhandle);

 /* Create database */                 
                    
extern int dlp_ResetSystem(int sd);

 /* Require reboot of Pilot after HotSync terminates. */
 
extern int dlp_AddSyncLogEntry(int sd, char * entry);

 /* Add an entry into the HotSync log on the Pilot.  Move to the next line
    with \n, as usual. You may invoke this command once or more before
    calling EndOfSync, but it is not required. */

extern int dlp_OpenConduit(int sd);

 /* State that the conduit has been succesfully opened -- puts up a status
     message on the Pilot, no other effect as far as I know. Not required.
     */
     
extern int dlp_EndOfSync(int sd, int status);

 /* Terminate HotSync. Required at the end of a session. The pi_socket layer
    will call this for you if you don't.
    
    Status: dlpEndCodeNormal, dlpEndCodeOutOfMemory, dlpEndCodeUserCan, or
            dlpEndCodeOther
  */            

extern int dlp_AbortSync(int sd);

 /* Terminate HotSync _without_ notifying Pilot. This will cause the Pilot
    to time out, and should (if I remember right) lose any changes to
    unclosed databases. _Never_ use under ordinary circumstances. If the
    sync needs to be aborted in a reasonable manner, use EndOfSync with a
    non-zero status. */
   
extern int dlp_ReadOpenDBInfo(int sd, int dbhandle, int * records);

 /* Return info about an opened database. Currently the only information
    returned is the number of records in the database. */


extern int dlp_MoveCategory(int sd, int handle, int fromcat, int tocat);

extern int dlp_WriteUserInfo(int sd, struct PilotUser *User);

 /* Tell the pilot who it is. */

extern int dlp_ReadUserInfo(int sd, struct PilotUser *User);

 /* Ask the pilot who it is. */
 
extern int dlp_ResetLastSyncPC(int sd);

 /* Convenience function to reset lastSyncPC in the UserInfo to 0 */

extern int dlp_ReadAppBlock(int sd, int fHandle, int offset,
                           unsigned char *dbuf, int dlen);

extern int dlp_WriteAppBlock(int sd, int fHandle, const unsigned char *dbuf,
                            int dlen);

extern int dlp_ReadSortBlock(int sd, int fHandle, int offset,
                           unsigned char *dbuf, int dlen);

extern int dlp_WriteSortBlock(int sd, int fHandle, const unsigned char *dbuf,
                            int dlen);

extern int dlp_ResetDBIndex(int sd, int dbhandle);

 /* Reset NextModified position to beginning */

extern int dlp_ReadRecordIDList(int sd, int dbhandle, int sort,
                         int start, int max, unsigned long * IDs);
                         
extern int dlp_WriteRecord(int sd, int dbhandle, int flags,
                 unsigned long recID, int catID, unsigned char *data, int length, long * NewID);

 /* Write a new record to an open database. 
      Flags: 0 or dlpRecAttrSecret
      RecID: a UniqueID to use for the new record, or 0 to have the
             Pilot create an ID for you.
      CatID: the category of the record
      data:  the record contents
      length: length of record. If -1, then strlen will be used on data
      
      NewID: storage for returned ID, or null. */

extern int dlp_DeleteRecord(int sd, int dbhandle, int all, unsigned long recID);

extern int dlp_ReadResourceByType(int sd, int fHandle, unsigned long type, int id, unsigned char* buffer, 
                          int* index, int* size);

extern int dlp_ReadResourceByIndex(int sd, int fHandle, int index, unsigned char* buffer,
                          unsigned long* type, int * id, int* size);

extern int dlp_WriteResource(int sd, int dbhandle, unsigned long type, int id,
                 const unsigned char *data, int length);

extern int dlp_DeleteResource(int sd, int dbhandle, int all, unsigned long restype, int resID);

extern int dlp_ReadNextModifiedRec(int sd, int fHandle, unsigned char *buffer,
                          unsigned long * id, int * index, int * size, int * attr, int * category);

extern int dlp_ReadRecordById(int sd, int fHandle, unsigned long id, unsigned char * buffer, 
                          int * index, int * size, int * attr, int * category);

extern int dlp_ReadRecordByIndex(int sd, int fHandle, int index, unsigned char * buffer, 
                          long * id, int * size, int * attr, int * category);

extern int dlp_CleanUpDatabase(int sd, int fHandle);

  /* Deletes all records in the opened database which are marked as archived
     or deleted. */

extern int dlp_ResetSyncFlags(int sd, int fHandle);

  /* For record databases, reset all dirty flags. For both record and
     resource databases, set the last sync time to now. */

extern int dlp_CallApplication(int sd, unsigned long creator, int action,
                        int length, unsigned char * data,
                        int * resultptr,
                        int * retlen, unsigned char * retdata);
                  
#endif /*_PILOT_DLP_H_*/
