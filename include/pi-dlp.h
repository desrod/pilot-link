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

/** @file pi-dlp.h
 *  @brief Direct protocol interface to the device using the HotSync protocol.
 *
 * The DLP layer is the lowest interface layer applications can use to access a handheld.
 * It provides equivalents to Palm Conduit Development Kit (CDK)'s SyncXXX functions, as
 * well as a number of convenience functions that are not found in the CDK.
 *
 * Once a device is connected and you have a socket number, you can start using DLP calls
 * to talk to it.
 *
 * The DLP protocol is the low level protocol that HotSync uses. Over the years, there have
 * been several iterations of DLP. Pre-Palm OS 5 devices have DLP 1.2 or lower. Palm OS 5
 * devices have DLP 1.3 or 1.4. Cobalt (Palm OS 6) uses DLP 2.1.
 *
 * Devices with DLP 1.4 and later are known to support transfers of large records and
 * resources (of size bigger than 64k). This is the case of the Tapwave Zodiac, for
 * example.
 *
 * Note that some devices report an incorrect version of DLP. Some Palm OS 5 devices report
 * using DLP 1.2 whereas they really support DLP 1.3.
 *
 * Depending on which devices you plan on being compatible with, you should adjust
 * PI_DLP_VERSION_MAJOR and PI_DLP_VERSION_MINOR. If you want to support devices up to and
 * including Palm OS 5, setting your DLP version to 1.4 is a good idea. If you want to be
 * able to connect to Palm OS 6, you need to set your DLP version to 2.1.
 */

#ifndef _PILOT_DLP_H_
#define _PILOT_DLP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>

#include "pi-macros.h"		/* For recordid_t */
#include "pi-buffer.h"		/* For pi_buffer_t */

/* Note: All of these functions return an integer that if greater
   then zero is the number of bytes in the result, zero if there was
   no result, or less then zero if an error occured. Any return
   fields will be set to zero if an error occurs. All calls to dlp_*
   functions should check for a return value less then zero.
 */

/* version of the DLP protocol supported in this version */
/* Hint for existing versions:
 * 1.2: Palm OS 4 / Palm OS 5 (OS 5 should be 1.3 but incorrectly reports 1.2)
 * 1.4: TapWave Palm OS 5
 * 2.1: Palm OS 6
 */
#define PI_DLP_VERSION_MAJOR 1			/**< Major DLP protocol version we report to the device. */
#define PI_DLP_VERSION_MINOR 4			/**< Minor DLP protocol version we report to the device. */

#define DLP_BUF_SIZE 0xffff			/**< Kept for compatibility, applications should avoid using this value. */

#define sysFileTSlotDriver	'libs'		/**< file type for slot driver libraries */

/** @name VFS file attribute definitions */
/*@{*/
#define vfsFileAttrReadOnly     (0x00000001UL)	/**< File is read only */
#define vfsFileAttrHidden       (0x00000002UL)	/**< File is hidden */
#define vfsFileAttrSystem       (0x00000004UL)	/**< File is a system file */
#define vfsFileAttrVolumeLabel  (0x00000008UL)	/**< File is the volume label */
#define vfsFileAttrDirectory    (0x00000010UL)	/**< File is a directory */
#define vfsFileAttrArchive      (0x00000020UL)	/**< File is archived */
#define vfsFileAttrLink         (0x00000040UL)	/**< File is a link to another file */
/*@}*/

/** @name Constants for dlp_VFSFileGetDate() and dlp_VFSFileSetDate() */
/*@{*/
#define vfsFileDateCreated 	1		/**< The date the file was created. */
#define vfsFileDateModified 	2		/**< The date the file was last modified. */
#define vfsFileDateAccessed 	3		/**< The date the file was last accessed. */
/*@}*/

#define vfsMountFlagsUseThisFileSystem	0x01	/**< Mount/Format the volume with the filesystem specified */

#define vfsMAXFILENAME 256

/** @name VFS file iterator definitions */
/*@{*/
#define vfsIteratorStart	0L
#define vfsIteratorStop		((unsigned long)0xffffffffL)
/*@}*/

/* constant for an invalid volume reference, guaranteed not to represent a
   valid one.  Use it like you would use NULL for a FILE*. */
