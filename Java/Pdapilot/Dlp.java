
package Pdapilot;

import java.io.*;

public class Dlp extends Socket {
		Dlp(int sock) { super(sock); }
		synchronized public int dlp_OpenDB(int card, int mode, String name) throws DlpException
			{ return calls.dlp_OpenDB(socket, card, mode, name); }
		synchronized public int dlp_CreateDB(Char4 creator, Char4 type, int card, int flags, int version, String name) throws DlpException
			{ return calls.dlp_CreateDB(socket, creator, type, card, flags, version, name); }
		synchronized public int dlp_CloseDB(int handle) throws DlpException
			{ return calls.dlp_CloseDB(socket, handle); }
		synchronized public int dlp_AddSyncLogEntry(String entry) throws DlpException
			{ return calls.dlp_AddSyncLogEntry(socket, entry); }
		synchronized public Record dlp_ReadRecordByIndex(int handle, int index, Database dbClass) throws DlpException
			{ return calls.dlp_ReadRecordByIndex(socket, handle, index, dbClass); }
		synchronized public Record dlp_ReadRecordByID(int handle, RecordID id, Database dbClass) throws DlpException
			{ return calls.dlp_ReadRecordByID(socket, handle, id, dbClass); }
		synchronized public int dlp_WriteRecord(int handle, Record record) throws DlpException
			{ return calls.dlp_WriteRecord(socket, handle, record); }
		synchronized public int dlp_WriteResource(int handle, Resource resource) throws DlpException
			{ return calls.dlp_WriteResource(socket, handle, resource); }
		synchronized public Resource dlp_ReadResourceByType(int handle, Char4 type, int id, Database dbClass) throws DlpException
			{ return calls.dlp_ReadResourceByType(socket, handle, type, id, dbClass); }
		synchronized public Resource dlp_ReadResourceByIndex(int handle, int index, Database dbClass) throws DlpException
			{ return calls.dlp_ReadResourceByIndex(socket, handle, index, dbClass); }
		synchronized public CardInfo dlp_ReadStorageInfo(int card) throws DlpException
			{ return calls.dlp_ReadStorageInfo(socket, card); }
		synchronized public java.util.Date dlp_GetSysDateTime() throws DlpException
			{ return calls.dlp_GetSysDateTime(socket); }
		synchronized public UserInfo dlp_ReadUserInfo() throws DlpException
			{ return calls.dlp_ReadUserInfo(socket); }
		synchronized public int dlp_WriteUserInfo(UserInfo info) throws DlpException
			{ return calls.dlp_WriteUserInfo(socket, info); }
		synchronized public SysInfo dlp_ReadSysInfo() throws DlpException
			{ return calls.dlp_ReadSysInfo(socket); }
		synchronized public void dlp_OpenConduit() throws DlpException
			{ calls.dlp_OpenConduit(socket); }
		synchronized public NetInfo dlp_ReadNetSyncInfo() throws DlpException
			{ return calls.dlp_ReadNetSyncInfo(socket); }
		synchronized public int dlp_WriteNetSyncInfo(NetInfo info) throws DlpException
			{ return calls.dlp_WriteNetSyncInfo(socket, info); }
		synchronized public DBInfo dlp_ReadDBList(int card, int flags, int start) throws DlpException
			{ return calls.dlp_ReadDBList(socket, card, flags, start); }
		synchronized public int dlp_DeleteDB(int card, String name) throws DlpException
			{ return calls.dlp_DeleteDB(socket, card, name); }
		synchronized public int dlp_EndOfSync(int status) throws DlpException
			{ return calls.dlp_EndOfSync(socket, status); }
		synchronized public int dlp_MoveCategory(int handle, int from, int to) throws DlpException
			{ return calls.dlp_MoveCategory(socket, handle, from, to); }
		synchronized public int dlp_ResetSyncFlags(int handle) throws DlpException
			{ return calls.dlp_ResetSyncFlags(socket, handle); }
		synchronized public int dlp_CleanUpDatabase(int handle) throws DlpException
			{ return calls.dlp_CleanUpDatabase(socket, handle); }
		synchronized public int dlp_DeleteRecord(int handle, boolean all, RecordID id) throws DlpException
			{ return calls.dlp_DeleteRecord(socket, handle, all, id); }
		synchronized public int dlp_DeleteResource(int handle, boolean all, Char4 type, int id) throws DlpException
			{ return calls.dlp_DeleteResource(socket, handle, all, type, id); }
		synchronized public AppBlock dlp_ReadAppBlock(int handle, Database dbClass) throws DlpException
			{ return calls.dlp_ReadAppBlock(socket, handle, dbClass); }
		synchronized public SortBlock dlp_ReadSortBlock(int handle, Database dbClass) throws DlpException
			{ return calls.dlp_ReadSortBlock(socket, handle, dbClass); }
		synchronized public int dlp_WriteAppBlock(int handle, AppBlock appblock) throws DlpException
			{ return calls.dlp_WriteAppBlock(socket, handle, appblock); }
		synchronized public int dlp_WriteSortBlock(int handle, SortBlock sortblock) throws DlpException
			{ return calls.dlp_WriteSortBlock(socket, handle, sortblock); }
		synchronized public Pref dlp_ReadAppPreference(Char4 creator, int id, boolean backup, Database dbClass) throws DlpException
			{ return calls.dlp_ReadAppPreference(socket, creator, id, backup, dbClass); }
		synchronized public int dlp_WriteAppPreference(Pref pref) throws DlpException
			{ return calls.dlp_WriteAppPreference(socket, pref); }
		synchronized public int dlp_ReadOpenDBInfo(int handle) throws DlpException
			{ return calls.dlp_ReadOpenDBInfo(socket, handle); }
		synchronized public int dlp_ReadFeature(Char4 creator, int id) throws DlpException
			{ return calls.dlp_ReadFeature(socket, creator, id); }
		synchronized public Record dlp_ReadNextModifiedRec(int handle, Database dbClass) throws DlpException
			{ return calls.dlp_ReadNextModifiedRec(socket, handle, dbClass); }
		synchronized public Record dlp_ReadNextModifiedRecInCategory(int handle, int category, Database dbClass) throws DlpException
			{ return calls.dlp_ReadNextModifiedRecInCategory(socket, handle, category, dbClass); }
		synchronized public Record dlp_ReadNextRecInCategory(int handle, int category, Database dbClass) throws DlpException
			{ return calls.dlp_ReadNextRecInCategory(socket, handle, category, dbClass); }
		synchronized public RecordID[] dlp_ReadRecordIDList(int handle, boolean sort, int start, int max) throws DlpException
			{ return calls.dlp_ReadRecordIDList(socket, handle, sort, start, max); }
		synchronized public int dlp_DeleteCategory(int handle, int category) throws DlpException
			{ return calls.dlp_DeleteCategory(socket, handle, category); }
		synchronized public int dlp_ResetDBIndex(int handle) throws DlpException
			{ return calls.dlp_ResetDBIndex(socket, handle); }
		synchronized public int dlp_ResetSystem() throws DlpException
			{ return calls.dlp_ResetSystem(socket); }
    	synchronized public byte[] dlp_CallApplication(Char4 creator, int type, int action, byte[] data, int[] retcode)  	throws DlpException
    		{ return calls.dlp_CallApplication(socket, creator, type, action, data, retcode); }
    			
