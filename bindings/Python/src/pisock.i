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


%module pisock
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
#define PI_AF_PILOT             0x00

#define PI_PF_DEV               0x01
#define PI_PF_SLP               0x02
#define PI_PF_SYS               0x03
#define PI_PF_PADP              0x04
#define PI_PF_NET               0x05
#define PI_PF_DLP               0x06

#define PI_SOCK_STREAM          0x0010
#define PI_SOCK_RAW             0x0030

#define PI_CMD_CMP 0x01
#define PI_CMD_NET 0x02
#define PI_CMD_SYS 0x03

#define PI_MSG_PEEK 0x01

enum PiOptLevels {
        PI_LEVEL_DEV,
        PI_LEVEL_SLP,
        PI_LEVEL_PADP,
        PI_LEVEL_NET,
        PI_LEVEL_SYS,
        PI_LEVEL_CMP,
        PI_LEVEL_DLP,
        PI_LEVEL_SOCK
};

enum PiOptDevice {
        PI_DEV_RATE,
        PI_DEV_ESTRATE,
        PI_DEV_HIGHRATE,
        PI_DEV_TIMEOUT
};

enum PiOptSLP {
        PI_SLP_DEST,
        PI_SLP_LASTDEST,
        PI_SLP_SRC,
        PI_SLP_LASTSRC,
        PI_SLP_TYPE,
        PI_SLP_LASTTYPE,
        PI_SLP_TXID,
        PI_SLP_LASTTXID
};

enum PiOptPADP {
        PI_PADP_TYPE,
        PI_PADP_LASTTYPE
};

enum PiOptCMP {
        PI_CMP_TYPE,
        PI_CMP_FLAGS,
        PI_CMP_VERS,
        PI_CMP_BAUD
};

enum PiOptNet {
        PI_NET_TYPE
};

enum PiOptSock {
        PI_SOCK_STATE
};

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
    int len;

    if (!PyArg_ParseTuple($input, "is#", &temp.pi_family, &dev, &len)) {
	return NULL;
    }
    if (len > 255) {
      // Should really raise an exception
      len = 255;
    }
    strncpy(temp.pi_device, dev, len);
    temp.pi_device[len] = 0;

    $1 = (struct sockaddr *)&temp;
}

%typemap (python, argout) struct sockaddr *OUTPUT {
    PyObject *o;

    if ($1) {
	o = Py_BuildValue("(is)", (int)((struct pi_sockaddr *)$1)->pi_family,
			  ((struct pi_sockaddr *)$1)->pi_device);
	$result = t_output_helper($result, o);
    }
}

%typemap (python,in,numinputs=0) struct sockaddr *OUTPUT (struct pi_sockaddr temp) {
    $1 = (struct sockaddr *)&temp;
}

%typemap (python,in,numinputs=0) int addrlen {
    $1 = sizeof(struct pi_sockaddr);
}

extern int pilot_connect (const char *port);
extern int pi_socket (int domain, int type, int protocol);
extern int pi_connect (int pi_sd, struct sockaddr *INPUT, int addrlen);
extern int pi_bind (int pi_sd, struct sockaddr *INPUT, int addrlen);
extern int pi_listen (int pi_sd, int backlog);
extern int pi_accept (int pi_sd, struct sockaddr *OUTPUT, size_t *OUTPUT);

extern int pi_accept_to (int pi_sd, struct sockaddr *OUTPUT, size_t *OUTPUT, int timeout);

extern ssize_t pi_send (int pi_sd, void *msg, size_t len, int flags);
extern ssize_t pi_recv (int pi_sd, pi_buffer_t *msg, size_t len, int flags);

extern int pi_read (int pi_sd, pi_buffer_t *msg, size_t len);
extern int pi_write (int pi_sd, void *msg, size_t len);

extern int pi_getsockname (int pi_sd, struct sockaddr *OUTPUT, size_t *OUTPUT);
extern int pi_getsockpeer (int pi_sd, struct sockaddr *OUTPUT, size_t *OUTPUT);

