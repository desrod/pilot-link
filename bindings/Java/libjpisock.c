#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <utime.h>
#include <stdio.h>
#include <ctype.h>

#include <stdarg.h>

#include <pi-source.h>
#include <pi-socket.h>
#include <pi-dlp.h>
#include <pi-file.h>
#include <pi-memo.h>
#include "libjpisock.h"

#define MAX_RESOURCE_SIZE 0xffff

int pilot_connect(JNIEnv * env, const char *port);
static void postPilotLinkException(JNIEnv *, const char *, int, int);
static void postJavaException(JNIEnv *, const char *, const char *);
static int getBasicTypeField(JNIEnv * env, jclass pClass, jobject pObject,
    const char * sFieldType, const char * sFieldName, void * pLocation);
static int assignBasicTypeField(JNIEnv * env, jclass pClass, jobject pObject,
    const char * sFieldType, const char * sFieldName, ...);
/*
    PilotLink.connect
*/

JNIEXPORT jint JNICALL Java_org_gnu_pilotlink_PilotLink_connect
   (JNIEnv *env, jobject obj, jstring jprt) {

    jint iResult = 0;
    const char * port = NULL;
    char * prt = NULL;

    /* Get working copy of port name */
    port = env->GetStringUTFChars(jprt, NULL);
    prt = (char *)malloc(strlen(port) + 1);
    if (prt != NULL) strcpy(prt, port);
    env->ReleaseStringUTFChars(jprt, port);

    iResult = pilot_connect(env, prt);
    if (prt != NULL) free(prt);
    
    
    return iResult;
}

/*
ReadAppInfo
*/
JNIEXPORT jobject JNICALL Java_org_gnu_pilotlink_PilotLink_readAppInfo
  (JNIEnv *env, jobject obj, jint handle, jint db)
{

    jclass      jClass_appInfo = NULL;
    jobject     jObject_appInfo = NULL;
    jmethodID   jMethod_appInfo = NULL;
    char pBuffer[0xffff];
    int bProceed = 1;
    int iNumBytesRead = 0;

    /*if (bProceed) {
      
        pBuffer = (jbyte *)malloc(0xffff);
        if (pBuffer == NULL) {
            postJavaException(env,
                "org/gnu/pilotlink/PilotLinkException",
                "Unable to allocate buffer for app-info block");
            bProceed = 0;
        }
    }*/
    if (bProceed) {
        /* Read app-info block && verify successful read */
        iNumBytesRead = dlp_ReadAppBlock(handle, db, 0, pBuffer, MAX_RESOURCE_SIZE);
        if (iNumBytesRead < 0) {
            /* Throw Java exception, iNumBytesRead is actually an error code */
            postPilotLinkException(env, "Unable to read app-block", iNumBytesRead, errno);
            bProceed = 0;
        }

    }
    if (bProceed) {
        /* Look up RawAppInfo class */
        jClass_appInfo = env->FindClass("org/gnu/pilotlink/RawAppInfo");
        if (jClass_appInfo == NULL) {
            /* pending ClassNotFoundException in env */
            bProceed = 0;
        }
    }
    if (bProceed) {
        /* Look up constructor method with char array argument */
        jMethod_appInfo = env->GetMethodID(jClass_appInfo, "<init>","([B)V");
        if (jMethod_appInfo == NULL) {
            /* pending NoSuchMethodException in env */
            bProceed = 0;
        }
    }
    if (bProceed) {
	
        jbyteArray jArray_buffer = env->NewByteArray(iNumBytesRead);
        env->SetByteArrayRegion(jArray_buffer, 0, (jint)iNumBytesRead, (const jbyte*)pBuffer);
        jObject_appInfo = env->NewObject(jClass_appInfo, jMethod_appInfo, jArray_buffer);
    }

   /* if (pBuffer != NULL) free(pBuffer);*/
    return jObject_appInfo;
}
/*
   readSysInfo
*/
JNIEXPORT jobject JNICALL Java_org_gnu_pilotlink_PilotLink_readSysInfo
  (JNIEnv *env, jobject obj, jint handle) {
    jclass      jClass_sysInfo = NULL;
    jobject     jObject_sysInfo = NULL;
    jmethodID   jMethod_sysInfo = NULL;
    struct SysInfo rSysInfo;
    int bProceed = 1;

    if (bProceed) {
        int iResult;

        /* Check that dlp_ReadSysInfo call actually succeeded */
        memset(&rSysInfo, 0, sizeof(struct SysInfo));
        iResult = dlp_ReadSysInfo(handle, &rSysInfo);
        if (iResult < 0) {
            /* Throw Java exception in case of failure */
            postPilotLinkException(env, "Unable to read SysInfo", iResult, errno);
            bProceed = 0;
        }
    }
    if (bProceed) {
        /* Look up SysInfo class */
        jClass_sysInfo = env->FindClass("org/gnu/pilotlink/SysInfo");
        if (jClass_sysInfo == NULL) {
            /* pending ClassNotFoundException in env */
            bProceed = 0;
        }
    }
    if (bProceed) {
        /* Look up constructor method with complete-specification argument */
        jMethod_sysInfo = env->GetMethodID(jClass_sysInfo, "<init>",
            "(Ljava/lang/String;JJSSSSJ)V");
        if (jMethod_sysInfo == NULL) {
            /* pending NoSuchMethodException in env */
            bProceed = 0;
        }
    }
    if (bProceed) {
        /* Explicit casts to jlong are required because env->NewObject() works
           very similar to (and probably identical) to printf() and related
           procedures which use stdarg.h routines for variable arguments.
           The constructor requires a jlong (64 bits) for some fields, but the
           corresponding fields in the C struct are of a system-dependent size
           (32 bits for an unsigned long in i386). Failure to cast to an appropriate
           size results in a stack misalignment which, at the very least, causes
           incorrect values to be received at the Java side. For the sake of
           completeness, jshort values are explicitly casted too.
         */
        jstring jString_prodID = env->NewStringUTF(rSysInfo.prodID);
        jObject_sysInfo = env->NewObject(jClass_sysInfo, jMethod_sysInfo,
            jString_prodID, (jlong)rSysInfo.romVersion, (jlong)rSysInfo.locale,
            (jshort)rSysInfo.dlpMajorVersion, (jshort)rSysInfo.dlpMinorVersion,
            (jshort)rSysInfo.compatMajorVersion,
            (jshort)rSysInfo.compatMinorVersion, (jlong)rSysInfo.maxRecSize);
    }

    return jObject_sysInfo;
}

