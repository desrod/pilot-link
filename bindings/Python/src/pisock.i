// -*-C-*-
//
// $Id$
//
// Copyright 1999-2001 Rob Tillotson <rob@pyrite.org>
// All Rights Reserved
//
// Permission to use, copy, modify, and distribute this software and
// its documentation for any purpose and without fee or royalty is
// hereby granted, provided that the above copyright notice appear in
// all copies and that both the copyright notice and this permission
// notice appear in supporting documentation or portions thereof,
// including modifications, that you you make.
//
// THE AUTHOR ROB TILLOTSON DISCLAIMS ALL WARRANTIES WITH REGARD TO
// THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
// RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
// CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
// CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE!
//

// This is an attempt at using SWIG to generate wrappers for the
// pilot-link library.  As one look at this source should tell you, it
// turned out to be a non-trivial amount of work (but less than doing
// it by hand).  The semantics of the resulting module are not all
// that different from the original Python interface to pilot-link,
// but function names and arguments are closer to those in the raw
// pilot-link library.  In particular, no attempt is made to create
// Python types for connections, databases, etc.; if you want object
// structure, shadow classes should be easy to create.
//
// All of the dlp_* functions will throw an exception when the library
// returns a negative status; the value of this exception will be a
// tuple of the numeric error code and a message.


%module _pisock
%{
#include <time.h>
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-file.h"
#include "pi-header.h"    
    
extern char *printlong(unsigned long);
extern unsigned long makelong(char *c);

void *__dlp_buf;

#define DLPMAXBUF 0xFFFF 

#define DGETLONG(src,key,default) (PyDict_GetItemString(src,key) ? PyInt_AsLong(PyDict_GetItemString(src,key)) : default)
#define DGETSTR(src,key,default) (PyDict_GetItemString(src,key) ? PyString_AsString(PyDict_GetItemString(src,key)) : default)

typedef int DLPERROR;
typedef int DLPDBERROR;
 typedef int PIERROR;

static PyObject *Error;
%}

%include typemaps.i

// initialize a buffer for use later
%init %{
    __dlp_buf = (void *)PyMem_Malloc(DLPMAXBUF);
    Error = PyErr_NewException("pisock.error", NULL, NULL);
    PyDict_SetItemString(d, "error", Error);
%}

//
//  Socket stuff (from pi-socket.h)
//
#define PI_AF_SLP 		0x0051        /* arbitrary, for completeness, just in case */
#define PI_AF_INETSLP 		0x0054    

#define PI_PF_SLP    		PI_AF_SLP
#define PI_PF_PADP   		0x0052
#define PI_PF_LOOP   		0x0053

#define PI_SOCK_STREAM    	0x0010
#define PI_SOCK_DGRAM     	0x0020
#define PI_SOCK_RAW       	0x0030
#define PI_SOCK_SEQPACKET 	0x0040

#define PI_SLP_SPEED		0x0001

#define PI_PilotSocketDLP       3
#define PI_PilotSocketConsole   1
#define PI_PilotSocketDebugger  0
#define PI_PilotSocketRemoteUI  2

//
// pi-sockaddr... the real structure might be defined in one of two
// different ways, but luckily SWIG doesn't really care.
//
%typemap (python,in) struct sockaddr *INPUT {
    static struct pi_sockaddr temp;
    char *dev;

    if (!PyArg_ParseTuple($source, "is", &temp.pi_family, &dev)) {
	return NULL;
    }

    strncpy(temp.pi_device, dev, 13);
    temp.pi_device[13] = 0;

    $target = (struct sockaddr *)&temp;
}

%typemap (python, argout) struct sockaddr *OUTPUT {
    PyObject *o;

    if ($source) {
	o = Py_BuildValue("(is)", (int)((struct pi_sockaddr *)$source)->pi_family,
			  ((struct pi_sockaddr *)$source)->pi_device);
	$target = t_output_helper($target, o);
    }
}

%typemap (python,ignore) struct sockaddr *OUTPUT (struct pi_sockaddr temp) {
    $target = (struct sockaddr *)&temp;
}

%typemap (python,ignore) int addrlen {
    $target = sizeof(struct pi_sockaddr);
}

