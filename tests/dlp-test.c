/* 
 * dlp-test.c:  DLP Regression Test
 *
 * (c) 2002, JP Rosevear.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *
 * This regression test calls every DLP function except the following:
 *    dlp_CallApplication
 *    dlp_ResetSystem
 *    dlp_ProcessRPC
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pi-debug.h"
#include "pi-source.h"
#include "pi-dlp.h"

/* Prototypes */
int pilot_connect(char *port);

/* For various protocol versions, set to 0 to not test those versions */
#define DLP_1_1 1
#define DLP_1_2 1

/* Logging defines */
#define CHECK_RESULT(func) \
  if (result < 0) { \
	LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST " #func " failed (%d)\n", result)); \
	goto error; \
  }

/* Utility defines */
#define pi_mktag(c1,c2,c3,c4) (((c1)<<24)|((c2)<<16)|((c3)<<8)|(c4))

#define CREATOR pi_mktag('T', 'e', 's', 't')
#define DATA pi_mktag('D', 'A', 'T', 'A')
#define INFO pi_mktag('I', 'N', 'F', 'O')

int
main (int argc, char **argv)
{
	int sd;
	int result;
	struct SysInfo s;
	struct PilotUser u1, u2;
	struct NetSyncInfo n1, n2;
	struct CardInfo c;
	struct DBInfo dbi;
	struct DBSizeInfo dbsi;
	unsigned long romVersion;
	time_t t1, t2;	
	int handle;
	unsigned char pref1[256], pref2[256];
	unsigned char ablock1[256];
	unsigned char sblock1[256];
	unsigned char ires1[256];
	unsigned char dres1[256];
	unsigned char record1[256], record2[256], record3[256];
	recordid_t rid1, rid2, rid3, rlist[4];
	int index, id_, count;
	unsigned long type;
	int cardno;
	int i;
	pi_buffer_t *record4,
		*dres2,
		*ires2,
		*appblock;

	record4 = pi_buffer_new (sizeof(record1));
	ires2 = pi_buffer_new (sizeof (ires1));
	dres2 = pi_buffer_new (sizeof (dres1));
	appblock = pi_buffer_new(256);
	
	sd = pilot_connect (argv[1]);

	t1 = time (NULL);
	LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "DLPTEST Starting at %s", ctime (&t1)));

	/*********************************************************************
	 *
	 * Test: Open Conduit
	 *
	 * Direct Testing Functions:
	 *   dlp_OpenConduit
	 *
	 * Indirect Testing Functions:
	 *   None
	 *
	 *********************************************************************/
	result = dlp_OpenConduit (sd);
	CHECK_RESULT(dlp_OpenConduit);

	/*********************************************************************
	 *
	 * Test: System Information
	 *
	 * Direct Testing Functions:
	 *   dlp_ReadSysInfo
	 *
	 * Indirect Testing Functions:
	 *   None
	 *
	 *********************************************************************/
	result = dlp_ReadSysInfo (sd, &s);
	CHECK_RESULT(dlp_ReadSysInfo);
	
	/*********************************************************************
	 *
	 * Test: User Info
	 *
	 * Direct Testing Functions:
	 *   dlp_WriteUserInfo
	 *   dlp_ReadUserInfo
	 *
	 * Indirect Testing Functions:
	 *   None
	 *
	 *********************************************************************/
	memset (&u1, '\0', sizeof (struct PilotUser));
	memset (&u2, '\0', sizeof (struct PilotUser));
	u1.passwordLength = 0;
	strcpy (u1.username, "Test User");
	strcpy (u1.password, "");
	u1.userID = 500;
	u1.viewerID = 5000;
	u1.lastSyncPC = 111111;
	u1.successfulSyncDate = time(NULL);
	u1.lastSyncDate = time(NULL) + 100;

	result = dlp_WriteUserInfo (sd, &u1);
	CHECK_RESULT(dlp_WriteUserInfo);
	result = dlp_ReadUserInfo (sd, &u2);
	CHECK_RESULT(dlp_ReadUserInfo);
	if (memcmp(&u1, &u2, sizeof(struct PilotUser) != 0)) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST User info mismatch\n"));
		goto error;
	}

	/*********************************************************************
	 *
	 * Test: Feature
	 *
	 * Direct Testing Functions:
	 *   dlp_ReadFeature
	 *
	 * Indirect Testing Functions:
	 *   None
	 *
	 *********************************************************************/
	romVersion = 0;

	result = dlp_ReadFeature(sd, makelong("psys"), 1, &romVersion);
	CHECK_RESULT(dlp_ReadFeature);
	if (romVersion != s.romVersion) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST Rom Version mismatch\n"));
		goto error;
	}
	
	/*********************************************************************
	 *
	 * Test: Net Sync Info
	 *
	 * Direct Testing Functions:
	 *   dlp_WriteNetSyncInfo
	 *   dlp_ReadNetSyncInfo
	 *
	 * Indirect Testing Functions:
	 *   None
	 *
	 *********************************************************************/
