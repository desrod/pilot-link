/*
 * Pilot.xs - Interface pilot-link library with Perl.
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA. *
 */

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include "patchlevel.h"

/* FIXME Hack to get around a perl macro problem with calling the type 'int
   dirty;' in pi-mail.h and pi-todo.h. This originally showed up with perl
   5.6 and gcc-3.x, and was fixed in gcc, but now appears in perl 5.8 with
   gcc-3.x. It smells like another internal macro being exposed into
   userland. */
#undef dirty

#include "pi-macros.h"
#include "pi-buffer.h"
#include "pi-file.h"
#include "pi-datebook.h"
#include "pi-memo.h"
#include "pi-expense.h"
#include "pi-address.h"
#include "pi-todo.h"
#include "pi-mail.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-syspkt.h"
#include "pi-error.h"
#include "pi-version.h"

#include "const-c.inc"


typedef unsigned char * CPTR;

static unsigned char mybuf[0xffff];
static pi_buffer_t pibuf = {NULL, 0, 0};

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
	t->tm_sec 	= (s = av_fetch(av, 0, 0)) ? SvIV(*s) : 0;
	t->tm_min 	= (s = av_fetch(av, 1, 0)) ? SvIV(*s) : 0;
	t->tm_hour 	= (s = av_fetch(av, 2, 0)) ? SvIV(*s) : 0;
	t->tm_mday 	= (s = av_fetch(av, 3, 0)) ? SvIV(*s) : 0;
	t->tm_mon 	= (s = av_fetch(av, 4, 0)) ? SvIV(*s) : 0;
	t->tm_year 	= (s = av_fetch(av, 5, 0)) ? SvIV(*s) : 0;
	t->tm_wday	= (s = av_fetch(av, 6, 0)) ? SvIV(*s) : 0;
	t->tm_yday	= (s = av_fetch(av, 7, 0)) ? SvIV(*s) : 0;
	t->tm_isdst 	= (s = av_fetch(av, 8, 0)) ? SvIV(*s) : 0;

	return t;
}

#ifndef newRV_noinc   
static SV * rv;
#define newRV_noinc(s) ((rv=newRV(s)), SvREFCNT_dec(s), rv)
#endif

#if (PATCHLEVEL < 3) || ((PATCHLEVEL == 3) && (SUBVERSION < 16))
#define sv_derived_from(x, y) sv_isobject((x))
#endif

extern char * printlong _((unsigned long val));
extern unsigned long makelong _((char * c));
SV * newSVChar4 _((unsigned long arg));
unsigned long SvChar4 _((SV *arg));

typedef struct {
	int errnop;
	struct pi_file * pf;
	SV * Class;
} PDA__Pilot__File;

typedef struct DLP {
	int errnop;
	int socket;
} PDA__Pilot__DLP;

typedef struct DLPDB {
	SV *	connection;
	int	socket;
	int	handle;
	int errnop;
	SV * dbname;
	int dbmode;
	int dbcard;
	SV * Class;
} PDA__Pilot__DLP__DB;

/* typedef PDA__Pilot__DLP__DB PDA__Pilot__DLP__ResourceDB;
   typedef PDA__Pilot__DLP__DB PDA__Pilot__DLP__RecordDB;
*/
typedef struct DBInfo DBInfo;
typedef struct PilotUser UserInfo;
typedef unsigned long Char4;
typedef int Result;

SV *
newSVChar4(arg)
	unsigned long arg;
{
	char * c = printlong(arg);
	if((isalpha(c[0]) || (c[0] == ' ') || (c[0] == '_')) &&
	   (isalpha(c[1]) || (c[1] == ' ') || (c[0] == '_')) &&
	   (isalpha(c[2]) || (c[2] == ' ') || (c[0] == '_')) &&
	   (isalpha(c[3]) || (c[3] == ' ') || (c[0] == '_'))) {
		return newSVpv(c,4);
	} else {
		return newSViv(arg);
	}
}

unsigned long
SvChar4(arg)
	SV * arg;
{
	if (SvIOKp(arg)) {
		return SvIV(arg);
	} else {
		STRLEN len;
		char * c = SvPV(arg, len);
		if (len != 4)
			croak("Char4 argument a string that isn't four bytes long");
		return makelong(c);
	}
}

#define pack_dbinfo(arg, var, failure) {	\
		if (failure < 0)  {		\
			arg = &sv_undef;	\
			self->errnop = failure;	\
		} else {			\
			HV * i = newHV();	\
			hv_store(i, "more",                 4, newSViv(var.more), 0);						\
	    		hv_store(i, "flagReadOnly",        12, newSViv((var.flags & dlpDBFlagReadOnly) !=0), 0);		\
		    	hv_store(i, "flagResource",        12, newSViv((var.flags & dlpDBFlagResource) !=0), 0);		\
		    	hv_store(i, "flagBackup",          10, newSViv((var.flags & dlpDBFlagBackup) !=0), 0);			\
	    		hv_store(i, "flagOpen",             8, newSViv((var.flags & dlpDBFlagOpen) !=0), 0);			\
		    	hv_store(i, "flagAppInfoDirty",    16, newSViv((var.flags & dlpDBFlagAppInfoDirty) !=0), 0);		\
		    	hv_store(i, "flagNewer",            9, newSViv((var.flags & dlpDBFlagNewer) !=0), 0);			\
	    		hv_store(i, "flagReset",            9, newSViv((var.flags & dlpDBFlagReset) !=0), 0);			\
		        hv_store(i, "flagCopyPrevention",  18, newSViv((var.flags & dlpDBFlagCopyPrevention) !=0), 0);		\
        		hv_store(i, "flagStream",          10, newSViv((var.flags & dlpDBFlagStream) !=0), 0);			\
	    		hv_store(i, "flagExcludeFromSync", 19, newSViv((var.miscFlags & dlpDBMiscFlagExcludeFromSync)!=0), 0);	\
		    	hv_store(i, "type",                 4, newSVChar4(var.type), 0);	\
		    	hv_store(i, "creator",              7, newSVChar4(var.creator), 0);	\
	    		hv_store(i, "version",              7, newSViv(var.version), 0);	\
		    	hv_store(i, "modnum",               6, newSViv(var.modnum), 0);		\
		    	hv_store(i, "index",                5, newSViv(var.index), 0);		\
	    		hv_store(i, "createDate",          10, newSViv(var.createDate), 0);	\
		    	hv_store(i, "modifyDate",          10, newSViv(var.modifyDate), 0);	\
		    	hv_store(i, "backupDate",          10, newSViv(var.backupDate), 0);	\
	    		hv_store(i, "name",                 4, newSVpv(var.name, 0), 0);	\
			arg = newRV_noinc((SV*)i);\
		}\
	}

#define unpack_dbinfo(arg, var)\
	if (SvROK(arg) && (SvTYPE(SvRV(arg))==SVt_PVHV)) {\
		HV * i = (HV*)SvRV(arg);\
		SV ** s;\
		var.more = (s = hv_fetch(i, "more", 4, 0)) ? SvIV(*s) : 0;\
		var.flags =\
			(((s = hv_fetch(i, "flagReadOnly",       12, 0)) && SvTRUE(*s)) ? dlpDBFlagReadOnly : 0) |	\
			(((s = hv_fetch(i, "flagResource",       12, 0)) && SvTRUE(*s)) ? dlpDBFlagResource : 0) |	\
			(((s = hv_fetch(i, "flagBackup",         10, 0)) && SvTRUE(*s)) ? dlpDBFlagBackup : 0) |	\
			(((s = hv_fetch(i, "flagOpen",            8, 0)) && SvTRUE(*s)) ? dlpDBFlagOpen : 0) |		\
			(((s = hv_fetch(i, "flagAppInfoDirty",   16, 0)) && SvTRUE(*s)) ? dlpDBFlagAppInfoDirty : 0)|	\
			(((s = hv_fetch(i, "flagNewer",           9, 0)) && SvTRUE(*s)) ? dlpDBFlagNewer : 0) |		\
			(((s = hv_fetch(i, "flagReset",           9, 0)) && SvTRUE(*s)) ? dlpDBFlagReset : 0) |		\
			(((s = hv_fetch(i, "flagCopyPrevention", 18, 0)) && SvTRUE(*s)) ? dlpDBFlagCopyPrevention : 0) |\
			(((s = hv_fetch(i, "flagStream",         10, 0)) && SvTRUE(*s)) ? dlpDBFlagStream : 0) |	\
	    	0;\
		var.miscFlags =\
			(((s = hv_fetch(i, "flagExcludeFromSync", 19, 0)) && SvTRUE(*s)) ? dlpDBMiscFlagExcludeFromSync : 0);\
		var.type 	= (s = hv_fetch(i, "type",        4, 0)) ? SvChar4(*s) : 0;\
		var.creator 	= (s = hv_fetch(i, "creator",     7, 0)) ? SvChar4(*s) : 0;\
		var.version 	= (s = hv_fetch(i, "version",     7, 0)) ? SvIV(*s) : 0;\
		var.modnum 	= (s = hv_fetch(i, "modnum",      6, 0)) ? SvIV(*s) : 0;\
		var.index 	= (s = hv_fetch(i, "index",       5, 0)) ? SvIV(*s) : 0;\
		var.createDate 	= (s = hv_fetch(i, "createDate", 10, 0)) ? SvIV(*s) : 0;\
		var.modifyDate 	= (s = hv_fetch(i, "modifyDate", 10, 0)) ? SvIV(*s) : 0;\
		var.backupDate 	= (s = hv_fetch(i, "backupDate", 10, 0)) ? SvIV(*s) : 0;\
		if ((s = hv_fetch(i, "name", 4, 0)) ? SvPV(*s,na) : 0)		\
			strncpy(var.name, SvPV(*s, na), sizeof(var.name));	\
		} else	{\
			croak("argument is not a hash reference");		\
		}

#define pack_userinfo(arg, var, failure) {	\
	if (failure < 0)  {			\
		arg = &sv_undef;		\
		self->errnop = failure;		\
	} else {				\
		HV * i = newHV();		\
		hv_store(i, "userID",              6, newSViv(var.userID), 0);				\
	    	hv_store(i, "viewerID",            8, newSViv(var.viewerID), 0);			\
	    	hv_store(i, "lastSyncPC",         10, newSViv(var.lastSyncPC), 0);			\
	    	hv_store(i, "successfulSyncDate", 18, newSViv(var.successfulSyncDate), 0);		\
	    	hv_store(i, "lastSyncDate",       12, newSViv(var.lastSyncDate), 0);			\
	    	hv_store(i, "name",                4, newSVpv(var.username,0), 0);			\
	    	hv_store(i, "password",            8, newSVpvn(var.password,var.passwordLength), 0);	\
		arg = newRV_noinc((SV*)i);	\
	}\
}

#define unpack_userinfo(arg, var)\
	if (SvROK(arg) && (SvTYPE(SvRV(arg))==SVt_PVHV)) {\
		HV * i = (HV*)SvRV(arg);\
		SV ** s;\
		var.userID 		= (s = hv_fetch(i, "userID",              6, 0)) ? SvIV(*s) : 0;\
		var.viewerID 		= (s = hv_fetch(i, "viewerID",            8, 0)) ? SvIV(*s) : 0;\
		var.lastSyncPC 		= (s = hv_fetch(i, "lastSyncPC",         10, 0)) ? SvIV(*s) : 0;\
		var.lastSyncDate 	= (s = hv_fetch(i, "lastSyncDate",       12, 0)) ? SvIV(*s) : 0;\
		var.successfulSyncDate 	= (s = hv_fetch(i, "successfulSyncDate", 18, 0)) ? SvIV(*s) : 0;\
		if ((s = hv_fetch(i, "name", 4, 0)) ? SvPV(*s,na) : 0)\
			strncpy(var.username, SvPV(*s, na), sizeof(var.username));\
		} else	{\
			croak("argument is not a hash reference");\
		}

#define PackAI {\
	HV * h;\
	if (SvRV(data) && (SvTYPE(h=(HV*)SvRV(data))==SVt_PVHV)) {\
		int count;	\
		PUSHMARK(sp);	\
	        XPUSHs(data);	\
		PUTBACK;	\
		count = perl_call_method("Pack", G_SCALAR);\
		SPAGAIN;	\
		if (count != 1)	\
			croak("Unable to pack app block");\
		data = POPs;	\
		PUTBACK;	\
	} else {		\
		croak("Unable to pack app block");\
	}			\
}

#define ReturnReadAI(buf,size)\
	if (result >=0) {\
		if (self->Class) {\
			int count;\
	    		PUSHMARK(sp);\
	    		XPUSHs(self->Class);\
	    		XPUSHs(newSVpvn((char *) buf, size));	\
		    	PUTBACK;\
		    	count = perl_call_method("appblock", G_SCALAR);\
		    	SPAGAIN;\
		    	if (count != 1)\
		    		croak("Unable to create appblock");\
	    	} else {\
	    		croak("Class not defined");\
	    	}\
	} else {\
		self->errnop = result;\
		PUSHs(&sv_undef);\
	}

#define PackSI\
	    {\
	    	HV * h;\
	    	if (SvRV(data) &&\
	    		(SvTYPE(h=(HV*)SvRV(data))==SVt_PVHV)) {\
	    		int count;\
	        	PUSHMARK(sp);\
	          	XPUSHs(data);\
		    	PUTBACK;\
		    	count = perl_call_method("Pack", G_SCALAR);\
		    	SPAGAIN;\
		    	if (count != 1)\
		    		croak("Unable to pack sort block");\
		    	data = POPs;\
		    	PUTBACK;\
	        }\
	        else {\
		    		croak("Unable to pack sort block");\
	        }\
	    }

#define ReturnReadSI(buf,size)\
	    if (result >=0) {\
	    	if (self->Class) {\
	    		int count;\
	    		PUSHMARK(sp);\
	    		XPUSHs(self->Class);\
	    		XPUSHs(newSVpvn((char *) buf, size));	\
		    	PUTBACK;\
		    	count = perl_call_method("sortblock", G_SCALAR);\
		    	SPAGAIN;\
		    	if (count != 1)\
		    		croak("Unable to create sortblock");\
	    	}\
	    	else {\
	    		croak("Class not defined");\
	    	}\
		} else {\
	    	self->errnop = result;\
	    	PUSHs(&sv_undef);\
	    }