extern int pi_socket (int domain, int type, int protocol);
extern int pi_connect (int pi_sd, struct sockaddr *INPUT, int addrlen);
extern int pi_bind (int pi_sd, struct sockaddr *INPUT, int addrlen);
extern int pi_listen (int pi_sd, int backlog);
extern int pi_accept (int pi_sd, struct sockaddr *OUTPUT, int *OUTPUT);

extern int pi_accept_to (int pi_sd, struct sockaddr *OUTPUT, int *OUTPUT, int timeout);

extern int pi_send (int pi_sd, void *msg, int len, unsigned int flags);
extern int pi_recv (int pi_sd, void *msg, int len, unsigned int flags);

extern int pi_read (int pi_sd, void *msg, int len);
extern int pi_write (int pi_sd, void *msg, int len);

extern int pi_getsockname (int pi_sd, struct sockaddr *OUTPUT, int *OUTPUT);
extern int pi_getsockpeer (int pi_sd, struct sockaddr *OUTPUT, int *OUTPUT);

// extern int pi_setmaxspeed (int pi_sd, int speed, int overclock);
extern int pi_getsockopt (int pi_sd, int level, int option_name, void * option_value, int * option_len);

extern int pi_version (int pi_sd);

extern int pi_tickle (int pi_sd);
extern int pi_watchdog (int pi_sd, int interval);

extern int pi_close (int pi_sd);


//
//  DLP (from pi-dlp.h)
//

// struct PilotUser
%typemap (python,in) struct PilotUser * {
    static struct PilotUser temp;
    int l;
    PyObject *foo;

    temp.userID = DGETLONG($source,"userID",0);
    temp.viewerID = DGETLONG($source,"viewerID",0);
    temp.lastSyncPC = DGETLONG($source,"lastSyncPC",0);
    temp.successfulSyncDate = DGETLONG($source,"successfulSyncDate",0);
    temp.lastSyncDate = DGETLONG($source,"lastSyncDate",0);
    strncpy(temp.username, DGETSTR($source,"name",""), 128);

    foo = PyDict_GetItemString($source,"password");
    if (PyString_Check(foo)) {
	l = PyString_Size(foo);
	temp.passwordLength = l;
	memcpy(temp.password, PyString_AsString(foo), l);
    }
    
    $target = &temp;    
}

%typemap (python,argout) struct PilotUser *OUTPUT {
    PyObject *o;
    
    if ($source) {
	o = Py_BuildValue("{slslslslslssss#}",
			  "userID", $source->userID,
			  "viewerID", $source->viewerID,
			  "lastSyncPC", $source->lastSyncPC,
			  "successfulSyncDate", $source->successfulSyncDate,
			  "lastSyncDate", $source->lastSyncDate,
			  "name", $source->username,
			  "password", $source->password, $source->passwordLength);
        $target = t_output_helper($target, o);
    }
}

%typemap (python,ignore) struct PilotUser *OUTPUT (struct PilotUser temp) {
    $target = &temp;
}

// struct SysInfo
%typemap (python,argout) struct SysInfo *OUTPUT {
    PyObject *o;
    
    if ($source) {
	o = Py_BuildValue("{slslss#}",
			  "romVersion", $source->romVersion,
			  "locale", $source->locale,
			  "name", $source->name, $source->nameLength);
	$target = t_output_helper($target, o);
    }
}

%typemap (python,ignore) struct SysInfo *OUTPUT (struct SysInfo temp) {
    $target = &temp;
}