#if DLP_1_1
	memset (&n1, '\0', sizeof (struct NetSyncInfo));
	memset (&n2, '\0', sizeof (struct NetSyncInfo));
	n1.lanSync = 0;
	strcpy (n1.hostName, "localhost");
	strcpy (n1.hostAddress, "192.168.1.1");
	strcpy (n1.hostSubnetMask, "255.255.255.0");

	result = dlp_WriteNetSyncInfo (sd, &n1);
	CHECK_RESULT(dlp_WriteNetSyncInfo);
	result = dlp_ReadNetSyncInfo (sd, &n2);
	CHECK_RESULT(dlp_ReadNetSyncInfo);
	if (memcmp(&n1, &n2, sizeof(struct NetSyncInfo) != 0)) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST Net sync info mismatch\n"));
		goto error;
	}
#endif

	/*********************************************************************
	 *
	 * Test: Time
	 *
	 * Direct Testing Functions:
	 *   dlp_SetSysDateTime
	 *   dlp_GetSysDateTime
	 *
	 * Indirect Testing Functions:
	 *   None
	 *
	 *********************************************************************/
	t1 = time(NULL);

	dlp_SetSysDateTime (sd, t1);
	CHECK_RESULT(dlp_SetSysDateTime);
	dlp_GetSysDateTime (sd, &t2);
	CHECK_RESULT(dlp_GetSysDateTime);
	if (t2 > t1 + 1) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST System Time Mismatch\n"));
		goto error;
	}

	/*********************************************************************
	 *
	 * Test: Storage Information
	 *
	 * Direct Testing Functions:
	 *   dlp_ReadStorageInfo
	 *
	 * Indirect Testing Functions:
	 *   None
	 *
	 *********************************************************************/
	c.more = 1;
	for (i = 0; c.more != 0; i++) {
		result = dlp_ReadStorageInfo (sd, i, &c);
		CHECK_RESULT(dlp_ReadStorageInfo);
	}

	/*********************************************************************
	 *
	 * Test: Database List
	 *
	 * Direct Testing Functions:
	 *   dlp_ReadDBList
	 *
	 * Indirect Testing Functions:
	 *   None
	 *
	 *********************************************************************/
	dbi.more = 1;
	for (i = 0; dbi.more != 0; i++) {
		result = dlp_ReadDBList (sd, 0, dlpDBListRAM | dlpDBListROM, i, record4);
		CHECK_RESULT(dlp_ReadDBList);
		memcpy(&dbi, record4->data, sizeof(struct DBInfo));
	}

	/*********************************************************************
	 *
	 * Test: Existing Database
	 *
	 * Direct Testing Functions:
	 *   dlp_OpenDB
	 *   dlp_CloseDB
	 *
	 * Indirect Testing Functions:
	 *   None
	 *
	 *********************************************************************/
	result = dlp_OpenDB (sd, 0, dlpOpenReadWrite, "ToDoDB", &handle);
	CHECK_RESULT(dlp_OpenDB);
	result = dlp_CloseDB (sd, handle);
	CHECK_RESULT(dlp_CloseDB);

	/*********************************************************************
	 *
	 * Test: New Database
	 *
	 * Direct Testing Functions:
	 *   dlp_CreateDB
	 *   dlp_CloseDB
	 *   dlp_DeleteDB
	 *
	 * Indirect Testing Functions:
	 *   None
	 *
	 *********************************************************************/
	result = dlp_CreateDB (sd, CREATOR, DATA, 0, dlpDBFlagResource, 1, "TestResource", &handle);
	CHECK_RESULT(dlp_CreateDB);
	result = dlp_CloseDB (sd, handle);
	CHECK_RESULT(dlp_CloseDB);
	result = dlp_DeleteDB (sd, 0, "TestResource");
	CHECK_RESULT(dlp_DeleteDB);

	/*********************************************************************
	 *
	 * Test: Database Info and Searching
	 *
	 * Direct Testing Functions:
	 *   dlp_SetDBInfo
	 *   dlp_FindDBByName
	 *   dlp_FindDBByOpenHandle
	 *   dlp_FindDBByTypeCreator
	 *
	 * Indirect Testing Functions:
	 *   dlp_CreateDB
	 *   dlp_OpenDB
	 *   dlp_ReadDBList
	 *   dlp_CloseDB
	 *   dlp_DeleteDB
	 *
	 *********************************************************************/
