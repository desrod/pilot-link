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
#include "Pdapilot_CategoryAppBlock.h"
#include "Pdapilot_memo_Record.h"
#include "Pdapilot_memo_AppBlock.h"
#include "Pdapilot_todo_Record.h"
#include "Pdapilot_todo_AppBlock.h"
#include "Pdapilot_address_Record.h"
#include "Pdapilot_address_AppBlock.h"
#include "Pdapilot_appointment_Record.h"
#include "Pdapilot_appointment_AppBlock.h"
#include "Pdapilot_mail_Record.h"
#include "Pdapilot_mail_AppBlock.h"
#include "Pdapilot_mail_SyncPref.h"
#include "Pdapilot_mail_SignaturePref.h"
#include "Pdapilot_RecordID.h"
#include "Pdapilot_Char4.h"
#include "Pdapilot_DBInfo.h"
#include "java_util_Date.h"

#include "pi-source.h"

#include "pi-dlp.h"
#include "pi-socket.h"
#include "pi-memo.h"
#include "pi-address.h"
#include "pi-datebook.h"
#include "pi-todo.h"
#include "pi-mail.h"

#include <signal.h>

/** Sun should be ashamed for the 1.0 JNI!
 */

static void throwDlpException(int code)
{
	execute_java_static_method(0, FindClass(0, "Pdapilot/DlpException",1),
		"kickWillyScuggins", "(I)V", (long)code);
}

static void throwCancelSyncException()
{
	execute_java_static_method(0, FindClass(0, "Pdapilot/CancelSyncException",1),
		"kickWillyScuggins", "()V");
}

static void throwIOException(int code)
{
	SignalError(0, "java/lang/IOException", 0);
}
 
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

static HArrayOfObject * makeDateArray(long length)
{
	return (HArrayOfObject*)execute_java_static_method(0, FindClass(0, "Pdapilot/calls",1),
		"makeDateArray", "(I)[Ljava/lang/Object;", (long)length);
}

static HArrayOfInt * makeIntArray(long length)
{
	return (HArrayOfInt*)execute_java_static_method(0, FindClass(0, "Pdapilot/calls",1),
		"makeIntArray", "(I)[I", (long)length);
}

/*static HObject* makeBooleanArray(long length) Ptui!
{
	return (HObject*)execute_java_static_method(0, FindClass(0, "Pdapilot/calls",1),
		"makeBooleanArray", "(I)[Z", (long)length);
}*/

static long getArrayLength(struct HArrayOfByte *b)
{
	return (long)execute_java_static_method(0, FindClass(0, "Pdapilot/calls",1),
		"getArrayLength", "([B)I", b);
}

static long getObjectArrayLength(struct HArrayOfObject *b)
{
	return (long)execute_java_static_method(0, FindClass(0, "Pdapilot/calls",1),
		"getObjectArrayLength", "([Ljava/lang/Object;)I", b);
}

Hjava_util_Date * makeJavaDate(time_t v) {
	if (v < 18000) {
		return 0;
	} else {
		struct tm * tm = localtime(&v);
		return (Hjava_util_Date*)execute_java_constructor(0, "java/util/Date", 0, 
			"(IIIIII)",
			tm->tm_year, tm->tm_mon, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec);
	}
	
}	

Hjava_util_Date * makeJavaDateTm(struct tm * tm) {
	return (Hjava_util_Date*)execute_java_constructor(0, "java/util/Date", 0, 
		"(IIIIII)",
		tm->tm_year, tm->tm_mon, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);
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

	tm.tm_year = (int)execute_java_dynamic_method(0, (HObject*)date, "getYear", "()I");
	tm.tm_mon = (int)execute_java_dynamic_method(0, (HObject*)date, "getMonth", "()I");
	tm.tm_mday = (int)execute_java_dynamic_method(0, (HObject*)date, "getDay", "()I");
	tm.tm_hour = (int)execute_java_dynamic_method(0, (HObject*)date, "getHours", "()I");
	tm.tm_min = (int)execute_java_dynamic_method(0, (HObject*)date, "getMinutes", "()I");
	tm.tm_sec = (int)execute_java_dynamic_method(0, (HObject*)date, "getSeconds", "()I");
	
	return mktime(&tm);
}

struct tm * readJavaDateTm(Hjava_util_Date * date) {
	static struct tm tm;
	
	tm.tm_year = (int)execute_java_dynamic_method(0, (HObject*)date, "getYear", "()I");
	tm.tm_mon = (int)execute_java_dynamic_method(0, (HObject*)date, "getMonth", "()I");
	tm.tm_mday = (int)execute_java_dynamic_method(0, (HObject*)date, "getDay", "()I");
	tm.tm_hour = (int)execute_java_dynamic_method(0, (HObject*)date, "getHours", "()I");
	tm.tm_min = (int)execute_java_dynamic_method(0, (HObject*)date, "getMinutes", "()I");
	tm.tm_sec = (int)execute_java_dynamic_method(0, (HObject*)date, "getSeconds", "()I");
	
	return &tm;
}

extern long Pdapilot_calls_pi_socket(struct HPdapilot_calls*self, long domain, long type, long protocol)
{
	int result = pi_socket(domain, type, protocol);
	if (result<0)
		throwIOException(result);
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
		throwIOException(result);
	return result;
}

