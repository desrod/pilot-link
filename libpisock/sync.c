/*
 * sync.c:  Implement generic synchronization algorithm
 *
 * Documentation of the sync cases is at:
 * http://www.palmos.com/dev/tech/docs/conduits/win/CnComp_Conduits.html#926365
 *
 * Copyright (c) 2000-2001, Ximian Inc.
 *
 * Author: JP Rosevear <jpr@helixcode.com> 
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <string.h>

#include "pi-dlp.h"
#include "pi-sync.h"

typedef enum {
	PILOT,
	DESKTOP,
	BOTH
} RecordModifier;

typedef struct _RecordQueueList RecordQueueList;
typedef struct _RecordQueue RecordQueue;

struct _RecordQueueList {
	DesktopRecord *drecord;
	PilotRecord *precord;

	RecordQueueList *next;
};

struct _RecordQueue {
	int count;

	RecordQueueList *rql;
};


#define PilotCheck(func)   if (rec_mod == PILOT || rec_mod == BOTH) if ((result = func) < 0) return result;
#define DesktopCheck(func) if (rec_mod == DESKTOP || rec_mod == BOTH) if ((result = func) < 0) return result;
#define ErrorCheck(func)   if ((result = func) < 0) return result;

/***********************************************************************
 *
 * Function:    sync_NewPilotRecord
 *
 * Summary:     Create a new, empty device record with the given buffer
 *		size
 *
 * Parmeters:   None
 *
 * Returns:     The new device record
 *
 ***********************************************************************/
PilotRecord *sync_NewPilotRecord(int buf_size)
{
	PilotRecord *precord;

	precord = (PilotRecord *) malloc(sizeof(PilotRecord));
	memset(precord, 0, sizeof(PilotRecord));

	precord->buffer = malloc(buf_size);

	return precord;
}

/***********************************************************************
 *
 * Function:    sync_CopyPilotRecord
 *
 * Summary:     Copies the given device record
 *
 * Parmeters:   None
 *
 * Returns:     The device record passed by sync_NewPilotRecord
 *
 ***********************************************************************/
PilotRecord *sync_CopyPilotRecord(const PilotRecord * precord)
{
	PilotRecord *new_record;

	new_record = sync_NewPilotRecord(sizeof(precord->buffer));

	new_record->recID = precord->recID;
	new_record->catID = precord->catID;
	new_record->flags = precord->flags;
	memcpy(new_record->buffer, precord->buffer, precord->len);
	new_record->len = precord->len;

	return new_record;
}


/***********************************************************************
 *
 * Function:    sync_FreePilotRecord
 *
 * Summary:     Free the memory of the given device record
 *
 * Parmeters:   The device record to free
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void sync_FreePilotRecord(PilotRecord * precord)
{
	if (precord->buffer)
		free(precord->buffer);

	free(precord);
}

/***********************************************************************
 *
 * Function:    sync_NewDesktopRecord
 *
 * Summary:     Create a new, empty, desktop record
 *
 * Parmeters:   None
 *
 * Returns:     The new, empty desktop record
 *
 ***********************************************************************/
DesktopRecord *sync_NewDesktopRecord(void)
{
	DesktopRecord *drecord;

	drecord = (DesktopRecord *) malloc(sizeof(DesktopRecord));
	memset(drecord, 0, sizeof(DesktopRecord));

	return drecord;
}

/***********************************************************************
 *
 * Function:    sync_CopyDesktopRecord
 *
 * Summary:     Copies the given desktop record
 *
 * Parmeters:   None
 *
 * Returns:     The new desktop record (DesktopRecord)
 *
 ***********************************************************************/
DesktopRecord *sync_CopyDesktopRecord(const DesktopRecord * drecord)
{
	DesktopRecord *new_record;

	new_record = sync_NewDesktopRecord();

	*new_record = *drecord;

	return new_record;
}