/*
  readUserInfo
  */
JNIEXPORT jobject JNICALL Java_org_gnu_pilotlink_PilotLink_readUserInfo
  (JNIEnv *env, jobject obj, jint handler) {

    jclass      jClass_user = NULL;
    jobject     jObject_user = NULL;
    jmethodID   jMethod_user = NULL;
    struct PilotUser rUserInfo;
    int bProceed = 1;

    if (bProceed) {
        int iResult;

        /* Check that dlp_ReadUserInfo call actually succeeded */
        memset(&rUserInfo, 0, sizeof(struct PilotUser));
        iResult = dlp_ReadUserInfo(handler, &rUserInfo);
        if (iResult < 0) {
            /* Throw Java exception in case of failure */
            postPilotLinkException(env, "Unable to read UserInfo", iResult, errno);
            bProceed = 0;
        } else {
            /* This was taken from previous code...  */
            rUserInfo.username[sizeof(rUserInfo.username) - 1] = '\0';
            rUserInfo.password[rUserInfo.passwordLength] = '\0';
        }
    }
    if (bProceed) {
        /* Look up User class */
        jClass_user = env->FindClass("org/gnu/pilotlink/User");
        if (jClass_user == NULL) {
            /* pending ClassNotFoundException in env */
            bProceed = 0;
        }
    }
    if (bProceed) {
        /* Look up constructor method with complete-specification argument.
           NOTE: a new constructor was provided in User.java in order to
           avoid having to create Date objects in C code, AND to fix a subtle
           bug in which the multiplication of time_t * int (miliseconds since
           Epoch) silently overflows because both time_t and int are 32-bit,
           and the result is NOT promoted to at least 64 bits, required for
           the constructor Date(long).
         */
        jMethod_user = env->GetMethodID(jClass_user, "<init>",
            "(Ljava/lang/String;Ljava/lang/String;JJJJJ)V");
        if (jMethod_user == NULL) {
            /* pending NoSuchMethodException in env */
            bProceed = 0;
        }
    }
    if (bProceed) {
        /* Explicit casts added - see object creation at
        Java_org_gnu_pilotlink_PilotLink_readSysInfo() for explanation. */
        jstring jString_username = env->NewStringUTF(rUserInfo.username);
        jstring jString_password = env->NewStringUTF(rUserInfo.password);
        jObject_user = env->NewObject(jClass_user, jMethod_user, jString_username,
            jString_password, (jlong)rUserInfo.userID, (jlong)rUserInfo.viewerID,
            (jlong)rUserInfo.lastSyncPC, (jlong)rUserInfo.lastSyncDate,
            (jlong)rUserInfo.successfulSyncDate);
    }
    return jObject_user;
}


/*
  writeUserInfo
  */
JNIEXPORT void JNICALL Java_org_gnu_pilotlink_PilotLink_writeUserInfo
  (JNIEnv *env, jobject obj, jint so, jobject user) {
    jclass      jClass_user = NULL;
    jmethodID   jMethod_user = NULL;
    struct PilotUser rUserInfo;
    jlong       jTempValue;
    jstring     jTempString;
    int bProceed = 1;

    /* Set everything not explicitly assigned to blanks */
    memset(&rUserInfo, 0, sizeof(struct PilotUser));

    /* Get object class for user... this should not fail */
    jClass_user = env->GetObjectClass(user);
    
    /* Assign all of the basic fields */
    if (bProceed) {
        bProceed = getBasicTypeField(env, jClass_user, user,
            "J", "userid", &jTempValue);
        rUserInfo.userID = jTempValue;
    }
    if (bProceed) {
        bProceed = getBasicTypeField(env, jClass_user, user,
            "J", "viewerid", &jTempValue);
        rUserInfo.viewerID = jTempValue;
    }
    if (bProceed) {
        bProceed = getBasicTypeField(env, jClass_user, user,
            "J", "lastSyncPC", &jTempValue);
        rUserInfo.lastSyncPC = jTempValue;
    }
    if (bProceed) {
        jTempString = NULL;
        bProceed = getBasicTypeField(env, jClass_user, user,
            "Ljava/lang/String;", "name", &jTempString);
        if (bProceed && jTempString != NULL) {
            const char * sTempString = env->GetStringUTFChars(jTempString, 0);
            strncpy(rUserInfo.username, sTempString, sizeof(rUserInfo.username) - 1);
            rUserInfo.username[sizeof(rUserInfo.username) - 1] = '\0';
            env->ReleaseStringUTFChars(jTempString, sTempString);
        }
    }
    if (bProceed) {
        jTempString = NULL;
        bProceed = getBasicTypeField(env, jClass_user, user,
            "Ljava/lang/String;", "password", &jTempString);
        if (bProceed && jTempString != NULL) {
            const char * sTempString = env->GetStringUTFChars(jTempString, 0);
            strncpy(rUserInfo.password, sTempString, sizeof(rUserInfo.password) - 1);
            rUserInfo.password[sizeof(rUserInfo.password) - 1] = '\0';
            rUserInfo.passwordLength = strlen(rUserInfo.password);
            env->ReleaseStringUTFChars(jTempString, sTempString);
        }
    }

    /* In order to get the correct time_t, a function call is needed. The methods
       User.getLastSyncDate_time_t() and User.getLastSuccessfulSyncDate_time_t()
       were created for this purpose.
     */
    if (bProceed) {
        jMethod_user = env->GetMethodID(jClass_user, "getLastSyncDate_time_t",
            "()J");
        if (jMethod_user == NULL) {
            /* pending NoSuchMethodException in env */
            bProceed = 0;
        } else {
            /* Use Date.getTime() in Java side and divide by 1000 */
            rUserInfo.lastSyncDate = env->CallLongMethod(user, jMethod_user);
        }
    }
    if (bProceed) {
        jMethod_user = env->GetMethodID(jClass_user, "getLastSuccessfulSyncDate_time_t",
            "()J");
        if (jMethod_user == NULL) {
            /* pending NoSuchMethodException in env */
            bProceed = 0;
        } else {
            /* Use Date.getTime() in Java side and divide by 1000 */
            rUserInfo.successfulSyncDate = env->CallLongMethod(user, jMethod_user);
        }
    }

    /* Write user info if all assignments were successful */
    if (bProceed) {
        int iResult = dlp_WriteUserInfo(so, &rUserInfo);
        if (iResult < 0) {
            postPilotLinkException(env, "Unable to write UserInfo", iResult, errno);
        }
    }
}

