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

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>

#include "pi-macros.h"		/* For recordid_t */
#include "pi-buffer.h"		/* For pi_buffer_t */

/* version of the DLP protocol supported in this version */
/* Hint for existing versions:
 * 1.2: Palm OS 4 / Palm OS 5 (OS 5 should be 1.3 but incorrectly reports 1.2)
 * 1.4: TapWave Palm OS 5
 * 2.1: Palm OS 6
 */
#define PI_DLP_VERSION_MAJOR 1
#define PI_DLP_VERSION_MINOR 4

#define PI_DLP_OFFSET_CMD  0
#define PI_DLP_OFFSET_ARGC 1
#define PI_DLP_OFFSET_ARGV 2

#define PI_DLP_ARG_TINY_LEN  0x000000FFL
#define PI_DLP_ARG_SHORT_LEN 0x0000FFFFL
#define PI_DLP_ARG_LONG_LEN  0xFFFFFFFFL

#define PI_DLP_ARG_FLAG_TINY  0x00
#define PI_DLP_ARG_FLAG_SHORT 0x80
#define PI_DLP_ARG_FLAG_LONG  0x40
#define PI_DLP_ARG_FLAG_MASK  0xC0

#define PI_DLP_ARG_FIRST_ID 0x20

#define DLP_BUF_SIZE 0xffff
#define DLP_REQUEST_DATA(req, arg, offset) &req->argv[arg]->data[offset]
#define DLP_RESPONSE_DATA(res, arg, offset) &res->argv[arg]->data[offset]


#define vfsFileAttrReadOnly     (0x00000001UL)
#define vfsFileAttrHidden       (0x00000002UL)
#define vfsFileAttrSystem       (0x00000004UL)
#define vfsFileAttrVolumeLabel  (0x00000008UL)
#define vfsFileAttrDirectory    (0x00000010UL)
#define vfsFileAttrArchive      (0x00000020UL)
#define vfsFileAttrLink         (0x00000040UL)

/* Mount/Format the volume with the filesystem specified */
#define vfsMountFlagsUseThisFileSystem	0x01

/* file type for slot driver libraries */
#define sysFileTSlotDriver	'libs'

/* The date the file was created. */
#define vfsFileDateCreated 	1

/* The date the file was last modified. */
#define vfsFileDateModified 	2

/* The date the file was last accessed. */
#define vfsFileDateAccessed 	3

#define vfsMAXFILENAME 256

#define vfsIteratorStart	0L
#define vfsIteratorStop		((unsigned long)0xffffffffL)

/* constant for an invalid volume reference, guaranteed not to represent a
   valid one.  Use it like you would use NULL for a FILE*. */
#define vfsInvalidVolRef 0

/* constant for an invalid file reference, guaranteed not to represent a
   valid one.  Use it like you would use NULL for a FILE*. */
#define vfsInvalidFileRef	0L

/* from the beginning (first data byte of file) */
#define vfsOriginBeginning	0

/* from the current position */
#define vfsOriginCurrent	1

/* From the end of file (one position beyond last data byte, only negative
   offsets are legally allowed) */
#define vfsOriginEnd		2

typedef unsigned long FileRef;


