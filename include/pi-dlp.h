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

#define PI_DLP_OFFSET_CMD  0
#define PI_DLP_OFFSET_ARGC 1
#define PI_DLP_OFFSET_ARGV 2
#define PI_DLP_OFFSET_ARGV 2

#define PI_DLP_ARG_TINY_LEN  0x000000FFL
#define PI_DLP_ARG_SHORT_LEN 0x0000FFFFL
#define PI_DLP_ARG_LONG_LEN  0xFFFFFFFFL

#define PI_DLP_ARG_FLAG_TINY  0x00
#define PI_DLP_ARG_FLAG_SHORT 0x80
#define PI_DLP_ARG_FLAG_LONG  0xC0

#define PI_DLP_ARG_FIRST_ID 0x20

#define DLP_BUF_SIZE 0xffff

	/* Note: All of these functions return an integer that if greater
	   then zero is the number of bytes in the result, zero if there was
	   no result, or less then zero if an error occured. Any return
	   fields will be set to zero if an error occurs. All calls to dlp_*
	   functions should check for a return value less then zero.
	 */
	struct PilotUser {
		int 	passwordLength;
		char 	username[128],
			password[128];
		unsigned long userID, viewerID, lastSyncPC;
		time_t successfulSyncDate, lastSyncDate;
	};

	struct SysInfo {
		unsigned long romVersion;
		unsigned long locale;
		unsigned char prodIDLength;
		char 	prodID[128];
		unsigned short dlpMajorVersion;
		unsigned short dlpMinorVersion;
		unsigned short compatMajorVersion;
		unsigned short compatMinorVersion;
		unsigned long  maxRecSize;		
	};

	struct DBInfo {
		int 	more;
		char name[34];
		unsigned int flags;
		unsigned int miscFlags;
		unsigned int version;
		unsigned long type, creator;
		unsigned long modnum;
		unsigned int index;
		time_t createDate, modifyDate, backupDate;
	};

	struct CardInfo {
		int 	card,
			version;
			int more;

		time_t 	creation;

		unsigned long romSize;
		unsigned long ramSize;
		unsigned long ramFree;

		char 	name[128],
			manufacturer[128];


	};

	struct NetSyncInfo {
		int 	lanSync;
		char 	hostName[256];			/* Null terminated string */
		char 	hostAddress[40];		/* Null terminated string */
		char 	hostSubnetMask[40];		/* Null terminated string */
	};

	enum dlpFunctions {
		/* range reserved for internal use */
		dlpReservedFunc = 0x0F,

		/* DLP 1.0 FUNCTIONS START HERE (PalmOS v1.0) */
		dlpFuncReadUserInfo,
	
		dlpFuncWriteUserInfo,
	
		dlpFuncReadSysInfo,
	
		dlpFuncGetSysDateTime,
	
		dlpFuncSetSysDateTime,
	
		dlpFuncReadStorageInfo,
	
		dlpFuncReadDBList,
	
		dlpFuncOpenDB,
	
		dlpFuncCreateDB,
	
		dlpFuncCloseDB,
	
		dlpFuncDeleteDB,
	
		dlpFuncReadAppBlock,
	
		dlpFuncWriteAppBlock,

		dlpFuncReadSortBlock,

		dlpFuncWriteSortBlock,

		dlpFuncReadNextModifiedRec,

		dlpFuncReadRecord,

		dlpFuncWriteRecord,

		dlpFuncDeleteRecord,

		dlpFuncReadResource,

		dlpFuncWriteResource,

		dlpFuncDeleteResource,

		dlpFuncCleanUpDatabase,

		dlpFuncResetSyncFlags,

		dlpFuncCallApplication,

		dlpFuncResetSystem,
	
		dlpFuncAddSyncLogEntry,
	
		dlpFuncReadOpenDBInfo,
	
		dlpFuncMoveCategory,
	
		dlpProcessRPC,
	
		dlpFuncOpenConduit,
	
		dlpFuncEndOfSync,
	
		dlpFuncResetRecordIndex,
	
		dlpFuncReadRecordIDList,
	
		/* DLP 1.1 FUNCTIONS ADDED HERE (PalmOS v2.0 Personal, and Professional) */
		dlpFuncReadNextRecInCategory,
	
		dlpFuncReadNextModifiedRecInCategory,
	
		dlpFuncReadAppPreference,
	
		dlpFuncWriteAppPreference,
	
		dlpFuncReadNetSyncInfo,
	
		dlpFuncWriteNetSyncInfo,

		dlpFuncReadFeature,
	
		/* DLP 1.2 FUNCTIONS ADDED HERE (PalmOS v3.0) */
		dlpFindDB,

		dlpSetDBInfo,
	
		dlpLastFunc

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
		dlpEndCodeNormal 	= 0,		/* Normal					 */
		dlpEndCodeOutOfMemory,			/* End due to low memory on Palm		 */
		dlpEndCodeUserCan,			/* Cancelled by user				 */
		dlpEndCodeOther				/* dlpEndCodeOther and higher == "Anything else" */
	};

	enum dlpDBList {
		dlpDBListRAM 		= 0x80,
		dlpDBListROM 		= 0x40
	};

	enum dlpErrors {
		dlpErrNoError 		= 0,
		dlpErrSystem 		= -1,
		dlpErrMemory 		= -2,
		dlpErrParam 		= -3,
		dlpErrNotFound 		= -4,
		dlpErrNoneOpen 		= -5,
		dlpErrAlreadyOpen 	= -6,
		dlpErrTooManyOpen 	= -7,
		dlpErrExists 		= -8,
		dlpErrOpen 		= -8,
		dlpErrDeleted 		= -10,
		dlpErrBusy 		= -11,
		dlpErrNotSupp 		= -12,
		dlpErrUnused1 		= -13,
		dlpErrReadOnly 		= -14,
		dlpErrSpace 		= -15,
		dlpErrLimit 		= -16,
		dlpErrSync 		= -17,
		dlpErrWrapper 		= -18,
		dlpErrArgument 		= -19,
		dlpErrSize 		= -20,
		dlpErrUnknown 		= -128
	};

	struct dlpArg {
		int id;

		int len;		
		unsigned char *data;
	};

	struct dlpRequest {
		enum dlpFunctions cmd;
		
		int argc;
		struct dlpArg **argv;
	};

	struct dlpResponse {
		enum dlpFunctions cmd;
		enum dlpErrors err;
		
		int argc;
		struct dlpArg **argv;
	};	

	struct dlpArg * dlp_arg_new (int id, int len);
	void dlp_arg_free (struct dlpArg *arg);
	int dlp_arg_len (int argc, struct dlpArg **argv);
	struct dlpRequest *dlp_request_new (enum dlpFunctions cmd, int argc, ...);
	struct dlpRequest * dlp_request_new_with_argid (enum dlpFunctions cmd, int argid, int argc, ...);
	struct dlpResponse *dlp_response_new (enum dlpFunctions cmd, int argc);
	int dlp_response_read (struct dlpResponse **res, int sd);
	int dlp_request_write (struct dlpRequest *req, int sd);
	void dlp_request_free (struct dlpRequest *req);
	void dlp_response_free (struct dlpResponse *req);
	
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
	   Read 	= 0x80
	   Write 	= 0x40
	   Exclusive 	= 0x20
	   ShowSecret 	= 0x10
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