/*
 * writeAppBlock
 */
JNIEXPORT int JNICALL Java_org_gnu_pilotlink_PilotLink_writeAppBlock
	(JNIEnv *env, jobject obj, jint handle, jint dbhandle, jbyteArray data, jint length) {
    jbyte * buffer;
    int iResult = 0;

    /* Allocate memory for copy of app block */
    if ((buffer = (jbyte *)malloc(length * sizeof(jbyte))) != NULL) {
        env->GetByteArrayRegion(data, 0, length, buffer);
        iResult = dlp_WriteAppBlock(handle, dbhandle, buffer, length);
        if (iResult < 0) {
            /* Throw Java exception to signal error */
            postPilotLinkException(env, "Unable to write app-block", iResult, errno);
        }
        free(buffer);
    } else {
        postJavaException(env,
            "org/gnu/pilotlink/PilotLinkException",
            "Unable to allocate memory for app block");
    }
    return iResult;
}

/*
   openConduit
*/
JNIEXPORT void JNICALL Java_org_gnu_pilotlink_PilotLink_openConduit
  (JNIEnv *env, jobject obj, jint handle) {
  int iResult = dlp_OpenConduit(handle);
  if (iResult < 0) postPilotLinkException(env, "Unable to open conduit", iResult, errno);
}

/*
 * createDB
 */

JNIEXPORT jint JNICALL Java_org_gnu_pilotlink_PilotLink_createDB
  (JNIEnv *env, jobject obj, jint handle, jlong creator, jstring jdbname, jlong type, jint flags, jint version)
{
    jint db = -1;

    /* Get working copy of database name */
    const char * dbname = env->GetStringUTFChars(jdbname, NULL);
    char * dbn = (char *)malloc(strlen(dbname) + 1);
    if (dbn != NULL) strcpy(dbn, dbname);
    env->ReleaseStringUTFChars(jdbname, dbname);

    if (dbn != NULL) {
        int ret = dlp_CreateDB(handle,creator,type,0,flags,version,dbn, &db);

        if (ret < 0) {
        /* debug message replaced by Java exception */
/*
            printf("Error creating db %s: %d\n",dbn, ret);
*/
            postPilotLinkException(env, "Could not create database",
                ret, errno);
        }
        free(dbn);
    } else {
        /* Unable to create copy - memory heap may be corrupted */
        postJavaException(env,
            "org/gnu/pilotlink/PilotLinkException",
            "Unable to create working copy of database name");
    }
    return db;
}

/*
 * deleteDB
 */

JNIEXPORT jint JNICALL Java_org_gnu_pilotlink_PilotLink_deleteDB
	(JNIEnv *env, jobject obj, jint handle, jstring jdbname) {

    int ret = 0;

    /* Get working copy of database name */
    const char * dbname = env->GetStringUTFChars(jdbname,NULL);
    char * dbn = (char*)malloc(strlen(dbname)+1);
    if (dbn != NULL) strcpy(dbn,dbname);
    env->ReleaseStringUTFChars(jdbname, dbname);

    if (dbn != NULL) {
        ret = dlp_DeleteDB(handle,0,dbn);
        if (ret < 0) {
        /* debug message replaced by Java exception */
/*
            printf("Error deleting db %s %d:\n",dbn,ret);
*/
            postPilotLinkException(env, "Could not delete database",
                ret, errno);
        }
        free(dbn);
    } else {
        /* Unable to create copy - memory heap may be corrupted */
        postJavaException(env,
            "org/gnu/pilotlink/PilotLinkException",
            "Unable to create working copy of database name");
    }
    return ret;
}