/* Not supported since 1.37.2.3 2002/01/06 07:05:27 , appears we should use
   pi_setsockopt now
extern int pi_setmaxspeed (int pi_sd, int speed, int overclock);
*/
extern int pi_setsockopt (int pi_sd, int level, int option_name, const void *option_value, size_t *option_len);
extern int pi_getsockopt (int pi_sd, int level, int option_name, void * option_value, size_t * option_len);

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

    temp.userID = DGETLONG($input,"userID",0);
    temp.viewerID = DGETLONG($input,"viewerID",0);
    temp.lastSyncPC = DGETLONG($input,"lastSyncPC",0);
    temp.successfulSyncDate = DGETLONG($input,"successfulSyncDate",0);
    temp.lastSyncDate = DGETLONG($input,"lastSyncDate",0);
    strncpy(temp.username, DGETSTR($input,"name",""), 128);

    foo = PyDict_GetItemString($input,"password");
    if (PyString_Check(foo)) {
	l = PyString_Size(foo);
	temp.passwordLength = l;
	memcpy(temp.password, PyString_AsString(foo), l);
    }
    
    $1 = &temp;    
}

%typemap (python,argout) struct PilotUser *OUTPUT {
    PyObject *o;
    
    if ($1) {
	o = Py_BuildValue("{slslslslslssss#}",
			  "userID", $1->userID,
			  "viewerID", $1->viewerID,
			  "lastSyncPC", $1->lastSyncPC,
			  "successfulSyncDate", $1->successfulSyncDate,
			  "lastSyncDate", $1->lastSyncDate,
			  "name", $1->username,
			  "password", $1->password, $1->passwordLength);
        $result = t_output_helper($result, o);
    }
}

%typemap (python,in,numinputs=0) struct PilotUser *OUTPUT (struct PilotUser temp) {
    $1 = &temp;
}

// struct SysInfo
%typemap (python,argout) struct SysInfo *OUTPUT {
    PyObject *o;
    
    if ($1) {
	o = Py_BuildValue("{slslss#}",
			  "romVersion", $1->romVersion,
			  "locale", $1->locale,
			  "name", $1->prodID, $1->prodIDLength);
	$result = t_output_helper($result, o);
    }
}

%typemap (python,in,numinputs=0) struct SysInfo *OUTPUT (struct SysInfo temp) {
    $1 = &temp;
}

// struct DBInfo
%typemap (python,argout) struct DBInfo *OUTPUT {
    PyObject *o;

    if ($1) {
	o = Py_BuildValue("{sisisisOsOsislslslslsisssisisisisisisisisisisi}",
			  "more", $1->more,
			  "flags", $1->flags,
			  "miscFlags", $1->miscFlags,
			  "type", PyString_FromStringAndSize(printlong($1->type), 4),
			  "creator", PyString_FromStringAndSize(printlong($1->creator), 4),
			  "version", $1->version,
			  "modnum", $1->modnum,
			  "createDate", $1->createDate,
			  "modifyDate", $1->modifyDate,
			  "backupDate", $1->backupDate,
			  "index", $1->index,
			  "name", $1->name,

			  "flagResource", !!($1->flags & dlpDBFlagResource),
			  "flagReadOnly", !!($1->flags & dlpDBFlagReadOnly),
			  "flagAppInfoDirty", !!($1->flags & dlpDBFlagAppInfoDirty),
			  "flagBackup", !!($1->flags & dlpDBFlagBackup),
			  "flagClipping", !!($1->flags & dlpDBFlagClipping),
			  "flagOpen", !!($1->flags & dlpDBFlagOpen),
			  "flagNewer", !!($1->flags & dlpDBFlagNewer),
			  "flagReset", !!($1->flags & dlpDBFlagReset),
			  "flagCopyPrevention", !!($1->flags & dlpDBFlagCopyPrevention),
			  "flagStream", !!($1->flags & dlpDBFlagStream),
			  "flagExcludeFromSync", !!($1->miscFlags & dlpDBMiscFlagExcludeFromSync));
	$result = t_output_helper($result, o);
    }
}

%typemap (python,in,numinputs=0) struct DBInfo *OUTPUT (struct DBInfo temp) {
    $1 = &temp;
}

