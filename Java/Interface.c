#include "Pdapilot_calls.h"
#include "Pdapilot_Database.h"
#include "Pdapilot_Record.h"
#include "Pdapilot_Pref.h"
#include "Pdapilot_AppBlock.h"
#include "Pdapilot_SortBlock.h"
#include "Pdapilot_Resource.h"
#include "Pdapilot_CardInfo.h"
#include "Pdapilot_UserInfo.h"
#include "Pdapilot_SysInfo.h"
#include "Pdapilot_NetInfo.h"
#include "Pdapilot_memo_Record.h"
#include "Pdapilot_memo_AppBlock.h"
#include "Pdapilot_RecordID.h"
#include "Pdapilot_Char4.h"
#include "Pdapilot_DBInfo.h"
#include "java_util_Date.h"

#include "pi-source.h"

#include "pi-dlp.h"
#include "pi-socket.h"
#include "pi-memo.h"

#include <signal.h>

/** Sun should be ashamed for the 1.0 JNI!
 */
 
static HArrayOfByte * getByteArray(long length)
{
	return (HArrayOfByte*)execute_java_static_method(0, FindClass(0, "Pdapilot/calls",1),
		"getByteArray", "(I)[B", (long)length);
}

static HArrayOfObject * makeRecordIDArray(long length)
{
	return (HArrayOfObject*)execute_java_static_method(0, FindClass(0, "Pdapilot/calls",1),
		"makeRecordIDArray", "(I)[LPdapilot/RecordID;", (long)length);
}

static HArrayOfObject * makeStringArray(long length)
{
	return (HArrayOfObject*)execute_java_static_method(0, FindClass(0, "Pdapilot/calls",1),
		"makeStringArray", "(I)[Ljava/lang/Object;", (long)length);
}

static HArrayOfInt * makeIntArray(long length)
{
	return (HArrayOfInt*)execute_java_static_method(0, FindClass(0, "Pdapilot/calls",1),
		"makeIntArray", "(I)[I", (long)length);
}

static long getArrayLength(struct HArrayOfByte *b)
{
	return (long)execute_java_static_method(0, FindClass(0, "Pdapilot/calls",1),
		"getArrayLength", "([B)I", b);
}

Hjava_util_Date * makeJavaDate(time_t v) {
	if (v < 18000) {
		return 0;
	} else {
		struct tm * tm;

		tm = localtime(&v);

		return (Hjava_util_Date*)execute_java_constructor(0, "java/util/Date", 0, 
			"(IIIIII)",
			tm->tm_year, tm->tm_mon, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec);
	}
	
}	

HPdapilot_Char4 * makeJavaChar4(long id) {
	return (HPdapilot_Char4*)execute_java_constructor(
		0, "Pdapilot/Char4", 0, 
		"(I)", id);
}

int getJavaChar4(HPdapilot_Char4 * id) {
	if (id)
		return unhand(id)->value;
	else
		return 0;
}

HPdapilot_RecordID * makeJavaRecordID(long id) {
	return (HPdapilot_RecordID*)execute_java_constructor(
		0, "Pdapilot/RecordID", 0, 
		"(I)", id);
}

int getJavaRecordID(HPdapilot_RecordID * id) {
	if (id)
		return unhand(id)->value;
	else
		return 0;
}

time_t readJavaDate(Hjava_util_Date * date) {
	struct tm tm;
	
	tm.tm_year = unhand(date)->tm_year;
	tm.tm_mon = unhand(date)->tm_mon;
	tm.tm_mday = unhand(date)->tm_mday;
	tm.tm_hour = unhand(date)->tm_hour;
	tm.tm_min = unhand(date)->tm_min;
	tm.tm_sec = unhand(date)->tm_sec;
	
	return mktime(&tm);
}

extern long Pdapilot_calls_pi_socket(struct HPdapilot_calls*self, long domain, long type, long protocol)
{
	int result = pi_socket(domain, type, protocol);
	if (result<0)
		SignalError(0, "java/lang/IOException", 0);
	return result;
}

extern long Pdapilot_calls_pi_bind(struct HPdapilot_calls*self, long socket, struct Hjava_lang_String * address)
{
	long len = javaStringLength(address)+3;
	struct pi_sockaddr * a = malloc(len);
	long result;
	a->pi_family = PI_AF_SLP;
	javaString2CString(address, a->pi_device, len-2);
	result = pi_bind(socket, (struct sockaddr*)a, len);
	free(a);
	if (result<0)
		SignalError(0, "java/lang/IOException", 0);
	return result;
}

extern long Pdapilot_calls_pi_listen(struct HPdapilot_calls*self, long socket, long backlog)
{
	int result = pi_listen(socket, backlog);
	if (result<0)
		SignalError(0, "java/lang/IOException", 0);
	return result;
	
}

