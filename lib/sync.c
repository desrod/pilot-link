/* sync.c:  Pilot synchronization logic
 *
 * Copyright 1996, Kenneth Albanowski
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
 
#include "pi-socket.h"
#include "dlp.h"
#include "sync.h"

void SyncRecord(PilotRecord * remote, struct SyncAbs * s, void * ld) {
  long local;
  if( remote->attr & dlpRecAttrArchived ) {
#if 0
    if(!s->MatchRecord(remote->ID,&local)) {
      s->AppendArchive(remote);
    } else {
      if(!s->IsModified(local)) {
        s->AppendArchive(remote);
        s->SetArchive(local,0);
        s->SetStatus(local,dlpRecAttrPending);
      } else {
        if(s->NoStatus(local)) {
          s->UseBackupStatus(remote);
        }
        if(s->IsModifed()) {
          /* Etc */
        }
      }
    }
#endif  
  }
  /* Etc */
}

void SyncDB(int handle, int db, struct SyncAbs * s, void * ld) {
	/* Slow sync */
	char buffer[0xffff];
	int i;
	int len;
	PilotRecord p;
	
	for(i=0;1;i++) {
  	                           
	  	int len = dlp_ReadRecordByIndex(handle, db, i, buffer, 0, 0, &p.attr, &p.category);
	  	if(len<0)
  			break;
		SyncRecord(&p, s, ld);
	}
	/* More stuff needed to process local flags */
}