#define vfsInvalidVolRef 0

/* constant for an invalid file reference, guaranteed not to represent a
   valid one.  Use it like you would use NULL for a FILE*. */
#define vfsInvalidFileRef	0L

/** @name Constants for dlp_VFSFileSeek() */
/*@{*/
#define vfsOriginBeginning	0		/**< From the beginning (first data byte of file) */
#define vfsOriginCurrent	1		/**< from the current position */
#define vfsOriginEnd		2		/**< From the end of file (one position beyond last data byte, only negative offsets are legally allowed) */
/*@}*/

typedef unsigned long FileRef;			/**< Type for file references when working with VFS files and directories. */


/** @name Volume attributes as found in VFSInfo.attributes */
/*@{*/
#define vfsVolAttrSlotBased	(0x00000001UL)
#define vfsVolAttrReadOnly	(0x00000002UL)	/**< Volume is read-only */
#define vfsVolAttrHidden	(0x00000004UL)
/*@}*/


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

struct PilotUser {
	size_t 	passwordLength;
	char 	username[128],
		password[128];
	unsigned long userID, viewerID, lastSyncPC;
	time_t successfulSyncDate, lastSyncDate;
};

/** Device information.
 *
 * This structure is filled by dlp_ReadSysInfo()
 */
struct SysInfo {
	unsigned long romVersion;		/**< Version of the device ROM, of the form 0xMMmmffssbb where MM=Major, mm=minor, ff=fix, ss=stage, bb=build */
	unsigned long locale;			/**< Locale for this device */
	unsigned char prodIDLength;		/**< Length of the prodID string */
	char 	prodID[128];			/**< Product ID */
	unsigned short dlpMajorVersion;		/**< Major version of the DLP protocol on this device */
	unsigned short dlpMinorVersion;		/**< Minor version of the DLP protocol on this device */
	unsigned short compatMajorVersion;	/**< Minimum major version of DLP this device is compatible with */
	unsigned short compatMinorVersion;	/**< Minimum minor version of DLP this device is compatible with */
	unsigned long  maxRecSize;		/**< Maximum record size. Usually <=0xFFFF or ==0 for older devices (means records are limited to 64k), can be much larger for devices with DLP >= 1.4 (i.e. 0x00FFFFFE) */
};

/** Database information.
 *
 * A database information block is returned by dlp_ReadDBList(), dlp_FindDBInfo(), dlp_FindDBByName(), dlp_FindDBByOpenHandle()
 * and dlp_FindDBByTypeCreator().
 */
struct DBInfo {
	int 	more;				/**< When reading database list using dlp_ReadDBList(), this flag is set if there are more databases to come */
	char name[34];				/**< Database name, 32 characters max. */
	unsigned int flags;			/**< Database flags (@see dlpDBFlags enum) */
	unsigned int miscFlags;			/**< Additional database flags filled by pilot-link (@see dlpDBMiscFlags enum) */
	unsigned int version;			/**< Database version number */
	unsigned long type;			/**< Database type (four-char code, i.e. 'appl') */
	unsigned long creator;			/**< Database creator (four-char code, i.e. 'DATA') */
	unsigned long modnum;			/**< Modification count */
	unsigned int index;			/**< Database index in database list */
	time_t createDate;			/**< Database creation date (using the machine's local time zone) */
	time_t modifyDate;			/**< Last time this database was modified (using the machine's local time zone). If the database was never modified, this field is set to 0x83DAC000 (Fri Jan  1 00:00:00 1904 GMT) */
	time_t backupDate;			/**< Last time this database was backed up using HotSync. If the database was never backed up, this field is set to 0x83DAC000 (Fri Jan  1 00:00:00 1904 GMT) */
};

/** Size information for a database.
 *
 * Returned by dlp_FindDBByName(), dlp_FindDBByOpenHandle() and dlp_FindDBByTypeCreator().
 */ 
struct DBSizeInfo {
	unsigned long numRecords;		/**< Number of records or resources */
	unsigned long totalBytes;		/**< Total number of bytes occupied by the database, including header and records list */
	unsigned long dataBytes;		/**< Total number of data bytes contained in the database's records or resources */
	unsigned long appBlockSize;		/**< Size of the appInfo block */
	unsigned long sortBlockSize;		/**< Size of the sortInfo block */
	unsigned long maxRecSize;		/**< note: this field is always set to 0 on return from dlp_FindDBxxx */
};

