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
 
#include <pi-source.h>
#include <pi-socket.h>
#include <pi-dlp.h>
#include <pi-file.h>
#include <pi-memo.h>
#include "libjpisock.h"

int pilot_connect(char *port);

/*
    PilotLink.connect
*/

JNIEXPORT jint JNICALL Java_org_gnu_pilotlink_PilotLink_connect
   (JNIEnv *env, jobject obj, jstring jprt) {
	const char* port=env->GetStringUTFChars(jprt,NULL);
	char* prt=(char*)malloc(strlen(port)+1);
	strcpy(prt,port);
	return pilot_connect(prt);
}
/*
ReadAppInfo
*/
JNIEXPORT jobject JNICALL Java_org_gnu_pilotlink_PilotLink_readAppInfo
  (JNIEnv *env, jobject obj, jint handle, jint db) {
	  
	
    unsigned char buffer[65535];
    
	int size=dlp_ReadAppBlock(handle,db, 0,buffer,65535);
	
	
	
	//printf("STill alive...\n");
	//fflush(stdout);
	jclass appinfo_cls=env->FindClass("org/gnu/pilotlink/RawAppInfo");
	if (appinfo_cls==NULL) {
		printf("Class not found! Sysinfo!\n");
		return NULL;
	}
	//printf("STill alive...\n");
	fflush(stdout);
	jmethodID appinfo_mid=env->GetMethodID(appinfo_cls, "<init>","([B)V");
	if (appinfo_mid==NULL) {
		printf("Problem mid!\n");
		fflush(stdout);
		return NULL;
	}
	//printf("STill alive...\n");
	//fflush(stdout);
    jbyteArray array=env->NewByteArray(size);
    env->SetByteArrayRegion(array,0,(jint)size,(jbyte*)buffer);
	jobject appinfo=env->NewObject(appinfo_cls, appinfo_mid, array);
	
	return appinfo;
}
/*
   readSysInfo
*/
JNIEXPORT jobject JNICALL Java_org_gnu_pilotlink_PilotLink_readSysInfo
  (JNIEnv *env, jobject obj, jint handle) {
	  
	struct SysInfo s;
	s.prodID[s.prodIDLength]=0;
	int r=dlp_ReadSysInfo(handle,&s);
	
	jstring prod=env->NewStringUTF(s.prodID);
	
	//printf("STill alive...\n");
	fflush(stdout);
	jclass sysinfo_cls=env->FindClass("org/gnu/pilotlink/SysInfo");
	if (sysinfo_cls==NULL) {
		printf("Class not found! Sysinfo!\n");
		return NULL;
	}
	//printf("STill alive...\n");
	fflush(stdout);
	jmethodID sysinfo_mid=env->GetMethodID(sysinfo_cls, "<init>","(Ljava/lang/String;JJSSSSJ)V");
	if (sysinfo_mid==NULL) {
		printf("Problem mid!\n");
		fflush(stdout);
		return NULL;
	}
	//printf("STill alive...\n");
	fflush(stdout);
	jobject sysinfo=env->NewObject(sysinfo_cls, sysinfo_mid, prod, s.romVersion, s.locale, s.dlpMajorVersion, s.dlpMinorVersion,s.compatMajorVersion,s.compatMinorVersion,s.maxRecSize);
	printf("Returning from getsysinfo...\n");
	fflush(stdout);
	return sysinfo;
}

/*
  readUserInfo
  */