extern long Pdapilot_calls_pi_accept(struct HPdapilot_calls*self, long socket)
{
	int result=pi_accept(socket, 0, 0);
	if (result<0)
		SignalError(0, "java/lang/IOException", 0);
	return result;
}

extern long Pdapilot_calls_pi_read(struct HPdapilot_calls*self, long socket, HArrayOfByte *b, long len)
{
	int result=pi_read(socket, unhand(b)->body, len);
	if (result<0)
		SignalError(0, "java/lang/IOException", 0);
	return result;
}

extern long Pdapilot_calls_pi_write(struct HPdapilot_calls*self, long socket, HArrayOfByte * b, long len)
{
	int result=pi_write(socket, unhand(b)->body, len);
	if (result<0)
		SignalError(0, "java/lang/IOException", 0);
	return result;
}

extern long Pdapilot_calls_pi_close(struct HPdapilot_calls * self, long socket)
{
	int result=pi_close(socket);
	if (result<0)
		SignalError(0, "java/lang/IOException", 0);
	return result;
}

extern long Pdapilot_calls_pi_version(struct HPdapilot_calls * self, long socket)
{
	int result=pi_version(socket);
	if (result<0)
		SignalError(0, "java/lang/IOException", 0);
	return result;
}

extern long Pdapilot_calls_pi_watchdog(struct HPdapilot_calls * self, long socket, long interval)
{
	int result=pi_watchdog(socket, interval);
	if (result<0)
		SignalError(0, "java/lang/IOException", 0);
	return result;
}

extern long Pdapilot_calls_pi_tickle(struct HPdapilot_calls * self, long socket)
{
	int result=pi_tickle(socket);
	if (result<0)
		SignalError(0, "java/lang/IOException", 0);
	return result;
}

extern long Pdapilot_calls_dlp_OpenDB(struct HPdapilot_calls * self, long socket, long card, long mode, Hjava_lang_String * name)
{
	int handle;
	int result = dlp_OpenDB(socket, card, mode, makeCString(name), &handle);
	if (result < 0) {
		SignalError(0, "Pdapilot/DlpException", 0);
		return result;
	}
	return handle;
}

extern long Pdapilot_calls_dlp_CreateDB(struct HPdapilot_calls * self, long socket, HPdapilot_Char4 * creator, HPdapilot_Char4 * type, long card, long flags, long version, Hjava_lang_String * name)
{
	int handle;
	int result = dlp_CreateDB(socket, unhand(creator)->value, unhand(creator)->value, card, flags, version, makeCString(name), &handle);
	if (result < 0) {
		SignalError(0, "Pdapilot/DlpException", 0);
		return result;
	}
	return handle;
}

extern long Pdapilot_calls_dlp_DeleteDB(struct HPdapilot_calls * self, long socket, long card, Hjava_lang_String * name)
{
	int result = dlp_DeleteDB(socket, card, makeCString(name));
	if (result < 0) {
		SignalError(0, "Pdapilot/DlpException", 0);
	}
	return result;
}

extern Hjava_util_Date * Pdapilot_calls_dlp_GetSysDateTime(struct HPdapilot_calls * self, long socket)
{
	time_t t;
	int result = dlp_GetSysDateTime(socket, &t);
	if (result < 0) {
		SignalError(0, "Pdapilot/DlpException", 0);
		return 0;
	}
	
	return makeJavaDate(t);
}


extern long Pdapilot_calls_dlp_SetSysDateTime(struct HPdapilot_calls * self, long socket, Hjava_util_Date * date)
{
	time_t t;
	int result;
	
	t = readJavaDate(date);
	
	result = dlp_SetSysDateTime(socket, t);
	if (result < 0) {
		SignalError(0, "Pdapilot/DlpException", 0);
	}
	return result;
}

extern long Pdapilot_calls_dlp_AddSyncLogEntry(struct HPdapilot_calls * self, long socket, Hjava_lang_String * entry)
{
	int result = dlp_AddSyncLogEntry(socket, makeCString(entry));
	if (result < 0)
		SignalError(0, "Pdapilot/DlpException", 0);
	return result;
}

extern long Pdapilot_calls_dlp_ResetSystem(struct HPdapilot_calls * self, long socket)
{
	int result = dlp_ResetSystem(socket);
	if (result < 0)
		SignalError(0, "Pdapilot/DlpException", 0);
	return result;
}

extern long Pdapilot_calls_dlp_EndOfSync(struct HPdapilot_calls * self, long socket, long status)
{
	int result = dlp_EndOfSync(socket, status);
	if (result < 0)
		SignalError(0, "Pdapilot/DlpException", 0);
	return result;
}