/** Information about a memory card.
 *
 * This structure describes a device's internal storage only, not removable media.
 * It is returned by dlp_ReadStorageInfo().
 */
struct CardInfo {
	int 	card;				/**< Memory card index (most devices only have one). */
	int	version;			/**< Version of the card */
	int	more;				/**< Set if there is another card after this one */
	time_t 	creation;			/**< Creation date (using the computer's local time zone) */
	unsigned long romSize;			/**< Size of the ROM block on this card (in bytes) */
	unsigned long ramSize;			/**< Size of the RAM block on this card (in bytes) */
	unsigned long ramFree;			/**< Total free RAM bytes */
	char 	name[128];			/**< Card name */
	char	manufacturer[128];		/**< Card manufacturer name */
};

/** Network HotSync information.
 *
 * Returned by dlp_ReadNetSyncInfo(). Gives the network location of a remote handheld.
 */
struct NetSyncInfo {
	int 	lanSync;			/**< Non-zero if LanSync is turned on on the device */
	char 	hostName[256];			/**< Device hostname if any. Null terminated string. */
	char 	hostAddress[40];		/**< Device host address. Null terminated string. */
	char 	hostSubnetMask[40];		/**< Device subnet mask. Null terminated string */
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

/** Database flags in DBInfo structure and also for dlp_CreateDB() */
enum dlpDBFlags {
	dlpDBFlagResource 	= 0x0001,	/**< Resource database */
	dlpDBFlagReadOnly 	= 0x0002,	/**< Database is read only */
	dlpDBFlagAppInfoDirty 	= 0x0004,	/**< AppInfo data has been modified */
	dlpDBFlagBackup 	= 0x0008,	/**< Database should be backed up during HotSync */
	dlpDBFlagHidden		= 0x0100,	/**< Database is hidden */
	dlpDBFlagLaunchable	= 0x0200,	/**< Database is launchable data (show in Launcher, launch app by Creator) */
	dlpDBFlagRecyclable	= 0x0400,	/**< Database will be deleted shortly */
	dlpDBFlagBundle		= 0x0800,	/**< Database is bundled with others having same creator (i.e. for Beam) */
	dlpDBFlagOpen 		= 0x8000,	/**< Database is currently open */

	/* v2.0 specific */
	dlpDBFlagNewer 		= 0x0010,	/**< Newer version may be installed over open DB (Palm OS 2.0 and later) */
	dlpDBFlagReset 		= 0x0020,	/**< Reset after installation (Palm OS 2.0 and later) */

	/* v3.0 specific */
	dlpDBFlagCopyPrevention = 0x0040,	/**< Database should not be beamed or sent (Palm OS 3.0 and later) */
	dlpDBFlagStream 	= 0x0080,	/**< Database is a file stream (Palm OS 3.0 and later) */