%typemap (python,in) struct DBInfo * {
    static struct DBInfo temp;

    temp.more = (int) DGETLONG($input, "more", 0);
    temp.type = makelong(DGETSTR($input, "type", "    "));
    temp.creator = makelong(DGETSTR($input, "creator", "    "));
    temp.version = DGETLONG($input, "version", 0);
    temp.modnum = DGETLONG($input, "modnum", 0);
    temp.createDate = DGETLONG($input, "createDate", 0);
    temp.modifyDate = DGETLONG($input, "modifyDate", 0);
    temp.backupDate = DGETLONG($input, "backupDate", 0);
    temp.index = DGETLONG($input, "index", 0);
    strncpy(temp.name, DGETSTR($input,"name",""), 34);
    temp.flags = 0;
    if (DGETLONG($input,"flagResource",0)) temp.flags |= dlpDBFlagResource;
    if (DGETLONG($input,"flagReadOnly",0)) temp.flags |= dlpDBFlagReadOnly;
    if (DGETLONG($input,"flagAppInfoDirty",0)) temp.flags |= dlpDBFlagAppInfoDirty;
    if (DGETLONG($input,"flagBackup",0)) temp.flags |= dlpDBFlagBackup;
    if (DGETLONG($input,"flagClipping",0)) temp.flags |= dlpDBFlagClipping;
    if (DGETLONG($input,"flagOpen",0)) temp.flags |= dlpDBFlagOpen;
    if (DGETLONG($input,"flagNewer",0)) temp.flags |= dlpDBFlagNewer;
    if (DGETLONG($input,"flagReset",0)) temp.flags |= dlpDBFlagReset;
    if (DGETLONG($input,"flagCopyPrevention",0)) temp.flags |= dlpDBFlagCopyPrevention;
    if (DGETLONG($input,"flagStream",0)) temp.flags |= dlpDBFlagStream;
    temp.miscFlags = 0;
    if (DGETLONG($input,"flagExcludeFromSync",0)) temp.miscFlags |= dlpDBMiscFlagExcludeFromSync;
    $1 = &temp;
}
    
    
// struct CardInfo
%typemap (python,argout) struct CardInfo *OUTPUT {
    PyObject *o;

    if ($1) {
	o = Py_BuildValue("{sisislslslslsssssi}",
			  "card", $1->card,
			  "version", $1->version,
			  "creation", $1->creation,
			  "romSize", $1->romSize,
			  "ramSize", $1->ramSize,
			  "ramFree", $1->ramFree,
			  "name", $1->name,
			  "manufacturer", $1->manufacturer,
			  "more", $1->more);
	$result = t_output_helper($result, o);
    }
}

%typemap (python,in,numinputs=0) struct CardInfo *OUTPUT (struct CardInfo temp) {
    $1 = &temp;
}

%typemap (python,argout) struct NetSyncInfo *OUTPUT {
    PyObject *o;
    if ($1){
	o = Py_BuildValue("{sissssss}",
			  "lanSync", $1->lanSync,
			  "hostName", $1->hostName,
			  "hostAddress", $1->hostAddress,
			  "hostSubnetMask", $1->hostSubnetMask);
	$result = t_output_helper($result, o);
    }
}

%typemap (python,in,numinputs=0) struct NetSyncInfo *OUTPUT (struct NetSyncInfo temp) {
    $1 = &temp;
}

%typemap (python,in) struct NetSyncInfo * {
    static struct NetSyncInfo temp;

    temp.lanSync = (int) DGETLONG($input,"lanSync",0);
    strncpy(temp.hostName, DGETSTR($input,"hostName",""), 256);
    strncpy(temp.hostAddress, DGETSTR($input,"hostAddress",""), 40);
    strncpy(temp.hostSubnetMask, DGETSTR($input,"hostSubnetMask",""), 40);

    $1 = &temp;
}

// a generic 4-character string type, for use as a type or creator ID
%typemap (python,in) unsigned long STR4 {
    if (!($input) || ($input == Py_None)) {
	$1 = 0;
    } else {
	if (!PyString_Check($input) || (PyString_Size($input) != 4)) {
	    PyErr_SetString(PyExc_ValueError, "argument must be a 4-character string");
	    return 0;
	}
	$1 = makelong(PyString_AsString($input));
    }
}

%typemap (python,in) long STR4 {
    if (!($input) || ($input == Py_None)) {
	$1 = 0;
    } else {
	if (!PyString_Check($input) || (PyString_Size($input) != 4)) {
	    PyErr_SetString(PyExc_ValueError, "argument must be a 4-character string");
	    return 0;
	}
	$1 = makelong(PyString_AsString($input));
    }
}

%typemap (python,argout) unsigned long *OUTSTR4 {
    PyObject *o;
    if ($1) {
	o = PyString_FromStringAndSize(printlong(*$1), 4);
	$result = t_output_helper($result, o);
    }
}

%typemap (python,in,numinputs=0) unsigned long *OUTSTR4 (unsigned long temp) {
    $1 = &temp;
}

