#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "../../include/pi-macros.h"
#include "../../include/pi-datebook.h"
#include "../../include/pi-socket.h"
#include "../../include/pi-dlp.h"

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

static char socket_buf[0xffff];

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

#ifndef newRV_noinc   
static SV * rv;
#define newRV_noinc(s) ((rv=newRV(s)), SvREFCNT_dec(s), rv)
#endif


MODULE = PDA::Pilot		PACKAGE = PDA::Pilot

double
constant(name,arg)
	char *		name
	int		arg


void
unpack_Appointment(record)
    SV * record
    CODE:
    {
    int len;
    int i;
    AV * e;
    HV * ret;
    struct Appointment a;
    SvPV(record, len);
    unpack_Appointment(&a, SvPV(record, len), len);
    
    ret = newHV();
    hv_store(ret, "event", 5, newSViv(a.event), 0);
    
    if (!a.event) {
	    hv_store(ret, "begin", 5, newRV_noinc((SV*)tmtoav(&a.begin)), 0);
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
   		hv_store(repeat, "freq", 4, newSViv(a.repeatFreq), 0);
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
    
    ST(0) = newRV_noinc((SV*)ret);
    }

void
unpack_AppointmentAppInfo(record)
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
    
    e = newAV();
    hv_store(ret, "categorynames", 13, newRV_noinc((SV*)e), 0);
    
    for (i=0;i<16;i++) {
    	av_push(e, newSVpv(a.CategoryName[i], 0));
    }

    e = newAV();
    hv_store(ret, "categoryids", 11, newRV_noinc((SV*)e), 0);
    
    for (i=0;i<16;i++) {
    	av_push(e, newSViv(a.CategoryID[i]));
    }

    hv_store(ret, "lastcatid", 9, newSViv(a.lastUniqueID), 0);

    hv_store(ret, "weekstart", 9, newSViv(a.startOfWeek), 0);

    ST(0) = newRV_noinc((SV*)ret);

    }


int
pi_close(socket)
	int	socket

int
pi_write(socket, msg)
	int	socket
	SV *	msg
	CODE: {
	    int len;
		RETVAL = pi_write(socket,SvPV(msg,len),len);
	}

int
pi_read(socket, msg)
	int	socket
	SV *	msg
	CODE: {
	    int len;
	    RETVAL = pi_read(socket, socket_buf, 0xffff);
	    if (RETVAL >=0) 
	    	sv_setpvn(msg, socket_buf, RETVAL);
	    else
	    	sv_setsv(msg, &sv_undef);
	}

int
pi_socket(domain, type, protocol)
	int	domain
	int	type
	int	protocol

int
pi_listen(socket, backlog)
	int	socket
	int	backlog

int
pi_bind(socket, sockaddr, addrlen)
	int	socket
	char *	sockaddr
	int	addrlen

int
pi_accept(socket, remoteaddr, addrlen)
	int	socket
	char *	remoteaddr
	int	&addrlen


int
dlp_OpenDB(sd, cardno, mode, name, dbhandle)
    INPUT:
	int	sd
	int	cardno
	int	mode
	char *	name
	int	&dbhandle
    OUTPUT:
	dbhandle

int
dlp_CloseDB(sd, dbhandle)
	int	sd
	int	dbhandle

int
dlp_EndOfSync(sd, status)
	int	sd
	int	status

int
dlp_ResetSystem(sd)
	int	sd

int
dlp_ReadAppBlock(sd, dbhandle, offset, dbuf, dlen)
	int	sd
	int	dbhandle
	int	offset
	char *	dbuf
	int	dlen

int
dlp_ReadOpenDBInfo(sd, dbhandle, records)
	int	sd
	int	dbhandle
	int	&records

int
dlp_ReadRecordByIndex(sd, fHandle, index, buffer, id, size, attr, category)
    PREINIT:
        SV *sv_buffer = SvROK(ST(3)) ? SvRV(ST(3)) : ST(3);
    INPUT:
	int	sd
	int	fHandle
	int	index
	long	&id
	int	&size
	int	&attr
	int	&category
	char *	buffer = sv_grow( sv_buffer, size+1 );
    OUTPUT:
	id
	size
	attr
	category
    CLEANUP:
        if (RETVAL >= 0) {
            SvCUR(sv_buffer) = RETVAL;
            SvPOK_only(sv_buffer);
            *SvEND(sv_buffer) = '\0';
            if (tainting)
                sv_magic(sv_buffer, 0, 't', 0, 0);
        }

