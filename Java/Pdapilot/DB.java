/* DB.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

package Pdapilot;

import java.io.*;

/** A representation of an open database.
 */

public class DB {
		Dlp socket;
		int handle;
		String dbname;
		int dbmode;
		int dbcard;
		
		public Database dbClass;
		
		DB(Dlp socket, int handle, String dbname, int dbmode, int dbcard) {
			this.socket = socket;
			this.handle = handle;
			this.dbname = dbname;
			this.dbmode = dbmode;
			this.dbcard = dbcard;
			
			System.out.println("Opening database '"+dbname+"'");
			dbClass = (Database)Database.dbClasses.get(dbname);
			if (dbClass == null) {
				dbClass = Database.defaultDbClass;
				System.out.println("Using default dbclass");
			}
		}

		DB(Dlp socket, int handle, String dbname, int dbmode, int dbcard, Database dbClass) {
			this.socket = socket;
			this.handle = handle;
			this.dbname = dbname;
			this.dbmode = dbmode;
			this.dbcard = dbcard;
			this.dbClass = dbClass;
			
			System.out.println("Opening database '"+dbname+"'");
		}
		
		public void close() throws DlpException { 
			/* this method should be idempotent */
		    if (handle != 0)
				socket.dlp_CloseDB(handle);
			handle = 0;
		}
		
		protected void finalize() throws DlpException {
			this.close();
		}
		
		public AppBlock getAppBlock() throws DlpException {
			return socket.dlp_ReadAppBlock(handle, dbClass);
		}
		
		public AppBlock newAppBlock() {
			return dbClass.newAppBlock();
		}

		public SortBlock getSortBlock() throws DlpException {
			return socket.dlp_ReadSortBlock(handle, dbClass);
		}

		public SortBlock newSortBlock() {
			return dbClass.newSortBlock();
		}

		public Record getRecord(int index) throws DlpException {
			return socket.dlp_ReadRecordByIndex(handle, index, dbClass);
		}

		public Record newRecord() {
			return dbClass.newRecord();
		}

		public Record newRecord(RecordID id) {
			return dbClass.newRecord(id);
		}

		public Resource newResource() {
			return dbClass.newResource();
		}

		public Resource newResource(Char4 type, int id) {
			return dbClass.newResource(type, id);
		}
		
		public Record getRecord(RecordID id) throws DlpException {
			return socket.dlp_ReadRecordByID(handle, id, dbClass);
		}

		public Record getNextRecord(int category) throws DlpException {
			return socket.dlp_ReadNextRecInCategory(handle, category, dbClass);
		}

		public Record getNextModRecord() throws DlpException {
			return socket.dlp_ReadNextModifiedRec(handle, dbClass);
		}

		public Record getNextModRecord(int category) throws DlpException {
			return socket.dlp_ReadNextModifiedRecInCategory(handle, category, dbClass);
		}
		
		public Resource getResource(int index) throws DlpException {
			return socket.dlp_ReadResourceByIndex(handle, index, dbClass);
		}

		public Resource getResource(Char4 type, int id) throws DlpException {
			return socket.dlp_ReadResourceByType(handle, type, id, dbClass);
		}
		
		public long setRecord(Record record) throws DlpException {
			return socket.dlp_WriteRecord(handle, record);
		}

		public void setResource(Resource resource) throws DlpException {
			socket.dlp_WriteResource(handle, resource);
		}
		
		public void setAppBlock(AppBlock appblock) throws DlpException {
			socket.dlp_WriteAppBlock(handle, appblock);
		}

		public void setSortBlock(SortBlock sortblock) throws DlpException {
			socket.dlp_WriteSortBlock(handle, sortblock);
		}

		public void deleteRecord(RecordID id) throws DlpException {
			socket.dlp_DeleteRecord(handle, false, id);
		}

		public void deleteRecords() throws DlpException {
			socket.dlp_DeleteRecord(handle, true, null);
		}

		public void deleteResource(Char4 type, int id) throws DlpException {
			socket.dlp_DeleteResource(handle, false, type, id);
		}

		public void deleteResources() throws DlpException {
			socket.dlp_DeleteResource(handle, true, null, 0);
		}
		
		public void deleteCategory(int category) throws DlpException {
			socket.dlp_DeleteCategory(handle, category);
		}
		
		public void moveCategory(int from, int to) throws DlpException {
			socket.dlp_MoveCategory(handle, from, to);
		}

		public void purge() throws DlpException {
			socket.dlp_CleanUpDatabase(handle);
		}

		public void resetFlags() throws DlpException {
			socket.dlp_ResetSyncFlags(handle);
		}

		public void resetNext() throws DlpException {
			socket.dlp_ResetDBIndex(handle);
		}
		
		public int getRecords() throws DlpException {
			return socket.dlp_ReadOpenDBInfo(handle);
		}

		public Pref getPref(int id)
			throws DlpException, IOException
		{	return getPref(id, true); }
		
		public Pref getPref(int id, boolean backup)
			throws DlpException, NoCreatorException, IOException
		{
			if (socket.version() < 0x101)
				socket.dlp_CloseDB(handle);
			if (dbClass.creator() == null)
				throw new NoCreatorException();
			Pref result = socket.dlp_ReadAppPreference(dbClass.creator(), id, backup, dbClass);
			if (socket.version() < 0x101)
				handle = socket.dlp_OpenDB(dbcard, dbmode, dbname);
			return result;
		}
		
		public Pref newPref(int id)
		{
			return dbClass.newPref(null, dbClass.creator(), id, 1, true);
		}
		
		public Pref newPref(int id, int version, boolean backup)
		{
			return dbClass.newPref(null, dbClass.creator(), id, version, backup);
		}

		public void setPref(Pref pref)
			throws DlpException, IOException
		{
			if (socket.version() < 0x101)
				socket.dlp_CloseDB(handle);
			socket.dlp_WriteAppPreference(pref);
			if (socket.version() < 0x101)
				handle = socket.dlp_OpenDB(dbcard, dbmode, dbname);
		}

		public RecordID[] getRecordIDs()throws DlpException
			{ return getRecordIDs(false);	}
		public RecordID[] getRecordIDs(boolean sort) 	throws DlpException
		{	return getRecordIDs(sort, 0); 	}
		public RecordID[] getRecordIDs(boolean sort, int start)		throws DlpException
		{	return getRecordIDs(sort, start, 0xffff);	}
		public RecordID[] getRecordIDs(boolean sort, int start, int max)
			throws DlpException
		{
			return socket.dlp_ReadRecordIDList(handle, sort, start, max);
		}

}