#define PackRecord {\
	HV * h;\
	if (SvRV(data) &&\
		(SvTYPE(h=(HV*)SvRV(data))==SVt_PVHV)) {\
	    	int count;\
	    	SV ** s;\
	    	if (!(s = hv_fetch(h, "id", 2, 0)) || !SvOK(*s))\
	    		croak("record must contain id");\
		id = SvIV(*s);\
    		attr = 0;\
		if (!(s = hv_fetch(h, "secret", 6, 0)) || !SvOK(*s))\
			croak("record must contain secret");\
		attr |= SvTRUE(*s) ? dlpRecAttrSecret : 0;\
		if (!(s = hv_fetch(h, "deleted", 7, 0)) || !SvOK(*s))\
			croak("record must contain deleted");\
		attr |= SvTRUE(*s) ? dlpRecAttrDeleted : 0;\
		if (!(s = hv_fetch(h, "modified", 8, 0)) || !SvOK(*s))\
	    		croak("record must contain modified");\
		attr |= SvTRUE(*s) ? dlpRecAttrDirty : 0;\
	    	if (!(s = hv_fetch(h, "busy", 4, 0)) || !SvOK(*s))\
	    		croak("record must contain busy");\
		attr |= SvTRUE(*s) ? dlpRecAttrBusy : 0;\
		if (!(s = hv_fetch(h, "archived", 8, 0)) || !SvOK(*s))\
			croak("record must contain archived");\
		attr |= SvTRUE(*s) ? dlpRecAttrArchived : 0;\
		if (!(s = hv_fetch(h, "category", 8, 0)) || !SvOK(*s))\
			croak("record must contain category");\
		category = SvIV(*s);\
	        PUSHMARK(sp);\
	        XPUSHs(data);\
		PUTBACK;\
		count = perl_call_method("Pack", G_SCALAR);\
		SPAGAIN;\
		if (count != 1)\
			croak("Unable to pack record");\
		data = POPs;\
		PUTBACK;\
	} else {\
		croak("Unable to pack record");\
	}\
}

#define PackRaw {\
	HV * h;\
	if (SvRV(data) &&\
		(SvTYPE(h=(HV*)SvRV(data))==SVt_PVHV)) {\
	    	int count;\
	        PUSHMARK(sp);\
	        XPUSHs(data);\
		PUTBACK;\
		count = perl_call_method("Raw", G_SCALAR);\
		SPAGAIN;\
		if (count != 1)	{\
			SV ** s = hv_fetch(h, "raw", 3, 0);\
		    	if (s)\
		    		data = *s;\
		} else {\
			data = POPs;\
			PUTBACK;\
		}\
	}\
}

#define ReturnReadRecord(buf,size)\
	    if (result >=0) {\
	    	if (self->Class) {\
	    		int count;\
	    		SV * ret;\
	    		PUSHMARK(sp);\
	    		XPUSHs(self->Class);\
	    		XPUSHs(newSVpvn((char *) buf, size));	\
		    	XPUSHs(sv_2mortal(newSViv(id)));\
		    	XPUSHs(sv_2mortal(newSViv(attr)));\
		    	XPUSHs(sv_2mortal(newSViv(category)));\
		    	XPUSHs(sv_2mortal(newSViv(index)));\
		    	PUTBACK;\
		    	count = perl_call_method("record", G_SCALAR);\
		    	SPAGAIN;\
		    	if (count != 1)\
		    		croak("Unable to create record");\
		    	ret = POPs;\
		    	PUTBACK;\
		    	PUSHs(ret);\
	    	}\
	    	else {\
	    		croak("Class not defined");\
	    	}\
		} else {\
	    	self->errnop = result;\
	    	PUSHs(&sv_undef);\
	    }

#define PackResource\
	    {\
	    	HV * h;\
	    	if (SvRV(data) &&\
	    		(SvTYPE(h=(HV*)SvRV(data))==SVt_PVHV)) {\
	    		int count;\
	    		SV ** s;\
	    		if (!(s = hv_fetch(h, "id", 2, 0)) || !SvOK(*s))\
	    			croak("record must contain id");\
    			id = SvIV(*s);\
	    		if (!(s = hv_fetch(h, "type", 4, 0)) || !SvOK(*s))\
	    			croak("record must contain type");\
    			type = SvChar4(*s);\
	        	PUSHMARK(sp);\
	          	XPUSHs(data);\
		    	PUTBACK;\
		    	count = perl_call_method("Pack", G_SCALAR);\
		    	SPAGAIN;\
		    	if (count != 1)\
		    		croak("Unable to pack resource");\
		    	data = POPs;\
		    	PUTBACK;\
	        }\
	        else {\
		    		croak("Unable to pack resource");\
	        }\
	    }

#define ReturnReadResource(buf,size)\
	    if (result >=0) {\
	    	if (self->Class) {\
	    		int count;\
	    		PUSHMARK(sp);\
	    		XPUSHs(self->Class);\
	    		XPUSHs(newSVpvn((char *) buf, size));	\
		    	XPUSHs(sv_2mortal(newSVChar4(type)));\
		    	XPUSHs(sv_2mortal(newSViv(id)));\
		    	XPUSHs(sv_2mortal(newSViv(index)));\
		    	PUTBACK;\
		    	count = perl_call_method("resource", G_SCALAR);\
		    	SPAGAIN;\
		    	if (count != 1)\
		    		croak("Unable to create resource");\
	    	}\
	    	else {\
	    		croak("Class not defined");\
	    	}\
		} else {\
	    	self->errnop = result;\
	    	PUSHs(&sv_undef);\
	    }

#define PackPref\
	    {\
	    	HV * h;\
	    	if (SvRV(data) &&\
	    		(SvTYPE(h=(HV*)SvRV(data))==SVt_PVHV)) {\
	    		int count;\
	    		SV ** s;\
	    		if (!(s = hv_fetch(h, "id", 2, 0)) || !SvOK(*s))\
	    			croak("record must contain id");\
    			id = SvIV(*s);\
	    		if (!(s = hv_fetch(h, "creator", 7, 0)) || !SvOK(*s))\
	    			croak("record must contain type");\
    			creator = SvChar4(*s);\
	    		if (!(s = hv_fetch(h, "version", 7, 0)) || !SvOK(*s))\
	    			croak("record must contain type");\
    			version = SvIV(*s);\
	    		if (!(s = hv_fetch(h, "backup", 6, 0)) || !SvOK(*s))\
	    			croak("record must contain type");\
    			backup = SvIV(*s);\
   	        	PUSHMARK(sp);\
	          	XPUSHs(data);\
		    	PUTBACK;\
		    	count = perl_call_method("Pack", G_SCALAR);\
		    	SPAGAIN;\
		    	if (count != 1)\
		    		croak("Unable to pack resource");\
		    	data = POPs;\
		    	PUTBACK;\
	        }\
	        else {\
		    		croak("Unable to pack resource");\
	        }\
	    }

#define ReturnReadPref(buf,size)\
	    if (result >=0) {\
			HV * h = perl_get_hv("PDA::Pilot::PrefClasses", 0);\
			SV ** s;\
    		int count;\
			if (!h)\
				croak("PrefClasses doesn't exist");\
			s = hv_fetch(h, printlong(creator), 4, 0);\
			if (!s)\
				s = hv_fetch(h, "", 0, 0);\
			if (!s)\
				croak("Default PrefClass not defined");\
    		PUSHMARK(sp);\
    		XPUSHs(newSVsv(*s));\
    		XPUSHs(newSVpvn((char *) buf, size));		\
	    	XPUSHs(sv_2mortal(newSVChar4(creator)));\
	    	XPUSHs(sv_2mortal(newSViv(id)));\
	    	XPUSHs(sv_2mortal(newSViv(version)));\
	    	XPUSHs(sv_2mortal(newSViv(backup)));\
	    	PUTBACK;\
	    	count = perl_call_method("pref", G_SCALAR);\
	    	SPAGAIN;\
	    	if (count != 1)\
	    		croak("Unable to create resource");\
		} else {\
	    	self->errnop = result;\
	    	PUSHs(&sv_undef);\
	    }

void doUnpackCategory(HV * self, struct CategoryAppInfo * c)
{
	AV * e = newAV();
	int i;
	
    hv_store(self, "categoryRenamed", 15, newRV_noinc((SV*)e), 0);

    for (i=0;i<16;i++) {
    	av_push(e, newSViv(c->renamed[i]));
    }

    e = newAV();
    hv_store(self, "categoryName", 12, newRV_noinc((SV*)e), 0);
    
    for (i=0;i<16;i++) {
    	av_push(e, newSVpv(c->name[i], 0));
    }

    e = newAV();
    hv_store(self, "categoryID", 10, newRV_noinc((SV*)e), 0);
    
    for (i=0;i<16;i++) {
    	av_push(e, newSViv(c->ID[i]));
    }

    hv_store(self, "categoryLastUniqueID", 20, newSViv(c->lastUniqueID), 0);
}


void doPackCategory(HV * self, struct CategoryAppInfo * c)
{
	SV ** s;
	AV * av;
	int i;
	
    if ((s = hv_fetch(self, "categoryName", 12, 0)) && SvOK(*s) && SvRV(*s) && (SvTYPE(av=(AV*)SvRV(*s))==SVt_PVAV))
    	for (i=0;i<16;i++)
    		strncpy(c->name[i], (s=av_fetch(av, i, 0)) ? SvPV(*s,na) : "", 16);
	else
		for (i=0;i<16;i++)
			strcpy(c->name[i], "");

	for (i=0;i<16;i++)
		c->name[i][15] = '\0';

    if ((s = hv_fetch(self, "categoryID", 10, 0)) && SvOK(*s) && SvRV(*s) && (SvTYPE(av=(AV*)SvRV(*s))==SVt_PVAV))
    	for (i=0;i<16;i++)
    		c->ID[i] = (s=av_fetch(av, i, 0)) ? SvIV(*s) : 0;
	else
		for (i=0;i<16;i++)
			c->ID[i] = 0;

    if ((s = hv_fetch(self, "categoryRenamed", 10, 0)) && SvOK(*s) && SvRV(*s) && (SvTYPE(av=(AV*)SvRV(*s))==SVt_PVAV))
    	for (i=0;i<16;i++)
    		c->renamed[i] = (s=av_fetch(av, i, 0)) ? SvIV(*s) : 0;
	else
		for (i=0;i<16;i++)
			c->renamed[i] = 0;
}

int SvList(SV * arg, char **list)
{
	int i;
	char * str = SvPV(arg, na);
	for (i=0;list[i];i++)
		if (strcasecmp(list[i], str)==0)
			return i;
	if (SvPOKp(arg)) {
		croak("Invalid value");
	}
	return SvIV(arg);
}

SV * newSVlist(int value, char **list)
{
	int i;
	for (i=0;list[i];i++)
		;
	if (value < i)
		return newSVpv(list[value], 0);
	else
		return newSViv(value);
}

MODULE = PDA::Pilot		PACKAGE = PDA::Pilot

INCLUDE: const-xs.inc


MODULE = PDA::Pilot		PACKAGE = PDA::Pilot::Appointment

