/* 
 * pi-dlp.h: Desktop Link Protocol implementation (ala SLP)
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. *
 */

#ifndef _PILOT_DLP_H_
#define _PILOT_DLP_H_

#include "pi-args.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "pi-macros.h"		/* For recordid_t */

#define DLP_BUF_SIZE 0xffff

	/* Note: All of these functions return an integer that if greater
	   then zero is the number of bytes in the result, zero if there was
	   no result, or less then zero if an error occured. Any return
	   fields will be set to zero if an error occurs. All calls to dlp_*
	   functions should check for a return value less then zero.
	 */
	struct PilotUser {
		unsigned long userID, viewerID, lastSyncPC;
		time_t successfulSyncDate, lastSyncDate;
		char username[128];
		int passwordLength;
		char password[128];
	};

	struct SysInfo {
		unsigned long romVersion;
		unsigned long locale;
		int nameLength;
		char name[128];
	};

	struct DBInfo {
		int more;
		unsigned int flags;
		unsigned int miscFlags;
		unsigned long type, creator;
		unsigned int version;
		unsigned long modnum;
		time_t createDate, modifyDate, backupDate;
		unsigned int index;
		char name[34];
	};

	struct CardInfo {
		int card;
		int version;
		time_t creation;
		unsigned long romSize, ramSize, ramFree;
		char name[128];
		char manufacturer[128];

		int more;
	};

	struct NetSyncInfo {
		int lanSync;
		char hostName[256];			/* Null terminated string */
		char hostAddress[40];			/* Null terminated string */
		char hostSubnetMask[40];		/* Null terminated string */
	};

	enum dlpDBFlags {
		dlpDBFlagResource 	= 0x0001,	/* Resource DB, instead of record DB            */
		dlpDBFlagReadOnly 	= 0x0002,	/* DB is read only                              */
		dlpDBFlagAppInfoDirty 	= 0x0004,	/* AppInfo data has been modified               */
		dlpDBFlagBackup 	= 0x0008,	/* DB is tagged for generic backup              */
		dlpDBFlagClipping 	= 0x0200,	/* DB is a Palm Query Application (PQA)         */
		dlpDBFlagOpen 		= 0x8000,	/* DB is currently open                         */

		/* v2.0 specific */
		dlpDBFlagNewer 		= 0x0010,	/* Newer version may be installed over open DB  */
		dlpDBFlagReset 		= 0x0020,	/* Reset after installation                     */

		/* v3.0 specific */
		dlpDBFlagCopyPrevention = 0x0040,	/* DB should not be beamed                      */
		dlpDBFlagStream 	= 0x0080	/* DB implements a file stream                  */
	};

	enum dlpDBMiscFlags {
		dlpDBMiscFlagExcludeFromSync = 0x80
	};

	enum dlpRecAttributes {
		dlpRecAttrDeleted 	= 0x80,		/* tagged for deletion during next sync         */
		dlpRecAttrDirty 	= 0x40,		/* record modified                              */
		dlpRecAttrBusy 		= 0x20,		/* record locked                                */
		dlpRecAttrSecret 	= 0x10,		/* record is secret                             */
		dlpRecAttrArchived 	= 0x08		/* tagged for archival during next sync         */
	};

	enum dlpOpenFlags {
		dlpOpenRead = 0x80,
		dlpOpenWrite = 0x40,
		dlpOpenExclusive = 0x20,
		dlpOpenSecret = 0x10,
		dlpOpenReadWrite = 0xC0
	};

	enum dlpEndStatus {
		dlpEndCodeNormal 	= 0,		/* Normal					*/
		dlpEndCodeOutOfMemory,			/* End due to low memory on Palm		*/
		dlpEndCodeUserCan,			/* Cancelled by user				*/
		dlpEndCodeOther				/* dlpEndCodeOther and higher mean "Anything else"	*/
	};

	enum dlpDBList {
		dlpDBListRAM 		= 0x80,
		dlpDBListROM 		= 0x40
	};

	enum dlpErrors {
		dlpErrNoError 		= -1,
		dlpErrSystem 		= -2,
		dlpErrMemory 		= -3,
		dlpErrParam 		= -4,
		dlpErrNotFound 		= -5,
		dlpErrNoneOpen 		= -6,
		dlpErrAlreadyOpen 	= -7,
		dlpErrTooManyOpen 	= -8,
		dlpErrExists 		= -9,
		dlpErrOpen 		= -10,
		dlpErrDeleted 		= -11,
		dlpErrBusy 		= -12,
		dlpErrNotSupp 		= -13,
		dlpErrUnused1 		= -14,
		dlpErrReadOnly 		= -15,
		dlpErrSpace 		= -16,
		dlpErrLimit 		= -17,
		dlpErrSync 		= -18,
		dlpErrWrapper 		= -19,
		dlpErrArgument 		= -20,
		dlpErrSize 		= -21,
		dlpErrUnknown 		= -128
	};

	extern char *dlp_errorlist[];
	extern char *dlp_strerror(int error);

	/* Get the time on the Palm and return it as a local time_t value. */ 
	extern int dlp_GetSysDateTime PI_ARGS((int sd, time_t * t));

	/* Set the time on the Palm using a local time_t value. */
	extern int dlp_SetSysDateTime PI_ARGS((int sd, time_t time));

	extern int dlp_ReadStorageInfo
	    PI_ARGS((int sd, int cardno, struct CardInfo * c));

	/* Read the system information block. */
	extern int dlp_ReadSysInfo PI_ARGS((int sd, struct SysInfo * s));

	/* flags must contain dlpDBListRAM and/or dlpDBListROM */
	extern int dlp_ReadDBList
	    PI_ARGS((int sd, int cardno, int flags, int start,
		     struct DBInfo * info));

	extern int dlp_FindDBInfo
	    PI_ARGS((int sd, int cardno, int start, char *dbname,
		     unsigned long type, unsigned long creator,
		     struct DBInfo * info));

	/* Open a database on the Palm. cardno is the target memory card
	   (always use zero for now), mode is the access mode, and name is
	   the ASCII name of the database.

	   Mode can contain any and all of these values:
	   Read = 0x80
	   Write = 0x40
	   Exclusive = 0x20
	   ShowSecret = 0x10
	 */
	extern int dlp_OpenDB
	    PI_ARGS((int sd, int cardno, int mode, char *name,
		     int *dbhandle));

	/* Close an opened database using the handle returned by OpenDB. */
	extern int dlp_CloseDB PI_ARGS((int sd, int dbhandle));

	/* Variant of CloseDB that closes all opened databases. */
	extern int dlp_CloseDB_All PI_ARGS((int sd));

	/* Delete a database. cardno: zero for now name: ascii name of DB. */
	extern int dlp_DeleteDB
	    PI_ARGS((int sd, int cardno, PI_CONST char *name));

	/* Create database */
	extern int dlp_CreateDB
	    PI_ARGS((int sd, long creator, long type, int cardno,
		     int flags, int version, PI_CONST char *name,
		     int *dbhandle));

	/* Require reboot of Palm after HotSync terminates. */
	extern int dlp_ResetSystem PI_ARGS((int sd));

	/* Add an entry into the HotSync log on the Palm.  Move to the next
	   line with \n, as usual. You may invoke this command once or more
	   before calling EndOfSync, but it is not required.
	 */
	extern int dlp_AddSyncLogEntry PI_ARGS((int sd, char *entry));

	/* State that the conduit has been succesfully opened -- puts up a status
	   message on the Palm, no other effect as far as I know. Not required.
	 */
	extern int dlp_OpenConduit PI_ARGS((int sd));

	/* Terminate HotSync. Required at the end of a session. The pi_socket layer
	   will call this for you if you don't.

	   Status: dlpEndCodeNormal, dlpEndCodeOutOfMemory, dlpEndCodeUserCan, or
	   dlpEndCodeOther
	 */
	extern int dlp_EndOfSync PI_ARGS((int sd, int status));


	/* Terminate HotSync _without_ notifying Palm. This will cause the
	   Palm to time out, and should (if I remember right) lose any
	   changes to unclosed databases. _Never_ use under ordinary
	   circumstances. If the sync needs to be aborted in a reasonable
	   manner, use EndOfSync with a non-zero status.
	 */
	extern int dlp_AbortSync PI_ARGS((int sd));

	/* Return info about an opened database. Currently the only information
	   returned is the number of records in the database. 
	 */
	extern int dlp_ReadOpenDBInfo
	    PI_ARGS((int sd, int dbhandle, int *records));

	extern int dlp_MoveCategory
	    PI_ARGS((int sd, int handle, int fromcat, int tocat));

	/* Tell the pilot who it is. */
	extern int dlp_WriteUserInfo
	    PI_ARGS((int sd, struct PilotUser * User));

	/* Ask the pilot who it is. */
	extern int dlp_ReadUserInfo
	    PI_ARGS((int sd, struct PilotUser * User));

	/* Convenience function to reset lastSyncPC in the UserInfo to 0 */
	extern int dlp_ResetLastSyncPC PI_ARGS((int sd));

	extern int dlp_ReadAppBlock
	    PI_ARGS((int sd, int fHandle, int offset, void *dbuf,
		     int dlen));

	extern int dlp_WriteAppBlock
	    PI_ARGS((int sd, int fHandle, PI_CONST void *dbuf, int dlen));

	extern int dlp_ReadSortBlock
	    PI_ARGS((int sd, int fHandle, int offset, void *dbuf,
		     int dlen));

	extern int dlp_WriteSortBlock
	    PI_ARGS((int sd, int fHandle, PI_CONST void *dbuf, int dlen));

	/* Reset NextModified position to beginning */
	extern int dlp_ResetDBIndex PI_ARGS((int sd, int dbhandle));

	extern int dlp_ReadRecordIDList
	    PI_ARGS((int sd, int dbhandle, int sort, int start, int max,
		     recordid_t * IDs, int *count));

	/* Write a new record to an open database.  
	   Flags: 0 or dlpRecAttrSecret 
	   RecID: a UniqueID to use for the new record, or 0 to have the
	          Palm create an ID for you.
	   CatID: the category of the record data: the record contents
	          length: length of record.
	   If -1, then strlen will be used on data 
	   NewID: storage for returned ID, or null.  
	 */

	extern int dlp_WriteRecord
	    PI_ARGS((int sd, int dbhandle, int flags, recordid_t recID,
		     int catID, void *data, int length,
		     recordid_t * NewID));

	extern int dlp_DeleteRecord
	    PI_ARGS((int sd, int dbhandle, int all, recordid_t recID));

	extern int dlp_DeleteCategory
	    PI_ARGS((int sd, int dbhandle, int category));

	extern int dlp_ReadResourceByType
	    PI_ARGS((int sd, int fHandle, unsigned long type, int id,
		     void *buffer, int *index, int *size));

	extern int dlp_ReadResourceByIndex
	    PI_ARGS((int sd, int fHandle, int index, void *buffer,
		     unsigned long *type, int *id, int *size));

	extern int dlp_WriteResource
	    PI_ARGS((int sd, int dbhandle, unsigned long type, int id,
		     PI_CONST void *data, int length));

	extern int dlp_DeleteResource
	    PI_ARGS((int sd, int dbhandle, int all, unsigned long restype,
		     int resID));

	extern int dlp_ReadNextModifiedRec
	    PI_ARGS((int sd, int fHandle, void *buffer, recordid_t * id,
		     int *index, int *size, int *attr, int *category));

	extern int dlp_ReadNextModifiedRecInCategory
	    PI_ARGS((int sd, int fHandle, int incategory, void *buffer,
		     recordid_t * id, int *index, int *size, int *attr));

	extern int dlp_ReadNextRecInCategory
	    PI_ARGS((int sd, int fHandle, int incategory, void *buffer,
		     recordid_t * id, int *index, int *size, int *attr));

	extern int dlp_ReadRecordById
	    PI_ARGS((int sd, int fHandle, recordid_t id, void *buffer,
		     int *index, int *size, int *attr, int *category));

	extern int dlp_ReadRecordByIndex
	    PI_ARGS((int sd, int fHandle, int index, void *buffer,
		     recordid_t * id, int *size, int *attr,
		     int *category));

	/* Deletes all records in the opened database which are marked as
	   archived or deleted.
	 */
	extern int dlp_CleanUpDatabase PI_ARGS((int sd, int fHandle));

	/* For record databases, reset all dirty flags. For both record and
	   resource databases, set the last sync time to now.
	 */
	extern int dlp_ResetSyncFlags PI_ARGS((int sd, int fHandle));


	/* 32-bit retcode and data over 64K only supported on v2.0 Palms */
	extern int dlp_CallApplication
	    PI_ARGS((int sd, unsigned long creator, unsigned long type,
		     int action, int length, void *data,
		     unsigned long *retcode, int maxretlen, int *retlen,
		     void *retdata));

	extern int dlp_ReadFeature
	    PI_ARGS((int sd, unsigned long creator, unsigned int num,
		     unsigned long *feature));

	/* PalmOS 2.0 only */
	extern int dlp_ReadNetSyncInfo
	    PI_ARGS((int sd, struct NetSyncInfo * i));

	/* PalmOS 2.0 only */
	extern int dlp_WriteNetSyncInfo
	    PI_ARGS((int sd, struct NetSyncInfo * i));

	extern int dlp_ReadAppPreference
	    PI_ARGS((int sd, unsigned long creator, int id, int backup,
		     int maxsize, void *buffer, int *size, int *version));

	extern int dlp_WriteAppPreference
	    PI_ARGS((int sd, unsigned long creator, int id, int backup,
		     int version, void *buffer, int size));

	struct RPC_params;

	extern int dlp_RPC
	    PI_ARGS((int sd, struct RPC_params * p,
		     unsigned long *result));

#ifdef __cplusplus
}
#endif
#endif				/*_PILOT_DLP_H_*/
