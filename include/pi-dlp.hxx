#ifndef _PI_DLP_HXX
#define _PI_DLP_HXX

typedef const char *const strConst_t;

extern "C" {
#include "pi-dlp.h"
}

class DLP 
{
     int _sd;		// The socket from pi_accept
     
   public:
     DLP(const int s) : _sd(s) {}
     DLP(strConst_t, const int = 1);
     ~DLP() { /* Do Nothing */ }

     const int sd(void) const { return _sd; }
     strConst_t strerror(const int e) const { return dlp_strerror(e); }

     // Get the time on the pilot and return it as a local time_t value
     int getSysDateTime(time_t *t) const { return dlp_GetSysDateTime(_sd, t); }

     // Set the time on the pilot using a local time_t value
     int setSysDateTime(time_t t) const { return dlp_SetSysDateTime(_sd, t); }

     int readStorageInfo(const int cardno, struct CardInfo *c) const {
	  return dlp_ReadStorageInfo(_sd, cardno, c);
     }

     // Read the system information block
     int readSysInfo(struct SysInfo *s) const {
	  return dlp_ReadSysInfo(_sd, s);
     }

     // Flags must contain dlpDBListRAM and/or dlpDBListROM
     int findDBInfo(const int cardno, const int start,
		    strConst_t dbname, const unsigned long type,
		    const unsigned long creator, struct DBInfo *info) const {
	  return dlp_FindDBInfo(_sd, cardno, start, (char*)dbname, type, creator,
				info);
     }

     /*
      * Open a database on the pilot.  Cardno is the target memory card (always
      * use zero for now), mode is the access mode, and name is the ASCII name
      * of the database.
      */
     int openDB(const int cardno, const int mode, strConst_t name,
		int *db) const {

	  return dlp_OpenDB(_sd, cardno, mode, (char*)name, db);
     }

     // Close an opened database using the handle returned by openDB
     int closeDB(const int db) const { return dlp_CloseDB(_sd, db); }

     // Close all opened databases
     int closeDB_All(void) const { return dlp_CloseDB_All(_sd); }

     // Delete a database.  Use cardno as zero for now
     int deleteDB(const int cardno, strConst_t name) const {
	  return dlp_DeleteDB(_sd, cardno, name);
     }

     // Create a database
     int createDB(const long creator, const long type, const int cardno,
		  const int flags, const int version, strConst_t name,
		  int *db) const {
	  return dlp_CreateDB(_sd, creator, type, cardno, flags, version, name,
			      db);
     }

     // Require a reboot of the pilot after a HotSync terminates
     int resetSystem(void) const { return dlp_ResetSystem(_sd); }

     /*
      * Add an entry to the HotSync log.  Move to the next line with \n as
      * usual.  You may invoke this command as many times as desired before
      * calling endOfSync, but it's not required
      */
     int addSyncLogEntry(strConst_t entry) const {
	  return dlp_AddSyncLogEntry(_sd, (char*)entry);
     }

     /*
      * States that the conduit has been succesfully opened.  It puts up a
      * status message on the pilot.  Not requried.  Should be used though.
      */
     int openConduit(void) const { return dlp_OpenConduit(_sd); }

     // Terminate the HotSync.  You must call this at the end of your session.
     int endOfSync(const int status) const {
	  return dlp_EndOfSync(_sd, status);
     }

     /*
      * Terminate HotSync _without_ notifying Pilot.  This will cause the Pilot
      * to time out, and should (if I remember right) lose any changes to
      * unclosed databases. _Never_ use under ordinary circumstances. If the
      * sync needs to be aborted in a reasonable manner, use EndOfSync with a
      * non-zero status.
      */
     int abortSync(void) const { return dlp_AbortSync(_sd); }

     /*
      * Returns info about an _opened_ database.  Currently, the only
      * information returned is the number of records in the database.
      */
     int readOpenDBInfo(const int db, int *records) const {
	  return dlp_ReadOpenDBInfo(_sd, db, records);
     }

     int moveCategory(const int handle, const int from, const int to) const {
	  return dlp_MoveCategory(_sd, handle, from, to);
     }

     int writeUserInfo(PilotUser *u) const { return dlp_WriteUserInfo(_sd,u); }
     int readUserInfo(PilotUser *u) const { return dlp_ReadUserInfo(_sd, u); }

     int resetLastSyncPC(void) const { return dlp_ResetLastSyncPC(_sd); }

     int readAppBlock(const int handle, const int offset, int reqbytes, pi_buffer_t *retbuf) const {
	  return dlp_ReadAppBlock(_sd, handle, offset, reqbytes, retbuf);
     }

     int writeAppBlock(const int h, const void *buf, const int len) const {
	  return dlp_WriteAppBlock(_sd, h, buf, len);
     }

     int readSortBlock(const int h, const int offset, int reqbytes, pi_buffer_t *retbuf) const {
	  return dlp_ReadSortBlock(_sd, h, offset, reqbytes, retbuf);
     }

     int writeSortBlock(const int h, const void *buf, int len) const {
	  return dlp_WriteSortBlock(_sd, h, buf, len);
     }

     int resetDBIndex(const int handle) const {
	  return dlp_ResetDBIndex(_sd, handle);
     }