SV *
Unpack(record)
    SV * record
    CODE:
    {
    STRLEN len;
    int i;
    AV * e;
    HV * ret, *h;
    struct Appointment a;
    char *str;
    
    if (SvOK(record) && SvRV(record) && (SvTYPE(SvRV(record)) == SVt_PVHV)) {
    	SV ** raw;
    	ret = (HV*)SvRV(record);
    	raw = hv_fetch(ret, "raw", 3, 0);
    	if (!raw || !SvOK(*raw))
    		croak("Unable to unpack");
    	RETVAL = newSVsv(record);
    	record = *raw;
    } else {
    	ret = newHV();
    	hv_store(ret, "raw", 3, newSVsv(record),0);
    	RETVAL = newRV_noinc((SV*)ret);
    }
    
    if (!SvPOK(record)) {
        croak("Not a string!?");
    }
    str = SvPV(record,len);
    if (len > 0)  { /* len == 0 when the record has the deleted flag set */
	pi_buffer_clear(&pibuf);
	if (!pi_buffer_append(&pibuf, str, len)) {
	    croak("Unable to reallocate buffer");
	}
	if (unpack_Appointment(&a, &pibuf, datebook_v1) < 0) {
	    croak("unpack_Appointment failed");
	}

	hv_store(ret, "event", 5, newSViv(a.event), 0);
	hv_store(ret, "begin", 5, newRV_noinc((SV*)tmtoav(&a.begin)), 0);
    
	if (!a.event) {
	    hv_store(ret, "end", 3, newRV_noinc((SV*)tmtoav(&a.end)), 0);
	}
    
	if (a.alarm) {
	    HV * alarm = newHV();
	    hv_store(ret, "alarm", 5, newRV_noinc((SV*)alarm), 0);
	    
	    hv_store(alarm, "advance", 7, newSViv(a.advance), 0);
	    hv_store(alarm, "units", 5, newSVpv((
						 (a.advanceUnits == 0) ? "minutes" :        /* Minutes */
						 (a.advanceUnits == 1) ? "hours" :     /* Hours */
						 (a.advanceUnits == 2) ? "days" :  /* Days */
						 0), 0), 0);
	    if (a.advanceUnits > 2) {
		warn("Invalid advance unit %d encountered", a.advanceUnits);
	    }
	}
	if (a.repeatType) {
	    HV * repeat = newHV();
	    hv_store(ret, "repeat", 6, newRV_noinc((SV*)repeat), 0);
	        
	    hv_store(repeat, "type", 4, newSVpv(DatebookRepeatTypeNames[a.repeatType],0), 0);
	    hv_store(repeat, "frequency", 9, newSViv(a.repeatFrequency), 0);
	    if (a.repeatType == repeatMonthlyByDay)
		hv_store(repeat, "day", 3, newSViv(a.repeatDay), 0);
	    else if (a.repeatType == repeatWeekly) {
		e = newAV();
		hv_store(repeat, "days", 4, newRV_noinc((SV*)e), 0);
		for (i=0;i<7;i++)
		    av_push(e,newSViv(a.repeatDays[i]));
	    }
	    hv_store(repeat, "weekstart", 9, newSViv(a.repeatWeekstart), 0);
	    if (!a.repeatForever)
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
    }
    }
    OUTPUT:
    RETVAL

SV *
Pack(record)
    SV * record
    CODE:
    {
    SV ** s;
    HV * h;
    long advance;
    struct Appointment a;

    if (!SvOK(record) || !SvRV(record) || (SvTYPE(h=(HV*)SvRV(record))!=SVt_PVHV))
    	RETVAL = record;
    else if ((s=hv_fetch(h, "deleted", 7, 0)) && SvOK(*s) && SvTRUE(*s) &&
    		(s=hv_fetch(h, "archived", 8, 0)) && SvOK(*s) && !SvTRUE(*s)) {
    	RETVAL = newSVpv("",0);
	    hv_store(h, "raw", 3, SvREFCNT_inc(RETVAL), 0);
    }
    else {

    a.event = (s = hv_fetch(h, "event", 5, 0)) ? SvIV(*s) : 0;
    if ((s= hv_fetch(h, "begin", 5, 0))  && SvOK(*s) && SvRV(*s) && (SvTYPE(SvRV(*s))==SVt_PVAV))
    	avtotm((AV*)SvRV(*s), &a.begin);
    else {
      memset(&a.begin, '\0', sizeof(struct tm));
      croak("appointments must contain a begin date");
    }
    if ((s= hv_fetch(h, "end", 3, 0))  && SvOK(*s) && SvRV(*s) && (SvTYPE(SvRV(*s))==SVt_PVAV))
    	avtotm((AV*)SvRV(*s), &a.end);
    else
    	memset(&a.end, '\0', sizeof(struct tm));

	if ((s = hv_fetch(h, "alarm", 5, 0)) && SvOK(*s) && SvRV(*s) && (SvTYPE(SvRV(*s))==SVt_PVHV)) {
		HV * h2 = (HV*)SvRV(*s);
		I32 u;
	    a.advance = (s = hv_fetch(h2, "advance", 7, 0)) ? SvIV(*s) : 0;
	    s = hv_fetch(h2, "units", 5, 0);
	    if SvIOK(*s) {
		u = SvIV(*s);
		switch (u) {
		case 60:
		    u = 0;
		    break;
		case 60*60:
		    u = 1;
		    break;
		case 60*60*24:
		    u = 2;
		    break;
		default:
		    croak("Invalid advance unit %d encountered", u);
		}
	    } else {
	    	if (strEQ(SvPV(*s, na), "minutes"))
		    u = 0;
	    	else if (strEQ(SvPV(*s, na), "hours"))
		    u = 1;
	    	else if (strEQ(SvPV(*s, na), "days"))
		    u = 2;
	    	else
		    croak("Invalid advance unit %d encountered", u);
	    }
	    a.advanceUnits = u;
	    if (a.advance > 254)
	    	warn("Alarm advance value %d out of range", a.advance);
	    a.alarm = 1;
    } else {
    	a.alarm = 0;
    	a.advance = 0;
    	a.advanceUnits = 0;
    }    	


	if ((s = hv_fetch(h, "repeat", 6, 0)) && SvOK(*s) && SvRV(*s) && (SvTYPE(SvRV(*s))==SVt_PVHV)) {
		HV * h2 = (HV*)SvRV(*s);
		int i;
	    a.repeatType = (s = hv_fetch(h2, "type", 4, 0)) ? SvList(*s, DatebookRepeatTypeNames) : 0;
	    a.repeatFrequency = (s = hv_fetch(h2, "frequency", 9, 0)) ? SvIV(*s) : 0;
	    a.repeatDay = 0;
	    for(i=0;i<7;i++)
	    	a.repeatDays[i] = 0;
	    if (a.repeatType == repeatMonthlyByDay ) {
	    	a.repeatDay = (s = hv_fetch(h2, "day", 3, 0)) ? SvIV(*s) : 0;
	    } else if (a.repeatType == repeatWeekly) {
			if ((s = hv_fetch(h2, "days", 4, 0)) && SvOK(*s) && SvRV(*s) && (SvTYPE(SvRV(*s))==SVt_PVAV)) {
				int i;
				AV * a2 = (AV*)SvRV(*s);
			    for (i=0;i<7;i++)
			    	if ((s = av_fetch(a2, i, 0)))
			    		a.repeatDays[i] = SvIV(*s);
			}
	    }
	    a.repeatWeekstart = (s = hv_fetch(h2, "weekstart", 9, 0)) ? SvIV(*s) : 0;
	    if ((s = hv_fetch(h2, "end", 3, 0)) && SvOK(*s) && SvRV(*s) && (SvTYPE(SvRV(*s))==SVt_PVAV))  {
	    	avtotm((AV*)SvRV(*s), &a.repeatEnd);
	    	a.repeatForever = 0;
	    } else {
	    	a.repeatForever = 1;
	    }
    } else {
    	a.repeatType = 0;
    	a.repeatForever = 0;
    	a.repeatFrequency = 0;
    	a.repeatDay = 0;
    	a.repeatWeekstart = 0;
    	memset(&a.repeatEnd,'\0', sizeof(struct tm));
    }    	

	a.exceptions = 0;
	a.exception = 0;
	if ((s = hv_fetch(h, "exceptions", 10, 0)) && SvOK(*s) && SvRV(*s) && (SvTYPE(SvRV(*s))==SVt_PVAV)) {
		int i;
		AV * a2 = (AV*)SvRV(*s);
		if (av_len(a2)>-1) {
		    a.exceptions = av_len(a2)+1;
		    a.exception = malloc(sizeof(struct tm)*a.exceptions);
		    for (i=0;i<a.exceptions;i++)
		    	if ((s = av_fetch(a2, i, 0)) && SvOK(*s) && SvRV(*s) && (SvTYPE(SvRV(*s))==SVt_PVAV))
		    		avtotm((AV*)SvRV(*s), a.exception+i);
		}
    }    	

    a.description = (s = hv_fetch(h, "description", 11, 0)) ? SvPV(*s,na) : 0;
    if (!a.description)
        croak("appointments must contain a description");
    a.note = (s = hv_fetch(h, "note", 4, 0)) ? SvPV(*s,na) : 0;

    if (pack_Appointment(&a, &pibuf, datebook_v1) < 0) {
	croak("pack_Appointment failed");
    }

    if (a.exception)
		free(a.exception);
    
    RETVAL = newSVpvn((char *) pibuf.data, pibuf.used);

    hv_store(h, "raw", 3, SvREFCNT_inc(RETVAL), 0);
    }
    }
    OUTPUT:
    RETVAL


SV *
UnpackAppBlock(record)
    SV * record
    CODE:
    {
    STRLEN len;
    AV * e;
    HV * ret;
    int i;
    struct AppointmentAppInfo a;

    if (SvOK(record) && SvRV(record) && (SvTYPE(SvRV(record)) == SVt_PVHV)) {
    	SV ** raw;
    	ret = (HV*)SvRV(record);
    	raw = hv_fetch(ret, "raw", 3, 0);
    	if (!raw || !SvOK(*raw))
    		croak("Unable to unpack");
    	RETVAL = newSVsv(record);
    	record = *raw;
    } else {
    	ret = newHV();
    	hv_store(ret, "raw", 3, newSVsv(record),0);
    	RETVAL = newRV_noinc((SV*)ret);
    }
    
    SvPV(record,len);
    if (unpack_AppointmentAppInfo(&a, (CPTR)SvPV(record, na), len)>0) {

		doUnpackCategory(ret, &a.category);
	
	    hv_store(ret, "startOfWeek", 11, newSViv(a.startOfWeek), 0);
	}

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
    	RETVAL = record;
    else {
    
    doPackCategory(h, &a.category);

    if ((s = hv_fetch(h, "startOfWeek", 11, 0)))
	    a.startOfWeek = SvIV(*s);
	else
		a.startOfWeek = 0;

    len = pack_AppointmentAppInfo(&a, (unsigned char *) mybuf, 0xffff);

    RETVAL = newSVpvn((char *) mybuf, len);

    hv_store(h, "raw", 3, SvREFCNT_inc(RETVAL), 0);
    }
    }
    OUTPUT:
    RETVAL

MODULE = PDA::Pilot		PACKAGE = PDA::Pilot::ToDo

SV *
Unpack(record)
    SV * record
    CODE:
    {
    STRLEN len;
    int i;
    AV * e;
    HV * ret;
    struct ToDo a;
    char *str;

    if (SvOK(record) && SvRV(record) && (SvTYPE(SvRV(record)) == SVt_PVHV)) {
    	SV ** raw;
    	ret = (HV*)SvRV(record);
    	raw = hv_fetch(ret, "raw", 3, 0);
    	if (!raw || !SvOK(*raw))
    		croak("Unable to unpack");
    	RETVAL = newSVsv(record);
    	record = *raw;
    } else {
    	ret = newHV();
    	hv_store(ret, "raw", 3, newSVsv(record),0);
    	RETVAL = newRV_noinc((SV*)ret);
    }
    
    str = SvPV(record,len);
    if (len > 0) { /* len == 0 if deleted flag is set */
	pi_buffer_clear(&pibuf);
	if (!pi_buffer_append(&pibuf, str, len)) {
	    croak("Unable to reallocate buffer");
	}
	if (unpack_ToDo(&a, &pibuf, todo_v1) < 0) {
	    croak("unpack_ToDo failed");
	}

	if (!a.indefinite)
	    hv_store(ret, "due", 3, newRV_noinc((SV*)tmtoav(&a.due)), 0);  
	hv_store(ret, "priority", 8, newSViv(a.priority), 0);
	hv_store(ret, "complete", 8, newSViv(a.complete), 0);
	if (a.description)
	    hv_store(ret, "description", 11, newSVpv((char*)a.description,0), 0);
	if (a.note)
	    hv_store(ret, "note", 4, newSVpv((char*)a.note,0), 0);
		
	free_ToDo(&a);
    }
    }
    OUTPUT:
    RETVAL

SV *
Pack(record)
    SV * record
    CODE:
    {
    SV ** s;
    HV * h;
    struct ToDo a;

    if (!SvRV(record) || (SvTYPE(h=(HV*)SvRV(record))!=SVt_PVHV))
    	RETVAL = record;
    else if ((s=hv_fetch(h, "deleted", 7, 0)) && SvOK(*s) && SvTRUE(*s) &&
    		(s=hv_fetch(h, "archived", 8, 0)) && SvOK(*s) && !SvTRUE(*s)) {
    	RETVAL = newSVpv("",0);
	    hv_store(h, "raw", 3, SvREFCNT_inc(RETVAL), 0);
    }
 	else {

    a.priority = (s = hv_fetch(h, "priority", 8, 0)) ? SvIV(*s) : 0;
    a.complete = (s = hv_fetch(h, "complete", 8, 0)) ? SvIV(*s) : 0;
    if ((s = hv_fetch(h, "due", 3, 0))  && SvOK(*s) && SvRV(*s) && (SvTYPE(SvRV(*s))==SVt_PVAV)) {
    	avtotm((AV*)SvRV(*s), &a.due);
    	a.indefinite = 0;
    }
    else {
    	memset(&a.due,'\0', sizeof(struct tm));
    	a.indefinite = 1;
    }
    
    a.description = (s = hv_fetch(h, "description", 11, 0)) ? SvPV(*s,na) : 0;
    a.note = (s = hv_fetch(h, "note", 4, 0)) ? SvPV(*s,na) : 0;

    if (pack_ToDo(&a, &pibuf, todo_v1) < 0) {
	croak("pack_ToDo failed");
    }
    
    RETVAL = newSVpvn((char *) pibuf.data, pibuf.used);

    hv_store(h, "raw", 3, SvREFCNT_inc(RETVAL), 0);
    }
    }
    OUTPUT:
    RETVAL


SV *
UnpackAppBlock(record)
    SV * record
    CODE:
    {
    STRLEN len;
    AV * e;
    HV * ret;
    int i;
    struct ToDoAppInfo a;

    if (SvOK(record) && SvRV(record) && (SvTYPE(SvRV(record)) == SVt_PVHV)) {
    	SV ** raw;
    	ret = (HV*)SvRV(record);
    	raw = hv_fetch(ret, "raw", 3, 0);
    	if (!raw || !SvOK(*raw))
    		croak("Unable to unpack");
    	RETVAL = newSVsv(record);
    	record = *raw;
    } else {
    	ret = newHV();
    	hv_store(ret, "raw", 3, newSVsv(record),0);
    	RETVAL = newRV_noinc((SV*)ret);
    }
    
    SvPV(record,len);
    if (unpack_ToDoAppInfo(&a, (CPTR)SvPV(record, na), len)>0) {

	    doUnpackCategory(ret, &a.category);

	    hv_store(ret, "dirty", 5, newSViv(a.dirty), 0);

	    hv_store(ret, "sortByPriority", 14, newSViv(a.sortByPriority), 0);
	}
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
    	RETVAL = record;
    else {

    doUnpackCategory(h, &a.category);
    

	doPackCategory(h, &a.category);

    a.dirty = (s = hv_fetch(h, "dirty", 5, 0)) ? SvIV(*s) : 0;
    a.sortByPriority = (s = hv_fetch(h, "sortByPriority", 14, 0)) ? SvIV(*s) : 0;

    len = pack_ToDoAppInfo(&a, mybuf, 0xffff);

    RETVAL = newSVpvn((char *) mybuf, len);

    hv_store(h, "raw", 3, SvREFCNT_inc(RETVAL), 0);
    }
    }
    OUTPUT:
    RETVAL

MODULE = PDA::Pilot		PACKAGE = PDA::Pilot::Address

SV *
Unpack(record)
    SV * record
    CODE:
    {
    STRLEN len;
    int i;
    AV * e;
    HV * ret;
    struct Address a;
    char *str;

    if (SvOK(record) && SvRV(record) && (SvTYPE(SvRV(record)) == SVt_PVHV)) {
    	SV ** raw;
    	ret = (HV*)SvRV(record);
    	raw = hv_fetch(ret, "raw", 3, 0);
    	if (!raw || !SvOK(*raw))
    		croak("Unable to unpack");
    	RETVAL = newSVsv(record);
    	record = *raw;
    } else {
    	ret = newHV();
    	hv_store(ret, "raw", 3, newSVsv(record),0);
    	RETVAL = newRV_noinc((SV*)ret);
    }
    
    str = SvPV(record,len);
    if (len > 0) { /* len == 0 when deleted flag is set */
	pi_buffer_clear(&pibuf);
	if (!pi_buffer_append(&pibuf, str, len)) {
	    croak("Unable to reallocate buffer");
	}
	if (unpack_Address(&a, &pibuf, address_v1) < 0) {
	    croak("unpack_Address failed");
	}

	e = newAV();
	hv_store(ret, "phoneLabel", 10, newRV_noinc((SV*)e), 0);
    
	for (i=0;i<5;i++) {
	    av_push(e, newSViv(a.phoneLabel[i]));
	}
	
	e = newAV();
	hv_store(ret, "entry", 5, newRV_noinc((SV*)e), 0);
	
	for (i=0;i<19;i++) {
	    av_push(e, a.entry[i] ? newSVpv(a.entry[i],0) : &sv_undef);
	}
	    
	hv_store(ret, "showPhone", 9, newSViv(a.showPhone), 0);
    
	free_Address(&a);
    }
    }
    OUTPUT:
    RETVAL

SV *
Pack(record)
    SV * record
    CODE:
    {
    SV ** s;
    HV * h;
    AV * av;
    int i;
    struct Address a;

    if (!SvRV(record) || (SvTYPE(h=(HV*)SvRV(record))!=SVt_PVHV))
    	RETVAL = record;
    else if ((s=hv_fetch(h, "deleted", 7, 0)) && SvOK(*s) && SvTRUE(*s) &&
    		(s=hv_fetch(h, "archived", 8, 0)) && SvOK(*s) && !SvTRUE(*s)) {
    	RETVAL = newSVpv("",0);
	    hv_store(h, "raw", 3, SvREFCNT_inc(RETVAL), 0);
    }
    else {

    if ((s = hv_fetch(h, "phoneLabel", 10, 0)) && SvOK(*s) && SvRV(*s) && (SvTYPE(av=(AV*)SvRV(*s))==SVt_PVAV))
    	for (i=0;i<5;i++)
    		a.phoneLabel[i] = ((s=av_fetch(av, i, 0)) && SvOK(*s)) ? SvIV(*s) : 0;
	else
		for (i=0;i<5;i++)
			a.phoneLabel[i] = 0;

    if ((s = hv_fetch(h, "entry", 5, 0)) && SvOK(*s) && SvRV(*s) && (SvTYPE(av=(AV*)SvRV(*s))==SVt_PVAV))
    	for (i=0;i<19;i++)
    		a.entry[i] = ((s=av_fetch(av, i, 0)) && SvOK(*s)) ? SvPV(*s,na) : 0;
	else
		for (i=0;i<19;i++)
			a.entry[i] = 0;
	
	if ((s = hv_fetch(h, "showPhone", 9, 0)))
	  a.showPhone = SvIV(*s);
	else
	  a.showPhone = 0;

    if (pack_Address(&a, &pibuf, address_v1) < 0) {
	croak("pack_Address failed");
    }
    
    RETVAL = newSVpvn((char *) pibuf.data, pibuf.used);

    hv_store(h, "raw", 3, SvREFCNT_inc(RETVAL), 0);
    }
    }
    OUTPUT:
    RETVAL


SV *
UnpackAppBlock(record)
    SV * record
    CODE:
    {
    STRLEN len;
    AV * e;
    HV * ret;
    int i;
    struct AddressAppInfo a;

    if (SvOK(record) && SvRV(record) && (SvTYPE(SvRV(record)) == SVt_PVHV)) {
    	SV ** raw;
    	ret = (HV*)SvRV(record);
    	raw = hv_fetch(ret, "raw", 3, 0);
    	if (!raw || !SvOK(*raw))
    		croak("Unable to unpack");
    	RETVAL = newSVsv(record);
    	record = *raw;
    } else {
    	ret = newHV();
    	hv_store(ret, "raw", 3, newSVsv(record),0);
    	RETVAL = newRV_noinc((SV*)ret);
    }
    
    SvPV(record,len);
    if (unpack_AddressAppInfo(&a, (CPTR)SvPV(record, na), len)>0) {
    
	    doUnpackCategory(ret, &a.category);
	    
	    e = newAV();
	    hv_store(ret, "labelRenamed", 12, newRV_noinc((SV*)e), 0);
	    
	    for (i=0;i<22;i++) {
	    	av_push(e, newSViv(a.labelRenamed[i]));
	    }
	
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
	    	av_push(e, newSVpv(a.phoneLabels[i],0));
	    }

	}
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
    	RETVAL = record;
    else {

    doPackCategory(h, &a.category);
    
    if ((s = hv_fetch(h, "labelRenamed", 12, 0)) && SvOK(*s) && SvRV(*s) && (SvTYPE(av=(AV*)SvRV(*s))==SVt_PVAV))
    	for (i=0;i<22;i++) a.labelRenamed[i] = (s=av_fetch(av, i, 0)) ? SvIV(*s) : 0;
	else
		for (i=0;i<22;i++) a.labelRenamed[i] = 0;
		
    a.country = (s = hv_fetch(h, "country", 7, 0)) ? SvIV(*s) : 0;
    a.sortByCompany = (s = hv_fetch(h, "sortByCompany", 13, 0)) ? SvIV(*s) : 0;

    if ((s = hv_fetch(h, "label", 5, 0)) && SvOK(*s) && SvRV(*s) && (SvTYPE(av=(AV*)SvRV(*s))==SVt_PVAV))
    	for (i=0;i<22;i++) strncpy(a.labels[i], (s=av_fetch(av, i, 0)) ? SvPV(*s,na) : "", 16);
	else
		for (i=0;i<22;i++) a.labels[i][0] = 0;
	for (i=0;i<22;i++) a.labels[i][15] = 0;

    if ((s = hv_fetch(h, "phoneLabel", 10, 0)) && SvOK(*s) && SvRV(*s) && (SvTYPE(av=(AV*)SvRV(*s))==SVt_PVAV))
    	for (i=0;i<8;i++) strncpy(a.phoneLabels[i], (s=av_fetch(av, i, 0)) ? SvPV(*s,na) : "", 16);
	else
		for (i=0;i<8;i++) a.phoneLabels[i][0] = 0;
	for (i=0;i<8;i++) a.phoneLabels[i][15] = 0;

    len = pack_AddressAppInfo(&a, mybuf, 0xffff);

    RETVAL = newSVpvn((char *) mybuf, len);

    hv_store(h, "raw", 3, SvREFCNT_inc(RETVAL), 0);
    }
    }
    OUTPUT:
    RETVAL