/*
   openDB
*/
JNIEXPORT jint JNICALL Java_org_gnu_pilotlink_PilotLink_openDB
  (JNIEnv *env, jobject obj, jint handle, jstring jdbname) {

    jint db = 0;

    /* Get working copy of database name */
    const char* dbname = env->GetStringUTFChars(jdbname,NULL);
    char* dbn = (char*)malloc(strlen(dbname)+1);
    if (dbn != NULL) strcpy(dbn,dbname);
    env->ReleaseStringUTFChars(jdbname, dbname);

    if (dbn != NULL) {
        int ret=dlp_OpenDB(handle,0x40|0x80,dlpOpenReadWrite,dbn,&db);
        if (ret < 0) {
/*
            printf("Error opening db %s: %d\n",dbn, ret);
*/
            postPilotLinkException(env, "Could not open database",
                ret, errno);
            return (jint)ret;
        }
    } else {
        /* Unable to create copy - memory heap may be corrupted */
        postJavaException(env,
            "org/gnu/pilotlink/PilotLinkException",
            "Unable to create working copy of database name");
    }

    return db;
}

/*
  getRecordCount
*/
JNIEXPORT jint JNICALL Java_org_gnu_pilotlink_PilotLink_getRecordCount
  (JNIEnv *env, jobject obj, jint handle, jint dbh) {
     jint num;
     int iResult;
     iResult = dlp_ReadOpenDBInfo(handle,dbh,&num);
     if (iResult < 0) {
        postPilotLinkException(env, "Could not get record count for database",
                iResult, errno);
     }
     return num;
}

/*
  getRecordByIndex
*/
JNIEXPORT jobject JNICALL Java_org_gnu_pilotlink_PilotLink_getRecordByIndex
  (JNIEnv *env, jobject obj, jint handle , jint db , jint idx) {
    jbyte * buffer;
    jobject jObject_RawRecord = NULL;

    if ((buffer = (jbyte *)malloc(MAX_RESOURCE_SIZE)) != NULL) {

        recordid_t id_;
        size_t iRecSize;
	jint iRecAttr, iRecCategory;
        jclass jClass_RawRecord;
        jmethodID jMethod_RawRecord;

        int iResult = dlp_ReadRecordByIndex(handle, db, idx, buffer,
            &id_, &iRecSize, &iRecAttr, &iRecCategory);
        if (iResult < 0) {
            postPilotLinkException(env, "Could not read database record by index",
                iResult, errno);
        } else if ((jClass_RawRecord = env->FindClass(
            "org/gnu/pilotlink/RawRecord")) == NULL) {
            /* Pending ClassNotFoundException in env */
        } else if ((jMethod_RawRecord = env->GetMethodID(jClass_RawRecord,
            "<init>", "([BJIII)V")) == NULL) {
            /* Pending NoSuchMethodException in env */
        } else {
            jbyteArray jArray_buffer = env->NewByteArray(iRecSize);
            env->SetByteArrayRegion(jArray_buffer, 0, iRecSize, buffer);
            jObject_RawRecord = env->NewObject(jClass_RawRecord, jMethod_RawRecord,
                jArray_buffer, (jlong)id_, (jint)iRecSize, (jint)iRecAttr, (jint)iRecCategory);
        }
        free(buffer);
    } else {
        /* Unable to create copy - memory heap may be corrupted */
        postJavaException(env,
            "org/gnu/pilotlink/PilotLinkException",
            "Unable to allocate buffer for record data");
    }
    return jObject_RawRecord;
}

/*
  deleteRecordByIndex
*/
JNIEXPORT jint JNICALL Java_org_gnu_pilotlink_PilotLink_deleteRecordByIndex
  (JNIEnv *env, jobject obj, jint handle, jint db, jint idx) {
  return 0;
}

/* Write Record
*/
JNIEXPORT jint JNICALL Java_org_gnu_pilotlink_PilotLink_writeRecord
  (JNIEnv *env, jobject obj, jint handle, jint db, jobject record) {
    jclass jClass_Record;
    jmethodID jMethod_Record[6];

    int iResult = 0;

    /* Get object class for record - should not fail */
    jClass_Record = env->GetObjectClass(record);

    /* Get all object methods for the class */
    if ((jMethod_Record[0] = env->GetMethodID(jClass_Record, "getSize", "()I")) != NULL &&
        (jMethod_Record[1] = env->GetMethodID(jClass_Record, "getBuffer", "()[B")) != NULL &&
        (jMethod_Record[2] = env->GetMethodID(jClass_Record, "getAttribs", "()I")) != NULL &&
        (jMethod_Record[3] = env->GetMethodID(jClass_Record, "getId", "()J")) != NULL &&
        (jMethod_Record[4] = env->GetMethodID(jClass_Record, "getCategory", "()I")) != NULL &&
        (jMethod_Record[5] = env->GetMethodID(jClass_Record, "setId", "(J)V")) != NULL) {

        /* Find out size of buffer && allocate memory && copy data into temp. buffer */
        jint iSize = env->CallIntMethod(record, jMethod_Record[0]);
        jbyte * buffer = (jbyte *)malloc(iSize * sizeof(jbyte));
        if (buffer != NULL) {
            recordid_t iNewID = 0;

            jbyteArray jArray_buffer = (jbyteArray)env->CallObjectMethod(record, jMethod_Record[1]);
            env->GetByteArrayRegion(jArray_buffer, 0, iSize, buffer);
            iResult = dlp_WriteRecord(handle, db, 
                env->CallLongMethod(record, jMethod_Record[2]),
                env->CallLongMethod(record, jMethod_Record[3]),
                env->CallLongMethod(record, jMethod_Record[4]),
                buffer, iSize, &iNewID);
            if (iResult < 0) {
                postPilotLinkException(env, "Could not write database record",
                    iResult, errno);
            } else {
                /* Assign new ID for record */
                env->CallVoidMethod(record, jMethod_Record[5], (jlong)iNewID);
            }
            free(buffer);
        } else {
            /* Unable to create copy - memory heap may be corrupted */
            postJavaException(env,
                "org/gnu/pilotlink/PilotLinkException",
                "Unable to allocate buffer for record data");
        }
    } else {
        /* Pending NoSuchMethodException in env */
    }

    return iResult;
}

