/* sync.c:  Pilot synchronization logic
 *
 * (c) 1996, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */
 
/* Note: This file currently does nothing useful, and even if completed
 * won't give you a full HotSync manager. 
 * 
 * Mostly I am including this as an exploration of the abstractions
 * needed to portably sync local databases.
 *  
 */

#include <stdio.h>
#include <errno.h>
#include "pi-socket.h"
#include "padp.h"
#include "dlp.h"
#include "sync.h"

struct PilotRecord {
	long ID;
	long attr;
	int length;
	unsigned char * record;
};

typedef int (*f1)(long);
typedef int (*f2)(long, long*);
typedef int (*f3)(struct PilotRecord *);
typedef int (*f4)(long, int);

struct SyncAbs {
	f2 MatchRecord;
	f1 IsNew;
	f1 IsModified;
	f1 IsArchived;
	f1 NoStatus;
	f3 AppendLocal;
	f3 AppendArchive;
	f4 SetArchive;
	f4 SetStatus;
	f3 UseBackupStatus;
};

void SyncRecord(struct PilotRecord * remote, struct SyncAbs * s) {
  long local;
  if( remote->attr & Archived ) {
    if(!s->MatchRecord(remote->id,&local)) {
      s->AppendArchive(remote);
    } else {
      if(!s->IsModified(local)) {
        s->AppendArchive(remote);
        s->SetArchive(local,0);
        s->SetStatus(local,Pending);
      } else {
        if(s->NoStatus(local)) {
          s->UseBackupStatus(remote);
        }
        if(s->IsModifed()) {
          /* Etc */
        }
      }
    }
  }
  /* Etc */
}
