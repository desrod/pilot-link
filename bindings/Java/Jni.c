/*
 * Jni.c:  Interface pilot-link library with Java.
 *
 * Copyright (C) 2001 David.Goodenough@DGA.co.uk
 *   This code contains a rewritten version of Interface.c
 * which was Copyright (C) 1997, 1998, Keith Albanowski
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <jni.h>
#include <stdlib.h>
#include <errno.h>

#include "pi-source.h"

#include "pi-dlp.h"
#include "pi-socket.h"
#include "pi-memo.h"
#include "pi-address.h"
#include "pi-datebook.h"
#include "pi-todo.h"
#include "pi-mail.h"
#include "pi-expense.h"
#include "pi-file.h"

#include <signal.h>
#ifdef __cplusplus
extern "C" {
#endif
/*****************************/
/* Internal helper routines. */
/*****************************/
/* These first few are helpers to call methods, create objects */
/* and get and set variables in objects.                       */
static jint callIntMethod( JNIEnv*env, jobject obj, const char *method, const char *sig, ...) {
   jclass cls = env->GetObjectClass( obj);
   jmethodID m = env->GetMethodID( cls, method, sig);
   if( m == NULL) 
      {
         printf( "callIntMethod: Unable to find method %s sig %s\n", method, sig);
         return 0;
         }
   va_list args;
   va_start( args, sig);
   jint ret = env->CallIntMethodV( obj, m, args);
   va_end( args);
   return ret;
   }
static jobject callObjectMethod( JNIEnv*env, jobject obj, const char *method, const char *sig, ...) {
   jclass cls = env->GetObjectClass( obj);
   jmethodID m = env->GetMethodID( cls, method, sig);
   if( m == NULL) 
      {
         printf( "callObjectMethod: Unable to find method %s sig %s\n", method, sig);
         return NULL;
         }
   va_list args;
   va_start( args, sig);
   jobject ret = env->CallObjectMethodV( obj, m, args);
   va_end( args);
   return ret;
   }
static jobject callStaticObjectMethod( JNIEnv*env, const char *cls, const char *method, const char *sig, ...) {
   jclass jc = env->FindClass( cls);
   jmethodID m = env->GetStaticMethodID( jc, method, sig);
   if( m == NULL) 
      {
         printf( "callStaticObjectMethod: Unable to find method %s sig %s in class %s\n", method, sig, cls);
         return NULL;
         }
   va_list args;
   va_start( args, sig);
   jobject ret = env->CallStaticObjectMethodV( jc, m, args);
   va_end( args);
   return ret;
   }
static jobject newObjectV( JNIEnv*env, const char *cls, const char *sig, ...) {
   jclass jc = env->FindClass( cls);
   if( jc == NULL) 
      {
         printf( "newObjectV failed to find class %s\n", cls);
         return NULL;
         }
   jmethodID m = env->GetMethodID( jc, "<init>", sig);
   if( m == NULL) 
      {
         printf( "newObjectV for sig %s, constructor not found\n", sig);
         return NULL;
         }
   va_list args;
   va_start( args, sig);
   jobject obj = env->NewObjectV( jc, m, args);
   va_end( args);
   return obj;
   }
static jobject newObject( JNIEnv*env, const char *cls) {
   jclass jc = env->FindClass( cls);
   if( jc == NULL) 
      { 
         printf( "newObject failed to find class %s\n", cls);
         return NULL;
         }
   jmethodID m = env->GetMethodID( jc, "<init>", "()V");
   if( m == NULL) 
      {
         printf( "newObject, null constructor not found\n");
         return NULL;
         }
   jobject obj = env->NewObject( jc, m);
   return obj;
   }
static jobjectArray newObjectArray( JNIEnv*env, const char *cls, jint len) {
   jclass jc = env->FindClass( cls);
   if( jc == NULL) printf( "newObjectArray: failed to find class %s\n", cls);
      else return env->NewObjectArray( len, jc, NULL);
   return NULL;
   }
static jint getIntField( JNIEnv*env, jobject obj, const char *field) {
   jclass cls = env->GetObjectClass( obj);
   jfieldID fid = env->GetFieldID( cls, field, "I");
   if( fid == NULL) printf( "getIntField for field %s, field not found\n", field);
      else return env->GetIntField( obj, fid);
   return 0;
   }
static jboolean getBooleanField( JNIEnv*env, jobject obj, const char *field) {
   jclass cls = env->GetObjectClass( obj);
   jfieldID fid = env->GetFieldID( cls, field, "Z");
   if( fid == NULL) printf( "getBooleanField for field %s, field not found\n", field);
      else return env->GetBooleanField( obj, fid);
   return JNI_FALSE;
   }
static jobject getObjectField( JNIEnv*env, jobject obj, const char *field, const char *sig) {
   jclass cls = env->GetObjectClass( obj);
   jfieldID fid = env->GetFieldID( cls, field, sig);
   if( fid == NULL) printf( "getObjectField for field %s sig %s, field not found\n", field, sig);
      else return env->GetObjectField( obj, fid);
   return NULL;
   }
static jstring getStringField( JNIEnv*env, jobject obj, const char *field) {
   return ( jstring)getObjectField( env, obj, field, "Ljava.lang.String");
   }
static void copyString( JNIEnv*env, char *d, int len, jobject obj, const char *field) {
   jstring string = getStringField( env, obj, field);
   jboolean isCopy;
   const char * s = env->GetStringUTFChars( string, &isCopy);
   memset( d, 0, len);
   strncpy( d, s, len - 1);
   if( isCopy == JNI_TRUE) env->ReleaseStringUTFChars( string, s);
   }
static char*getStringValue( JNIEnv*env, jstring string) {
   int len = env->GetStringLength( string) + 1;
   char*ret = ( char*)malloc( len);
   jboolean isCopy;
   const char*s = env->GetStringUTFChars( string, &isCopy);
   strncpy( ret, s, len - 1);
   if( isCopy == JNI_TRUE) env->ReleaseStringUTFChars( string, s);
   return ret; // NOTE caller is responsible for freeing ret
   }
static char*getStringFieldValue( JNIEnv*env, jobject obj, const char *field) {
   jstring s = getStringField( env, obj, field);
   return getStringValue( env, s);
   }
static void setIntField( JNIEnv*env, jobject obj, const char *field, jint value) {   
   jclass cls = env->GetObjectClass( obj);
   jfieldID fid = env->GetFieldID( cls, field, "I");
   if( fid == NULL) printf( "setIntField for field %s, field not found\n", field);
      else env->SetIntField( obj, fid, value);
   }
static void setBooleanField( JNIEnv*env, jobject obj, const char *field, jboolean value) {
   jclass cls = env->GetObjectClass( obj);
   jfieldID fid = env->GetFieldID( cls, field, "Z");
   if( fid == NULL) printf( "setBooleanField for field %s, field not found\n", field);
      else env->SetBooleanField( obj, fid, value);
   }
static void setObjectField( JNIEnv*env, jobject obj, const char *field, const char *sig, jobject value) {
   jclass cls = env->GetObjectClass( obj);
   jfieldID fid = env->GetFieldID( cls, field, sig);
   if( fid == NULL) printf( "setObjectField for field %s sig %s, field not found\n", field, sig);
      else env->SetObjectField( obj, fid, value);
   }
static void setStringField( JNIEnv*env, jobject obj, const char *field, jstring value) {
   setObjectField( env, obj, field, "Ljava/lang/String;", value);
   }
static void setStringFieldValue( JNIEnv*env, jobject obj, const char *field, const char*value) {
   if( value == NULL) setStringField( env, obj, field, NULL);
      else setStringField( env, obj, field, env->NewStringUTF( value));
   }
static jint getIntArrayElement( JNIEnv*env, jintArray obj, jint index) {
   jint value;
   env->GetIntArrayRegion( obj, index, 1 ,&value);
   return value;
   }
static void setIntArrayElement( JNIEnv*env, jintArray obj, jint index, jint value) {
   jint t = value;
   env->SetIntArrayRegion( obj, index, 1, &t);
   }
static jboolean getBooleanArrayElement( JNIEnv*env, jbooleanArray obj, jint index) {
   jboolean value;
   env->GetBooleanArrayRegion( obj, index, 1, &value);
   return value;
   }
static void setBooleanArrayElement( JNIEnv*env, jbooleanArray obj, jint index, jboolean value) {
   jboolean t = value;
   env->SetBooleanArrayRegion( obj, index, 1, &t);
   }
static jobject getObjectArrayElement( JNIEnv*env, jobjectArray obj, jint index) {
   jobject value = env->GetObjectArrayElement( obj, index);
   return value;
   }
static jstring getStringArrayElement( JNIEnv*env, jobjectArray obj, jint index) {
   return ( jstring)getObjectArrayElement( env, obj, index);
   }
static void copyStringElement( JNIEnv*env, char *d, int len, jobjectArray obj, jint index) {
   jstring string = getStringArrayElement( env, obj, index);
   jboolean isCopy;
   const char * s = env->GetStringUTFChars( string, &isCopy);
   memset( d, 0, len);
   strncpy( d, s, len);
   if( isCopy == JNI_TRUE) env->ReleaseStringUTFChars( string, s);
   }
static void setFixedString( JNIEnv*env, char *to, jint len, jstring from) {
   jboolean isCopy;
   const char *s = env->GetStringUTFChars( from, &isCopy);
   memset( to, 0, len);
   strncpy( to, s, len - 1);
   if( isCopy == JNI_TRUE) env->ReleaseStringUTFChars( from, s);
   }
static jobject makeJavaRecordID( JNIEnv*env, unsigned long id) {
   if( id == 0) return NULL;
   return newObjectV( env, "org/gnu/pdapilot/RecordID", "(I)V", id);
   }
static int getRecordID( JNIEnv*env, jobject id) {
   return getIntField( env, id, "value");
   }
static int getRecordIDField( JNIEnv*env, jobject obj, const char *field) {
   jobject id = getObjectField( env, obj, field, "Lorg/gnu/pdapilot/RecordID");
   return getRecordID( env, id);
   }
static void setRecordIDField( JNIEnv*env, jobject obj, const char *field, unsigned long id) {
   setObjectField( env, 
                   obj, 
                   field, 
                   "L/org/gnu/pdapilot/RecordID;",
                   makeJavaRecordID( env, id));
   }
/**
 * This routine throws a DlpException (with code code).  
 * Remember that this routine does return normally.
 * @param env JNIEnv* pointer to the JNI environment
 * @param code int the status code to be put in the exception
 */
static void throwDlpException( JNIEnv *env, int code) {
   jobject dlpe = newObjectV( env,
                             "org.gnu.pdapilot.DlpException",
                             "(I)V",
                             code);
   env->Throw( ( jthrowable)dlpe);
   }
/**
 * This routine throws an IOException (with code code).  
 * Remember that this routine does return normally.
 * @param env JNIEnv* pointer to the JNI environment
 * @param code int the status code to be put in the exception
 */
static void throwIOException( JNIEnv *env, int code) {
   jobject ioe = newObjectV( env,
                            "java.lang.IOException",
                            "(I)V",
                            code);
   env->Throw( ( jthrowable)ioe);
   }
/**
 * This routine throws a CancelSyncException.
 * Remember that this routine does return normally.
 * @param env JNIEnv* pointer to the JNI environment
 */
static void throwCancelSyncException( JNIEnv *env) {
   jobject dlpe = newObject( env,
                             "org/gnu/pdapilot/CancelSyncException");
   env->Throw( ( jthrowable)dlpe);
   }
/**
 * This routine turns a time_t into a java.util.Date object
 * @param env JNIEnv* the JNI environment
 * @param v time_t the time to be converted
 * @returns jobject the new java.util.Date object
 */
static jobject makeJavaDate( JNIEnv *env, time_t v) {
   if( v < 18000) return 0;
      else {
         struct tm *tm = localtime(&v);
         return newObjectV( env,
                           "java/util/Date", 
                           "(IIIIII)V",
                           tm->tm_year,
                           tm->tm_mon,
                           tm->tm_mday,
                           tm->tm_hour,
                           tm->tm_min,
                           tm->tm_sec);
         }
   }
static void setDateField( JNIEnv*env, jobject obj, const char *field, time_t v) {
   jobject date = makeJavaDate( env, v);
   setObjectField( env, obj, field, "Ljava/util/Date;", date);
   }
static jobject makeDateTime( JNIEnv*env, struct tm *tm) {
   if( tm == NULL) return NULL;
   return newObjectV( env,
                     "java/util/Date", 
                     "(IIIIII)V",
                     tm->tm_year,
                     tm->tm_mon,
                     tm->tm_mday,
                     tm->tm_hour,
                     tm->tm_min,
                     tm->tm_sec);
   }
static void setDateTimeField( JNIEnv*env, jobject obj, const char *field, struct tm*v) {
   jobject date = makeDateTime( env, v);
   setObjectField( env, obj, field, "Ljava/util/Date;", date);
   }
/**
 * turns a java.util.Date object into a time_t
 * @param env JNIEnv* JNI environment pointer
 * @param date jobject java.util.Date object to be converted
 * @returns time_t the converted time
 */
static struct tm readDateTime( JNIEnv *env, jobject date) {
   struct tm tm;
   jclass cls = env->GetObjectClass( date);
   tm.tm_year = callIntMethod( env, date, "getYear", "()I");
   tm.tm_mon  = callIntMethod( env, date, "getMonth", "()I");
   tm.tm_mday = callIntMethod( env, date, "getDay", "()I");
   tm.tm_hour = callIntMethod( env, date, "getHours", "()I");
   tm.tm_min  = callIntMethod( env, date, "getMinutes", "()I");
   tm.tm_sec  = callIntMethod( env, date, "getSeconds", "()I");
   return tm;
   }
static time_t readDate( JNIEnv*env, jobject date) {
   struct tm tm = readDateTime( env, date);
   return mktime( &tm);
   }