/*CloseDB*/
JNIEXPORT void JNICALL Java_org_gnu_pilotlink_PilotLink_closeDB
  (JNIEnv *env, jobject obj, jint han, jint db) {
    int bException = 0;
    int ret=dlp_CleanUpDatabase(han, db);
    if (ret<0) {
//       printf("Error cleaning up! %d\n",ret);
        postPilotLinkException(env, "Unable to clean up before database close",
            ret, errno);
        bException = 1;
    }
    ret=dlp_CloseDB(han, db);
    if (ret < 0 && !bException) {
//       printf("Error closing DB: %d\n",ret);
        postPilotLinkException(env, "Unable to close database",
            ret, errno);
    }
}
/*ENDSync*/
JNIEXPORT void JNICALL Java_org_gnu_pilotlink_PilotLink_endSync
  (JNIEnv *env, jobject obj, jint sd) {
    int iResult = dlp_EndOfSync(sd, dlpEndCodeNormal);
    if (iResult < 0) postPilotLinkException(env, "Unable to signal endSync",
        iResult, errno);
}
/*Close*/
JNIEXPORT void JNICALL Java_org_gnu_pilotlink_PilotLink_close
   (JNIEnv *env, jobject obj, jint sd) {
    pi_close(sd);
}

/*pilot-connect*/
int pilot_connect(JNIEnv * env, const char *port)
{
        int     parent_sd       = -1,   /* Client socket, formerly sd   */
                client_sd       = -1,   /* Parent socket, formerly sd2  */
                result;
        struct  pi_sockaddr addr;
        struct  stat attr;
        struct  SysInfo sys_info;
        const char    *defport = "/dev/pilot";
        int bProceed = 1;

        if (port == NULL && (port = getenv("PILOTPORT")) == NULL) {

                /* err seems to be used for stat() only */
                int err = 0;

                /* Commented out debug code */
/*
                fprintf(stderr, "   No $PILOTPORT specified and no -p "
                        "<port> given.\n"
                        "   Defaulting to '%s'\n", defport);
*/
                port = defport;
                err = stat(port, &attr);

                /* Moved err check inside if() block - err only meaningful here */
                if (err) {
                        /* Throw an exception - FileNotFoundException seems appropriate here */
                        postJavaException(env, "java/io/FileNotFoundException", strerror(errno));
                        bProceed = 0;
                }
        }

/* At this point, either bProceed is 0, or port != NULL, further checks are unnecesary */

        /* Check bProceed to account for previous exceptions */
        if (bProceed && !(parent_sd = pi_socket(PI_AF_PILOT, PI_SOCK_STREAM, PI_PF_DLP))) {
                /* Throw exception here to inform nature of connection failure. */
                const char * sTemplate = "Unable to create socket '%s'";
                char * sMessage = (char *)malloc(strlen(sTemplate) + strlen(port) + 1);
                if (sMessage != NULL) sprintf(sMessage, sTemplate, port);
                postJavaException(env, "org/gnu/pilotlink/PilotLinkException",
                    (sMessage != NULL) ? sMessage : port);
                if (sMessage != NULL) free(sMessage);

                bProceed = 0;
        }

        /* Check bProceed to account for previous exceptions */
        if (bProceed) {
                        addr.pi_family = PI_AF_PILOT;
                        strncpy(addr.pi_device, port, sizeof(addr.pi_device));
                        result =
                            pi_bind(parent_sd, (struct sockaddr *) &addr, sizeof(addr));
        }

        if (bProceed && result < 0) {
                int save_errno = errno;
/*
                const char *portname;

                portname = (port != NULL) ? port : getenv("PILOTPORT");
                if (portname) {
                        fprintf(stderr, "\n");
                        errno = save_errno;
                        fprintf(stderr, "   ERROR: %s (%d)\n\n", strerror(errno),
                                errno);

                        if (errno == 2) {
                                fprintf(stderr, "   The device %s does not exist..\n",
                                        portname);
                                fprintf(stderr, "   Possible solution:\n\n\tmknod %s c "
                                        "<major> <minor>\n\n", portname );

                        } else if (errno == 13) {
                                fprintf(stderr, "   Please check the "
                                        "permissions on %s..\n",   portname );
                                fprintf(stderr, "   Possible solution:\n\n\tchmod 0666 "
                                        "%s\n\n", portname );

                        } else if (errno == 19) {
                                fprintf(stderr, "   Press the HotSync button first and "
                                        "relaunch this conduit..\n\n");
                        } else if (errno == 21) {
                                fprintf(stderr, "   The port specified must contain a "
                                        "device name, and %s was a directory.\n"
                                        "   Please change that to reference a real "
                                        "device, and try again\n\n", portname );
                        }

                        fprintf(stderr, "   Unable to bind to port: %s\n",
                                portname) ;
                        fprintf(stderr, "   Please use --help for more "
                                "information\n\n");
                } else
                        fprintf(stderr, "\n   No port specified\n");
*/
                const char * sTemplate = "Unable to bind to port %s - (%d) %s%s";
                const char * sFailureReason;
                char * sMessage;

                switch (save_errno) {
                case 2:
                    sFailureReason = "; Device does not exist (use mknod to fix)";
                    break;
                case 13:
                    sFailureReason = "; Access denied on device (use chmod to fix)";
                    break;
                case 19:
                    sFailureReason = "; HotSync button must be pressed first";
                    break;
                case 21:
                    sFailureReason = "; Device name appears to be a directory";
                    break;
                default:
                    sFailureReason = "";
                    break;
                }
                sMessage = (char *)malloc(strlen(sTemplate) + strlen(port) + 16 +
                    strlen(strerror(save_errno)) + strlen(sFailureReason));
                if (sMessage != NULL) {
                    sprintf(sMessage, sTemplate, port, save_errno, strerror(save_errno), sFailureReason);
                    postJavaException(env, "org/gnu/pilotlink/PilotLinkException", sMessage);
                    free(sMessage);
                } else {
                    /* Unable to malloc(), inform of errno string */
                    postJavaException(env, "org/gnu/pilotlink/PilotLinkException", strerror(save_errno));
                }

                pi_close(parent_sd);
                pi_close(client_sd);
//                return -1;
                bProceed = 0;
        }

/* Removed debug message, maybe add notification framework to invoke here
        fprintf(stderr,
                "\n   Listening to port: %s\n\n   Please press the HotSync "
                "button now... ",
                port ? port : getenv("PILOTPORT"));
*/
        /* Check bProceed to account for previous exceptions */
        if (bProceed && pi_listen(parent_sd, 1) == -1) {
                /* debug message replaced with Java exception */
                /* fprintf(stderr, "\n   Error listening on %s\n", port); */
                postJavaException(env,
                    "org/gnu/pilotlink/PilotLinkException",
                    "Unable to listen on port");
                pi_close(parent_sd);
                pi_close(client_sd);
/*                return -1; */
                bProceed = 0;
        }

        /* Check bProceed to account for previous exceptions */
        if (bProceed) {
                client_sd = pi_accept_to(parent_sd, 0, 0, 5);
                if (client_sd == -1) {
                        /* debug message replaced with Java exception */
                        /* fprintf(stderr, "\n   Error accepting data on %s\n", port); */
                        postJavaException(env,
                            "org/gnu/pilotlink/PilotLinkException",
                            "Unable to accept data on port");
                        pi_close(parent_sd);
                        pi_close(client_sd);
/*                        return -1;*/
                        bProceed = 0;
                } else {
                    int so_timeout = 16;
                    size_t sizeof_so_timeout = sizeof(so_timeout);

                    pi_setsockopt(client_sd, PI_LEVEL_DEV, PI_DEV_TIMEOUT, 
                        &so_timeout, &sizeof_so_timeout);
                }
        }

/* Removed debug message, maybe add notification framework to invoke here */
/*        fprintf(stderr, "Connected\n\n"); */

        /* Check bProceed to account for previous exceptions */
        if (bProceed) {
        
                int iResult;
                int iNumRetries = 5;
                
                while (iNumRetries > 0 && (iResult = dlp_ReadSysInfo(client_sd, &sys_info)) < 0)
                    iNumRetries--;
                if (iResult < 0) {
                    /* debug message replaced with Java exception */
                    /* fprintf(stderr, "\n   Error read system info on %s\n", port); */
                    postJavaException(env,
                        "org/gnu/pilotlink/PilotLinkException",
                        "Unable to read system info on port");
                    pi_close(parent_sd);
                    pi_close(client_sd);
/*                return -1; */
                    bProceed = 0;
                }
        }
/* Removed debug message, maybe add notification framework to invoke here */
/*        printf("Opening counduit...\n"); */
        dlp_OpenConduit(client_sd);
/* Why should parent_sd remain open after connect? Nobody receives it. */
        pi_close(parent_sd);
        return client_sd;
}

