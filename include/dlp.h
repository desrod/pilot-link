
#ifndef _PILOT_DLP_H_
#define _PILOT_DLP_H_

#include <sys/time.h>

/* Note: All of these functions return an integer that if greater then zero
   is the number of bytes in the result, zero if there was no result, or
   less then zero if an error occured. Any return fields will be set to zero
   if an error occurs. All calls to dlp_* functions should check for a 
   return value less then zero. */
   
struct PilotUser {
  long userID, viewerID, lastSyncPC;
  time_t succSyncDate, lastSyncDate;
  char username[128];
  int passwordLen;
  char password[128];
};

struct SysInfo {
  long ROMVersion;
  long localizationID;
  int namelength;
  char name[128];
};

enum dlpDBFlags {
	dlpDBFlagResource = 0x0001, /* Resource DB, instead of record DB */
	dlpDBFlagReadOnly = 0x0002, /* DB is read only */
	dlpDBFlagAppInfoDirty = 0x0004, /* AppInfo data has been modified */
	dlpDBFlagBackup = 0x0008, /* DB is tagged for generic backup */
	dlpDBFlagOpen = 0x8000, /* DB is currently open */
};

enum dlpRecAttributes {
	dlpRecAttrDeleted = 0x80, /* tagged for deletion during next sync */
	dlpRecAttrDirty   = 0x40, /* record modified */
	dlpRecAttrBusy    = 0x20, /* record locked  */
	dlpRecAttrSecret  = 0x10, /* record is secret*/
	dlpRecAttrArchived= 0x08, /* tagged for archival during next sync*/
};

enum dlpEndStatus {
        dlpEndCodeNormal = 0,  /* Normal */
        dlpEndCodeOutOfMemory, /* End due to low memory on Pilot */
        dlpEndCodeUserCan,     /* Cancelled by user */
        dlpEndCodeOther        /* dlpEndCodeOther and higher mean "Anything else" */
};
                                        

int dlp_GetSysDateTime(int sd, time_t * t);

  /* Get the time on the Pilot and return it as a local time_t value. */
  
int dlp_SetSysDateTime(int sd, time_t time);

  /* Set the time on the Pilot using a local time_t value. */

int dlp_ReadSysInfo(int sd, struct SysInfo * s);

  /* Read the system information block. */
  
int dlp_OpenDB(int sd, int cardno, int mode, char * name, int * dbhandle);

  /* Open a database on the Pilot. cardno is the target memory card (always
     use zero for now), mode is the access mode, and name is the ASCII name
     of the database.
     
     Mode can contain any and all of these values:  Read = 0x80
                                                    Write = 0x40
                                                    Exclusive = 0x20
                                                    ShowSecret = 0x10
  */
  
int dlp_CloseDB(int sd, int dbhandle);

  /* Close an opened database using the handle returned by OpenDB. */

int dlp_CloseDB_All(int sd);

  /* Variant of CloseDB that closes all opened databases. */

int dlp_DeleteDB(int sd, int cardno, const char * name);

  /* Delete a database.
       cardno: zero for now
       name: ascii name of DB. */
  
int dlp_CreateDB(int sd, long creator, long type, int cardno,
                 int flags, int version, const char * name, int * dbhandle);

 /* Create database */                 
                    
int dlp_ResetSystem(int sd);

 /* Require reboot of Pilot after HotSync terminates. */
 
int dlp_AddSyncLogEntry(int sd, char * entry);

 /* Add an entry into the HotSync log on the Pilot.  Move to the next line
    with \n, as usual. You may invoke this command once or more before
    calling EndOfSync, but it is not required. */

int dlp_OpenConduit(int sd);

 /* State that the conduit has been succesfully opened -- puts up a status
     message on the Pilot, no other effect as far as I know. Not required.
     */
     
int dlp_EndOfSync(int sd, int status);

 /* Terminate HotSync. Required at the end of a session. The pi_socket layer
    will call this for you if you don't.
    
    Status: dlpEndCodeNormal, dlpEndCodeOutOfMemory, dlpEndCodeUserCan, or
            dlpEndCodeOther
  */            

int dlp_AbortSync(int sd);

 /* Terminate HotSync _without_ notifying Pilot. This will cause the Pilot
    to time out, and should (if I remember right) lose any changes to
    unclosed databases. _Never_ use under ordinary circumstances. If the
    sync needs to be aborted in a reasonable manner, use EndOfSync with a
    non-zero status. */
   
int dlp_ReadOpenDBInfo(int sd, int dbhandle, int * records);

 /* Return info about an opened database. Currently the only information
    returned is the number of records in the database. */

int dlp_WriteUserInfo(int sd, struct PilotUser *User);

 /* Tell the pilot who it is. */

int dlp_ReadUserInfo(int sd, struct PilotUser *User);

 /* Ask the pilot who it is. */

int dlp_WriteRecord(int sd, unsigned char dbhandle, unsigned char flags,
                 long recID, short catID, char *data, int length, long * NewID);

 /* Write a new record to an open database. 
      Flags: 0 or dlpRecAttrSecret
      RecID: a UniqueID to use for the new record, or 0 to have the
             Pilot create an ID for you.
      CatID: the category of the record
      data:  the record contents
      length: length of record. If -1, then strlen will be used on data
      
      NewID: storage for returned ID, or null. */

int dlp_WriteResource(int sd, unsigned char dbhandle, long type, int id,
                 const char *data, int length);
                  
#endif /*_PILOT_DLP_H_*/