// struct DBInfo
%typemap (python,argout) struct DBInfo *OUTPUT {
    PyObject *o;

    if ($source) {
	o = Py_BuildValue("{sisisisOsOsislslslslsisssisisisisisisisisisisi}",
			  "more", $source->more,
			  "flags", $source->flags,
			  "miscFlags", $source->miscFlags,
			  "type", PyString_FromStringAndSize(printlong($source->type), 4),
			  "creator", PyString_FromStringAndSize(printlong($source->creator), 4),
			  "version", $source->version,
			  "modnum", $source->modnum,
			  "createDate", $source->createDate,
			  "modifyDate", $source->modifyDate,
			  "backupDate", $source->backupDate,
			  "index", $source->index,
			  "name", $source->name,

			  "flagResource", !!($source->flags & dlpDBFlagResource),
			  "flagReadOnly", !!($source->flags & dlpDBFlagReadOnly),
			  "flagAppInfoDirty", !!($source->flags & dlpDBFlagAppInfoDirty),
			  "flagBackup", !!($source->flags & dlpDBFlagBackup),
			  "flagClipping", !!($source->flags & dlpDBFlagClipping),
			  "flagOpen", !!($source->flags & dlpDBFlagOpen),
			  "flagNewer", !!($source->flags & dlpDBFlagNewer),
			  "flagReset", !!($source->flags & dlpDBFlagReset),
			  "flagCopyPrevention", !!($source->flags & dlpDBFlagCopyPrevention),
			  "flagStream", !!($source->flags & dlpDBFlagStream),
			  "flagExcludeFromSync", !!($source->miscFlags & dlpDBMiscFlagExcludeFromSync));
	$target = t_output_helper($target, o);
    }
}

%typemap (python,ignore) struct DBInfo *OUTPUT (struct DBInfo temp) {
    $target = &temp;
}

%typemap (python,in) struct DBInfo * {
    static struct DBInfo temp;

    temp.more = (int) DGETLONG($source, "more", 0);
    temp.type = makelong(DGETSTR($source, "type", "    "));
    temp.creator = makelong(DGETSTR($source, "creator", "    "));
    temp.version = DGETLONG($source, "version", 0);
    temp.modnum = DGETLONG($source, "modnum", 0);
    temp.createDate = DGETLONG($source, "createDate", 0);
    temp.modifyDate = DGETLONG($source, "modifyDate", 0);
    temp.backupDate = DGETLONG($source, "backupDate", 0);
    temp.index = DGETLONG($source, "index", 0);
    strncpy(temp.name, DGETSTR($source,"name",""), 34);
    temp.flags = 0;
    if (DGETLONG($source,"flagResource",0)) temp.flags |= dlpDBFlagResource;
    if (DGETLONG($source,"flagReadOnly",0)) temp.flags |= dlpDBFlagReadOnly;
    if (DGETLONG($source,"flagAppInfoDirty",0)) temp.flags |= dlpDBFlagAppInfoDirty;
    if (DGETLONG($source,"flagBackup",0)) temp.flags |= dlpDBFlagBackup;
    if (DGETLONG($source,"flagClipping",0)) temp.flags |= dlpDBFlagClipping;
    if (DGETLONG($source,"flagOpen",0)) temp.flags |= dlpDBFlagOpen;
    if (DGETLONG($source,"flagNewer",0)) temp.flags |= dlpDBFlagNewer;
    if (DGETLONG($source,"flagReset",0)) temp.flags |= dlpDBFlagReset;
    if (DGETLONG($source,"flagCopyPrevention",0)) temp.flags |= dlpDBFlagCopyPrevention;
    if (DGETLONG($source,"flagStream",0)) temp.flags |= dlpDBFlagStream;
    temp.miscFlags = 0;
    if (DGETLONG($source,"flagExcludeFromSync",0)) temp.miscFlags |= dlpDBMiscFlagExcludeFromSync;
    $target = &temp;
}
    
    
// struct CardInfo
%typemap (python,argout) struct CardInfo *OUTPUT {
    PyObject *o;

    if ($source) {
	o = Py_BuildValue("{sisislslslslsssssi}",
			  "card", $source->card,
			  "version", $source->version,
			  "creation", $source->creation,
			  "romSize", $source->romSize,
			  "ramSize", $source->ramSize,
			  "ramFree", $source->ramFree,
			  "name", $source->name,
			  "manufacturer", $source->manufacturer,
			  "more", $source->more);
	$target = t_output_helper($target, o);
    }
}

%typemap (python,ignore) struct CardInfo *OUTPUT (struct CardInfo temp) {
    $target = &temp;
}