		public void close() throws DlpException, IOException {
			/* This method must be idempotent */
			if (this.socket != 0) {
				dlp_EndOfSync(constants.dlpEndCodeNormal);
				super.close();
			}
		}

		public void close(int status) throws DlpException, IOException {
			/* This method must be idempotent */
			if (this.socket != 0) {
				dlp_EndOfSync(status);
				super.close();
			}
		}

		public void reset() throws DlpException {
			dlp_ResetSystem();
		}

		public DB open(Pdapilot.Database dbClass)
			throws DlpException
		{
			int handle = this.dlp_OpenDB(0, 0x40|0x80|0x20, dbClass.dbname());
			return new DB(this, handle, dbClass.dbname(), 0x40|0x80|0x20, 0, dbClass);
		}
		
		public DB open(String name) throws DlpException { return this.open(name,0x40|0x80|0x20); }
		public DB open(String name, int mode) throws DlpException { return this.open(name,mode,0); }
		public DB open(String name, int mode, int card) throws DlpException {
			int handle = this.dlp_OpenDB(card, mode, name);
			return new DB(this, handle, name, mode, card);
		}
		
		public DB create(String name, Char4 creator, Char4 type, int flags, int version) throws DlpException
			{ return create(name, creator, type, flags, version, 0); }
		public DB create(String name, Char4 creator, Char4 type, int flags, int version, int card) 
			throws DlpException
		{
			int handle = this.dlp_CreateDB(creator, type, card, flags, version, name);
			return new DB(this, handle, name, 0x80|0x40|0x20, card);
		}
		