static time_t getDateField( JNIEnv*env, jobject obj, const char*field) {
   jobject date = getObjectField( env, obj, field, "Ljava/util/Date;");
   return readDate( env, date);
   }
static struct tm getDateTimeField( JNIEnv*env, jobject obj, const char*field) {
   jobject date = getObjectField( env, obj, field, "Ljava/util/Date;");
   return readDateTime( env, date);
   }
static unsigned long getChar4( JNIEnv*env, jobject id) {
   if( id != NULL) return getIntField( env, id, "value");
   return 0;
   }
static jobject makeChar4( JNIEnv*env, unsigned long id) {
   return newObjectV( env, "org/gnu/pdapilot/Char4", "(I)V", id);
   }
static void setChar4Field( JNIEnv*env, jobject obj, const char*field, unsigned long id) {
   setObjectField( env, obj, field, "Lorg/gnu/pdapilot/Char4;", makeChar4( env, id));
   }   
static unsigned long getChar4Field( JNIEnv*env, jobject obj, const char*field) {
   jobject char4 = getObjectField( env, obj, field, "Lorg/gnu/pdapilot/Char4;");
if( char4 == NULL) printf( "getChar4Field: Unable to find Char4 field %s\n", field);
   return getChar4( env, char4);
   }
/**
 * this does the real work of converting a Category object from
 * its CategoryAppInfo structure into the variables in the 
 * CategoryAppInfo object
 * @param env JNIEnv* the JNI environment
 * @param self jobject CategoryAppInfo object to receive the data
 * @param c struct CategoryAppInfo binary source of the data
 */
static void doUnpackCategories( JNIEnv *env, jobject self, struct CategoryAppInfo *c) {
   jint i;
   setIntField( env, self, "categoryLastUniqueID", c->lastUniqueID);
   jclass stringClass = env->FindClass( "java/lang/String");
   jobjectArray names = env->NewObjectArray( 16, stringClass, NULL);
   jintArray ids = env->NewIntArray( 16);
   jbooleanArray renamed = env->NewBooleanArray( 16);
   for( i = 0; i < 16; i++) {
      env->SetObjectArrayElement( names, i, env->NewStringUTF( c->name[ i]));
      setIntArrayElement( env, ids, i, c->ID[ i]);
      setBooleanArrayElement( env, renamed, i, c->renamed[ i]);
      }
   setObjectField( env, self, "categoryName", "[Ljava/lang/String;", names);
   setObjectField( env, self, "categoryID", "[I", ids);
   setObjectField( env, self, "categoryRenamed", "[Z", renamed);
   }
static void doPackCategories( JNIEnv*env, jobject self, struct CategoryAppInfo *c) {
   int i;
   c->lastUniqueID = getIntField( env, self, "categoryLastUniqueID");
   jobjectArray names = ( jobjectArray)getObjectField( env, self, "categoryName", "[Ljava.lang.String;");
   jintArray ids = ( jintArray)getObjectField( env, self, "categoryID", "[I");
   jbooleanArray renamed = ( jbooleanArray)getObjectField( env, self, "categoryRenamed", "[Z");
   for( i = 0; i < 16; i++) {
      jstring n = getStringArrayElement( env, names, i);
      setFixedString( env, c->name[ i], 16, n);
      c->ID[i] = getIntArrayElement( env, ids, i);
      c->renamed[i] = getBooleanArrayElement( env, renamed, i);
      }
   }
static jbyteArray newByteArray( JNIEnv*env, jbyte*buf, jint len) {
   jbyteArray a = env->NewByteArray( len);
   if( a != NULL) env->SetByteArrayRegion( a, 0, len, ( jbyte *)buf);
      else printf( "newByteArray: Unable to allocate Byte Array length %d\n", len);
   return a;
   }
static jobject newRecordObj( JNIEnv*env, jbyte*buf, int len, jobject id, int attr, int cat, jobject dbClass) {
   jbyteArray a = newByteArray( env, buf, len);
   return callObjectMethod( env, 
                            dbClass, 
                            "newRecord", 
                            "([BLorg/gnu/pdapilot/RecordID;III)Lorg/gnu/pdapilot/Record;",
                            a,
                            id,
                            index,
                            attr,
                            cat);
   }
static jobject newRecord( JNIEnv*env, jbyte*buf, int len, int id, int attr, int cat, jobject dbClass) {
   return newRecordObj( env, buf, len, makeJavaRecordID( env, id), attr, cat, dbClass);
   }
static jobject newResource( JNIEnv*env, jbyte*buf, int len, jobject type, jint id, jint index, jobject dbClass) {
   jbyteArray a = newByteArray( env, buf, len);
   return callObjectMethod( env,
                            dbClass,
                            "newResource",
                            "([BLorg/gnu/pdapilot/Char4;II)Lorg/gnu/pdapilot/Resource;",
                            a, 
                            type, 
                            id, 
                            index);
   }
/***************************************/
/* Now for the real JNI interface code */
/***************************************/
/*
 * Class:     org_gnu_pdapilot_CategoryAppBlock
 * Method:    unpack
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL Java_org_gnu_pdapilot_CategoryAppBlock_unpack
  (JNIEnv *env, jobject obj, jbyteArray buf) {
   struct CategoryAppInfo c;
   jboolean isCopy;
   jbyte *b = env->GetByteArrayElements( buf, &isCopy);
   jint len = env->GetArrayLength( buf);
   unpack_CategoryAppInfo( &c, ( unsigned char *)b, len);
   doUnpackCategories( env, obj, &c);
   }
/*
 * Class:     __org_gnu_pdapilot_CategoryAppBlock
 * Method:    pack
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_gnu_pdapilot_CategoryAppBlock_pack
  (JNIEnv *env, jobject obj) {
   jbyte *buf = ( jbyte *)malloc( 0xffff);
   struct CategoryAppInfo c;
   doPackCategories( env, obj, &c);
   int len = pack_CategoryAppInfo( &c, ( unsigned char *)buf, 0xffff);
   jbyteArray result = newByteArray( env, buf, len);
   free( buf);
   return result;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    openDB
 * Signature: (IIILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Dlp_openDB
  (JNIEnv *env, jobject obj, jint socket, jint card, jint mode, jstring name) {
   jint handle;
   jboolean isCopy;
   const char *n = env->GetStringUTFChars( name, &isCopy);
   jint result = dlp_OpenDB( socket, card, mode, (char *)n, ( int *)&handle);
   if( isCopy == JNI_TRUE) env->ReleaseStringUTFChars( name, n);
   if( result < 0) throwDlpException( env, result);
   return handle;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    deleteDB
 * Signature: (IILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Dlp_deleteDB
  (JNIEnv *env, jobject obj, jint socket, jint card, jstring name) {
   jboolean isCopy;
   const char *n = env->GetStringUTFChars( name, &isCopy);
   jint result = dlp_DeleteDB( socket, card, n);
   if( isCopy == JNI_TRUE) env->ReleaseStringUTFChars( name, n);
   if( result < 0) throwDlpException( env, result);
   return result;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    endOfSync
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Dlp_endOfSync
  (JNIEnv *env, jobject obj, jint socket, jint status) {
   int result = dlp_EndOfSync( socket, status);
   if( result < 0) throwDlpException( env, result);
   return result;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    deleteCategory
 * Signature: (III)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Dlp_deleteCategory
  (JNIEnv *env, jobject obj, jint socket, jint handle, jint category) {
   int result = dlp_DeleteCategory( socket, handle, category);
   if( result < 0) throwDlpException( env, result);
   return result;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    closeDB
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Dlp_closeDB
  (JNIEnv *env, jobject obj, jint socket, jint handle) {
   int result = dlp_CloseDB( socket, handle);
   if( result < 0) throwDlpException( env, result);
   return result;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    addSyncLogEntry
 * Signature: (ILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Dlp_addSyncLogEntry
  (JNIEnv *env, jobject obj, jint socket, jstring entry) {
   jboolean isCopy;
   const char *e = env->GetStringUTFChars( entry, &isCopy);
   jint result = dlp_AddSyncLogEntry( socket, (char *)e);
   if( isCopy == JNI_TRUE) env->ReleaseStringUTFChars( entry, e);
   if( result < 0) throwDlpException( env, result);
   return result;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    strerror
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_gnu_pdapilot_Dlp_strerror
  (JNIEnv *env, jclass cls, jint error) {
   char *result = dlp_strerror( error);
   if( result == NULL) result = "Unknown DLP error";
   return env->NewStringUTF( result);
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    getSysDateTime
 * Signature: (I)Ljava/util/Date;
 */