extern long Pdapilot_calls_dlp_MoveCategory(struct HPdapilot_calls * self, long socket, long handle, long from, long to)
{
	int result = dlp_MoveCategory(socket, handle, from, to);
	if (result < 0)
		SignalError(0, "Pdapilot/DlpException", 0);
	return result;
}

extern long Pdapilot_calls_dlp_DeleteRecord(struct HPdapilot_calls * self, long socket, long handle, long all, HPdapilot_RecordID * id)
{
	int result = dlp_DeleteRecord(socket, handle, all, getJavaRecordID(id));
	if (result < 0)
		SignalError(0, "Pdapilot/DlpException", 0);
	return result;
}

extern long Pdapilot_calls_dlp_DeleteCategory(struct HPdapilot_calls * self, long socket, long handle, long category)
{
	int result = dlp_DeleteCategory(socket, handle, category);
	if (result < 0)
		SignalError(0, "Pdapilot/DlpException", 0);
	return result;
}

extern long Pdapilot_calls_dlp_ReadOpenDBInfo(struct HPdapilot_calls * self, long socket, long handle)
{
	int count;
	int result = dlp_ReadOpenDBInfo(socket, handle, &count);
	if (result < 0)
		SignalError(0, "Pdapilot/DlpException", 0);
	return count;
}

extern long Pdapilot_calls_dlp_DeleteResource(struct HPdapilot_calls * self, long socket, long handle, long all, HPdapilot_Char4 * type, long id)
{
	int result = dlp_DeleteResource(socket, handle, all, unhand(type)->value, id);
	if (result < 0)
		SignalError(0, "Pdapilot/DlpException", 0);
	return result;
}

extern long Pdapilot_calls_dlp_ResetSyncFlags(struct HPdapilot_calls * self, long socket, long handle)
{
	int result = dlp_ResetSyncFlags(socket, handle);
	if (result < 0)
		SignalError(0, "Pdapilot/DlpException", 0);
	return result;
}

extern long Pdapilot_calls_dlp_CleanUpDatabase(struct HPdapilot_calls * self, long socket, long handle)
{
	int result = dlp_CleanUpDatabase(socket, handle);
	if (result < 0)
		SignalError(0, "Pdapilot/DlpException", 0);
	return result;
}


extern HPdapilot_CardInfo * Pdapilot_calls_dlp_ReadStorageInfo(struct HPdapilot_calls * self, long socket, long card)
{
	HPdapilot_CardInfo * output = NULL;
	struct CardInfo c;
	int result = dlp_ReadStorageInfo(socket, card, &c);
	if (result == -5) {
		return 0;
	} else if (result < 0) {
		SignalError(0, "Pdapilot/DlpException", 0);
		return 0;
	}
	output = (HPdapilot_CardInfo*)execute_java_constructor(0,
		 "Pdapilot/CardInfo", 0, "()");
	unhand(output)->name = makeJavaString(c.name, strlen(c.name));
	unhand(output)->manufacturer = makeJavaString(c.manuf, strlen(c.manuf));
	unhand(output)->number = c.cardno;
	unhand(output)->ROMsize = c.ROMsize;
	unhand(output)->RAMsize = c.RAMsize;
	unhand(output)->RAMfree = c.RAMfree;
	
	unhand(output)->more = c.more;
	unhand(output)->creation = makeJavaDate(c.creation);

	return output;
}

extern HPdapilot_SysInfo * Pdapilot_calls_dlp_ReadSysInfo(struct HPdapilot_calls * self, long socket)
{
	HPdapilot_SysInfo * output = NULL;
	struct SysInfo s;
	int result = dlp_ReadSysInfo(socket, &s);
	if (result < 0) {
		SignalError(0, "Pdapilot/DlpException", 0);
		return 0;
	}
	output = (HPdapilot_SysInfo*)execute_java_constructor(0,
		 "Pdapilot/SysInfo", 0, "()");
	unhand(output)->ROMversion = s.ROMVersion;
	unhand(output)->localeID = s.localizationID;
	unhand(output)->name = makeJavaString(s.name, s.namelength);

	return output;
}

