/*
 * sync.c:  Implement generic synchronization algorithm
 *
 * Copyright (c) 2000, Helix Code Inc.
 *
 * Author: JP Rosevear <jpr@helixcode.com> 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdlib.h>
#include <string.h>

#include "pi-dlp.h"
#include "pi-sync.h"

#define PErrorCheck(r) if (r < 0) return r;
#define ErrorCheck(r)  if (r != 0) return r; 

PilotRecord *
sync_NewPilotRecord (int buf_size)
{
	PilotRecord *precord;
	
	precord = (PilotRecord *) malloc (sizeof (PilotRecord));
	memset (precord, 0, sizeof (PilotRecord));
	
	precord->buffer = malloc (buf_size);
	
	return precord;
}

DesktopRecord *
sync_NewDesktopRecord (void)
{
	DesktopRecord *drecord;
	
	drecord = (DesktopRecord *) malloc (sizeof (DesktopRecord));
	memset (drecord, 0, sizeof (DesktopRecord));

	return drecord;
}

static int
delete_both (SyncHandler *sh, int dbhandle, DesktopRecord *drecord, PilotRecord *precord)
{
	int result = 0;
	
	if (drecord != NULL) {
		result = sh->DeleteRecord (sh, drecord);
		ErrorCheck(result);
	}
	
	
	if (precord != NULL) {
		result = dlp_DeleteRecord (sh->sd, dbhandle, 0, precord->recID);
		ErrorCheck(result);
	}

	return result;
}

static int
store_record_on_pilot (SyncHandler *sh, int dbhandle, DesktopRecord *drecord)
{
	PilotRecord precord;
	recordid_t id;
	int result = 0;
	
	memset (&precord, 0, sizeof (PilotRecord));
	result = sh->Prepare (sh, drecord, &precord);
	ErrorCheck(result);

	result = dlp_WriteRecord (sh->sd, dbhandle, 0,
				  precord.recID, precord.catID, 
				  precord.buffer, precord.len, &id);
	PErrorCheck(result);

	result = sh->SetPilotID (sh, drecord, id);

	return result;
}

static int
close_db (SyncHandler *sh, int dbhandle)
{
	dlp_CleanUpDatabase (sh->sd, dbhandle);
	
	dlp_ResetSyncFlags (sh->sd, dbhandle);
	dlp_CloseDB (sh->sd, dbhandle);

	return 0;
}

static int
sync_record (SyncHandler *sh, int dbhandle, DesktopRecord *drecord, PilotRecord *precord)
{
	int parch = 0, pdel = 0, pchange = 0;
	int darch = 0, ddel = 0, dchange = 0;
	int result = 0;
	
	/* The flags are calculated like this because the deleted and dirty
	   pilot flags are not mutually exclusive */
	if (precord) {
		parch = precord->flags & dlpRecAttrArchived;
		pdel = precord->flags & dlpRecAttrDeleted;
		pchange = (precord->flags & dlpRecAttrDirty) && (!pdel);
	}
	
	if (drecord) {
		darch = drecord->flags & dlpRecAttrArchived;
		ddel = drecord->flags & dlpRecAttrDeleted;
		dchange = (drecord->flags & dlpRecAttrDirty) && (!ddel);
	}
	
	if (precord != NULL && drecord == NULL) {
		sh->AddRecord (sh, precord);

	} else if (precord == NULL && drecord != NULL) {
		store_record_on_pilot (sh, dbhandle, drecord);
		
	} else if (parch && ddel) {
		sh->ReplaceRecord (sh, drecord, precord);
		sh->ArchiveRecord (sh, drecord, 0);
		sh->SetStatusCleared (sh, drecord);
		
	} else if (parch && !darch && !dchange) {
		sh->ArchiveRecord (sh, drecord, 1);
		
	} else if (parch && drecord == NULL) {
		sh->AddRecord (sh, precord);
		sh->ArchiveRecord (sh, drecord, 1);
		
	} else if (parch && pchange && !darch && dchange) {
		int comp;
		
		comp = sh->Compare (sh, precord, drecord);
		if (comp == 0) {
			sh->ArchiveRecord (sh, drecord, 1);
			result = dlp_DeleteRecord (sh->sd, dbhandle, 0, precord->recID);
			ErrorCheck(result);
			sh->SetStatusCleared (sh, drecord);
		} else {
			result = dlp_DeleteRecord (sh->sd, dbhandle, 0, precord->recID);
			ErrorCheck(result);
			store_record_on_pilot (sh, dbhandle, drecord);				
			sh->AddRecord (sh, precord);
			sh->SetStatusCleared (sh, drecord);
		}
		
	} else if (parch && !pchange && !darch && dchange) {
		dlp_DeleteRecord (sh->sd, dbhandle, 0, precord->recID);
		store_record_on_pilot (sh, dbhandle, drecord);
		sh->SetStatusCleared (sh, drecord);

	} else if (pchange && darch && dchange) {
		int comp;
		
		comp = sh->Compare (sh, precord, drecord);
		if (comp == 0) {
			result = dlp_DeleteRecord (sh->sd, dbhandle, 0, precord->recID);
			ErrorCheck(result);
		} else {
			sh->ArchiveRecord (sh, drecord, 0);
			sh->ReplaceRecord (sh, drecord, precord);
		}
		sh->SetStatusCleared (sh, drecord);

	} else if (pchange && darch && !dchange) {
		sh->ArchiveRecord (sh, drecord, 0);
		sh->ReplaceRecord (sh, drecord, precord);
		sh->SetStatusCleared (sh, drecord);
			
	} else if (pchange && dchange) {
		int comp;
		
		comp = sh->Compare (sh, precord, drecord);
		if (comp != 0) {
			sh->AddRecord (sh, precord);
			drecord->recID = 0;
 			store_record_on_pilot (sh, dbhandle, drecord);
		}
		sh->SetStatusCleared (sh, drecord);
		
	} else if (pchange && ddel) {
		sh->ReplaceRecord (sh, drecord, precord);
		sh->SetStatusCleared (sh, drecord);
		
	} else if (pchange && !dchange) {
		sh->ReplaceRecord (sh, drecord, precord);
		sh->SetStatusCleared (sh, drecord);
		
	} else if (pdel && dchange) {
		store_record_on_pilot (sh, dbhandle, drecord);
		sh->SetStatusCleared (sh, drecord);
		
	} else if (pdel && !dchange) {
		delete_both (sh, dbhandle, drecord, precord);
		sh->SetStatusCleared (sh, drecord);
		
	} else if (!pchange && darch) {
		dlp_DeleteRecord (sh->sd, dbhandle, 0, precord->recID);
		sh->SetStatusCleared (sh, drecord);
		
	} else if (!pchange && dchange) {
		dlp_DeleteRecord (sh->sd, dbhandle, 0, precord->recID);
		store_record_on_pilot (sh, dbhandle, drecord);
		sh->SetStatusCleared (sh, drecord);
		
	} else if (!pchange && ddel) {
		delete_both (sh, dbhandle, drecord, precord);
		sh->SetStatusCleared (sh, drecord);

	}

	return 0;
}