     int readRecordIDList(const int handle, const int sort, const int start,
			  const int max, recordid_t *ids, int *count) const {
	  return dlp_ReadRecordIDList(_sd, handle, sort, start, max, ids, count);
     }

     /* Write a new record to an open database. 
      * Flags: 0 or dlpRecAttrSecret
      * RecID: a UniqueID to use for the new record, or 0 to have the
      *        Pilot create an ID for you.
      * CatID: the category of the record
      * data:  the record contents
      * length: length of record. If -1, then strlen will be used on data
      *
      * NewID: storage for returned ID, or null.
      */
     int writeRecord(const int handle, const int flags, const recordid_t recID,
		     const int catID, const void *data, const int length,
		     recordid_t *newID) const {
	  return dlp_WriteRecord(_sd, handle, flags, recID, catID, (void*)data,
				 length, newID);
     }

     int deleteRecord(const int handle, const int all, const recordid_t recID) const {
	  return dlp_DeleteRecord(_sd, handle, all, recID);
     }

     int deleteCategory(const int handle, const int category) const {
	  return dlp_DeleteCategory(_sd, handle, category);
     }

     int readResourceByType(const int handle, const unsigned long type,
			    const int id, pi_buffer_t *buffer, int *idx) const {
	  return dlp_ReadResourceByType(_sd, handle, type, id, buffer, idx);
     }

     int readResourceByIndex(const int handle, const int idx, pi_buffer_t *buffer,
			     unsigned long *type, int *id) const {
	  return dlp_ReadResourceByIndex(_sd, handle, idx, buffer, type, id);
     }

     int writeResource(const int handle, const unsigned long type,
		       const int id, const void *data, const int len) const {
	  return dlp_WriteResource(_sd, handle, type, id, data, len);
     }

     int deleteResource(const int handle, const int all,
			const unsigned long restype, const int resID) const {
	  return dlp_DeleteResource(_sd, handle, all, restype, resID);
     }

     int readNextModifiedRec(const int handle, pi_buffer_t *buffer, recordid_t *id,
			     int *idx, int *attr, int *cat) const {
	  return dlp_ReadNextModifiedRec(_sd, handle, buffer, id, idx, attr, cat);
     }

     int readNextModifiedRecInCategory(const int handle, const int incat,
				       pi_buffer_t *buffer, recordid_t *id, int *idx,
				       int *attr) const {
	  return dlp_ReadNextModifiedRecInCategory(_sd, handle, incat, buffer,
						   id, idx, attr);
     }

     int readNextRecInCategory(const int handle, const int incat, pi_buffer_t *buffer,
			       recordid_t *id, int *idx,
			       int *attr) const {
	  return dlp_ReadNextRecInCategory(_sd, handle, incat, buffer, id, idx, attr);
     }

     int readRecordById(const int handle, const recordid_t id, pi_buffer_t *buffer,
			int *idx, int *attr, int *category) const {
	  return dlp_ReadRecordById(_sd, handle, id, buffer, idx, attr,
				    category);
     }

     int readRecordByIndex(const int handle, const int idx, pi_buffer_t *buffer,
			   recordid_t *id, int *attr,
			   int *category) const {
	  return dlp_ReadRecordByIndex(_sd, handle, idx, buffer, id,
				       attr, category);
     }

     // Deletes all records in the opened database marked archived or deleted
     int cleanUpDatabase(const int handle) const {
	  return dlp_CleanUpDatabase(_sd, handle);
     }
     
     /*
      * For record databases, reset all dirty flags.  For both record and
      * resource databases, set the last sync time to now.
      */
     int resetSyncFlags(const int handle) const {
	  return dlp_ResetSyncFlags(_sd, handle);
     }

     // 32-bit retcode and data over 64K only supported on v2.0 pilots
     int callApplication(const unsigned long creator,
			 const unsigned long type, const int action,
			 const int length, void *data,
			 unsigned long *retcode,
			 pi_buffer_t *retdata) const {
	  return dlp_CallApplication(_sd, creator, type, action, length, data,
				     retcode, retdata);
     }

     int readFeature(const unsigned long creator, const unsigned int num,
		     unsigned long *feature) const {
	  return dlp_ReadFeature(_sd, creator, num, feature);
     }

     // PalmOS 2.0 and above only
     int readNetSyncInfo(NetSyncInfo *i) const {
	  return dlp_ReadNetSyncInfo(_sd, i);
     }

     // PlamOS 2.0 and above only
     int writeNetSyncInfo(NetSyncInfo *i) const {
	  return dlp_WriteNetSyncInfo(_sd, i);
     }

     int readAppPreference(const unsigned long creator, const int id,
			   const int backup, const int maxsize, void *buffer,
			   size_t *size, int *version) const {
	  return dlp_ReadAppPreference(_sd, creator, id, backup, maxsize,
				       buffer, size, version);
     }

     int writeAppPreference(const unsigned long creator, const int id,
			    const int backup, const int version, void *buffer,
			    int size) const {
	  return dlp_WriteAppPreference(_sd, creator, id, backup, version,
					buffer, size);
     }

     int RPC(RPC_params *p, unsigned long *result) const {
	  return dlp_RPC(_sd, p, result);
     }
};

#endif // _PI_DLP_HXX
