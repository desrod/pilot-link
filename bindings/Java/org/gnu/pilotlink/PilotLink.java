package org.gnu.pilotlink;

import java.io.IOException;

public class PilotLink {
	static {
		System.loadLibrary("jpisock");
	}

	public boolean isConnected() {
		return handle >= 0;
	}

	public PilotLink(String port) throws IOException, PilotLinkException {
		System.out.println("Initialising Pilot-Link-Java-Bindings V"+Version.VERSION+" build #"+Version.BUILD);
		handle = connect(port);
	}

	private int handle;

	private native/* synchronized */int connect(String port)
			throws IOException, PilotLinkException;

	public SysInfo getSysInfo() throws PilotLinkException {
		//		System.out.println("Getting Sysinfo...");
		SysInfo si = readSysInfo(handle); 
		return si;
	}

	private native/* synchronized */SysInfo readSysInfo(int handle)
			throws PilotLinkException;

	public User getUserInfo() throws PilotLinkException {
		User u = readUserInfo(handle);
		return u;
	}

	private native/* synchronized */User readUserInfo(int handle)
			throws PilotLinkException;
	/**
	* @deprecated
	*/
	public RawAppInfo getAppInfo(int db) throws PilotLinkException {
		return readAppInfo(handle, db);
	}

	private native/* synchronized */RawAppInfo readAppInfo(int h, int db)
			throws PilotLinkException;
	
	public RawAppInfo getAppInfoBlock(String dbname) {
		return getAppInfoBlock(handle,dbname);
	}
	private native RawAppInfo getAppInfoBlock(int h, String dbnmae);

	public void writeUserInfo(User u) throws PilotLinkException {
		writeUserInfo(handle, u);
	}

	private native/* synchronized */void writeUserInfo(int h, User u)
			throws PilotLinkException;

	private long toLong(char[] charArray) {
		int length = charArray.length;
		long result = 0;
		for (int i = 0; i < length - 1; i++) {
			result |= charArray[i];
			result <<= 8;
		}
		result |= charArray[length - 1];
		return result;
	}

	public int createDB(String dbn, String creator, String type)
			throws PilotLinkException {
		//		System.out.println("Creating new database...");
		long longCreator = toLong(creator.toCharArray());
		long longType = toLong(type.toCharArray());

		return createDB(handle, longCreator, dbn, longType, 0, 1);
	}

	public int createDB(String dbn, String creator, String type, int flags,
			int version) throws PilotLinkException {
		long longCreator = toLong(creator.toCharArray());
		long longType = toLong(type.toCharArray());

		return createDB(handle, longCreator, dbn, longType, flags, version);
	}

	private native/* synchronized */int createDB(int handle, long creator,
			String dbname, long type) throws PilotLinkException;

	private native int createDB(int handle, long creator, String dbname,
			long type, int flags, int version) throws PilotLinkException;

	public int deleteDB(String dbn) throws PilotLinkException {
		//		System.out.println("Deleting database " + dbn);
		return deleteDB(handle, dbn);
	}

	private native/* synchronized */int deleteDB(int handle, String dbname)
			throws PilotLinkException;

	public int openDB(String dbn) throws PilotLinkException {
		return openDB(handle, dbn);
	}

	private native/* synchronized */int openDB(int handle, String dbname)
			throws PilotLinkException;

	public int writeAppBlock(byte[] data, int dbhandle)
			throws PilotLinkException {
		return writeAppBlock(handle, dbhandle, data, data.length);
	}

	private native/* synchronized */int writeAppBlock(int handle,
			int dbhandle, byte[] data, int length) throws PilotLinkException;

	public int getRecordCount(int dbhandle) throws PilotLinkException {
		return getRecordCount(handle, dbhandle);
	}

	private native/* synchronized */int getRecordCount(int handle, int dbhandle)
			throws PilotLinkException;

	public Record getRecordByIndex(int dbh, int idx) throws PilotLinkException {
		return getRecordByIndex(handle, dbh, idx);
	}

	private native/* synchronized */RawRecord getRecordByIndex(int handle,
			int dbhandle, int idx) throws PilotLinkException;

	public void deleteRecordById(int dbhandle, long id) {
		deleteRecordById(handle, dbhandle, id);
	}

	private native/* synchronized */int deleteRecordById(int handle,
			int dbhandle, long id);

	public boolean writeRecord(int dbh, Record r) throws PilotLinkException {
		return writeRecord(handle, dbh, r) > 0;
	}

	private native/* synchronized */int writeRecord(int handle, int dbhandle,
			Record r) throws PilotLinkException;

	public int writeNewRecord(int dbhandle, Record r) throws PilotLinkException {
		return writeRecord(handle, dbhandle, r);
	}

	public void closeDB(int dbh) throws PilotLinkException {
		closeDB(handle, dbh);
		dbh = 0;
	}

	private native/* synchronized */void closeDB(int handle, int dbhandle)
			throws PilotLinkException;

	public void endSync() throws PilotLinkException {
		endSync(handle);
	}

	private native/* synchronized */void endSync(int handle)
			throws PilotLinkException;

	public void close() {
		close(handle);
		handle = 0;
	}

	private native/* synchronized */void close(int handle);

	public void openConduit() throws PilotLinkException {
		openConduit(handle);
	}

	private native/* synchronized */void openConduit(int handle)
			throws PilotLinkException;

	private native/* synchronized */RawRecord getResourceByIndex(int handle,
			int dbhandle, int idx) throws PilotLinkException;

	private native/* synchronized */void writeResource(int handle,
			int dbhandle, RawRecord r) throws PilotLinkException;

	private native/* synchronized */void resetSystem(int handle)
			throws PilotLinkException;

	private native/* synchronized */DBInfo readDBList(int handle, int cardno,
			int flags, int start) throws PilotLinkException;

	public RawRecord getResourceByIndex(int dbhandle, int idx)
			throws PilotLinkException {
		return getResourceByIndex(handle, dbhandle, idx);
	}

	public void writeResource(int dbhandle, RawRecord r)
			throws PilotLinkException {
		writeResource(handle, dbhandle, r);
	}

	public void resetSystem() throws PilotLinkException {
		resetSystem(handle);
	}

	public DBInfo readDBList(int cardno, int flags, int start)
			throws PilotLinkException {
		return readDBList(handle, cardno, flags, start);
	}
}