%typemap (python,argout) struct NetSyncInfo *OUTPUT {
    PyObject *o;
    if ($source){
	o = Py_BuildValue("{sissssss}",
			  "lanSync", $source->lanSync,
			  "hostName", $source->hostName,
			  "hostAddress", $source->hostAddress,
			  "hostSubnetMask", $source->hostSubnetMask);
	$target = t_output_helper($target, o);
    }
}

%typemap (python,ignore) struct NetSyncInfo *OUTPUT (struct NetSyncInfo temp) {
    $target = &temp;
}

%typemap (python,in) struct NetSyncInfo * {
    static struct NetSyncInfo temp;

    temp.lanSync = (int) DGETLONG($source,"lanSync",0);
    strncpy(temp.hostName, DGETSTR($source,"hostName",""), 256);
    strncpy(temp.hostAddress, DGETSTR($source,"hostAddress",""), 40);
    strncpy(temp.hostSubnetMask, DGETSTR($source,"hostSubnetMask",""), 40);

    $target = &temp;
}

// a generic 4-character string type, for use as a type or creator ID
%typemap (python,in) unsigned long STR4 {
    if (!($source) || ($source == Py_None)) {
	$target = 0;
    } else {
	if (!PyString_Check($source) || (PyString_Size($source) != 4)) {
	    PyErr_SetString(PyExc_ValueError, "argument must be a 4-character string");
	    return 0;
	}
	$target = makelong(PyString_AsString($source));
    }
}

%typemap (python,in) long STR4 {
    if (!($source) || ($source == Py_None)) {
	$target = 0;
    } else {
	if (!PyString_Check($source) || (PyString_Size($source) != 4)) {
	    PyErr_SetString(PyExc_ValueError, "argument must be a 4-character string");
	    return 0;
	}
	$target = makelong(PyString_AsString($source));
    }
}

%typemap (python,argout) unsigned long *OUTSTR4 {
    PyObject *o;
    if ($source) {
	o = PyString_FromStringAndSize(printlong(*$source), 4);
	$target = t_output_helper($target, o);
    }
}

%typemap (python,ignore) unsigned long *OUTSTR4 (unsigned long temp) {
    $target = &temp;
}

//

// a char value that allows None for a null value.
%typemap (python,in) char *ALLOWNULL {
    if (!($source) || ($source == Py_None)) {
	$target = NULL;
    } else {
	$target = PyString_AsString($source);
    }
}


enum dlpDBFlags {
	dlpDBFlagResource = 0x0001, /* Resource DB, instead of record DB */
	dlpDBFlagReadOnly = 0x0002, /* DB is read only */
	dlpDBFlagAppInfoDirty = 0x0004, /* AppInfo data has been modified */
	dlpDBFlagBackup = 0x0008, /* DB is tagged for generic backup */
	dlpDBFlagClipping = 0x0200, /* DB is a Palm Query Application (PQA) */
	dlpDBFlagOpen = 0x8000, /* DB is currently open */
	
	/* v2.0 specific */
	dlpDBFlagNewer = 0x0010, /* Newer version may be installed over open DB */
	dlpDBFlagReset = 0x0020, /* Reset after installation */

	/* v3.0 specific */
	dlpDBFlagCopyPrevention = 0x0040, /* DB should not be beamed */
	dlpDBFlagStream = 0x0080          /* DB implements a file stream */
};

enum dlpDBMiscFlags {
	dlpDBMiscFlagExcludeFromSync = 0x80
};

enum dlpRecAttributes {
	dlpRecAttrDeleted = 0x80, /* tagged for deletion during next sync */
	dlpRecAttrDirty   = 0x40, /* record modified */
	dlpRecAttrBusy    = 0x20, /* record locked  */
	dlpRecAttrSecret  = 0x10, /* record is secret*/
	dlpRecAttrArchived= 0x08 /* tagged for archival during next sync*/
};

enum dlpOpenFlags {
        dlpOpenRead = 0x80,
        dlpOpenWrite = 0x40,
        dlpOpenExclusive = 0x20,
        dlpOpenSecret = 0x10,
        dlpOpenReadWrite = 0xC0
};