#if DLP_1_2
	result = dlp_CreateDB (sd, CREATOR, DATA, 0, 0, 1, "TestRecord", &handle);
	CHECK_RESULT(dlp_CreateDB);
	result = dlp_SetDBInfo (sd, handle, dlpDBFlagBackup, dlpDBFlagCopyPrevention, 0, 0, 0, 0, 0, 0);
	CHECK_RESULT(dlp_SetDBInfo);	
	result = dlp_CloseDB (sd, handle);
	CHECK_RESULT(dlp_CloseDB);

	result = dlp_OpenDB (sd, 0, dlpOpenReadWrite, "TestRecord", &handle);
	CHECK_RESULT(dlp_OpenDB);
	
	result = dlp_FindDBByOpenHandle (sd, handle, &cardno, NULL, &dbi, &dbsi);
	CHECK_RESULT(dlp_FindDBByOpenHandle);
	if (strcmp (dbi.name, "TestRecord") || !(dbi.flags & dlpDBFlagBackup)) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST Database info mismatch with openhandle\n"));
		goto error;
	}

	result = dlp_CloseDB (sd, handle);
	CHECK_RESULT(dlp_CloseDB);
	
	result = dlp_FindDBByName (sd, 0, "TestRecord", NULL, NULL, &dbi, &dbsi);
	CHECK_RESULT(dlp_FindDBByName);
	if (strcmp (dbi.name, "TestRecord") || !(dbi.flags & dlpDBFlagBackup)) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST Database info mismatch with name\n"));
		goto error;
	}

	result = dlp_FindDBByTypeCreator (sd, DATA, CREATOR, 1, 0, &cardno, NULL, NULL, &dbi, &dbsi);
	CHECK_RESULT(dlp_FindDBByName);
	if (strcmp (dbi.name, "TestRecord") || !(dbi.flags & dlpDBFlagBackup)) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST Database info mismatch with type/creator\n"));
		goto error;
	}

	result = dlp_DeleteDB (sd, 0, "TestRecord");
	CHECK_RESULT(dlp_DeleteDB);
