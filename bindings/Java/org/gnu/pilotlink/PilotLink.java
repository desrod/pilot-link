package org.gnu.pilotlink;

public class PilotLink {
	static {
		System.loadLibrary("jpisock");
	}
	public boolean isConnected() {
		return handle>=0;
	}
	public PilotLink(String port) {
		handle=connect(port);
	}
        private int handle;
	private native int connect(String port);
	
	public SysInfo getSysInfo() {
		System.out.println("Getting Sysinfo...");
		SysInfo si=readSysInfo(handle);
		return si;
	}
	private native SysInfo readSysInfo(int handle);

	public User getUserInfo() {
		User u=readUserInfo(handle);
		return u;
	}
	private native User readUserInfo(int handle);
	public RawAppInfo getAppInfo(int db) {
		return readAppInfo(handle,db);
	}
    
	private native RawAppInfo readAppInfo(int h, int db);
	public void writeUserInfo(User u) {
		writeUserInfo(handle, u);
	}
	private native void writeUserInfo(int h, User u);
	
	private long toLong(char[] charArray) {
		int length = charArray.length;
		long result = 0;
		for (int i=0; i < length-1; i++) {
			result |= charArray[i]; 
			result <<=8;
		}
		result |= charArray[length-1];
		return result;
	}
		
	public int createDB(String dbn, String creator, String type) {
		System.out.println("Creating new database...");
		long longCreator=toLong(creator.toCharArray());
		long longType=toLong(type.toCharArray());
		
		return createDB(handle,longCreator,dbn,longType);
	}

	private native int createDB(int handle, long creator, String dbname, long type);
	
	public int deleteDB(String dbn) {
		System.out.println("Deleting database " + dbn);
		return deleteDB(handle,dbn);
	}

	private native int deleteDB(int handle, String dbname);
	
	public int openDB(String dbn) {
		return openDB(handle, dbn);
	}
	private native int openDB(int handle, String dbname);

	public int writeAppBlock(byte[] data, int dbhandle) {
		return writeAppBlock(handle,dbhandle,data,data.length);
	}

	private native int writeAppBlock(int handle, int dbhandle, byte[] data, int length);

	public int getRecordCount(int dbhandle) {
		return getRecordCount(handle,dbhandle);
	}
	private native int getRecordCount(int handle, int dbhandle);
	
	public Record getRecordByIndex(int dbh, int idx) {
		return getRecordByIndex(handle,dbh,idx);
	}
	private native RawRecord getRecordByIndex(int handle, int dbhandle, int idx);

	public void deleteRecordByIndex(int dbhandle, int idx) {
		deleteRecordByIndex(handle,dbhandle,idx);
	}
	
	private native int deleteRecordByIndex(int handle, int dbhandle, int id);
	
	public boolean writeRecord(int dbh, Record r) {
		return writeRecord(handle, dbh,  r)>0;
	}
	private native int writeRecord(int handle,int dbhandle, Record r);
	
	public int writeNewRecord(int dbhandle, Record r) {
		return writeRecord(handle,dbhandle,r);
	}
	public void closeDB(int dbh) {
		closeDB(handle,dbh);
		dbh=0;
	}
	private native void closeDB(int handle, int dbhandle);
	
	public void endSync() {
		endSync(handle);
	}
	private native void endSync(int handle);
	public void close() {
		close(handle);
		handle=0;
	}
	private native void close(int handle);
	
    public void openConduit() {
        openConduit(handle);
    }
    
    private native void openConduit(int handle);
}