	/* OS 6+ */
	dlpDBFlagSchema		= 0x1000,	/**< Schema database (Palm OS 6.0 and later) */
	dlpDBFlagSecure		= 0x2000,	/**< Secure database (Palm OS 6.0 and later) */
	dlpDBFlagExtended	= dlpDBFlagSecure, /**< Set if Schema not set and DB is Extended (Palm OS 6.0 and later) */
	dlpDBFlagFixedUp	= 0x4000	/**< Temp flag used to clear DB on write (Palm OS 6.0 and later) */
};

/** Misc. flags in DBInfo structure */
enum dlpDBMiscFlags {
	dlpDBMiscFlagExcludeFromSync = 0x80,	/**< DLP 1.1 and later: exclude this database from sync */
	dlpDBMiscFlagRamBased 	= 0x40		/**< DLP 1.2 and later: this database is in RAM */
};

/** Database record attributes */
enum dlpRecAttributes {
	dlpRecAttrDeleted 	= 0x80,		/**< Tagged for deletion during next sync */
	dlpRecAttrDirty 	= 0x40,		/**< Record modified */
	dlpRecAttrBusy 		= 0x20,		/**< Record locked (in use) */
	dlpRecAttrSecret 	= 0x10,		/**< Record is secret */
	dlpRecAttrArchived 	= 0x08		/**< Tagged for archival during next sync */
};

/** Mode flags used in dlp_OpenDB() */
enum dlpOpenFlags {
	dlpOpenRead 		= 0x80,		/**< Open database for reading */
	dlpOpenWrite 		= 0x40,		/**< Open database for writing */
	dlpOpenExclusive 	= 0x20,		/**< Open database with exclusive access */
	dlpOpenSecret 		= 0x10,		/**< Show secret records */
	dlpOpenReadWrite 	= 0xC0		/**< Open database for reading and writing (equivalent to (#dlpOpenRead | #dlpOpenWrite)) */
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

/** End status values for dlp_EndOfSync() */
enum dlpEndStatus {
	dlpEndCodeNormal 	= 0,		/**< Normal termination */
	dlpEndCodeOutOfMemory,			/**< End due to low memory on device */
	dlpEndCodeUserCan,			/**< Cancelled by user */
	dlpEndCodeOther				/**< dlpEndCodeOther and higher == "Anything else" */
};

/** Flags passed to dlp_ReadDBList() */
enum dlpDBList {
	dlpDBListRAM 		= 0x80,		/**< List RAM databases */
	dlpDBListROM 		= 0x40,		/**< List ROM databases */
	dlpDBListMultiple	= 0x20		/**< DLP 1.2 and above: list as many databases as possible at once */
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

/* @name Functions internally used by dlp.c */
/*@{*/
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
/*@}*/

/** @brief Set the version of the DLP protocol we report to the device.
 *
 * During the handshake phase, the device and the desktop exchange the
 * version of the DLP protocol both support. If the device's DLP version
 * is higher than the desktop's, the device usually refuses to connect.
 * 
 * @param major Protocol major version
 * @param minor Protocol minor version
 */
extern void dlp_set_protocol_version
		PI_ARGS((int major, int minor));

/** @brief Get the time from the device and return it as a local time_t value
 *
 * @param sd Socket number
 * @param t Pointer to a time_t to fill
 * @return A negative value if an error occured (see pi-error.h)
 */
extern int dlp_GetSysDateTime PI_ARGS((int sd, time_t *t));

/** @brief Set the time on the Palm using a local time_t value.
 *
 * @param sd Socket number
 * @param t New time to set the device to (expressed using the computer's timezone)
 * @return A negative value if an error occured (see pi-error.h)
 */
extern int dlp_SetSysDateTime PI_ARGS((int sd, time_t t));

/** @brief Read information about internal handheld memory
 *
 * @param sd Socket number
 * @param cardno Card number (zero based)
 * @param c Returned information about the memory card.
 * @return A negative value if an error occured (see pi-error.h)
 */
extern int dlp_ReadStorageInfo
	PI_ARGS((int sd, int cardno, struct CardInfo * c));

/** @brief Read the system information block
 *
 * @param sd Socket number
 * @param s Returned system information
 * @return A negative value if an error occured (see pi-error.h)
 */
extern int dlp_ReadSysInfo PI_ARGS((int sd, struct SysInfo * s));

/** @brief Read the database list from the device
 *
 * The database list can be read either one database at a time (slower),
 * or passing ::dlpDBListMultiple in the @p flags member. Pass ::dlpDBListRAM
 * in @p flags to get the list of databases in RAM, and ::dlpDBListROM to get
 * the list of databases in ROM. You can mix flags to obtain the desired
 * result. Passing ::dlpDBListMultiple will return several DBInfo
 * structures at once (usually 20). Use (info->used / sizeof(DBInfo)) to
 * know how many database information blocks were returned.
 * For the next call, pass the last DBInfo->index value + 1 to start to
 * the next database. @n @n
 * When all the database informations have been retrieved, this function returns
 * #PI_ERR_DLP_PALMOS and pi_palmos_error() returns #dlpErrNotFound.
 * 
 * @param sd Socket number
 * @param cardno Card number (should be 0)
 * @param flags Flags (see #dlpDBList enum)
 * @param start Index of first database to list (zero based)
 * @param info Buffer filled with one or more DBInfo structure
 * @return A negative value if an error occured or the DB list is exhausted (see pi-error.h)
 *
 */
extern int dlp_ReadDBList
	PI_ARGS((int sd, int cardno, int flags, int start,
		pi_buffer_t *info));

/** @brief Look for a database on the device
 *
 * This function does not match any DLP layer function, but is
 * intended as a shortcut for programs looking for databases. It
 * uses a fairly byzantine mechanism for ordering the RAM databases
 * before the ROM ones. You must feed the @a index slot from the
 * returned info in @p start the next time round.
 *
 * @param sd Socket number
 * @param cardno Card number (should be 0)
 * @param start Index of first database to list (zero based)
 * @param dbname If not NULL, look for a database with this name
 * @param type If not 0, matching database must have this type
 * @param creator If not 0, matching database must have this creator code
 * @param info Returned database information on success
 * @return A negative value if an error occured (see pi-error.h)
 */
extern int dlp_FindDBInfo
	PI_ARGS((int sd, int cardno, int start, const char *dbname,
		unsigned long type, unsigned long creator,
		struct DBInfo *info));

/** @brief Open a database on the Palm.
 *
 * @param sd Socket number
 * @param cardno Card number (should be 0)
 * @param mode Open mode (see #dlpOpenFlags enum)
 * @param name Database name
 * @param dbhandle Returned database handle to use if other calls like dlp_CloseDB()
 * @return A negative value if an error occured (see pi-error.h)
 */
extern int dlp_OpenDB
	PI_ARGS((int sd, int cardno, int mode, PI_CONST char *name,
		int *dbhandle));

/** @brief Close an opened database
 *
 * @param sd Socket number
 * @param dbhandle The DB handle returned by dlp_OpenDB()
 * @return A negative value if an error occured (see pi-error.h)
 */
extern int dlp_CloseDB PI_ARGS((int sd, int dbhandle));

/** @brief Close all opened databases
 *
 * @param sd Socket number
 * @return A negative value if an error occured (see pi-error.h)
 */
extern int dlp_CloseDB_All PI_ARGS((int sd));

/** @brief Delete an existing database from the device
 *
 * @param sd Socket number
 * @param cardno Card number (should be 0)
 * @param name Database name
 * @return A negative value if an error occured (see pi-error.h)
 */
extern int dlp_DeleteDB
	PI_ARGS((int sd, int cardno, PI_CONST char *name));

/** @brief Create database on the device
 *
 * After creation, the database is open and ready for use. You should
 * call dlp_CloseDB() once you're done with the database.
 *
 * @param sd Socket number
 * @param creator Creator code for the new database (four-char code)
 * @param type Type code for the new database (four-char code)
 * @param cardno Card number (should be 0)
 * @param flags Database flags (see #dlpDBFlags enum)
 * @param version Database version number
 * @param name Database name
 * @param dbhandle On return, DB handle to pass to other calls like dlp_CloseDB()
 * @return A negative value if an error occured (see pi-error.h)
 */
extern int dlp_CreateDB
	PI_ARGS((int sd, unsigned long creator, unsigned long type,
		int cardno, int flags, unsigned int version,
		PI_CONST char *name, int *dbhandle));

/** @brief Require reboot of device after HotSync terminates
 *
 * @param sd Socket number
 * @return A negative value if an error occured (see pi-error.h)
 */
extern int dlp_ResetSystem PI_ARGS((int sd));

/** @brief Add an entry into the HotSync log on the device
 *
 * Move to the next line with \n, as usual. You may invoke this
 * command once or more before calling dlp_EndOfSync(), but it is
 * not required.
 *
 * @param sd Socket number
 * @param entry Nul-terminated string with the text to insert in the log
 * @return A negative value if an error occured (see pi-error.h)
 */
extern int dlp_AddSyncLogEntry PI_ARGS((int sd, char *entry));

/** @brief State that a conduit has started running on the desktop
 *
 * Puts up a status message on the device. Calling this method regularly
 * is also the only reliable way to know whether the user pressed the Cancel
 * button on the device.
 *
 * @param sd Socket number
 * @return A negative value if an error occured (see pi-error.h)
 */
extern int dlp_OpenConduit PI_ARGS((int sd));

/** @brief Terminate connection with the device
 *
 * Required at the end of a session. The pi_socket layer
 * will call this for you if you don't. After the device receives this
 * command, it will terminate the connection.
 *
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