/*
 * getResourceByIndex()
 */
JNIEXPORT jobject JNICALL Java_org_gnu_pilotlink_PilotLink_getResourceByIndex
  (JNIEnv * env, jobject obj, jint iSockHandle, jint iDBHandle, jint iRsrcIndex) {

    unsigned long iRsrcType;    /* Resource type */
    int iRsrcID;                /* Resource ID */
    size_t iRsrcSize;              /* Actual resource size */
    jbyte * pRsrcData;          /* Buffer for resource data */
    int iResult;                /* Result of library call */
    jobject pRecordObject = NULL;

    /* Get "enough" memory for incoming resource data */
    if ((pRsrcData = (jbyte *)malloc(MAX_RESOURCE_SIZE * sizeof(jbyte))) != NULL) {

        /* Invoke C library function */
        iResult = dlp_ReadResourceByIndex(iSockHandle, iDBHandle, iRsrcIndex,
            pRsrcData, &iRsrcType, &iRsrcID, &iRsrcSize);
        if (iResult >= 0) {
            jclass pRecordClass;
            jmethodID pRecordConstructor;

            /* Look up class and constructor method */
            if ((pRecordClass = env->FindClass("org/gnu/pilotlink/RawRecord")) != NULL &&
                (pRecordConstructor = env->GetMethodID(pRecordClass, "<init>",
                "([BJIII)V")) != NULL ) {

                /* Fill a Java array with resource data && invoke constructor */
                jbyteArray pJavaArray = env->NewByteArray(iRsrcSize);
                env->SetByteArrayRegion(pJavaArray, 0, iRsrcSize, pRsrcData);
                pRecordObject = env->NewObject(pRecordClass, pRecordConstructor,
                    pJavaArray, (jlong)iRsrcID, (jint)iRsrcSize, (jint)0, (jint)iRsrcType);
            }
        } else {
            postPilotLinkException(env, "Unable to read resource by index", iResult, errno);
        }
        free(pRsrcData);
    } else {
        postJavaException(env,
            "org/gnu/pilotlink/PilotLinkException",
            "Unable to allocate buffer for copy of resource");
    }

    return pRecordObject;
}

/*
 * writeResource()
 */