extern long Pdapilot_calls_pi_listen(struct HPdapilot_calls*self, long socket, long backlog)
{
	int result = pi_listen(socket, backlog);
	if (result<0)
		throwIOException(result);
	return result;
	
}

extern long Pdapilot_calls_pi_accept(struct HPdapilot_calls*self, long socket)
{
	int result=pi_accept(socket, 0, 0);
	if (result<0)
		throwIOException(result);
	return result;
}

extern long Pdapilot_calls_pi_read(struct HPdapilot_calls*self, long socket, HArrayOfByte *b, long len)
{
	int result=pi_read(socket, unhand(b)->body, len);
	if (result<0)
		throwIOException(result);
	return result;
}

extern long Pdapilot_calls_pi_write(struct HPdapilot_calls*self, long socket, HArrayOfByte * b, long len)
{
	int result=pi_write(socket, unhand(b)->body, len);
	if (result<0)
		throwIOException(result);
	return result;
}

extern long Pdapilot_calls_pi_close(struct HPdapilot_calls * self, long socket)
{
	int result=pi_close(socket);
	if (result<0)
		throwIOException(result);
	return result;
}

extern long Pdapilot_calls_pi_version(struct HPdapilot_calls * self, long socket)
{
	int result=pi_version(socket);
	if (result<0)
		throwIOException(result);
	return result;
}

extern long Pdapilot_calls_pi_watchdog(struct HPdapilot_calls * self, long socket, long interval)
{
	int result=pi_watchdog(socket, interval);
	if (result<0)
		throwIOException(result);
	return result;
}

extern long Pdapilot_calls_pi_tickle(struct HPdapilot_calls * self, long socket)
{
	int result=pi_tickle(socket);
	if (result<0)
		throwIOException(result);
	return result;
}

extern long Pdapilot_calls_dlp_OpenDB(struct HPdapilot_calls * self, long socket, long card, long mode, Hjava_lang_String * name)
{
	int handle;
	int result = dlp_OpenDB(socket, card, mode, makeCString(name), &handle);
	if (result < 0) {
		throwDlpException(result);
		return result;
	}
	return handle;
}

extern long Pdapilot_calls_dlp_CreateDB(struct HPdapilot_calls * self, long socket, HPdapilot_Char4 * creator, HPdapilot_Char4 * type, long card, long flags, long version, Hjava_lang_String * name)
{
	int handle;
	int result = dlp_CreateDB(socket, unhand(creator)->value, unhand(creator)->value, card, flags, version, makeCString(name), &handle);
	if (result < 0) {
		throwDlpException(result);
		return result;
	}
	return handle;
}

extern long Pdapilot_calls_dlp_DeleteDB(struct HPdapilot_calls * self, long socket, long card, Hjava_lang_String * name)
{
	int result = dlp_DeleteDB(socket, card, makeCString(name));
	if (result < 0) {
		throwDlpException(result);
	}
	return result;
}

extern Hjava_util_Date * Pdapilot_calls_dlp_GetSysDateTime(struct HPdapilot_calls * self, long socket)
{
	time_t t;
	int result = dlp_GetSysDateTime(socket, &t);
	if (result < 0) {
		throwDlpException(result);
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
		throwDlpException(result);
	}
	return result;
}

extern long Pdapilot_calls_dlp_AddSyncLogEntry(struct HPdapilot_calls * self, long socket, Hjava_lang_String * entry)
{
	int result = dlp_AddSyncLogEntry(socket, makeCString(entry));
	if (result < 0)
		throwDlpException(result);
	return result;
}

extern long Pdapilot_calls_dlp_ResetSystem(struct HPdapilot_calls * self, long socket)
{
	int result = dlp_ResetSystem(socket);
	if (result < 0)
		throwDlpException(result);
	return result;
}

extern long Pdapilot_calls_dlp_EndOfSync(struct HPdapilot_calls * self, long socket, long status)
{
	int result = dlp_EndOfSync(socket, status);
	if (result < 0)
		throwDlpException(result);
	return result;
}

extern long Pdapilot_calls_dlp_MoveCategory(struct HPdapilot_calls * self, long socket, long handle, long from, long to)
{
	int result = dlp_MoveCategory(socket, handle, from, to);
	if (result < 0)
		throwDlpException(result);
	return result;
}

extern long Pdapilot_calls_dlp_DeleteRecord(struct HPdapilot_calls * self, long socket, long handle, long all, HPdapilot_RecordID * id)
{
	int result = dlp_DeleteRecord(socket, handle, all, getJavaRecordID(id));
	if (result < 0)
		throwDlpException(result);
	return result;
}

extern long Pdapilot_calls_dlp_DeleteCategory(struct HPdapilot_calls * self, long socket, long handle, long category)
{
	int result = dlp_DeleteCategory(socket, handle, category);
	if (result < 0)
		throwDlpException(result);
	return result;
}

extern long Pdapilot_calls_dlp_ReadOpenDBInfo(struct HPdapilot_calls * self, long socket, long handle)
{
	int count;
	int result = dlp_ReadOpenDBInfo(socket, handle, &count);
	if (result < 0)
		throwDlpException(result);
	return count;
}

extern long Pdapilot_calls_dlp_DeleteResource(struct HPdapilot_calls * self, long socket, long handle, long all, HPdapilot_Char4 * type, long id)
{
	int result = dlp_DeleteResource(socket, handle, all, unhand(type)->value, id);
	if (result < 0)
		throwDlpException(result);
	return result;
}

