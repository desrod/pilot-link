#ifndef _PILOT_SYNC_H_
#define _PILOT_SYNC_H_

typedef struct {
	long ID;
	unsigned int attr;
	int length;
	int category;
	unsigned char * record;
} PilotRecord;

/*typedef int (*f1)(long);
typedef int (*f2)(long, long*);
typedef int (*f3)(struct PilotRecord *);
typedef int (*f4)(long, int);*/

struct SyncAbs {
	int (*MatchRecord)(PilotRecord*, void**LocalRecord, void*LocalData);
	int (*IsNew)(void*,void*);
	int (*IsModified)(void*,void*);
	int (*IsArchived)(void*,void*);
	int (*IsNothing)(void*,void*);
	int (*AppendLocal)(PilotRecord*,void*);
	int (*AppendArchive)(PilotRecord*,void*);
	/*f4 SetArchive;
	f4 SetStatus;
	f3 UseBackupStatus;*/
};


#endif