/***********************************************************************
 *
 * Function:    sync_FreeDesktopRecord
 *
 * Summary:     Free the memory of the given desktop record
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void sync_FreeDesktopRecord(DesktopRecord * drecord)
{
	free(drecord);
}

/***********************************************************************
 *
 * Function:    add_record_queue
 *
 * Summary:     Add records to process to the queue, until no more 
 *		records exist
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void
add_record_queue(RecordQueue * rq, PilotRecord * precord,
		 DesktopRecord * drecord)
{
	RecordQueueList *item;

	item = malloc(sizeof(RecordQueueList));

	if (drecord != NULL) {
		item->drecord = drecord;
		item->precord = NULL;
	} else {
		item->drecord = NULL;
		item->precord = sync_CopyPilotRecord(precord);
	}

	if (rq) {
		item->next = rq->rql;
		rq->rql = item;
	} else {
		item->next = NULL;
	}

	rq->count++;
}

/***********************************************************************
 *
 * Function:    free_record_queue_list
 *
 * Summary:     Free the list
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int free_record_queue_list(SyncHandler * sh, RecordQueueList * rql)
{
	RecordQueueList *item;
	int result = 0;

	while (rql != NULL) {
		item = rql;
		rql = rql->next;

		if (item->drecord)
			ErrorCheck(sh->FreeMatch(sh, item->drecord));
		if (item->precord)
			sync_FreePilotRecord(item->precord);

		free(item);
	}

	return 0;
}

/***********************************************************************
 *
 * Function:    delete_both
 *
 * Summary:     Delete records from both Palm and desktop, same time
 *
 * Parmeters:   None
 *
 * Returns:     negative number if error, otherwise 0 to indicate
 *		success
 *
 ***********************************************************************/
static int
delete_both(SyncHandler * sh, int dbhandle, DesktopRecord * drecord,
	    PilotRecord * precord, RecordModifier rec_mod)
{
	int result = 0;

	if (drecord != NULL)
		DesktopCheck(sh->DeleteRecord(sh, drecord));

	if (precord != NULL)
		PilotCheck(dlp_DeleteRecord
			   (sh->sd, dbhandle, 0, precord->recID));

	return result;
}

/***********************************************************************
 *
 * Function:    store_record_on_pilot
 *
 * Summary:     Store the new/modified record on the Palm 
 *
 * Parmeters:   None
 *
 * Returns:     0 if success, otherwise negative number
 *
 ***********************************************************************/
static int
store_record_on_pilot(SyncHandler * sh, int dbhandle,
		      DesktopRecord * drecord, RecordModifier rec_mod)
{
	PilotRecord precord;
	recordid_t id;
	int result = 0;

	memset(&precord, 0, sizeof(PilotRecord));

	result = sh->Prepare(sh, drecord, &precord);
	if (result != 0)
		return result;

	PilotCheck(dlp_WriteRecord(sh->sd, dbhandle, 0,
				   precord.recID, precord.catID,
				   precord.buffer, precord.len, &id));

	DesktopCheck(sh->SetPilotID(sh, drecord, id));

	return result;
}

/***********************************************************************
 *
 * Function:    open_db
 *
 * Summary:     Open a given database by handle for operation
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int open_db(SyncHandler * sh, int *dbhandle)
{
	if (sh->secret)
		return dlp_OpenDB(sh->sd, 0, dlpOpenReadWrite
				  && dlpOpenSecret, sh->name, dbhandle);
	else
		return dlp_OpenDB(sh->sd, 0, dlpOpenReadWrite, sh->name,
				  dbhandle);
}

/***********************************************************************
 *
 * Function:    close_db
 *
 * Summary:     Close a given database by handle
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int close_db(SyncHandler * sh, int dbhandle)
{
	dlp_CleanUpDatabase(sh->sd, dbhandle);

	dlp_ResetSyncFlags(sh->sd, dbhandle);
	dlp_CloseDB(sh->sd, dbhandle);

	return 0;
}

/***********************************************************************
 *
 * Function:    sync_record
 *
 * Summary:     Synchronize a record to the Palm
 *
 * Parmeters:   None
 *
 * Returns:     0 if success, otherwise return a negative number
 *
 ***********************************************************************/