enum dlpEndStatus {
        dlpEndCodeNormal = 0,  /* Normal */
        dlpEndCodeOutOfMemory, /* End due to low memory on Pilot */
        dlpEndCodeUserCan,     /* Cancelled by user */
        dlpEndCodeOther        /* dlpEndCodeOther and higher mean "Anything else" */
};

enum dlpDBList {
	dlpDBListRAM = 0x80,
	dlpDBListROM = 0x40
};

enum dlpErrors { 
  dlpErrNoError = -1,
  dlpErrSystem  = -2,
  dlpErrMemory  = -3,
  dlpErrParam   = -4,
  dlpErrNotFound = -5,
  dlpErrNoneOpen = -6,
  dlpErrAlreadyOpen = -7,
  dlpErrTooManyOpen = -8,
  dlpErrExists = -9,
  dlpErrOpen = -10,
  dlpErrDeleted = -11,
  dlpErrBusy = -12,
  dlpErrNotSupp = -13,
  dlpErrUnused1 = -14,
  dlpErrReadOnly = -15,
  dlpErrSpace = -16,
  dlpErrLimit = -17,
  dlpErrSync = -18,
  dlpErrWrapper = -19,
  dlpErrArgument = -20,
  dlpErrSize = -21,
  dlpErrUnknown = -128
};

extern char * dlp_errorlist[];
extern char * dlp_strerror(int error);

// note: $source is a pointer, because swig is treating DLPERROR as an opaque
// type.  but since we typedef it to int, the compiler doesn't complain...
%typemap (python,out) DLPERROR {
    if (*($source) < 0) {
	PyErr_SetObject(Error, Py_BuildValue("(is)", *($source),
					     dlp_strerror(*($source))));
	return NULL;
    }
    $target = Py_None;
    Py_INCREF(Py_None);
}
%typemap (python,out) DLPDBERROR {
    if (*($source) == -5) {
	Py_INCREF(Py_None);
	return Py_None;
    } else if (*($source) < 0) {
	PyErr_SetObject(Error, Py_BuildValue("(is)", *($source),
					     dlp_strerror(*($source))));
	return NULL;
    }
    $target = Py_None;
    Py_INCREF(Py_None);
}


/*  %typemap (python,in) time_t time { */
/*      if (!($source) || ($source == Py_None)) { */
/*  	$target = 0; */
/*      } else { */
/*  	if (!PyInt_Check($source)) { */
/*  	    PyErr_SetString(PyExc_ValueError, "argument must be an integer"); */
/*  	    return 0; */
/*  	} */
/*  	$target = PyInt_AsLong($source); */
/*      } */
/*  } */
%apply long { time_t }

// DLP functions
extern DLPERROR dlp_GetSysDateTime (int sd, time_t *OUTPUT);
extern DLPERROR dlp_SetSysDateTime (int sd, time_t INPUT);
extern DLPERROR dlp_ReadStorageInfo (int sd, int cardno, struct CardInfo *OUTPUT);
extern DLPERROR dlp_ReadSysInfo (int sd, struct SysInfo *OUTPUT);
extern DLPERROR dlp_ReadDBList (int sd, int cardno, int flags, int start,
				struct DBInfo *OUTPUT);
// note: creator and type are 4-char strings or None, and name is a string or None.
extern DLPERROR dlp_FindDBInfo (int sd, int cardno, int start, char *ALLOWNULL,
				unsigned long STR4,
				unsigned long STR4, struct DBInfo *OUTPUT);
extern DLPERROR dlp_OpenDB (int sd, int cardno, int mode, char * name, int *OUTPUT);
extern DLPERROR dlp_CloseDB (int sd, int dbhandle);
extern DLPERROR dlp_CloseDB_All (int sd);
extern DLPERROR dlp_DeleteDB (int sd, int cardno, const char * name);
// note: creator and type are 4-char strings.
extern DLPERROR dlp_CreateDB (int sd, long STR4, long STR4, int cardno,
			      int flags, int version, const char * name, int *OUTPUT);