MODULE = PDA::Pilot		PACKAGE = PDA::Pilot::Memo

SV *
Unpack(record)
    SV * record
    CODE:
    {
    STRLEN len;
    int i;
    AV * e;
    HV * ret;
    struct Memo a;
    char *str;

    if (SvOK(record) && SvRV(record) && (SvTYPE(SvRV(record)) == SVt_PVHV)) {
    	SV ** raw;
    	ret = (HV*)SvRV(record);
    	raw = hv_fetch(ret, "raw", 3, 0);
    	if (!raw || !SvOK(*raw))
    		croak("Unable to unpack");
    	RETVAL = newSVsv(record);
    	record = *raw;
    } else {
    	ret = newHV();
    	hv_store(ret, "raw", 3, newSVsv(record),0);
    	RETVAL = newRV_noinc((SV*)ret);
    }
    
    str = SvPV(record,len);
    if (len > 0) { /* len == 0 if deleted flag is set */
	pi_buffer_clear(&pibuf);
	if (!pi_buffer_append(&pibuf, str, len)) {
	    croak("Unable to reallocate buffer");
	}
	if (unpack_Memo(&a, &pibuf, memo_v1) < 0) {
	    croak("unpack_Memo failed");
	}


	hv_store(ret, "text", 4, newSVpv(a.text,0), 0);

	free_Memo(&a);
    }
    }
    OUTPUT:
    RETVAL

SV *
Pack(record)
    SV * record
    CODE:
    {
    SV ** s;
    HV * h;
    struct Memo a;
    
    if (!SvRV(record) || (SvTYPE(h=(HV*)SvRV(record))!=SVt_PVHV))
    	RETVAL = record;
    else if ((s=hv_fetch(h, "deleted", 7, 0)) && SvOK(*s) && SvTRUE(*s) &&
    		(s=hv_fetch(h, "archived", 8, 0)) && SvOK(*s) && !SvTRUE(*s)) {
    	RETVAL = newSVpv("",0);
	    hv_store(h, "raw", 3, SvREFCNT_inc(RETVAL), 0);
    }
    else {
    
    if ((s = hv_fetch(h, "text", 4, 0)))
	    a.text = SvPV(*s,na);
	else
		a.text = 0;
    
    if (pack_Memo(&a, &pibuf, memo_v1) < 0) {
	croak("pack_Memo failed");
    }
    
    RETVAL = newSVpvn((char *) pibuf.data, pibuf.used);
    
    hv_store(h, "raw", 3, SvREFCNT_inc(RETVAL), 0);
    }
    }
    OUTPUT:
    RETVAL

SV *
UnpackAppBlock(record)
    SV * record
    CODE:
    {
    STRLEN len;
    AV * e;
    HV * ret;
    int i;
    struct MemoAppInfo a;

    if (SvOK(record) && SvRV(record) && (SvTYPE(SvRV(record)) == SVt_PVHV)) {
    	SV ** raw;
    	ret = (HV*)SvRV(record);
    	raw = hv_fetch(ret, "raw", 3, 0);
    	if (!raw || !SvOK(*raw))
    		croak("Unable to unpack");
    	RETVAL = newSVsv(record);
    	record = *raw;
    } else {
    	ret = newHV();
    	hv_store(ret, "raw", 3, newSVsv(record),0);
    	RETVAL = newRV_noinc((SV*)ret);
    }
    
    SvPV(record,len);
    if (unpack_MemoAppInfo(&a, (CPTR)SvPV(record, na), len)>0) {

	    doUnpackCategory(ret, &a.category);

	    hv_store(ret, "sortByAlpha", 11, newSViv(a.sortByAlpha), 0);
	}
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
    	RETVAL = record;
    else {

    doPackCategory(h, &a.category);
    
		
    if ((s = hv_fetch(h, "sortByAlpha", 11, 0)))
	    a.sortByAlpha = SvIV(*s);
	else
		a.sortByAlpha = 0;
    
    len = pack_MemoAppInfo(&a, mybuf, 0xffff);

    RETVAL = newSVpvn((char *) mybuf, len);

    hv_store(h, "raw", 3, SvREFCNT_inc(RETVAL), 0);
    }
    }
    OUTPUT:
    RETVAL

MODULE = PDA::Pilot		PACKAGE = PDA::Pilot::Expense

SV *
Unpack(record)
    SV * record
    CODE:
    {
    STRLEN len;
    int i;
    HV * ret;
    struct Expense e;

    if (SvOK(record) && SvRV(record) && (SvTYPE(SvRV(record)) == SVt_PVHV)) {
    	SV ** raw;
    	ret = (HV*)SvRV(record);
    	raw = hv_fetch(ret, "raw", 3, 0);
    	if (!raw || !SvOK(*raw))
    		croak("Unable to unpack");
    	RETVAL = newSVsv(record);
    	record = *raw;
    } else {
    	ret = newHV();
    	hv_store(ret, "raw", 3, newSVsv(record),0);
    	RETVAL = newRV_noinc((SV*)ret);
    }
    
    SvPV(record,len);
    if (len > 0) { /* len == 0 if deleted flag is set */
	if (unpack_Expense(&e, (CPTR)SvPV(record, na), len)>0) {

	    hv_store(ret, "date", 4, newRV_noinc((SV*)tmtoav(&e.date)), 0);
	    hv_store(ret, "type", 4, newSVlist(e.type,ExpenseTypeNames),0);
	    hv_store(ret, "payment", 7, newSVlist(e.payment,ExpensePaymentNames),0);
	    hv_store(ret, "currency", 8, newSViv(e.currency),0);
	    if (e.amount)
		hv_store(ret, "amount", 6, newSVpv(e.amount,0), 0);
	    if (e.vendor)
		hv_store(ret, "vendor", 6, newSVpv(e.vendor,0), 0);
	    if (e.city)
		hv_store(ret, "city", 4, newSVpv(e.city,0), 0);
	    if (e.note)
		hv_store(ret, "note", 4, newSVpv(e.note,0), 0);
	    if (e.attendees)
	    	hv_store(ret, "attendees", 9, newSVpv(e.attendees,0), 0);

	    free_Expense(&e);
	}
    }
    }
    OUTPUT:
    RETVAL

SV *
Pack(record)
    SV * record
    CODE:
    {
    STRLEN len;
    SV ** s;
    HV * h;
    struct Expense e;
    
    if (!SvRV(record) || (SvTYPE(h=(HV*)SvRV(record))!=SVt_PVHV))
    	RETVAL = record;
    else if ((s=hv_fetch(h, "deleted", 7, 0)) && SvOK(*s) && SvTRUE(*s) &&
    		(s=hv_fetch(h, "archived", 8, 0)) && SvOK(*s) && !SvTRUE(*s)) {
    	RETVAL = newSVpv("",0);
	    hv_store(h, "raw", 3, SvREFCNT_inc(RETVAL), 0);
    }
    else {

    if ((s = hv_fetch(h, "type", 4, 0)))
    	e.type = SvList(*s,ExpenseTypeNames);
    else
    	croak("must have type");
    if ((s = hv_fetch(h, "payment", 7, 0)))
    	e.payment = SvList(*s,ExpensePaymentNames);
    else
    	croak("must have payment");
    if ((s = hv_fetch(h, "currency", 8, 0)))
    	e.currency = SvIV(*s);
    else
    	croak("must have currency");
    
    if ((s = hv_fetch(h, "date", 4, 0)) && SvOK(*s) && SvRV(*s) && (SvTYPE(SvRV(*s))==SVt_PVAV))
    	avtotm((AV*)SvRV(*s), &e.date);
    else
    	croak("expense record must contain date");
    if ((s = hv_fetch(h, "amount", 6, 0))) e.amount = SvPV(*s,na);
	else e.amount = 0;
    if ((s = hv_fetch(h, "vendor", 6, 0))) e.vendor = SvPV(*s,na);
	else e.vendor = 0;
    if ((s = hv_fetch(h, "city", 4, 0))) e.city = SvPV(*s,na);
	else e.city = 0;
    if ((s = hv_fetch(h, "attendess", 9, 0))) e.attendees = SvPV(*s,na);
	else e.attendees = 0;
    if ((s = hv_fetch(h, "note", 4, 0))) e.note = SvPV(*s,na);
	else e.note = 0;
    
    len = pack_Expense(&e, mybuf, 0xffff);
    
    RETVAL = newSVpvn((char *) mybuf, len);
    
    hv_store(h, "raw", 3, SvREFCNT_inc(RETVAL), 0);


    }
    }
    OUTPUT:
    RETVAL