JNIEXPORT jobject JNICALL Java_org_gnu_pilotlink_PilotLink_readUserInfo
  (JNIEnv *env, jobject obj, jint handler) {
   struct PilotUser U;
   dlp_ReadUserInfo(handler, &U);
   U.username[127]=0;
   U.password[U.passwordLength]=0;

   printf("Last sync-->%ld\n",U.lastSyncDate);
   printf("Last ssync->%ld\n",U.successfulSyncDate);
  jstring str_name=env->NewStringUTF(U.username);
  if (str_name==NULL) {
	return NULL;
 }
  jstring str_pw=env->NewStringUTF(U.password);
  if (str_pw==NULL) {
     return NULL;
  }
  jlong vid=U.viewerID;
  jlong uid=U.userID;
  jlong lspc=U.lastSyncPC;
  
  jclass calclass=env->FindClass("java/util/Date");
  if (calclass==NULL) {
      return NULL;
  } 
  
  jmethodID cal_mid=env->GetMethodID(calclass,"<init>","(J)V");
  if (cal_mid==NULL){
     return NULL;
  }
  
  jobject lsd_date=env->NewObject(calclass,cal_mid,U.lastSyncDate*1000);
  jobject sucd_date=env->NewObject(calclass,cal_mid,U.successfulSyncDate*1000);

  jclass usercls=env->FindClass("org/gnu/pilotlink/User");
  if (usercls==NULL) {
    printf("USERCLASS not found!\n");
    return NULL;
  } else {
    printf("ok...\n");
  }
  jmethodID umid=env->GetMethodID(usercls,"<init>","(Ljava/lang/String;Ljava/lang/String;JJJLjava/util/Date;Ljava/util/Date;)V");
  if (umid==NULL) {
     printf("MethodID not found!\n");
     return NULL;
  } else {
     printf("ok...\n");
  }
  //printf("Returning from getuserinfo...\n");
  jobject u=env->NewObject(usercls,umid,str_name,str_pw,uid,vid,lspc,lsd_date, sucd_date); 
  return u;
}


/*
  writeUserInfo
  */
JNIEXPORT void JNICALL Java_org_gnu_pilotlink_PilotLink_writeUserInfo
  (JNIEnv *env, jobject obj, jint so, jobject user) {
    printf("Not implemented yet...sorry\n");
}

/*
 * writeAppBlock
 */
JNIEXPORT int JNICALL Java_org_gnu_pilotlink_PilotLink_writeAppBlock
	(JNIEnv *env, jobject obj, jint handle, jint dbhandle, jbyteArray data, jint length) {
		char buffer[65536];
		env->GetByteArrayRegion(data,0,length,(jbyte*)buffer);
		return dlp_WriteAppBlock(handle,dbhandle,buffer,length);
}

/*
   openConduit
*/
JNIEXPORT void JNICALL Java_org_gnu_pilotlink_PilotLink_openConduit
  (JNIEnv *env, jobject obj, jint handle) {
  dlp_OpenConduit(handle);
}

/*
 * createDB
 */

JNIEXPORT jint JNICALL Java_org_gnu_pilotlink_PilotLink_createDB
  (JNIEnv *env, jobject obj, jint handle, jlong creator, jstring jdbname, jlong type) {
	const char* dbname=env->GetStringUTFChars(jdbname,NULL);
	char* dbn=(char*)malloc(strlen(dbname)+1);
	strcpy(dbn,dbname);

	jint db;
	int ret=dlp_CreateDB(handle,creator,type,0,0,1,dbn,&db);
	if (ret<0) {
		printf("Error creating db %s: %d\n",dbn, ret);
	}
	return db;
	
  }

/*
 * closeDB
 */

JNIEXPORT jint JNICALL Java_org_gnu_pilotlink_PilotLink_deleteDB
	(JNIEnv *env, jobject obj, jint handle, jstring jdbname) {
		const char* dbname=env->GetStringUTFChars(jdbname,NULL);
		char* dbn = (char*)malloc(strlen(dbname)+1);
		strcpy(dbn,dbname);
		int ret=dlp_DeleteDB(handle,0,dbn);
		if (ret < 0) {
			printf("Error deleting db %s %d:\n",dbn,ret);
		}
		return ret;
}
 
/*
   openDB
*/
JNIEXPORT jint JNICALL Java_org_gnu_pilotlink_PilotLink_openDB
  (JNIEnv *env, jobject obj, jint handle, jstring jdbname) {
	const char* dbname=env->GetStringUTFChars(jdbname,NULL);
	char* dbn=(char*)malloc(strlen(dbname)+1);
	strcpy(dbn,dbname);
	jint db;
	int ret=dlp_OpenDB(handle,0,dlpOpenReadWrite,dbn,&db);
    if (ret<0) {
        printf("Error opening db %s: %d\n",dbn, ret);
        return (jint)ret;
    }
	return db;
}