extern DLPERROR dlp_ResetSystem (int sd);
extern DLPERROR dlp_AddSyncLogEntry (int sd, char * entry);
extern DLPERROR dlp_OpenConduit (int sd);
extern DLPERROR dlp_EndOfSync (int sd, int status);
extern DLPERROR dlp_AbortSync (int sd);
extern DLPERROR dlp_ReadOpenDBInfo (int sd, int dbhandle, int *OUTPUT);
extern DLPERROR dlp_MoveCategory (int sd, int handle, int fromcat, int tocat);
extern DLPERROR dlp_WriteUserInfo (int sd, struct PilotUser *User);
extern DLPERROR dlp_ReadUserInfo (int sd, struct PilotUser *OUTPUT);
extern DLPERROR dlp_ResetLastSyncPC (int sd);

// The kludge hits.-more- You die.
//
// This is a TOTAL hack.  As far as I know, the order in which SWIG processes typemaps
// is never specified anywhere.  However, it appears that it does them in the basic
// order of
//    - "ignore" typemaps
//    - basic argument processing
//    - "in" typemaps
//    - "check" typemaps
//    - ...
%typemap (python,in) void *INBUF (int __buflen) {
    __buflen = PyString_Size($source);
    $target = (void *)PyString_AsString($source);
}

%typemap (python,ignore) int INBUFLEN {
}
%typemap (python,check) int INBUFLEN {
    $target = __buflen;
}

%typemap (python,argout) void *OUTBUF {
    PyObject *o;
    if ($source) {
	o = Py_BuildValue("s#", $source, __buflen);
	$target = t_output_helper($target, o);
    }
}
%typemap (python,ignore) void *OUTBUF {
    $target = __dlp_buf;
}
%typemap (python,ignore) int  *OUTBUFLEN (int __buflen) {
    $target = &__buflen;
}

%typemap (python,ignore) int DLPMAXBUF {
    $target = DLPMAXBUF;
}

%apply unsigned long { recordid_t };


%native(dlp_ReadAppBlock) PyObject *_wrap_dlp_ReadAppBlock(PyObject *, PyObject *);

extern DLPERROR dlp_WriteAppBlock(int sd, int dbhandle, const void *INBUF, int INBUFLEN);

%native(dlp_ReadSortBlock) PyObject *_wrap_dlp_ReadSortBlock(PyObject *, PyObject *);

extern DLPERROR dlp_WriteSortBlock(int sd, int dbhandle, const void *INBUF, int INBUFLEN);

extern DLPERROR dlp_ResetDBIndex (int sd, int dbhandle);

%native(dlp_ReadRecordIDList) PyObject *_wrap_dlp_ReadRecordIDList(PyObject *, PyObject *);

extern DLPDBERROR dlp_WriteRecord (int sd, int dbhandle, int flags,
				 recordid_t INPUT, int catID, void *INBUF,
				 int INBUFLEN, recordid_t *OUTPUT);
extern DLPERROR dlp_DeleteRecord (int sd, int dbhandle, int all, recordid_t INPUT);
extern DLPERROR dlp_DeleteCategory (int sd, int dbhandle, int category);

extern DLPDBERROR dlp_ReadResourceByType (int sd, int fHandle, unsigned long STR4, int id,
				   void *OUTBUF, 
				   int *OUTPUT, int *OUTBUFLEN);
extern DLPDBERROR dlp_ReadResourceByIndex (int sd, int fHandle, int index, void *OUTBUF,
                          unsigned long *OUTSTR4, int *OUTPUT, int *OUTBUFLEN);

extern DLPDBERROR dlp_WriteResource (int sd, int dbhandle, unsigned long STR4, int id,
				   const void *INBUF, int INBUFLEN);
extern DLPERROR dlp_DeleteResource (int sd, int dbhandle, int all, unsigned long STR4,
				    int resID);

extern DLPDBERROR dlp_ReadNextModifiedRec (int sd, int fHandle, void *OUTBUF,
					 recordid_t *OUTPUT, int *OUTPUT, int *OUTBUFLEN,
					 int *OUTPUT,
					 int *OUTPUT);
extern DLPDBERROR dlp_ReadNextModifiedRecInCategory (int sd, int fHandle, int incategory,
						   void *OUTBUF,
						   recordid_t *OUTPUT, int *OUTPUT,
						   int *OUTBUFLEN, int *OUTPUT);