SV *
UnpackAppBlock(record)
    SV * record
    CODE:
    {
    STRLEN len;
    HV * ret;
    AV * a;
    int i;
    struct ExpenseAppInfo e;

    if (SvOK(record) && SvRV(record) && (SvTYPE(SvRV(record)) == SVt_PVHV)) {
    	SV ** raw;
    	ret = (HV*)SvRV(record);
    	raw = hv_fetch(ret, "raw", 3, 0);
    	if (!raw || !SvOK(*raw))
    		croak("Unable to unpack");
    	RETVAL = newSVsv(record);
    	record = *raw;
    } else {
    	ret = newHV();
    	hv_store(ret, "raw", 3, newSVsv(record),0);
    	RETVAL = newRV_noinc((SV*)ret);
    }
    
    SvPV(record,len);
    if (unpack_ExpenseAppInfo(&e, (CPTR)SvPV(record, na), len)>0) {

		hv_store(ret, "sortOrder", 9, newSVlist(e.sortOrder,ExpenseSortNames),0);
		a = newAV();
		hv_store(ret, "currencies", 10, newRV_noinc((SV*)a), 0);
		for (i=0;i<4;i++) {
			HV * h = newHV();
			hv_store(h, "name", 4, newSVpv(e.currencies[i].name, 0), 0);
			hv_store(h, "symbol", 6, newSVpv(e.currencies[i].symbol, 0), 0);
			hv_store(h, "rate", 4, newSVpv(e.currencies[i].rate, 0), 0);
			av_store(a, i, (SV*)newRV_noinc((SV*)h));
		}

	    doUnpackCategory(ret, &e.category);
	}
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
    struct ExpenseAppInfo e;
    
    if (!SvRV(record) || (SvTYPE(h=(HV*)SvRV(record))!=SVt_PVHV))
    	RETVAL = record;
    else {

    doPackCategory(h, &e.category);

	e.sortOrder = (s = hv_fetch(h, "sortOrder", 9, 0)) ? SvList(*s, ExpenseSortNames) : 0;
	if ((s=hv_fetch(h, "currencies", 10, 0)) && SvOK(*s) && SvRV(*s) && (SvTYPE(av=(AV*)SvRV(*s))==SVt_PVAV)) {
		for(i=0;i<4;i++) {
			HV * hv;
			if ((s=av_fetch(av, i, 0)) && SvOK(*s) && SvRV(*s) && (SvTYPE(hv=(HV*)SvRV(*s))==SVt_PVHV)) {
				if (s = hv_fetch(hv, "name", 4, 0)) {
					strncpy(e.currencies[i].name, SvPV(*s, na), 16);
					e.currencies[i].name[15] = 0;
				}
				if (s = hv_fetch(hv, "symbol", 6, 0)) {
					strncpy(e.currencies[i].symbol, SvPV(*s, na), 4);
					e.currencies[i].symbol[3] = 0;
				}
				if (s = hv_fetch(hv, "rate", 4, 0)) {
					strncpy(e.currencies[i].rate, SvPV(*s, na), 8);
					e.currencies[i].rate[7] = 0;
				}
			}
		}
	} else
		for(i=0;i<4;i++) {
			e.currencies[i].symbol[0] = 0;
			e.currencies[i].name[0] = 0;
			e.currencies[i].rate[0] = 0;
		}
	
    len = pack_ExpenseAppInfo(&e, mybuf, 0xffff);

    RETVAL = newSVpvn((char *) mybuf, len);

    hv_store(h, "raw", 3, SvREFCNT_inc(RETVAL), 0);
    }
    }
    OUTPUT:
    RETVAL

SV *
UnpackPref(record)
    SV * record
    CODE:
    {
    STRLEN len;
    AV * e;
    HV * ret;
    int i;
    struct ExpensePref a;

    if (SvOK(record) && SvRV(record) && (SvTYPE(SvRV(record)) == SVt_PVHV)) {
    	SV ** raw;
    	ret = (HV*)SvRV(record);
    	raw = hv_fetch(ret, "raw", 3, 0);
    	if (!raw || !SvOK(*raw))
    		croak("Unable to unpack");
    	RETVAL = newSVsv(record);
    	record = *raw;
    } else {
    	ret = newHV();
    	hv_store(ret, "raw", 3, newSVsv(record),0);
    	RETVAL = newRV_noinc((SV*)ret);
    }
    
    SvPV(record,len);
    if (unpack_ExpensePref(&a, (CPTR)SvPV(record, na), len)>0) {

	    hv_store(ret, "unitOfDistance", 14, newSVlist(a.unitOfDistance, ExpenseDistanceNames), 0);
	    hv_store(ret, "currentCategory", 15, newSViv(a.currentCategory), 0);
	    hv_store(ret, "defaultCurrency", 15, newSViv(a.defaultCurrency), 0);
	    hv_store(ret, "attendeeFont", 8, newSViv(a.attendeeFont), 0);
	    hv_store(ret, "showAllCategories", 17, newSViv(a.showAllCategories), 0);
	    hv_store(ret, "showCurrency", 12, newSViv(a.showCurrency), 0);
	    hv_store(ret, "saveBackup", 10, newSViv(a.saveBackup), 0);
	    hv_store(ret, "allowQuickFill", 14, newSViv(a.allowQuickFill), 0);
	    e = newAV();
	    for (i=0;i<5;i++)
			av_store(e, i, newSViv(a.currencies[i]));
		hv_store(ret, "currencies", 10, (SV*)newRV_noinc((SV*)e), 0);
	    hv_store(ret, "noteFont", 8, newSViv(a.noteFont), 0);
	}
    }
    OUTPUT:
    RETVAL

SV *
PackPref(record, id)
    SV * record
    int	id
    CODE:
    {
    int i;
    int len;
    SV ** s;
    HV * h;
    AV * av;
    struct ExpensePref a;
    
    if (!SvRV(record) || (SvTYPE(h=(HV*)SvRV(record))!=SVt_PVHV))
    	RETVAL = record;
    else {

	a.unitOfDistance = (s = hv_fetch(h, "unitOfDistance", 14, 0)) ? SvList(*s, ExpenseDistanceNames) : 0;
	a.currentCategory = (s=hv_fetch(h,"currentCategory",15,0)) ? SvIV(*s) : 0;
	a.defaultCurrency = (s=hv_fetch(h,"defaultCurrency",15,0)) ? SvIV(*s) : 0;
	a.attendeeFont = (s=hv_fetch(h,"attendeeFont",8,0)) ? SvIV(*s) : 0;
	a.showAllCategories = (s=hv_fetch(h,"showAllCategories",17,0)) ? SvIV(*s) : 0;
	a.showCurrency = (s=hv_fetch(h,"showCurrency",12,0)) ? SvIV(*s) : 0;
	a.saveBackup = (s=hv_fetch(h,"saveBackup",10,0)) ? SvIV(*s) : 0;
	a.allowQuickFill = (s=hv_fetch(h,"allowQuickFill",14,0)) ? SvIV(*s) : 0;
	
	if ((s=hv_fetch(h, "currencies", 10, 0)) && SvOK(*s) && SvRV(*s) && (SvTYPE(av=(AV*)SvRV(*s))==SVt_PVAV)) {
		for(i=0;i<5;i++)
			a.currencies[i] = (s=av_fetch(av, i, 0)) ? SvIV(*s) : 0;
	} else
		for(i=0;i<5;i++)
			a.currencies[i] = 0;
	a.noteFont = (s=hv_fetch(h,"noteFont",8,0)) ? SvIV(*s) : 0;
		
    len = pack_ExpensePref(&a, mybuf, 0xffff);

    RETVAL = newSVpvn((char *) mybuf, len);

    hv_store(h, "raw", 3, SvREFCNT_inc(RETVAL), 0);
    }
    }
    OUTPUT:
    RETVAL

MODULE = PDA::Pilot		PACKAGE = PDA::Pilot::Mail

SV *
Unpack(record)
    SV * record
    CODE:
    {
    STRLEN len;
    int i;
    AV * e;
    HV * ret;
    struct Mail a;

    if (SvOK(record) && SvRV(record) && (SvTYPE(SvRV(record)) == SVt_PVHV)) {
    	SV ** raw;
    	ret = (HV*)SvRV(record);
    	raw = hv_fetch(ret, "raw", 3, 0);
    	if (!raw || !SvOK(*raw))
    		croak("Unable to unpack");
    	RETVAL = newSVsv(record);
    	record = *raw;
    } else {
    	ret = newHV();
    	hv_store(ret, "raw", 3, newSVsv(record),0);
    	RETVAL = newRV_noinc((SV*)ret);
    }
    
    SvPV(record,len);
    if (len > 0) { /* len == 0 if deleted flag is set */
	if (unpack_Mail(&a, (CPTR)SvPV(record, na), len)>0) {
    
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
	}
    }
    }
    OUTPUT:
    RETVAL

SV *
Pack(record)
    SV * record
    CODE:
    {
    STRLEN len;
    SV ** s;
    HV * h;
    struct Mail a;
    
    if (!SvRV(record) || (SvTYPE(h=(HV*)SvRV(record))!=SVt_PVHV))
    	RETVAL = record;
    else if ((s=hv_fetch(h, "deleted", 7, 0)) && SvOK(*s) && SvTRUE(*s) &&
    		(s=hv_fetch(h, "archived", 8, 0)) && SvOK(*s) && !SvTRUE(*s)) {
    	RETVAL = newSVpv("",0);
	    hv_store(h, "raw", 3, SvREFCNT_inc(RETVAL), 0);
    }
    else {
    
    a.subject = (s = hv_fetch(h, "subject", 7, 0)) ? SvPV(*s,na) : 0;
    a.from = (s = hv_fetch(h, "from", 4, 0)) ? SvPV(*s,na) : 0;
    a.to = (s = hv_fetch(h, "to", 2, 0)) ? SvPV(*s,na) : 0;
    a.cc = (s = hv_fetch(h, "cc", 2, 0)) ? SvPV(*s,na) : 0;
    a.bcc = (s = hv_fetch(h, "bcc", 3, 0)) ? SvPV(*s,na) : 0;
    a.replyTo = (s = hv_fetch(h, "replyTo", 7, 0)) ? SvPV(*s,na) : 0;
    a.sentTo = (s = hv_fetch(h, "sentTo", 6, 0)) ? SvPV(*s,na) : 0;
    a.body = (s = hv_fetch(h, "body", 4, 0)) ? SvPV(*s,na) : 0;
    
    a.read = (s = hv_fetch(h, "read", 4, 0)) ? SvIV(*s) : 0;
    a.signature = (s = hv_fetch(h, "signature", 9, 0)) ? SvIV(*s) : 0;
    a.confirmRead = (s = hv_fetch(h, "confirmRead", 11, 0)) ? SvIV(*s) : 0;
    a.confirmDelivery = (s = hv_fetch(h, "confirmDelivery", 15, 0)) ? SvIV(*s) : 0;
    a.priority = (s = hv_fetch(h, "priority", 8, 0)) ? SvIV(*s) : 0;
    a.addressing = (s = hv_fetch(h, "addressing", 10, 0)) ? SvIV(*s) : 0;
    
    a.dated = (s = hv_fetch(h, "date", 4, 0)) ? 1 : 0;
    if (s && SvOK(*s) && SvRV(*s) && (SvTYPE(SvRV(*s))==SVt_PVAV)) avtotm((AV*)SvRV(*s), &a.date);

    len = pack_Mail(&a, mybuf, 0xffff);
    
    RETVAL = newSVpvn((char *) mybuf, len);

    hv_store(h, "raw", 3, SvREFCNT_inc(RETVAL), 0);
    }
    }
    OUTPUT:
    RETVAL

SV *
UnpackAppBlock(record)
    SV * record
    CODE:
    {
    STRLEN len;
    AV * e;
    HV * ret;
    int i;
    struct MailAppInfo a;

    if (SvOK(record) && SvRV(record) && (SvTYPE(SvRV(record)) == SVt_PVHV)) {
    	SV ** raw;
    	ret = (HV*)SvRV(record);
    	raw = hv_fetch(ret, "raw", 3, 0);
    	if (!raw || !SvOK(*raw))
    		croak("Unable to unpack");
    	RETVAL = newSVsv(record);
    	record = *raw;
    } else {
    	ret = newHV();
    	hv_store(ret, "raw", 3, newSVsv(record),0);
    	RETVAL = newRV_noinc((SV*)ret);
    }
    
    SvPV(record,len);
    if (unpack_MailAppInfo(&a, (CPTR)SvPV(record, na), len)>0) {

	    doUnpackCategory(ret, &a.category);

	    hv_store(ret, "sortOrder", 9, newSVlist(a.sortOrder, MailSortTypeNames), 0);

	    hv_store(ret, "dirty", 5, newSViv(a.dirty), 0);
	    hv_store(ret, "unsentMessage", 13, newSViv(a.unsentMessage), 0);
	}
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
    	RETVAL = record;
    else {


    doPackCategory(h, &a.category);
    
    if ((s = hv_fetch(h, "sortOrder", 9, 0)))
	    a.sortOrder = SvList(*s, MailSortTypeNames);
	else
		a.sortOrder = 0;

	a.dirty = (s=hv_fetch(h,"dirty",5,0)) ? SvIV(*s) : 0;
	a.unsentMessage = (s=hv_fetch(h,"unsentMessage",13,0)) ? SvIV(*s) : 0;

    len = pack_MailAppInfo(&a, mybuf, 0xffff);

    RETVAL = newSVpvn((char *) mybuf, len);

    hv_store(h, "raw", 3, SvREFCNT_inc(RETVAL), 0);
    }
    }
    OUTPUT:
    RETVAL

SV *
UnpackSyncPref(record)
    SV * record
    CODE:
    {
    STRLEN len;
    AV * e;
    HV * ret;
    int i;
    struct MailSyncPref a;

    if (SvOK(record) && SvRV(record) && (SvTYPE(SvRV(record)) == SVt_PVHV)) {
    	SV ** raw;
    	ret = (HV*)SvRV(record);
    	raw = hv_fetch(ret, "raw", 3, 0);
    	if (!raw || !SvOK(*raw))
    		croak("Unable to unpack");
    	RETVAL = newSVsv(record);
    	record = *raw;
    } else {
    	ret = newHV();
    	hv_store(ret, "raw", 3, newSVsv(record),0);
    	RETVAL = newRV_noinc((SV*)ret);
    }
    
    SvPV(record,len);
    if (unpack_MailSyncPref(&a, (CPTR)SvPV(record, na), len)>0) {

	    hv_store(ret, "syncType", 8, newSVlist(a.syncType, MailSyncTypeNames), 0);
	    hv_store(ret, "getHigh", 7, newSViv(a.getHigh), 0);
	    hv_store(ret, "getContaining", 13, newSViv(a.getContaining), 0);
	    hv_store(ret, "truncate", 8, newSViv(a.truncate), 0);
  
	    if (a.filterTo)  
		    hv_store(ret, "filterTo", 8, newSVpv(a.filterTo, 0), 0);
	    if (a.filterFrom)  
		    hv_store(ret, "filterFrom", 10, newSVpv(a.filterFrom, 0), 0);
	    if (a.filterSubject)  
	 	   hv_store(ret, "filterSubject", 13, newSVpv(a.filterSubject, 0), 0);
	}
    }
    OUTPUT:
    RETVAL