static int
sync_record(SyncHandler * sh, int dbhandle,
	    DesktopRecord * drecord, PilotRecord * precord,
	    RecordQueue * rq, RecordModifier rec_mod)
{
	int parch = 0, pdel = 0, pchange = 0;
	int darch = 0, ddel = 0, dchange = 0;
	int result = 0;

	/* The state is calculated like this because the deleted and dirty
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

	if (precord != NULL && !parch && !pdel && drecord == NULL) {
		DesktopCheck(sh->AddRecord(sh, precord));

	} else if (precord == NULL && drecord != NULL) {
		add_record_queue(rq, NULL, drecord);

	} else if (parch && ddel) {
		DesktopCheck(sh->ReplaceRecord(sh, drecord, precord));
		DesktopCheck(sh->ArchiveRecord(sh, drecord, 1));
		DesktopCheck(sh->SetStatusCleared(sh, drecord));

	} else if (parch && !darch && !dchange && drecord != NULL) {
		DesktopCheck(sh->ArchiveRecord(sh, drecord, 1));

	} else if (parch && drecord == NULL) {
		DesktopCheck(sh->AddRecord(sh, precord));
		ErrorCheck(sh->Match(sh, precord, &drecord));
		if (drecord == NULL)
			return -1;
		DesktopCheck(sh->ArchiveRecord(sh, drecord, 1));
		ErrorCheck(sh->FreeMatch(sh, drecord));

	} else if (parch && pchange && !darch && dchange) {
		int comp;

		comp = sh->Compare(sh, precord, drecord);
		if (comp == 0) {
			DesktopCheck(sh->ArchiveRecord(sh, drecord, 1));
			PilotCheck(dlp_DeleteRecord
				   (sh->sd, dbhandle, 0, precord->recID));
			DesktopCheck(sh->SetStatusCleared(sh, drecord));
		} else {
			PilotCheck(dlp_WriteRecord(sh->sd, dbhandle, 0, 0,
						   precord->catID,
						   precord->buffer,
						   precord->len,
						   &precord->recID));
			DesktopCheck(sh->AddRecord(sh, precord));
			add_record_queue(rq, NULL, drecord);
			DesktopCheck(sh->SetStatusCleared(sh, drecord));
		}

	} else if (parch && !pchange && !darch && dchange) {
		PilotCheck(dlp_DeleteRecord
			   (sh->sd, dbhandle, 0, precord->recID));
		add_record_queue(rq, NULL, drecord);
		DesktopCheck(sh->SetStatusCleared(sh, drecord));

	} else if (pchange && darch && dchange) {
		int comp;

		comp = sh->Compare(sh, precord, drecord);
		if (comp == 0) {
			PilotCheck(dlp_DeleteRecord
				   (sh->sd, dbhandle, 0, precord->recID));
		} else {
			DesktopCheck(sh->ArchiveRecord(sh, drecord, 0));
			DesktopCheck(sh->
				     ReplaceRecord(sh, drecord, precord));
		}
		DesktopCheck(sh->SetStatusCleared(sh, drecord));

	} else if (pchange && darch && !dchange) {
		DesktopCheck(sh->ArchiveRecord(sh, drecord, 0));
		DesktopCheck(sh->ReplaceRecord(sh, drecord, precord));
		DesktopCheck(sh->SetStatusCleared(sh, drecord));

	} else if (pchange && dchange) {
		int comp;

		comp = sh->Compare(sh, precord, drecord);
		if (comp != 0) {
			DesktopCheck(sh->AddRecord(sh, precord));
			drecord->recID = 0;
			add_record_queue(rq, NULL, drecord);
		}
		DesktopCheck(sh->SetStatusCleared(sh, drecord));

	} else if (pchange && ddel) {
		DesktopCheck(sh->ReplaceRecord(sh, drecord, precord));
		DesktopCheck(sh->SetStatusCleared(sh, drecord));

	} else if (pchange && !dchange) {
		DesktopCheck(sh->ReplaceRecord(sh, drecord, precord));
		DesktopCheck(sh->SetStatusCleared(sh, drecord));

	} else if (pdel && dchange) {
		add_record_queue(rq, NULL, drecord);
		DesktopCheck(sh->SetStatusCleared(sh, drecord));

	} else if (pdel && !dchange) {
		DesktopCheck(delete_both
			     (sh, dbhandle, drecord, precord, rec_mod));
		DesktopCheck(sh->SetStatusCleared(sh, drecord));

	} else if (!pchange && darch) {
		PilotCheck(dlp_DeleteRecord
			   (sh->sd, dbhandle, 0, precord->recID));
		DesktopCheck(sh->SetStatusCleared(sh, drecord));

	} else if (!pchange && dchange) {
		PilotCheck(dlp_DeleteRecord
			   (sh->sd, dbhandle, 0, precord->recID));
		add_record_queue(rq, NULL, drecord);
		DesktopCheck(sh->SetStatusCleared(sh, drecord));

	} else if (!pchange && ddel) {
		DesktopCheck(delete_both
			     (sh, dbhandle, drecord, precord, rec_mod));
		DesktopCheck(sh->SetStatusCleared(sh, drecord));

	}

	return result;
}

/***********************************************************************
 *
 * Function:    sync_CopyToPilot
 *
 * Summary:     Copy all desktop records to the Palm
 *
 * Parmeters:   None
 *
 * Returns:     0 on success, non-zero otherwise
 *
 ***********************************************************************/
int sync_CopyToPilot(SyncHandler * sh)
{
	int dbhandle;
	DesktopRecord *drecord = NULL;
	int slow = 0;
	int result = 0;

	result = open_db(sh, &dbhandle);
	if (result < 0)
		goto cleanup;

	result = sh->Pre(sh, dbhandle, &slow);
	if (result < 0)
		goto cleanup;

	result = dlp_DeleteRecord(sh->sd, dbhandle, 1, 0);
	if (result < 0)
		goto cleanup;


	while (sh->ForEach(sh, &drecord) == 0 && drecord) {
		result =
		    store_record_on_pilot(sh, dbhandle, drecord, BOTH);
		if (result < 0)
			goto cleanup;
	}

	result = sh->Post(sh, dbhandle);

      cleanup:
	close_db(sh, dbhandle);

	return result;
}

/***********************************************************************
 *
 * Function:    sync_CopyFromPilot
 *
 * Summary:     Copy all device records from the Palm to the desktop
 *
 * Parmeters:   None
 *
 * Returns:     0 on success, non-zero otherwise
 *
 ***********************************************************************/
int sync_CopyFromPilot(SyncHandler * sh)
{
	int dbhandle;
	int index;
	DesktopRecord *drecord = NULL;
	PilotRecord *precord = sync_NewPilotRecord(DLP_BUF_SIZE);
	int slow = 0;
	int result = 0;

	result = open_db(sh, &dbhandle);
	if (result < 0)
		goto cleanup;

	result = sh->Pre(sh, dbhandle, &slow);
	if (result < 0)
		goto cleanup;

	while (sh->ForEach(sh, &drecord) == 0 && drecord) {
		result = sh->DeleteRecord(sh, drecord);
		if (result < 0)
			goto cleanup;
	}

	index = 0;
	while (dlp_ReadRecordByIndex
	       (sh->sd, dbhandle, index, precord->buffer, &precord->recID,
		&precord->len, &precord->flags, &precord->catID) > 0) {
		result = sh->AddRecord(sh, precord);
		if (result < 0)
			goto cleanup;

		index++;
	}

	result = sh->Post(sh, dbhandle);

      cleanup:
	close_db(sh, dbhandle);

	return result;
}

/***********************************************************************
 *
 * Function:    sync_MergeFromPilot_process
 *
 * Summary:     Try to be intelligent about merging data in a record
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int
sync_MergeFromPilot_process(SyncHandler * sh, int dbhandle,
			    RecordQueue * rq, RecordModifier rec_mod)
{
	RecordQueueList *item;
	int result = 0;

	for (item = rq->rql; item != NULL; item = item->next) {
		if (item->drecord != NULL) {
			store_record_on_pilot(sh, dbhandle, item->drecord,
					      rec_mod);
		} else {
			PilotCheck(dlp_WriteRecord(sh->sd, dbhandle, 0, 0,
						   item->precord->catID,
						   item->precord->buffer,
						   item->precord->len,
						   &item->precord->recID));
		}
	}
	free_record_queue_list(sh, rq->rql);

	return result;
}

/***********************************************************************
 *
 * Function:    sync_MergeFromPilot_fast
 *
 * Summary:     er, fast merge from Palm to desktop
 *
 * Parmeters:   None
 *
 * Returns:     0 if success, nonzero otherwise
 *
 ***********************************************************************/
static int
sync_MergeFromPilot_fast(SyncHandler * sh, int dbhandle,
			 RecordModifier rec_mod)
{
	PilotRecord *precord = sync_NewPilotRecord(DLP_BUF_SIZE);
	DesktopRecord *drecord = NULL;
	RecordQueue rq = { 0, NULL };
	int result = 0;

	while (dlp_ReadNextModifiedRec(sh->sd, dbhandle, precord->buffer,
				       &precord->recID, NULL,
				       &precord->len, &precord->flags,
				       &precord->catID) >= 0) {
		int count = rq.count;

		ErrorCheck(sh->Match(sh, precord, &drecord));
		ErrorCheck(sync_record
			   (sh, dbhandle, drecord, precord, &rq, rec_mod));

		if (drecord && rq.count == count)
			ErrorCheck(sh->FreeMatch(sh, drecord));
	}
	sync_FreePilotRecord(precord);

	result = sync_MergeFromPilot_process(sh, dbhandle, &rq, rec_mod);

	return result;
}

/***********************************************************************
 *
 * Function:    sync_MergeFromPilot_slow
 *
 * Summary:     uh, slow merge from Palm to desktop
 *
 * Parmeters:   None
 *
 * Returns:     0 if success, nonzero otherwise
 *
 ***********************************************************************/
static int
sync_MergeFromPilot_slow(SyncHandler * sh, int dbhandle,
			 RecordModifier rec_mod)
{
	PilotRecord *precord = sync_NewPilotRecord(DLP_BUF_SIZE);
	DesktopRecord *drecord = NULL;
	RecordQueue rq = { 0, NULL };
	int index, parch, psecret;
	int result = 0;

	index = 0;
	while (dlp_ReadRecordByIndex
	       (sh->sd, dbhandle, index, precord->buffer, &precord->recID,
		&precord->len, &precord->flags, &precord->catID) > 0) {
		int count = rq.count;

		ErrorCheck(sh->Match(sh, precord, &drecord));

		/* Since this is a slow sync, we must calculate the flags */
		parch = precord->flags & dlpRecAttrArchived;
		psecret = precord->flags & dlpRecAttrSecret;

		precord->flags = 0;
		if (drecord == NULL) {
			precord->flags = precord->flags | dlpRecAttrDirty;
		} else {
			int comp;

			comp = sh->Compare(sh, precord, drecord);
			if (comp != 0) {
				precord->flags =
				    precord->flags | dlpRecAttrDirty;
			}
		}
		if (parch)
			precord->flags =
			    precord->flags | dlpRecAttrArchived;
		if (psecret)
			precord->flags = precord->flags | dlpRecAttrSecret;

		ErrorCheck(sync_record
			   (sh, dbhandle, drecord, precord, &rq, rec_mod));

		if (drecord && rq.count == count)
			ErrorCheck(sh->FreeMatch(sh, drecord));

		index++;
	}
	sync_FreePilotRecord(precord);

	result = sync_MergeFromPilot_process(sh, dbhandle, &rq, rec_mod);

	return result;
}

/***********************************************************************
 *
 * Function:    sync_MergeFromPilot
 *
 * Summary:     Synchronize the device records to the desktop, but do 
 *		not alter the device records
 *
 * Parmeters:   None
 *
 * Returns:     0 on success, non-zero otherwise
 *
 ***********************************************************************/
int sync_MergeFromPilot(SyncHandler * sh)
{
	int dbhandle;
	int slow = 0;
	int result = 0;

	result = open_db(sh, &dbhandle);
	if (result < 0)
		goto cleanup;

	result = sh->Pre(sh, dbhandle, &slow);
	if (result < 0)
		goto cleanup;

	if (!slow) {
		result = sync_MergeFromPilot_fast(sh, dbhandle, DESKTOP);
		if (result < 0)
			goto cleanup;
	} else {
		result = sync_MergeFromPilot_slow(sh, dbhandle, DESKTOP);
		if (result < 0)
			goto cleanup;
	}

	result = sh->Post(sh, dbhandle);

      cleanup:
	close_db(sh, dbhandle);

	return result;
}

/***********************************************************************
 *
 * Function:    sync_MergeToPilot_fast
 *
 * Summary:     Fast merge from desktop to Palm
 *
 * Parmeters:   None
 *
 * Returns:     0 on success, non-zero otherwise
 *
 ***********************************************************************/
static int
sync_MergeToPilot_fast(SyncHandler * sh, int dbhandle,
		       RecordModifier rec_mod)
{
	PilotRecord *precord = NULL;
	DesktopRecord *drecord = NULL;
	RecordQueue rq = { 0, NULL };
	int result = 0;

	while (sh->ForEachModified(sh, &drecord) == 0 && drecord) {
		if (drecord->recID != 0) {
			precord = sync_NewPilotRecord(DLP_BUF_SIZE);
			precord->recID = drecord->recID;
			PilotCheck(dlp_ReadRecordById(sh->sd, dbhandle,
						      precord->recID,
						      precord->buffer,
						      NULL, &precord->len,
						      &precord->flags,
						      &precord->catID));
		}

		ErrorCheck(sync_record
			   (sh, dbhandle, drecord, precord, &rq, rec_mod));

		if (precord)
			free(precord);
		precord = NULL;
	}

	result = sync_MergeFromPilot_process(sh, dbhandle, &rq, rec_mod);

	return result;
}

/***********************************************************************
 *
 * Function:    sync_MergeToPilot_slow
 *
 * Summary:     Slow merge from desktop to Palm
 *
 * Parmeters:   None
 *
 * Returns:     0 on success, non-zero otherwise
 *
 ***********************************************************************/
static int
sync_MergeToPilot_slow(SyncHandler * sh, int dbhandle,
		       RecordModifier rec_mod)
{
	PilotRecord *precord = NULL;
	DesktopRecord *drecord = NULL;
	RecordQueue rq = { 0, NULL };
	int darch, dsecret;
	int result = 0;

	while (sh->ForEach(sh, &drecord) == 0 && drecord) {
		if (drecord->recID != 0) {
			precord = sync_NewPilotRecord(DLP_BUF_SIZE);
			precord->recID = drecord->recID;
			PilotCheck(dlp_ReadRecordById(sh->sd, dbhandle,
						      precord->recID,
						      precord->buffer,
						      NULL, &precord->len,
						      &precord->flags,
						      &precord->catID));
		}

		/* Since this is a slow sync, we must calculate the flags */
		darch = drecord->flags & dlpRecAttrArchived;
		dsecret = drecord->flags & dlpRecAttrSecret;

		drecord->flags = 0;
		if (precord == NULL) {
			drecord->flags = drecord->flags | dlpRecAttrDirty;
		} else {
			int comp;

			comp = sh->Compare(sh, precord, drecord);
			if (comp != 0) {
				drecord->flags =
				    drecord->flags | dlpRecAttrDirty;
			}
		}
		if (darch)
			drecord->flags =
			    drecord->flags | dlpRecAttrArchived;
		if (dsecret)
			drecord->flags = drecord->flags | dlpRecAttrSecret;

		ErrorCheck(sync_record
			   (sh, dbhandle, drecord, precord, &rq, rec_mod));

		if (precord)
			free(precord);
		precord = NULL;
	}

	result = sync_MergeFromPilot_process(sh, dbhandle, &rq, rec_mod);

	return result;
}

/***********************************************************************
 *
 * Function:    sync_MergeToPilot
 *
 * Summary:     Synchronize the desktop records to the pilot, but do
 *		not alter the desktop records
 *
 * Parmeters:   None
 *
 * Returns:     0 on success, non-zero otherwise
 *
 ***********************************************************************/
int sync_MergeToPilot(SyncHandler * sh)
{
	int dbhandle;
	int slow = 0;
	int result = 0;

	result = open_db(sh, &dbhandle);
	if (result < 0)
		goto cleanup;

	result = sh->Pre(sh, dbhandle, &slow);
	if (result < 0)
		goto cleanup;

	if (!slow) {
		sync_MergeToPilot_fast(sh, dbhandle, PILOT);
		if (result < 0)
			goto cleanup;
	} else {
		sync_MergeToPilot_slow(sh, dbhandle, PILOT);
		if (result < 0)
			goto cleanup;
	}

	result = sh->Post(sh, dbhandle);

      cleanup:
	close_db(sh, dbhandle);

	return result;
}

/***********************************************************************
 *
 * Function:    sync_Synchronize
 *
 * Summary:     Synchronizes the pilot database and the desktop database
 *
 * Parmeters:   None
 *
 * Returns:     0 on success, non-zero otherwise
 *
 ***********************************************************************/
int sync_Synchronize(SyncHandler * sh)
{
	int dbhandle;
	int slow = 0;
	int result = 0;

	result = open_db(sh, &dbhandle);
	if (result < 0)
		goto cleanup;

	result = sh->Pre(sh, dbhandle, &slow);
	if (result != 0)
		goto cleanup;

	if (!slow) {
		result = sync_MergeFromPilot_fast(sh, dbhandle, BOTH);
		if (result < 0)
			goto cleanup;

		result = sync_MergeToPilot_fast(sh, dbhandle, BOTH);
		if (result < 0)
			goto cleanup;
	} else {
		result = sync_MergeFromPilot_slow(sh, dbhandle, BOTH);
		if (result < 0)
			goto cleanup;

		result = sync_MergeToPilot_slow(sh, dbhandle, BOTH);
		if (result < 0)
			goto cleanup;
	}

	result = sh->Post(sh, dbhandle);

      cleanup:
	close_db(sh, dbhandle);

	return result;
}