JNIEXPORT void JNICALL Java_org_gnu_pilotlink_PilotLink_writeResource
  (JNIEnv * env, jobject obj, jint iSockHandle, jint iDBHandle, jobject pRecord)
{
    jlong iRsrcID;              /* Resource ID for record */
    jint iRsrcType;             /* Resource type for record */
    jint iRsrcSize;             /* Actual resource size */
    jbyte * pRsrcData = NULL;   /* Buffer for resource data */

    jclass pRecordClass;        /* Java class for record */
    jmethodID pRecordMethod;    /* Java instance method for record */
    jbyteArray pDataArray;      /* Java array for resource data */
    jint iResult = -1;          /* Result of library call */
    int bProceed = 1;

    /* Get reference to record class */
    pRecordClass = env->GetObjectClass(pRecord);

    /* Find out required attributes for resource write */
    if (bProceed) {
        /* Find out category (resource type) */
        if ((pRecordMethod = env->GetMethodID(pRecordClass, "getCategory", "()I")) != NULL) {
            iRsrcType = env->CallIntMethod(pRecord, pRecordMethod);
        } else {
            /* Method lookup failed */
            bProceed = 0;
        }
    }
    if (bProceed) {
        /* Find out resource ID */
        if ((pRecordMethod = env->GetMethodID(pRecordClass, "getId", "()J")) != NULL) {
            iRsrcID = env->CallLongMethod(pRecord, pRecordMethod);
        } else {
            /* Method lookup failed */
            bProceed = 0;
        }
    }
    if (bProceed) {
        /* Find out resource size && allocate memory */
        if ((pRecordMethod = env->GetMethodID(pRecordClass, "getSize", "()I")) != NULL) {
            iRsrcSize = env->CallIntMethod(pRecord, pRecordMethod);
            if (iRsrcSize > 0) pRsrcData = (jbyte *)malloc(iRsrcSize * sizeof(char));
            if (pRsrcData == NULL) {
                postJavaException(env,
                    "org/gnu/pilotlink/PilotLinkException",
                    "Unable to allocate buffer for copy of resource");
                bProceed = 0;
            }
        } else {
            /* Method lookup failed */
            bProceed = 0;
        }
    }
    if (bProceed) {
        /* Get copy of resource data in allocated memory */
        if ((pRecordMethod = env->GetMethodID(pRecordClass, "getBuffer", "()[B")) != NULL) {
            pDataArray = (jbyteArray)env->CallObjectMethod(pRecord, pRecordMethod);
            env->GetByteArrayRegion(pDataArray, 0, iRsrcSize, pRsrcData);
            iResult = dlp_WriteResource(iSockHandle, iDBHandle, iRsrcType, iRsrcID,
                pRsrcData, iRsrcSize);
            if (iResult < 0) {
                postPilotLinkException(env, "Unable to write resource", iResult, errno);
            }
        } else {
            /* Method lookup failed */
            bProceed = 0;
        }
    }

    if (pRsrcData != NULL) free(pRsrcData);
}

JNIEXPORT void JNICALL Java_org_gnu_pilotlink_PilotLink_resetSystem
  (JNIEnv * env, jobject, jint iSockHandle)
{
    int iResult = dlp_ResetSystem(iSockHandle);
    if (iResult < 0) {
        postPilotLinkException(env, "Unable to reset system", iResult, errno);
    }
}

/*
 * readDBList
 */
JNIEXPORT jobject JNICALL Java_org_gnu_pilotlink_PilotLink_readDBList
  (JNIEnv * env, jobject, jint iSockHandle, jint cardno, jint flags, jint start)
{
    int iResult = 0;
    struct DBInfo rInfoDB;
    jobject pDBInfoObject = NULL;

    /* Execute low-level library call... */
    iResult = dlp_ReadDBList(iSockHandle, cardno, flags, start, &rInfoDB);
    if (iResult >= 0) {
        jclass pDBInfoClass = NULL;
        jmethodID pDBInfoMethod = NULL;
        int bProceed = 1;

        /* Find class for DBInfo - throws ClassNotFoundException if not found */
        if (bProceed) {
            pDBInfoClass = env->FindClass("org/gnu/pilotlink/DBInfo");
            if (pDBInfoClass == NULL) bProceed = 0;
        }

        /* Find constructor method for class DBInfo */
        if (bProceed) {
            pDBInfoMethod = env->GetMethodID(pDBInfoClass, "<init>",
                "(Ljava/lang/String;IIIIIISSSBB)V");
            if (pDBInfoMethod != NULL && env->ExceptionOccurred() == NULL) {
                /* Build new object, propagate any exception into Java caller */
                jstring jString_nombre = env->NewStringUTF(rInfoDB.name);
                pDBInfoObject = env->NewObject(pDBInfoClass, pDBInfoMethod,
                    jString_nombre, (jint)rInfoDB.createDate, (jint)rInfoDB.modifyDate,
                    (jint)rInfoDB.backupDate, (jint)rInfoDB.type, (jint)rInfoDB.creator,
                    (jint)rInfoDB.modnum, (jshort)rInfoDB.flags, (jshort)rInfoDB.version,
                    (jshort)rInfoDB.index, (jbyte)rInfoDB.miscFlags, (jbyte)rInfoDB.more);
                if (env->ExceptionOccurred() != NULL) bProceed = 0;
            } else {
                /* pending NoSuchMethodException in env */
                bProceed = 0;
            }
        }

    /* "Not Found" error returns null, else throw Java exception to inform problem with listing */
    } else if (iResult != -5) {
        postPilotLinkException(env, "Unable to read DB list entry", iResult, errno);
    }
    return pDBInfoObject;
}