SV *
PackSyncPref(record, id)
    SV * record
    int	id
    CODE:
    {
    int i;
    int len;
    SV ** s;
    HV * h;
    AV * av;
    struct MailSyncPref a;
    
    if (!SvRV(record) || (SvTYPE(h=(HV*)SvRV(record))!=SVt_PVHV))
    	RETVAL = record;
    else {

    if ((s = hv_fetch(h, "syncType", 8, 0)))
	    a.syncType = SvList(*s, MailSyncTypeNames);
	else
		a.syncType = 0;

	a.getHigh = (s=hv_fetch(h,"getHigh",7,0)) ? SvIV(*s) : 0;
	a.getContaining = (s=hv_fetch(h,"getContaining",13,0)) ? SvIV(*s) : 0;
	a.truncate = (s=hv_fetch(h,"truncate",8,0)) ? SvIV(*s) : 0;

	a.filterTo = (s=hv_fetch(h,"filterTo",8,0)) ? SvPV(*s,na) : 0;
	a.filterFrom = (s=hv_fetch(h,"filterFrom",10,0)) ? SvPV(*s,na) : 0;
	a.filterSubject = (s=hv_fetch(h,"filterSubject",13,0)) ? SvPV(*s,na) : 0;

    len = pack_MailSyncPref(&a, mybuf, 0xffff);

    RETVAL = newSVpvn((char *) mybuf, len);

    hv_store(h, "raw", 3, SvREFCNT_inc(RETVAL), 0);
    }
    }
    OUTPUT:
    RETVAL

SV *
UnpackSignaturePref(record)
    SV * record
    CODE:
    {
    STRLEN len;
    AV * e;
    HV * ret;
    int i;
    struct MailSignaturePref a;

    if (SvOK(record) && SvRV(record) && (SvTYPE(SvRV(record)) == SVt_PVHV)) {
    	SV ** raw;
    	ret = (HV*)SvRV(record);
    	raw = hv_fetch(ret, "raw", 3, 0);
    	if (!raw || !SvOK(*raw))
    		croak("Unable to unpack");
    	RETVAL = newSVsv(record);
    	record = *raw;
    } else {
    	ret = newHV();
    	hv_store(ret, "raw", 3, newSVsv(record),0);
    	RETVAL = newRV_noinc((SV*)ret);
    }
    
    SvPV(record,len);
    if (unpack_MailSignaturePref(&a, (CPTR)SvPV(record, na), len)>0) {
  
	    if (a.signature)  
		    hv_store(ret, "signature", 9, newSVpv(a.signature, 0), 0);
	}
    }
    OUTPUT:
    RETVAL

SV *
PackSignaturePref(record, id)
    SV * record
    int	id
    CODE:
    {
    int i;
    int len;
    SV ** s;
    HV * h;
    AV * av;
    struct MailSignaturePref a;
    
    if (!SvRV(record) || (SvTYPE(h=(HV*)SvRV(record))!=SVt_PVHV))
    	RETVAL = record;
    else {

	a.signature = (s=hv_fetch(h,"signature",9,0)) ? SvPV(*s,na) : 0;

    len = pack_MailSignaturePref(&a, mybuf, 0xffff);

    RETVAL = newSVpvn((char *) mybuf, len);

    hv_store(h, "raw", 3, SvREFCNT_inc(RETVAL), 0);
    }
    }
    OUTPUT:
    RETVAL

MODULE = PDA::Pilot		PACKAGE = PDA::Pilot

int
close(socket)
	int	socket
	CODE:
	RETVAL = pi_close(socket);
	OUTPUT:
	RETVAL

int
write(socket, msg)
	int	socket
	SV *	msg
	CODE:
	{
	    STRLEN len;
	    SvPV(msg, len);
		RETVAL = pi_write(socket,SvPV(msg,na),len);
	}

SV *
read(socket, len)
	int	socket
	int	len


	CODE:
	{
	    int result;

	    result = pi_read(socket, &pibuf, len);
	    if (result >=0) 
	    	RETVAL = newSVpvn((char *) pibuf.data, result);
	    else
	    	RETVAL = &sv_undef;
	}
	OUTPUT:
	RETVAL

int
socket(domain, type, protocol)
	int	domain
	int	type
	int	protocol
	CODE:
	RETVAL = pi_socket(domain, type, protocol);
	OUTPUT:
	RETVAL

int
listen(socket, backlog)
	int	socket
	int	backlog
	CODE:
	RETVAL = pi_listen(socket, backlog);
	OUTPUT:
	RETVAL

char *
errorText(error)
	int error
	CODE:
	RETVAL = dlp_strerror(error);
	OUTPUT:
	RETVAL

int
bind(socket, port)
	int	socket
	char *  port
	CODE:
	RETVAL = pi_bind(socket, port);
	OUTPUT:
	RETVAL

int
openPort(port)
	char *	port
	CODE:
	{
		int socket = pi_socket(PI_AF_PILOT, PI_SOCK_STREAM, PI_PF_DLP);
		pi_bind(socket, port);
		pi_listen(socket, 1);
		RETVAL = socket;
	}
	OUTPUT:
	RETVAL

void
accept(socket)
	int	socket
	PPCODE:
	{
		struct pi_sockaddr a;
		size_t len = sizeof(struct pi_sockaddr);
		int result;
		result = pi_accept(socket, (struct sockaddr*)&a, &len);
		EXTEND(SP, 1);
		if (result < 0) {
			PUSHs(sv_newmortal());
		} else {
			PDA__Pilot__DLP * x = malloc(sizeof(PDA__Pilot__DLP));
			SV * sv = newSViv((IV)(void*)x);
			x->errnop = 0;
			x->socket = result;
			SV * rv = newRV_noinc(sv);
			sv_bless(rv, gv_stashpv("PDA::Pilot::DLPPtr",0));
			PUSHs(sv_2mortal(rv));
		}
		/* In list context, return error code as a second value */
		if (GIMME_V == G_ARRAY) {
		        EXTEND(SP, 1);
			if (result < 0) {
				PUSHs(sv_2mortal(newSViv(result)));
			} else {
				PUSHs(sv_newmortal());
			}
		}
			
	}

MODULE = PDA::Pilot		PACKAGE = PDA::Pilot::DLP::DBPtr

void
DESTROY(db)
	PDA::Pilot::DLP::DB *	db
	CODE:
	if (db->Class)
		SvREFCNT_dec(db->Class);
	if (db->handle)
		dlp_CloseDB(db->socket, db->handle);
	if (db->dbname)
		SvREFCNT_dec(db->dbname);
	SvREFCNT_dec(db->connection);
	free(db);

int
errno(self)
	PDA::Pilot::DLP::DB *self
	CODE:
		RETVAL = self->errnop;
		self->errnop = 0;
	OUTPUT:
	RETVAL

int
palmos_errno(self)
        PDA::Pilot::DLP::DB *self
	CODE:
		RETVAL = pi_palmos_error(self->socket);
	OUTPUT:
	RETVAL

MODULE = PDA::Pilot		PACKAGE = PDA::Pilot::DLP::DBPtr

SV *
class(self, name=0)
	PDA::Pilot::DLP::DB *self
	SV *	name
	CODE:
	{
		SV ** s = 0;
		HV * h;
		if (name) {
			STRLEN len;
			h = perl_get_hv("PDA::Pilot::DBClasses", 0);
			if (!h)
				croak("DBClasses doesn't exist");
			if (SvOK(name)) {
				(void)SvPV(name,len);
				s = hv_fetch(h, SvPV(name,na), len, 0);
			}
			if (!s)
				s = hv_fetch(h, "", 0, 0);
			if (!s)
				croak("Default DBClass not defined");
			SvREFCNT_inc(*s);
			if (self->Class)
				SvREFCNT_dec(self->Class);
			self->Class = *s;
		}
		RETVAL = newSVsv(self->Class);
	}
	OUTPUT:
	RETVAL

Result
close(self)
	PDA::Pilot::DLP::DB *self
	CODE:
	RETVAL = dlp_CloseDB(self->socket, self->handle);
	self->handle=0;
	OUTPUT:
	RETVAL

Result
setSortBlock(self, data)
	PDA::Pilot::DLP::DB *self
	SV *data
	CODE:
	{
		STRLEN len;
		void * c;
		PackSI;
		c = SvPV(data, len);
		RETVAL = dlp_WriteSortBlock(self->socket, self->handle, c, len);
	}
	OUTPUT:
	RETVAL

SV *
getAppBlock(self)
	PDA::Pilot::DLP::DB *self
	PPCODE:
	{
		int result = dlp_ReadAppBlock(self->socket, self->handle, 0, -1, &pibuf);
		ReturnReadAI(pibuf.data, result);
	}

SV *
getSortBlock(self)
	PDA::Pilot::DLP::DB *self
	PPCODE:
	{
		int result = dlp_ReadSortBlock(self->socket,self->handle, 0, -1, &pibuf);
		ReturnReadSI(pibuf.data, result);
	}

Result
setAppBlock(self, data)
	PDA::Pilot::DLP::DB *self
	SV *data
	CODE:
	{
		STRLEN len;
		void * c;
		PackAI;
		c = SvPV(data, len);
		RETVAL = dlp_WriteAppBlock(self->socket, self->handle, c, len);
	}
	OUTPUT:
	RETVAL

Result
purge(self)
	PDA::Pilot::DLP::DB *self
	CODE:
	RETVAL = dlp_CleanUpDatabase(self->socket, self->handle);
	OUTPUT:
	RETVAL

Result
resetFlags(self)
	PDA::Pilot::DLP::DB *self
	CODE:
	RETVAL = dlp_ResetSyncFlags(self->socket, self->handle);
	OUTPUT:
	RETVAL

Result
deleteCategory(self, category)
	PDA::Pilot::DLP::DB *self
	int	category
	CODE:
	RETVAL = dlp_DeleteCategory(self->socket, self->handle, category);
	OUTPUT:
	RETVAL

void
newRecord(self, id=0, attr=0, cat=0)
	PDA::Pilot::DLP::DB *self
	SV *	id
	SV *	attr
	SV *	cat
	PPCODE:
	{
    	if (self->Class) {									
    		int count;										
    		PUSHMARK(sp);									
    		XPUSHs(self->Class);
    		if (id)
	    		XPUSHs(id);
	    	if (attr)
	    		XPUSHs(attr);
	    	if (cat)
	    		XPUSHs(cat);
	    	PUTBACK;										
	    	count = perl_call_method("record", G_SCALAR);
	    	SPAGAIN;										
	    	if (count != 1)									
	    		croak("Unable to create record");			
    	}													
    	else {												
    		croak("Class not defined");						
    	}													
    }

void
newResource(self, type=0, id=0)
	PDA::Pilot::DLP::DB *self
	SV *	type
	SV *	id
	PPCODE:
	{
    	if (self->Class) {									
    		int count;										
    		PUSHMARK(sp);									
    		XPUSHs(self->Class);							
    		if (type)	
    			XPUSHs(type);
    		if (id)
    			XPUSHs(id);
	    	PUTBACK;										
	    	count = perl_call_method("resource", G_SCALAR);
	    	SPAGAIN;										
	    	if (count != 1)									
	    		croak("Unable to create record");			
    	}													
    	else {												
    		croak("Class not defined");						
    	}													
    }

void
newAppBlock(self)
	PDA::Pilot::DLP::DB *self
	PPCODE:
	{
    	if (self->Class) {									
    		int count;										
    		PUSHMARK(sp);									
    		XPUSHs(self->Class);							
	    	PUTBACK;										
	    	count = perl_call_method("appblock", G_SCALAR);
	    	SPAGAIN;										
	    	if (count != 1)									
	    		croak("Unable to create record");			
    	}													
    	else {												
    		croak("Class not defined");						
    	}													
    }

void
newSortBlock(self)
	PDA::Pilot::DLP::DB *self
	PPCODE:
	{
    	if (self->Class) {									
    		int count;										
    		PUSHMARK(sp);									
    		XPUSHs(self->Class);							
	    	PUTBACK;										
	    	count = perl_call_method("sortblock", G_SCALAR);
	    	SPAGAIN;										
	    	if (count != 1)									
	    		croak("Unable to create record");			
    	}													
    	else {												
    		croak("Class not defined");						
    	}													
    }

void
newPref(self, id=0, version=0, backup=0, creator=0)
	PDA::Pilot::DLP::DB *self
	SV *	id
	SV *	version
	SV *	backup
	SV *	creator
	PPCODE:
	{
		if (!creator) {
			int count;
			PUSHMARK(sp);
			XPUSHs(self->Class);
			PUTBACK;
			count = perl_call_method("creator", G_SCALAR);
			SPAGAIN;
			if (count != 1)
				croak("Unable to get creator");
			creator = POPs;
			PUTBACK;
		}
    	if (self->Class) {									
    		int count;										
    		PUSHMARK(sp);									
    		XPUSHs(self->Class);
    		if (creator)
    			XPUSHs(creator);
    		if (id)
    			XPUSHs(id);
    		if (version)
    			XPUSHs(version);
    		if (backup)
    			XPUSHs(backup);
	    	PUTBACK;										
	    	count = perl_call_method("pref", G_SCALAR);
	    	SPAGAIN;										
	    	if (count != 1)									
	    		croak("Unable to create record");			
    	}													
    	else {												
    		croak("Class not defined");						
    	}													
    }

void
getRecord(self, index)
	PDA::Pilot::DLP::DB *self
	int	index
	PPCODE:
	{
		int attr, category;
		unsigned long id;
		int result;

		result = dlp_ReadRecordByIndex(self->socket, self->handle, index, &pibuf, &id, &attr, &category);
		ReturnReadRecord(pibuf.data, pibuf.used);
	}

Result
moveCategory(self, fromcat, tocat)
	PDA::Pilot::DLP::DB *self
	int	fromcat
	int	tocat
	CODE:
	RETVAL = dlp_MoveCategory(self->socket, self->handle, fromcat, tocat);
	OUTPUT:
	RETVAL


Result
deleteRecord(self, id)
	PDA::Pilot::DLP::DB *self
	unsigned long	id
	CODE:
	RETVAL = dlp_DeleteRecord(self->socket, self->handle, 0, id);
	OUTPUT:
	RETVAL


Result
deleteRecords(self)
	PDA::Pilot::DLP::DB *self
	CODE:
	RETVAL = dlp_DeleteRecord(self->socket, self->handle, 1, 0);
	OUTPUT:
	RETVAL

Result
resetNext(self)
	PDA::Pilot::DLP::DB *self
	CODE:
	RETVAL = dlp_ResetDBIndex(self->socket, self->handle);
	OUTPUT:
	RETVAL