/*
  getRecordCount
*/
JNIEXPORT jint JNICALL Java_org_gnu_pilotlink_PilotLink_getRecordCount
  (JNIEnv *env, jobject obj, jint handle, jint dbh) {
     jint num;
     dlp_ReadOpenDBInfo(handle,dbh,&num);
     return num;
}

/*
  getRecordByIndex
*/
JNIEXPORT jobject JNICALL Java_org_gnu_pilotlink_PilotLink_getRecordByIndex
  (JNIEnv *env, jobject obj, jint handle , jint db , jint idx) {
	  jbyte buffer[65536];
	  recordid_t id;
	  jint size, attr, category;
	  //printf("Getting record..\n");
	  int ret = dlp_ReadRecordByIndex(handle, db, idx, buffer, 
				  &id, &size, &attr, &category);
          if (ret<0) {
		return NULL;
	  }				
	  //printf("getting class!\n");
	  jclass rcls=env->FindClass("org/gnu/pilotlink/RawRecord");
	  if (rcls==NULL) {
		  return NULL;
	  }
	  jmethodID rid=env->GetMethodID(rcls,"<init>","([BJIII)V");
	  if (rid==NULL) {
		  printf("jmethodID is null!\n");
		  return NULL;
	  }
	  jbyteArray array=env->NewByteArray(size);
	  env->SetByteArrayRegion(array,0,size,buffer);
	  jobject record=env->NewObject(rcls, rid, array, (jlong)id,size,attr,category );
	  return record;
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
	  
	  jlong id=0;
	  jclass cls=env->GetObjectClass(record);
	  jmethodID mid=env->GetMethodID(cls,"getCategory","()I");
	
	  if (mid==NULL) { printf("Error getting mid!"); return -1; }
	  jint cat=env->CallIntMethod(record,mid);
	  
	  jmethodID mid2=env->GetMethodID(cls,"getId","()J");

	  id=env->CallLongMethod(record,mid2, NULL);
	  if (id==0) {
		  printf("Creating new entry!\n");
	  }

	  mid=env->GetMethodID(cls,"getAttribs","()I");
	  jint attr=env->CallIntMethod(record,mid);

	  mid=env->GetMethodID(cls,"getSize","()I");
	  jint size=env->CallIntMethod(record,mid);
      //printf("Got size: %d\n",size);
	  char buffer[65536];
	  mid=env->GetMethodID(cls,"getBuffer","()[B");
	  jbyteArray arr=(jbyteArray)env->CallObjectMethod(record,mid);
	  env->GetByteArrayRegion(arr,0,size,(jbyte*)buffer);
	  recordid_t i=0;
	  int ret=dlp_WriteRecord(handle,db,attr, id, cat, buffer,size,&i);
	  if (id==0 && ret>0) {
		  mid=env->GetMethodID(cls,"setId","(J)V");
		  env->CallVoidMethod(record,mid,(jlong)i);
	  }
	  return ret;
}

/*CloseDB*/
JNIEXPORT void JNICALL Java_org_gnu_pilotlink_PilotLink_closeDB
  (JNIEnv *env, jobject obj, jint han, jint db) {
   int ret=dlp_CleanUpDatabase(han, db);
   if (ret<0) {
       printf("Error cleaning up! %d\n",ret);
   }
   ret=dlp_CloseDB(han, db);
   if (ret<0) {
       printf("Error closing DB: %d\n",ret);
   }
}
/*ENDSync*/
JNIEXPORT void JNICALL Java_org_gnu_pilotlink_PilotLink_endSync
  (JNIEnv *env, jobject obj, jint sd) {
  dlp_EndOfSync(sd, dlpEndCodeNormal);
}
/*Close*/
JNIEXPORT void JNICALL Java_org_gnu_pilotlink_PilotLink_close
   (JNIEnv *env, jobject obj, jint sd) {
   pi_close(sd);
}


