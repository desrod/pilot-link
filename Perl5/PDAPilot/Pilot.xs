#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "../../include/pi-macros.h"
#include "../../include/pi-file.h"
#include "../../include/pi-datebook.h"
#include "../../include/pi-memo.h"
#include "../../include/pi-address.h"
#include "../../include/pi-todo.h"
#include "../../include/pi-mail.h"
#include "../../include/pi-socket.h"
#include "../../include/pi-dlp.h"
#include "../../include/pi-syspkt.h"

static int
not_here(s)
char *s;
{
    croak("%s not implemented on this architecture", s);
    return -1;
}

static double
constant(name, arg)
char *name;
int arg;
{
    errno = 0;
    switch (*name) {
    case 'A':
	break;
    case 'B':
	break;
    case 'C':
	break;
    case 'D':
	break;
    case 'E':
	break;
    case 'F':
	break;
    case 'G':
	break;
    case 'H':
	break;
    case 'I':
	break;
    case 'J':
	break;
    case 'K':
	break;
    case 'L':
	break;
    case 'M':
	break;
    case 'N':
	break;
    case 'O':
	break;
    case 'P':
	if (strEQ(name, "PI_AF_SLP"))
#ifdef PI_AF_SLP
	    return PI_AF_SLP;
#else
	    goto not_there;
#endif
	if (strEQ(name, "PI_PF_LOOP"))
#ifdef PI_PF_LOOP
	    return PI_PF_LOOP;
#else
	    goto not_there;
#endif
	if (strEQ(name, "PI_PF_PADP"))
#ifdef PI_PF_PADP
	    return PI_PF_PADP;
#else
	    goto not_there;
#endif
	if (strEQ(name, "PI_PF_SLP"))
#ifdef PI_PF_SLP
	    return PI_PF_SLP;
#else
	    goto not_there;
#endif
	if (strEQ(name, "PI_PilotSocketConsole"))
#ifdef PI_PilotSocketConsole
	    return PI_PilotSocketConsole;
#else
	    goto not_there;
#endif
	if (strEQ(name, "PI_PilotSocketDLP"))
#ifdef PI_PilotSocketDLP
	    return PI_PilotSocketDLP;
#else
	    goto not_there;
#endif
	if (strEQ(name, "PI_PilotSocketDebugger"))
#ifdef PI_PilotSocketDebugger
	    return PI_PilotSocketDebugger;
#else
	    goto not_there;
#endif
	if (strEQ(name, "PI_PilotSocketRemoteUI"))
#ifdef PI_PilotSocketRemoteUI
	    return PI_PilotSocketRemoteUI;
#else
	    goto not_there;
#endif
	if (strEQ(name, "PI_SOCK_DGRAM"))
#ifdef PI_SOCK_DGRAM
	    return PI_SOCK_DGRAM;
#else
	    goto not_there;
#endif
	if (strEQ(name, "PI_SOCK_RAW"))
#ifdef PI_SOCK_RAW
	    return PI_SOCK_RAW;
#else
	    goto not_there;
#endif
	if (strEQ(name, "PI_SOCK_SEQPACKET"))
#ifdef PI_SOCK_SEQPACKET
	    return PI_SOCK_SEQPACKET;
#else
	    goto not_there;
#endif
	if (strEQ(name, "PI_SOCK_STREAM"))
#ifdef PI_SOCK_STREAM
	    return PI_SOCK_STREAM;
#else
	    goto not_there;
#endif
	break;
    case 'Q':
	break;
    case 'R':
	break;
    case 'S':
	break;
    case 'T':
	break;
    case 'U':
	break;
    case 'V':
	break;
    case 'W':
	break;
    case 'X':
	break;
    case 'Y':
	break;
    case 'Z':
	break;
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

static char mybuf[0xffff];

static AV * tmtoav (struct tm * t) {
	AV * ret = newAV();
	
	av_push(ret, newSViv(t->tm_sec));
	av_push(ret, newSViv(t->tm_min));
	av_push(ret, newSViv(t->tm_hour));
	av_push(ret, newSViv(t->tm_mday));
	av_push(ret, newSViv(t->tm_mon));
	av_push(ret, newSViv(t->tm_year));
	av_push(ret, newSViv(t->tm_wday));
	av_push(ret, newSViv(t->tm_yday));
	av_push(ret, newSViv(t->tm_isdst));
	
	return ret;
}

struct tm * avtotm (AV * av, struct tm * t) {
	SV ** s;
	t->tm_sec = (s = av_fetch(av, 0, 0)) ? SvIV(*s) : 0;
	t->tm_min = (s = av_fetch(av, 1, 0)) ? SvIV(*s) : 0;
	t->tm_hour = (s = av_fetch(av, 2, 0)) ? SvIV(*s) : 0;
	t->tm_mday = (s = av_fetch(av, 3, 0)) ? SvIV(*s) : 0;
	t->tm_mon = (s = av_fetch(av, 4, 0)) ? SvIV(*s) : 0;
	t->tm_year = (s = av_fetch(av, 5, 0)) ? SvIV(*s) : 0;
	t->tm_wday= (s = av_fetch(av, 6, 0)) ? SvIV(*s) : 0;
	t->tm_yday= (s = av_fetch(av, 7, 0)) ? SvIV(*s) : 0;
	t->tm_isdst = (s = av_fetch(av, 8, 0)) ? SvIV(*s) : 0;
	
	return t;
}

#ifndef newRV_noinc   
static SV * rv;
#define newRV_noinc(s) ((rv=newRV(s)), SvREFCNT_dec(s), rv)
#endif

extern char * printlong (unsigned long val);
extern unsigned long makelong (char * c);

typedef struct {
	int errno;
	struct pi_file * pf;
} PDA__Pilot__File;
typedef struct DLP {
	int errno;
	int socket;
} PDA__Pilot__DLP;
typedef struct DLPDB {
	SV *	connection;
	int	socket;
	int	handle;
	int errno;
} PDA__Pilot__DLP__DB;

typedef struct DBInfo DBInfo;
typedef struct PilotUser UserInfo;
typedef unsigned long Char4;
typedef int Result;

SV *
newSVChar4(arg)
	unsigned long arg;
{
	char * c = printlong(arg);
	if(	(isalpha(c[0]) || (c[0] == ' ')) &&
		(isalpha(c[1]) || (c[1] == ' ')) &&
		(isalpha(c[2]) || (c[2] == ' ')) &&
		(isalpha(c[3]) || (c[3] == ' ')))
		return newSVpv(c,4);
	else
		return newSViv(arg);
}

unsigned long
SvChar4(arg)
	SV * arg;
{
	if (SvIOKp(arg))
		return SvIV(arg);
	else {
		int len;
		char * c = SvPV(arg, len);
		if (len != 4)
			croak("Char4 argument a string that isn't four bytes long");
		return makelong(c);
	}
}

#define pack_dbinfo(arg, var, failure)	\
	{	\
		if (failure < 0)  {	\
		   arg = &sv_undef;	\
		   self->errno = failure;\
		} else {	\
			HV * i = newHV();	\
			hv_store(i, "more", 4, newSViv(var.more), 0);	\
	    	hv_store(i, "flags", 5, newSViv(var.flags), 0);	\
	    	hv_store(i, "miscflags", 9, newSViv(var.miscflags), 0);	\
	    	hv_store(i, "type", 4, newSVChar4(var.type), 0);	\
	    	hv_store(i, "creator", 7, newSVChar4(var.creator), 0);	\
	    	hv_store(i, "version", 7, newSViv(var.version), 0);	\
	    	hv_store(i, "modnum", 6, newSViv(var.modnum), 0);	\
	    	hv_store(i, "index", 5, newSViv(var.index), 0);	\
	    	hv_store(i, "crdate", 6, newSViv(var.crdate), 0);	\
	    	hv_store(i, "moddate", 7, newSViv(var.moddate), 0);	\
	    	hv_store(i, "backupdate", 10, newSViv(var.backupdate), 0);	\
	    	hv_store(i, "name", 4, newSVpv(var.name, 0), 0);	\
			arg = newRV((SV*)i);	\
		}	\
	}

#define unpack_dbinfo(arg, var)	\
	if ((SvTYPE(arg) == SVt_RV) && (SvTYPE(SvRV(arg))==SVt_PVHV)) {	\
	    HV * i = (HV*)SvRV(arg);	\
	    SV ** s;	\
	    var.more = (s = hv_fetch(i, "more", 4, 0)) ? SvIV(*s) : 0;	\
	    var.flags = (s = hv_fetch(i, "flags", 5, 0)) ? SvIV(*s) : 0;	\
	    var.miscflags = (s = hv_fetch(i, "miscflags", 9, 0)) ? SvIV(*s) : 0;	\
	    var.type = (s = hv_fetch(i, "type", 4, 0)) ? SvChar4(*s) : 0;	\
	    var.creator = (s = hv_fetch(i, "creator", 7, 0)) ? SvChar4(*s) : 0;	\
	    var.version = (s = hv_fetch(i, "version", 7, 0)) ? SvIV(*s) : 0;	\
	    var.modnum = (s = hv_fetch(i, "modnum", 6, 0)) ? SvIV(*s) : 0;	\
	    var.index = (s = hv_fetch(i, "index", 5, 0)) ? SvIV(*s) : 0;	\
	    var.crdate = (s = hv_fetch(i, "crdate", 6, 0)) ? SvIV(*s) : 0;	\
	    var.moddate = (s = hv_fetch(i, "moddate", 7, 0)) ? SvIV(*s) : 0;	\
	    var.backupdate = (s = hv_fetch(i, "backupdate", 10, 0)) ? SvIV(*s) : 0;	\
	    if ((s = hv_fetch(i, "name", 4, 0)) ? SvPV(*s,na) : 0)	\
	    	strcpy(var.name, SvPV(*s, na));	\
	} else	{\
		croak("argument is not a hash reference"); \
	}

#define pack_userinfo(arg, var, failure)	\
	{	\
		if (failure < 0)  {	\
		   arg = &sv_undef;	\
		   self->errno = failure;\
		} else {	\
			HV * i = newHV();	\
			hv_store(i, "userID", 6, newSViv(var.userID), 0);	\
	    	hv_store(i, "viewerID", 8, newSViv(var.viewerID), 0);	\
	    	hv_store(i, "lastSyncPC", 10, newSViv(var.lastSyncPC), 0);	\
	    	hv_store(i, "lastGoodSync", 12, newSViv(var.succSyncDate), 0);	\
	    	hv_store(i, "lastSync", 8, newSViv(var.lastSyncDate), 0);	\
	    	hv_store(i, "name", 4, newSVpv(var.username,0), 0);	\
	    	hv_store(i, "password", 8, newSVpv(var.password,var.passwordLen), 0);	\
			arg = newRV((SV*)i);	\
		}	\
	}

#define unpack_userinfo(arg, var)	\
	if ((SvTYPE(arg) == SVt_RV) && (SvTYPE(SvRV(arg))==SVt_PVHV)) {	\
	    HV * i = (HV*)SvRV(arg);	\
	    SV ** s;	\
	    var.userID = (s = hv_fetch(i, "userID", 6, 0)) ? SvIV(*s) : 0;	\
	    var.viewerID = (s = hv_fetch(i, "viewerID", 8, 0)) ? SvIV(*s) : 0;	\
	    var.lastSyncPC = (s = hv_fetch(i, "lastSyncPC", 10, 0)) ? SvIV(*s) : 0;	\
	    var.lastSyncDate = (s = hv_fetch(i, "lastSync", 8, 0)) ? SvIV(*s) : 0;	\
	    var.succSyncDate = (s = hv_fetch(i, "lastGoodSync", 12, 0)) ? SvIV(*s) : 0;	\
	    if ((s = hv_fetch(i, "name", 4, 0)) ? SvPV(*s,na) : 0)	\
	    	strcpy(var.username, SvPV(*s, na));	\
	} else	{\
		croak("argument is not a hash reference"); \
	}

#define ReturnReadRecord(buf,size) \
	    if (result >=0) {	\
		    EXTEND(sp, 5);	\
	    	PUSHs(sv_2mortal(newSVpv(buf, size)));	\
		    if (GIMME != G_SCALAR) {	\
		    	PUSHs(sv_2mortal(newSViv(index)));	\
		    	PUSHs(sv_2mortal(newSViv(id)));	\
		    	PUSHs(sv_2mortal(newSViv(attr)));	\
		    	PUSHs(sv_2mortal(newSViv(category)));	\
		    }	\
		} else {	\
	    	PUSHs(&sv_undef);	\
	    	self->errno = result;\
	    }

#define ReturnReadResource(buf,size) \
	    if (result >=0) {	\
		    EXTEND(sp, 4);	\
	    	PUSHs(sv_2mortal(newSVpv(buf, size)));	\
		    if (GIMME != G_SCALAR) {	\
		    	PUSHs(sv_2mortal(newSViv(index)));	\
		    	PUSHs(sv_2mortal(newSVChar4(type)));	\
		    	PUSHs(sv_2mortal(newSViv(id)));	\
		    }	\
		} else {	\
	    	PUSHs(&sv_undef);	\
	    	self->errno = result;\
	    }

MODULE = PDA::Pilot		PACKAGE = PDA::Pilot

double
constant(name,arg)
	char *		name
	int		arg

MODULE = PDA::Pilot		PACKAGE = PDA::Pilot::Appointment

SV *
Unpack(record)
    SV * record
    CODE:
    {
    int len;
    int i;
    AV * e;
    HV * ret;
    struct Appointment a;
    (void)SvPV(record, len);
    unpack_Appointment(&a, SvPV(record, len), len);
    
    ret = newHV();
    hv_store(ret, "event", 5, newSViv(a.event), 0);
    hv_store(ret, "begin", 5, newRV_noinc((SV*)tmtoav(&a.begin)), 0);
    
    if (!a.event) {
	    hv_store(ret, "end", 3, newRV_noinc((SV*)tmtoav(&a.end)), 0);
    }
    
    if (a.alarm) {
        HV * alarm = newHV();
        hv_store(ret, "alarm", 5, newRV_noinc((SV*)alarm), 0);
        
   		hv_store(alarm, "advance", 7, newSViv(a.advance), 0);
	    hv_store(alarm, "units", 5, newSViv(
	    	(a.advanceUnits == 0) ? (60) :        /* Minutes */
	    	(a.advanceUnits == 1) ? (60*60) :     /* Hours */
	    	(a.advanceUnits == 2) ? (60*60*24) :  /* Days */
	    	0), 0);
    }
    if (a.repeatType) {
        HV * repeat = newHV();
        hv_store(ret, "repeat", 6, newRV_noinc((SV*)repeat), 0);
        
   		hv_store(repeat, "type", 4, newSViv(a.repeatType), 0);
   		hv_store(repeat, "forever", 7, newSViv(a.repeatForever), 0);
   		hv_store(repeat, "frequency", 9, newSViv(a.repeatFreq), 0);
   		hv_store(repeat, "on", 2, newSViv(a.repeatOn), 0);
   		hv_store(repeat, "weekstart", 9, newSViv(a.repeatWeekstart), 0);
   		hv_store(repeat, "end", 3, newRV_noinc((SV*)tmtoav(&a.repeatEnd)),0);
    }
    
    if (a.exceptions) {
	    e = newAV();
	    hv_store(ret, "exceptions", 10, newRV_noinc((SV*)e), 0);
	    for (i=0;i<a.exceptions;i++) {
	    	av_push(e,newRV_noinc((SV*)tmtoav(&a.exception[i])));
	    }
	}
    
    if (a.description)
      hv_store(ret, "description", 11, newSVpv((char*)a.description,0), 0);

    if (a.note)
      hv_store(ret, "note", 4, newSVpv((char*)a.note,0), 0);
    
    free_Appointment(&a);
    
    RETVAL = newRV_noinc((SV*)ret);
    }
    OUTPUT:
    RETVAL

SV *
Pack(record)
    SV * record
    CODE:
    {
    int len;
    SV ** s;
    HV * h;
    long advance;
    struct Appointment a;
    
    if (!SvRV(record) || (SvTYPE(h=(HV*)SvRV(record))!=SVt_PVHV))
    	croak("record is not a hash reference");

    a.event = (s = hv_fetch(h, "event", 5, 0)) ? SvIV(*s) : 0;
    if (s= hv_fetch(h, "begin", 5, 0)) 
    	avtotm((AV*)SvRV(*s), &a.begin);
    else
    	memset(&a.begin, '\0', sizeof(struct tm));
    if (s= hv_fetch(h, "end", 3, 0)) 
    	avtotm((AV*)SvRV(*s), &a.end);
    else
    	memset(&a.end, '\0', sizeof(struct tm));

	if ((s = hv_fetch(h, "alarm", 5, 0)) && SvRV(*s) && (SvTYPE(SvRV(*s))==SVt_PVHV)) {
		HV * h2 = (HV*)SvRV(*s);
	    a.advance = (s = hv_fetch(h2, "advance", 7, 0)) ? SvIV(*s) : 0;
	    a.advanceUnits = (s = hv_fetch(h2, "units", 5, 0)) ? SvIV(*s) : 0;
	    a.alarm = 1;
    } else {
    	a.alarm = 0;
    	a.advance = 0;
    	a.advanceUnits = 0;
    }    	

	if ((s = hv_fetch(h, "repeat", 6, 0)) && SvRV(*s) && (SvTYPE(SvRV(*s))==SVt_PVHV)) {
		HV * h2 = (HV*)SvRV(*s);
	    a.repeatType = (s = hv_fetch(h2, "type", 4, 0)) ? SvIV(*s) : 0;
	    a.repeatForever = (s = hv_fetch(h2, "forever", 7, 0)) ? SvIV(*s) : 0;
	    a.repeatFreq = (s = hv_fetch(h2, "frequency", 9, 0)) ? SvIV(*s) : 0;
	    a.repeatOn = (s = hv_fetch(h2, "on", 2, 0)) ? SvIV(*s) : 0;
	    a.repeatWeekstart = (s = hv_fetch(h2, "weekstart", 9, 0)) ? SvIV(*s) : 0;
	    if (s = hv_fetch(h2, "end", 3, 0)) 
	    	avtotm((AV*)SvRV(*s), &a.repeatEnd);
    } else {
    	a.repeatType = 0;
    	a.repeatForever = 0;
    	a.repeatFreq = 0;
    	a.repeatOn = 0;
    	a.repeatWeekstart = 0;
    	memset(&a.repeatEnd,'\0', sizeof(struct tm));
    }    	

	if ((s = hv_fetch(h, "exceptions", 10, 0)) && SvRV(*s) && (SvTYPE(SvRV(*s))==SVt_PVAV)) {
		int i;
		AV * a2 = (AV*)SvRV(*s);
	    a.exceptions = av_len(a2);
	    a.exception = malloc(sizeof(struct tm)*a.exceptions);
	    for (i=0;i<a.exceptions;i++)
	    	if ((s = av_fetch(a2, i, 0)))
	    		avtotm((AV*)SvRV(*s), a.exception+i);
    } else {
    	a.exceptions = 0;
    	a.exception = 0;
    }    	

    a.description = (s = hv_fetch(h, "description", 11, 0)) ? SvPV(*s,na) : 0;
    a.note = (s = hv_fetch(h, "note", 4, 0)) ? SvPV(*s,na) : 0;

    pack_Appointment(&a, mybuf, &len);
    
    if (a.exception)
		free(a.exception);
    
    RETVAL = newSVpv(mybuf, len);
    }
    OUTPUT:
    RETVAL


SV *
UnpackAppBlock(record)
    SV * record
    CODE:
    {
    int len;
    AV * e;
    HV * ret;
    int i;
    struct AppointmentAppInfo a;
    SvPV(record, len);
    unpack_AppointmentAppInfo(&a, SvPV(record, len), len);

    ret = newHV();
    
    hv_store(ret, "renamedCategories",17 , newSViv(a.renamedcategories), 0);
    
    e = newAV();
    hv_store(ret, "categoryName", 12, newRV_noinc((SV*)e), 0);
    
    for (i=0;i<16;i++) {
    	av_push(e, newSVpv(a.CategoryName[i], 0));
    }

    e = newAV();
    hv_store(ret, "categoryID", 10, newRV_noinc((SV*)e), 0);
    
    for (i=0;i<16;i++) {
    	av_push(e, newSViv(a.CategoryID[i]));
    }

    hv_store(ret, "lastUniqueID", 12, newSViv(a.lastUniqueID), 0);

    hv_store(ret, "startOfWeek", 11, newSViv(a.startOfWeek), 0);

    RETVAL = newRV_noinc((SV*)ret);

    }
    OUTPUT:
    RETVAL

SV *
PackAppBlock(record)
    SV * record
    CODE:
    {
    int i;
    int len;
    SV ** s;
    HV * h;
    AV * av;
    struct AppointmentAppInfo a;
    
    if (!SvRV(record) || (SvTYPE(h=(HV*)SvRV(record))!=SVt_PVHV))
    	croak("record is not a hash reference");
    
    if ((s = hv_fetch(h, "renamedCategories", 17, 0)))
	    a.renamedcategories = SvIV(*s);
	else
		a.renamedcategories = 0;

    if ((s = hv_fetch(h, "categoryName", 12, 0)) && SvRV(*s) && (SvTYPE(av=(AV*)SvRV(*s))==SVt_PVAV))
    	for (i=0;i<16;i++)
    		strncpy(a.CategoryName[i], (s=av_fetch(av, i, 0)) ? SvPV(*s,na) : "", 16);
	else
		for (i=0;i<16;i++)
			strcpy(a.CategoryName[i], "");

	for (i=0;i<16;i++)
		a.CategoryName[i][15] = '\0';

    if ((s = hv_fetch(h, "categoryID", 10, 0)) && SvRV(*s) && (SvTYPE(av=(AV*)SvRV(*s))==SVt_PVAV))
    	for (i=0;i<16;i++)
    		a.CategoryID[i] = (s=av_fetch(av, i, 0)) ? SvIV(*s) : 0;
	else
		for (i=0;i<16;i++)
			a.CategoryID[i] = 0;

    if ((s = hv_fetch(h, "lastUniqueID", 12, 0)))
	    a.lastUniqueID = SvIV(*s);
	else
		a.lastUniqueID = 0;
		
    if ((s = hv_fetch(h, "startOfWeek", 11, 0)))
	    a.startOfWeek = SvIV(*s);
	else
		a.startOfWeek = 0;

    pack_AppointmentAppInfo(&a, mybuf, &len);

    RETVAL = newSVpv(mybuf, len);
    }
    OUTPUT:
    RETVAL

MODULE = PDA::Pilot		PACKAGE = PDA::Pilot::ToDo

SV *
Unpack(record)
    SV * record
    CODE:
    {
    int len;
    int i;
    AV * e;
    HV * ret;
    struct ToDo a;
    (void)SvPV(record, len);
    unpack_ToDo(&a, SvPV(record, len), len);
    
    ret = newHV();
    hv_store(ret, "indefinite", 10, newSViv(a.indefinite), 0);
    hv_store(ret, "due", 3, newRV_noinc((SV*)tmtoav(&a.due)), 0);  
    hv_store(ret, "priority", 8, newSViv(a.priority), 0);
    hv_store(ret, "complete", 8, newSViv(a.complete), 0);
    if (a.description)
      hv_store(ret, "description", 11, newSVpv((char*)a.description,0), 0);
    if (a.note)
      hv_store(ret, "note", 4, newSVpv((char*)a.note,0), 0);
    
    free_ToDo(&a);
    
    RETVAL = newRV_noinc((SV*)ret);
    }
    OUTPUT:
    RETVAL

SV *
Pack(record)
    SV * record
    CODE:
    {
    int len;
    SV ** s;
    HV * h;
    struct ToDo a;

    if (!SvRV(record) || (SvTYPE(h=(HV*)SvRV(record))!=SVt_PVHV))
    	croak("record is not a hash reference");

    a.indefinite = (s = hv_fetch(h, "indefinite", 10, 0)) ? SvIV(*s) : 0;
    a.priority = (s = hv_fetch(h, "priority", 8, 0)) ? SvIV(*s) : 0;
    a.complete = (s = hv_fetch(h, "complete", 8, 0)) ? SvIV(*s) : 0;
    if (!a.indefinite && (s = hv_fetch(h, "due", 3, 0))) 
    	avtotm((AV*)SvRV(*s), &a.due);
    else
    	memset(&a.due,'\0', sizeof(struct tm));
    
    a.description = (s = hv_fetch(h, "description", 11, 0)) ? SvPV(*s,na) : 0;
    a.note = (s = hv_fetch(h, "note", 4, 0)) ? SvPV(*s,na) : 0;

    pack_ToDo(&a, mybuf, &len);
    
    RETVAL = newSVpv(mybuf, len);
    }
    OUTPUT:
    RETVAL


SV *
UnpackAppBlock(record)
    SV * record
    CODE:
    {
    int len;
    AV * e;
    HV * ret;
    int i;
    struct ToDoAppInfo a;
    SvPV(record, len);
    unpack_ToDoAppInfo(&a, SvPV(record, len), len);

    ret = newHV();
    
    hv_store(ret, "renamedCategories",17 , newSViv(a.renamedcategories), 0);
    
    e = newAV();
    hv_store(ret, "categoryName", 12, newRV_noinc((SV*)e), 0);
    
    for (i=0;i<16;i++) {
    	av_push(e, newSVpv(a.CategoryName[i], 0));
    }

    e = newAV();
    hv_store(ret, "categoryID", 10, newRV_noinc((SV*)e), 0);
    
    for (i=0;i<16;i++) {
    	av_push(e, newSViv(a.CategoryID[i]));
    }

    hv_store(ret, "lastUniqueID", 12, newSViv(a.lastUniqueID), 0);

    hv_store(ret, "dirty", 5, newSViv(a.dirty), 0);

    hv_store(ret, "sortByPriority", 14, newSViv(a.sortByPriority), 0);

    RETVAL = newRV_noinc((SV*)ret);

    }
    OUTPUT:
    RETVAL

SV *
PackAppBlock(record)
    SV * record
    CODE:
    {
    int i;
    int len;
    SV ** s;
    HV * h;
    AV * av;
    struct ToDoAppInfo a;
    
    if (!SvRV(record) || (SvTYPE(h=(HV*)SvRV(record))!=SVt_PVHV))
    	croak("record is not a hash reference");
    
    if ((s = hv_fetch(h, "renamedCategories", 17, 0)))
	    a.renamedcategories = SvIV(*s);
	else
		a.renamedcategories = 0;

    if ((s = hv_fetch(h, "categoryName", 12, 0)) && SvRV(*s) && (SvTYPE(av=(AV*)SvRV(*s))==SVt_PVAV))
    	for (i=0;i<16;i++)
    		strncpy(a.CategoryName[i], (s=av_fetch(av, i, 0)) ? SvPV(*s,na) : "", 16);
	else
		for (i=0;i<16;i++)
			strcpy(a.CategoryName[i], "");

	for (i=0;i<16;i++)
		a.CategoryName[i][15] = '\0';

    if ((s = hv_fetch(h, "categoryID", 10, 0)) && SvRV(*s) && (SvTYPE(av=(AV*)SvRV(*s))==SVt_PVAV))
    	for (i=0;i<16;i++)
    		a.CategoryID[i] = (s=av_fetch(av, i, 0)) ? SvIV(*s) : 0;
	else
		for (i=0;i<16;i++)
			a.CategoryID[i] = 0;

    if ((s = hv_fetch(h, "lastUniqueID", 12, 0)))
	    a.lastUniqueID = SvIV(*s);
	else
		a.lastUniqueID = 0;

    a.dirty = (s = hv_fetch(h, "dirty", 5, 0)) ? SvIV(*s) : 0;
    a.sortByPriority = (s = hv_fetch(h, "sortByPriority", 14, 0)) ? SvIV(*s) : 0;

    pack_ToDoAppInfo(&a, mybuf, &len);

    RETVAL = newSVpv(mybuf, len);
    }
    OUTPUT:
    RETVAL

MODULE = PDA::Pilot		PACKAGE = PDA::Pilot::Address

SV *
Unpack(record)
    SV * record
    CODE:
    {
    int len;
    int i;
    AV * e;
    HV * ret;
    struct Address a;
    (void)SvPV(record, len);
    unpack_Address(&a, SvPV(record, len), len);

    ret = newHV();

    e = newAV();
    hv_store(ret, "phoneLabel", 10, newRV_noinc((SV*)e), 0);
    
    for (i=0;i<5;i++) {
    	av_push(e, newSViv(a.phonelabel[i]));
    }

    e = newAV();
    hv_store(ret, "entry", 5, newRV_noinc((SV*)e), 0);

    for (i=0;i<19;i++) {
    	av_push(e, a.entry[i] ? newSVpv(a.entry[i],0) : &sv_undef);
    }
    
    free_Address(&a);
    
    RETVAL = newRV_noinc((SV*)ret);
    }
    OUTPUT:
    RETVAL

SV *
Pack(record)
    SV * record
    CODE:
    {
    int len;
    SV ** s;
    HV * h;
    AV * av;
    int i;
    struct Address a;

    if (!SvRV(record) || (SvTYPE(h=(HV*)SvRV(record))!=SVt_PVHV))
    	croak("record is not a hash reference");

    if ((s = hv_fetch(h, "phoneLabel", 10, 0)) && SvRV(*s) && (SvTYPE(av=(AV*)SvRV(*s))==SVt_PVAV))
    	for (i=0;i<5;i++)
    		a.phonelabel[i] = ((s=av_fetch(av, i, 0)) && SvOK(*s)) ? SvIV(*s) : 0;
	else
		for (i=0;i<5;i++)
			a.phonelabel[i] = 0;

    if ((s = hv_fetch(h, "entry", 5, 0)) && SvRV(*s) && (SvTYPE(av=(AV*)SvRV(*s))==SVt_PVAV))
    	for (i=0;i<19;i++)
    		a.entry[i] = ((s=av_fetch(av, i, 0)) && SvOK(*s)) ? SvPV(*s,na) : 0;
	else
		for (i=0;i<19;i++)
			a.entry[i] = 0;

    pack_Address(&a, mybuf, &len);
    
    RETVAL = newSVpv(mybuf, len);
    }
    OUTPUT:
    RETVAL


SV *
UnpackAppBlock(record)
    SV * record
    CODE:
    {
    int len;
    AV * e;
    HV * ret;
    int i;
    struct AddressAppInfo a;
    SvPV(record, len);
    unpack_AddressAppInfo(&a, SvPV(record, len), len);
    
    ret = newHV();
    
    hv_store(ret, "renamedCategories",17 , newSViv(a.renamedcategories), 0);
    
    e = newAV();
    hv_store(ret, "categoryName", 12, newRV_noinc((SV*)e), 0);
    
    for (i=0;i<16;i++) {
    	av_push(e, newSVpv(a.CategoryName[i], 0));
    }

    e = newAV();
    hv_store(ret, "categoryID", 10, newRV_noinc((SV*)e), 0);
    
    for (i=0;i<16;i++) {
    	av_push(e, newSViv(a.CategoryID[i]));
    }

    hv_store(ret, "lastUniqueID", 12, newSViv(a.lastUniqueID), 0);

    hv_store(ret, "dirtyfieldlabels", 16, newSViv(a.dirtyfieldlabels), 0);

    hv_store(ret, "country", 7, newSViv(a.country), 0);
    hv_store(ret, "sortByCompany", 13, newSViv(a.sortByCompany), 0);

    e = newAV();
    hv_store(ret, "label", 5, newRV_noinc((SV*)e), 0);
    
    for (i=0;i<22;i++) {
    	av_push(e, newSVpv(a.labels[i],0));
    }

    e = newAV();
    hv_store(ret, "phoneLabel", 10, newRV_noinc((SV*)e), 0);
    
    for (i=0;i<8;i++) {
    	av_push(e, newSVpv(a.phonelabels[i],0));
    }

    RETVAL = newRV_noinc((SV*)ret);

    }
    OUTPUT:
    RETVAL

SV *
PackAppBlock(record)
    SV * record
    CODE:
    {
    int i;
    int len;
    SV ** s;
    HV * h;
    AV * av;
    struct AddressAppInfo a;
    
    if (!SvRV(record) || (SvTYPE(h=(HV*)SvRV(record))!=SVt_PVHV))
    	croak("record is not a hash reference");
    
    if ((s = hv_fetch(h, "renamedCategories", 17, 0)))
	    a.renamedcategories = SvIV(*s);
	else
		a.renamedcategories = 0;

    if ((s = hv_fetch(h, "categoryName", 12, 0)) && SvRV(*s) && (SvTYPE(av=(AV*)SvRV(*s))==SVt_PVAV))
    	for (i=0;i<16;i++)
    		strncpy(a.CategoryName[i], (s=av_fetch(av, i, 0)) ? SvPV(*s,na) : "", 16);
	else
		for (i=0;i<16;i++)
			strcpy(a.CategoryName[i], "");

	for (i=0;i<16;i++)
		a.CategoryName[i][15] = '\0';

    if ((s = hv_fetch(h, "categoryID", 10, 0)) && SvRV(*s) && (SvTYPE(av=(AV*)SvRV(*s))==SVt_PVAV))
    	for (i=0;i<16;i++)
    		a.CategoryID[i] = (s=av_fetch(av, i, 0)) ? SvIV(*s) : 0;
	else
		for (i=0;i<16;i++)
			a.CategoryID[i] = 0;

    if ((s = hv_fetch(h, "lastUniqueID", 12, 0)))
	    a.lastUniqueID = SvIV(*s);
	else
		a.lastUniqueID = 0;

    a.dirtyfieldlabels = (s = hv_fetch(h, "dirtyfieldlabels", 16, 0)) ? SvIV(*s) : 0;
    a.country = (s = hv_fetch(h, "country", 7, 0)) ? SvIV(*s) : 0;
    a.sortByCompany = (s = hv_fetch(h, "sortByCompany", 13, 0)) ? SvIV(*s) : 0;

    if ((s = hv_fetch(h, "label", 5, 0)) && SvRV(*s) && (SvTYPE(av=(AV*)SvRV(*s))==SVt_PVAV))
    	for (i=0;i<22;i++) strncpy(a.labels[i], (s=av_fetch(av, i, 0)) ? SvPV(*s,na) : "", 16);
	else
		for (i=0;i<22;i++) a.labels[i][0] = 0;
	for (i=0;i<22;i++) a.labels[i][15] = 0;

    if ((s = hv_fetch(h, "phoneLabel", 10, 0)) && SvRV(*s) && (SvTYPE(av=(AV*)SvRV(*s))==SVt_PVAV))
    	for (i=0;i<8;i++) strncpy(a.phonelabels[i], (s=av_fetch(av, i, 0)) ? SvPV(*s,na) : "", 16);
	else
		for (i=0;i<8;i++) a.phonelabels[i][0] = 0;
	for (i=0;i<8;i++) a.phonelabels[i][15] = 0;

    pack_AddressAppInfo(&a, mybuf, &len);

    RETVAL = newSVpv(mybuf, len);
    }
    OUTPUT:
    RETVAL

MODULE = PDA::Pilot		PACKAGE = PDA::Pilot::Memo

SV *
Unpack(record)
    SV * record
    CODE:
    {
    int len;
    int i;
    AV * e;
    HV * ret;
    struct Memo a;
    (void)SvPV(record, len);
    unpack_Memo(&a, SvPV(record, len), len);
    
    ret = newHV();
    hv_store(ret, "text", 4, newSVpv(a.text,0), 0);

    free_Memo(&a);

    RETVAL = newRV_noinc((SV*)ret);
    }
    OUTPUT:
    RETVAL

SV *
Pack(record)
    SV * record
    CODE:
    {
    int len;
    SV ** s;
    HV * h;
    struct Memo a;
    
    if (!SvRV(record) || (SvTYPE(h=(HV*)SvRV(record))!=SVt_PVHV))
    	croak("record is not a hash reference");
    
    if ((s = hv_fetch(h, "text", 4, 0)))
	    a.text = SvPV(*s,na);
	else
		a.text = 0;
    
    pack_Memo(&a, mybuf, &len);
    
    RETVAL = newSVpv(mybuf, len);
    }
    OUTPUT:
    RETVAL

SV *
UnpackAppBlock(record)
    SV * record
    CODE:
    {
    int len;
    AV * e;
    HV * ret;
    int i;
    struct MemoAppInfo a;
    SvPV(record, len);
    unpack_MemoAppInfo(&a, SvPV(record, len), len);
    
    ret = newHV();

    hv_store(ret, "renamedCategories",17 , newSViv(a.renamedcategories), 0);
    
    e = newAV();
    hv_store(ret, "categoryName", 12, newRV_noinc((SV*)e), 0);
    
    for (i=0;i<16;i++) {
    	av_push(e, newSVpv(a.CategoryName[i], 0));
    }

    e = newAV();
    hv_store(ret, "categoryID", 10, newRV_noinc((SV*)e), 0);
    
    for (i=0;i<16;i++) {
    	av_push(e, newSViv(a.CategoryID[i]));
    }

    hv_store(ret, "lastUniqueID", 12, newSViv(a.lastUniqueID), 0);

    hv_store(ret, "sortOrder", 9, newSViv(a.sortOrder), 0);

    RETVAL = newRV_noinc((SV*)ret);

    }
    OUTPUT:
    RETVAL

SV *
PackAppBlock(record)
    SV * record
    CODE:
    {
    int i;
    int len;
    SV ** s;
    HV * h;
    AV * av;
    struct MemoAppInfo a;
    
    if (!SvRV(record) || (SvTYPE(h=(HV*)SvRV(record))!=SVt_PVHV))
    	croak("record is not a hash reference");
    
    if ((s = hv_fetch(h, "renamedCategories", 17, 0)))
	    a.renamedcategories = SvIV(*s);
	else
		a.renamedcategories = 0;

    if ((s = hv_fetch(h, "categoryName", 12, 0)) && SvRV(*s) && (SvTYPE(av=(AV*)SvRV(*s))==SVt_PVAV))
    	for (i=0;i<16;i++)
    		strncpy(a.CategoryName[i], (s=av_fetch(av, i, 0)) ? SvPV(*s,na) : "", 16);
	else
		for (i=0;i<16;i++)
			strcpy(a.CategoryName[i], "");

	for (i=0;i<16;i++)
		a.CategoryName[i][15] = '\0';

    if ((s = hv_fetch(h, "categoryID", 10, 0)) && SvRV(*s) && (SvTYPE(av=(AV*)SvRV(*s))==SVt_PVAV))
    	for (i=0;i<16;i++)
    		a.CategoryID[i] = (s=av_fetch(av, i, 0)) ? SvIV(*s) : 0;
	else
		for (i=0;i<16;i++)
			a.CategoryID[i] = 0;

    if ((s = hv_fetch(h, "lastUniqueID", 12, 0)))
	    a.lastUniqueID = SvIV(*s);
	else
		a.lastUniqueID = 0;
		
    if ((s = hv_fetch(h, "sortOrder", 9, 0)))
	    a.sortOrder = SvIV(*s);
	else
		a.sortOrder = 0;
    
    pack_MemoAppInfo(&a, mybuf, &len);

    RETVAL = newSVpv(mybuf, len);
    }
    OUTPUT:
    RETVAL

MODULE = PDA::Pilot		PACKAGE = PDA::Pilot::Mail

SV *
Unpack(record)
    SV * record
    CODE:
    {
    int len;
    int i;
    AV * e;
    HV * ret;
    struct Mail a;
    (void)SvPV(record, len);
    unpack_Mail(&a, SvPV(record, len), len);
    
    ret = newHV();
    if (a.subject) hv_store(ret, "subject", 7, newSVpv(a.subject,0), 0);
    if (a.from) hv_store(ret, "from", 4, newSVpv(a.from,0), 0);
    if (a.to) hv_store(ret, "to", 2, newSVpv(a.to,0), 0);
    if (a.cc) hv_store(ret, "cc", 2, newSVpv(a.cc,0), 0);
    if (a.bcc) hv_store(ret, "bcc", 3, newSVpv(a.bcc,0), 0);
    if (a.replyTo) hv_store(ret, "replyTo", 7, newSVpv(a.replyTo,0), 0);
    if (a.sentTo) hv_store(ret, "sentTo", 6, newSVpv(a.sentTo,0), 0);
    if (a.body) hv_store(ret, "body", 4, newSVpv(a.body,0), 0);
    
    hv_store(ret, "read", 4, newSViv(a.read), 0);
    hv_store(ret, "signature", 9, newSViv(a.signature), 0);
    hv_store(ret, "confirmRead", 11, newSViv(a.confirmRead), 0);
    hv_store(ret, "confirmDelivery", 15, newSViv(a.confirmDelivery), 0);
    hv_store(ret, "priority", 8, newSViv(a.priority), 0);
    hv_store(ret, "addressing", 10, newSViv(a.addressing), 0);

	if (a.dated)
		hv_store(ret, "date", 4, newRV_noinc((SV*)tmtoav(&a.date)), 0);

    free_Mail(&a);
    RETVAL = newRV_noinc((SV*)ret);
    }
    OUTPUT:
    RETVAL

SV *
Pack(record)
    SV * record
    CODE:
    {
    int len;
    SV ** s;
    HV * h;
    struct Mail a;
    
    if (!SvRV(record) || (SvTYPE(h=(HV*)SvRV(record))!=SVt_PVHV))
    	croak("record is not a hash reference");
    
    a.subject = (s = hv_fetch(h, "subject", 7, 0)) ? SvPV(*s,na) : 0;
    a.from = (s = hv_fetch(h, "from", 4, 0)) ? SvPV(*s,na) : 0;
    a.to = (s = hv_fetch(h, "to", 2, 0)) ? SvPV(*s,na) : 0;
    a.cc = (s = hv_fetch(h, "cc", 2, 0)) ? SvPV(*s,na) : 0;
    a.bcc = (s = hv_fetch(h, "bcc", 3, 0)) ? SvPV(*s,na) : 0;
    a.replyTo = (s = hv_fetch(h, "replyTo", 7, 0)) ? SvPV(*s,na) : 0;
    a.sentTo = (s = hv_fetch(h, "sentTo", 6, 0)) ? SvPV(*s,na) : 0;
    a.body = (s = hv_fetch(h, "body", 4, 0)) ? SvPV(*s,na) : 0;
    
    a.read = (s = hv_fetch(h, "body", 4, 0)) ? SvIV(*s) : 0;
    a.signature = (s = hv_fetch(h, "signature", 9, 0)) ? SvIV(*s) : 0;
    a.confirmRead = (s = hv_fetch(h, "confirmRead", 11, 0)) ? SvIV(*s) : 0;
    a.confirmDelivery = (s = hv_fetch(h, "confirmDelivery", 15, 0)) ? SvIV(*s) : 0;
    a.priority = (s = hv_fetch(h, "priority", 8, 0)) ? SvIV(*s) : 0;
    a.addressing = (s = hv_fetch(h, "addressing", 10, 0)) ? SvIV(*s) : 0;
    
    a.dated = (s = hv_fetch(h, "date", 4, 0)) ? 1 : 0;
    if (s) avtotm((AV*)SvRV(*s), &a.date);

    pack_Mail(&a, mybuf, &len);
    
    RETVAL = newSVpv(mybuf, len);
    }
    OUTPUT:
    RETVAL

SV *
UnpackAppBlock(record)
    SV * record
    CODE:
    {
    int len;
    AV * e;
    HV * ret;
    int i;
    struct MailAppInfo a;
    SvPV(record, len);
    unpack_MailAppInfo(&a, SvPV(record, len), len);
    
    ret = newHV();

    hv_store(ret, "renamedCategories",17 , newSViv(a.renamedcategories), 0);
    
    e = newAV();
    hv_store(ret, "categoryName", 12, newRV_noinc((SV*)e), 0);
    
    for (i=0;i<16;i++) {
    	av_push(e, newSVpv(a.CategoryName[i], 0));
    }

    e = newAV();
    hv_store(ret, "categoryID", 10, newRV_noinc((SV*)e), 0);
    
    for (i=0;i<16;i++) {
    	av_push(e, newSViv(a.CategoryID[i]));
    }

    hv_store(ret, "lastUniqueID", 12, newSViv(a.lastUniqueID), 0);


    hv_store(ret, "sortOrder", 9, newSViv(a.sortOrder), 0);

    hv_store(ret, "dirtyfieldlabels", 16, newSViv(a.dirtyfieldlabels), 0);
    hv_store(ret, "unsentMessage", 13, newSViv(a.unsentMessage), 0);

    RETVAL = newRV_noinc((SV*)ret);

    }
    OUTPUT:
    RETVAL

SV *
PackAppBlock(record)
    SV * record
    CODE:
    {
    int i;
    int len;
    SV ** s;
    HV * h;
    AV * av;
    struct MailAppInfo a;
    
    if (!SvRV(record) || (SvTYPE(h=(HV*)SvRV(record))!=SVt_PVHV))
    	croak("record is not a hash reference");
    
    if ((s = hv_fetch(h, "renamedCategories", 17, 0)))
	    a.renamedcategories = SvIV(*s);
	else
		a.renamedcategories = 0;

    if ((s = hv_fetch(h, "categoryName", 12, 0)) && SvRV(*s) && (SvTYPE(av=(AV*)SvRV(*s))==SVt_PVAV))
    	for (i=0;i<16;i++)
    		strncpy(a.CategoryName[i], (s=av_fetch(av, i, 0)) ? SvPV(*s,na) : "", 16);
	else
		for (i=0;i<16;i++)
			strcpy(a.CategoryName[i], "");

	for (i=0;i<16;i++)
		a.CategoryName[i][15] = '\0';

    if ((s = hv_fetch(h, "categoryID", 10, 0)) && SvRV(*s) && (SvTYPE(av=(AV*)SvRV(*s))==SVt_PVAV))
    	for (i=0;i<16;i++)
    		a.CategoryID[i] = (s=av_fetch(av, i, 0)) ? SvIV(*s) : 0;
	else
		for (i=0;i<16;i++)
			a.CategoryID[i] = 0;

    if ((s = hv_fetch(h, "lastUniqueID", 12, 0)))
	    a.lastUniqueID = SvIV(*s);
	else
		a.lastUniqueID = 0;
		
    if ((s = hv_fetch(h, "sortOrder", 9, 0)))
	    a.sortOrder = SvIV(*s);
	else
		a.sortOrder = 0;

	a.dirtyfieldlabels = (s=hv_fetch(h,"dirtyfieldlabels",16,0)) ? SvIV(*s) : 0;
	a.unsentMessage = (s=hv_fetch(h,"unsentMessage",13,0)) ? SvIV(*s) : 0;

    pack_MailAppInfo(&a, mybuf, &len);

    RETVAL = newSVpv(mybuf, len);
    }
    OUTPUT:
    RETVAL

MODULE = PDA::Pilot		PACKAGE = PDA::Pilot

int
Close(socket)
	int	socket
	CODE:
	RETVAL = pi_close(socket);
	OUTPUT:
	RETVAL

int
Write(socket, msg)
	int	socket
	SV *	msg
	CODE: {
	    int len;
		RETVAL = pi_write(socket,SvPV(msg,len),len);
	}

SV *
Read(socket, len)
	int	socket
	int	len
	CODE:
	{
	    int result;
	    if (len > sizeof(mybuf))
	    	len = sizeof(mybuf);
	    result = pi_read(socket, mybuf, len);
	    if (RETVAL >=0) 
	    	RETVAL = newSVpv(mybuf, result);
	    else
	    	RETVAL = &sv_undef;
	}
	OUTPUT:
	RETVAL

int
Socket(domain, type, protocol)
	int	domain
	int	type
	int	protocol
	CODE:
	RETVAL = pi_socket(domain, type, protocol);
	OUTPUT:
	RETVAL

int
Listen(socket, backlog)
	int	socket
	int	backlog
	CODE:
	RETVAL = pi_listen(socket, backlog);
	OUTPUT:
	RETVAL

int
Bind(socket, sockaddr)
	int	socket
	SV *	sockaddr
	CODE:
	{
		struct pi_sockaddr a;
		HV * h;
		if ((h=(HV*)SvRV(sockaddr)) && (SvTYPE(SvRV(sockaddr))==SVt_PVHV)) {
			SV ** s;
			char * name;
			struct pi_sockaddr * a;
	    	if ((s = hv_fetch(h, "device", 6, 0)))
	    		name = SvPV(*s,na);
	    	else
	    		name = "";
			a = calloc(1,sizeof(struct pi_sockaddr)+strlen(name));
			strcpy(a->pi_device, name);
	    	a->pi_family = (s = hv_fetch(h, "family", 6, 0)) ? SvIV(*s) : 0;
	    	a->pi_port = (s = hv_fetch(h, "port", 4, 0)) ? SvIV(*s) : 0;
			RETVAL = pi_bind(socket, a, sizeof(struct pi_sockaddr)+strlen(name));
		} else {
			int len;
			void * c = SvPV(sockaddr, len);
			RETVAL = pi_bind(socket, (struct pi_sockaddr*)c, len);
		}
	}
	OUTPUT:
	RETVAL

int
OpenPort(port)
	char *	port
	CODE:
	{
		struct pi_sockaddr a;
		int socket = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP);
		
		strcpy(a.pi_device, port);
		a.pi_port = PI_PilotSocketDLP;
		a.pi_family = PI_AF_SLP;
		pi_bind(socket, &a, sizeof(a));
		
		pi_listen(socket, 1);
		
		RETVAL = socket;
	}
	OUTPUT:
	RETVAL

SV *
Accept(socket)
	int	socket
	CODE:
	{
		struct pi_sockaddr a;
		int len = sizeof(struct pi_sockaddr);
		int result;
		result = pi_accept(socket, &a, &len);
		if (result < 0) {
			RETVAL = newSViv(result);
		} else {
			PDA__Pilot__DLP * x = malloc(sizeof(PDA__Pilot__DLP));
			SV * sv = newSViv((IV)(void*)x);
			x->errno = 0;
			x->socket = result;
			RETVAL = newRV(sv);
			SvREFCNT_dec(sv);
			sv_bless(RETVAL, gv_stashpv("PDA::Pilot::DLPPtr",0));
		}
	}
	OUTPUT:
	RETVAL

MODULE = PDA::Pilot		PACKAGE = PDA::Pilot::DLP::DBPtr

void
DESTROY(db)
	PDA::Pilot::DLP::DB *	db
	CODE:
	if (db->handle)
		dlp_CloseDB(db->socket, db->handle);
	SvREFCNT_dec(db->connection);
	free(db);

int
errno(self)
	PDA::Pilot::DLP::DB *	self
	CODE:
		RETVAL = self->errno;
		self->errno = 0;

MODULE = PDA::Pilot		PACKAGE = PDA::Pilot::DLP::DBPtr

Result
Close(self)
	PDA::Pilot::DLP::DB *	self
	CODE:
	RETVAL = dlp_CloseDB(self->socket, self->handle);
	OUTPUT:
	RETVAL

Result
SetSortBlock(self, block)
	PDA::Pilot::DLP::DB *	self
	SV *	block
	CODE:
	{
		int len;
		void * c = SvPV(block, len);
		RETVAL = dlp_WriteSortBlock(self->socket, self->handle, c, len);
	}
	OUTPUT:
	RETVAL

Result
Purge(self)
	PDA::Pilot::DLP::DB *	self
	CODE:
	RETVAL = dlp_CleanUpDatabase(self->socket, self->handle);
	OUTPUT:
	RETVAL

Result
ResetFlags(self)
	PDA::Pilot::DLP::DB *	self
	CODE:
	RETVAL = dlp_ResetSyncFlags(self->socket, self->handle);
	OUTPUT:
	RETVAL

Result
DeleteCategory(self, category)
	PDA::Pilot::DLP::DB *	self
	int	category
	CODE:
	RETVAL = dlp_DeleteCategory(self->socket, self->handle, category);
	OUTPUT:
	RETVAL

void
GetRecord(self, index)
	PDA::Pilot::DLP::DB *	self
	int	index
	PPCODE:
	{
		int attr, category;
		unsigned long id;
		int size, result;
	    result = dlp_ReadRecordByIndex(self->socket, self->handle, index, mybuf, &id, &size, &attr, &category);
	    ReturnReadRecord(mybuf,size);
	}

Result
MoveCategory(self, fromcat, tocat)
	PDA::Pilot::DLP::DB *	self
	int	fromcat
	int	tocat
	CODE:
	RETVAL = dlp_MoveCategory(self->socket, self->handle, fromcat, tocat);
	OUTPUT:
	RETVAL


Result
DeleteRecord(self, id)
	PDA::Pilot::DLP::DB *	self
	unsigned long	id
	CODE:
	RETVAL = dlp_DeleteRecord(self->socket, self->handle, 0, id);
	OUTPUT:
	RETVAL


Result
DeleteRecords(self)
	PDA::Pilot::DLP::DB *	self
	CODE:
	RETVAL = dlp_DeleteRecord(self->socket, self->handle, 1, 0);
	OUTPUT:
	RETVAL

Result
ResetNext(self)
	PDA::Pilot::DLP::DB *	self
	CODE:
	RETVAL = dlp_ResetDBIndex(self->socket, self->handle);
	OUTPUT:
	RETVAL

SV *
GetAppBlock(self, len=0xffff, offset=0)
	PDA::Pilot::DLP::DB *	self
	int len
	int	offset
	CODE:
	{
		int result = dlp_ReadAppBlock(self->socket, self->handle, offset, mybuf, len);
		if (result >= 0)
			RETVAL = newSVpv(mybuf, result);
		else {
			RETVAL = &sv_undef;
			self->errno = result;
		}
	}
	OUTPUT:
	RETVAL

SV *
GetSortBlock(self, len=0xffff, offset=0)
	PDA::Pilot::DLP::DB *	self
	int len
	int	offset
	CODE:
	{
		int result = dlp_ReadSortBlock(self->socket,self->handle, offset, mybuf, len);
		if (result >= 0)
			RETVAL = newSVpv(mybuf, result);
		else {
			RETVAL = &sv_undef;
			self->errno = result;
		}
	}
	OUTPUT:
	RETVAL

Result
SetAppBlock(self, block)
	PDA::Pilot::DLP::DB *	self
	SV *	block
	CODE:
	{
		int len;
		void * c = SvPV(block, len);
		RETVAL = dlp_WriteAppBlock(self->socket, self->handle, c, len);
	}
	OUTPUT:
	RETVAL


int
Records(self)
	PDA::Pilot::DLP::DB *	self
	CODE:
	{
		int result = dlp_ReadOpenDBInfo(self->socket, self->handle, &RETVAL);
		if (result < 0) {
			RETVAL = -1;
			self->errno = result;
		}
	}
	OUTPUT:
	RETVAL

void
RecordIDs(self, sort=0)
	PDA::Pilot::DLP::DB *	self
	int	sort
	PPCODE:
	{
		recordid_t * id = (recordid_t*)mybuf;
		int result;
		int start;
		int count;
		int i;
		AV * list = newAV();
		
		start = 0;
		for(;;) {
			result = dlp_ReadRecordIDList(self->socket, self->handle, sort, start,
				0xFFFF/sizeof(recordid_t), id, &count);
			if (result < 0) {
				self->errno = result;
				break;
			} else {
				for(i=0;i<count;i++) {
					EXTEND(sp,1);
					PUSHs(sv_2mortal(newSViv(id[i])));
				}
				if (count == (0xFFFF/sizeof(recordid_t)))
					start = count;
				else
					break;
			}
		}
	}

void
GetRecordByID(self, id)
	PDA::Pilot::DLP::DB *	self
	unsigned long id
	PPCODE:
	{
		int size, result, attr, category, index;
	    result = dlp_ReadRecordById(self->socket, self->handle, id, mybuf, &index, &size, &attr, &category);
	    ReturnReadRecord(mybuf,size);
	}

void
GetNextChanged(self, category=-1)
	PDA::Pilot::DLP::DB *	self
	int	category
	PPCODE:
	{
		int size, result, attr, index, category;
		unsigned long id;
		if (category == -1)
	    	result = dlp_ReadNextModifiedRec(self->socket, self->handle, mybuf, &id, &index, &size, &attr, &category);
	    else
	    	result = dlp_ReadNextModifiedRecInCategory(self->socket, self->handle, category, mybuf, &id, &index, &size, &attr);
	    ReturnReadRecord(mybuf,size);
	}

void
GetNextInCategory(self, category)
	PDA::Pilot::DLP::DB *	self
	int	category
	PPCODE:
	{
		int size, result, attr, index;
		unsigned long id;
	    result = dlp_ReadNextRecInCategory(self->socket, self->handle, category, mybuf, &id, &index, &size, &attr);
	    ReturnReadRecord(mybuf,size);
	}

unsigned long
SetRecord(self, data, id, attr, category)
	PDA::Pilot::DLP::DB *	self
	unsigned long	id
	int	attr
	int	category
	SV *	data
	CODE:
	{
		int len, result;
		void * c = SvPV(data, len);
		result = dlp_WriteRecord(self->socket, self->handle, attr, id, category, c, len, &RETVAL);
		if (result<0) {
			RETVAL = 0;
			self->errno = result;
		}
	}
	OUTPUT:
	RETVAL

void
GetResourceByID(self, type, id)
	PDA::Pilot::DLP::DB *	self
	Char4	type
	int	id
	PPCODE:
	{
		int size, result, index;
	    result = dlp_ReadResourceByType(self->socket, self->handle, type, id, mybuf, &index, &size);
	    ReturnReadResource(mybuf,size);
	}

void
GetResource(self, index)
	PDA::Pilot::DLP::DB *	self
	int	index
	PPCODE:
	{
		int size, result, id;
		Char4 type;
	    result = dlp_ReadResourceByIndex(self->socket, self->handle, index, mybuf, &type, &id, &size);
	    ReturnReadResource(mybuf,size);
	}

SV *
SetResource(self, data, type, id)
	PDA::Pilot::DLP::DB *	self
	SV *	data
	Char4	type
	int	id
	CODE:
	{
		int len, result;
		void * c = SvPV(data, len);
		result = dlp_WriteResource(self->socket, self->handle, type, id, c, len);
		if (result < 0) {
			self->errno = result;
			RETVAL = newSVsv(&sv_undef);
		} else
			RETVAL = newSViv(result);
	}
	OUTPUT:
	RETVAL

Result
DeleteResource(self, type, id)
	PDA::Pilot::DLP::DB *	self
	Char4	type
	int	id
	CODE:
	RETVAL = dlp_DeleteResource(self->socket, self->handle, 0, type, id);
	OUTPUT:
	RETVAL

Result
DeleteResources(self)
	PDA::Pilot::DLP::DB *	self
	CODE:
	RETVAL = dlp_DeleteResource(self->socket, self->handle, 1, 0, 0);
	OUTPUT:
	RETVAL

MODULE = PDA::Pilot		PACKAGE = PDA::Pilot::DLPPtr

void
DESTROY(self)
	PDA::Pilot::DLP *	self
	CODE:
	pi_close(self->socket);
	free(self);

int
errno(self)
	PDA::Pilot::DLP *	self
	CODE:
		RETVAL = self->errno;
		self->errno = 0;

SV *
GetTime(self)
	PDA::Pilot::DLP *	self
	CODE:
	{
		time_t t;
		int result = dlp_GetSysDateTime(self->socket, &t);
		if (result < 0) {
			self->errno = result;
			RETVAL = newSVsv(&sv_undef);
		} else
			RETVAL = newSViv(t);
	}
	OUTPUT:
	RETVAL

Result
SetTime(self, time)
	PDA::Pilot::DLP *	self
	long	time
	CODE:
	RETVAL = dlp_SetSysDateTime(self->socket, time);
	OUTPUT:
	RETVAL

SV *
SysInfo(self)
	PDA::Pilot::DLP *	self
	CODE:
	{
		struct SysInfo si;
		int result = dlp_ReadSysInfo(self->socket, &si);
		if (result < 0) {
			self->errno = result;
			RETVAL = newSVsv(&sv_undef);
		} else {
			HV * i = newHV();
			hv_store(i, "ROMversion", 10, newSViv(si.ROMVersion), 0);	\
	    	hv_store(i, "localizationID", 14, newSViv(si.localizationID), 0);	\
	    	hv_store(i, "name", 4, newSVpv(si.name, si.namelength), 0);	\
			RETVAL = newRV((SV*)i);
		}
	}
	OUTPUT:
	RETVAL

SV *
CardInfo(self, cardno=0)
	PDA::Pilot::DLP *	self
	int	cardno
	CODE:
	{
		struct CardInfo c;
		int result = dlp_ReadStorageInfo(self->socket, cardno, &c);
		if (result < 0) {
			self->errno = result;
			RETVAL = newSVsv(&sv_undef);
		} else {
			HV * i = newHV();
			hv_store(i, "cardno", 6, newSViv(c.cardno), 0);	\
	    	hv_store(i, "version", 7, newSViv(c.version), 0);	\
	    	hv_store(i, "created", 8, newSViv(c.creation), 0);	\
	    	hv_store(i, "ROMsize", 7, newSViv(c.ROMsize), 0);	\
	    	hv_store(i, "RAMsize", 7, newSViv(c.RAMsize), 0);	\
	    	hv_store(i, "RAMfree", 7, newSViv(c.RAMfree), 0);	\
	    	hv_store(i, "name", 4, newSVpv(c.name,0), 0);	\
	    	hv_store(i, "manufacturer", 12, newSVpv(c.manuf,0), 0);	\
			RETVAL = newRV((SV*)i);
		}
	}
	OUTPUT:
	RETVAL

int
SetUserInfo(self, info)
	PDA::Pilot::DLP *	self
	UserInfo	&info
	CODE:
	RETVAL = dlp_WriteUserInfo(self->socket, &info);
	OUTPUT:
	RETVAL

void
Battery(self)
	PDA::Pilot::DLP *	self
	PPCODE:
	{
		int warn, critical, ticks, kind, AC;
		unsigned long voltage;
		int result;
		struct RPC_params p;
		
		PackRPC(&p,0xA0B6, RPC_IntReply,
			RPC_Byte(0), RPC_ShortPtr(&warn), RPC_ShortPtr(&critical),
			RPC_ShortPtr(&ticks), RPC_BytePtr(&kind), RPC_BytePtr(&AC), RPC_End);
			
		result = dlp_RPC(self->socket, &p, &voltage);

		if (result==0) {
			EXTEND(sp,5);
			PUSHs(sv_2mortal(newSVnv((float)voltage/100)));
			PUSHs(sv_2mortal(newSVnv((float)warn/100)));
			PUSHs(sv_2mortal(newSVnv((float)critical/100)));
			PUSHs(sv_2mortal(newSViv(kind)));
			PUSHs(sv_2mortal(newSViv(AC)));
		}
	}

SV *
GetUserInfo(self)
	PDA::Pilot::DLP *	self
	CODE:
	{
		UserInfo info;
		int result;
		result = dlp_ReadUserInfo(self->socket, &info);
		pack_userinfo(RETVAL, info, result);
	}
	OUTPUT:
	RETVAL

Result
Delete(self, name, cardno=0)
	PDA::Pilot::DLP *	self
	char *	name
	int	cardno
	CODE:
	{
		UserInfo info;
		int result;
		RETVAL = dlp_DeleteDB(self->socket, cardno, name);
	}
	OUTPUT:
	RETVAL

SV *
Open(self, name, mode=dlpOpenReadWrite, cardno=0)
	PDA::Pilot::DLP *	self
	int	cardno
	int	mode
	char *	name
	CODE:
	{
		int handle;
		int result = dlp_OpenDB(self->socket, cardno, mode, name, &handle);
		if (result<0) {
			self->errno = result;
			RETVAL = &sv_undef;
		} else {
			PDA__Pilot__DLP__DB * x = malloc(sizeof(PDA__Pilot__DLP__DB));
			SV * sv = newSViv((IV)(void*)x);
			SvREFCNT_inc(ST(0));
			x->connection = ST(0);
			x->socket = self->socket;
			x->handle = handle;
			x->errno = 0;
			RETVAL = newRV(sv);
			SvREFCNT_dec(sv);
			sv_bless(RETVAL, gv_stashpv("PDA::Pilot::DLP::DBPtr",0));
		}
	}
    OUTPUT:
    RETVAL

void
GetAppPref(self, creator, number, backup=1)
	PDA::Pilot::DLP *	self
	Char4	creator
	int	number
	int	backup
	PPCODE:
	{
	}

int
SetAppPref(self, data, creator, number, version, backup=1)
	PDA::Pilot::DLP *	self
	SV *	data
	Char4	creator
	int	number
	int	version
	int	backup
	PPCODE:
	{
	}


Result
Close(self, status=0)
	PDA::Pilot::DLP *	self
	int	status
	CODE:
	RETVAL = dlp_EndOfSync(self->socket, status);
	OUTPUT:
	RETVAL

Result
Abort(self)
	PDA::Pilot::DLP *	self
	CODE:
	RETVAL = dlp_AbortSync(self->socket);
	OUTPUT:
	RETVAL

Result
Reset(self)
	PDA::Pilot::DLP *	self
	CODE:
	RETVAL = dlp_ResetSystem(self->socket);
	OUTPUT:
	RETVAL

Result
Status(self)
	PDA::Pilot::DLP *	self
	CODE:
	RETVAL = dlp_OpenConduit(self->socket);
	OUTPUT:
	RETVAL

Result
Log(self, message)
	PDA::Pilot::DLP *	self
	char *	message
	CODE:
	RETVAL = dlp_AddSyncLogEntry(self->socket,message);
	OUTPUT:
	RETVAL


Result
Dirty(self)
	PDA::Pilot::DLP *	self
	CODE:
	RETVAL = dlp_ResetLastSyncPC(self->socket);
	OUTPUT:
	RETVAL

SV *
GetDBInfo(self, start, where, cardno=0)
	PDA::Pilot::DLP *	self
	int	start
	int	where
	int	cardno
	CODE:
	{
		DBInfo info;
		int result = dlp_ReadDBList(self->socket, cardno, where, start, &info);
		pack_dbinfo(RETVAL, info, result);
	}
	OUTPUT:
	RETVAL

SV *
FindDBInfo(self, start, name, creator, type, cardno=0)
	PDA::Pilot::DLP *	self
	int	start
	SV *	name
	SV *	creator
	SV *	type
	int	cardno
	CODE:
	{
		DBInfo info;
		Char4 c,t;
		int result;
		if (SvOK(creator))
			c = SvChar4(creator);
		else
			c = 0;
		if (SvOK(type))
			t = SvChar4(type);
		else
			t = 0;
		result = dlp_FindDBInfo(self->socket, cardno, start, 
			SvOK(name) ? SvPV(name,na) : 0,
			t, c, &info);
		pack_dbinfo(RETVAL, info, result);
	}
	OUTPUT:
	RETVAL

SV *
GetFeature(self, creator, number)
	PDA::Pilot::DLP *	self
	Char4	creator
	int	number
	CODE:
	{
		unsigned long f;
		int result;
		if ((result = dlp_ReadFeature(self->socket, creator, number, &f))<0) {
			RETVAL = newSVsv(&sv_undef);
			self->errno = result;
		} else {
			RETVAL = newSViv(f);
		}
	}
	OUTPUT:
	RETVAL


void
Call(self, creator, type, action, data=&sv_undef, maxretlen=0xFFFF)
	PDA::Pilot::DLP *	self
	Char4	creator
	Char4	type
	int	action
	SV *	data
	int	maxretlen
	PPCODE:
	{
		unsigned long retcode;
		int len, result;
		(void)SvPV(data,len);
		result = dlp_CallApplication(self->socket, creator, type, action, len, SvPV(data,len),
		                    &retcode, maxretlen, &len, mybuf);
		EXTEND(sp, 2);
		if (result >= 0) {
			PUSHs(sv_2mortal(newSVpv(mybuf, len)));
			if (GIMME != G_SCALAR) {
				PUSHs(sv_2mortal(newSViv(retcode)));
			}
		} else
			PUSHs(&sv_undef);
	}

MODULE = PDA::Pilot		PACKAGE = PDA::Pilot::File

PDA::Pilot::File *
Open(name)
	char *	name
	CODE:
	RETVAL = malloc(sizeof(PDA__Pilot__File));
	RETVAL->errno = 0;
	RETVAL->pf = pi_file_open(name);
	OUTPUT:
	RETVAL

PDA::Pilot::File *
Create(name, info)
	char *	name
	DBInfo	info
	CODE:
	RETVAL = malloc(sizeof(PDA__Pilot__File));
	RETVAL->errno = 0;
	RETVAL->pf = pi_file_create(name, &info);
	OUTPUT:
	RETVAL

MODULE = PDA::Pilot		PACKAGE = PDA::Pilot::FilePtr

int
errno(self)
	PDA::Pilot::File *	self
	CODE:
		RETVAL = self->errno;
		self->errno = 0;

void
DESTROY(self)
	PDA::Pilot::File *	self
	CODE:
	if (self->pf)
		pi_file_close(self->pf);
	free(self);

int
Close(self)
	PDA::Pilot::File *	self
	CODE:
	if (self->pf) {
		RETVAL = pi_file_close(self->pf);
		self->pf = 0;
	} else
		RETVAL = 0;
	OUTPUT:
	RETVAL

SV *
GetAppBlock(self)
	PDA::Pilot::File *	self
	CODE:
	{
	    int len, result;
	    void * buf;
		result = pi_file_get_app_info(self->pf, &buf, &len);
		if (result)
		    RETVAL = &sv_undef;
		else
		    RETVAL = newSVpv(buf, len);
	}
	OUTPUT:
	RETVAL

SV *
GetSortBlock(self)
	PDA::Pilot::File *	self
	CODE:
	{
	    int len, result;
	    void * buf;
		result = pi_file_get_sort_info(self->pf, &buf, &len);
		if (result) {
			self->errno = result;
		    RETVAL = &sv_undef;
		} else
		    RETVAL = newSVpv(buf, len);
	}
	OUTPUT:
	RETVAL

SV *
Records(self)
	PDA::Pilot::File *	self
	CODE:
	{
		int len, result;
		result = pi_file_get_entries(self->pf, &len);
		if (result) {
			self->errno = result;
			RETVAL = &sv_undef;
		} else
			RETVAL = newSViv(len);
	}
	OUTPUT:
	RETVAL

SV *
GetResource(self, index)
	PDA::Pilot::File *	self
	int	index
	CODE:
	{
	    int len, result, id;
	    Char4 type;
	    void * buf;
		result = pi_file_read_resource(self->pf, index, &buf, &len, &type, &id);
		ReturnReadResource(buf,len);
	}
	OUTPUT:
	RETVAL

SV *
GetRecord(self, index)
	PDA::Pilot::File *	self
	int	index
	CODE:
	{
	    int len, result, attr, category;
	    unsigned long id;
	    void * buf;
		result = pi_file_read_record(self->pf, index, &buf, &len, &attr, &category, &id);
		ReturnReadRecord(buf,len);
	}
	OUTPUT:
	RETVAL

SV *
GetRecordByID(self, id)
	PDA::Pilot::File *	self
	unsigned long	id
	CODE:
	{
	    int len, result;
	    int attr, category, index;
	    void * buf;
		result = pi_file_read_record_by_id(self->pf, id, &buf, &len, &index, &attr, &category);
		ReturnReadRecord(buf, len);
	}
	OUTPUT:
	RETVAL

int
CheckID(self, uid)
	PDA::Pilot::File *	self
	unsigned long	uid
	CODE:
	RETVAL = pi_file_id_used(self->pf, uid);
	OUTPUT:
	RETVAL

SV *
GetDBInfo(self)
	PDA::Pilot::File *	self
	CODE:
	{
		DBInfo result;
		int err = pi_file_get_info(self->pf, &result);
		pack_dbinfo(RETVAL, result, err);
	}
	OUTPUT:
	RETVAL

int
SetDBInfo(self, info)
	PDA::Pilot::File *	self
	DBInfo	info
	CODE:
	RETVAL = pi_file_set_info(self->pf, &info);
	OUTPUT:
	RETVAL

int
SetAppBlock(self, info)
	PDA::Pilot::File *	self
	SV *	info
	CODE:
	{
	    int len;
	    char * c = SvPV(info, len);
		RETVAL = pi_file_set_app_info(self->pf, c, len);
    }
	OUTPUT:
	RETVAL

int
SetSortBlock(self, info)
	PDA::Pilot::File *	self
	SV *	info
	CODE:
	{
	    int len;
	    char * c = SvPV(info, len);
		RETVAL = pi_file_set_sort_info(self->pf, c, len);
    }
	OUTPUT:
	RETVAL

int
AddResource(self, data, type, id)
	PDA::Pilot::File *	self
	SV *	data
	Char4	type
	int	id
	CODE:
	{
	    int len, result;
	    void * buf = SvPV(data, len);
		RETVAL = pi_file_append_resource(self->pf, buf, len, type, id);
	}
	OUTPUT:
	RETVAL

int
AddRecord(self, data, uid, attr, category)
	PDA::Pilot::File *	self
	SV *	data
	unsigned long	uid
	int	attr
	int	category
	CODE:
	{
	    int len, result;
	    void * buf = SvPV(data, len);
		RETVAL = pi_file_append_record(self->pf, buf, len, attr, category, uid);
	}
	OUTPUT:
	RETVAL


int
Install(self, socket, cardno)
	PDA::Pilot::File *	self
	PDA::Pilot::DLP *	socket
	int	cardno
	CODE:
	RETVAL = pi_file_install(self->pf, socket->socket, cardno);
	OUTPUT:
	RETVAL

int
Retrieve(self, socket, cardno)
	PDA::Pilot::File *	self
	PDA::Pilot::DLP *	socket
	int	cardno
	CODE:
	RETVAL = pi_file_retrieve(self->pf, socket->socket, cardno);
	OUTPUT:
	RETVAL
