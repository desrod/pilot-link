
#ifndef _PILOT_DLP_H_
#define _PILOT_DLP_H_

/* Note: All of these functions return an integer that if greater then zero
   is the number of bytes in the result, zero if there was no result, or
   less then zero if an error occured. Any return fields will be set to zero
   if an error occurs. All calls to dlp_* functions should check for a 
   return value less then zero. */
   
struct PilotUser {
  long id1, id2, id3;
  char username[128];
};

int dlp_GetSysDateTime(int sd, time_t * t);

  /* Get the time on the Pilot and return it as a local time_t value. */
  
int dlp_SetSysDateTime(int sd, time_t time);

  /* Set the time on the Pilot using a local time_t value. */
  
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
    will call this for you if you don't. */
   
int dlp_ReadOpenDBInfo(int sd, int dbhandle, int * records);

 /* Return info about an opened database. Currently the only information
    returned is the number of records in the database. */

int dlp_WriteUser(int sd, struct PilotUser *User, time_t tm);

 /* Tell the pilot who it is. */

int dlp_ReadUser(int sd, struct PilotUser *User);

 /* Ask the pilot who it is. */

int dlp_WriteRec(int sd, unsigned char dbhandle, unsigned char flags,
                 long recID, short catID, char *text);

 /* Write a new record to a database */


#endif /*_PILOT_DLP_H_*/