int
getRecords(self)
	PDA::Pilot::DLP::DB *self
	CODE:
	{
		int result = dlp_ReadOpenDBInfo(self->socket, self->handle, &RETVAL);
		if (result < 0) {
			RETVAL = newSVsv(&sv_undef);
			self->errnop = result;
		}
	}
	OUTPUT:
	RETVAL

void
getRecordIDs(self, sort=0)
	PDA::Pilot::DLP::DB *self
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
				self->errnop = result;
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
getRecordByID(self, id)
	PDA::Pilot::DLP::DB *self
	unsigned long id
	PPCODE:
	{
	    int result, attr, category, index;

	    result = dlp_ReadRecordById(self->socket, self->handle, id, &pibuf, &index, &attr, &category);
	    ReturnReadRecord(pibuf.data, pibuf.used);
	}

void
getNextModRecord(self, category=-1)
	PDA::Pilot::DLP::DB *self
	int	category
	PPCODE:
	{
	    int result, attr, index;
	    unsigned long id;

	    if (category == -1)
	    	result = dlp_ReadNextModifiedRec(self->socket, self->handle, &pibuf, &id, &index, &attr, &category);
	    else
	    	result = dlp_ReadNextModifiedRecInCategory(self->socket, self->handle, category, &pibuf, &id, &index, &attr);
	    ReturnReadRecord(pibuf.data, pibuf.used);
	}

void
getNextRecord(self, category)
	PDA::Pilot::DLP::DB *self
	int	category
	PPCODE:
	{
	   int result, attr, index;
	    unsigned long id;

	    result = dlp_ReadNextRecInCategory(self->socket, self->handle, category, &pibuf, &id, &index, &attr);
	    ReturnReadRecord(pibuf.data, pibuf.used);
	}

unsigned long
setRecord(self, data)
	PDA::Pilot::DLP::DB *self
	SV *data
	CODE:
	{
		STRLEN len;
		unsigned long id;
		int attr, category;
		int result;
		void * c;
		PackRecord;
		c = SvPV(data, len);
		result = dlp_WriteRecord(self->socket, self->handle, attr, id, category, c, len, &RETVAL);
		if (result<0) {
			RETVAL = 0;
			self->errnop = result;
		}
	}
	OUTPUT:
	RETVAL

unsigned long
setRecordRaw(self, data, id, attr, category)
	PDA::Pilot::DLP::DB *self
	unsigned long	id
	int	attr
	int	category
	SV *data
	CODE:
	{
		STRLEN len;
		int result;
		void * c;
		PackRaw;
		c = SvPV(data, len);
		result = dlp_WriteRecord(self->socket, self->handle, attr, id, category, c, len, &RETVAL);
		if (result<0) {
			RETVAL = 0;
			self->errnop = result;
		}
	}
	OUTPUT:
	RETVAL

void
setResourceByID(self, type, id)
	PDA::Pilot::DLP::DB *self
	Char4	type
	int	id
	PPCODE:
	{
	   int result, index;

	    result = dlp_ReadResourceByType(self->socket, self->handle, type, id, &pibuf, &index);
	    ReturnReadResource(pibuf.data, pibuf.used);
	}

void
getResource(self, index)
	PDA::Pilot::DLP::DB *self
	int	index
	PPCODE:
	{
	    int result, id;
	    Char4 type;

	    result = dlp_ReadResourceByIndex(self->socket, self->handle, index, &pibuf, &type, &id);
	    ReturnReadResource(pibuf.data, pibuf.used);
	}

SV *
setResource(self, data)
	PDA::Pilot::DLP::DB *self
	SV *data
	CODE:
	{
		STRLEN len;
		int result;
		Char4 type;
		int id;
		void * c;
		PackResource;
		c = SvPV(data, len);
		result = dlp_WriteResource(self->socket, self->handle, type, id, c, len);
		if (result < 0) {
			self->errnop = result;
			RETVAL = newSVsv(&sv_undef);
		} else
			RETVAL = newSViv(result);
	}
	OUTPUT:
	RETVAL

Result
deleteResource(self, type, id)
	PDA::Pilot::DLP::DB *self
	Char4	type
	int	id
	CODE:
	RETVAL = dlp_DeleteResource(self->socket, self->handle, 0, type, id);
	OUTPUT:
	RETVAL

Result
deleteResources(self)
	PDA::Pilot::DLP::DB *self
	CODE:
	RETVAL = dlp_DeleteResource(self->socket, self->handle, 1, 0, 0);
	OUTPUT:
	RETVAL

void
getPref(self, id=0, backup=1)
	PDA::Pilot::DLP::DB *self
	int	id
	int	backup
	PPCODE:
	{
		Char4 creator;
	    int version, result;
		size_t len;
	    SV * c, n, v;
	    int r;
		if (self->Class) {
			int count;
			PUSHMARK(sp);
			XPUSHs(self->Class);
			PUTBACK;
			count = perl_call_method("creator", G_SCALAR);
			SPAGAIN;
			if (count != 1)
				croak("Unable to get creator");
			creator = SvChar4(POPs);
			PUTBACK;
        }
        if (pi_version(self->socket)< 0x101)
		    r = dlp_CloseDB(self->socket, self->handle);
	    result = dlp_ReadAppPreference(self->socket, creator, id, backup, 0xFFFF, mybuf, &len, &version);
	    if (pi_version(self->socket)< 0x101)
		    r = dlp_OpenDB(self->socket, self->dbcard, self->dbmode, SvPV(self->dbname,na), &self->handle);
	    ReturnReadPref(mybuf, len);
	}

SV *
setPref(self, data)
	PDA::Pilot::DLP::DB *self
	SV *data
	PPCODE:
	{
		Char4	creator;
		int id;
		int	version;
		int	backup;
	    STRLEN len;
	    int result;
	    void * buf;
	    int r;
	    PackPref;
	    buf = SvPV(data, len);
    	if (pi_version(self->socket)< 0x101)
	    	r = dlp_CloseDB(self->socket, self->handle);
	    result = dlp_WriteAppPreference(self->socket, creator, id, backup, version, buf, len);
    	if (pi_version(self->socket)< 0x101)
		    r = dlp_OpenDB(self->socket, self->dbcard, self->dbmode, SvPV(self->dbname,na), &self->handle);
		if (result < 0) {
			self->errnop = result;
			RETVAL = newSVsv(&sv_undef);
		} else {
			RETVAL = newSViv(result);
		}
	}

SV *
setPrefRaw(self, data, number, version, backup=1)
	PDA::Pilot::DLP::DB *self
	SV *data
	int	number
	int	version
	int	backup
	PPCODE:
	{
	    STRLEN len;
		Char4 creator;
	    int version, result;
	    void * buf;
	    PackRaw;
	    buf = SvPV(data, len);
		if (self->Class) {
			int count;
			PUSHMARK(sp);
			XPUSHs(self->Class);
			PUTBACK;
			count = perl_call_method("creator", G_SCALAR);
			SPAGAIN;
			if (count != 1)
				croak("Unable to get creator");
			creator = SvChar4(POPs);
			PUTBACK;
        }
	    result = dlp_WriteAppPreference(self->socket, creator, number, backup, version, buf, len);
		if (result < 0) {
			self->errnop = result;
			RETVAL = newSVsv(&sv_undef);
		} else {
			RETVAL = newSViv(result);
		}
	}


MODULE = PDA::Pilot		PACKAGE = PDA::Pilot::DLPPtr

void
DESTROY(self)
	PDA::Pilot::DLP *self
	CODE:
	if (self->socket)
		pi_close(self->socket);
	free(self);

int
errno(self)
	PDA::Pilot::DLP *self
	CODE:
		RETVAL = self->errnop;
		self->errnop = 0;
	OUTPUT:
	RETVAL

int
palmos_errno(self)
        PDA::Pilot::DLP *self
	CODE:
		RETVAL = pi_palmos_error(self->socket);
	OUTPUT:
	RETVAL

SV *
getTime(self)
	PDA::Pilot::DLP *self
	CODE:
	{
		time_t t;
		int result = dlp_GetSysDateTime(self->socket, &t);
		if (result < 0) {
			self->errnop = result;
			RETVAL = newSVsv(&sv_undef);
		} else
			RETVAL = newSViv(t);
	}
	OUTPUT:
	RETVAL

Result
setTime(self, time)
	PDA::Pilot::DLP *self
	long	time
	CODE:
	RETVAL = dlp_SetSysDateTime(self->socket, time);
	OUTPUT:
	RETVAL

SV *
getSysInfo(self)
	PDA::Pilot::DLP *self
	CODE:
	{
		struct SysInfo si;
		int result = dlp_ReadSysInfo(self->socket, &si);
		if (result < 0) {
			self->errnop = result;
			RETVAL = newSVsv(&sv_undef);
		} else {
			HV * i = newHV();
			hv_store(i, "romVersion", 10, newSViv(si.romVersion), 0);\
	    	hv_store(i, "locale", 6, newSViv(si.locale), 0);\
	    	hv_store(i, "product", 4, newSVpvn(si.prodID, si.prodIDLength), 0);\
			RETVAL = newRV((SV*)i);
		}
	}
	OUTPUT:
	RETVAL

SV *
getCardInfo(self, cardno=0)
	PDA::Pilot::DLP *self
	int	cardno
	CODE:
	{
		struct CardInfo c;
		int result = dlp_ReadStorageInfo(self->socket, cardno, &c);
		if (result < 0) {
			self->errnop = result;
			RETVAL = newSVsv(&sv_undef);
		} else {
			HV * i = newHV();
			hv_store(i, "card", 6, newSViv(c.card), 0);\
	    	hv_store(i, "version", 7, newSViv(c.version), 0);\
	    	hv_store(i, "created", 8, newSViv(c.creation), 0);\
	    	hv_store(i, "romSize", 7, newSViv(c.romSize), 0);\
	    	hv_store(i, "ramSize", 7, newSViv(c.ramSize), 0);\
	    	hv_store(i, "ramFree", 7, newSViv(c.ramFree), 0);\
	    	hv_store(i, "name", 4, newSVpv(c.name,0), 0);\
	    	hv_store(i, "manufacturer", 12, newSVpv(c.manufacturer,0), 0);\
			RETVAL = newRV((SV*)i);
		}
	}
	OUTPUT:
	RETVAL

int
setUserInfo(self, info)
	PDA::Pilot::DLP *self
	UserInfo	&info
	CODE:
	RETVAL = dlp_WriteUserInfo(self->socket, &info);
	OUTPUT:
	RETVAL

void
getBattery(self)
	PDA::Pilot::DLP *self
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
getUserInfo(self)
	PDA::Pilot::DLP *self
	CODE:
	{
		UserInfo info;
		int result;
		result = dlp_ReadUserInfo(self->socket, &info);
		pack_userinfo(RETVAL, info, result);
	}
	OUTPUT:
	RETVAL

void
newPref(self, creator, id=0, version=0, backup=0)
	PDA::Pilot::DLP *self
	Char4	creator
	SV *	id
	SV *	version
	SV *	backup
	PPCODE:
	{
		HV * h = perl_get_hv("PDA::Pilot::PrefClasses", 0);
		SV ** s;										
   		int count;											
		if (!h)												
			croak("PrefClasses doesn't exist");				
		s = hv_fetch(h, printlong(creator), 4, 0);			
		if (!s)												
			s = hv_fetch(h, "", 0, 0);						
		if (!s)												
			croak("Default PrefClass not defined");			
   		PUSHMARK(sp);										
   		XPUSHs(newSVsv(*s));								
   		XPUSHs(&sv_undef);									
    	XPUSHs(sv_2mortal(newSVChar4(creator)));			
    	if (id)
	    	XPUSHs(id);					
	    if (version)
	    	XPUSHs(version);				
	    if (backup)
	    	XPUSHs(backup);
    	PUTBACK;											
    	count = perl_call_method("pref", G_SCALAR);			
    	SPAGAIN;											
    	if (count != 1)										
    		croak("Unable to create resource");				
    }

Result
delete(self, name, cardno=0)
	PDA::Pilot::DLP *self
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
open(self, name, mode=0, cardno=0)
	PDA::Pilot::DLP *self
	char *	name
	SV *	mode
	int	cardno
	CODE:
	{
		int handle;
		int nummode;
		int result;
		if (!mode)
			nummode = dlpOpenRead|dlpOpenWrite|dlpOpenSecret;
		else {
			char *c;
			STRLEN len;
			nummode = SvIV(mode);
			if (SvPOKp(mode)) {
				c = SvPV(mode, len);
				while (*c) {
					switch (*c) {
					case 'r':
						nummode |= dlpOpenRead;
						break;
					case 'w':
						nummode |= dlpOpenWrite;
						break;
					case 'x':
						nummode |= dlpOpenExclusive;
						break;
					case 's':
						nummode |= dlpOpenSecret;
						break;
					}
					c++;
				}
			}
		}
		result = dlp_OpenDB(self->socket, cardno, nummode, name, &handle);
		if (result<0) {
			self->errnop = result;
			RETVAL = &sv_undef;
		} else {
			int type;
			PDA__Pilot__DLP__DB * x = malloc(sizeof(PDA__Pilot__DLP__DB));
			SV * sv = newSViv((IV)(void*)x);
			SvREFCNT_inc(ST(0));
			x->connection = ST(0);
			x->socket = self->socket;
			x->handle = handle;
			x->errnop = 0;
			x->dbname = newSVpv(name,0);
			x->dbmode = nummode;
			x->dbcard = cardno;
			RETVAL = newRV(sv);
			SvREFCNT_dec(sv);
			sv_bless(RETVAL, gv_stashpv("PDA::Pilot::DLP::DBPtr",0));
			{
				HV * h = perl_get_hv("PDA::Pilot::DBClasses", 0);
				SV ** s;
				if (!h)
					croak("DBClasses doesn't exist");
				s = hv_fetch(h, name, strlen(name), 0);
				if (!s)
					s = hv_fetch(h, "", 0, 0);
				if (!s)
					croak("Default DBClass not defined");
				x->Class = *s; 
				SvREFCNT_inc(*s);
			}
		}
	}
    OUTPUT:
    RETVAL

