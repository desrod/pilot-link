/* sync.c:  Pilot synchronization logic
 *
 * Code written by Kenneth Albanowski, based on work apparently
 * Copyright (C) 1996, U.S. Robotics Inc. (Possibly Copyright
 * Palm Computing, Inc., or Copyright The Windward Group.)
 *
 * This file contains no information directly copied from the PC Conduit SDK,
 * but paraphrases many of the algorithms used in that source.
 *
 */
 
/* Note: This file currently does nothing useful, and even if completed
 * won't give you a full HotSync manager. 
 * 
 * Mostly I am including this as an exploration of the abstractions
 * needed to portably sync local databases.
 *
 * Revised note: most of the algorithms needed are here, except for ones
 * that checks the Pilot to decided whether slow or fast sync is needed, and
 * to deal with category synchronization.
 * Always use slow sync for now (and be sure to modify the Pilot's 
 * PCID afterwards.)
 *  
 * Revised revision: all sections of code directly derived (albeit in a
 * paraphased manner) from code in the Palm Conduit SDK, _which are not
 * blatently obvious and irreducible_ have been temporarily removed until
 * word is obtained from Palm that the code may be publically released.
 *
 * The issue that needs to be resolved is whether Palm Computing Inc. will
 * allow the public distribution of source code derived from source code in
 * their Pilot PC Conduit SDK. The licensing information that I recieved
 * with the PC and Mac SDKs is not sufficient to resolve this question.
 *
 * In keeping with the sprit of the EU practice for reverse engineering, the
 * interface that I have designed to plug into the Palm algorithms has been
 * retained.
 *
 */
 
#include <stdio.h>
#include "pi-socket.h"
#include "dlp.h"

#define Abstract_sync
#include "sync.h"

/* Given a remote (Pilot) record, stored in a PilotRecord structure, determine what,
   if anything, should be done with it, by looking at its flags, and possibly looking
   it up in the local database. */
int SyncRecord(int handle, int db, PilotRecord * Remote, struct SyncAbs * s, int slowsync) {
  /* --- Paraphrased code derived from Palm;s Conduit SDK elided --- */
  abort(); /* For lack of anything better to do */
  return 0;
}

/* Iterate over local records, copying records to remote, or deleting, or
    archiving, as flags dictate. This is the last step in any sync. */
void MergeToRemote(int handle, int db, struct SyncAbs * s) {
  /* --- Paraphrased code derived from Palm's Conduit SDK elided --- */
  abort(); /* For lack of anything better to do */
  return;
}

/* Perform a "slow" sync. This requires that the local (PC) has
   consistent, accurate, and sufficient modification flags. All
   of the records on the remote (Pilot) are pulled in, and compared
   for modifications */
int SlowSync(int handle, int db, struct SyncAbs * s ) {
	int index = 0;
	int retval = 0;
	char buffer[0xffff];
	PilotRecord p;
	p.record = buffer;
	
  	/* --- Paraphrased code derived from Palm's Conduit SDK elided --- */
	index = 0;
	
	while(dlp_ReadRecordByIndex(handle,db, index, p.record, &p.ID, &p.length, &p.attr, &p.category)>=0) {
		p.secret = p.attr & dlpRecAttrSecret;
		p.archived = p.attr & dlpRecAttrArchived;
		if(p.attr & dlpRecAttrDeleted)
			p.attr = RecordDeleted;
		else if (p.attr & dlpRecAttrDirty)
			p.attr = RecordModified;
		else
			p.attr = RecordNothing;
		SyncRecord(handle, db, &p, s, 1);
		index++;
	}
	
	MergeToRemote(handle,db,s);
	
	return retval;
}

/* Perform a "fast" sync. This requires that both the remote (Pilot) and
   local (PC) have consistent, accurate, and sufficient modification flags.
   If this is not true, a slow sync should be used */
int FastSync(int handle, int db, struct SyncAbs * s ) {
	int index = 0;
	int retval = 0;
	char buffer[0xffff];
	PilotRecord p;
	p.record = buffer;
	
	while(dlp_ReadNextModifiedRec(handle,db, p.record, &p.ID, &index, &p.length, &p.attr, &p.category)>=0) {
	        printf("Got a modified record\n");
		p.secret = p.attr & dlpRecAttrSecret;
		p.archived = p.attr & dlpRecAttrArchived;
		if(p.attr & dlpRecAttrDeleted)
			p.attr = RecordDeleted;
		else if (p.attr & dlpRecAttrDirty)
			p.attr = RecordModified;
		else
			p.attr = RecordNothing;
		SyncRecord(handle, db, &p, s, 0);
	}
	
	MergeToRemote(handle,db,s);
	
	return retval;
}

/* Overwrite remote (Pilot) with local (PC) records */
int CopyToRemote(int handle, int db, struct SyncAbs * s) {
	LocalRecord * Local = 0;
	int retval = 0;
	dlp_DeleteRecord(handle, db, 1, 0);
	while(s->Iterate(s,&Local) && Local) {
		if (Local->archived) {
			retval = s->ClearStatusArchiveLocal(s,Local);
			s->SetStatus(s,Local,RecordDeleted);
		} else if (Local->attr != RecordDeleted) {
			PilotRecord * p = s->Transmit(s,Local);
			s->SetStatus(s,Local,RecordNothing);
			p->attr = 0;
			if(p->secret)
				p->attr |= dlpRecAttrSecret ;
			retval = (dlp_WriteRecord(handle, db, p->attr, p->ID, 
		                p->category, p->record, p->length, 0) < 0 );
		        s->FreeTransmit(s,Local, p);
		}
	}
	s->Purge(s);
	return retval;
}

/* Overwrite local (PC) with remote (Pilot) records. */
int CopyFromRemote(int handle, int db, struct SyncAbs * s) {
	char buffer[0xffff];
	int index = 0;
	PilotRecord p;
	p.record = buffer;
	s->DeleteAll(s);
	while(dlp_ReadRecordByIndex(handle,db, index, p.record, &p.ID, &p.length, &p.attr, &p.category)>=0) {
		p.secret = p.attr & dlpRecAttrSecret;
		p.archived = p.attr & dlpRecAttrArchived;
		if(p.attr & dlpRecAttrDeleted)
			p.attr = RecordDeleted;
		else if (p.attr & dlpRecAttrDirty)
			p.attr = RecordModified;
		else
			p.attr = RecordNothing;
		if( p.archived) {
			p.attr = 0;
			p.archived = 0;
			s->ArchiveRemote(s,0,&p);
		} else if (p.attr != RecordDeleted) {
			p.attr = 0;
			p.archived = 0;
			s->StoreRemote(s,&p);
		}
		index++;
	}
	dlp_CleanUpDatabase(handle,db);
	return 0;
}