extern HPdapilot_DBInfo * Pdapilot_calls_dlp_ReadDBList(struct HPdapilot_calls * self, long socket, long card, long flags, long start)
{
	HPdapilot_DBInfo * output = NULL;
	struct DBInfo i;
	int result = dlp_ReadDBList(socket, card, flags, start, &i);
	if (result == -5) 
		return 0;
	else if (result < 0) {
		SignalError(0, "Pdapilot/DlpException", 0);
		return 0;
	}
	
	output = (HPdapilot_DBInfo*)execute_java_constructor(0,
		 "Pdapilot/DBInfo", 0, "()");
		 
	unhand(output)->flags = i.flags;
	unhand(output)->index = i.index;
	unhand(output)->version = i.version;
	unhand(output)->modnum = i.modnum;
	unhand(output)->miscflags = i.miscflags;
	unhand(output)->type = makeJavaChar4(i.type);
	unhand(output)->creator = makeJavaChar4(i.creator);
	unhand(output)->creation = makeJavaDate(i.crdate);
	unhand(output)->modification = makeJavaDate(i.moddate);
	unhand(output)->backup = makeJavaDate(i.backupdate);
	unhand(output)->name = makeJavaString(i.name, strlen(i.name));
	unhand(output)->card = card;

	unhand(output)->more = i.more;

	return output;
}

extern HPdapilot_UserInfo * Pdapilot_calls_dlp_ReadUserInfo(struct HPdapilot_calls * self, long socket)
{
	HPdapilot_UserInfo * output = NULL;
	struct PilotUser u;
	int result = dlp_ReadUserInfo(socket, &u);
	if (result < 0) {
		SignalError(0, "Pdapilot/DlpException", 0);
		return 0;
	}
	output = (HPdapilot_UserInfo*)execute_java_constructor(0,
		 "Pdapilot/UserInfo", 0, "()");
	
	unhand(output)->username = makeJavaString(u.username, strlen(u.username));
	unhand(output)->userID = u.userID;
	unhand(output)->viewerID = u.viewerID;
	unhand(output)->lastSyncPC = u.lastSyncPC;
	unhand(output)->password= getByteArray(u.passwordLen);
	memcpy(unhand(unhand(output)->password)->body, u.password, u.passwordLen);
	
	unhand(output)->syncSuccess = makeJavaDate(u.succSyncDate);
	unhand(output)->syncLast = makeJavaDate(u.lastSyncDate);

	return output;
}

extern long Pdapilot_calls_dlp_WriteUserInfo(struct HPdapilot_calls * self, long socket, HPdapilot_UserInfo * user)
{
	struct PilotUser u;
	int result;
	
	u.userID = unhand(user)->userID;
	u.viewerID = unhand(user)->viewerID;
	u.lastSyncPC = unhand(user)->lastSyncPC;
	u.passwordLen = getArrayLength(unhand(user)->password);
	memcpy(u.password, unhand(unhand(user)->password)->body, u.passwordLen);
	u.succSyncDate = readJavaDate(unhand(user)->syncSuccess);
	u.lastSyncDate = readJavaDate(unhand(user)->syncLast);
	javaString2CString(unhand(user)->username, u.username, 127);
	
	result = dlp_WriteUserInfo(socket, &u);
	if (result < 0) {
		SignalError(0, "Pdapilot/DlpException", 0);
	}
	return result;
}


extern HPdapilot_NetInfo * Pdapilot_calls_dlp_ReadNetSyncInfo(struct HPdapilot_calls * self, long socket)
{
	HPdapilot_NetInfo * output = NULL;
	struct NetSyncInfo i;
	int result = dlp_ReadNetSyncInfo(socket, &i);
	if (result < 0) {
		SignalError(0, "Pdapilot/DlpException", 0);
		return 0;
	}
	output = (HPdapilot_NetInfo*)execute_java_constructor(0,
		 "Pdapilot/NetInfo", 0, "()");
		 
	unhand(output)->LANsync = i.lansync;
	unhand(output)->hostName = makeJavaString(i.PCName, strlen(i.PCName));
	unhand(output)->hostAddress = makeJavaString(i.PCAddr, strlen(i.PCAddr));
	unhand(output)->hostSubnetMask = makeJavaString(i.PCMask, strlen(i.PCMask));
	
	return output;
}

extern long Pdapilot_calls_dlp_WriteNetSyncInfo(struct HPdapilot_calls * self, long socket, HPdapilot_NetInfo * info)
{
	struct NetSyncInfo i;
	int result;
	
	i.lansync = unhand(info)->LANsync;
	javaString2CString(unhand(info)->hostName, i.PCName, 256);
	javaString2CString(unhand(info)->hostAddress, i.PCAddr, 40);
	javaString2CString(unhand(info)->hostSubnetMask, i.PCMask, 40);
	
	result = dlp_WriteNetSyncInfo(socket, &i);
	if (result < 0) {
		SignalError(0, "Pdapilot/DlpException", 0);
	}
	return result;
}