extern DLPDBERROR dlp_ReadNextRecInCategory (int sd, int fHandle, int incategory,
					   void *OUTBUFLEN,
					   recordid_t *OUTPUT, int *OUTPUT,
					   int *OUTBUFLEN, int *OUTPUT);
extern DLPDBERROR dlp_ReadRecordById (int sd, int fHandle, recordid_t INPUT, void *OUTBUF, 
				    int *OUTPUT, int *OUTBUFLEN, int *OUTPUT, int *OUTPUT);
extern DLPDBERROR dlp_ReadRecordByIndex (int sd, int fHandle, int index, void *OUTBUF, 
				       recordid_t *OUTPUT, int *OUTBUFLEN,
				       int *OUTPUT, int *OUTPUT);
extern DLPERROR dlp_CleanUpDatabase (int sd, int fHandle);
extern DLPERROR dlp_ResetSyncFlags (int sd, int fHandle);
// complex enough to probably need native code.
//extern int dlp_CallApplication (int sd, unsigned long STR4, unsigned long STR4, int action,
//                        int length, void * data,
//                        unsigned long *OUTPUT, int maxretlen, int *OUTPUT, void * retdata);
extern DLPERROR dlp_ReadFeature (int sd, unsigned long STR4, unsigned int num, 
				 unsigned long *OUTPUT);
extern DLPERROR dlp_ReadNetSyncInfo (int sd, struct NetSyncInfo *OUTPUT);
extern DLPERROR dlp_WriteNetSyncInfo (int sd, struct NetSyncInfo * i);
extern DLPERROR dlp_ReadAppPreference (int sd, unsigned long STR4, int id, int backup,
				  int DLPMAXBUF, void *OUTBUF, int *OUTBUFLEN, int *OUTPUT);
extern DLPERROR dlp_WriteAppPreference (int sd, unsigned long STR4, int id, int backup,
					int version, void *INBUF, int INBUFLEN);
// and the most complex of all... i'm not even sure how it works.
//extern int dlp_RPC (int sd, struct RPC_params * p, unsigned long *OUTPUT);

// for functions that return 0 on success or something else on error
%typemap (python,out) PIERROR {
    if (*($source) != 0) {
	PyErr_SetObject(Error, Py_BuildValue("(is)", *($source),
					     "pisock error"));
	return NULL;
    }
    $target = Py_None;
    Py_INCREF(Py_None);
}

%typemap (python,argout) void **OUTBUF {
    PyObject *o;
    if ($source) {
	o = Py_BuildValue("s#", *($source), __buflen);
	$target = t_output_helper($target, o);
    }
}
%typemap (python,ignore) void **OUTBUF {
    static void *foo;
    $target = &foo;
}


/* pi-file */

extern struct pi_file *pi_file_open (char *name);
extern PIERROR pi_file_close (struct pi_file *pf);
extern PIERROR pi_file_get_info (struct pi_file *pf, struct DBInfo *OUTPUT);
extern PIERROR pi_file_get_app_info  (struct pi_file *pf, void **OUTBUF,
					      int *OUTBUFLEN);
extern PIERROR pi_file_get_sort_info (struct pi_file *pf, void **OUTBUF,
					      int *OUTBUFLEN);
extern PIERROR pi_file_read_resource (struct pi_file *pf, int idx, void **OUTBUF,
				  int *OUTBUFLEN, unsigned long *OUTSTR4,
				  int *OUTPUT);
extern PIERROR pi_file_read_resource_by_type_id (struct pi_file *pf,
					     unsigned long STR4, int id,
					     void **OUTBUF, int *OUTBUFLEN, int *OUTPUT);
extern PIERROR pi_file_type_id_used (struct pi_file *pf, unsigned long STR4, int id);
extern PIERROR pi_file_read_record (struct pi_file *pf, int idx, void **OUTBUF, int *OUTBUFLEN,
				    int *OUTPUT, int *OUTPUT, recordid_t *OUTPUT);
extern PIERROR pi_file_get_entries (struct pi_file *pf, int *OUTPUT);
extern PIERROR pi_file_read_record_by_id (struct pi_file *pf, recordid_t INPUT, void **OUTBUF,
					  int *OUTBUFLEN, int *OUTPUT, int *OUTPUT,
					  int *OUTPUT);