/* Volume attributes as found in VFSInfo.attributes */
#define vfsVolAttrSlotBased	(0x00000001UL)
#define vfsVolAttrReadOnly	(0x00000002UL)
#define vfsVolAttrHidden	(0x00000004UL)

	struct VFSDirInfo {
		unsigned long attr;
		char name[vfsMAXFILENAME];
	};

	typedef struct VFSAnyMountParamTag {
		unsigned short volRefNum;
		unsigned short reserved;
		unsigned long  mountClass;
	} VFSAnyMountParamType;

	struct VFSSlotMountParamTag {
		VFSAnyMountParamType vfsMountParam;
		unsigned short slotLibRefNum;
		unsigned short slotRefNum;
	};

	struct VFSInfo {
		/* 0: read-only etc. */
		unsigned long   attributes;

		/* 4: Filesystem type for this volume (defined below).
		      These you can expect to see in devices:
		          'vfat' (FAT12/FAT16 with long name support)
		      Other values observed:
		          'twmf' (Tapwave Zodiac internal VFS)
		      PalmSource defines these, but don't bet on device support:
		          'afsu' (Andrew network filesystem)
		          'ext2' (Linux ext2 filesystem)
		          'fats' (FAT12/FAT16 with 8.3 names)
		          'ffsb' (BSD block-based filesystem)
		          'hfse' (Macintosh HFS+)
		          'hfss' (Macintosh HFS, pre-8.x)
		          'hpfs' (OS/2 High Performance Filesystem)
		          'mfso' (Original Macintosh filesystem)
		          'nfsu' (NFS mount)
		          'novl' (Novell filesystem)
		          'ntfs' (Windows NT filesystem)
		*/
		unsigned long   fsType;

		/* 8: Creator code of filesystem driver for this volume. */
		unsigned long   fsCreator;

		/* For slot based filesystems: (mountClass = VFSMountClass_SlotDriver)
		   12: mount class that mounted this volume */
		unsigned long   mountClass;

		/* 16: Library on which the volume is mounted */
		int slotLibRefNum;

		/* 18: ExpMgr slot number of card containing volume */
		int slotRefNum;

		/* 20: Type of card media (mediaMemoryStick, mediaCompactFlash, etc.)
		       These you can expect to see in devices:
		           'cfsh' (CompactFlash)
		           'mmcd' (MultiMedia Card)
		           'mstk' (Memory Stick)
		           'sdig' (SD card)
		       Other values observed:
		           'TFFS' (palmOne Tungsten T5 internal VFS)
		           'twMF' (Tapwave Zodiac internal VFS)
		       PalmSource also defines these:
		           'pose' (Host filesystem emulated by POSE)
		           'PSim' (Host filesystem emulated by Mac Simulator)
		           'ramd' (RAM disk)
		           'smed' (SmartMedia)
		*/
		unsigned long   mediaType;

		/* 24: reserved for future use (other mountclasses may need more space) */
		unsigned long   reserved;
	};

	/* Note: All of these functions return an integer that if greater
	   then zero is the number of bytes in the result, zero if there was
	   no result, or less then zero if an error occured. Any return
	   fields will be set to zero if an error occurs. All calls to dlp_*
	   functions should check for a return value less then zero.
	 */
	struct PilotUser {
		size_t 	passwordLength;
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

	struct DBSizeInfo {
		unsigned long numRecords;
		unsigned long totalBytes;
		unsigned long dataBytes;
		unsigned long appBlockSize;
		unsigned long sortBlockSize;
		unsigned long maxRecSize;		/* note: this field is always set to 0 on return from dlp_FindDBxxx */
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
		dlpFuncReadUserInfo,			/* 0x10 */
		dlpFuncWriteUserInfo,			/* 0x11 */
		dlpFuncReadSysInfo,			/* 0x12 */
		dlpFuncGetSysDateTime,			/* 0x13 */
		dlpFuncSetSysDateTime,			/* 0x14 */
		dlpFuncReadStorageInfo,			/* 0x15 */
		dlpFuncReadDBList,			/* 0x16 */
		dlpFuncOpenDB,				/* 0x17 */
		dlpFuncCreateDB,			/* 0x18 */
		dlpFuncCloseDB,				/* 0x19 */
		dlpFuncDeleteDB,			/* 0x1a */
		dlpFuncReadAppBlock,			/* 0x1b */
		dlpFuncWriteAppBlock,			/* 0x1c */
		dlpFuncReadSortBlock,			/* 0x1d */
		dlpFuncWriteSortBlock,			/* 0x1e */
		dlpFuncReadNextModifiedRec,		/* 0x1f */
		dlpFuncReadRecord,			/* 0x20 */
		dlpFuncWriteRecord,			/* 0x21 */
		dlpFuncDeleteRecord,			/* 0x22 */
		dlpFuncReadResource,			/* 0x23 */
		dlpFuncWriteResource,			/* 0x24 */
		dlpFuncDeleteResource,			/* 0x25 */
		dlpFuncCleanUpDatabase,			/* 0x26 */
		dlpFuncResetSyncFlags,			/* 0x27 */
		dlpFuncCallApplication,			/* 0x28 */
		dlpFuncResetSystem,			/* 0x29 */
		dlpFuncAddSyncLogEntry,			/* 0x2a */
		dlpFuncReadOpenDBInfo,			/* 0x2b */
		dlpFuncMoveCategory,			/* 0x2c */
		dlpProcessRPC,				/* 0x2d */
		dlpFuncOpenConduit,			/* 0x2e */
		dlpFuncEndOfSync,			/* 0x2f */
		dlpFuncResetRecordIndex,		/* 0x30 */
		dlpFuncReadRecordIDList,		/* 0x31 */

		/* DLP 1.1 FUNCTIONS ADDED HERE (PalmOS v2.0 Personal, and Professional) */
		dlpFuncReadNextRecInCategory,   	/* 0x32 */
		dlpFuncReadNextModifiedRecInCategory,   /* 0x33 */
		dlpFuncReadAppPreference,		/* 0x34 */
		dlpFuncWriteAppPreference,		/* 0x35 */
		dlpFuncReadNetSyncInfo,			/* 0x36 */
		dlpFuncWriteNetSyncInfo,		/* 0x37 */
		dlpFuncReadFeature,			/* 0x38 */

		/* DLP 1.2 FUNCTIONS ADDED HERE (PalmOS v3.0) */
		dlpFuncFindDB,				/* 0x39 */
		dlpFuncSetDBInfo,			/* 0x3a */

		/* DLP 1.3 FUNCTIONS ADDED HERE (PalmOS v4.0) */
		dlpLoopBackTest,			/* 0x3b */
		dlpFuncExpSlotEnumerate,		/* 0x3c */
		dlpFuncExpCardPresent,			/* 0x3d */
		dlpFuncExpCardInfo,			/* 0x3e */
		dlpFuncVFSCustomControl,		/* 0x3f */
		dlpFuncVFSGetDefaultDir,		/* 0x40 */
		dlpFuncVFSImportDatabaseFromFile,	/* 0x41 */
		dlpFuncVFSExportDatabaseToFile, 	/* 0x42 */
		dlpFuncVFSFileCreate,			/* 0x43 */
		dlpFuncVFSFileOpen,			/* 0x44 */
		dlpFuncVFSFileClose,			/* 0x45 */
		dlpFuncVFSFileWrite,			/* 0x46 */
		dlpFuncVFSFileRead,			/* 0x47 */
		dlpFuncVFSFileDelete,			/* 0x48 */
		dlpFuncVFSFileRename,			/* 0x49 */
		dlpFuncVFSFileEOF,			/* 0x4a */
		dlpFuncVFSFileTell,			/* 0x4b */
		dlpFuncVFSFileGetAttributes,		/* 0x4c */
		dlpFuncVFSFileSetAttributes,		/* 0x4d */
		dlpFuncVFSFileGetDate,			/* 0x4e */
		dlpFuncVFSFileSetDate,			/* 0x4f */
		dlpFuncVFSDirCreate,			/* 0x50 */
		dlpFuncVFSDirEntryEnumerate,		/* 0x51 */
		dlpFuncVFSGetFile,			/* 0x52 */
		dlpFuncVFSPutFile,			/* 0x53 */
		dlpFuncVFSVolumeFormat,			/* 0x54 */
		dlpFuncVFSVolumeEnumerate,		/* 0x55 */
		dlpFuncVFSVolumeInfo,			/* 0x56 */
		dlpFuncVFSVolumeGetLabel,		/* 0x57 */
		dlpFuncVFSVolumeSetLabel,		/* 0x58 */
		dlpFuncVFSVolumeSize,			/* 0x59 */
		dlpFuncVFSFileSeek,			/* 0x5a */
		dlpFuncVFSFileResize,			/* 0x5b */
		dlpFuncVFSFileSize,			/* 0x5c */

		/* DLP 1.4-TW functions added here (Palm OS 5/TapWave) */
		dlpFuncExpSlotMediaType,		/* 0x5d */
		dlpFuncWriteRecordEx,			/* 0x5e - function to write >64k records in TapWave */
		dlpFuncWriteResourceEx,			/* 0x5f - function to write >64k resources in TapWave */
		dlpFuncReadRecordEx,			/* 0x60 - function to read >64k records by index in TapWave */
		dlpFuncUnknown1,			/* 0x61 (may be bogus definition in tapwave headers, is listed as dlpFuncReadRecordStream)*/
		dlpFuncUnknown3,			/* 0x62 */
		dlpFuncUnknown4,			/* 0x63 */
		dlpFuncReadResourceEx,			/* 0x64 - function to read resources >64k by index in TapWave */
		dlpLastFunc
	};

	enum dlpDBFlags {
		dlpDBFlagResource 	= 0x0001,	/* Resource DB, instead of record DB            */
		dlpDBFlagReadOnly 	= 0x0002,	/* DB is read only                              */
		dlpDBFlagAppInfoDirty 	= 0x0004,	/* AppInfo data has been modified               */
		dlpDBFlagBackup 	= 0x0008,	/* DB is tagged for generic backup              */
		dlpDBFlagHidden		= 0x0100,	/* DB is hidden                                 */
		dlpDBFlagLaunchable	= 0x0200,	/* DB is launchable data (show in Launcher, launch app by Creator) */
		dlpDBFlagRecyclable	= 0x0400,	/* DB will be deleted shortly                   */
		dlpDBFlagBundle		= 0x0800,	/* DB is bundled with others having same creator (i.e. for Beam) */
		dlpDBFlagOpen 		= 0x8000,	/* DB is currently open                         */

		/* v2.0 specific */
		dlpDBFlagNewer 		= 0x0010,	/* Newer version may be installed over open DB  */
		dlpDBFlagReset 		= 0x0020,	/* Reset after installation                     */

		/* v3.0 specific */
		dlpDBFlagCopyPrevention = 0x0040,	/* DB should not be beamed                      */
		dlpDBFlagStream 	= 0x0080,	/* DB implements a file stream                  */

		/* OS 6+ */
		dlpDBFlagSchema		= 0x1000,	/* DB is Schema database                        */
		dlpDBFlagSecure		= 0x2000,	/* DB is Secure database                        */
		dlpDBFlagExtended	= dlpDBFlagSecure, /* Set if Schema not set and DB is Extended  */
		dlpDBFlagFixedUp	= 0x4000	/* temp flag used to clear DB on write          */
	};

	enum dlpDBMiscFlags {
		dlpDBMiscFlagExcludeFromSync = 0x80,	/* defined for DLP 1.1 */
		dlpDBMiscFlagRamBased 	= 0x40		/* defined for DLP 1.2 */
	};

	enum dlpRecAttributes {
		dlpRecAttrDeleted 	= 0x80,		/* tagged for deletion during next sync         */
		dlpRecAttrDirty 	= 0x40,		/* record modified                              */
		dlpRecAttrBusy 		= 0x20,		/* record locked                                */
		dlpRecAttrSecret 	= 0x10,		/* record is secret                             */
		dlpRecAttrArchived 	= 0x08		/* tagged for archival during next sync         */
	};

	enum dlpOpenFlags {
		dlpOpenRead 		= 0x80,
		dlpOpenWrite 		= 0x40,
		dlpOpenExclusive 	= 0x20,
		dlpOpenSecret 		= 0x10,
		dlpOpenReadWrite 	= 0xC0
	};

	enum dlpVFSOpenFlags {
		dlpVFSOpenExclusive 	= 0x1,
		dlpVFSOpenRead 		= 0x2,
		dlpVFSOpenWrite 	= 0x5 		/* implies exclusive */,
		dlpVFSOpenReadWrite 	= 0x7 		/* read | write */,

		/* Remainder are aliases and special cases not for VFSFileOpen */
		vfsModeExclusive 	= dlpVFSOpenExclusive,
		vfsModeRead 		= dlpVFSOpenRead,
		vfsModeWrite 		= dlpVFSOpenWrite,
		vfsModeReadWrite	= vfsModeRead | vfsModeWrite,
		vfsModeCreate 		= 0x8 		/* Create file if it doesn't exist. Handled in VFS layer */,
		vfsModeTruncate 	= 0x10 		/* Truncate to 0 bytes on open. Handled in VFS layer */,
		vfsModeLeaveOpen 	= 0x20 		/* Leave file open even if foreground task closes. */

	} ;

	enum dlpEndStatus {
		dlpEndCodeNormal 	= 0,		/* Normal					 */
		dlpEndCodeOutOfMemory,			/* End due to low memory on Palm		 */
		dlpEndCodeUserCan,			/* Cancelled by user				 */
		dlpEndCodeOther				/* dlpEndCodeOther and higher == "Anything else" */
	};

	enum dlpDBList {				/* flags passed to dlp_ReadDBList */
		dlpDBListRAM 		= 0x80,
		dlpDBListROM 		= 0x40,
		dlpDBListMultiple	= 0x20		/* defined for DLP 1.2 */
	};

	enum dlpFindDBOptFlags {
		dlpFindDBOptFlagGetAttributes	= 0x80,
		dlpFindDBOptFlagGetSize		= 0x40,
		dlpFindDBOptFlagMaxRecSize	= 0x20
	};

	enum dlpFindDBSrchFlags {
		dlpFindDBSrchFlagNewSearch	= 0x80,
		dlpFindDBSrchFlagOnlyLatest	= 0x40
	};

	/* After a DLP transaction, there may be a DLP or Palm OS error
	 * if the result code is PI_ERR_DLP_PALMOS. In this case, use
	 * pi_palmos_error(sd) to obtain the error code. It can be in the
	 * DLP error range (0 > error < dlpErrLastError), or otherwise
	 * in the Palm OS error range (see Palm OS header files for
	 * definitions, in relation with each DLP call)
	 */
	enum dlpErrors {
		dlpErrNoError 		= 0,
		dlpErrSystem,		/* 0x0001 */
		dlpErrIllegalReq,	/* 0x0002 */
		dlpErrMemory,		/* 0x0003 */
		dlpErrParam,		/* 0x0004 */
		dlpErrNotFound,		/* 0x0005 */
		dlpErrNoneOpen,		/* 0x0006 */
		dlpErrAlreadyOpen,	/* 0x0007 */
		dlpErrTooManyOpen,	/* 0x0008 */
		dlpErrExists,		/* 0x0009 */
		dlpErrOpen,			/* 0x000a */
		dlpErrDeleted,		/* 0x000b */
		dlpErrBusy,			/* 0x000c */
		dlpErrNotSupp,		/* 0x000d */
		dlpErrUnused1,		/* 0x000e */
		dlpErrReadOnly,		/* 0x000f */
		dlpErrSpace,		/* 0x0010 */
		dlpErrLimit,		/* 0x0011 */
		dlpErrSync,			/* 0x0012 */
		dlpErrWrapper,		/* 0x0013 */
		dlpErrArgument,		/* 0x0014 */
		dlpErrSize,			/* 0x0015 */

		dlpErrUnknown = 127
	};

	struct dlpArg {
		int 	id_;	/* ObjC has a type id */
		size_t	len;
		char *data;
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

	extern time_t dlp_ptohdate(unsigned const char *data);
	extern void dlp_htopdate(time_t time, unsigned char *data);

	extern struct dlpArg * dlp_arg_new PI_ARGS((int id_, size_t len));
	extern void dlp_arg_free PI_ARGS((struct dlpArg *arg));
	extern int dlp_arg_len PI_ARGS((int argc, struct dlpArg **argv));

	extern struct dlpRequest *dlp_request_new
	        PI_ARGS((enum dlpFunctions cmd, int argc, ...));
	extern struct dlpRequest * dlp_request_new_with_argid
	        PI_ARGS((enum dlpFunctions cmd, int argid, int argc, ...));
	extern void dlp_request_free PI_ARGS((struct dlpRequest *req));

	extern struct dlpResponse *dlp_response_new
	        PI_ARGS((enum dlpFunctions cmd, int argc));
	extern ssize_t dlp_response_read PI_ARGS((struct dlpResponse **res,
		int sd));
	extern ssize_t dlp_request_write PI_ARGS((struct dlpRequest *req,
		int sd));
	extern void dlp_response_free PI_ARGS((struct dlpResponse *req));

	extern int dlp_exec PI_ARGS((int sd, struct dlpRequest *req,
		struct dlpResponse **res));

	extern char *dlp_errorlist[];
	extern char *dlp_strerror(int error);

	extern void dlp_set_protocol_version
			PI_ARGS((int major, int minor));

	/* Get the time on the Palm and return it as a local time_t value. */
	extern int dlp_GetSysDateTime PI_ARGS((int sd, time_t *t));

	/* Set the time on the Palm using a local time_t value. */
	extern int dlp_SetSysDateTime PI_ARGS((int sd, time_t t));

	extern int dlp_ReadStorageInfo
		PI_ARGS((int sd, int cardno, struct CardInfo * c));

	/* Read the system information block. */
	extern int dlp_ReadSysInfo PI_ARGS((int sd, struct SysInfo * s));

	/* flags must contain dlpDBListRAM and/or dlpDBListROM
	 * and can optionally contain dlpDBListMultiple (will be honored
	 * if the OS supports it). Returns one or more DBInfo structs
	 * packed in the pi_buffer_t
	 */
	extern int dlp_ReadDBList
		PI_ARGS((int sd, int cardno, int flags, int start,
			pi_buffer_t *info));

	extern int dlp_FindDBInfo
		PI_ARGS((int sd, int cardno, int start, const char *dbname,
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
		PI_ARGS((int sd, int cardno, int mode, PI_CONST char *name,
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
		PI_ARGS((int sd, unsigned long creator, unsigned long type,
			int cardno, int flags, unsigned int version,
			PI_CONST char *name, int *dbhandle));

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
		PI_ARGS((int sd, int fHandle, int offset, int reqbytes,
			pi_buffer_t *retbuf));

	extern int dlp_WriteAppBlock
		PI_ARGS((int sd,int fHandle,PI_CONST void *dbuf,size_t dlen));

	extern int dlp_ReadSortBlock
		PI_ARGS((int sd, int fHandle, int offset, int reqbytes,
			pi_buffer_t *retbuf));

	extern int dlp_WriteSortBlock
		PI_ARGS((int sd, int fHandle, PI_CONST void *dbuf,
			size_t dlen));

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
			int catID, void *data, size_t length,
			recordid_t * NewID));

	extern int dlp_DeleteRecord
		PI_ARGS((int sd, int dbhandle, int all, recordid_t recID));

	extern int dlp_DeleteCategory
		PI_ARGS((int sd, int dbhandle, int category));

	extern int dlp_ReadResourceByType
		PI_ARGS((int sd, int fHandle, unsigned long type, int id_,
			pi_buffer_t *buffer, int *resindex));

	extern int dlp_ReadResourceByIndex
		PI_ARGS((int sd, int fHandle, int resindex, pi_buffer_t *buffer,
			unsigned long *type, int *id_));

	extern int dlp_WriteResource
		PI_ARGS((int sd, int dbhandle, unsigned long type, int id_,
			PI_CONST void *data, size_t length));

	extern int dlp_DeleteResource
		PI_ARGS((int sd, int dbhandle, int all, unsigned long restype,
			int resID));

	extern int dlp_ReadNextModifiedRec
		PI_ARGS((int sd, int fHandle, pi_buffer_t *buffer, recordid_t * id_,
			int *recindex, int *attr, int *category));

	extern int dlp_ReadNextModifiedRecInCategory
		PI_ARGS((int sd, int fHandle, int incategory, pi_buffer_t *buffer,
			recordid_t * id_, int *recindex, int *attr));

	extern int dlp_ReadNextRecInCategory
		PI_ARGS((int sd, int fHandle, int incategory, pi_buffer_t *buffer,
			recordid_t *recuid, int *recindex, int *attr));

	extern int dlp_ReadRecordById
		PI_ARGS((int sd, int fHandle, recordid_t id_, pi_buffer_t *buffer,
			int *recindex, int *attr, int *category));

	extern int dlp_ReadRecordByIndex
		PI_ARGS((int sd, int fHandle, int recindex, pi_buffer_t *buffer,
			recordid_t *recuid, int *attr, int *category));

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
			int action, size_t length, const void *data,
			unsigned long *retcode, pi_buffer_t *retbuf));

	extern int dlp_ReadFeature
		PI_ARGS((int sd, unsigned long creator, unsigned int num,
			unsigned long *feature));

	extern int dlp_GetROMToken
		PI_ARGS((int sd, unsigned long token, char *buffer, size_t *size));

	/* PalmOS 2.0 only */
	extern int dlp_ReadNetSyncInfo
		PI_ARGS((int sd, struct NetSyncInfo * i));

	/* PalmOS 2.0 only */
	extern int dlp_WriteNetSyncInfo
		PI_ARGS((int sd, struct NetSyncInfo * i));

	extern int dlp_ReadAppPreference
		PI_ARGS((int sd, unsigned long creator, int id_, int backup,
			int maxsize, void *buffer, size_t *size, int *version));

	extern int dlp_WriteAppPreference
		PI_ARGS((int sd, unsigned long creator, int id_, int backup,
			int version, void *buffer, size_t size));

	/* PalmOS 3.0 only */
	extern int dlp_SetDBInfo
	        PI_ARGS((int sd, int dbhandle, int flags, int clearFlags, unsigned int version,
			 time_t createDate, time_t modifyDate, time_t backupDate,
			 unsigned long type, unsigned long creator));

	extern int dlp_FindDBByName
	        PI_ARGS((int sd, int cardno, PI_CONST char *name, unsigned long *localid, int *dbhandle,
			 struct DBInfo *info, struct DBSizeInfo *size));

	extern int dlp_FindDBByOpenHandle
	        PI_ARGS((int sd, int dbhandle, int *cardno, unsigned long *localid,
			 struct DBInfo *info, struct DBSizeInfo *size));

	extern int dlp_FindDBByTypeCreator
	        PI_ARGS((int sd, unsigned long type, unsigned long creator, int start,
			 int latest, int *cardno, unsigned long *localid, int *dbhandle,
			 struct DBInfo *info, struct DBSizeInfo *size));

	struct RPC_params;

	extern int dlp_RPC
		PI_ARGS((int sd, struct RPC_params * p,
			unsigned long *result));

	/* Palm OS 4.0 only */
	extern int dlp_ExpSlotEnumerate
		PI_ARGS((int sd, int *numSlots, int *slotRefs));

	extern int dlp_ExpCardPresent
		PI_ARGS((int sd, int SlotRef));

	extern int dlp_ExpCardInfo
		PI_ARGS((int sd, int SlotRef, unsigned long *flags,
			 int *numStrings, char **strings));

	extern int dlp_VFSGetDefaultDir
		PI_ARGS((int sd, int volRefNum, const char *name,char *dir, int *len));

	extern int dlp_VFSImportDatabaseFromFile
		PI_ARGS((int sd, int volRefNum, const char *pathNameP,
			 int *cardno, unsigned long *localid));

	extern int dlp_VFSExportDatabaseToFile
		PI_ARGS((int sd, int volRefNum, const char *pathNameP,
			int cardno, unsigned int localid));

	extern int dlp_VFSFileCreate
		PI_ARGS((int sd, int volRefNum,const char *name));

	extern int dlp_VFSFileOpen
		PI_ARGS((int sd, int volRefNum, const char *path, int openMode,
			FileRef *outFileRef));

	extern int dlp_VFSFileClose
		PI_ARGS((int sd, FileRef afile));

	extern int dlp_VFSFileWrite
		PI_ARGS((int sd, FileRef afile, unsigned char *data, size_t len));

	extern int dlp_VFSFileRead
		PI_ARGS((int sd, FileRef afile, pi_buffer_t *data, size_t numBytes));

	extern int dlp_VFSFileDelete
		PI_ARGS((int sd, int volRefNum, const char *name));

	extern int dlp_VFSFileRename
		PI_ARGS((int sd, int volRefNum, const char *name,
			const char *newname));

	extern int dlp_VFSFileEOF
		PI_ARGS((int sd, FileRef afile));

	extern int dlp_VFSFileTell
		PI_ARGS((int sd, FileRef afile,int *position));

	extern int dlp_VFSFileGetAttributes
		PI_ARGS((int sd, FileRef afile, unsigned long *attributes));

	extern int dlp_VFSFileSetAttributes
		PI_ARGS((int sd, FileRef afile, unsigned long attributes));

	extern int dlp_VFSFileGetDate
		PI_ARGS((int sd, FileRef afile, int which, time_t *date));

	extern int dlp_VFSFileSetDate
		PI_ARGS((int sd, FileRef afile, int which, time_t date));

	extern int dlp_VFSDirCreate
		PI_ARGS((int sd, int volRefNum, const char *path));

	extern int dlp_VFSDirEntryEnumerate
		PI_ARGS((int sd, FileRef dirRefNum, unsigned long *dirIterator,
			int *maxDirItems, struct VFSDirInfo *dirItems));

	extern int dlp_VFSVolumeFormat
		PI_ARGS((int sd, unsigned char flags, int fsLibRef,
			struct VFSSlotMountParamTag *param));

	extern int dlp_VFSVolumeEnumerate
		PI_ARGS((int sd, int *numVols, int *volRefs));

	extern int dlp_VFSVolumeInfo
		PI_ARGS((int sd, int volRefNum, struct VFSInfo *volInfo));

	extern int dlp_VFSVolumeGetLabel
		PI_ARGS((int sd, int volRefNum, int *len, char *name));

	extern int dlp_VFSVolumeSetLabel
		PI_ARGS((int sd, int volRefNum, const char *name));

	extern int dlp_VFSVolumeSize
		PI_ARGS((int sd, int volRefNum, long *volSizeUsed,
			long *volSizeTotal));

	extern int dlp_VFSFileSeek
		PI_ARGS((int sd, FileRef afile, int origin, int offset));

	extern int dlp_VFSFileResize
		PI_ARGS((int sd, FileRef afile, int newSize));

	extern int dlp_VFSFileSize
		PI_ARGS((int sd, FileRef afile,int *size));

	/* DLP 1.4 only (Palm OS 5.2, seen on TapWave) */
	extern int dlp_ExpSlotMediaType
		PI_ARGS((int sd, int slotNum, unsigned long *mediaType));

#ifdef __cplusplus
}
#endif
#endif				/*_PILOT_DLP_H_*/