		public CardInfo getCardInfo(int card)
			throws DlpException
		{
			return this.dlp_ReadStorageInfo(card);
		}
		
		public java.util.Date getTime()
			throws DlpException
		{
			return dlp_GetSysDateTime();
		}

		public UserInfo getUserInfo()
			throws DlpException
		{
			return dlp_ReadUserInfo();
		}

		public SysInfo getSysInfo()
			throws DlpException
		{
			return dlp_ReadSysInfo();
		}
		
		public void getStatus()
			throws DlpException, CancelSyncException
		{
			dlp_OpenConduit();
		}

		public DBInfo getDBInfo(int start, boolean RAM, boolean ROM, int card)
			throws DlpException
		{
			return dlp_ReadDBList(card, (RAM ? constants.dlpDBListRAM : 0) |
										(ROM ? constants.dlpDBListROM : 0),
										start);
		}
		
		public int getFeature(Char4 type, int id)
			throws DlpException
		{
			return dlp_ReadFeature(type, id);
		}
		
		public Pref getPref(Char4 creator, int id)
			throws DlpException
		{	return getPref(creator, id, true); }
		public Pref getPref(Char4 creator, int id, boolean backup)
			throws DlpException
		{
			Database dbClass = (Database)Database.prefClasses.get(creator);
			if (dbClass == null)
				dbClass = Database.defaultPrefClass;
			return dlp_ReadAppPreference(creator, id, backup, dbClass);
		}

		public Pref newPref(Char4 creator, int id)
			throws DlpException
		{	return newPref(creator, id, 1); }
		public Pref newPref(Char4 creator, int id, int version)
			throws DlpException
		{	return newPref(creator, id, version, true); }
		public Pref newPref(Char4 creator, int id, int version, boolean backup)
			throws DlpException
		{
			Database dbClass = (Database)Database.prefClasses.get(creator);
			if (dbClass == null)
				dbClass = Database.defaultPrefClass;
			return dbClass.newPref(null, creator, id, version, backup);
		}

		public void setPref(Pref pref)
			throws DlpException
		{
			dlp_WriteAppPreference(pref);
		}

		public NetInfo getNetInfo()
			throws DlpException
		{
			return dlp_ReadNetSyncInfo();
		}

		public void setNetInfo(NetInfo info)
			throws DlpException
		{
			dlp_WriteNetSyncInfo(info);
		}
		
		public void log(String message) throws DlpException {
			dlp_AddSyncLogEntry(message);
		}
		
		public byte[] callApplication(Char4 creator, int type, int action, byte[] data, int[] retcode)
			throws DlpException
		{
			return dlp_CallApplication(creator, type, action, data, retcode);
		}
		
		public void delete(String dbname) throws DlpException
			{	delete(dbname, 0); }
		public void delete(String dbname, int card) throws DlpException {
			dlp_DeleteDB(card, dbname);
		}
}