/*pilot-connect*/
int pilot_connect(char *port)
{
        int     parent_sd       = -1,   /* Client socket, formerly sd   */
                client_sd       = -1,   /* Parent socket, formerly sd2  */
                result,
                err     = 0;
        struct  pi_sockaddr addr;
        struct  stat attr;
        struct  SysInfo sys_info;
        char    *defport = "/dev/pilot";
 
        if (port == NULL && (port = getenv("PILOTPORT")) == NULL) {
                fprintf(stderr, "   No $PILOTPORT specified and no -p "
                        "<port> given.\n"
                        "   Defaulting to '%s'\n", defport);
                port = defport;
                err = stat(port, &attr);
        }
 
        if (err) {
                fprintf(stderr, "   ERROR: %s (%d)\n\n", strerror(errno), errno);
                fprintf(stderr, "   Error accessing: '%s'. Does '%s' exist?\n",
                       port, port);
                //fprintf(stderr, "   Please use --help for more information\n\n");
                exit(1);
        }
 
        if (!(parent_sd = pi_socket(PI_AF_PILOT, PI_SOCK_STREAM, PI_PF_DLP))) {
                fprintf(stderr, "\n   Unable to create socket '%s'\n",
                        port ? port : getenv("PILOTPORT"));
                return -1;
        }
 
        if (port != NULL) {
                addr.pi_family = PI_AF_PILOT;
                strncpy(addr.pi_device, port, sizeof(addr.pi_device));
                result =
                    pi_bind(parent_sd, (struct sockaddr *) &addr, sizeof(addr));
        } else {
                result = pi_bind(parent_sd, NULL, 0);
        }
 
        if (result < 0) {
                int save_errno = errno;
                char *portname;
 
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
                                        "<major> <minor>\n\n", portname);
 
                        } else if (errno == 13) {
                                fprintf(stderr, "   Please check the "
                                        "permissions on %s..\n", portname);
                                fprintf(stderr, "   Possible solution:\n\n\tchmod 0666 "
                                        "%s\n\n", portname);
 
                        } else if (errno == 19) {
                                fprintf(stderr, "   Press the HotSync button first and "
                                        "relaunch this conduit..\n\n");
                        } else if (errno == 21) {
                                fprintf(stderr, "   The port specified must contain a "
                                        "device name, and %s was a directory.\n"
                                        "   Please change that to reference a real "
                                        "device, and try again\n\n", portname);
                        }
 
                        fprintf(stderr, "   Unable to bind to port: %s\n",
                                portname);
                        fprintf(stderr, "   Please use --help for more "
                                "information\n\n");
                } else
                        fprintf(stderr, "\n   No port specified\n");
                pi_close(parent_sd);
                pi_close(client_sd);
                return -1;
        }
 
        fprintf(stderr,
                "\n   Listening to port: %s\n\n   Please press the HotSync "
                "button now... ",
                port ? port : getenv("PILOTPORT"));
 
        if (pi_listen(parent_sd, 1) == -1) {
                fprintf(stderr, "\n   Error listening on %s\n", port);
                pi_close(parent_sd);
                pi_close(client_sd);
                return -1;
        }
 
        client_sd = pi_accept(parent_sd, 0, 0);
        if (client_sd == -1) {
                fprintf(stderr, "\n   Error accepting data on %s\n", port);
                pi_close(parent_sd);
                pi_close(client_sd);
                return -1;
        }
 
        fprintf(stderr, "Connected\n\n");
 
        if (dlp_ReadSysInfo(client_sd, &sys_info) < 0) {
                fprintf(stderr, "\n   Error read system info on %s\n", port);
                pi_close(parent_sd);
                pi_close(client_sd);
                return -1;
        }
        printf("Opening counduit...\n");
        dlp_OpenConduit(client_sd);
        return client_sd;
}