#endif

	/*********************************************************************
	 *
	 * Test: App Preference
	 *
	 * Direct Testing Functions:
	 *   dlp_WriteAppPreference
	 *   dlp_ReadAppPreference
	 *
	 * Indirect Testing Functions:
	 *   None
	 *
	 *********************************************************************/
	memset (pref1, '\0', sizeof (pref1));
	memset (pref2, '\0', sizeof (pref2));
	pref1[9] = 'T';
	pref2[10] = 'T';
	
	result = dlp_WriteAppPreference (sd, CREATOR, 0, 1, 1, pref1, sizeof(pref1));
	CHECK_RESULT(dlp_WriteAppPrefence);
	result = dlp_ReadAppPreference (sd, CREATOR, 0, 1, sizeof(pref2), pref2, NULL, NULL);
	CHECK_RESULT(dlp_ReadAppPreference);
	if (memcmp(&pref1, &pref2, sizeof(pref1) != 0)) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST Preference mismatch\n"));
		goto error;
	}

	/*********************************************************************
	 *
	 * Test: Record
	 *
	 * Direct Testing Functions:
	 *   dlp_WriteAppBlock
	 *   dlp_ReadAppBlock
	 *   dlp_WriteSortBlock
	 *   dlp_ReadSortBlock
	 *   dlp_WriteRecord
	 *   dlp_ReadOpenDBInfo
	 *   dlp_ReadRecordById
	 *   dlp_ReadRecordByIndex
	 *   dlp_ReadNextModifiedRec
	 *   dlp_ReadNextRecInCategory
	 *   dlp_ReadNextModifiedRecInCategory
	 *   dlp_MoveCategory
	 *   dlp_DeleteRecord
	 *   dlp_DeleteCategory
	 *   dlp_ResetDBIndex
	 *
	 * Indirect Testing Functions:
	 *   dlp_CreateDB
	 *   dlp_CloseDB
	 *   dlp_DeleteDB
	 *
	 *********************************************************************/
	memset (ablock1, '\0', sizeof (ablock1));
	memset (sblock1, '\0', sizeof (sblock1));
	memset (record1, '\0', sizeof (record1));
	memset (record2, '\0', sizeof (record2));
	memset (record3, '\0', sizeof (record3));
	ablock1[3] = 'T';
	sblock1[17] = 'T';
	record1[32] = 'T';
	record2[33] = 'T';
	record3[34] = 'T';
	
	result = dlp_CreateDB (sd, CREATOR, DATA, 0, 0, 1, "TestRecord", &handle);
	CHECK_RESULT(dlp_CreateDB);

	/* Write and read back an app block */
	result = dlp_WriteAppBlock (sd, handle, ablock1, sizeof(ablock1));
	CHECK_RESULT(dlp_WriteAppBlock);
	result = dlp_ReadAppBlock (sd, handle, 0, sizeof(ablock1), appblock);
	CHECK_RESULT(dlp_ReadAppBlock);
	if (result != sizeof(ablock1) || memcmp(ablock1, appblock->data, sizeof(ablock1) != 0)) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST App block mismatch\n"));
		goto error;
	}

	/* Write and read back a sort block */
	result = dlp_WriteSortBlock (sd, handle, sblock1, sizeof(sblock1));
	CHECK_RESULT(dlp_WriteSortBlock);
	result = dlp_ReadSortBlock (sd, handle, 0, sizeof(sblock1), appblock);
	CHECK_RESULT(dlp_ReadSortBlock);
	if (result != sizeof(sblock1) || memcmp(sblock1, appblock->data, sizeof(sblock1) != 0)) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST App block mismatch\n"));
		goto error;
	}
	
	/* Write some records out */
	result = dlp_WriteRecord (sd, handle, 0, 0, 1, record1, sizeof(record1), &rid1);
	CHECK_RESULT(dlp_WriteRecord);
	result = dlp_WriteRecord (sd, handle, dlpRecAttrDirty, 0, 2, record2, sizeof(record2), &rid2);
	CHECK_RESULT(dlp_WriteRecord);
	result = dlp_WriteRecord (sd, handle, 0, 0, 3, record3, sizeof(record3), &rid3);
	CHECK_RESULT(dlp_WriteRecord);

	/* Get the db info */
	result = dlp_ReadOpenDBInfo (sd, handle, &count);
	CHECK_RESULT(dlp_ReadOpenDBInfo);
	if (count != 3) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST Read wrong open database info\n"));
		goto error;
	}

	/* Get the id list */
	result = dlp_ReadRecordIDList (sd, handle, 0, 0, 4, rlist, &count);
	CHECK_RESULT(dlp_ReadRecordIDList);
	if (count != 3) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST Read wrong record id list length\n"));
		goto error;
	}
	for (i = 0; i < 3; i++) {
		if (rlist[i] != rid1 && rlist[i] != rid2 && rlist[i] != rid3) {
			LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST Read wrong record id\n"));
			goto error;			
		}
	}
	
	/* Try reading the records in various ways */
	result = dlp_ReadRecordById (sd, handle, rid1, record4, &index, NULL, NULL);
	CHECK_RESULT(dlp_ReadRecordById);
	if (memcmp(record1, record4->data, sizeof(record1) != 0)) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST Record by Id mismatch\n"));
		goto error;
	}
	result = dlp_ReadRecordByIndex (sd, handle, index, record4, NULL, NULL, NULL);
	CHECK_RESULT(dlp_ReadRecordByIndex);
	if (memcmp(record1, record4->data, sizeof(record1) != 0)) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST Record by index mismatch\n"));
		goto error;
	}
	result = dlp_ReadNextModifiedRec (sd, handle, record4, NULL, NULL, NULL, NULL);
	CHECK_RESULT(dlp_ReadNextModifiedRec);
	if (memcmp(record2, record4->data, sizeof(record2) != 0)) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST Next modified record mismatch\n"));
		goto error;
	}
	
	/* Reset because of the above next modified record call */
	result = dlp_ResetDBIndex (sd, handle);
	CHECK_RESULT(dlp_ResetDBIndex)

	/* This is a DLP 1.1 call, but pilot-link has a 1.0 implementation */
	result = dlp_ReadNextRecInCategory (sd, handle, 3, record4, NULL, NULL, NULL);
	CHECK_RESULT(dlp_ReadNextRecInCategory)
	if (memcmp(record3, record4->data, sizeof(record3) != 0)) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST dlp_ReadNextRecInCategory mismatch\n"));
		goto error;
	}

	/* Reset because of the above next record in category call */
	result = dlp_ResetDBIndex (sd, handle);
	CHECK_RESULT(dlp_ResetDBIndex)

	/* This is a DLP 1.1 call, but pilot-link has a 1.0 implementation */
	result = dlp_ReadNextModifiedRecInCategory (sd, handle, 2, record4, NULL, NULL, NULL);
	CHECK_RESULT(dlp_ReadNextModifiedRecInCategory)
	if (memcmp(record2, record4->data, sizeof(record2) != 0)) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST dlp_ReadNextModifiedRecInCategory mismatch\n"));
		goto error;
	}

	/* Reset because of the above next modified record in category call */
	result = dlp_ResetDBIndex (sd, handle);
	CHECK_RESULT(dlp_ResetDBIndex)

	/* Move a category and try to read the record back in */
	result = dlp_MoveCategory (sd, handle, 1, 4);
	CHECK_RESULT(dlp_MoveCategory)
	result = dlp_ReadNextRecInCategory (sd, handle, 4, record4, NULL, NULL, NULL);
	CHECK_RESULT(dlp_ReadNextRecInCategory)
	if (memcmp(record1, record4->data, sizeof(record1) != 0)) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST dlp_ReadNextRecInCategory mismatch\n"));
		goto error;
	}
	
	/* Delete records in various ways */
	result = dlp_DeleteRecord (sd, handle, 0, rid1);
	CHECK_RESULT(dlp_DeleteRecord <Single>);
	result = dlp_ReadRecordById (sd, handle, rid1, record4, NULL, NULL, NULL);
	if (result >= 0) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST Deleted record could still be read\n"));
		goto error;
	}
	result = dlp_DeleteCategory (sd, handle, 3);
	CHECK_RESULT(dlp_DeleteCategory);
	result = dlp_ReadRecordById (sd, handle, rid3, record4, NULL, NULL, NULL);
	if (result >= 0) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST Deleted category could still be read\n"));
		goto error;
	}
	result = dlp_DeleteRecord (sd, handle, 1, 0);
	CHECK_RESULT(dlp_DeleteRecord <All>);
	result = dlp_ReadRecordById (sd, handle, rid2, record4, NULL, NULL, NULL);
	if (result >= 0) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST Deleted all record could still be read\n"));
		goto error;
	}

	result = dlp_CloseDB (sd, handle);
	CHECK_RESULT(dlp_CloseDB);
	result = dlp_DeleteDB (sd, 0, "TestRecord");
	CHECK_RESULT(dlp_DeleteDB);

	/*********************************************************************
	 *
	 * Test: Resource
	 *
	 * Direct Testing Functions:
	 *   dlp_WriteResource
	 *   dlp_ReadResourceByType
	 *   dlp_ReadResourceByIndex
	 *   dlp_DeleteResource
	 *
	 * Indirect Testing Functions:
	 *   dlp_CreateDB
	 *   dlp_CloseDB
	 *   dlp_DeleteDB
	 *
	 *********************************************************************/
	memset (ires1, '\0', sizeof (ires1));
	memset (dres1, '\0', sizeof (dres1));
	ires1[3] = 'T';
	dres1[4] = 'T';

	result = dlp_CreateDB (sd, CREATOR, DATA, 0, dlpDBFlagResource, 1, "TestResource", &handle);
	CHECK_RESULT(dlp_CreateDB);

	/* Write out some resources */
	result = dlp_WriteResource (sd, handle, INFO, 1, ires1, sizeof(ires1));
	CHECK_RESULT(dlp_WriteResource);
	result = dlp_WriteResource (sd, handle, DATA, 0, dres1, sizeof(dres1));
	CHECK_RESULT(dlp_WriteResource);

	/* Read in the resources by various methods */
	result = dlp_ReadResourceByType (sd, handle, INFO, 1, ires2, &index);
	CHECK_RESULT(dlp_ReadResourceByType)
	if (memcmp(ires1, ires2->data, sizeof(ires1) != 0)) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST Resource by type mismatch\n"));
		goto error;
	}
	result = dlp_ReadResourceByIndex (sd, handle, index, ires2, &type, &id_);
	CHECK_RESULT(dlp_ReadResourceByIndex)
	if (memcmp(ires1, ires2->data, sizeof(ires1) != 0)) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST Resource by index mismatch\n"));
		goto error;
	}
	if (type != INFO) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST Resource by index return type mismatch\n"));
		goto error;
	}
	if (id_ != 1) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST Resource by index return id mismatch\n"));
		goto error;
	}

	/* Delete resources by the various methods */
	result = dlp_DeleteResource (sd, handle, 0, INFO, 1);
	CHECK_RESULT(dlp_DeleteResource <Single>)
	result = dlp_ReadResourceByType (sd, handle, INFO, 1, ires2, &index);
	if (result >= 0) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST Deleted resource could still be read\n"));
		goto error;
	}
	result = dlp_DeleteResource (sd, handle, 1, INFO, 1);
	CHECK_RESULT(dlp_DeleteResource <All>)
	result = dlp_ReadResourceByType (sd, handle, DATA, 0, dres2, &index);
	if (result >= 0) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST Deleted all resource could still be read\n"));
		goto error;
	}

	result = dlp_CloseDB (sd, handle);
	CHECK_RESULT(dlp_CloseDB)
	result = dlp_DeleteDB (sd, 0, "TestResource");
	CHECK_RESULT(dlp_DeleteDB)

	/*********************************************************************
	 *
	 * Test: Database Cleanup
	 *
	 * Direct Testing Functions:
	 *   dlp_CleanUpDatabase
	 *   dlp_ResetSyncFlags
	 *
	 * Indirect Testing Functions:
	 *   dlp_CreateDB
	 *   dlp_WriteRecord
	 *   dlp_CloseDB
	 *   dlp_DeleteDB
	 *
	 *********************************************************************/
	result = dlp_CreateDB (sd, CREATOR, DATA, 0, 0, 1, "TestRecord", &handle);
	CHECK_RESULT(dlp_CreateDB);

	/* Create dummy records */
	result = dlp_WriteRecord (sd, handle, dlpRecAttrDeleted, 0, 0, record1, sizeof(record1), &rid1);
	CHECK_RESULT(dlp_WriteRecord);
	result = dlp_WriteRecord (sd, handle, dlpRecAttrDirty, 0, 0, record2, sizeof(record2), &rid2);
	CHECK_RESULT(dlp_WriteRecord);

	/* Call the test functions */
	result = dlp_CleanUpDatabase (sd, handle);
	CHECK_RESULT(dlp_CleanUpDatabase);
	result = dlp_ResetSyncFlags (sd, handle);
	CHECK_RESULT(dlp_ResetSyncFlags);

	result = dlp_CloseDB (sd, handle);
	CHECK_RESULT(dlp_CloseDB);

	/* Confirm the test functions worked */
	result = dlp_OpenDB (sd, 0, dlpOpenReadWrite, "TestRecord", &handle);
	CHECK_RESULT(dlp_OpenDB);

	result = dlp_ReadRecordById (sd, handle, rid1, record4, NULL, NULL, NULL);
	if (result >= 0) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST Cleaned up record could still be read\n"));
		goto error;
	}
	result = dlp_ReadNextModifiedRec (sd, handle, record4, NULL, NULL, NULL, NULL);
	if (result >= 0) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "DLPTEST Modified recorded could still be read\n"));
		goto error;
	}

	result = dlp_CloseDB (sd, handle);
	CHECK_RESULT(dlp_CloseDB);
	result = dlp_DeleteDB (sd, 0, "TestRecord");
	CHECK_RESULT(dlp_DeleteDB);

	/*********************************************************************
	 *
	 * Test: Sync Log
	 *
	 * Direct Testing Functions:
	 *   dlp_AddSyncLogEntry
	 *
	 * Indirect Testing Functions:
	 *   None
	 *
	 *********************************************************************/
	result = dlp_AddSyncLogEntry (sd, "dlp-test added sync log entry");
	CHECK_RESULT(dlp_AddSyncLogEntry);

	/*********************************************************************
	 *
	 * Test: End Sync
	 *
	 * Direct Testing Functions:
	 *   dlp_EndOfSync
	 *
	 * Indirect Testing Functions:
	 *   None
	 *
	 *********************************************************************/
	result = dlp_EndOfSync (sd, dlpEndCodeNormal);
	CHECK_RESULT(dlp_EndOfSync);

	t1 = time (NULL);
	LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "DLPTEST Ending at %s", ctime (&t1)));

 error:
	pi_close (sd);
	pi_buffer_free (record4);
	pi_buffer_free (ires2);
	pi_buffer_free (dres2);
	pi_buffer_free(appblock);
	return 0;
}