extern PIERROR pi_file_id_used (struct pi_file *pf, recordid_t INPUT);
extern struct pi_file *pi_file_create (char *name, struct DBInfo *INPUT);
extern PIERROR pi_file_set_info (struct pi_file *pf, struct DBInfo *INPUT);
extern PIERROR pi_file_set_app_info (struct pi_file *pf, void *INBUF, int INBUFLEN);
extern PIERROR pi_file_set_sort_info (struct pi_file *pf, void *INBUF, int INBUFLEN);
extern PIERROR pi_file_append_resource (struct pi_file *pf, void *INBUF, int INBUFLEN,
					unsigned long STR4, int id);
extern PIERROR pi_file_append_record (struct pi_file *pf, void *INBUF, int INBUFLEN,
				      int attr, int category, recordid_t INPUT);
extern PIERROR pi_file_retrieve (struct pi_file *pf, int socket, int cardno);
extern PIERROR pi_file_install (struct pi_file *pf, int socket, int cardno);
extern PIERROR pi_file_merge (struct pi_file *pf, int socket, int cardno);

// pi-inet / pi-inetserial
// pi-padp
// pi-serial
// pi-slp
// pi-sync


// and some miscellaneous things 
void print_splash (char *progname); 

%{

#define PYCFUNC(x) static PyObject *x (PyObject *self, PyObject *args)

PYCFUNC(_wrap_dlp_ReadAppBlock) {
    int a0, a1, a2;
    int ret;
    PyObject *o;
    
    if (!PyArg_ParseTuple(args,"iii", &a0, &a1, &a2))
	return NULL;

    ret = dlp_ReadAppBlock(a0, a1, a2, __dlp_buf, DLPMAXBUF);
    if (ret < 0) {
	PyErr_SetObject(Error, Py_BuildValue("(is)", ret, dlp_strerror(ret)));
	return NULL;
    } else if (ret > 0) {
	o = Py_BuildValue("s#", __dlp_buf, ret);
    } else {
	o = Py_None;
	Py_INCREF(Py_None);
    }
    return o;
}

PYCFUNC(_wrap_dlp_ReadSortBlock) {
    int a0, a1, a2;
    int ret;
    PyObject *o;
    
    if (!PyArg_ParseTuple(args,"iii", &a0, &a1, &a2))
	return NULL;

    ret = dlp_ReadSortBlock(a0, a1, a2, __dlp_buf, DLPMAXBUF);
    if (ret < 0) {
	PyErr_SetObject(Error, Py_BuildValue("(is)", ret, dlp_strerror(ret)));
	return NULL;
    } else if (ret > 0) {
	o = Py_BuildValue("s#", __dlp_buf, ret);
    } else {
	o = Py_None;
	Py_INCREF(Py_None);
    }
    return o;
}

/* sd, dbf, sort, start, max, recordid_t *IDS, int *count
 */
PYCFUNC(_wrap_dlp_ReadRecordIDList) {
    int sd, dbf, sort, start, max;
    int ret;
    recordid_t *buf;
    int count, i;
    PyObject *list;
    
    if (!PyArg_ParseTuple(args, "iiiii", &sd, &dbf, &sort, &start, &max))
	return NULL;

/* this is a rather simplistic wrapper.  if max is too big, we just
 * refuse to do it; we don't loop, figuring that that is the job of
 * the python wrapper.
 */
    if (max > (0xFFFF/sizeof(recordid_t))) {
	PyErr_SetString(PyExc_ValueError, "can only return about 64k worth of ids at once");
	return NULL;
    }

    buf = (recordid_t *)__dlp_buf;
    ret = dlp_ReadRecordIDList(sd, dbf, sort, start, max, buf, &count);

    if (ret < 0) {
	PyErr_SetObject(Error, Py_BuildValue("(is)", ret, dlp_strerror(ret)));
	return NULL;
    } else {
	list = PyList_New(0);
	for (i=0; i<count; i++)
	    PyList_Append(list, PyInt_FromLong((long)buf[i]));
	return list;
    }
}

%}
