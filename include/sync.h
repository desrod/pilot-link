#ifndef _PILOT_SYNC_H_
#define _PILOT_SYNC_H_

enum { RecordNothing , RecordNew, RecordDeleted, RecordModified, RecordPending };

struct PilotRecord {
	long ID;
	int attr;
	int archived;
	int secret;
	int length;
	int category;
	unsigned char * record;
};

struct SyncAbs;
struct LocalRecord;

typedef struct SyncAbs SyncAbs;
typedef struct LocalRecord LocalRecord;
typedef struct PilotRecord PilotRecord;

/* This is a bit complex: we are setting up the list of structure members
   needed to implement synchronization. These are defined outside of a 
   structure definition, so they can be pasted into multiple definitions,
   with the result of two structures sharing (at least in part) the same
   layout. It's very likely that this approach is less portable then using
   void*'s, but its less messy to read. */

#define StandardLocalRecord \
	int attr; \
	int archived; \
	int secret

#define StandardSyncAbs \
	int (*MatchRecord)(SyncAbs*, LocalRecord**, PilotRecord*); \
	int (*FreeMatch)(SyncAbs*, LocalRecord**); \
	int (*ArchiveLocal)(SyncAbs*, LocalRecord*); \
	int (*ArchiveRemote)(SyncAbs*,LocalRecord*,PilotRecord*); \
	int (*StoreRemote)(SyncAbs*,PilotRecord*); \
	int (*ClearStatusArchiveLocal)(SyncAbs*,LocalRecord*); \
	int (*Iterate)(SyncAbs*,LocalRecord**); \
	int (*IterateSpecific)(SyncAbs*,LocalRecord**, int flag, int archived); \
	int (*Purge)(SyncAbs*); \
	int (*SetStatus)(SyncAbs*,LocalRecord*,int status); \
	int (*SetArchived)(SyncAbs*,LocalRecord*, int); \
	unsigned long (*GetPilotID)(SyncAbs*,LocalRecord*); \
	int (*SetPilotID)(SyncAbs*,LocalRecord*,unsigned long); \
	int (*Compare)(SyncAbs*,LocalRecord*,PilotRecord*); \
	int (*CompareBackup)(SyncAbs*,LocalRecord*,PilotRecord*); \
	int (*FreeTransmit)(SyncAbs*,LocalRecord*,PilotRecord*); \
	int (*DeleteAll)(SyncAbs*); \
	PilotRecord * (*Transmit)(SyncAbs*,LocalRecord*)

#ifdef Abstract_sync

/* Only lib/sync should define Abstract_sync. All other code must
   define their own LocalRecord and SyncAbs structures. */

struct LocalRecord {
        StandardLocalRecord;
};

struct SyncAbs {
        StandardSyncAbs;
};

#endif

/* Erase all local records, and copy all remote records to local */
int CopyFromRemote(int handle, int db, struct SyncAbs * s);
/* Erase all remote records, and copy all local records to remote */
int CopyToRemote(int handle, int db, struct SyncAbs * s);
/* Synchronize local and remote using flags on remote */
int FastSync(int handle, int db, struct SyncAbs * s );
/* Synchronize by pulling all data off remote, and comparing to backup
   on local if flags show no change */
int SlowSync(int handle, int db, struct SyncAbs * s );

#endif