//

// a char value that allows None for a null value.
%typemap (python,in) char *ALLOWNULL {
    if (!($input) || ($input == Py_None)) {
	$1 = NULL;
    } else {
	$1 = PyString_AsString($input);
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

%typemap (python,out) DLPERROR {
    if ($1 < 0) {
	PyErr_SetObject(Error, Py_BuildValue("(is)", $1,
					     dlp_strerror($1)));
	return NULL;
    }
    $result = Py_None;
    Py_INCREF(Py_None);
}
%typemap (python,out) DLPDBERROR {
    if ($1 == -5) {
	Py_INCREF(Py_None);
	return Py_None;
    } else if ($1 < 0) {
	PyErr_SetObject(Error, Py_BuildValue("(is)", $1,
					     dlp_strerror($1)));
	return NULL;
    }
    $result = Py_None;
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
				pi_buffer_t *OUTBUF);
// note: creator and type are 4-char strings or None, and name is a string or None.
extern DLPERROR dlp_FindDBInfo (int sd, int cardno, int start, const char *ALLOWNULL,
				unsigned long STR4,
				unsigned long STR4, struct DBInfo *OUTPUT);
extern DLPERROR dlp_OpenDB (int sd, int cardno, int mode, char * name, int *OUTPUT);
extern DLPERROR dlp_CloseDB (int sd, int dbhandle);
extern DLPERROR dlp_CloseDB_All (int sd);
extern DLPERROR dlp_DeleteDB (int sd, int cardno, const char * name);
// note: creator and type are 4-char strings.
extern DLPERROR dlp_CreateDB (int sd, unsigned long STR4, unsigned long STR4, 
			      int cardno, int flags, unsigned int version, 
			      const char * name, int *OUTPUT);
extern DLPERROR dlp_ResetSystem (int sd);
extern DLPERROR dlp_AddSyncLogEntry (int sd, char *entry);
extern DLPERROR dlp_OpenConduit (int sd);
extern DLPERROR dlp_EndOfSync (int sd, int status);
extern DLPERROR dlp_AbortSync (int sd);
extern DLPERROR dlp_ReadOpenDBInfo (int sd, int dbhandle, int *OUTPUT);
extern DLPERROR dlp_MoveCategory (int sd, int handle, int fromcat, int tocat);
extern DLPERROR dlp_WriteUserInfo (int sd, struct PilotUser *User);
extern DLPERROR dlp_ReadUserInfo (int sd, struct PilotUser *OUTPUT);
extern DLPERROR dlp_ResetLastSyncPC (int sd);

// XXX
%typemap (python,in) (const void *INBUF, size_t INBUFLEN) {
  $1 = (void *)PyString_AsString($input);
  $2 = PyString_Size($input);
}

// Used by dlp_ReadAppPreference
%typemap (python,argout) (void *OUTBUF, size_t *OUTBUFLEN) {
  PyObject *o;
  if ($1) {
    o = Py_BuildValue("s#", $1, $2);
    $result = t_output_helper($result, o);
  }
}

%typemap (python,in,numinputs=0) (pi_buffer_t *OUTBUF) {
  $1 = pi_buffer_new(0xFFFF);
}

%typemap (python,in,numinputs=0) (size_t *OUTBUFLEN) (size_t outbuflen) {
  outbuflen = 0xFFFF;
  $1 = &outbuflen;
}

%typemap (python,argout) (pi_buffer_t *OUTBUF) {
  PyObject *o1;
  if ($1) {
    o1 = Py_BuildValue("s#", $1->data, $1->used);
    pi_buffer_free($1);
    $result = t_output_helper($result, o1);
  }
}

// If we can't use OUTBUF->used for the size, it can get a bit more messy:

/* %typemap (python,argout) (pi_buffer_t *OUTBUF, size_t *OUTBUFLEN) { */
/*   PyObject *o; */
/*   if ($1) { */
/*     o = Py_BuildValue("s#", $1, $2); */
/*     $result = t_output_helper($result, o); */
/*   } */
/* } */

/* %typemap (python,argout) (pi_buffer_t *OUTBUF,  */
/* 			  int *OUTPUT, size_t *OUTBUFLEN) { */
/*   PyObject *o1, *o2; */
/*   if ($1) { */
/*     o1 = Py_BuildValue("s#", $1->data, $1->used); */
/*     $result = t_output_helper($result, o1); */
/*   } */
/*   o2 = PyInt_FromLong((long) ($2)); */
/*   $result = t_output_helper($result, o2);     */
/* } */

/* %typemap (python,argout) (pi_buffer_t *OUTBUF, unsigned long *OUTSTR4,  */
/* 			  int *OUTPUT, size_t *OUTBUFLEN) { */
/*   PyObject *o1, *o2, *o3; */
/*   if ($1) { */
/*     o1 = Py_BuildValue("s#", $1->data, $1->used); */
/*     $result = t_output_helper($result, o1); */
/*   } */
/*   if ($2) { */
/*     o2 = PyString_FromStringAndSize(printlong(*$2), 4); */
/*     $result = t_output_helper($result, o2); */
/*   }	 */
/*   o3 = PyInt_FromLong((long) ($3)); */
/*   $result = t_output_helper($result, o3);     */
/* } */

/* // can we make this duplication go away using %apply? */
/* %typemap (python,argout) (void *OUTBUF, recordid_t *OUTPUT, int *OUTPUT, size_t *OUTBUFLEN) { */
/*   PyObject *o1, *o2, *o3; */
/*   if ($1) { */
/*     o1 = Py_BuildValue("s#", $1, $4); */
/*     $result = t_output_helper($result, o1); */
/*   } */
/*   if ($2) { */
/*     o2 = PyString_FromStringAndSize(printlong(*$2), 4); */
/*     $result = t_output_helper($result, o2); */
/*   }	 */
/*   o3 = PyInt_FromLong((long) ($3)); */
/*   $result = t_output_helper($result, o3);    */ 
/*} */

%typemap (python,in,numinputs=0) int DLPMAXBUF {
    $1 = DLPMAXBUF;
}

%apply unsigned long { recordid_t };


%native(dlp_ReadAppBlock) PyObject *_wrap_dlp_ReadAppBlock(PyObject *, PyObject *);

extern DLPERROR dlp_WriteAppBlock(int sd, int dbhandle, const void *INBUF, size_t INBUFLEN);

%native(dlp_ReadSortBlock) PyObject *_wrap_dlp_ReadSortBlock(PyObject *, PyObject *);

extern DLPERROR dlp_WriteSortBlock(int sd, int dbhandle, const void *INBUF, size_t INBUFLEN);

extern DLPERROR dlp_ResetDBIndex (int sd, int dbhandle);

%native(dlp_ReadRecordIDList) PyObject *_wrap_dlp_ReadRecordIDList(PyObject *, PyObject *);

extern DLPDBERROR dlp_WriteRecord (int sd, int dbhandle, int flags,
				 recordid_t INPUT, int catID, void *INBUF,
				 size_t INBUFLEN, recordid_t *OUTPUT);
extern DLPERROR dlp_DeleteRecord (int sd, int dbhandle, int all, recordid_t INPUT);
extern DLPERROR dlp_DeleteCategory (int sd, int dbhandle, int category);

extern DLPDBERROR dlp_ReadResourceByType (int sd, int fHandle, unsigned long STR4, int id,
				   pi_buffer_t *OUTBUF, int *OUTPUT);
extern DLPDBERROR dlp_ReadResourceByIndex (int sd, int fHandle, int index, 
					   pi_buffer_t *OUTBUF, unsigned long *OUTSTR4,
					   int *OUTPUT);

extern DLPDBERROR dlp_WriteResource (int sd, int dbhandle, unsigned long STR4, int id,
				   const void *INBUF, size_t INBUFLEN);
extern DLPERROR dlp_DeleteResource (int sd, int dbhandle, int all, unsigned long STR4,
				    int resID);

extern DLPDBERROR dlp_ReadNextModifiedRec (int sd, int fHandle, pi_buffer_t *OUTBUF,
					 recordid_t *OUTPUT, int *OUTPUT,
					 int *OUTPUT, int *OUTPUT);
extern DLPDBERROR dlp_ReadNextModifiedRecInCategory (int sd, int fHandle, int incategory,
						     pi_buffer_t *OUTBUF, 
						     recordid_t *OUTPUT,
						     int *OUTPUT, int *OUTPUT);
extern DLPDBERROR dlp_ReadNextRecInCategory (int sd, int fHandle, int incategory,
					   pi_buffer_t *OUTBUF,
					   recordid_t *OUTPUT, int *OUTPUT,
					     int *OUTPUT);
extern DLPDBERROR dlp_ReadRecordById (int sd, int fHandle, recordid_t INPUT, 
				      pi_buffer_t *OUTBUF, int *OUTPUT, int *OUTPUT, 
				      int *OUTPUT);
extern DLPDBERROR dlp_ReadRecordByIndex (int sd, int fHandle, int index, 
					 pi_buffer_t *OUTBUF, 
					 recordid_t *OUTPUT, 
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
				       int DLPMAXBUF, void *OUTBUF, size_t *OUTBUFLEN, 
				       int *OUTPUT);
extern DLPERROR dlp_WriteAppPreference (int sd, unsigned long STR4, int id, int backup,
					int version, void *INBUF, size_t INBUFLEN);
// and the most complex of all... i'm not even sure how it works.
//extern int dlp_RPC (int sd, struct RPC_params * p, unsigned long *OUTPUT);

// for functions that return 0 on success or something else on error
%typemap (python,out) PIERROR {
    int *res_pointer, res;
    res_pointer = (int *) $1;
    res = *res_pointer;
    if (res != 0) {
	PyErr_SetObject(Error, Py_BuildValue("(is)", res,
					     "pisock error"));
	return NULL;
    }
    $result = Py_None;
    Py_INCREF(Py_None);
}

// XXX
//%typemap (python,argout) void **OUTBUF {
//    PyObject *o;
//    if ($1) {
//	o = Py_BuildValue("s#", *($1), __buflen);
//	$result = t_output_helper($result, o);
//    }
//}
//%typemap (python,in,numinputs=0) void **OUTBUF {
//    static void *foo;
//    $1 = &foo;
//}


/* pi-file */

extern struct pi_file *pi_file_open (const char *name);
extern PIERROR pi_file_close (struct pi_file *pf);
extern PIERROR pi_file_get_info (struct pi_file *pf, struct DBInfo *OUTPUT);
extern PIERROR pi_file_get_app_info  (struct pi_file *pf, void **OUTBUF,
					      size_t *OUTBUFLEN);
extern PIERROR pi_file_get_sort_info (struct pi_file *pf, void **OUTBUF,
					      size_t *OUTBUFLEN);
extern PIERROR pi_file_read_resource (struct pi_file *pf, int idx, void **OUTBUF,
				  size_t *OUTBUFLEN, unsigned long *OUTSTR4,
				  int *OUTPUT);
extern PIERROR pi_file_read_resource_by_type_id (struct pi_file *pf,
					     unsigned long STR4, int id,
					     void **OUTBUF, size_t *OUTBUFLEN, int *OUTPUT);
extern PIERROR pi_file_type_id_used (struct pi_file *pf, unsigned long STR4, int id);
extern PIERROR pi_file_read_record (struct pi_file *pf, int idx, void **OUTBUF, size_t *OUTBUFLEN,
				    int *OUTPUT, int *OUTPUT, recordid_t *OUTPUT);
extern PIERROR pi_file_get_entries (struct pi_file *pf, int *OUTPUT);
extern PIERROR pi_file_read_record_by_id (struct pi_file *pf, recordid_t INPUT, void **OUTBUF,
					  size_t *OUTBUFLEN, int *OUTPUT, int *OUTPUT,
					  int *OUTPUT);
extern PIERROR pi_file_id_used (struct pi_file *pf, recordid_t INPUT);
extern struct pi_file *pi_file_create (char *name, struct DBInfo *INPUT);
extern PIERROR pi_file_set_info (struct pi_file *pf, struct DBInfo *INPUT);
extern PIERROR pi_file_set_app_info (struct pi_file *pf, void *INBUF, size_t INBUFLEN);
extern PIERROR pi_file_set_sort_info (struct pi_file *pf, void *INBUF, size_t INBUFLEN);
extern PIERROR pi_file_append_resource (struct pi_file *pf, void *INBUF, size_t INBUFLEN,
					unsigned long STR4, int id);
extern PIERROR pi_file_append_record (struct pi_file *pf, void *INBUF, size_t INBUFLEN,
				      int attr, int category, recordid_t INPUT);
extern PIERROR pi_file_retrieve (struct pi_file *pf, int socket, int cardno);
extern PIERROR pi_file_install (struct pi_file *pf, int socket, int cardno);
extern PIERROR pi_file_merge (struct pi_file *pf, int socket, int cardno);

// pi-inet / pi-inetserial
// pi-padp
// pi-serial
// pi-slp
// pi-sync

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