extern long Pdapilot_calls_dlp_OpenConduit(struct HPdapilot_calls * self, long socket)
{
	int result = dlp_OpenConduit(socket);
	if (result == dlpErrSync)
		SignalError(0, "Pdapilot/CancelSyncException", 0);
	else if (result < 0)
		SignalError(0, "Pdapilot/DlpException", 0);
	return result;
}

extern long Pdapilot_calls_dlp_CloseDB(struct HPdapilot_calls * self, long socket, long handle)
{
	int result;
	result = dlp_CloseDB(socket, handle);
	if (result<0)
		SignalError(0, "Pdapilot/DlpException", 0);
	return result;
}

extern Hjava_lang_String * Pdapilot_calls_dlp_strerror(struct HPdapilot_calls *self, long error)
{
	char * result = dlp_strerror(error);
	if (!result)
		result = "Unknown DLP error";
	return makeJavaString(result, strlen(result));
}

extern HPdapilot_Record * Pdapilot_calls_dlp_ReadRecordByIndex(struct HPdapilot_calls * self, long socket, long handle, long index, HPdapilot_Database * dbClass)
{
	int attr, cat;
	recordid_t id;
	char * buffer = malloc(0xffff);
	int len;
	HPdapilot_Record * output = NULL;
	
	int result = dlp_ReadRecordByIndex(socket, handle, index, buffer, &id, &len, &attr, &cat);
	
	if (result >= 0) {
		HArrayOfByte * a = getByteArray(len);
		memcpy(unhand(a)->body, buffer, len);
		output = (HPdapilot_Record*)execute_java_dynamic_method(0, (HObject*)dbClass, 
			"newRecord", "([BLPdapilot/RecordID;III)LPdapilot/Record;",
			a, makeJavaRecordID(id), (long)index, (long)attr, (long)cat);
	} else if (result != -5) {
		SignalError(0, "Pdapilot/DlpException", 0);
	}
	
	free(buffer);
	
	return output;
}

extern HPdapilot_Record * Pdapilot_calls_dlp_ReadNextModifiedRec(struct HPdapilot_calls * self, long socket, long handle, HPdapilot_Database * dbClass)
{
	int attr, cat, index;
	recordid_t id;
	char * buffer = malloc(0xffff);
	int len;
	HPdapilot_Record * output = NULL;
	
	int result = dlp_ReadNextModifiedRec(socket, handle, buffer, &id, &index, &len, &attr, &cat);
	
	if (result >= 0) {
		HArrayOfByte * a = getByteArray(len);
		memcpy(unhand(a)->body, buffer, len);
		output = (HPdapilot_Record*)execute_java_dynamic_method(0, (HObject*)dbClass, 
			"newRecord", "([BLPdapilot/RecordID;III)LPdapilot/Record;",
			a, makeJavaRecordID(id), (long)index, (long)attr, (long)cat);
	} else if (result != -5) {
		SignalError(0, "Pdapilot/DlpException", 0);
	}
	
	free(buffer);
	
	return output;
}

extern HPdapilot_Record * Pdapilot_calls_dlp_ReadNextModifiedRecInCategory(struct HPdapilot_calls * self, long socket, long handle, long cat, HPdapilot_Database * dbClass)
{
	int attr, index;
	recordid_t id;
	char * buffer = malloc(0xffff);
	int len;
	HPdapilot_Record * output = NULL;
	
	int result = dlp_ReadNextModifiedRecInCategory(socket, handle, cat, buffer, &id, &index, &len, &attr);
	
	if (result >= 0) {
		HArrayOfByte * a = getByteArray(len);
		memcpy(unhand(a)->body, buffer, len);
		output = (HPdapilot_Record*)execute_java_dynamic_method(0, (HObject*)dbClass, 
			"newRecord", "([BLPdapilot/RecordID;III)LPdapilot/Record;",
			a, makeJavaRecordID(id), (long)index, (long)attr, (long)cat);
	} else if (result != -5) {
		SignalError(0, "Pdapilot/DlpException", 0);
	}
	
	free(buffer);
	
	return output;
}

extern HPdapilot_Record * Pdapilot_calls_dlp_ReadNextRecInCategory(struct HPdapilot_calls * self, long socket, long handle, long cat, HPdapilot_Database * dbClass)
{
	int attr, index;
	recordid_t id;
	char * buffer = malloc(0xffff);
	int len;
	HPdapilot_Record * output = NULL;
	
	int result = dlp_ReadNextRecInCategory(socket, handle, cat, buffer, &id, &index, &len, &attr);
	
	if (result >= 0) {
		HArrayOfByte * a = getByteArray(len);
		memcpy(unhand(a)->body, buffer, len);
		output = (HPdapilot_Record*)execute_java_dynamic_method(0, (HObject*)dbClass, 
			"newRecord", "([BLPdapilot/RecordID;III)LPdapilot/Record;",
			a, makeJavaRecordID(id), (long)index, (long)attr, (long)cat);
	} else if (result != -5) {
		SignalError(0, "Pdapilot/DlpException", 0);
	}
	
	free(buffer);
	
	return output;
}