extern long Pdapilot_calls_dlp_ResetSyncFlags(struct HPdapilot_calls * self, long socket, long handle)
{
	int result = dlp_ResetSyncFlags(socket, handle);
	if (result < 0)
		throwDlpException(result);
	return result;
}

extern long Pdapilot_calls_dlp_CleanUpDatabase(struct HPdapilot_calls * self, long socket, long handle)
{
	int result = dlp_CleanUpDatabase(socket, handle);
	if (result < 0)
		throwDlpException(result);
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
		throwDlpException(result);
		return 0;
	}
	output = (HPdapilot_CardInfo*)execute_java_constructor(0,
		 "Pdapilot/CardInfo", 0, "()");
	unhand(output)->name = makeJavaString(c.name, strlen(c.name));
	unhand(output)->manufacturer = makeJavaString(c.manufacturer, strlen(c.manufacturer));
	unhand(output)->card = c.card;
	unhand(output)->romSize = c.romSize;
	unhand(output)->ramSize = c.ramSize;
	unhand(output)->ramFree = c.ramFree;
	
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
		throwDlpException(result);
		return 0;
	}
	output = (HPdapilot_SysInfo*)execute_java_constructor(0,
		 "Pdapilot/SysInfo", 0, "()");
	unhand(output)->romVersion = s.romVersion;
	unhand(output)->locale = s.locale;
	unhand(output)->name = makeJavaString(s.name, s.nameLength);

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
		throwDlpException(result);
		return 0;
	}
	
	output = (HPdapilot_DBInfo*)execute_java_constructor(0,
		 "Pdapilot/DBInfo", 0, "()");
		 
	unhand(output)->flags = i.flags;
	unhand(output)->index = i.index;
	unhand(output)->version = i.version;
	unhand(output)->modnum = i.modnum;
	unhand(output)->miscFlags = i.miscFlags;
	unhand(output)->type = makeJavaChar4(i.type);
	unhand(output)->creator = makeJavaChar4(i.creator);
	unhand(output)->createDate = makeJavaDate(i.createDate);
	unhand(output)->modifyDate = makeJavaDate(i.modifyDate);
	unhand(output)->backupDate = makeJavaDate(i.backupDate);
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
		throwDlpException(result);
		return 0;
	}
	output = (HPdapilot_UserInfo*)execute_java_constructor(0,
		 "Pdapilot/UserInfo", 0, "()");
	
	unhand(output)->username = makeJavaString(u.username, strlen(u.username));
	unhand(output)->userID = u.userID;
	unhand(output)->viewerID = u.viewerID;
	unhand(output)->lastSyncPC = u.lastSyncPC;
	unhand(output)->password= getByteArray(u.passwordLength);
	memcpy(unhand(unhand(output)->password)->body, u.password, u.passwordLength);
	
	unhand(output)->successfulSyncDate = makeJavaDate(u.successfulSyncDate);
	unhand(output)->lastSyncDate = makeJavaDate(u.lastSyncDate);

	return output;
}

extern long Pdapilot_calls_dlp_WriteUserInfo(struct HPdapilot_calls * self, long socket, HPdapilot_UserInfo * user)
{
	struct PilotUser u;
	int result;
	
	u.userID = unhand(user)->userID;
	u.viewerID = unhand(user)->viewerID;
	u.lastSyncPC = unhand(user)->lastSyncPC;
	u.passwordLength = getArrayLength(unhand(user)->password);
	memcpy(u.password, unhand(unhand(user)->password)->body, u.passwordLength);
	u.successfulSyncDate = readJavaDate(unhand(user)->successfulSyncDate);
	u.lastSyncDate = readJavaDate(unhand(user)->lastSyncDate);
	javaString2CString(unhand(user)->username, u.username, 127);
	
	result = dlp_WriteUserInfo(socket, &u);
	if (result < 0) {
		throwDlpException(result);
	}
	return result;
}


extern HPdapilot_NetInfo * Pdapilot_calls_dlp_ReadNetSyncInfo(struct HPdapilot_calls * self, long socket)
{
	HPdapilot_NetInfo * output = NULL;
	struct NetSyncInfo i;
	int result = dlp_ReadNetSyncInfo(socket, &i);
	if (result < 0) {
		throwDlpException(result);
		return 0;
	}
	output = (HPdapilot_NetInfo*)execute_java_constructor(0,
		 "Pdapilot/NetInfo", 0, "()");
		 
	unhand(output)->lanSync = i.lanSync;
	unhand(output)->hostName = makeJavaString(i.hostName, strlen(i.hostName));
	unhand(output)->hostAddress = makeJavaString(i.hostAddress, strlen(i.hostAddress));
	unhand(output)->hostSubnetMask = makeJavaString(i.hostSubnetMask, strlen(i.hostSubnetMask));
	
	return output;
}

extern long Pdapilot_calls_dlp_WriteNetSyncInfo(struct HPdapilot_calls * self, long socket, HPdapilot_NetInfo * info)
{
	struct NetSyncInfo i;
	int result;
	
	i.lanSync = unhand(info)->lanSync;
	javaString2CString(unhand(info)->hostName, i.hostName, 256);
	javaString2CString(unhand(info)->hostAddress, i.hostAddress, 40);
	javaString2CString(unhand(info)->hostSubnetMask, i.hostSubnetMask, 40);
	
	result = dlp_WriteNetSyncInfo(socket, &i);
	if (result < 0) {
		throwDlpException(result);
	}
	return result;
}