static void postPilotLinkException(JNIEnv * env, const char * sTexto, int iResult, int save_errno)
{
    const char * sTemplate = "%s: %s - %s";
    const char * sGenericException = "org/gnu/pilotlink/PilotLinkException";
    const char * sDBExistsException = "org/gnu/pilotlink/DatabaseExistsException";
    const char * sDBNotFoundException = "org/gnu/pilotlink/DatabaseNotFoundException";
    const char * sExceptionClass;
    char * sMessage = NULL;

    // Make save_errno positive in all cases
    if (save_errno < 0) save_errno = -save_errno;

    // Choose which exception to report
    switch (abs(iResult)) {
    case 5:
        sExceptionClass = sDBNotFoundException;
        break;
    case 9:
        sExceptionClass = sDBExistsException;
        break;
    default:
        sExceptionClass = sGenericException;
        break;
    }

    // Attempt to describe underlying errno for exception
    sMessage = (char *)malloc(strlen(sTemplate) + strlen(sTexto) +
        strlen(dlp_strerror(iResult)) + strlen(strerror(save_errno)) + 1);
    if (sMessage != NULL) {
        sprintf(sMessage, sTemplate, sTexto, dlp_strerror(iResult), strerror(save_errno));
        postJavaException(env, sExceptionClass, sMessage);
        free(sMessage);
    } else {
        postJavaException(env, sExceptionClass, dlp_strerror(iResult));
    }
}

/* Post a named exception in the Java environment */
static void postJavaException(JNIEnv * env, const char * sExceptionClass, const char * sMessage)
{
    jclass pExceptionClass = env->FindClass(sExceptionClass);
    if (pExceptionClass != NULL) env->ThrowNew(pExceptionClass, sMessage);
}

/* Get a value from a named Java field of basic type or String */
static int getBasicTypeField(JNIEnv * env, jclass pClass, jobject pObject,
    const char * sFieldType, const char * sFieldName, void * pLocation)
{
    int bProceed = 1;
    jfieldID pFieldID = NULL;
    jthrowable pException = NULL;

    pFieldID = env->GetFieldID(pClass, sFieldName, sFieldType);
    if (pFieldID != NULL && (pException = env->ExceptionOccurred()) == NULL) {
        if (strcmp(sFieldType, "I") == 0) {
            *((jint *)pLocation) = env->GetIntField(pObject, pFieldID);
        } else if (strcmp(sFieldType, "S") == 0) {
            *((jshort *)pLocation) = env->GetShortField(pObject, pFieldID);
        } else if (strcmp(sFieldType, "B") == 0) {
            *((jbyte *)pLocation) = env->GetByteField(pObject, pFieldID);
        } else if (strcmp(sFieldType, "Z") == 0) {
            *((jboolean *)pLocation) = env->GetBooleanField(pObject, pFieldID);
        } else if (strcmp(sFieldType, "C") == 0) {
            *((jchar *)pLocation) = env->GetCharField(pObject, pFieldID);
        } else if (strcmp(sFieldType, "J") == 0) {
            *((jlong *)pLocation) = env->GetLongField(pObject, pFieldID);
        } else if (strcmp(sFieldType, "F") == 0) {
            *((jfloat *)pLocation) = env->GetFloatField(pObject, pFieldID);
        } else if (strcmp(sFieldType, "D") == 0) {
            *((jdouble *)pLocation) = env->GetDoubleField(pObject, pFieldID);
        } else if (sFieldType[0] == 'L') {
            *((jobject *)pLocation) = env->GetObjectField(pObject, pFieldID);
        }
    } else {
        bProceed = 0;
    }
    return bProceed;
}

/* Assign a value to a named Java field of basic type or String */
static int assignBasicTypeField(JNIEnv * env, jclass pClass, jobject pObject,
    const char * sFieldType, const char * sFieldName, ...)
{
    int bProceed = 1;
    jfieldID pFieldID = NULL;
    jthrowable pException = NULL;

    pFieldID = env->GetFieldID(pClass, sFieldName, sFieldType);
    if (pFieldID != NULL && (pException = env->ExceptionOccurred()) == NULL) {

        va_list ap;

        va_start(ap, sFieldName);
        if (strcmp(sFieldType, "I") == 0) {
            jint iValue = va_arg(ap, jint);
            env->SetIntField(pObject, pFieldID, iValue);
        } else if (strcmp(sFieldType, "S") == 0) {
            jshort iValue = va_arg(ap, int);
            env->SetShortField(pObject, pFieldID, iValue);
        } else if (strcmp(sFieldType, "B") == 0) {
            jbyte iValue = va_arg(ap, int);
            env->SetByteField(pObject, pFieldID, iValue);
        } else if (strcmp(sFieldType, "Z") == 0) {
            jboolean iValue = va_arg(ap, int);
            env->SetBooleanField(pObject, pFieldID, iValue);
        } else if (strcmp(sFieldType, "C") == 0) {
            jchar iValue = va_arg(ap, int);
            env->SetCharField(pObject, pFieldID, iValue);
        } else if (strcmp(sFieldType, "J") == 0) {
            jlong iValue = va_arg(ap, jlong);
            env->SetLongField(pObject, pFieldID, iValue);
        } else if (strcmp(sFieldType, "F") == 0) {
            jfloat iValue = va_arg(ap, double);
            env->SetFloatField(pObject, pFieldID, iValue);
        } else if (strcmp(sFieldType, "D") == 0) {
            jdouble iValue = va_arg(ap, jdouble);
            env->SetDoubleField(pObject, pFieldID, iValue);
        } else if (strcmp(sFieldType, "Ljava/lang/String;") == 0) {
            const char * s = va_arg(ap, const char *);
            jstring jNameString = env->NewStringUTF(s);
            env->SetObjectField(pObject, pFieldID, jNameString);
        } else if (sFieldType[0] == 'L') {
            jobject pAssignedObject = va_arg(ap, jobject);
            env->SetObjectField(pObject, pFieldID, pAssignedObject);
        }
        va_end(ap);
    } else {
        bProceed = 0;
    }
    return bProceed;
}