extern HPdapilot_AppBlock * Pdapilot_calls_dlp_ReadAppBlock(struct HPdapilot_calls * self, long socket, long handle, HPdapilot_Database * dbClass)
{
	char * buffer = malloc(0xffff);
	int len;
	HPdapilot_AppBlock * output = NULL;
	
	int result = dlp_ReadAppBlock(socket, handle, 0, buffer, 0xffff);
	
	if (result >= 0) {
		HArrayOfByte * a;
		len = result;
		a = getByteArray(len);
		memcpy(unhand(a)->body, buffer, len);
		output = (HPdapilot_AppBlock*)execute_java_dynamic_method(0, (HObject*)dbClass, 
			"newAppBlock", "([B)LPdapilot/AppBlock;",
			a);
	} else if (result != -5) {
		SignalError(0, "Pdapilot/DlpException", 0);
	}
	
	free(buffer);
	
	return output;
}

extern HPdapilot_Pref * Pdapilot_calls_dlp_ReadAppPreference(struct HPdapilot_calls * self, long socket, HPdapilot_Char4 * creator, long id, long backup, HPdapilot_Database * dbClass)
{
	char * buffer = malloc(0xffff);
	int len, version;
	HPdapilot_Pref * output = NULL;
	
	int result = dlp_ReadAppPreference(socket, getJavaChar4(creator), id, backup, 0xffff, buffer, &len, &version);
	
	if (result >= 0) {
		HArrayOfByte * a;
		a = getByteArray(len);
		memcpy(unhand(a)->body, buffer, len);
		output = (HPdapilot_Pref*)execute_java_dynamic_method(0, (HObject*)dbClass, 
			"newPref", "([BLPdapilot/Char4;IIZ)LPdapilot/Pref;",
			a, creator, id, version, backup);
	} else if (result != -5) {
		SignalError(0, "Pdapilot/DlpException", 0);
	}
	
	free(buffer);
	
	return output;
}

extern long Pdapilot_calls_dlp_ReadFeature(struct HPdapilot_calls * self, long socket, HPdapilot_Char4 * creator, long id)
{
	long feature;
	
	int result = dlp_ReadFeature(socket, getJavaChar4(creator), id, &feature);
	
	if (result < 0)
		SignalError(0, "Pdapilot/DlpException", 0);
	
	return feature;
}

extern HArrayOfObject * Pdapilot_calls_dlp_ReadRecordIDList(struct HPdapilot_calls * self, long socket, long handle, long sort, long start, long max)
{
	recordid_t * l = malloc(sizeof(recordid_t)*max);
	int count;
	int result;
	HArrayOfObject * output = 0;
	int i;
	
	result = dlp_ReadRecordIDList(socket, handle, sort, start, max, l, &count);
	
	if (result < 0)
		SignalError(0, "Pdapilot/DlpException", 0);
	else {
		output = makeRecordIDArray(count);
		if (output) {
			for (i=0;i<count;i++) {
				unhand(output)->body[i] = (HObject*)makeJavaRecordID(l[i]);
			}
		}
	}
		
	free(l);
	
	return output;
}

extern HPdapilot_SortBlock * Pdapilot_calls_dlp_ReadSortBlock(struct HPdapilot_calls * self, long socket, long handle, HPdapilot_Database * dbClass)
{
	char * buffer = malloc(0xffff);
	int len;
	HPdapilot_SortBlock * output = NULL;
	
	int result = dlp_ReadSortBlock(socket, handle, 0, buffer, 0xffff);
	
	if (result >= 0) {
		HArrayOfByte * a;
		len = result;
		a = getByteArray(len);
		memcpy(unhand(a)->body, buffer, len);
		output = (HPdapilot_SortBlock*)execute_java_dynamic_method(0, (HObject*)dbClass, 
			"newSortBlock", "([B)LPdapilot/SortBlock;",
			a);
	} else if (result != -5) {
		SignalError(0, "Pdapilot/DlpException", 0);
	}
	
	free(buffer);
	
	return output;
}

