/* sync.h: Header for generic synchronization algorithm
 *
 * Copyright (c) 2000, Helix Code Inc.
 *
 * Author: JP Rosevear <jpr@helixcode.com> 
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

#ifndef _PILOT_SYNC_H_
#define _PILOT_SYNC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "pi-macros.h"

typedef struct _SyncHandler SyncHandler;
typedef struct _DesktopRecord DesktopRecord;
typedef struct _PalmRecord PalmRecord;
	
struct _DesktopRecord {
	int recID;
	int catID;
	int flags;
};

struct _PalmRecord {
	recordid_t recID;
	int catID;
	int flags;
	void *buffer;
	int len;
};

struct _SyncHandler {
	int sd;
	char *name;
	void *data;

	int (*SetRecordID) (SyncHandler *, recordid_t, DesktopRecord *);
	int (*GetRecordByID) (SyncHandler *, recordid_t, DesktopRecord **);
	int (*CompareRecords) (SyncHandler *, PalmRecord *, DesktopRecord *);
	
	int (*ForEachRecord) (SyncHandler *, DesktopRecord **);
	int (*ForEachModifiedRecord) (SyncHandler *, DesktopRecord **);
	
	int (*AddRecord) (SyncHandler *, PalmRecord *);
	int (*AddArchiveRecord) (SyncHandler *, PalmRecord *);

	int (*DeleteRecord) (SyncHandler *, DesktopRecord *);
	int (*DeleteArchiveRecord) (SyncHandler *, DesktopRecord *);
	
	int (*PrepareRecord) (SyncHandler *, DesktopRecord *, PalmRecord **);
	int (*FreeRecord) (SyncHandler *, PalmRecord *);
};

PalmRecord *sync_NewPalmRecord (int buf_size);
DesktopRecord *sync_NewDesktopRecord (void);	

int sync_CopyToRemote (SyncHandler *sh);
int sync_CopyFromRemote (SyncHandler *sh);

int sync_MergeToRemote (SyncHandler *sh, int slow);
int sync_MergeFromRemote (SyncHandler *sh, int slow);

int sync_Synchronize (SyncHandler *sh, int slow);

#ifdef __cplusplus
}
#endif

#endif