JNIEXPORT jobject JNICALL Java_org_gnu_pdapilot_Dlp_getSysDateTime
  (JNIEnv *env, jobject obj, jint socket) {
   time_t t;
   int result = dlp_GetSysDateTime( socket, &t);
   if( result >= 0) return makeJavaDate( env, t);
   throwDlpException( env, result);
   return NULL;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    setSysDateTime
 * Signature: (ILjava/util/Date;)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Dlp_setSysDateTime
  (JNIEnv *env, jobject obj, jint socket, jobject date) {
   time_t t = readDate( env, date);
   jint result = dlp_SetSysDateTime( socket, t);
   if( result < 0) throwDlpException( env, result);
   return result;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    readAppBlock
 * Signature: (IILorg/gnu/pdapilot/Database;)Lorg/gnu/pdapilot/AppBlock;
 */
JNIEXPORT jobject JNICALL Java_org_gnu_pdapilot_Dlp_readAppBlock
  (JNIEnv *env, jobject obj, jint socket, jint handle, jobject dbClass) {
   jbyte*buf = ( jbyte*)malloc( 0xffff);
   int len;
   jobject ret = NULL;
   int result = dlp_ReadAppBlock( socket, handle, 0, buf, 0xffff);
   if( result > 0)
      {  // good result, now build the AppBlock from the byte array
         len = result;
         jbyteArray a = newByteArray( env, buf, len);
         ret = callObjectMethod( env,
                                 dbClass,
                                 "newAppBlock", 
                                 "([B)Lorg/gnu/pdapilot/AppBlock;",
                                 a);
         }
      else if( ( result != -5) &&
               ( result != 0)   ) throwDlpException( env, result);
   free( buf);
   return ret;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    readSortBlock
 * Signature: (IILorg/gnu/pdapilot/Database;)Lorg/gnu/pdapilot/SortBlock;
 */
JNIEXPORT jobject JNICALL Java_org_gnu_pdapilot_Dlp_readSortBlock
  (JNIEnv *env, jobject obj, jint socket, jint handle, jobject dbClass) {
   jbyte*buf = ( jbyte*)malloc( 0xffff);
   int len;
   jobject ret = NULL;
   int result = dlp_ReadSortBlock( socket, handle, 0, buf, 0xffff);
   if( result > 0)
      {  // good result, now build the SortBlock from the byte array
         len = result;
         jbyteArray a = newByteArray( env, buf, len);
         ret = callObjectMethod( env,
                                 dbClass,
                                 "newSortBlock", 
                                 "([B)Lorg/gnu/pdapilot/SortBlock;",
                                 a);
         }
      else if( ( result != -5) &&
               ( result != 0)   ) throwDlpException( env, result);
   free( buf);
   return ret;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    readAppPreference
 * Signature: (ILorg/gnu/pdapilot/Char4;IZLorg/gnu/pdapilot/Database;)Lorg/gnu/pdapilot/Pref;
 */
JNIEXPORT jobject JNICALL Java_org_gnu_pdapilot_Dlp_readAppPreference
  (JNIEnv *env, jobject obj, jint socket, jobject creator, jint id, jboolean backup, jobject dbClass) {
   jbyte*buf = ( jbyte*)malloc( 0xffff);
   int len, version;
   jobject ret = NULL;
   int result = dlp_ReadAppPreference( socket, 
                                       getChar4( env, creator), 
                                       id, 
                                       backup, 
                                       0x0ffff,
                                       buf, 
                                       &len, 
                                       &version);
   if( result > 0)
      {  // good result, now build the Pref from the byte array
         len = result;
         jbyteArray a = newByteArray( env, buf, len);
         ret = callObjectMethod( env,
                                 dbClass,
                                 "newPref", 
                                 "([BLorg/gnu/pdapilot/Char4;IIZ)Lorg/gnu/pdapilot/Pref;",
                                 a, 
                                 creator, 
                                 id, 
                                 version, 
                                 backup);
         }
      else if( ( result != -5) &&
               ( result != 0)   ) throwDlpException( env, result);
   free( buf);
   return ret;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    readRecordByIndex
 * Signature: (IIILorg/gnu/pdapilot/Database;)Lorg/gnu/pdapilot/Record;
 */
JNIEXPORT jobject JNICALL Java_org_gnu_pdapilot_Dlp_readRecordByIndex
  (JNIEnv *env, jobject obj, jint socket, jint handle, jint index, jobject dbClass) {
   jbyte*buf = ( jbyte*)malloc( 0xffff);
   int len, attr, cat;
   recordid_t id;
   jobject ret = NULL;
   int result = dlp_ReadRecordByIndex( socket, 
                                       handle,
                                       index, 
                                       buf, 
                                       &id,
                                       &len, 
                                       &attr, 
                                       &cat);
   if( result > 0) ret = newRecord( env, buf, len, id, attr, cat, dbClass);
      else if( ( result != -5) &&
               ( result != 0)   ) throwDlpException( env, result);
   free( buf);
   return ret;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    readNextModifiedRec
 * Signature: (IILorg/gnu/pdapilot/Database;)Lorg/gnu/pdapilot/Record;
 */
JNIEXPORT jobject JNICALL Java_org_gnu_pdapilot_Dlp_readNextModifiedRec
  (JNIEnv *env, jobject obj, jint socket, jint handle, jobject dbClass) {
   jbyte*buf = ( jbyte*)malloc( 0xffff);
   int len, attr, cat, index;
   recordid_t id;
   jobject ret = NULL;
   int result = dlp_ReadNextModifiedRec( socket, 
                                         handle,
                                         buf, 
                                         &id, 
                                         &index,
                                         &len, 
                                         &attr, 
                                         &cat);
   if( result > 0)
      {  // good result, now build the Pref from the byte array
         jbyteArray a = newByteArray( env, buf, len);
         ret = callObjectMethod( env, 
                                 dbClass,
                                 "newRecord", 
                                 "([BLorg/gnu/pdapilot/RecordID;III)Lorg/gnu/pdapilot/Record;",
                                 a, 
                                 makeJavaRecordID( env, id), 
                                 index, 
                                 attr, 
                                 cat);
         }
      else if( ( result != -5) &&
               ( result != 0)   ) throwDlpException( env, result);
   free( buf);
   return ret;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    readNextModifiedRecInCategory
 * Signature: (IIILorg/gnu/pdapilot/Database;)Lorg/gnu/pdapilot/Record;
 */
JNIEXPORT jobject JNICALL Java_org_gnu_pdapilot_Dlp_readNextModifiedRecInCategory
  (JNIEnv *env, jobject obj, jint socket, jint handle, jint cat, jobject dbClass) {
   jbyte*buf = ( jbyte*)malloc( 0xffff);
   int len, attr, index;
   recordid_t id;
   jobject ret = NULL;
   int result = dlp_ReadNextModifiedRecInCategory( socket, 
                                                   handle,
                                                   cat,
                                                   buf, 
                                                   &id, 
                                                   &index,
                                                   &len, 
                                                   &attr); 
   if( result > 0)  ret = newRecord( env, buf, len, id, attr, cat, dbClass);
      else if( ( result != -5) &&
               ( result != 0)   ) throwDlpException( env, result);
   free( buf);
   return ret;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    readNextRecInCategory
 * Signature: (IIILorg/gnu/pdapilot/Database;)Lorg/gnu/pdapilot/Record;
 */
JNIEXPORT jobject JNICALL Java_org_gnu_pdapilot_Dlp_readNextRecInCategory
  (JNIEnv*env, jobject obj, jint socket, jint handle, jint cat, jobject dbClass) {
   jbyte*buf = ( jbyte*)malloc( 0xffff);
   int len, attr, index;
   recordid_t id;
   jobject ret = NULL;
   int result = dlp_ReadNextRecInCategory( socket, 
                                           handle,
                                           cat,
                                           buf, 
                                           &id, 
                                           &index,
                                           &len, 
                                           &attr); 
   if( result > 0)  ret = newRecord( env, buf, len, id, attr, cat, dbClass);
      else if( ( result != -5) &&
               ( result != 0)   ) throwDlpException( env, result);
   free( buf);
   return ret;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    readRecordByID
 * Signature: (IILorg/gnu/pdapilot/RecordID;Lorg/gnu/pdapilot/Database;)Lorg/gnu/pdapilot/Record;
 */
JNIEXPORT jobject JNICALL Java_org_gnu_pdapilot_Dlp_readRecordByID
  (JNIEnv*env, jobject obj, jint socket, jint handle, jobject id, jobject dbClass) {
   jbyte*buf = ( jbyte*)malloc( 0xffff);
   int len, attr, index, cat;
   jobject ret = NULL;
   int result = dlp_ReadRecordById( socket, 
                                    handle,
                                    getRecordID( env, id),
                                    buf, 
                                    &index,
                                    &len, 
                                    &attr,
                                    &cat); 
   if( result > 0)  ret = newRecordObj( env, buf, len, id, attr, cat, dbClass);
      else if( ( result != -5) &&
               ( result != 0)   ) throwDlpException( env, result);
   free( buf);
   return ret;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    readResourceByType
 * Signature: (IILorg/gnu/pdapilot/Char4;ILorg/gnu/pdapilot/Database;)Lorg/gnu/pdapilot/Resource;
 */
JNIEXPORT jobject JNICALL Java_org_gnu_pdapilot_Dlp_readResourceByType
  (JNIEnv*env, jobject obj, jint socket, jint handle, jobject type, jint id, jobject dbClass) {
   jbyte*buf = ( jbyte*)malloc( 0xffff);
   int len, index;
   jobject ret = NULL;
   int result = dlp_ReadResourceByType( socket, 
                                        handle,
                                        getIntField( env, type, "value"),
                                        id,
                                        buf, 
                                        &index,
                                        &len); 
   if( result > 0)  ret = newResource( env, buf, len, type, id, index, dbClass);
      else if( ( result != -5) &&
               ( result != 0)   ) throwDlpException( env, result);
   free( buf);
   return ret;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    readResourceByIndex
 * Signature: (IIILorg/gnu/pdapilot/Database;)Lorg/gnu/pdapilot/Resource;
 */
JNIEXPORT jobject JNICALL Java_org_gnu_pdapilot_Dlp_readResourceByIndex
  (JNIEnv*env, jobject obj, jint socket, jint handle, jint index, jobject dbClass) {
   jbyte*buf = ( jbyte*)malloc( 0xffff);
   int len, id;
   unsigned long type;
   jobject ret = NULL;
   int result = dlp_ReadResourceByIndex( socket, 
                                         handle,
                                         index,
                                         buf,
                                         &type, 
                                         &id,
                                         &len); 
   if( result > 0)  ret = newResource( env, 
                                       buf, 
                                       len, 
                                       makeChar4( env, type), 
                                       id, 
                                       index, 
                                       dbClass);
      else if( ( result != -5) &&
               ( result != 0)   ) throwDlpException( env, result);
   free( buf);
   return ret;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    writeRecord
 * Signature: (IILorg/gnu/pdapilot/Record;)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Dlp_writeRecord
  (JNIEnv*env, jobject obj, jint socket, jint handle, jobject record) {
   int attr, cat, len, result;
   jbyte *buffer;
   recordid_t id;
   jboolean isCopy;
   jbyteArray b = ( jbyteArray)callObjectMethod( env, record, "pack", "(){B");
   if( b == NULL) return 0;
   id = getRecordID( env, record);
   attr = 0;
   attr |= getBooleanField( env, record, "deleted") ?  0x80 : 0;
   attr |= getBooleanField( env, record, "modified") ? 0x40 : 0;
   attr |= getBooleanField( env, record, "busy") ?     0x20 : 0;
   attr |= getBooleanField( env, record, "secret") ?   0x10 : 0;
   attr |= getBooleanField( env, record, "archived") ? 0x08 : 0;
   cat = getIntField( env, record, "category");
   buffer = env->GetByteArrayElements( b, &isCopy);
   len = env->GetArrayLength( b);
   result = dlp_WriteRecord( socket, handle, attr, id, cat, buffer, len, &id);
   if( isCopy == JNI_TRUE) env->ReleaseByteArrayElements( b, buffer, JNI_ABORT);
   if( result >= 0) return id;
   throwDlpException( env, result);
   return 0;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    writeAppPreference
 * Signature: (ILorg/gnu/pdapilot/Pref;)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Dlp_writeAppPreference
  (JNIEnv*env, jobject obj, jint socket, jobject pref) {
   jbyte *buffer;
   int len, id, version, result, backup;
   jint creator;
   jboolean isCopy;
   jbyteArray b = ( jbyteArray)callObjectMethod( env, pref, "pack", "(){B");
   if( b == NULL) return 0;
   creator = getChar4( env, getObjectField( env, pref, "creator", "Lorg/gnu/pdapilot/Char4"));
   id = getIntField( env, pref, "id");
   version = getIntField( env, pref, "version");
   backup = getIntField( env, pref, "backup");
   buffer = env->GetByteArrayElements( b, &isCopy);
   len = env->GetArrayLength( b);
   result = dlp_WriteAppPreference( socket, creator, id, backup, version, buffer, len);
   if( isCopy == JNI_TRUE) env->ReleaseByteArrayElements( b, buffer, JNI_ABORT);
   if( result >= 0) return id;
   throwDlpException( env, result);
   return 0;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    writeResource
 * Signature: (IILorg/gnu/pdapilot/Resource;)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Dlp_writeResource
  (JNIEnv*env, jobject obj, jint socket, jint handle, jobject resource) {
   jbyte *buffer;
   int len, result;
   jint type, id;
   jboolean isCopy;
   jbyteArray b = ( jbyteArray)callObjectMethod( env, resource, "pack", "(){B");
   if( b == NULL) return 0;
   type = getChar4( env, getObjectField( env, resource, "type", "Lorg/gnu/pdapilot/Char4"));
   id = getIntField( env, resource, "id");
   buffer = env->GetByteArrayElements( b, &isCopy);
   len = env->GetArrayLength( b);
   result = dlp_WriteResource( socket, handle, type, id, buffer, len);
   if( isCopy == JNI_TRUE) env->ReleaseByteArrayElements( b, buffer, JNI_ABORT);
   if( result >= 0) return id;
   throwDlpException( env, result);
   return 0;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    writeAppBlock
 * Signature: (IILorg/gnu/pdapilot/AppBlock;)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Dlp_writeAppBlock
  (JNIEnv*env, jobject obj, jint socket, jint handle, jobject appblock) {
   jbyte *buffer;
   int len, result;
   jboolean isCopy;
   jbyteArray b = ( jbyteArray)callObjectMethod( env, appblock, "pack", "(){B");
   if( b == NULL) return 0;
   buffer = env->GetByteArrayElements( b, &isCopy);
   len = env->GetArrayLength( b);
   result = dlp_WriteAppBlock( socket, handle, buffer, len);
   if( isCopy == JNI_TRUE) env->ReleaseByteArrayElements( b, buffer, JNI_ABORT);
   if( result < 0) throwDlpException( env, result);
   return 0;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    writeSortBlock
 * Signature: (IILorg/gnu/pdapilot/SortBlock;)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Dlp_writeSortBlock
  (JNIEnv*env, jobject obj, jint socket, jint handle, jobject sortblock) {
   jbyte *buffer;
   int len, result;
   jboolean isCopy;
   jbyteArray b = ( jbyteArray)callObjectMethod( env, sortblock, "pack", "(){B");
   if( b == NULL) return 0;
   buffer = env->GetByteArrayElements( b, &isCopy);
   len = env->GetArrayLength( b);
   result = dlp_WriteSortBlock( socket, handle, buffer, len);
   if( isCopy == JNI_TRUE) env->ReleaseByteArrayElements( b, buffer, JNI_ABORT);
   if( result < 0) throwDlpException( env, result);
   return 0;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    readStorageInfo
 * Signature: (II)Lorg/gnu/pdapilot/CardInfo;
 */
JNIEXPORT jobject JNICALL Java_org_gnu_pdapilot_Dlp_readStorageInfo
  (JNIEnv*env, jobject obj, jint socket, jint card) {
   jobject output = NULL;
   struct CardInfo c;
   int result = dlp_ReadStorageInfo( socket, card, &c);
   if( ( result == -5) ||
       ( result == 0)   ) return 0;
      else if( result < 0) {
         throwDlpException( env, result);
         return 0;
         }
   output = newObject( env, "org/gnu/pdapilot/CardInfo");
   setStringField( env, output, "name", env->NewStringUTF( c.name));
   setStringField( env, output, "manufacturer", env->NewStringUTF( c.manufacturer));
   setIntField( env, output, "card", c.card);
   setIntField( env, output, "romSize", c.romSize);
   setIntField( env, output, "ramSize", c.ramSize);
   setIntField( env, output, "ramFree", c.ramFree);
   setBooleanField( env, output, "more", c.more);
   setDateField( env, output, "creation", c.creation);
   return output;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    createDB
 * Signature: (ILorg/gnu/pdapilot/Char4;Lorg/gnu/pdapilot/Char4;IIILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Dlp_createDB
  (JNIEnv*env, jobject obj, jint socket, jobject creator, jobject type, jint card, jint flags, jint version, jstring name) {
   int handle;
   jboolean isCopy;
   const char *n = env->GetStringUTFChars( name, &isCopy);
   int result = dlp_CreateDB( socket, 
                              getChar4( env, creator),
                              getChar4( env, creator), // why??
                              card,
                              flags,
                              version,
                              n,
                              &handle);
   if( isCopy == JNI_TRUE) env->ReleaseStringUTFChars( name, n);
   if( result >= 0) return handle;
   throwDlpException( env, result);
   return result;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    resetSystem
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Dlp_resetSystem
  (JNIEnv*env, jobject obj, jint socket) {
   int result = dlp_ResetSystem( socket);
   if( result < 0) throwDlpException( env, result);
   return result;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    openConduit
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Dlp_openConduit
  (JNIEnv*env, jobject obj, jint socket) {
   int result = dlp_OpenConduit( socket);
   if( result == dlpErrSync) throwCancelSyncException( env);
      else if( result < 0) throwDlpException( env, result);
   return result;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    readUserInfo
 * Signature: (I)Lorg/gnu/pdapilot/UserInfo;
 */
JNIEXPORT jobject JNICALL Java_org_gnu_pdapilot_Dlp_readUserInfo
  (JNIEnv*env, jobject obj, jint socket) {
   jobject output;
   struct PilotUser u;
   int result = dlp_ReadUserInfo( socket, &u);
   output = newObject( env, "org/gnu/pdapilot/UserInfo");
   setStringField( env, output, "username", env->NewStringUTF( u.username));
   setIntField( env, output, "userID", u.userID);
   setIntField( env, output, "viewerID", u.viewerID);
   setIntField( env, output, "lastSyncPC", u.lastSyncPC);
   jbyteArray pwd = newByteArray( env, (jbyte*)u.password, u.passwordLength);
   setObjectField( env, output, "password", "[B", pwd);
   setDateField( env, output, "successfulSyncDate", u.successfulSyncDate);
   setDateField( env, output, "lastSyncDate", u.lastSyncDate);
   return output;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    writeUserInfo
 * Signature: (ILorg/gnu/pdapilot/UserInfo;)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Dlp_writeUserInfo
  (JNIEnv*env, jobject obj, jint socket, jobject user) {
   struct PilotUser u;
   int result;
   jboolean isCopy;
   jbyte*p;
   const char*username;
   u.userID = getIntField( env, user, "userID");
   u.viewerID = getIntField( env, user, "viewerID");
   u.lastSyncPC = getIntField( env, user, "lastSyncPC");
   jbyteArray n = ( jbyteArray)getObjectField( env, user, "password", "[B");
   u.passwordLength = env->GetArrayLength( n);
   p = env->GetByteArrayElements( n, &isCopy);
   memcpy( u.password, p, u.passwordLength);
   if( isCopy == JNI_TRUE) env->ReleaseByteArrayElements( n, p, JNI_ABORT);
   u.lastSyncDate = getDateField( env, user, "lastSyncDate");
   u.successfulSyncDate = getDateField( env, user, "successfulSyncDate");
   jstring s = getStringField( env, user, "username");
   username = env->GetStringUTFChars( s, &isCopy);
   strcpy( u.username, username);
   if( isCopy == JNI_TRUE) env->ReleaseStringUTFChars( s, username);
   result = dlp_WriteUserInfo( socket, &u);
   if( result < 0) throwDlpException( env, result);
   return result;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    readSysInfo
 * Signature: (I)Lorg/gnu/pdapilot/SysInfo;
 */
JNIEXPORT jobject JNICALL Java_org_gnu_pdapilot_Dlp_readSysInfo
  (JNIEnv*env, jobject obj, jint socket) {
   jobject output;
   struct SysInfo s;
   int result = dlp_ReadSysInfo( socket, &s);
   output = newObject( env, "org/gnu/pdapilot/SysInfo");
   setIntField( env, output, "romVersion", s.romVersion);
   setIntField( env, output, "locale", s.locale);
   // is s.name 0 delimited, the old code used s.namelength?
   setStringField( env, output, "name", env->NewStringUTF( s.prodID));
   return output;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    readNetSyncInfo
 * Signature: (I)Lorg/gnu/pdapilot/NetInfo;
 */
JNIEXPORT jobject JNICALL Java_org_gnu_pdapilot_Dlp_readNetSyncInfo
  (JNIEnv*env, jobject obj, jint socket) {
   jobject output = NULL;
   struct NetSyncInfo i;
   int result = dlp_ReadNetSyncInfo( socket, &i);
   if( result < 0) 
      {
         throwDlpException( env, result);
         return 0;
         }
   output = newObject( env, "/org/gnu/pdapilot/NetInfo");
   setIntField( env, output, "lanSync", i.lanSync);
   setStringField( env, output, "hostName", env->NewStringUTF( i.hostName));
   setStringField( env, output, "hostAddress", env->NewStringUTF( i.hostAddress));
   setStringField( env, output, "hostSubnetMask", env->NewStringUTF( i.hostSubnetMask));
   return output;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    writeNetSyncInfo
 * Signature: (ILorg/gnu/pdapilot/NetInfo;)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Dlp_writeNetSyncInfo
  (JNIEnv*env, jclass obj, jint socket, jobject info) {
   struct NetSyncInfo i;
   int result;
   i.lanSync = getIntField( env, info, "lanSync");
   copyString( env, i.hostName, 256, info, "hostName");
   copyString( env, i.hostAddress, 40, info, "hostAddress");
   copyString( env, i.hostSubnetMask, 40, info, "hostSubnetMast");
   result = dlp_WriteNetSyncInfo( socket, &i);
   if( result < 0) throwDlpException( env, result);
   return result;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    readDBList
 * Signature: (IIII)Lorg/gnu/pdapilot/DBInfo;
 */
JNIEXPORT jobject JNICALL Java_org_gnu_pdapilot_Dlp_readDBList
  (JNIEnv*env, jobject obj, jint socket, jint card, jint flags, jint start) {
   jobject output = NULL;
   struct DBInfo i;
   int result = dlp_ReadDBList( socket, card, flags, start, &i);
   if( ( result == -5) ||
       ( result == 0)   ) return 0;
      else if( result < 0) {
         throwDlpException( env, result);
         return 0;
         }
   output = newObject( env, "org/gnu/pdapilot/DBInfo");
   setBooleanField( env, output, "flagReadOnly", !!( i.flags & dlpDBFlagReadOnly));
   setBooleanField( env, output, "flagResource", !!( i.flags & dlpDBFlagResource));
   setBooleanField( env, output, "flagBackup", !!( i.flags & dlpDBFlagBackup));
   setBooleanField( env, output, "flagOpen", !!( i.flags & dlpDBFlagOpen));
   setBooleanField( env, output, "flagAppInfoDirty", !!( i.flags & dlpDBFlagAppInfoDirty));
   setBooleanField( env, output, "flagNewer", !!( i.flags & dlpDBFlagNewer));
   setBooleanField( env, output, "flagReset", !!( i.flags & dlpDBFlagReset));
   setBooleanField( env, output, "flagCopyPrevention", !!( i.flags & dlpDBFlagCopyPrevention));
   setBooleanField( env, output, "flagStream", !!( i.flags & dlpDBFlagStream));
   setBooleanField( env, output, "flagExcludeFromSync", !!( i.miscFlags & dlpDBMiscFlagExcludeFromSync));
   setIntField( env, output, "index", i.index);
   setIntField( env, output, "version", i.version);
   setIntField( env, output, "modnum", i.modnum);
   setChar4Field( env, output, "type", i.type);
   setChar4Field( env, output, "creator", i.creator);
   setDateField( env, output, "createDate", i.createDate);
   setDateField( env, output, "modifyDate", i.modifyDate);
   setDateField( env, output, "backupDate", i.backupDate);
   setStringField( env, output, "name", env->NewStringUTF( i.name));
   setIntField( env, output, "card", card);
   setBooleanField( env, output, "more", i.more);
   return output; 
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    cleanUpDatabase
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Dlp_cleanUpDatabase
  (JNIEnv*env, jobject obj, jint socket, jint handle) {
   int result = dlp_CleanUpDatabase( socket, handle);
   if( result < 0) throwDlpException( env, result);
   return result;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    resetSyncFlags
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Dlp_resetSyncFlags
  (JNIEnv*env, jobject obj, jint socket, jint handle) {
   int result = dlp_ResetSyncFlags( socket, handle);
   if( result < 0) throwDlpException( env, result);
   return result;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    moveCategory
 * Signature: (IIII)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Dlp_moveCategory
  (JNIEnv*env, jobject obj, jint socket, jint handle, jint from, jint to) {
   int result = dlp_MoveCategory( socket, handle, from, to);
   if( result < 0) throwDlpException( env, result);
   return result;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    deleteRecord
 * Signature: (IIZLorg/gnu/pdapilot/RecordID;)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Dlp_deleteRecord
  (JNIEnv*env, jobject obj, jint socket, jint handle, jint all, jobject id) {
   int result = dlp_DeleteRecord( socket, handle, all, getRecordID( env, id));
   if( result < 0) throwDlpException( env, result);
   return result;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    deleteResource
 * Signature: (IIZLorg/gnu/pdapilot/Char4;I)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Dlp_deleteResource
  (JNIEnv*env, jobject obj, jint socket, jint handle, jboolean all, jobject type, jint id) {
   int result = dlp_DeleteResource( socket, handle, all, getChar4( env, type), id);
   if( result < 0) throwDlpException( env, result);
   return result;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    readOpenDBInfo
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Dlp_readOpenDBInfo
  (JNIEnv*env, jobject obj, jint socket, jint handle) {
   int count;
   int result = dlp_ReadOpenDBInfo( socket, handle, &count);
   if( result < 0) throwDlpException( env, result);
   return count;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    readFeature
 * Signature: (ILorg/gnu/pdapilot/Char4;I)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Dlp_readFeature
  (JNIEnv*env, jobject obj, jint socket, jobject creator, jint id) {
   unsigned long feature;
   int result = dlp_ReadFeature( socket, getChar4( env, creator), id, &feature);
   if( result < 0) throwDlpException( env, result);
   return feature;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    readRecordIDList
 * Signature: (IIZII)[Lorg/gnu/pdapilot/RecordID;
 */
JNIEXPORT jobjectArray JNICALL Java_org_gnu_pdapilot_Dlp_readRecordIDList
  (JNIEnv*env, jobject obj, jint socket, jint handle, jboolean sort, jint start, jint max) {
   recordid_t *l = ( recordid_t *)malloc( sizeof( recordid_t) * max);
   int count;
   int result;
   jobjectArray output = NULL;
   int i;
   result = dlp_ReadRecordIDList( socket, handle, sort, start, max, l, &count);
   if( result < 0) throwDlpException( env, result);
      else {
         output = newObjectArray( env, "org/gnu/pdapilot/RecordID", count);
         if( output != NULL)
            {
               for( i = 0; i < count; i++) {
                  env->SetObjectArrayElement( output, 
                                              i, 
                                              makeJavaRecordID( env, l[ i]));
                  }
               }
         }
   free( l);
   return output;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    callApplication
 * Signature: (ILorg/gnu/pdapilot/Char4;II[B[I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_gnu_pdapilot_Dlp_callApplication
  (JNIEnv*env, jobject obj, jint socket, jobject creator, jint type, jint action, jbyteArray outgoing_data, jintArray retcode) {
   jbyte *buffer = ( jbyte *)malloc( 0xffff);
   int len;
   unsigned long ret;
   jbyteArray incomming_data;
   jboolean isCopy;
   jbyte *out = env->GetByteArrayElements( outgoing_data, &isCopy);
   int result = dlp_CallApplication( socket, 
                                     getChar4( env, creator),
                                     type,
                                     action,
                                     env->GetArrayLength( outgoing_data),
                                     out,
                                     &ret,
                                     0xffff,
                                     &len,
                                     buffer);
   if( isCopy == JNI_TRUE) env->ReleaseByteArrayElements( outgoing_data, out, JNI_ABORT);
   if( result > 0)
      {
         incomming_data = newByteArray( env, buffer, len);
         jintArray array = ( jintArray)getObjectField( env, retcode, "body", "[I");
         setIntArrayElement( env, array, 0, result);
         }
      else if( ( result != -5) &&
               ( result != 0)   ) throwDlpException( env, result);
   free( buffer);
   return incomming_data;
   }
/*
 * Class:     __org_gnu_pdapilot_Dlp
 * Method:    resetDBIndex
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Dlp_resetDBIndex
  (JNIEnv*env, jobject obj, jint socket, jint handle) {
   int result = dlp_ResetDBIndex( socket, handle);
   if( result < 0) throwDlpException( env, result);
   return result;
   }
/*
 * Class:     __org_gnu_pdapilot_File
 * Method:    pi_file_create
 * Signature: (Ljava/lang/String;Lorg/gnu/pdapilot/DBInfo;)V
 */
JNIEXPORT void JNICALL Java_org_gnu_pdapilot_File_pi_1file_1create
  (JNIEnv*env, jobject obj, jstring name, jobject info) {
   struct pi_file *pf;
   struct DBInfo dbInfo;
   dbInfo.flags = getBooleanField( env, info, "flagReadOnly") ? dlpDBFlagReadOnly : 0 |
                  getBooleanField( env, info, "flagResource") ? dlpDBFlagResource : 0 |
                  getBooleanField( env, info, "flagBackup") ? dlpDBFlagBackup : 0 |
                  getBooleanField( env, info, "flagOpen") ? dlpDBFlagOpen : 0 |
                  getBooleanField( env, info, "flagAppInfoDirty") ? dlpDBFlagAppInfoDirty : 0 |
                  getBooleanField( env, info, "flagNewer") ? dlpDBFlagNewer : 0 |
                  getBooleanField( env, info, "flagReset") ? dlpDBFlagReset : 0 |
                  getBooleanField( env, info, "flagCopyPrevention") ? dlpDBFlagCopyPrevention : 0 |
                  getBooleanField( env, info, "flagStream") ? dlpDBFlagStream : 0;
   dbInfo.miscFlags = getBooleanField( env, info, "flagExcludeFromSync") ? dlpDBMiscFlagExcludeFromSync : 0;
   dbInfo.version = getIntField( env, info, "version");
   dbInfo.modnum = getIntField( env, info, "modnum");
   dbInfo.type = getChar4Field( env, info, "type");
   dbInfo.creator = getChar4Field( env, info, "creator");
   dbInfo.createDate = getDateField( env, info, "createDate");
   dbInfo.modifyDate = getDateField( env, info, "modifyDate");
   dbInfo.backupDate = getDateField( env, info, "backupDate");
   copyString( env, dbInfo.name, 34, info, "name");
   setIntField( env, obj, "_pf", 0);
   pf = pi_file_create( dbInfo.name, &dbInfo);
   if( pf == NULL) 
      {
         throwIOException( env, errno);
         return;
         }
   setIntField( env, obj, "_pf", ( int)pf);
   }
/*
 * Class:     __org_gnu_pdapilot_File
 * Method:    pi_file_open
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_gnu_pdapilot_File_pi_1file_1open
  (JNIEnv*env, jobject file, jstring name) {
   char*n = getStringValue( env, name);
   free( n);
   struct pi_file *pf = pi_file_open( n);
   setIntField( env, file, "_pf", ( jint)pf);
   if( pf == NULL) throwIOException( env, errno);
   }
/*
 * Class:     __org_gnu_pdapilot_File
 * Method:    pi_file_read_record
 * Signature: (ILorg/gnu/pdapilot/Database;)Lorg/gnu/pdapilot/Record;
 */
JNIEXPORT jobject JNICALL Java_org_gnu_pdapilot_File_pi_1file_1read_1record
  (JNIEnv*env, jobject file, jint index, jobject dbClass) {
   struct pi_file *pf = ( struct pi_file *)getIntField( env, file, "_pf");
   int attr, cat, len;
   recordid_t id;
   jbyte*buffer;
   jobject output = NULL;
   int result = pi_file_read_record( pf, index, ( void **)&buffer, &len, &attr, &cat, &id);
   if( result >= 0) 
      {
         jbyteArray a = newByteArray( env, buffer, len);
         output = callObjectMethod( env, 
                                    dbClass, 
                                    "newRecord", 
                                    "([BLorg/gnu/pdapilot/RecordID;III)Lorg/gnu/pdapilot/Record;",
                                    a,
                                    makeJavaRecordID( env, id),
                                    index, 
                                    attr,
                                    cat);
         }
      else {
         throwIOException( env, errno);
         return 0;
         }
   return output;
   }
/*
 * Class:     __org_gnu_pdapilot_File
 * Method:    pi_file_read_record_by_id
 * Signature: (Lorg/gnu/pdapilot/RecordID;Lorg/gnu/pdapilot/Database;)Lorg/gnu/pdapilot/Record;
 */
JNIEXPORT jobject JNICALL Java_org_gnu_pdapilot_File_pi_1file_1read_1record_1by_1id
  (JNIEnv*env, jobject file, jobject jid, jobject dbClass) {
   struct pi_file *pf = ( struct pi_file *)getIntField( env, file, "_pf");
   int attr, cat, len, index;
   recordid_t id = getRecordID( env, jid);
   jbyte*buffer;
   jobject output = NULL;
   int result = pi_file_read_record_by_id( pf, id, ( void **)&buffer, &len, &index, &attr, &cat);
   if( result >= 0) 
      {
         jbyteArray a = newByteArray( env, buffer, len);
         output = callObjectMethod( env, 
                                    dbClass, 
                                    "newRecord", 
                                    "([BLorg/gnu/pdapilot/RecordID;III)Lorg/gnu/pdapilot/Record;",
                                    a,
                                    makeJavaRecordID( env, id),
                                    index, 
                                    attr,
                                    cat);
         }
      else {
         throwIOException( env, errno);
         return 0;
         }
   return output;
   }
/*
 * Class:     __org_gnu_pdapilot_File
 * Method:    pi_file_read_resource
 * Signature: (ILorg/gnu/pdapilot/Database;)Lorg/gnu/pdapilot/Resource;
 */
JNIEXPORT jobject JNICALL Java_org_gnu_pdapilot_File_pi_1file_1read_1resource
  (JNIEnv*env, jobject file, jint index, jobject dbClass) {
   struct pi_file *pf = ( struct pi_file *)getIntField( env, file, "_pf");
   int len, id;
   unsigned long type;
   jbyte*buffer;
   jobject output = NULL;
   int result = pi_file_read_resource( pf, index, ( void **)&buffer, &len, &type, &id);
   if( result >= 0) 
      {
         jbyteArray a = newByteArray( env, buffer, len);
         output = callObjectMethod( env, 
                                    dbClass, 
                                    "newRecord", 
                                    "([BLorg/gnu/pdapilot/Char4;II)Lorg/gnu/pdapilot/Resource;",
                                    a,
                                    type,
                                    id,
                                    index);
         }
      else {
         throwIOException( env, errno);
         return 0;
         }
   return output;
   }

/*
 * Class:     __org_gnu_pdapilot_File
 * Method:    pi_file_get_entries
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_File_pi_1file_1get_1entries
  (JNIEnv*env, jobject file) {
   struct pi_file *pf = ( struct pi_file *)getIntField( env, file, "_pf");
   int entries;
   int result = pi_file_get_entries( pf, &entries);
   if( result >= 0) return entries;
   throwIOException( env, errno);
   return 0;
   }
/*
 * Class:     __org_gnu_pdapilot_File
 * Method:    pi_file_get_info
 * Signature: ()Lorg/gnu/pdapilot/DBInfo;
 */
JNIEXPORT jobject JNICALL Java_org_gnu_pdapilot_File_pi_1file_1get_1info
  (JNIEnv*env, jobject file) {
   struct pi_file *pf = ( struct pi_file*)getIntField( env, file, "_pf");
   jobject output;
   struct DBInfo i;
   if( pi_file_get_info( pf, &i) < 0) 
      {
         throwIOException( env, errno);
         return 0;
         }
   output = newObject( env, "org/gnu/pdapilot/DBInfo");
   setBooleanField( env, output, "flagReadOnly", !!( i.flags & dlpDBFlagReadOnly));
   setBooleanField( env, output, "flagResource", !!( i.flags & dlpDBFlagResource));
   setBooleanField( env, output, "flagBackup", !!( i.flags & dlpDBFlagBackup));
   setBooleanField( env, output, "flagOpen", !!( i.flags & dlpDBFlagOpen));
   setBooleanField( env, output, "flagAppInfoDirty", !!( i.flags & dlpDBFlagAppInfoDirty));
   setBooleanField( env, output, "flagNewer", !!( i.flags & dlpDBFlagNewer));
   setBooleanField( env, output, "flagReset", !!( i.flags & dlpDBFlagReset));
   setBooleanField( env, output, "flagCopyPrevention", !!( i.flags & dlpDBFlagCopyPrevention));
   setBooleanField( env, output, "flagStream", !!( i.flags & dlpDBFlagStream));
   setBooleanField( env, output, "flagExcludeFromSync", !!( i.miscFlags & dlpDBMiscFlagExcludeFromSync));
   setIntField( env, output, "index", i.index);
   setIntField( env, output, "version", i.version);
   setIntField( env, output, "modnum", i.modnum);
   setChar4Field( env, output, "type", i.type);
   setChar4Field( env, output, "creator", i.creator);
   setDateField( env, output, "createDate", i.createDate);
   setDateField( env, output, "modifyDate", i.modifyDate);
   setDateField( env, output, "backupDate", i.backupDate);
   setStringField( env, output, "name", env->NewStringUTF( i.name));
   setIntField( env, output, "card", 0);
   setIntField( env, output, "more", i.more);
   return output; 
   }
/*
 * Class:     __org_gnu_pdapilot_File
 * Method:    pi_file_get_app_info
 * Signature: (Lorg/gnu/pdapilot/Database;)Lorg/gnu/pdapilot/AppBlock;
 */
JNIEXPORT jobject JNICALL Java_org_gnu_pdapilot_File_pi_1file_1get_1app_1info
  (JNIEnv*env, jobject file, jobject dbClass) {
   struct pi_file *pf = ( struct pi_file*)getIntField( env, file, "_pf");
   jbyte*buffer;
   int len;
   jobject output;
   int result = pi_file_get_app_info( pf, ( void**)&buffer, &len);
   if( result >= 0)
      {
         len = result;
         jbyteArray a = newByteArray( env, buffer, len);
         output = callObjectMethod( env, 
                                    dbClass, 
                                    "newAppBlock", 
                                    "([B)Lorg/gnu/pdapilot/AppBlock;",
                                    a);
         }
      else {
         throwIOException( env, errno);
         output = 0;
         }
   return output;
   }
/*
 * Class:     __org_gnu_pdapilot_File
 * Method:    pi_file_get_sort_info
 * Signature: (Lorg/gnu/pdapilot/Database;)Lorg/gnu/pdapilot/SortBlock;
 */
JNIEXPORT jobject JNICALL Java_org_gnu_pdapilot_File_pi_1file_1get_1sort_1info
  (JNIEnv*env, jobject file, jobject dbClass) {
   struct pi_file *pf = ( struct pi_file*)getIntField( env, file, "_pf");
   jbyte*buffer;
   int len;
   jobject output;
   int result = pi_file_get_sort_info( pf, ( void**)&buffer, &len);
   if( result >= 0)
      {
         len = result;
         jbyteArray a = newByteArray( env, buffer, len);
         output = callObjectMethod( env, 
                                    dbClass, 
                                    "newSortBlock", 
                                    "([B)Lorg/gnu/pdapilot/SortBlock;",
                                    a);
         }
      else {
         throwIOException( env, errno);
         output = 0;
         }
   return output;
   }
/*
 * Class:     __org_gnu_pdapilot_File
 * Method:    pi_file_set_info
 * Signature: (Lorg/gnu/pdapilot/DBInfo;)V
 */
JNIEXPORT void JNICALL Java_org_gnu_pdapilot_File_pi_1file_1set_1info
  (JNIEnv*env, jobject file, jobject info) {
   struct pi_file *pf = ( struct pi_file*)getIntField( env, file, "_pf");
   struct DBInfo dbInfo;
   dbInfo.flags = getBooleanField( env, info, "flagReadOnly") ? dlpDBFlagReadOnly : 0 |
                  getBooleanField( env, info, "flagResource") ? dlpDBFlagResource : 0 |
                  getBooleanField( env, info, "flagBackup") ? dlpDBFlagBackup : 0 |
                  getBooleanField( env, info, "flagOpen") ? dlpDBFlagOpen : 0 |
                  getBooleanField( env, info, "flagAppInfoDirty") ? dlpDBFlagAppInfoDirty : 0 |
                  getBooleanField( env, info, "flagNewer") ? dlpDBFlagNewer : 0 |
                  getBooleanField( env, info, "flagReset") ? dlpDBFlagReset : 0 |
                  getBooleanField( env, info, "flagCopyPrevention") ? dlpDBFlagCopyPrevention : 0 |
                  getBooleanField( env, info, "flagStream") ? dlpDBFlagStream : 0;
   dbInfo.miscFlags = getBooleanField( env, info, "flagExcludeFromSync") ? dlpDBMiscFlagExcludeFromSync : 0;
   dbInfo.version = getIntField( env, info, "version");
   dbInfo.modnum = getIntField( env, info, "modnum");
   dbInfo.type = getChar4Field( env, info, "type");
   dbInfo.creator = getChar4Field( env, info, "creator");
   dbInfo.createDate = getDateField( env, info, "createDate");
   dbInfo.modifyDate = getDateField( env, info, "modifyDate");
   dbInfo.backupDate = getDateField( env, info, "backupDate");
   copyString( env, dbInfo.name, 34, info, "name");
   if( pi_file_set_info( pf, &dbInfo) < 0) throwIOException( env, errno);
   }
/*
 * Class:     __org_gnu_pdapilot_File
 * Method:    pi_file_set_app_info
 * Signature: (Lorg/gnu/pdapilot/AppBlock;)V
 */
JNIEXPORT void JNICALL Java_org_gnu_pdapilot_File_pi_1file_1set_1app_1info
  (JNIEnv*env, jobject file, jobject appBlock) {
   struct pi_file *pf = ( struct pi_file*)getIntField( env, file, "_pf");
   int len, result;
   jbyteArray b = ( jbyteArray)callObjectMethod( env, appBlock, "pack", "()[B");
   if( b == NULL) return;
   jboolean isCopy;
   jbyte*buffer = env->GetByteArrayElements( b, &isCopy);
   result = pi_file_set_app_info( pf, buffer, len);
   if( isCopy == JNI_TRUE) env->ReleaseByteArrayElements( b, buffer, JNI_ABORT);   if( result < 0) throwIOException( env, errno);
   if( result < 0) throwIOException( env, errno);
   }
/*
 * Class:     __org_gnu_pdapilot_File
 * Method:    pi_file_set_sort_info
 * Signature: (Lorg/gnu/pdapilot/SortBlock;)V
 */
JNIEXPORT void JNICALL Java_org_gnu_pdapilot_File_pi_1file_1set_1sort_1info
  (JNIEnv*env, jobject file, jobject sortBlock) {
   struct pi_file *pf = ( struct pi_file*)getIntField( env, file, "_pf");
   int len, result;
   jbyteArray b = ( jbyteArray)callObjectMethod( env, sortBlock, "pack", "()[B");
   if( b == NULL) return;
   jboolean isCopy;
   jbyte*buffer = env->GetByteArrayElements( b, &isCopy);
   result = pi_file_set_sort_info( pf, buffer, len);
   if( isCopy == JNI_TRUE) env->ReleaseByteArrayElements( b, buffer, JNI_ABORT);   if( result < 0) throwIOException( env, errno);
   if( result < 0) throwIOException( env, errno);
   }
/*
 * Class:     __org_gnu_pdapilot_File
 * Method:    pi_file_append_record
 * Signature: (Lorg/gnu/pdapilot/Record;)V
 */
JNIEXPORT void JNICALL Java_org_gnu_pdapilot_File_pi_1file_1append_1record
  (JNIEnv*env, jobject file, jobject record) {
   struct pi_file *pf = ( struct pi_file*)getIntField( env, file, "_pf");
   int attr, cat, len, result;
   jbyteArray b;
   recordid_t id;
   b = ( jbyteArray)callObjectMethod( env, record, "pack", "()[B");
   if( b == NULL) return;
   id = getRecordIDField( env, record, "id");
   attr = 0;
   attr |= getBooleanField( env, record, "deleted") ? 0x80 : 0;
   attr |= getBooleanField( env, record, "modified") ? 0x40 : 0;
   attr |= getBooleanField( env, record, "busy") ? 0x20 : 0;
   attr |= getBooleanField( env, record, "secret") ? 0x10 : 0;
   attr |= getBooleanField( env, record, "archived") ? 0x08 : 0;
   cat = getIntField( env, record, "category");
   jboolean isCopy;
   jbyte*buffer = env->GetByteArrayElements( b, &isCopy);
   result = pi_file_append_record( pf, buffer, len, attr, cat, id);
   if( isCopy == JNI_TRUE) env->ReleaseByteArrayElements( b, buffer, JNI_ABORT);   if( result < 0) throwIOException( env, errno);
   if( result < 0) throwIOException( env, errno);
   }
/*
 * Class:     __org_gnu_pdapilot_File
 * Method:    pi_file_append_resource
 * Signature: (Lorg/gnu/pdapilot/Resource;)V
 */
JNIEXPORT void JNICALL Java_org_gnu_pdapilot_File_pi_1file_1append_1resource
  (JNIEnv*env, jobject file, jobject resource) {
   struct pi_file *pf = ( struct pi_file*)getIntField( env, file, "_pf");
   int len, result, type, id;
   jbyteArray b;
   b = ( jbyteArray)callObjectMethod( env, resource, "pack", "()[B");
   if( b == NULL) return;
   jboolean isCopy;
   jbyte*buffer = env->GetByteArrayElements( b, &isCopy);
   type = getIntField( env, getObjectField( env, resource, "type", "Lorg/gnu/pdapilot/Char4"), "value");
   id = getIntField( env, resource, "id");
   result = pi_file_append_resource( pf, buffer, len, type, id);
   if( isCopy == JNI_TRUE) env->ReleaseByteArrayElements( b, buffer, JNI_ABORT);   if( result < 0) throwIOException( env, errno);
   if( result < 0) throwIOException( env, errno);
   }
/*
 * Class:     __org_gnu_pdapilot_File
 * Method:    pi_file_close
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_gnu_pdapilot_File_pi_1file_1close
  (JNIEnv*env, jobject file) {
   struct pi_file *pf = ( struct pi_file*)getIntField( env, file, "_pf");
   if( pf != NULL)
      {
         pi_file_close( pf);
         setIntField( env, file, "_pf", 0);
         }
   }
/*
 * Class:     __org_gnu_pdapilot_File
 * Method:    pi_file_retrieve
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_org_gnu_pdapilot_File_pi_1file_1retrieve
  (JNIEnv*env, jobject file, jint socket, jint card) {
   struct pi_file *pf = ( struct pi_file*)getIntField( env, file, "_pf");
   int result = pi_file_retrieve( pf, socket, card);
   if( result < 0) throwIOException( env, errno);
   }
/*
 * Class:     __org_gnu_pdapilot_File
 * Method:    pi_file_install
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_org_gnu_pdapilot_File_pi_1file_1install
  (JNIEnv*env, jobject file, jint socket, jint card) {
   struct pi_file *pf = ( struct pi_file*)getIntField( env, file, "_pf");
   int result = pi_file_install( pf, socket, card);
   if( result < 0) throwIOException( env, errno);
   }
/*
 * Class:     __org_gnu_pdapilot_File
 * Method:    pi_file_merge
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_org_gnu_pdapilot_File_pi_1file_1merge
  (JNIEnv*env, jobject file, jint socket, jint card) {
   struct pi_file *pf = ( struct pi_file*)getIntField( env, file, "_pf");
   int result = pi_file_merge( pf, socket, card);
   if( result < 0) throwIOException( env, errno);
   }
/*
 * Class:     __org_gnu_pdapilot_Socket
 * Method:    close
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Socket_close
  (JNIEnv*env, jobject obj, jint socket) {
   int result = pi_close( socket);
   if( result < 0) throwIOException( env, result);
   return result;
   }
/*
 * Class:     __org_gnu_pdapilot_Socket
 * Method:    socket
 * Signature: (III)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Socket_socket
  (JNIEnv *env, jobject obj, jint domain, jint type, jint protocol) {
   int result = pi_socket( domain, type, protocol);
   if( result < 0) throwIOException( env, result);
   return result;
   }
/*
 * Class:     __org_gnu_pdapilot_Socket
 * Method:    bind
 * Signature: (ILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Socket_bind
  (JNIEnv *env, jobject obj, jint socket, jstring address) {
   jboolean isCopy;
   const char *addr = env->GetStringUTFChars( address, &isCopy);
   struct pi_sockaddr *s = ( struct pi_sockaddr *)malloc( strlen( addr) + 3);
   jint result;
   jint len = strlen( addr);
   s->pi_family = PI_AF_PILOT;
   strcpy( s->pi_device, addr);
   result = pi_bind( socket, ( struct sockaddr *)s, len);
   free( s);
   if( isCopy == JNI_TRUE) env->ReleaseStringUTFChars( address, addr);
   if( result < 0) throwIOException( env, result);
   return result;
   }
/*
 * Class:     __org_gnu_pdapilot_Socket
 * Method:    listen
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Socket_listen
  (JNIEnv *env, jobject, jint socket, jint backlog) {
   jint result = pi_listen( socket, backlog);
   if( result < 0) throwIOException( env, result);
   return result;   
   }
/*
 * Class:     __org_gnu_pdapilot_Socket
 * Method:    accept
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Socket_accept
  (JNIEnv *env, jobject obj, jint socket) {
   jint result = pi_accept( socket, 0, 0);
   if( result < 0) throwIOException( env, result);
   return result;
   }
/*
 * Class:     __org_gnu_pdapilot_Socket
 * Method:    version
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Socket_version
  (JNIEnv *env, jobject obj, jint socket) {
   jint result = pi_version( socket);
   if( result < 0) throwIOException( env, result);
   return result;
   }
/*
 * Class:     __org_gnu_pdapilot_Socket
 * Method:    tickle
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Socket_tickle
  (JNIEnv *env, jobject obj, jint socket) {
   jint result = pi_tickle( socket);
   if( result < 0) throwIOException( env, result);
   return result;
   }
/*
 * Class:     __org_gnu_pdapilot_Socket
 * Method:    watchdog
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Socket_watchdog
  (JNIEnv *env, jobject obj, jint socket, jint interval) {
   jint result = pi_watchdog( socket, interval);
   if( result < 0) throwIOException( env, result);
   return result;
   }
/*
 * Class:     __org_gnu_pdapilot_Socket
 * Method:    read
 * Signature: (I[BI)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Socket_read
  (JNIEnv *env, jobject, jint socket, jbyteArray buf, jint len) {
   jboolean isCopy;
   jbyte *b = env->GetByteArrayElements( buf, &isCopy);
   jint result = pi_read( socket, b, len);
   if( isCopy == JNI_TRUE) env->ReleaseByteArrayElements( buf, b, 0);
   if( result < 0) throwIOException( env, result);
   return result;
   }
/*
 * Class:     __org_gnu_pdapilot_Socket
 * Method:    write
 * Signature: (I[BI)I
 */
JNIEXPORT jint JNICALL Java_org_gnu_pdapilot_Socket_write
  (JNIEnv *env, jobject, jint socket, jbyteArray buf, jint len) {
   jboolean isCopy;
   jbyte *b = env->GetByteArrayElements( buf, &isCopy);
   jint result = pi_write( socket, b, len);
   if( isCopy == JNI_TRUE) env->ReleaseByteArrayElements( buf, b, JNI_ABORT);
   if( result < 0) throwIOException( env, result);
   return result;
   }
/*
 * Class:     __org_gnu_pdapilot_address_AppBlock
 * Method:    unpack
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL Java_org_gnu_pdapilot_address_AppBlock_unpack
  (JNIEnv*env, jobject appBlock, jbyteArray);

/*
 * Class:     __org_gnu_pdapilot_address_AppBlock
 * Method:    pack
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_gnu_pdapilot_address_AppBlock_pack
  (JNIEnv*env, jobject appBlock) {
   jbyte*buffer = ( jbyte*)malloc( 0xffff);
   int len, i;
   jbyteArray output;
   struct AddressAppInfo a;
   a.sortByCompany = getIntField( env, appBlock, "sortByCompany");
   jintArray renamed = ( jintArray)getObjectField( env, appBlock, "labelRenamed", "[I");
   for( i = 0; i < 22; i++) {
      jint t;
      env->GetIntArrayRegion( renamed, i, 1, &t);
      a.labelRenamed[ i] = t;
      }
   doPackCategories( env, appBlock, &a.category);
   jobjectArray labels = ( jobjectArray)getObjectField( env, appBlock, "labels", "[Ljava/lang/String");
   for( i = 0; i < 22; i++) {
      copyStringElement( env, a.labels[ i], 16, labels, i);
      }
   jobjectArray phone = ( jobjectArray)getObjectField( env, appBlock, "phoneLabel", "[Ljava/lang/String");
   for( i = 0; i < 8; i++) {
      copyStringElement( env, a.phoneLabels[ i], 16, phone, i);
      }
   len = pack_AddressAppInfo( &a, ( unsigned char*)buffer, 0xffff);
   output = newByteArray( env, buffer, len);
   setIntField( env, appBlock, "raw", ( int)output);
   free( buffer);
   return output;
   }
/*
 * Class:     __org_gnu_pdapilot_address_Record
 * Method:    unpack
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL Java_org_gnu_pdapilot_address_Record_unpack
  (JNIEnv*env, jobject record, jbyteArray b) {
   struct Address a;
   int i;
   jboolean isCopy;
   jbyte*buf = env->GetByteArrayElements( b, &isCopy);
   unpack_Address( &a, ( unsigned char*)buf, env->GetArrayLength( b));
   if( isCopy == JNI_TRUE) env->ReleaseByteArrayElements( b, buf, JNI_ABORT);
   setIntField( env, record, "raw", ( int)b);
   setIntField( env, record, "showPhone", a.showPhone);
   jobjectArray entries = newObjectArray( env, "java/lang/String", 19);
   for( i = 0; i < 19; i++) {
      env->SetObjectArrayElement( entries, i, env->NewStringUTF( a.entry[ i]));
      }
   setObjectField( env, record, "entry", "[Ljava/lang/String;", entries);
   jintArray phones = env->NewIntArray( 5);
   for( i = 0; i < 5; i++) {
      jint t = a.phoneLabel[ i];
      env->SetIntArrayRegion( phones, i, 1, &t);
      }
   setObjectField( env, record, "phoneLabel", "[I", phones);
   free_Address( &a);
   }
/*
 * Class:     __org_gnu_pdapilot_address_Record
 * Method:    pack
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_gnu_pdapilot_address_Record_pack
  (JNIEnv*env, jobject record) {
   jbyte*buffer = ( jbyte*)malloc( 0xffff);
   int len, i;
   jbyteArray output;
   struct Address a;
   jobjectArray entries = ( jobjectArray)getObjectField( env, record, "entry", "[Ljava/lang/String;");
   for( i = 0; i < 19; i++) {
      copyStringElement( env, a.entry[ i], 16, entries, i);
      }
   jintArray phones = ( jintArray)getObjectField( env, record, "phoneLabel", "[I");
   for( i = 0; i < 5; i++) {
      jint t;
      env->GetIntArrayRegion( phones, i, 1, &t);
      a.phoneLabel[ i] = t;
      }
   a.showPhone = getIntField( env, record, "showPhone");
   len = pack_Address( &a, ( unsigned char*)buffer, 0xffff);
   output = newByteArray( env, buffer, len);
   setIntField( env, record, "raw", ( int)output);
   free( buffer);
   return output;
   }
/*
 * Class:     __org_gnu_pdapilot_appointment_AppBlock
 * Method:    unpack
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL Java_org_gnu_pdapilot_appointment_AppBlock_unpack
  (JNIEnv*env, jobject record, jbyteArray b) {
   struct AppointmentAppInfo a;
   int i;
   jboolean isCopy;
   jbyte*buf = env->GetByteArrayElements( b, &isCopy);
   unpack_AppointmentAppInfo( &a, ( unsigned char*)buf, env->GetArrayLength( b));
   if( isCopy == JNI_TRUE) env->ReleaseByteArrayElements( b, buf, JNI_ABORT);
   setObjectField( env, record, "raw", "[B", b);
   doUnpackCategories( env, record, &a.category);
   setIntField( env, record, "startOfWeek", a.startOfWeek);
   }
/*
 * Class:     __org_gnu_pdapilot_appointment_AppBlock
 * Method:    pack
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_gnu_pdapilot_appointment_AppBlock_pack
  (JNIEnv*env, jobject record) {
   jbyte*buffer = ( jbyte*)malloc( 0x0ffff);
   int len;
   jbyteArray output;
   struct AppointmentAppInfo a;
   a.startOfWeek = getIntField( env, record, "startOfWeek");
   doPackCategories( env, record, &a.category);
   len = pack_AppointmentAppInfo( &a, ( unsigned char *)buffer, 0x0ffff);
   output = newByteArray( env, buffer, len);
   setObjectField( env, record, "raw", "[B", output);
   free( buffer);
   return output;
   }
/*
 * Class:     __org_gnu_pdapilot_appointment_Record
 * Method:    unpack
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL Java_org_gnu_pdapilot_appointment_Record_unpack
  (JNIEnv*env, jobject record, jbyteArray b) {
   struct Appointment a;
   int i;
   jboolean isCopy;
   jbyte*buf = env->GetByteArrayElements( b, &isCopy);
   unpack_Appointment( &a, ( unsigned char*)buf, env->GetArrayLength( b));
   if( isCopy == JNI_TRUE) env->ReleaseByteArrayElements( b, buf, JNI_ABORT);
   setStringFieldValue( env, record, "note", a.note);
   setStringFieldValue( env, record, "description", a.description);
   setDateTimeField( env, record, "begin", &a.begin);
   setDateTimeField( env, record, "end", a.event ? NULL : &a.end);
   setBooleanField( env, record, "alarm", a.alarm);
   setIntField( env, record, "advance", a.advance);
   setObjectField( env, 
                   record,
                   "advanceUnits", 
                   "Lorg/gnu/pdapilot/appointment/Time;",
                   callStaticObjectMethod( env,
                                           "org/gnu/pdapilot/appointment/Time",
                                           "get",
                                           "(I)Lorg/gnu/pdapilot/appointment/Time;",
                                           a.advanceUnits));
// This next bit is removed temporarily as for some odd reason we get
// gash values in a.exceptions and this causes havoc.
//   if( a.exceptions) 
//      {
//printf( "appointment_Record_unpack: Processing %d exceptions\n", a.exceptions);
//         jobjectArray e = newObjectArray( env, "java/util/Date", a.exceptions); 
//         setObjectField( env, record, "exceptions", "[Ljava/util/Date;", e);
//         for( i = 0; i < a.exceptions; i++) {
//            env->SetObjectArrayElement( e, i, makeDateTime( env, &a.exception[ i]));
//            }
//         }
   setIntField( env, record, "repeatDay", a.repeatDay);
   setIntField( env, record, "repeatWeekStart", a.repeatWeekstart);
   setObjectField( env, 
                   record,
                   "repeatType", 
                   "Lorg/gnu/pdapilot/appointment/Repeat;",
                   callStaticObjectMethod( env,
                                           "org/gnu/pdapilot/appointment/Repeat",
                                           "get",
                                           "(I)Lorg/gnu/pdapilot/appointment/Repeat;",
                                           a.repeatType));
   jbooleanArray wdays = env->NewBooleanArray( 7);
   for( i = 0; i < 7; i++) {
      setBooleanArrayElement( env, wdays, i, a.repeatDays[ i]);
      }
   setObjectField( env, record, "repeatWeekdays", "[Z", wdays);
   setDateTimeField( env, 
                     record, 
                     "repeatEnd", 
                     a.repeatForever ? NULL : &a.repeatEnd);
   free_Appointment( &a);
   }
/*
 * Class:     __org_gnu_pdapilot_appointment_Record
 * Method:    pack
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_gnu_pdapilot_appointment_Record_pack
  (JNIEnv*env, jobject record) {
   jbyte*buffer = ( jbyte*)malloc( 0xffff);
   int len, i;
   jbyteArray output;
   struct Appointment a;
   a.note = getStringFieldValue( env, record, "note");
   a.description = getStringFieldValue( env, record, "description");
   a.begin = getDateTimeField( env, record, "begin");
   jobject end = getObjectField( env, record, "end", "Ljava/util/Date;");
   if( end == NULL) a.event = 1;
      else {
         a.end = readDateTime( env, end);
         a.event = 0;
         }
   a.alarm = getBooleanField( env, record, "alarm");
   a.advance = getIntField( env, record, "advance");
   // now invoke method getValue from record.advanceUnits
   jobject au = getObjectField( env,
                                record, 
                                "advanceUnits", 
                                "Lorg/gnu/pdapilot/appointment/Time;");
   a.advanceUnits = callIntMethod( env, au, "getValue", "()I");
   jobject rt = getObjectField( env,
                                record, 
                                "repeatType", 
                                "Lorg/gnu/pdapilot/appointment/Repeat;");
   a.repeatType = ( enum repeatTypes)callIntMethod( env, rt, "getValue", "()I");
   if( a.repeatType == repeatWeekly) 
      {
         jbooleanArray dways = ( jbooleanArray)getObjectField( env, record, "repeatWeekdays", "[Z");
         for( int i = 0; i < 7; i++) {
            a.repeatDays[ i] = getBooleanArrayElement( env, dways, i);
            }
         }
      else a.repeatDay = ( enum DayOfMonthType)getIntField( env, record, "repeatDay");
   a.repeatWeekstart = getIntField( env, record, "repeatWeekStart");
   end = getObjectField( env, record, "repeatEnd", "Ljava/util/Date;");
   if( end == NULL) a.repeatForever = 1;
      else {
         a.repeatEnd = getDateTimeField( env, record, "repeatEnd");
         a.repeatForever = 0;
         }
   jobjectArray exceptions = ( jobjectArray)getObjectField( env, record, "exceptions", "[I");
   if( exceptions != NULL) 
      {
         a.exceptions = env->GetArrayLength( exceptions);
         a.exception = ( struct tm*)malloc( sizeof( struct tm) * a.exceptions);
         for( i = 0; i < a.exceptions; i++) {
            jobject d = env->GetObjectArrayElement( exceptions, i);
            a.exception[ i] = readDateTime( env, d);
            }
         }
   len = pack_Appointment( &a, ( unsigned char *)buffer, 0xffff);
   output = newByteArray( env, buffer, len);
   setObjectField( env, record, "raw", "[B", output);
   return output;
   }
/*
 * Class:     __org_gnu_pdapilot_expense_AppBlock
 * Method:    unpack
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL Java_org_gnu_pdapilot_expense_AppBlock_unpack
  (JNIEnv*env, jobject appBlock, jbyteArray b) {
   int i;
   struct ExpenseAppInfo a;
   jboolean isCopy;
   jbyte*buf = env->GetByteArrayElements( b, &isCopy);
   unpack_ExpenseAppInfo( &a, ( unsigned char*)buf, env->GetArrayLength( b));
   if( isCopy == JNI_TRUE) env->ReleaseByteArrayElements( b, buf, JNI_ABORT);
   setObjectField( env, appBlock, "raw", "[B", b);
   jobjectArray curarr = newObjectArray( env, 
                                         "org/gnu/pdapilot/expense/CustomCurrency",
                                         4);
   for( i - 0; i < 4; i++) {
      jobject cur = newObject( env, "org/gnu/pdapilot/expense/CustomCurrency");
      env->SetObjectArrayElement( curarr, i, cur);
      setStringFieldValue( env, cur, "symbol", a.currencies[ i].symbol);
      setStringFieldValue( env, cur, "name", a.currencies[ i].name);
      setStringFieldValue( env, cur, "rate", a.currencies[ i].rate);
      }
   doUnpackCategories( env, appBlock, &a.category);
   setObjectField( env, 
                   appBlock, 
                   "sortOrder",
                   "Lorg/gnu/pdapilot/expenses/Sort;", 
                   newObject( env, "org/gnu/pdapilot/expenses/Sort"));
   }
/*
 * Class:     __org_gnu_pdapilot_expense_AppBlock
 * Method:    pack
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_gnu_pdapilot_expense_AppBlock_pack
  (JNIEnv*env, jobject appBlock) {
   jbyteArray output = NULL;
   jbyte*buffer = ( jbyte*)malloc( 0xffff);
   int len, i;
   struct ExpenseAppInfo a;
   jobject so = getObjectField( env, appBlock, "sortOrder", "Lorg/gnu/pdapilot/expense/Sort;");
   a.sortOrder = ( enum ExpenseSort)callIntMethod( env, so, "getValue", "()I");
   jobjectArray currencies = ( jobjectArray)getObjectField( env, appBlock, "currencies", "Lorg/gnu/pdapilot/CustomCurrency;");
   if( currencies != NULL) 
      {
         for( i = 0; i < 4; i++) {
            jobject cur = env->GetObjectArrayElement( currencies, i);
            jstring t = getStringField( env, cur, "name");
            if( t == NULL) a.currencies[ i].name[ 0] = 0;
               else setFixedString( env, a.currencies[ i].name, 16, t);
            t = getStringField( env, cur, "symbol");
            if( t == NULL) a.currencies[ i].symbol[ 0] = 0;
               else setFixedString( env, a.currencies[ i].symbol, 4, t);
            t = getStringField( env, cur, "rate");
            if( t == NULL) a.currencies[ i].rate[ 0] = 0;
               else setFixedString( env, a.currencies[ i].rate, 8, t);
            }
         }
   doPackCategories( env, appBlock, &a.category);
   len = pack_ExpenseAppInfo( &a, ( unsigned char*)buffer, len);
   output = newByteArray( env, buffer, len);
   setObjectField( env, appBlock, "raw", "[B", output);
   free( buffer);
   return output;
   }
/*
 * Class:     __org_gnu_pdapilot_expense_Pref
 * Method:    unpack
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL Java_org_gnu_pdapilot_expense_Pref_unpack
  (JNIEnv*env, jobject pref, jbyteArray b) {
   struct ExpensePref a;
   int i;
   jboolean isCopy;
   jbyte*buf = env->GetByteArrayElements( b, &isCopy);
   unpack_ExpensePref( &a, ( unsigned char*)buf, env->GetArrayLength( b));
   if( isCopy == JNI_TRUE) env->ReleaseByteArrayElements( b, buf, JNI_ABORT);
   setObjectField( env, pref, "raw", "[B", b);
   setIntField( env, pref, "currentCategory", a.currentCategory);
   setIntField( env, pref, "defaultCurrency", a.defaultCurrency);
   // NOTE this next one looks wrong, but its what the old code did
   setIntField( env, pref, "currentCategory", a.noteFont);
   setIntField( env, pref, "showAllCategories", a.showAllCategories);
   setIntField( env, pref, "showCurrency", a.showCurrency);
   setIntField( env, pref, "allowQuickFill", a.allowQuickFill);
   setObjectField( env, 
                   pref,
                   "unitOfDistance",
                   "Lorg/gnu/pdapilot/expense/Distance",
                   newObjectV( env, 
                              "org/gnu/pdapilot/expense/Distance",
                              "()V",
                              a.unitOfDistance));
   jintArray currencies = env->NewIntArray( 7);
   for( i = 0; i < 7; i++) {
      jint t = a.currencies[ i];
      env->SetIntArrayRegion( currencies, i, 1, &t);
      }
   }
/*
 * Class:     __org_gnu_pdapilot_expense_Pref
 * Method:    pack
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_gnu_pdapilot_expense_Pref_pack
  (JNIEnv*env, jobject pref) {
   jbyteArray output = NULL;
   jbyte*buffer = ( jbyte*)malloc( 0xffff);
   int len, i;
   struct ExpensePref a;
   a.currentCategory = getIntField( env, pref, "currentCategory");
   a.defaultCurrency = getIntField( env, pref, "defaultCurrency");
   a.noteFont = getIntField( env, pref, "currentCategory");
   a.showAllCategories = getBooleanField( env, pref, "showAllCategories");
   a.showCurrency = getBooleanField( env, pref, "showCurrencies");
   a.saveBackup = getBooleanField( env, pref, "saveBackup");
   a.allowQuickFill = getBooleanField( env, pref, "allowQuickFill");
   jobject unit = getObjectField( env, pref, "unitOfDistance", "Lorg/gnu/pdapilot/expense/distance;");
   a.unitOfDistance = ( enum ExpenseDistance)callIntMethod( env,
                                                            unit,
                                                            "getValue",
                                                            "()I");
   jintArray cur = ( jintArray)getObjectField( env, pref, "currencies", "[I");
   for( i = 0; i < 7; i++) {
      a.currencies[ i] = getIntArrayElement( env, cur, i);
      }
   len = pack_ExpensePref( &a, ( unsigned char*)buffer, len);
   output = newByteArray( env, buffer, len);
   setObjectField( env, pref, "raw", "[B", output);
   free( buffer);
   return output;
   }
/*
 * Class:     __org_gnu_pdapilot_expense_Record
 * Method:    unpack
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL Java_org_gnu_pdapilot_expense_Record_unpack
  (JNIEnv*env, jobject record, jbyteArray b) {
   struct Expense a;
   jboolean isCopy;
   jbyte*buf = env->GetByteArrayElements( b, &isCopy);
   unpack_Expense( &a, ( unsigned char*)buf, env->GetArrayLength( b));
   if( isCopy == JNI_TRUE) env->ReleaseByteArrayElements( b, buf, JNI_ABORT);
   setDateTimeField( env, record, "date", &a.date);
   setObjectField( env, 
                   record, 
                   "type", 
                   "Lorg/gnu/pdapilot/expense/Type;",
                   newObjectV( env,
                              "org/gnu/pdapilot/expense/Type",
                              "(I)V",
                              a.type));
   setObjectField( env, 
                   record, 
                   "payment", 
                   "Lorg/gnu/pdapilot/expense/paymemt;",
                   newObjectV( env,
                              "org/gnu/pdapilot/expense/Payment",
                              "(I)V",
                              a.payment));
   setStringFieldValue( env, record, "amount", a.amount);
   setStringFieldValue( env, record, "vendor", a.vendor);
   setStringFieldValue( env, record, "city", a.city);
   setStringFieldValue( env, record, "attendees", a.attendees);
   setStringFieldValue( env, record, "note", a.note);
   free_Expense( &a);
   }
/*
 * Class:     __org_gnu_pdapilot_expense_Record
 * Method:    pack
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_gnu_pdapilot_expense_Record_pack
  (JNIEnv*env, jobject record) {
   jbyte*buffer = ( jbyte*)malloc( 0xffff);
   int len;
   jbyteArray output;
   struct Expense a;
   a.date = getDateTimeField( env, record, "date");
   a.currency = getIntField( env, record, "currency");
   jobject type = getObjectField( env, record, "type", "Lorg/gnu/pdapilot/expense/Type;");
   a.type = ( enum ExpenseType)callIntMethod( env, type, "getValue", "()I");
   jobject payment = getObjectField( env, record, "payment", "Lorg/gnu/pdapilot/expense/Payment;");
   a.payment = ( enum ExpensePayment)callIntMethod( env, payment, "getValue", "()I");
   a.amount = getStringFieldValue( env, record, "amount");
   a.vendor = getStringFieldValue( env, record, "vendor");
   a.city = getStringFieldValue( env, record, "city");
   a.attendees = getStringFieldValue( env, record, "attendees");
   a.note = getStringFieldValue( env, record, "note");
   len = pack_Expense( &a, ( unsigned char*)buffer, 0xffff);
   output = newByteArray( env, buffer, len);
   setObjectField( env, record, "raw", "[B", output);
   free( buffer);
   return output;
   }
/*
 * Class:     __org_gnu_pdapilot_mail_AppBlock
 * Method:    unpack
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL Java_org_gnu_pdapilot_mail_AppBlock_unpack
  (JNIEnv*env, jobject appBlock, jbyteArray b) {
   struct MailAppInfo a;
   jboolean isCopy;
   jbyte*buf = env->GetByteArrayElements( b, &isCopy);
   unpack_MailAppInfo( &a, ( unsigned char*)buf, env->GetArrayLength( b));
   if( isCopy == JNI_TRUE) env->ReleaseByteArrayElements( b, buf, JNI_ABORT);
   setObjectField( env, appBlock, "raw", "[B", b);
   doUnpackCategories( env, appBlock, &a.category);
   setIntField( env, appBlock, "dirty", a.dirty);
   setObjectField( env,
                   appBlock,
                   "sortOrder",
                   "Lorg/gnu/pdapilot/mail/Sort;",
                   newObjectV( env,
                              "Lorg/gnu/pdapilot/mail/Sort;",
                              "(I)V",
                              a.sortOrder));
   setRecordIDField( env, appBlock, "unsentMessage", a.unsentMessage);
   }
/*
 * Class:     __org_gnu_pdapilot_mail_AppBlock
 * Method:    pack
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_gnu_pdapilot_mail_AppBlock_pack
  (JNIEnv*env, jobject appBlock) {
   jbyte*buffer = ( jbyte*)malloc( 0xffff);
   int len;
   jbyteArray output;
   struct MailAppInfo a;
   doPackCategories( env, appBlock, &a.category);
   jobject so = getObjectField( env, appBlock, "sortOrder", "Lorg/gnu/pdapilot/mail/Sort;");
   a.sortOrder = callIntMethod( env, so, "getValue", "()I");
   a.dirty = getIntField( env, appBlock, "dirty");
   a.unsentMessage = getRecordIDField( env, appBlock, "unsentMessage");
   len = pack_MailAppInfo( &a, ( unsigned char*)buffer, 0xffff);
   output = newByteArray( env, buffer, len);
   setObjectField( env, appBlock, "raw", "[B", output);
   free( buffer);
   return output;
   }
/*
 * Class:     __org_gnu_pdapilot_mail_Record
 * Method:    unpack
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL Java_org_gnu_pdapilot_mail_Record_unpack
  (JNIEnv*env, jobject record, jbyteArray b) {
   struct Mail a;
   jboolean isCopy;
   jbyte*buf = env->GetByteArrayElements( b, &isCopy);
   unpack_Mail( &a, ( unsigned char*)buf, env->GetArrayLength( b));
   if( isCopy == JNI_TRUE) env->ReleaseByteArrayElements( b, buf, JNI_ABORT);
   setDateTimeField( env, record, "dated", &a.date);
   setIntField( env, record, "read", a.read);
   setIntField( env, record, "signature", a.signature);
   setIntField( env, record, "confirmRead", a.confirmRead);
   setIntField( env, record, "confirmDelivery", a.confirmDelivery);
   setIntField( env, record, "priority", a.priority);
   setIntField( env, record, "addressing", a.addressing);
   setStringFieldValue( env, record, "subject", a.subject);
   setStringFieldValue( env, record, "from", a.from);
   setStringFieldValue( env, record, "to", a.to);
   setStringFieldValue( env, record, "cc", a.cc);
   setStringFieldValue( env, record, "bcc", a.bcc);
   setStringFieldValue( env, record, "replyTo", a.replyTo);
   setStringFieldValue( env, record, "sentTo", a.sentTo);
   setStringFieldValue( env, record, "body", a.body);
   free_Mail( &a);
   }
/*
 * Class:     __org_gnu_pdapilot_mail_Record
 * Method:    pack
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_gnu_pdapilot_mail_Record_pack
  (JNIEnv*env, jobject record) {
   jbyte*buffer = ( jbyte*)malloc( 0xffff);
   int len;
   jbyteArray output;
   struct Mail a;
   a.dated = getObjectField( env, record, "date", "Ljava/util/Date;") == NULL ? 1 : 0;
   if( a.dated) a.date = getDateTimeField( env, record, "date");
   a.read = getIntField( env, record, "read");
   a.signature = getIntField( env, record, "signature");
   a.confirmRead = getIntField( env, record, "confirmRead");
   a.confirmDelivery = getIntField( env, record, "confirmDelivery");
   a.priority = getIntField( env, record, "priorty");
   a.addressing = getIntField( env, record, "addressing");
   a.subject = getStringFieldValue( env, record, "subject");
   a.from = getStringFieldValue( env, record, "from");
   a.to = getStringFieldValue( env, record, "to");
   a.cc = getStringFieldValue( env, record, "cc");
   a.bcc = getStringFieldValue( env, record, "bcc");
   a.replyTo = getStringFieldValue( env, record, "replyTo");
   a.sentTo = getStringFieldValue( env, record, "sendTo");
   a.body = getStringFieldValue( env, record, "body");
   len = pack_Mail( &a, ( unsigned char*)buffer, 0xffff);
   output = newByteArray( env, buffer, len);
   setObjectField( env, record, "raw", "[B", output);
   free( buffer);
   return output;
   }
/*
 * Class:     __org_gnu_pdapilot_mail_SignaturePref
 * Method:    unpack
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL Java_org_gnu_pdapilot_mail_SignaturePref_unpack
  (JNIEnv*env, jobject pref, jbyteArray b) {
   struct MailSignaturePref a;
   jboolean isCopy;
   jbyte*buf = env->GetByteArrayElements( b, &isCopy);
   unpack_MailSignaturePref( &a, ( unsigned char*)buf, env->GetArrayLength( b));
   if( isCopy == JNI_TRUE) env->ReleaseByteArrayElements( b, buf, JNI_ABORT);
   setObjectField( env, pref, "raw", "[B", b);
   setStringFieldValue( env, pref, "signature", a.signature);
   }
/*
 * Class:     __org_gnu_pdapilot_mail_SignaturePref
 * Method:    pack
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_gnu_pdapilot_mail_SignaturePref_pack
  (JNIEnv*env, jobject pref) {
   jbyte*buffer = ( jbyte*)malloc( 0xffff);
   int len;
   jbyteArray output;
   struct MailSignaturePref a;
   a.signature = getStringFieldValue( env, pref, "signature");
   len = pack_MailSignaturePref( &a, ( unsigned char *)buffer, 0xffff);
   output = newByteArray( env, buffer, len);
   setObjectField( env, pref, "raw", "[B", output);
   free( buffer);
   return output;
   }
/*
 * Class:     __org_gnu_pdapilot_mail_SyncPref
 * Method:    unpack
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL Java_org_gnu_pdapilot_mail_SyncPref_unpack
  (JNIEnv*env, jobject pref, jbyteArray b) {
   struct MailSyncPref a;
   jboolean isCopy;
   jbyte*buf = env->GetByteArrayElements( b, &isCopy);
   unpack_MailSyncPref( &a, ( unsigned char*)buf, env->GetArrayLength( b));
   if( isCopy == JNI_TRUE) env->ReleaseByteArrayElements( b, buf, JNI_ABORT);
   setObjectField( env, pref, "raw", "[B", b);
   setObjectField( env, 
                   pref, 
                   "syncType",
                   "Lorg/gnu/pdapilot/mail/Sync;",
                   newObjectV( env,
                              "org/gnu/pdapilot/mail/Sync",
                              "(I)V",
                              a.syncType));
   setIntField( env, pref, "getHigh", a.getHigh);
   setIntField( env, pref, "getContaining", a.getContaining);
   setIntField( env, pref, "truncate", a.truncate);
   setStringFieldValue( env, pref, "filterTo", a.filterTo);
   setStringFieldValue( env, pref, "filterFrom", a.filterFrom);
   setStringFieldValue( env, pref, "filterSubject", a.filterSubject);
   }
/*
 * Class:     __org_gnu_pdapilot_mail_SyncPref
 * Method:    pack
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_gnu_pdapilot_mail_SyncPref_pack
  (JNIEnv*env, jobject pref) {
   jbyte*buffer = ( jbyte*)malloc( 0xffff);
   int len;
   jbyteArray output;
   struct MailSyncPref a;
   jobject st = getObjectField( env, pref, "syncType", "Lorg/gnu/pdapilot/mail/Sync;");
   a.syncType = callIntMethod( env, st, "getValue", "()I");
   
   a.getHigh = getIntField( env, pref, "getHigh");
   a.getContaining = getIntField( env, pref, "getContaining");
   a.truncate = getIntField( env, pref, "truncate");
   a.filterTo = getStringFieldValue( env, pref, "filterTo");
   a.filterFrom = getStringFieldValue( env, pref, "filterFrom");
   a.filterSubject = getStringFieldValue( env, pref, "filterSubject");
   len = pack_MailSyncPref( &a, ( unsigned char*)buffer, 0xffff);
   output = newByteArray( env, buffer, len);
   setObjectField( env, pref, "raw", "[B", output);
   free( buffer);
   return output;
   }
/*
 * Class:     __org_gnu_pdapilot_memo_AppBlock
 * Method:    unpack
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL Java_org_gnu_pdapilot_memo_AppBlock_unpack
  (JNIEnv*env, jobject appBlock, jbyteArray b) {
   struct MemoAppInfo m;
   jboolean isCopy;
   jbyte*buf = env->GetByteArrayElements( b, &isCopy);
   unpack_MemoAppInfo( &m, ( unsigned char*)buf, env->GetArrayLength( b));
   if( isCopy == JNI_TRUE) env->ReleaseByteArrayElements( b, buf, JNI_ABORT);
   setObjectField( env, appBlock, "raw", "[B", b);
   doUnpackCategories( env, appBlock, &m.category);
   setBooleanField( env, appBlock, "sortByAlpha", m.sortByAlpha);
   }
/*
 * Class:     __org_gnu_pdapilot_memo_AppBlock
 * Method:    pack
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_gnu_pdapilot_memo_AppBlock_pack
  (JNIEnv*env, jobject appBlock) {
   jbyte*buffer = ( jbyte*)malloc( 0xffff);
   int len;
   jbyteArray output;
   struct MemoAppInfo m;
   doPackCategories( env, appBlock, &m.category);
   m.sortByAlpha = getBooleanField( env, appBlock, "sortByAlpha");
   len = pack_MemoAppInfo( &m, ( unsigned char*)buffer, 0xffff);
   output = newByteArray( env, buffer, len);
   setObjectField( env, appBlock, "raw", "[B", output);
   free( buffer);
   return output;
   }
/*
 * Class:     __org_gnu_pdapilot_memo_Record
 * Method:    unpack
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL Java_org_gnu_pdapilot_memo_Record_unpack
  (JNIEnv*env, jobject record, jbyteArray b) {
   struct Memo m;
   jboolean isCopy;
   jbyte*buf = env->GetByteArrayElements( b, &isCopy);
   unpack_Memo( &m, ( unsigned char*)buf, env->GetArrayLength( b));
   if( isCopy == JNI_TRUE) env->ReleaseByteArrayElements( b, buf, JNI_ABORT);
   setObjectField( env, record, "raw", "[B", b);
   setStringFieldValue( env, record, "text", m.text);
   free_Memo( &m);
   }
/*
 * Class:     __org_gnu_pdapilot_memo_Record
 * Method:    pack
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_gnu_pdapilot_memo_Record_pack
  (JNIEnv*env, jobject record) {
   jbyte*buffer = ( jbyte*)malloc( 0xffff);
   int len;
   jbyteArray output;
   struct Memo m;
   m.text = getStringFieldValue( env, record, "text");
   len = pack_Memo( &m, ( unsigned char*)buffer, 0xffff);
   output = newByteArray( env, buffer, len);
   setObjectField( env, record, "raw", "[B", output);
   free( buffer);
   return output;
   }
/*
 * Class:     __org_gnu_pdapilot_todo_AppBlock
 * Method:    unpack
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL Java_org_gnu_pdapilot_todo_AppBlock_unpack
  (JNIEnv*env, jobject appBlock, jbyteArray b) {
   struct ToDoAppInfo m;
   jboolean isCopy;
   jbyte*buf = env->GetByteArrayElements( b, &isCopy);
   unpack_ToDoAppInfo( &m, ( unsigned char*)buf, env->GetArrayLength( b));
   if( isCopy == JNI_TRUE) env->ReleaseByteArrayElements( b, buf, JNI_ABORT);
   setObjectField( env, appBlock, "raw", "[B", b);
   setIntField( env, appBlock, "dirty", m.dirty);
   setIntField( env, appBlock, "sortByPriority", m.sortByPriority);
   }
/*
 * Class:     __org_gnu_pdapilot_todo_AppBlock
 * Method:    pack
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_gnu_pdapilot_todo_AppBlock_pack
  (JNIEnv*env, jobject appBlock) {
   jbyte*buffer = ( jbyte*)malloc( 0xffff);
   int len;
   jbyteArray output;
   struct ToDoAppInfo t;
   t.sortByPriority = getIntField( env, appBlock, "sortByPriority");
   t.dirty = getIntField( env, appBlock, "dirty");
   doPackCategories( env, appBlock, &t.category);
   len = pack_ToDoAppInfo( &t, ( unsigned char*)buffer, 0xffff);
   output = newByteArray( env, buffer, len);
   setObjectField( env, appBlock, "raw", "[B", output);
   free( buffer);
   return output;
   }
/*
 * Class:     __org_gnu_pdapilot_todo_Record
 * Method:    unpack
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL Java_org_gnu_pdapilot_todo_Record_unpack
  (JNIEnv*env, jobject record, jbyteArray b) {
   struct ToDo t;
   jboolean isCopy;
   jbyte*buf = env->GetByteArrayElements( b, &isCopy);
   unpack_ToDo( &t, ( unsigned char*)buf, env->GetArrayLength( b));
   if( isCopy == JNI_TRUE) env->ReleaseByteArrayElements( b, buf, JNI_ABORT);
   setObjectField( env, record, "raw", "[B", b);
   if( t.indefinite) setDateTimeField( env, record, "due", 0);
      else setDateTimeField( env, record, "due", &t.due);
   setIntField( env, record, "priority", t.priority);
   setIntField( env, record, "complete", t.complete);
   setStringFieldValue( env, record, "description", t.description);
   setStringFieldValue( env, record, "note", t.note);
   free_ToDo( &t);
   }
/*
 * Class:     __org_gnu_pdapilot_todo_Record
 * Method:    pack
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_gnu_pdapilot_todo_Record_pack
  (JNIEnv*env, jobject record) {
   jbyte*buffer = ( jbyte*)malloc( 0xffff);
   int len;
   jbyteArray output;
   struct ToDo t;
   t.description = getStringFieldValue( env, record, "description");
   t.note = getStringFieldValue( env, record, "note");
   t.priority = getIntField( env, record, "priority");
   t.complete = getIntField( env, record, "complete");
   t.indefinite = 1;
   jobject due = getObjectField( env, record, "due", "Ljava/util/Date;");
   if( due != NULL)
      {
         struct tm tm = readDateTime( env, due);
         t.due = tm;
         t.indefinite = 0;
         }
   len = pack_ToDo( &t, ( unsigned char*)buffer, 0xffff);
   output = newByteArray( env, buffer, len);
   setObjectField( env, record, "raw", "[B", output);
   free( buffer);
   return output;
   }
#ifdef __cplusplus
}
#endif