extern long Pdapilot_calls_dlp_OpenConduit(struct HPdapilot_calls * self, long socket)
{
	int result = dlp_OpenConduit(socket);
	if (result == dlpErrSync)
		throwCancelSyncException();
	else if (result < 0)
		throwDlpException(result);
	return result;
}

extern long Pdapilot_calls_dlp_CloseDB(struct HPdapilot_calls * self, long socket, long handle)
{
	int result;
	result = dlp_CloseDB(socket, handle);
	if (result<0)
		throwDlpException(result);
	return result;
}

extern long Pdapilot_calls_dlp_ResetDBIndex(struct HPdapilot_calls * self, long socket, long handle)
{
	int result;
	result = dlp_ResetDBIndex(socket, handle);
	if (result<0)
		throwDlpException(result);
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
		throwDlpException(result);
	}
	
	free(buffer);
	
	return output;
}

extern HArrayOfByte * Pdapilot_calls_dlp_CallApplication(struct HPdapilot_calls * self, long socket, HPdapilot_Char4 * creator, long type, long action, HArrayOfByte * outgoing_data, HArrayOfInt * retcode)
{
	char * buffer = malloc(0xffff);
	int len;
	unsigned long ret;
	HArrayOfByte * incoming_data = 0;
		
	int result = dlp_CallApplication(socket, getJavaChar4(creator), type, action, getArrayLength(outgoing_data), unhand(outgoing_data)->body, &ret, 0xffff, &len, buffer);
	
	if (result >= 0) {
		incoming_data = getByteArray(len);
		memcpy(unhand(incoming_data)->body, buffer, len);
		unhand(retcode)->body[0] = ret;
	} else if (result != -5) {
		throwDlpException(result);
	}
	
	free(buffer);
	
	return incoming_data;
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
		throwDlpException(result);
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
		throwDlpException(result);
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
		throwDlpException(result);
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
		throwDlpException(result);
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
		throwDlpException(result);
	}
	
	free(buffer);
	
	return output;
}

