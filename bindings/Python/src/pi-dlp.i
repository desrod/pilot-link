// -*-C-*-

extern char * dlp_errorlist[];
extern char * dlp_strerror(int error);

enum dlpDBFlags {
	dlpDBFlagResource = 0x0001, /* Resource DB, instead of record DB */
	dlpDBFlagReadOnly = 0x0002, /* DB is read only */
	dlpDBFlagAppInfoDirty = 0x0004, /* AppInfo data has been modified */
	dlpDBFlagBackup = 0x0008, /* DB is tagged for generic backup */
	dlpDBFlagClipping = 0x0200, /* DB is a Palm Query Application (PQA) */
	dlpDBFlagOpen = 0x8000, /* DB is currently open */
	
	/* v2.0 specific */
	dlpDBFlagNewer = 0x0010, /* Newer version may be installed over open DB */
	dlpDBFlagReset = 0x0020, /* Reset after installation */

	/* v3.0 specific */
	dlpDBFlagCopyPrevention = 0x0040, /* DB should not be beamed */
	dlpDBFlagStream = 0x0080          /* DB implements a file stream */
};

enum dlpDBMiscFlags {
	dlpDBMiscFlagExcludeFromSync = 0x80
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
extern DLPERROR dlp_WriteNetSyncInfo (int sd, struct NetSyncInfo * i);
extern DLPERROR dlp_ReadAppPreference (int sd, unsigned long STR4, int id_, int backup,
				       int DLPMAXBUF, void *OUTBUF, size_t *OUTBUFLEN, 
				       int *OUTPUT);
extern DLPERROR dlp_WriteAppPreference (int sd, unsigned long STR4, int id, int backup,
					int version, void *INBUF, size_t INBUFLEN);
// and the most complex of all... i'm not even sure how it works.
//extern int dlp_RPC (int sd, struct RPC_params * p, unsigned long *OUTPUT);
