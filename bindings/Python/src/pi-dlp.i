// -*-C-*-

extern char * dlp_strerror(int error);

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

enum dlpDBMiscFlags {
	dlpDBMiscFlagExcludeFromSync = 0x80,
	dlpDBMiscFlagRamBased 	= 0x40		/**< DLP 1.2 and later: this database is in RAM */
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
	dlpDBListROM = 0x40,
	dlpDBListMultiple = 0x20		/* defined for DLP 1.2 */
};

enum dlpErrors {
	dlpErrNoError = 0,	/**< No error */
	dlpErrSystem,		/**< System error (0x0001) */
	dlpErrIllegalReq,	/**< Illegal request, not supported by this version of DLP (0x0002) */
	dlpErrMemory,		/**< Not enough memory (0x0003) */
	dlpErrParam,		/**< Invalid parameter (0x0004) */
	dlpErrNotFound,		/**< File, database or record not found (0x0005) */
	dlpErrNoneOpen,		/**< No file opened (0x0006) */
	dlpErrAlreadyOpen,	/**< File already open (0x0007) */
	dlpErrTooManyOpen,	/**< Too many open files (0x0008) */
	dlpErrExists,		/**< File already exists (0x0009) */
	dlpErrOpen,		/**< Can't open file (0x000a) */
	dlpErrDeleted,		/**< File deleted (0x000b) */
	dlpErrBusy,		/**< Record busy (0x000c) */
	dlpErrNotSupp,		/**< Call not supported (0x000d) */
	dlpErrUnused1,		/**< @e Unused (0x000e) */
	dlpErrReadOnly,		/**< File is read-only (0x000f) */
	dlpErrSpace,		/**< Not enough space left on device (0x0010) */
	dlpErrLimit,		/**< Limit reached (0x0011) */
	dlpErrSync,		/**< Sync error (0x0012) */
	dlpErrWrapper,		/**< Wrapper error (0x0013) */
	dlpErrArgument,		/**< Invalid argument (0x0014) */
	dlpErrSize,		/**< Invalid size (0x0015) */

	dlpErrUnknown = 127	/**< Unknown error (0x007F) */
};

// DLP functions
extern DLPERROR dlp_GetSysDateTime (int sd, time_t *OUTPUT);
extern DLPERROR dlp_SetSysDateTime (int sd, time_t INPUT);
extern DLPERROR dlp_ReadStorageInfo (int sd, int cardno, struct CardInfo *OUTPUT);
extern DLPERROR dlp_ReadSysInfo (int sd, struct SysInfo *OUTPUT);
extern DLPERROR dlp_ReadDBList (int sd, int cardno, int flags, int start,
				pi_buffer_t *OUTDBInfoList);
// note: creator and type are 4-char strings or None, and name is a string or None.
extern DLPERROR dlp_FindDBInfo (int sd, int cardno, int start, const char *ALLOWNULL,
				unsigned long STR4,
				unsigned long STR4, struct DBInfo *OUTPUT);
extern DLPERROR dlp_OpenDB (int sd, int cardno, int mode, PI_CONST char * name, int *OUTPUT);
extern DLPERROR dlp_CloseDB (int sd, int dbhandle);
extern DLPERROR dlp_CloseDB_All (int sd);
extern DLPERROR dlp_DeleteDB (int sd, int cardno, const char * name);
// note: creator and type are 4-char strings.
extern DLPERROR dlp_CreateDB (int sd, unsigned long STR4, unsigned long STR4, 
			      int cardno, int flags, unsigned int version, 
			      const char * name, int *OUTPUT);
extern DLPERROR dlp_ResetSystem (int sd);
extern DLPERROR dlp_AddSyncLogEntry (int sd, char *entry);
extern DLPERROR dlp_OpenConduit (int sd);
extern DLPERROR dlp_EndOfSync (int sd, int status);
extern DLPERROR dlp_AbortSync (int sd);
extern DLPERROR dlp_ReadOpenDBInfo (int sd, int dbhandle, int *OUTPUT);
extern DLPERROR dlp_MoveCategory (int sd, int handle, int fromcat, int tocat);
extern DLPERROR dlp_WriteUserInfo (int sd, struct PilotUser *User);
extern DLPERROR dlp_ReadUserInfo (int sd, struct PilotUser *OUTPUT);
extern DLPERROR dlp_ResetLastSyncPC (int sd);