extern HPdapilot_Record * Pdapilot_calls_dlp_ReadRecordByID(struct HPdapilot_calls * self, long socket, long handle, HPdapilot_RecordID * id, HPdapilot_Database * dbClass)
{
	int attr, cat;
	char * buffer = malloc(0xffff);
	int len;
	int index;
	HPdapilot_Record * output = NULL;
	
	int result = dlp_ReadRecordById(socket, handle, getJavaRecordID(id), buffer, &index, &len, &attr, &cat);
	
	if (result >= 0) {
		HArrayOfByte * a = getByteArray(len);
		memcpy(unhand(a)->body, buffer, len);
		output = (HPdapilot_Record*)execute_java_dynamic_method(0, (HObject*)dbClass, 
			"newRecord", "([BLPdapilot/RecordID;III)LPdapilot/Record;",
			a, id, (long)index, (long)attr, (long)cat);
	} else if (result != -5) {
		SignalError(0, "Pdapilot/DlpException", 0);
	}
	
	free(buffer);
	
	return output;
}

extern HPdapilot_Resource * Pdapilot_calls_dlp_ReadResourceByType(struct HPdapilot_calls * self, long socket, long handle, HPdapilot_Char4 * type, long id, HPdapilot_Database * dbClass)
{
	char * buffer = malloc(0xffff);
	int len;
	int index;
	HPdapilot_Resource * output = NULL;
	
	int result = dlp_ReadResourceByType(socket, handle, unhand(type)->value, id, buffer, &index, &len);
	
	if (result >= 0) {
		HArrayOfByte * a = getByteArray(len);
		memcpy(unhand(a)->body, buffer, len);
		output = (HPdapilot_Resource*)execute_java_dynamic_method(0, (HObject*)dbClass, 
			"newResource", "([BLPdapilot/Char4;II)LPdapilot/Resource;",
			a, type, (long)id, (long)index);
	} else if (result != -5) {
		SignalError(0, "Pdapilot/DlpException", 0);
	}
	
	free(buffer);
	
	return output;
}

extern HPdapilot_Resource * Pdapilot_calls_dlp_ReadResourceByIndex(struct HPdapilot_calls * self, long socket, long handle, long index, HPdapilot_Database * dbClass)
{
	long type;
	char * buffer = malloc(0xffff);
	int len, id;
	HPdapilot_Resource * output = NULL;
	
	int result = dlp_ReadResourceByIndex(socket, handle, index, buffer, &type, &id, &len);
	
	if (result >= 0) {
		HArrayOfByte * a = getByteArray(len);

		memcpy(unhand(a)->body, buffer, len);
		output = (HPdapilot_Resource*)execute_java_dynamic_method(0, (HObject*)dbClass, 
			"newResource", "([BLPdapilot/Char4;II)LPdapilot/Resource;",
			a, makeJavaChar4(type), (long)id, (long)index);
	} else if (result != -5) {
		SignalError(0, "Pdapilot/DlpException", 0);
	}
	
	free(buffer);
	
	return output;
}

extern long Pdapilot_calls_dlp_WriteRecord(struct HPdapilot_calls * self, long socket, long handle, HPdapilot_Record * record)
{
	int attr, cat;
	HArrayOfByte * b;
	char * buffer;
	int len;
	recordid_t id;
	int result;
	
	b = (HArrayOfByte*)execute_java_dynamic_method(0, (HObject*)record, "pack", "()[B");
	if (!b)
		return 0;
	
	id = getJavaRecordID(unhand(record)->id);
	attr = unhand(record)->attr;
	cat = unhand(record)->cat;
	buffer = unhand(b)->body;
	len = getArrayLength(b);
	
	result = dlp_WriteRecord(socket, handle, attr, id, cat, buffer, len, &id);
	
	if (result >= 0) {
		return id;
	} else  {
		SignalError(0, "Pdapilot/DlpException", 0);
	}
	
	return 0;
}

extern long Pdapilot_calls_dlp_WriteAppPreference(struct HPdapilot_calls * self, long socket, HPdapilot_Pref * pref)
{
	HArrayOfByte * b;
	char * buffer;
	int len;
	long creator;
	int id, version;
	int result;
	int backup;
	
	b = (HArrayOfByte*)execute_java_dynamic_method(0, (HObject*)pref, "pack", "()[B");
	if (!b)
		return 0;
	
	creator = getJavaChar4(unhand(pref)->creator);
	id = unhand(pref)->id;
	version = unhand(pref)->version;
	backup = unhand(pref)->backup;
	buffer = unhand(b)->body;
	len = getArrayLength(b);
	
	result = dlp_WriteAppPreference(socket, creator, id, backup, version, buffer, len);
	
	if (result < 0)
		SignalError(0, "Pdapilot/DlpException", 0);
	
	return result;
}

