
package Pdapilot;

import java.io.*;

public class calls {
	static public native int pi_close(int socket) throws IOException;
	static public native int pi_socket(int domain, int type, int protocol) throws IOException;
	static public native int pi_bind(int socket, String device) throws IOException;
	static public native int pi_listen(int socket, int backlog) throws IOException;
	static public native int pi_accept(int socket) throws IOException;
	static public native int pi_version(int socket) throws IOException;
	static public native int pi_tickle(int socket) throws IOException;
	static public native int pi_watchdog(int socket, int interval) throws IOException;
	static public native int pi_read(int socket, byte[] data, int len) throws IOException;
	static public native int pi_write(int socket, byte[] data, int len) throws IOException;
	static public native int dlp_OpenDB(int socket, int card, int mode, String name) throws DlpException;
	static public native int dlp_DeleteDB(int socket, int card, String name) throws DlpException;
	static public native int dlp_EndOfSync(int socket, int status) throws DlpException;
	static public native int dlp_DeleteCategory(int socket, int handle, int category) throws DlpException;
	static public native int dlp_CloseDB(int socket, int handle) throws DlpException;
	static public native int dlp_AddSyncLogEntry(int socket, String entry) throws DlpException;
	static public native String dlp_strerror(int error);

	static public native java.util.Date dlp_GetSysDateTime(int socket) throws DlpException;
	static public native int dlp_SetSysDateTime(int socket, java.util.Date date) throws DlpException;
	static public native AppBlock dlp_ReadAppBlock(int socket, int handle, Database dbClass) throws DlpException;
	static public native SortBlock dlp_ReadSortBlock(int socket, int handle, Database dbClass) throws DlpException;
	static public native Pref dlp_ReadAppPreference(int socket, Char4 creator, int id, boolean backup, Database dbClass) throws DlpException;
	static public native Record dlp_ReadRecordByIndex(int socket, int handle, int index, Database dbClass) throws DlpException;
	static public native Record dlp_ReadNextModifiedRec(int socket, int handle, Database dbClass) throws DlpException;
	static public native Record dlp_ReadNextModifiedRecInCategory(int socket, int handle, int category, Database dbClass) throws DlpException;
	static public native Record dlp_ReadNextRecInCategory(int socket, int handle, int category, Database dbClass) throws DlpException;
	static public native Record dlp_ReadRecordByID(int socket, int handle, RecordID id, Database dbClass) throws DlpException;
	static public native Resource dlp_ReadResourceByType(int socket, int handle, Char4 type, int id, Database dbClass) throws DlpException;
	static public native Resource dlp_ReadResourceByIndex(int socket, int handle, int index, Database dbClass) throws DlpException;
	static public native int dlp_WriteRecord(int socket, int handle, Record record) throws DlpException;
	static public native int dlp_WriteAppPreference(int socket, Pref pref) throws DlpException;
	static public native int dlp_WriteResource(int socket, int handle, Resource resource) throws DlpException;
	static public native int dlp_WriteAppBlock(int socket, int handle, AppBlock appblock) throws DlpException;
	static public native int dlp_WriteSortBlock(int socket, int handle, SortBlock sortblock) throws DlpException;
	static public native CardInfo dlp_ReadStorageInfo(int socket, int card) throws DlpException;
	static public native int dlp_CreateDB(int socket, Char4 creator, Char4 type, int card, int flags, int version, String name) throws DlpException;
	static public native int dlp_ResetSystem(int socket) throws DlpException;
	static public native int dlp_OpenConduit(int socket) throws DlpException;
	static public native UserInfo dlp_ReadUserInfo(int socket) throws DlpException;
	static public native int dlp_WriteUserInfo(int socket, UserInfo info) throws DlpException;
	static public native SysInfo dlp_ReadSysInfo(int socket) throws DlpException;
	static public native NetInfo dlp_ReadNetSyncInfo(int socket) throws DlpException;
	static public native int dlp_WriteNetSyncInfo(int socket, NetInfo info) throws DlpException;
	static public native DBInfo dlp_ReadDBList(int socket, int card, int flags, int start) throws DlpException;
	static public native int dlp_CleanUpDatabase(int socket, int handle) throws DlpException;
	static public native int dlp_ResetSyncFlags(int socket, int handle) throws DlpException;
	static public native int dlp_MoveCategory(int socket, int handle, int from, int to) throws DlpException;
	static public native int dlp_DeleteRecord(int socket, int handle, boolean all, RecordID id) throws DlpException;
	static public native int dlp_DeleteResource(int socket, int handle, boolean all, Char4 type, int id) throws DlpException;
	static public native int dlp_ReadOpenDBInfo(int socket, int handle) throws DlpException;
	static public native int dlp_ReadFeature(int socket, Char4 creator, int id) throws DlpException;
	static public native RecordID[] dlp_ReadRecordIDList(int socket, int handle, boolean sort, int start, int max) throws DlpException;
	
	static public byte[] getByteArray(int length) { return new byte[length]; }
	static public int getArrayLength(byte[] array) { return array.length; }
	static public RecordID[] makeRecordIDArray(int length) { return new RecordID[length]; }
	static public Object[] makeStringArray(int length) { return new String[length]; }
	static public int[] makeIntArray(int length) { return new int[length]; }
	
	static {
		System.loadLibrary("JavaPisock");
	}
	
	
}