int
sync_CopyToPilot (SyncHandler *sh)
{
	int dbhandle;
	DesktopRecord *drecord = NULL;
	int slow = 0;
	int result = 0;
	
	result = dlp_OpenDB (sh->sd, 0, dlpOpenReadWrite, sh->name, &dbhandle);
	PErrorCheck(result);

	result = sh->Pre (sh, dbhandle, &slow);
	ErrorCheck(result);
	
	result = dlp_DeleteRecord (sh->sd, dbhandle, 1, 0);
	PErrorCheck(result);
	
	while (sh->ForEach (sh, &drecord) == 0 && drecord) {
		result = store_record_on_pilot (sh, dbhandle, drecord);
		if (result != 0) break;
	}

	result = sh->Post (sh, dbhandle);

	close_db (sh, dbhandle);

	return result;
}

int
sync_CopyFromPilot (SyncHandler *sh)
{
	int dbhandle;
	int index;
	DesktopRecord *drecord = NULL;
	PilotRecord *precord = sync_NewPilotRecord (DLP_BUF_SIZE);
	int slow = 0;
	int result = 0;

	result = dlp_OpenDB (sh->sd, 0, dlpOpenReadWrite, sh->name, &dbhandle);
	PErrorCheck(result);

	result = sh->Pre (sh, dbhandle, &slow);
	ErrorCheck(result);
	
	while (sh->ForEach (sh, &drecord) == 0 && drecord) {
		result = sh->DeleteRecord (sh, drecord);
		ErrorCheck(result);
	}
	
	index = 0;
	while (dlp_ReadRecordByIndex (sh->sd, dbhandle, index, precord->buffer, 
				      &precord->recID, &precord->len, 
				      &precord->flags, &precord->catID) > 0) {
		result = sh->AddRecord (sh, precord);
		ErrorCheck(result);
		
		index++;
	}

	result = sh->Post (sh, dbhandle);

	close_db (sh, dbhandle);

	return result;
}