SV *
create(self, name, creator, type, flags, version, cardno=0)
	PDA::Pilot::DLP *self
	char *	name
	Char4	creator
	Char4	type
	int	flags
	int	version
	int	cardno
	CODE:
	{
		int handle;
		int result = dlp_CreateDB(self->socket, creator, type, cardno, flags, version, name, &handle);
		if (result<0) {
			self->errnop = result;
			RETVAL = &sv_undef;
		} else {
			PDA__Pilot__DLP__DB * x = malloc(sizeof(PDA__Pilot__DLP__DB));
			SV * sv = newSViv((IV)(void*)x);
			SvREFCNT_inc(ST(0));
			x->connection = ST(0);
			x->socket = self->socket;
			x->handle = handle;
			x->errnop = 0;
			x->dbname = newSVpv(name,0);
			x->dbmode = dlpOpenRead|dlpOpenWrite|dlpOpenSecret;
			x->dbcard = cardno;
			RETVAL = newRV(sv);
			SvREFCNT_dec(sv);
			sv_bless(RETVAL, gv_stashpv("PDA::Pilot::DLP::DBPtr",0));
			{
				HV * h = perl_get_hv("PDA::Pilot::DBClasses", 0);
				SV ** s;
				if (!h)
					croak("DBClasses doesn't exist");
				s = hv_fetch(h, name, strlen(name), 0);
				if (!s)
					s = hv_fetch(h, "", 0, 0);
				if (!s)
					croak("Default DBClass not defined");
				x->Class = *s; 
				SvREFCNT_inc(*s);
			}
		}
	}
    OUTPUT:
    RETVAL



void
getPref(self, creator, id=0, backup=1)
	PDA::Pilot::DLP *self
	Char4	creator
	int	id
	int	backup
	PPCODE:
	{
	    int version, result;
		size_t len;
	    SV * c, n, v;
	    result = dlp_ReadAppPreference(self->socket, creator, id, backup, 0xFFFF, mybuf, &len, &version);
	    ReturnReadPref(mybuf, len);
	}

SV *
setPref(self, data)
	PDA::Pilot::DLP *self
	SV *data
	PPCODE:
	{
		Char4	creator;
		int id;
		int	version;
		int	backup;
	    STRLEN len;
	    int result;
	    void * buf;
	    PackPref;
	    buf = SvPV(data, len);
	    result = dlp_WriteAppPreference(self->socket, creator, id, backup, version, buf, len);
		if (result < 0) {
			self->errnop = result;
			RETVAL = newSVsv(&sv_undef);
		} else {
			RETVAL = newSViv(result);
		}
	}

SV *
setPrefRaw(self, data, creator, number, version, backup=1)
	PDA::Pilot::DLP *self
	SV *data
	Char4	creator
	int	number
	int	version
	int	backup
	PPCODE:
	{
	    STRLEN len;
	    int version, result;
	    void * buf;
	    PackRaw;
	    buf = SvPV(data, len);
	    result = dlp_WriteAppPreference(self->socket, creator, number, backup, version, buf, len);
		if (result < 0) {
			self->errnop = result;
			RETVAL = newSVsv(&sv_undef);
		} else {
			RETVAL = newSViv(result);
		}
	}


Result
close(self, status=0)
	PDA::Pilot::DLP *self
	int	status
	CODE:
	RETVAL = dlp_EndOfSync(self->socket, status) || pi_close(self->socket);
	if (!RETVAL)
		self->socket = 0;
	OUTPUT:
	RETVAL

Result
abort(self)
	PDA::Pilot::DLP *self
	CODE:
	RETVAL = dlp_AbortSync(self->socket) || pi_close(self->socket);
	if (!RETVAL)
		self->socket = 0;
	OUTPUT:
	RETVAL

Result
reset(self)
	PDA::Pilot::DLP *self
	CODE:
	RETVAL = dlp_ResetSystem(self->socket);
	OUTPUT:
	RETVAL

Result
getStatus(self)
	PDA::Pilot::DLP *self
	CODE:
	RETVAL = dlp_OpenConduit(self->socket);
	OUTPUT:
	RETVAL

Result
log(self, message)
	PDA::Pilot::DLP *self
	char *	message
	CODE:
	RETVAL = dlp_AddSyncLogEntry(self->socket,message);
	OUTPUT:
	RETVAL


Result
dirty(self)
	PDA::Pilot::DLP *self
	CODE:
	RETVAL = dlp_ResetLastSyncPC(self->socket);
	OUTPUT:
	RETVAL

SV *
getDBInfo(self, start, RAM=1, ROM=0, cardno=0)
	PDA::Pilot::DLP *self
	int	start
	int	RAM
	int	ROM
	int	cardno
	CODE:
	{
		struct DBInfo info;

		int where = (RAM ? dlpDBListRAM : 0) | (ROM ? dlpDBListROM : 0);
		int result = dlp_ReadDBList(self->socket, cardno, where, start, &pibuf);
		pack_dbinfo(RETVAL,(*(struct DBInfo *)(pibuf.data)), result);
	}
	OUTPUT:
	RETVAL

SV *
findDBInfo(self, start, name, creator, type, cardno=0)
	PDA::Pilot::DLP *self
	int	start
	SV *	name
	SV *	creator
	SV *	type
	int	cardno
	CODE:
	{
		struct DBInfo info;
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
getFeature(self, creator, number)
	PDA::Pilot::DLP *self
	Char4	creator
	int	number
	CODE:
	{
		unsigned long f;
		int result;
		if ((result = dlp_ReadFeature(self->socket, creator, number, &f))<0) {
			RETVAL = newSVsv(&sv_undef);
			self->errnop = result;
		} else {
			RETVAL = newSViv(f);
		}
	}
	OUTPUT:
	RETVAL


void
getROMToken(self,token)
	PDA::Pilot::DLP *self
	Char4 token
	PPCODE:
	{
		char buffer[50];
		long long_token;
		size_t size;
		int result;

		result = dlp_GetROMToken(self->socket, token, buffer, &size);

		if (result==0) {
			EXTEND(sp,1);
			PUSHs(sv_2mortal(newSVpvn(buffer, size)));
		}
	}

void
callApplication(self, creator, type, action, data=&sv_undef)
	PDA::Pilot::DLP *self
	Char4	creator
	Char4	type
	int	action
	SV *data
	PPCODE:
	{
		unsigned long retcode;
		STRLEN len;
		int result;
		(void)SvPV(data,len);
		result = dlp_CallApplication(self->socket, creator, 
				    type, action, len, SvPV(data,na),
		                    &retcode, &pibuf);
		EXTEND(sp, 2);
		if (result >= 0) {
		        PUSHs(sv_2mortal(newSVpvn((char *) pibuf.data, pibuf.used)));
			if (GIMME != G_SCALAR) {
				PUSHs(sv_2mortal(newSViv(retcode)));
			}
		} else
			PUSHs(&sv_undef);
	}

int
tickle(self)
	PDA::Pilot::DLP *self
	CODE:
	{
		RETVAL = pi_tickle(self->socket);
	}
	OUTPUT:
	RETVAL

int
watchdog(self, interval)
	PDA::Pilot::DLP *self
	int interval
	CODE:
	{
		RETVAL = pi_watchdog(self->socket, interval);
	}
	OUTPUT:
	RETVAL

MODULE = PDA::Pilot		PACKAGE = PDA::Pilot::File

PDA::Pilot::File *
open(name)
	char *	name
	CODE:
	{
		RETVAL = calloc(sizeof(PDA__Pilot__File),1);
		RETVAL->errnop = 0;
		RETVAL->pf = pi_file_open(name);
		{
			HV * h = perl_get_hv("PDA::Pilot::DBClasses", 0);
			SV ** s;
			if (!h)
				croak("DBClasses doesn't exist");
			s = hv_fetch(h, name, strlen(name), 0);
			if (!s)
				s = hv_fetch(h, "", 0, 0);
			if (!s)
				croak("Default DBClass not defined");
			RETVAL->Class = *s; 
			SvREFCNT_inc(*s);
		}
	}
	OUTPUT:
	RETVAL

PDA::Pilot::File *
create(name, info)
	char *	name
	DBInfo	info
	CODE:
	RETVAL = calloc(sizeof(PDA__Pilot__File),1);
	RETVAL->errnop = 0;
	RETVAL->pf = pi_file_create(name, &info);
	{
		HV * h = perl_get_hv("PDA::Pilot::DBClasses", 0);
		SV ** s;
		if (!h)
			croak("DBClasses doesn't exist");
		s = hv_fetch(h, name, strlen(name), 0);
		if (!s)
			s = hv_fetch(h, "", 0, 0);
		if (!s)
			croak("Default DBClass not defined");
		RETVAL->Class = *s; 
		SvREFCNT_inc(*s);
	}
	OUTPUT:
	RETVAL

MODULE = PDA::Pilot		PACKAGE = PDA::Pilot::FilePtr

int
errno(self)
	PDA::Pilot::File *self
	CODE:
		RETVAL = self->errnop;
		self->errnop = 0;
	OUTPUT:
	RETVAL

void
DESTROY(self)
	PDA::Pilot::File *self
	CODE:
	if (self->pf)
		pi_file_close(self->pf);
	if (self->Class)
		SvREFCNT_dec(self->Class);
	free(self);

SV *
class(self, name=0)
	PDA::Pilot::File *self
	SV *	name
	CODE:
	{
		SV ** s = 0;
		HV * h;
		if (name) {
			STRLEN len;
			h = perl_get_hv("PDA::Pilot::DBClasses", 0);
			if (!h)
				croak("DBClasses doesn't exist");
			if (SvOK(name)) {
				(void)SvPV(name, len);
				s = hv_fetch(h, SvPV(name, na), len, 0);
			}
			if (!s)
				s = hv_fetch(h, "", 0, 0);
			if (!s)
				croak("Default DBClass not defined");
			SvREFCNT_inc(*s);
			if (self->Class)
				SvREFCNT_dec(self->Class);
			self->Class = *s;
		}
		RETVAL = newSVsv(self->Class);
	}
	OUTPUT:
	RETVAL

int
close(self)
	PDA::Pilot::File *self
	CODE:
	if (self->pf) {
		RETVAL = pi_file_close(self->pf);
		self->pf = 0;
	} else
		RETVAL = 0;
	OUTPUT:
	RETVAL

SV *
getAppBlock(self)
	PDA::Pilot::File *self
	PPCODE:
	{
		int result = 0;
	    size_t len;
	    void * buf;
		pi_file_get_app_info(self->pf, &buf, &len);
		ReturnReadAI(buf, (int)len);
	}

SV *
getSortBlock(self)
	PDA::Pilot::File *self
	PPCODE:
	{
		int result = 0;
	    size_t len;
	    void * buf;
		pi_file_get_sort_info(self->pf, &buf, &len);
		ReturnReadSI(buf, (int)len);
	}


SV *
getRecords(self)
	PDA::Pilot::File *self
	CODE:
	{
		int len, result = 0;
		pi_file_get_entries(self->pf, &len);
		RETVAL = newSViv((int)len);
	}
	OUTPUT:
	RETVAL

SV *
getResource(self, index)
	PDA::Pilot::File *self
	int	index
	CODE:
	{
	    int result, id;
		size_t len;
	    Char4 type;
	    void * buf;
		result = pi_file_read_resource(self->pf, index, &buf, &len, &type, &id);
		ReturnReadResource(buf,(int)len);
	}
	OUTPUT:
	RETVAL

SV *
getRecord(self, index)
	PDA::Pilot::File *self
	int	index
	PPCODE:
	{
	    int result, attr, category;
		size_t len;
	    unsigned long id;
	    void * buf;
		result = pi_file_read_record(self->pf, index, &buf, &len, &attr, &category, &id);
		ReturnReadRecord(buf,(int)len);
	}


SV *
getRecordByID(self, id)
	PDA::Pilot::File *self
	unsigned long	id
	CODE:
	{
	    int result;
		size_t len;
	    int attr, category, index;
	    void * buf;
		result = pi_file_read_record_by_id(self->pf, id, &buf, &len, &index, &attr, &category);
		ReturnReadRecord(buf, (int)len);
	}
	OUTPUT:
	RETVAL

int
checkID(self, uid)
	PDA::Pilot::File *self
	unsigned long	uid
	CODE:
	RETVAL = pi_file_id_used(self->pf, uid);
	OUTPUT:
	RETVAL

SV *
getDBInfo(self)
	PDA::Pilot::File *self
	CODE:
	{
		DBInfo result;
		pi_file_get_info(self->pf, &result);
		pack_dbinfo(RETVAL, result, 0);
	}
	OUTPUT:
	RETVAL

int
setDBInfo(self, info)
	PDA::Pilot::File *self
	DBInfo	info
	CODE:
	RETVAL = pi_file_set_info(self->pf, &info);
	OUTPUT:
	RETVAL

int
setAppBlock(self, data)
	PDA::Pilot::File *self
	SV *data
	CODE:
	{
	    STRLEN len;
	    char * c;
	    PackAI;
	    c = SvPV(data, len);
		RETVAL = pi_file_set_app_info(self->pf, c, len);
    }
	OUTPUT:
	RETVAL

int
setSortBlock(self, data)
	PDA::Pilot::File *self
	SV *data
	CODE:
	{
	    STRLEN len;
	    char * c;
	    PackSI;
	    c = SvPV(data, len);
		RETVAL = pi_file_set_sort_info(self->pf, c, len);
    }
	OUTPUT:
	RETVAL

int
addResource(self, data, type, id)
	PDA::Pilot::File *self
	SV *data
	Char4	type
	int id
	CODE:
	{
	    STRLEN len;
	    int result;
	    void * buf;
	    PackResource;
	    buf = SvPV(data, len);
		RETVAL = pi_file_append_resource(self->pf, buf, len, type, id);
	}
	OUTPUT:
	RETVAL

int
addRecord(self, data)
	PDA::Pilot::File *self
	SV *data
	CODE:
	{
	    STRLEN len;
	    unsigned long id;
	    int attr, category;
	    int result;
	    void * buf;
	    PackRecord;
	    buf = SvPV(data, len);
		RETVAL = pi_file_append_record(self->pf, buf, len, attr, category, id);
	}
	OUTPUT:
	RETVAL

int
addRecordRaw(self, data, uid, attr, category)
	PDA::Pilot::File *self
	SV *data
	unsigned long	uid
	int attr
	int category
	CODE:
	{
	    STRLEN len;
	    int result;
	    void * buf;
	    PackRaw;
	    buf = SvPV(data, len);
		RETVAL = pi_file_append_record(self->pf, buf, len, attr, category, uid);
	}
	OUTPUT:
	RETVAL


int
install(self, socket, cardno)
	PDA::Pilot::File *self
	PDA::Pilot::DLP *socket
	int cardno
	CODE:
	RETVAL = pi_file_install(self->pf, socket->socket, cardno, NULL);
	OUTPUT:
	RETVAL

int
retrieve(self, socket, cardno)
	PDA::Pilot::File *self
	PDA::Pilot::DLP *socket
	int cardno
	CODE:
	RETVAL = pi_file_retrieve(self->pf, socket->socket, cardno, NULL);
	OUTPUT:
	RETVAL