extern long Pdapilot_calls_dlp_WriteAppBlock(struct HPdapilot_calls * self, long socket, long handle, HPdapilot_AppBlock * appblock)
{
	HArrayOfByte * b;
	char * buffer;
	int len;
	int result;
	
	b = (HArrayOfByte*)execute_java_dynamic_method(0, (HObject*)appblock, "pack", "()[B");
	if (!b)
		return 0;
	
	buffer = unhand(b)->body;
	len = getArrayLength(b);
	
	result = dlp_WriteAppBlock(socket, handle, buffer, len);
	
	if (result < 0)
		SignalError(0, "Pdapilot/DlpException", 0);
	
	return result;
}

extern long Pdapilot_calls_dlp_WriteSortBlock(struct HPdapilot_calls * self, long socket, long handle, HPdapilot_SortBlock * sortblock)
{
	HArrayOfByte * b;
	char * buffer;
	int len;
	int result;
	
	b = (HArrayOfByte*)execute_java_dynamic_method(0, (HObject*)sortblock, "pack", "()[B");
	if (!b)
		return 0;
	
	buffer = unhand(b)->body;
	len = getArrayLength(b);
	
	result = dlp_WriteSortBlock(socket, handle, buffer, len);
	
	if (result < 0)
		SignalError(0, "Pdapilot/DlpException", 0);
	
	return result;
}

extern long Pdapilot_calls_dlp_WriteResource(struct HPdapilot_calls * self, long socket, long handle, HPdapilot_Resource * resource)
{
	char * buffer;
	HArrayOfByte * b;
	int len;
	long type;
	long id;
	int result;

	b = (HArrayOfByte*)execute_java_dynamic_method(0, (HObject*)resource, "pack", "()[B");
	if (!b)
		return 0;
	
	type = unhand(unhand(resource)->type)->value;
	id = unhand(resource)->id;
	buffer = unhand(b)->body;
	len = getArrayLength(b);
	
	result = dlp_WriteResource(socket, handle, type, id, buffer, len);
	
	if (result < 0)
		SignalError(0, "Pdapilot/DlpException", 0);
	
	return result;
}


extern void Pdapilot_memo_Record_unpack(struct HPdapilot_memo_Record * self,  struct HArrayOfByte *b)
{
	struct Memo m;
	unpack_Memo(&m, unhand(b)->body, getArrayLength(b));
	
	unhand(self)->raw = b;
	
	unhand(self)->text = m.text ? makeJavaString(m.text, strlen(m.text)) : 0;
	free_Memo(&m);
}

extern HArrayOfByte* Pdapilot_memo_Record_pack(struct HPdapilot_memo_Record * self){
	char * buffer = malloc(0xffff);
	int len;
	HArrayOfByte * output;
	
	struct Memo m;
	
	m.text = makeCString(unhand(self)->text);
	pack_Memo(&m, buffer, &len);
	
	output = getByteArray(len);
	memcpy(unhand(output)->body, buffer, len);
	
	unhand(self)->raw = output;
	
	free(buffer);
	
	return output;
}

extern void Pdapilot_memo_AppBlock_unpack(struct HPdapilot_memo_AppBlock * self,  struct HArrayOfByte *b)
{
	struct MemoAppInfo m;
	int i;

	unpack_MemoAppInfo(&m, unhand(b)->body, getArrayLength(b));
	
	unhand(self)->raw = b;
	
	unhand(self)->renamedCategories = m.renamedcategories;
	unhand(self)->lastUniqueID = m.lastUniqueID;
	unhand(self)->sorted = m.sortOrder;
	if (!(unhand(self)->categoryName = makeStringArray(16)))
		return;
	if (!(unhand(self)->categoryID = makeIntArray(16)))
		return;
	
	for (i=0;i<16;i++) {
		unhand(unhand(self)->categoryName)->body[i] = (HObject*)makeJavaString(m.CategoryName[i], strlen(m.CategoryName[i]));
		unhand(unhand(self)->categoryID)->body[i] = m.CategoryID[i];
	}
}

extern HArrayOfByte* Pdapilot_memo_AppBlock_pack(struct HPdapilot_memo_AppBlock * self){
	char * buffer = malloc(0xffff);
	int len;
	HArrayOfByte * output;
	int i;
	
	struct MemoAppInfo m;
	
	m.renamedcategories = unhand(self)->renamedCategories;
	m.lastUniqueID = unhand(self)->lastUniqueID;
	m.sortOrder = unhand(self)->sorted;

	for (i=0;i<16;i++) {
		javaString2CString((HString*)unhand(unhand(self)->categoryName)->body[i], m.CategoryName[i], 16);
		m.CategoryID[i] = unhand(unhand(self)->categoryID)->body[i];
	}
	
	pack_MemoAppInfo(&m, buffer, &len);
	
	output = getByteArray(len);
	memcpy(unhand(output)->body, buffer, len);
	
	unhand(self)->raw = output;
	
	free(buffer);
	
	return output;
}