static int
sync_MergeFromPilot_fast (SyncHandler *sh, int dbhandle)
{
	PilotRecord *precord = sync_NewPilotRecord (DLP_BUF_SIZE);
	DesktopRecord *drecord = NULL;
	int result = 0;
	
	while (dlp_ReadNextModifiedRec (sh->sd, dbhandle, precord->buffer,
					&precord->recID, NULL, &precord->len,
					&precord->flags, &precord->catID) >= 0) {
		result = sh->Match (sh, precord, &drecord);
		ErrorCheck(result);
		
		result = sync_record (sh, dbhandle, drecord, precord);
		ErrorCheck(result);

		if (drecord) {
			result = sh->FreeMatch (sh, drecord);
			ErrorCheck(result);
		}
	}

	return result;
}

static int
sync_MergeFromPilot_slow (SyncHandler *sh, int dbhandle)
{
	PilotRecord *precord = sync_NewPilotRecord (DLP_BUF_SIZE);
	DesktopRecord *drecord = NULL;
	int index;
	int parch, psecret;
	int result = 0;

	index = 0;
	while (dlp_ReadRecordByIndex (sh->sd, dbhandle, index, precord->buffer, 
				      &precord->recID, &precord->len, 
				      &precord->flags, &precord->catID) > 0) {
		result = sh->Match (sh, precord, &drecord);
		ErrorCheck(result);

		/* Since this is a slow sync, we must calculate the flags */
		parch = precord->flags & dlpRecAttrArchived;
		psecret = precord->flags & dlpRecAttrSecret;

		precord->flags = 0;
		if (drecord == NULL) {
			precord->flags = precord->flags | dlpRecAttrDirty;
		} else {
			int comp;
			
			comp = sh->Compare (sh, precord, drecord);
			if (comp != 0) {
				precord->flags = precord->flags | dlpRecAttrDirty;
			}
		}
		if (parch)
			precord->flags = precord->flags | dlpRecAttrArchived;
		if (psecret)
			precord->flags = precord->flags | dlpRecAttrSecret;
		
		result = sync_record (sh, dbhandle, drecord, precord);
		ErrorCheck(result);

		if (drecord) {
			result = sh->FreeMatch (sh, drecord);
			ErrorCheck(result);
		}
		
		index++;
	}	

	return result;
}

int
sync_MergeFromPilot (SyncHandler *sh)
{
	int dbhandle;
	int slow = 0;
	int result = 0;
	
	result = dlp_OpenDB (sh->sd, 0, dlpOpenReadWrite, sh->name, &dbhandle);
	PErrorCheck(result);

	result = sh->Pre (sh, dbhandle, &slow);
	ErrorCheck(result);
	
	if (!slow)
		result = sync_MergeFromPilot_fast (sh, dbhandle);
	else
		result = sync_MergeFromPilot_slow (sh, dbhandle);
	ErrorCheck(result);

	result = sh->Post (sh, dbhandle);

	close_db (sh, dbhandle);

	return result;
}