extern DLPERROR dlp_ReadAppBlock(int sd, int dbhandle, int offset, 
				 int reqbytes, 
				 pi_buffer_t *OUTBUF);

extern DLPERROR dlp_WriteAppBlock(int sd, int dbhandle, const void *INBUF, size_t INBUFLEN);

extern DLPERROR dlp_ReadSortBlock(int sd, int dbhandle, int offset, 
				 int reqbytes, 
				 pi_buffer_t *OUTBUF);


extern DLPERROR dlp_WriteSortBlock(int sd, int dbhandle, const void *INBUF, size_t INBUFLEN);

extern DLPERROR dlp_ResetDBIndex (int sd, int dbhandle);

%native(dlp_ReadRecordIDList) PyObject *_wrap_dlp_ReadRecordIDList(PyObject *, PyObject *);

extern DLPDBERROR dlp_WriteRecord (int sd, int dbhandle, int flags,
				 recordid_t INPUT, int catID, void *INBUF,
				 size_t INBUFLEN, recordid_t *OUTPUT);
extern DLPERROR dlp_DeleteRecord (int sd, int dbhandle, int all, recordid_t INPUT);
extern DLPERROR dlp_DeleteCategory (int sd, int dbhandle, int category);

extern DLPDBERROR dlp_ReadResourceByType (int sd, int fHandle, unsigned long STR4, int id,
				   pi_buffer_t *OUTBUF, int *OUTPUT);
extern DLPDBERROR dlp_ReadResourceByIndex (int sd, int fHandle, int index, 
					   pi_buffer_t *OUTBUF, unsigned long *OUTSTR4,
					   int *OUTPUT);

extern DLPDBERROR dlp_WriteResource (int sd, int dbhandle, unsigned long STR4, int id,
				   const void *INBUF, size_t INBUFLEN);
extern DLPERROR dlp_DeleteResource (int sd, int dbhandle, int all, unsigned long STR4,
				    int resID);

extern DLPDBERROR dlp_ReadNextModifiedRec (int sd, int fHandle, pi_buffer_t *OUTBUF,
					 recordid_t *OUTPUT, int *OUTPUT,
					 int *OUTPUT, int *OUTPUT);
extern DLPDBERROR dlp_ReadNextModifiedRecInCategory (int sd, int fHandle, int incategory,
						     pi_buffer_t *OUTBUF, 
						     recordid_t *OUTPUT,
						     int *OUTPUT, int *OUTPUT);
extern DLPDBERROR dlp_ReadNextRecInCategory (int sd, int fHandle, int incategory,
					   pi_buffer_t *OUTBUF,
					   recordid_t *OUTPUT, int *OUTPUT,
					     int *OUTPUT);
extern DLPDBERROR dlp_ReadRecordById (int sd, int fHandle, recordid_t INPUT, 
				      pi_buffer_t *OUTBUF, int *OUTPUT, int *OUTPUT, 
				      int *OUTPUT);
extern DLPDBERROR dlp_ReadRecordByIndex (int sd, int fHandle, int index, 
					 pi_buffer_t *OUTBUF, 
					 recordid_t *OUTPUT, 
					 int *OUTPUT, int *OUTPUT);
extern DLPERROR dlp_CleanUpDatabase (int sd, int fHandle);
extern DLPERROR dlp_ResetSyncFlags (int sd, int fHandle);
// complex enough to probably need native code.
//extern int dlp_CallApplication (int sd, unsigned long STR4, unsigned long STR4, int action,
//                        int length, void * data,
//                        unsigned long *OUTPUT, int maxretlen, int *OUTPUT, void * retdata);
extern DLPERROR dlp_ReadFeature (int sd, unsigned long STR4, unsigned int num, 
				 unsigned long *OUTPUT);
extern DLPERROR dlp_ReadNetSyncInfo (int sd, struct NetSyncInfo *OUTPUT);
extern DLPERROR dlp_WriteNetSyncInfo (int sd, const struct NetSyncInfo *i);
extern DLPERROR dlp_ReadAppPreference (int sd, unsigned long STR4, int id_, int backup,
				       int DLPMAXBUF, void *OUTBUF, size_t *OUTBUFLEN, 
				       int *OUTPUT);
extern DLPERROR dlp_WriteAppPreference (int sd, unsigned long STR4, int id, int backup,
					int version, void *INBUF, size_t INBUFLEN);