extern long Pdapilot_calls_dlp_ReadFeature(struct HPdapilot_calls * self, long socket, HPdapilot_Char4 * creator, long id)
{
	long feature;
	
	int result = dlp_ReadFeature(socket, getJavaChar4(creator), id, &feature);
	
	if (result < 0)
		throwDlpException(result);
	
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
		throwDlpException(result);
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
		throwDlpException(result);
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
		throwDlpException(result);
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
		throwDlpException(result);
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
		throwDlpException(result);
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
	cat = unhand(record)->category;
	buffer = unhand(b)->body;
	len = getArrayLength(b);
	
	result = dlp_WriteRecord(socket, handle, attr, id, cat, buffer, len, &id);
	
	if (result >= 0) {
		return id;
	} else  {
		throwDlpException(result);
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
		throwDlpException(result);
	
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
		throwDlpException(result);
	
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
		throwDlpException(result);
	
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
		throwDlpException(result);
	
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
	len = pack_Memo(&m, buffer, 0xffff);
	
	output = getByteArray(len);
	memcpy(unhand(output)->body, buffer, len);
	
	unhand(self)->raw = output;
	
	free(buffer);
	
	return output;
}

static void doUnpackCategories(HPdapilot_CategoryAppBlock * self, struct CategoryAppInfo * c)
{
	int i;
	unhand(self)->categoryLastUniqueID = c->lastUniqueID;
	if (!(unhand(self)->categoryName = makeStringArray(16)))
		return;
	if (!(unhand(self)->categoryID = makeIntArray(16)))
		return;
	if (!(unhand(self)->categoryRenamed = makeIntArray(16)))
		return;
	
	for (i=0;i<16;i++) {
		unhand(unhand(self)->categoryName)->body[i] = (HObject*)makeJavaString(c->name[i], strlen(c->name[i]));
		unhand(unhand(self)->categoryID)->body[i] = c->ID[i];
		unhand(unhand(self)->categoryRenamed)->body[i] = c->renamed[i];
	}
}

static void doPackCategories(HPdapilot_CategoryAppBlock * self, struct CategoryAppInfo * c)
{
	int i;
	c->lastUniqueID = unhand(self)->categoryLastUniqueID;

	for (i=0;i<16;i++) {
		javaString2CString((HString*)unhand(unhand(self)->categoryName)->body[i], c->name[i], 16);
		c->ID[i] = unhand(unhand(self)->categoryID)->body[i];
		c->renamed[i] = unhand(unhand(self)->categoryRenamed)->body[i];
	}
}

extern void Pdapilot_memo_AppBlock_unpack(struct HPdapilot_memo_AppBlock * self,  struct HArrayOfByte *b)
{
	struct MemoAppInfo m;

	unpack_MemoAppInfo(&m, unhand(b)->body, getArrayLength(b));
	
	unhand(self)->raw = b;

	doUnpackCategories((HPdapilot_CategoryAppBlock*)self, &m.category);
	
	unhand(self)->sortByAlpha = m.sortByAlpha;
}

extern HArrayOfByte* Pdapilot_memo_AppBlock_pack(struct HPdapilot_memo_AppBlock * self){
	char * buffer = malloc(0xffff);
	int len;
	HArrayOfByte * output;
	
	struct MemoAppInfo m;

	doPackCategories((HPdapilot_CategoryAppBlock*)self, &m.category);
	
	m.sortByAlpha = unhand(self)->sortByAlpha;

	len = pack_MemoAppInfo(&m, buffer, 0xffff);
	
	output = getByteArray(len);
	memcpy(unhand(output)->body, buffer, len);
	
	unhand(self)->raw = output;
	
	free(buffer);
	
	return output;
}


extern void Pdapilot_todo_Record_unpack(struct HPdapilot_todo_Record * self,  struct HArrayOfByte *b)
{
	struct ToDo t;
	unpack_ToDo(&t, unhand(b)->body, getArrayLength(b));
	
	unhand(self)->raw = b;
	
	unhand(self)->due = t.indefinite ? makeJavaDateTm(&t.due) : 0;
	unhand(self)->priority = t.priority;
	unhand(self)->complete = t.complete;
	unhand(self)->description = t.description ? makeJavaString(t.description, strlen(t.description)) : 0;
	unhand(self)->note = t.note ? makeJavaString(t.note, strlen(t.note)) : 0;
	
	free_ToDo(&t);
}


extern HArrayOfByte* Pdapilot_todo_Record_pack(struct HPdapilot_todo_Record * self){
	char * buffer = malloc(0xffff);
	int len;
	HArrayOfByte * output;
	
	struct ToDo t;
	
	struct tm * tm;
	
	t.description = unhand(self)->description ? makeCString(unhand(self)->description) : 0;
	t.note = unhand(self)->note ? makeCString(unhand(self)->note) : 0;
	t.priority = unhand(self)->priority;
	t.complete = unhand(self)->complete;
	t.indefinite = 1;
	if (unhand(self)->due) {
		tm = readJavaDateTm(unhand(self)->due);
		if (tm) {
			t.due = *tm;
			t.indefinite = 0;
		}
	}
	
	len = pack_ToDo(&t, buffer, 0xffff);
	
	output = getByteArray(len);
	memcpy(unhand(output)->body, buffer, len);
	
	unhand(self)->raw = output;
	
	free(buffer);
	
	return output;
}

extern void Pdapilot_todo_AppBlock_unpack(struct HPdapilot_todo_AppBlock * self,  struct HArrayOfByte *b)
{
	struct ToDoAppInfo t;

	unpack_ToDoAppInfo(&t, unhand(b)->body, getArrayLength(b));
	
	unhand(self)->raw = b;

	doUnpackCategories((HPdapilot_CategoryAppBlock*)self, &t.category);
	
	unhand(self)->dirty = t.dirty;
	unhand(self)->sortByPriority = t.sortByPriority;
}

extern HArrayOfByte* Pdapilot_todo_AppBlock_pack(struct HPdapilot_todo_AppBlock * self){
	char * buffer = malloc(0xffff);
	int len;
	HArrayOfByte * output;
	
	struct ToDoAppInfo t;
	
	t.sortByPriority = unhand(self)->sortByPriority;
	t.dirty = unhand(self)->dirty;

	doPackCategories((HPdapilot_CategoryAppBlock*)self, &t.category);
	
	len = pack_ToDoAppInfo(&t, buffer, 0xffff);
	
	output = getByteArray(len);
	memcpy(unhand(output)->body, buffer, len);
	
	unhand(self)->raw = output;
	
	free(buffer);
	
	return output;
}

extern void Pdapilot_CategoryAppBlock_unpack(struct HPdapilot_CategoryAppBlock * self,  struct HArrayOfByte *b)
{
	struct CategoryAppInfo c;

	unpack_CategoryAppInfo(&c, unhand(b)->body, getArrayLength(b));
	
	unhand(self)->raw = b;


	doUnpackCategories(self, &c);
}

extern HArrayOfByte* Pdapilot_CategoryAppBlock_pack(struct HPdapilot_CategoryAppBlock * self){
	char * buffer = malloc(0xffff);
	int len;
	HArrayOfByte * output;
	
	struct CategoryAppInfo c;
	
	doPackCategories(self, &c);
	
	len = pack_CategoryAppInfo(&c, buffer, 0xffff);
	
	output = getByteArray(len);
	memcpy(unhand(output)->body, buffer, len);
	
	unhand(self)->raw = output;
	
	free(buffer);
	
	return output;
}


extern void Pdapilot_address_Record_unpack(struct HPdapilot_address_Record * self,  struct HArrayOfByte *b)
{
	struct Address a;
	int i;
	unpack_Address(&a, unhand(b)->body, getArrayLength(b));
	
	
	unhand(self)->raw = b;
	
	unhand(self)->showPhone = a.showPhone;

	if (!(unhand(self)->entry = makeStringArray(19)))
		return;
	if (!(unhand(self)->phoneLabel = makeIntArray(5)))
		return;

	for (i=0;i<19;i++)
		unhand(unhand(self)->entry)->body[i] = a.entry[i] ? (HObject*)makeJavaString(a.entry[i], strlen(a.entry[i])) : 0;

	for (i=0;i<5;i++)
		unhand(unhand(self)->phoneLabel)->body[i] = a.phoneLabel[i];

	free_Address(&a);
}


extern HArrayOfByte* Pdapilot_address_Record_pack(struct HPdapilot_address_Record * self){
	char * buffer = malloc(0xffff);
	int len;
	int i;
	HArrayOfByte * output;
	
	struct Address a;

	for (i=0;i<19;i++)
		javaString2CString((HString*)unhand(unhand(self)->entry)->body[i], a.entry[i], 16);

	for (i=0;i<5;i++)
		a.phoneLabel[i] = unhand(unhand(self)->phoneLabel)->body[i];
	
	a.showPhone = unhand(self)->showPhone;
	
	len = pack_Address(&a, buffer, 0xffff);
	
	output = getByteArray(len);
	memcpy(unhand(output)->body, buffer, len);
	
	unhand(self)->raw = output;
	
	free(buffer);
	
	return output;
}

extern void Pdapilot_address_AppBlock_unpack(struct HPdapilot_address_AppBlock * self,  struct HArrayOfByte *b)
{
	struct AddressAppInfo a;
	int i;

	unpack_AddressAppInfo(&a, unhand(b)->body, getArrayLength(b));
	
	unhand(self)->raw = b;

	doUnpackCategories((HPdapilot_CategoryAppBlock*)self, &a.category);
	
	if (!(unhand(self)->labelRenamed = makeIntArray(22)))
		return;
	for(i=0;i<22;i++)
		unhand(unhand(self)->labelRenamed)->body[i] = a.labelRenamed[i];
	unhand(self)->sortByCompany = a.sortByCompany;

	if (!(unhand(self)->label = makeStringArray(22)))
		return;
	if (!(unhand(self)->phoneLabel = makeStringArray(8)))
		return;
	
	for (i=0;i<22;i++)
		unhand(unhand(self)->label)->body[i] = (HObject*)makeJavaString(a.labels[i], strlen(a.labels[i]));
	for (i=0;i<8;i++)
		unhand(unhand(self)->phoneLabel)->body[i] = (HObject*)makeJavaString(a.phoneLabels[i], strlen(a.phoneLabels[i]));

}

extern HArrayOfByte* Pdapilot_address_AppBlock_pack(struct HPdapilot_address_AppBlock * self){
	char * buffer = malloc(0xffff);
	int len;
	int i;
	HArrayOfByte * output;
	
	struct AddressAppInfo a;
	
	a.sortByCompany = unhand(self)->sortByCompany;
	for(i=0;i<22;i++)
		a.labelRenamed[i] = unhand(unhand(self)->labelRenamed)->body[i];

	doPackCategories((HPdapilot_CategoryAppBlock*)self, &a.category);

	for (i=0;i<22;i++)
		javaString2CString((HString*)unhand(unhand(self)->label)->body[i], a.labels[i], 16);

	for (i=0;i<8;i++)
		javaString2CString((HString*)unhand(unhand(self)->phoneLabel)->body[i], a.phoneLabels[i], 16);
	
	len = pack_AddressAppInfo(&a, buffer, 0xffff);
	
	output = getByteArray(len);
	memcpy(unhand(output)->body, buffer, len);
	
	unhand(self)->raw = output;
	
	free(buffer);
	
	return output;
}


extern void Pdapilot_appointment_Record_unpack(struct HPdapilot_appointment_Record * self,  struct HArrayOfByte *b)
{
	struct Appointment a;
	int i;
	unpack_Appointment(&a, unhand(b)->body, getArrayLength(b));

	unhand(self)->note = a.note ? makeJavaString(a.note, strlen(a.note)) : 0;
	unhand(self)->description = a.description ? makeJavaString(a.description, strlen(a.description)) : 0;
	
	unhand(self)->event = a.event;
	unhand(self)->begin = makeJavaDateTm(&a.begin);
	unhand(self)->end = makeJavaDateTm(&a.end);

	unhand(self)->alarm = a.alarm;
	unhand(self)->advance = a.advance;
	unhand(self)->advanceUnits = (a.advanceUnits == 0) ? 60 : (a.advanceUnits == 1) ? 60*60 : (a.advanceUnits == 2) ? 60*60*26 : 0;

	if (a.exceptions) {
		if (!(unhand(self)->exceptions = makeDateArray(a.exceptions)))
			return;
		for(i=0;i<a.exceptions;i++)
			unhand(unhand(self)->exceptions)->body[i] = (HObject*)makeJavaDateTm(&a.exception[i]);
	}

	unhand(self)->repeatDay = a.repeatOn;
	unhand(self)->repeatWeekStart = a.repeatWeekstart;
	unhand(self)->repeatType = a.repeatType;
	if (!(unhand(self)->repeatWeekdays = makeIntArray(7)))
		return;
	for(i=0;i<7;i++)
		unhand(unhand(self)->repeatWeekdays)->body[i] = !!(a.repeatOn & (1<<i));

	unhand(self)->repeatEnd = a.repeatForever ? 0 : makeJavaDateTm(&a.repeatEnd);

	free_Appointment(&a);
}

extern HArrayOfByte* Pdapilot_appointment_Record_pack(struct HPdapilot_appointment_Record * self){
	char * buffer = malloc(0xffff);
	int len;
	int i;
	HArrayOfByte * output;
	
	struct Appointment a;

	a.note = unhand(self)->note ? makeCString(unhand(self)->note) : 0;
	a.description = unhand(self)->description ? makeCString(unhand(self)->description) : 0;
	
	a.event = unhand(self)->event;
	a.begin = *readJavaDateTm(unhand(self)->begin);
	a.end = *readJavaDateTm(unhand(self)->end);

	a.alarm = unhand(self)->alarm;
	a.advance = unhand(self)->advance;
	switch (unhand(self)->advanceUnits) {
	case 60:
		a.advanceUnits = 0;
		break;
	case 60*60:
		a.advanceUnits = 1;
		break;
	case 60*60*24:
		a.advanceUnits = 2;
		break;
	/*default:
		throw*/
	}
	
	a.repeatType = unhand(self)->repeatType;
	if (a.repeatType == repeatWeekly) {
		a.repeatOn = 0;
		for(i=0;i<7;i++)
			if (unhand(unhand(self)->repeatWeekdays)->body[i])
				a.repeatOn |= 1<<i;
	} else {
		a.repeatOn = unhand(self)->repeatDay;
	}
	a.repeatWeekstart = unhand(self)->repeatWeekStart;
	
	if (unhand(self)->repeatEnd) {
		a.repeatForever = 0;
		a.repeatEnd = *readJavaDateTm(unhand(self)->repeatEnd);
	} else
		a.repeatForever = 1;
	if (unhand(self)->exceptions) {
		a.exceptions = getObjectArrayLength(unhand(self)->exceptions);
		a.exception = malloc(sizeof(struct tm)*a.exceptions);
		for(i=0;i<a.exceptions;i++)
			a.exception[i] = *readJavaDateTm((Hjava_util_Date*)unhand(unhand(self)->exceptions)->body[i]);
	}
	
	len = pack_Appointment(&a, buffer, 0xffff);
	
	output = getByteArray(len);
	memcpy(unhand(output)->body, buffer, len);
	
	unhand(self)->raw = output;
	
	free(buffer);
	
	return output;
}

extern void Pdapilot_appointment_AppBlock_unpack(struct HPdapilot_appointment_AppBlock * self,  struct HArrayOfByte *b)
{
	struct AppointmentAppInfo a;

	unpack_AppointmentAppInfo(&a, unhand(b)->body, getArrayLength(b));
	
	unhand(self)->raw = b;

	doUnpackCategories((HPdapilot_CategoryAppBlock*)self, &a.category);
	
	unhand(self)->startOfWeek = a.startOfWeek;
}

extern HArrayOfByte* Pdapilot_appointment_AppBlock_pack(struct HPdapilot_appointment_AppBlock * self){
	char * buffer = malloc(0xffff);
	int len;
	HArrayOfByte * output;
	
	struct AppointmentAppInfo a;
	
	a.startOfWeek = unhand(self)->startOfWeek;

	doPackCategories((HPdapilot_CategoryAppBlock*)self, &a.category);

	len = pack_AppointmentAppInfo(&a, buffer, 0xffff);
	
	output = getByteArray(len);
	memcpy(unhand(output)->body, buffer, len);
	
	unhand(self)->raw = output;
	
	free(buffer);
	
	return output;
}

/* mail */
extern void Pdapilot_mail_Record_unpack(struct HPdapilot_mail_Record * self,  struct HArrayOfByte *b)
{
	struct Mail a;
	unpack_Mail(&a, unhand(b)->body, getArrayLength(b));

	if (a.dated)
	  unhand(self)->date = makeJavaDateTm(&a.date);
	else
	  unhand(self)->date = 0;
	
	unhand(self)->read = a.read;
	unhand(self)->signature = a.signature;
	unhand(self)->confirmRead = a.confirmRead;
	unhand(self)->confirmDelivery = a.confirmDelivery;
	unhand(self)->priority = a.priority;
	unhand(self)->addressing = a.addressing;
	
	unhand(self)->subject = a.subject ? makeJavaString(a.subject, strlen(a.subject)) : 0;
	unhand(self)->from = a.from ? makeJavaString(a.from, strlen(a.from)) : 0;
	unhand(self)->to = a.to ? makeJavaString(a.to, strlen(a.to)) : 0;
	unhand(self)->cc = a.cc ? makeJavaString(a.cc, strlen(a.cc)) : 0;
	unhand(self)->bcc = a.bcc ? makeJavaString(a.bcc, strlen(a.bcc)) : 0;
	unhand(self)->replyTo = a.replyTo ? makeJavaString(a.replyTo, strlen(a.replyTo)) : 0;
	unhand(self)->sentTo = a.sentTo ? makeJavaString(a.sentTo, strlen(a.sentTo)) : 0;
	unhand(self)->body = a.body ? makeJavaString(a.body, strlen(a.body)) : 0;
	
	free_Mail(&a);
}


extern HArrayOfByte* Pdapilot_mail_Record_pack(struct HPdapilot_mail_Record * self){
	char * buffer = malloc(0xffff);
	int len;
	HArrayOfByte * output;
	
	struct Mail a;

	a.dated = unhand(self)->date ? 1 : 0;
	if (a.dated)
	  a.date = *readJavaDateTm(unhand(self)->date);
	  
	a.read = unhand(self)->read;
	a.signature = unhand(self)->signature;
	a.confirmRead = unhand(self)->confirmRead;
	a.confirmDelivery = unhand(self)->confirmDelivery;
	a.priority = unhand(self)->priority;
	a.addressing = unhand(self)->addressing;
	
	a.subject = unhand(self)->subject ? makeCString(unhand(self)->subject) : 0;
	a.from = unhand(self)->from ? makeCString(unhand(self)->from) : 0;
	a.to = unhand(self)->to ? makeCString(unhand(self)->to) : 0;
	a.cc = unhand(self)->cc ? makeCString(unhand(self)->cc) : 0;
	a.bcc = unhand(self)->bcc ? makeCString(unhand(self)->bcc) : 0;
	a.replyTo = unhand(self)->replyTo ? makeCString(unhand(self)->replyTo) : 0;
	a.sentTo = unhand(self)->sentTo ? makeCString(unhand(self)->sentTo) : 0;
	a.body = unhand(self)->body ? makeCString(unhand(self)->body) : 0;
	
	len = pack_Mail(&a, buffer, 0xffff);
	
	output = getByteArray(len);
	memcpy(unhand(output)->body, buffer, len);
	
	unhand(self)->raw = output;
	
	free(buffer);
	
	return output;
}

extern void Pdapilot_mail_AppBlock_unpack(struct HPdapilot_mail_AppBlock * self,  struct HArrayOfByte *b)
{
	struct MailAppInfo a;

	unpack_MailAppInfo(&a, unhand(b)->body, getArrayLength(b));
	
	unhand(self)->raw = b;

	doUnpackCategories((HPdapilot_CategoryAppBlock*)self, &a.category);
	
	unhand(self)->dirty = a.dirty;
	unhand(self)->sortOrder = a.sortOrder;
	unhand(self)->unsentMessage = a.unsentMessage ? makeJavaRecordID(a.unsentMessage) : 0;

}

extern HArrayOfByte* Pdapilot_mail_AppBlock_pack(struct HPdapilot_mail_AppBlock * self){
	char * buffer = malloc(0xffff);
	int len;
	HArrayOfByte * output;
	
	struct MailAppInfo a;
	
	a.sortOrder = unhand(self)->sortOrder;
	a.dirty = unhand(self)->dirty;
	a.unsentMessage = unhand(self)->unsentMessage ? getJavaRecordID(unhand(self)->unsentMessage) : 0;
	
	len = pack_MailAppInfo(&a, buffer, 0xffff);
	
	output = getByteArray(len);
	memcpy(unhand(output)->body, buffer, len);
	
	unhand(self)->raw = output;
	
	free(buffer);
	
	return output;
}

extern void Pdapilot_mail_SyncPref_unpack(struct HPdapilot_mail_SyncPref * self,  struct HArrayOfByte *b)
{
	struct MailSyncPref a;

	unpack_MailSyncPref(&a, unhand(b)->body, getArrayLength(b));
	
	unhand(self)->raw = b;
	
	unhand(self)->syncType = a.syncType;
	unhand(self)->getHigh = a.getHigh;
	unhand(self)->getContaining = a.getContaining;
	unhand(self)->truncate = a.truncate;
	unhand(self)->filterTo = a.filterTo ? makeJavaString(a.filterTo, strlen(a.filterTo)) : 0;
	unhand(self)->filterFrom = a.filterFrom ? makeJavaString(a.filterFrom, strlen(a.filterFrom)) : 0;
	unhand(self)->filterSubject = a.filterSubject ? makeJavaString(a.filterSubject, strlen(a.filterSubject)) : 0;

}

extern HArrayOfByte* Pdapilot_mail_SyncPref_pack(struct HPdapilot_mail_SyncPref * self){
	char * buffer = malloc(0xffff);
	int len;
	HArrayOfByte * output;
	
	struct MailSyncPref a;
	
	a.syncType = unhand(self)->syncType;
	a.getHigh = unhand(self)->getHigh;
	a.getContaining = unhand(self)->getContaining;
	a.truncate = unhand(self)->truncate;
	a.filterTo = unhand(self)->filterTo ? makeCString(unhand(self)->filterTo) : 0;
	a.filterFrom = unhand(self)->filterFrom ? makeCString(unhand(self)->filterFrom) : 0;
	a.filterSubject = unhand(self)->filterSubject ? makeCString(unhand(self)->filterSubject) : 0;
	
	len = pack_MailSyncPref(&a, buffer, 0xffff);
	
	output = getByteArray(len);
	memcpy(unhand(output)->body, buffer, len);
	
	unhand(self)->raw = output;
	
	free(buffer);
	
	return output;
}

extern void Pdapilot_mail_SignaturePref_unpack(struct HPdapilot_mail_SignaturePref * self,  struct HArrayOfByte *b)
{
	struct MailSignaturePref a;

	unpack_MailSignaturePref(&a, unhand(b)->body, getArrayLength(b));
	
	unhand(self)->raw = b;

	unhand(self)->signature = a.signature ? makeJavaString(a.signature, strlen(a.signature)) : 0;

}

extern HArrayOfByte* Pdapilot_mail_SignaturePref_pack(struct HPdapilot_mail_SignaturePref * self){
	char * buffer = malloc(0xffff);
	int len;
	HArrayOfByte * output;
	
	struct MailSignaturePref a;
	
	a.signature = unhand(self)->signature ? makeCString(unhand(self)->signature) : 0;

	len = pack_MailSignaturePref(&a, buffer, 0xffff);
	
	output = getByteArray(len);
	memcpy(unhand(output)->body, buffer, len);
	
	unhand(self)->raw = output;
	
	free(buffer);
	
	return output;
}