static int
sync_MergeToPilot_fast (SyncHandler *sh, int dbhandle)
{
	PilotRecord *precord = NULL;
	DesktopRecord *drecord = NULL;
	int result = 0;
	
	while (sh->ForEachModified (sh, &drecord) == 0 && drecord) {
		if (drecord->recID != 0) {
			precord = sync_NewPilotRecord (DLP_BUF_SIZE);
			precord->recID = drecord->recID;
			result = dlp_ReadRecordById (sh->sd, dbhandle, 
						     precord->recID,
						     precord->buffer, 
						     NULL, &precord->len, 
						     &precord->flags,
						     &precord->catID);
			PErrorCheck(result);
		}
		
		result = sync_record (sh, dbhandle, drecord, precord);
		ErrorCheck(result);

		if (precord)
			free (precord);
		precord = NULL;
	}
	
	return result;
}

static int
sync_MergeToPilot_slow (SyncHandler *sh, int dbhandle)
{
	PilotRecord *precord = NULL;
	DesktopRecord *drecord = NULL;
	int darch, dsecret;
	int result = 0;
	
	while (sh->ForEach (sh, &drecord) == 0 && drecord) {
		if (drecord->recID != 0) {
			precord = sync_NewPilotRecord (DLP_BUF_SIZE);
			precord->recID = drecord->recID;
			result = dlp_ReadRecordById (sh->sd, dbhandle, 
						     precord->recID,
						     precord->buffer, 
						     NULL, &precord->len, 
						     &precord->flags, 
						     &precord->catID);
			PErrorCheck(result);
		}

		/* Since this is a slow sync, we must calculate the flags */
		darch = drecord->flags & dlpRecAttrArchived;
		dsecret = drecord->flags & dlpRecAttrSecret;

		drecord->flags = 0;
		if (precord == NULL) {
			drecord->flags = drecord->flags | dlpRecAttrDirty;
		} else {
			int comp;
			
			comp = sh->Compare (sh, precord, drecord);
			if (comp != 0) {
				drecord->flags = drecord->flags | dlpRecAttrDirty;
			}
		}
		if (darch)
			drecord->flags = drecord->flags | dlpRecAttrArchived;
		if (dsecret)
			drecord->flags = drecord->flags | dlpRecAttrSecret;
		
		result = sync_record (sh, dbhandle, drecord, precord);
		ErrorCheck(result);

		if (precord)
			free (precord);
		precord = NULL;
	}

	return result;
}

int
sync_MergeToPilot (SyncHandler *sh)
{
	int dbhandle;
	int slow = 0;
	int result = 0;
	
	result = dlp_OpenDB (sh->sd, 0, dlpOpenReadWrite, sh->name, &dbhandle);
	PErrorCheck(result);

	result = sh->Pre (sh, dbhandle, &slow);
	ErrorCheck(result);

	if (!slow)
		result = sync_MergeToPilot_fast (sh, dbhandle);
	else
		result = sync_MergeToPilot_slow (sh, dbhandle);
	ErrorCheck(result);
	
	result = sh->Post (sh, dbhandle);

	close_db (sh, dbhandle);

	return result;
}

int
sync_Synchronize (SyncHandler *sh)
{
	int dbhandle;
	int slow = 0;
	int result = 0;
	
	result = dlp_OpenDB (sh->sd, 0, dlpOpenReadWrite, sh->name, &dbhandle);
	PErrorCheck(result);

	result = sh->Pre (sh, dbhandle, &slow);
	ErrorCheck(result);
	
	if (!slow) {
		result = sync_MergeFromPilot_fast (sh, dbhandle);
		ErrorCheck(result);
		
		result = sync_MergeToPilot_fast (sh, dbhandle);
		ErrorCheck(result);
	} else {
		result = sync_MergeFromPilot_slow (sh, dbhandle);
		ErrorCheck(result);
		
		result = sync_MergeToPilot_slow (sh, dbhandle);
		ErrorCheck(result);
	}
	
	result = sh->Post (sh, dbhandle);
	
	close_db (sh, dbhandle);

	return result;
}



