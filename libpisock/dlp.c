/*
 * dlp.c:  Palm DLP protocol
 *
 * Copyright (c) 1996, 1997, Kenneth Albanowski
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef WIN32
#include <winsock.h>		/* for hton* */
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "pi-debug.h"
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-syspkt.h"

#define get_date(ptr) (dlp_ptohdate((ptr)))

#define set_date(ptr,val) (dlp_htopdate((val),(ptr)))

#define DLP_BUF_SIZE 0xffff
#define DLP_REQUEST_DATA(req, arg, offset) &req->argv[arg]->data[offset]
#define DLP_RESULT_DATA(res, arg, offset) &res->argv[arg]->data[offset]

static unsigned char dlp_buf[DLP_BUF_SIZE];


/* Define prototypes */
int dlp_exec(int sd, int cmd, int arg, const unsigned char *msg, int msglen, 
             unsigned char *result, int maxlen);
int dlp_exec_new(int sd, struct dlpRequest *req, struct dlpResponse **res);

char *dlp_errorlist[] = {
	"No error",
	"General System error",
	"Illegal Function",
	"Out of memory",
	"Invalid parameter",
	"Not found",
	"None Open",
	"Already Open",
	"Too many Open",
	"Already Exists",
	"Cannot Open",
	"Record deleted",
	"Record busy",
	"Operation not supported",
	"-Unused-",
	"Read only",
	"Not enough space",
	"Limit exceeded",
	"Sync cancelled",
	"Bad arg wrapper",
	"Argument missing",
	"Bad argument size"
};

	"Incompatible API version"
};


char *dlp_strerror(int error)
{
	if (error < 0)
		error = -error;
	if ((unsigned int) error >= (sizeof(dlp_errorlist)/(sizeof(char *))))
		return "Unknown error";
	else
		return dlp_errorlist[error];
}

  LOG(PI_DBG_DLP, PI_DBG_LVL_INFO, "DLP %s sd: %d\n", #name, sd);

#ifdef PI_DEBUG
#define Trace(name) \
      LOG(PI_DBG_DLP, PI_DBG_LVL_ERR, "DLP Error  %s (%d)\n", dlp_errorlist[-result], result); \
#define Expect(count)    \
      LOG(PI_DBG_DLP, PI_DBG_LVL_ERR, "DLP Read %d bytes, expected at least %d\n", result, count); \
    if (result < 0) {    \
      LOG((PI_DBG_DLP, PI_DBG_LVL_ERR, "DLP Error  %s (%d)\n", dlp_errorlist[-result], result)); \
    } else {             \
      LOG((PI_DBG_DLP, PI_DBG_LVL_ERR, "DLP Read %d bytes, expected at least %d\n", result, count)); \
      LOG(PI_DBG_DLP, PI_DBG_LVL_INFO, "DLP RX %d bytes\n", result);
    }                    \
    return result;       \
  } else                 \
      LOG((PI_DBG_DLP, PI_DBG_LVL_INFO, "DLP RX %d bytes\n", result));
#else
struct dlpArg *
dlp_arg_new (int id, int len) 
{
	struct dlpArg *arg;
	
	arg = malloc(sizeof (struct dlpArg));
	arg->id = id;
	arg->len = len;
	if (len > 0)
		arg->data = malloc (sizeof (unsigned char) * len);
	else
		arg->data = NULL;
	
	return arg;
}

void
dlp_arg_free (struct dlpArg *arg)
{
	if (arg->data != NULL)
		free (arg->data);
	free (arg);
}

int
dlp_arg_len (int argc, struct dlpArg **argv)
{
	int i, len = 0;
	
	for (i = 0; i < argc; i++) {
		struct dlpArg *arg = argv[i];
		
		if (arg->len < PI_DLP_ARG_TINY_LEN)
			len += 2;
		else if (arg->len < PI_DLP_ARG_SHORT_LEN)
			len += 4;
		else if (arg->len < PI_DLP_ARG_LONG_LEN) 
			len += 6;
		else
			return -1;
		
		len += arg->len;
	}

	return len;
}

struct dlpRequest *
dlp_request_new (enum dlpFunctions cmd, int argc, ...) 
{
	struct dlpRequest *req;
	va_list ap;
	int i;
	
	req = malloc (sizeof (struct dlpRequest));
	req->cmd = cmd;

	req->argc = argc;
	req->argv = malloc (sizeof (struct dlpArg *) * argc);

	va_start (ap, argc);
	for (i = 0; i < argc; i++) {
		int len;

		len = va_arg (ap, int);
		req->argv[i] = dlp_arg_new (PI_DLP_ARG_FIRST_ID + i, len);
	}
	va_end (ap);
	
	return req;	
}

struct dlpResponse *
dlp_response_new (enum dlpFunctions cmd, int argc) 
{
	struct dlpResponse *res;
	
	res = malloc (sizeof (struct dlpResponse));
	res->cmd = cmd;
	res->err = dlpErrNoError;
	
	res->argc = argc;
	res->argv = malloc (sizeof (struct dlpArg *) * argc);
	
	return res;
}

int
dlp_response_read (struct dlpResponse **res, int sd)
{
	struct dlpResponse *response;
	unsigned char dlp_buf[DLP_BUF_SIZE], *buf;
	short argid;
	int bytes, len, i;
	
	bytes = pi_read(sd, dlp_buf, DLP_BUF_SIZE);
	if (bytes < 0)
		return -1;	

	response = dlp_response_new (dlp_buf[0] & 0x7f, dlp_buf[1]);
	*res = response;

	response->err = get_short (&dlp_buf[2]);

	buf = dlp_buf + 4;
	for (i = 0, argid = PI_DLP_ARG_FIRST_ID; i < response->argc; i++, argid++) {
		if (get_byte (buf) == argid) {
			len = get_byte(&buf[1]);
			buf += 2;
		} else if ((get_byte (buf) & 0x7f) == argid) {
			len = get_short (&buf[2]);
			buf += 4;
		} else if ((get_short(buf) & 0x3FFF) == argid) {
			len = get_long (&buf[2]);
			buf += 6;
		} else {
			return -1;
		}
		
		response->argv[i] = dlp_arg_new (argid, len);
		memcpy (response->argv[i]->data, buf, len);
		buf += len;
	}
		     
	return 0;
}

int
dlp_request_write (struct dlpRequest *req, int sd)
{
	unsigned char *exec_buf, *buf;
	short argid;
	int len, i;
	
	len = dlp_arg_len (req->argc, req->argv) + 2;
	exec_buf = (unsigned char *) malloc (sizeof (unsigned char) * len);
	
	set_byte (&exec_buf[PI_DLP_OFFSET_CMD], req->cmd);
	set_byte (&exec_buf[PI_DLP_OFFSET_ARGC], req->argc);

	buf = &exec_buf[PI_DLP_OFFSET_ARGV];	
	for (i = 0, argid = PI_DLP_ARG_FIRST_ID; i < req->argc; i++, argid++) {
		struct dlpArg *arg = req->argv[i];
		
		if (arg->len < PI_DLP_ARG_TINY_LEN) {
			set_byte(&buf[0], argid | PI_DLP_ARG_FLAG_TINY);
			set_byte(&buf[1], arg->len);
			
			memcpy(&buf[2], arg->data, arg->len);
			buf += arg->len + 2;			
		} else if (arg->len < PI_DLP_ARG_SHORT_LEN) {
			set_byte(&buf[0], argid | PI_DLP_ARG_FLAG_SHORT);
			set_byte(&buf[1], 0);
			set_short(&buf[2], arg->len);
			
			memcpy (&buf[4], arg->data, arg->len);
			buf += arg->len + 4;			
		} else if (arg->len < PI_DLP_ARG_LONG_LEN) {
			set_short (&buf[0], argid | PI_DLP_ARG_FLAG_LONG);
			set_long (&buf[2], arg->len);

			memcpy (&buf[6], arg->data, arg->len);
			buf += arg->len + 6;
		} else {
			return i;
		}
	}

	if (pi_write(sd, exec_buf, len) < len) {
		errno = -EIO;
		return -1;
	}

	return i;
}

void
dlp_request_free (struct dlpRequest *req)
{
	int i;

	for (i = 0; i < req->argc; i++)
		dlp_arg_free (req->argv[i]);
	
	free (req->argv);
	free (req);
}

void
dlp_response_free (struct dlpResponse *res) 
{
	int i;

	for (i = 0; i < res->argc; i++)
		dlp_arg_free (res->argv[i]);
	
	free (res->argv);
	free (res);	
}

int dlp_exec(int sd, int cmd, int arg,const unsigned char *msg, int msglen, 
	     unsigned char *result, int maxlen)
{
	static unsigned char exec_buf[DLP_BUF_SIZE];
	int 	i,	
		err;

	exec_buf[0] = (unsigned char) cmd;
	if (msg && arg && msglen) {
		memcpy(&exec_buf[6], msg, msglen);
		exec_buf[1] = (unsigned char) 1;
		exec_buf[2] = (unsigned char) (arg | 0x80);
		exec_buf[3] = (unsigned char) 0;
		set_short(exec_buf + 4, msglen);
		i = msglen + 6;
	} else {
		exec_buf[1] = (unsigned char) 0;
		i = 2;
	}

	if (pi_write(sd, &exec_buf[0], i) < i) {
		errno = -EIO;
		return -1;
	}

	i = pi_read(sd, &exec_buf[0], DLP_BUF_SIZE);
	if (i < 0)
		return -1;
	
	err = get_short(exec_buf + 2);

	if (err != 0) {
		errno = -EIO;
		return -err;
	}
	
	if (exec_buf[0] != (unsigned char) (cmd | 0x80)) {		/* received wrong response 	*/
		errno = -ENOMSG;
		return -1;
	}

	if ((exec_buf[1] == (unsigned char) 0) || (result == 0))	/* no return blocks or buffers 	*/
		return 0;

	/* assume only one return block */
	if ((exec_buf[4] & 0xC0) == 0xC0) {	/* Long arg */
		i = get_long(exec_buf + 6);

		if (i > maxlen)
			i = maxlen;

		memcpy(result, &exec_buf[10], i);
	} else if (exec_buf[4] & 0x80) {	/* Short arg */
		i = get_short(exec_buf + 6);

		if (i > maxlen)
			i = maxlen;

		memcpy(result, &exec_buf[8], i);
	} else {		/* Tiny arg */
		i = (int) exec_buf[5];

		if (i > maxlen)
			i = maxlen;

		memcpy(result, &exec_buf[6], i);
	}

	return i;

	
	if (res->argv)
int dlp_exec_new(int sd, struct dlpRequest *req, struct dlpResponse **res)
{
	if (dlp_request_write (req, sd) < req->argc) {
		errno = -EIO;
		return -1;
	}

	if (dlp_response_read (res, sd) < 0) {
		errno = -EIO;
		return -1;
	}
	
	if ((*res)->cmd != req->cmd) {
		/* Response was not for this command */
		errno = -ENOMSG;
		return -1;
	}

	return 0;
}


	return bytes;
}

/* These conversion functions are strictly for use within the DLP layer. 
   This particular date/time format does not occur anywhere else within the
   Palm or its communications. */

/* Notice: These conversion functions apply a possibly incorrect timezone
   correction. They use the local time on the UNIX box, and transmit this
   directly to the Palm. This assumes that the Palm has the same local time.
   If the Palm is communicating from a different timezone, this is not
   necessarily correct.
   
   It would be possible to compare the current time on the Palm with the
   current time on the UNIX box, and use that as the timezone offset, but
   this would break if the Palm had the wrong time, or one or the either
   didn't have the proper local (wall) time.
   
   In any case, since the (possibly incorrect) timezone correction is
   applied both way, there is no immediate problem.
   -- KJA */

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Convert Palm format to time_t
 *
 * Parameters:  None
 *
 * Returns:     time_t struct to mktime
 *
 ***********************************************************************/
static time_t dlp_ptohdate(unsigned const char *data)
{
	struct tm t;

	t.tm_sec 	= (int) data[6];
	t.tm_min 	= (int) data[5];
	t.tm_hour 	= (int) data[4];
	t.tm_mday 	= (int) data[3];
	t.tm_mon 	= (int) data[2] - 1;
	t.tm_year 	= ((data[0] << 8) | data[1]) - 1900;
	t.tm_wday 	= 0;
	t.tm_yday 	= 0;
	t.tm_isdst 	= -1;

	/* Seems like year comes back as all zeros if the date is "empty"
	   (but other fields can vary).  And mktime() chokes on 1900 B.C. 
	   (result of 0 minus 1900), returning -1, which the higher level
	   code can't deal with (passes it straight on to utime(), which
	   simply leaves the file's timestamp "as-is").
	 
	   So, since year 0 appears to mean "no date", we'll return an odd
	   number that works out to precisely one day before the start of
	   the Palm's clock (thus little chance of being run into by any
	   Palm-based time stamp). */

	if (data[0] == 0 && data[1] == 0)

	/* This original calculation was wrong, and reported one day earlier
	   return (time_t) 0x83D8FE00;		// Wed Dec 30 16:00:00 1903 GMT
	   following: 
		perl -e '$date=localtime(0x83D8FE00); print $date,"\n"'

	   return (time_t) 0x83D96E80;		// Thu Dec 31 00:00:00 1903 GMT
	   return (time_t) 0x00007080;		// Thu Jan  1 00:00:00 1970 GMT
	   Here are others, depending on what your system requirements are: 

	   return (time_t) 0x83D96E80;	// Thu Dec 31 00:00:00 1903 GMT
	   return (time_t) 0x00007080;	// Thu Jan  1 00:00:00 1970 GMT

	return (time_t) 0x83DAC000;		/* Fri Jan  1 00:00:00 1904 GMT */
	   that's what we'll use here until something else breaks it.

	*/
	return (time_t) 0x83DAC000;	/* Fri Jan  1 00:00:00 1904 GMT */
	return mktime(&t);
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Convert time_t to Palm format
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void dlp_htopdate(time_t time, unsigned char *data)
{				/* @+ptrnegate@ */
	int 	year;
	struct 	tm *t = localtime(&time);

	ASSERT(t != 0);

	year = t->tm_year + 1900;

	data[7] = (unsigned char) 0;	/* packing spacer */
	data[6] = (unsigned char) t->tm_sec;
	data[5] = (unsigned char) t->tm_min;
	data[4] = (unsigned char) t->tm_hour;
	data[3] = (unsigned char) t->tm_mday;
	data[2] = (unsigned char) (t->tm_mon + 1);
	data[0] = (unsigned char) ((year >> 8) & 0xff);
	data[1] = (unsigned char) ((year >> 0) & 0xff);
 *  -- DHS
 ***********************************************************************/


/***********************************************************************
 *
 * Function:    dlp_GetSysDateTime
 * Parmeters:   None
 * Summary:     DLP 1.0 GetSysDateTime function to get device date 
 *		and time
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
	unsigned char buf[8];
{
	int 	result;
	struct dlpRequest *req;
	result = dlp_exec(sd, dlpFuncGetSysDateTime, 0x20, 0, 0, buf, 8);

	Expect(8);
	dlp_request_free(req);
	*t = dlp_ptohdate(buf);

	LOG(PI_DBG_DLP, PI_DBG_LVL_INFO, "DLP GetSysDateTime %s", ctime(t));
		*t = dlp_ptohdate(DLP_RESPONSE_DATA (res, 0, 0));
	}

	dlp_response_free(res);
	
	return result;
}

/***********************************************************************
 *
 * Function:    dlp_SetSysDateTime
 * Parmeters:   None
 * Summary:     DLP 1.0 SetSysDateTime function to set the device date
 *		and time
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
	unsigned char buf[8];
{
	dlp_htopdate(time, buf);
	struct dlpRequest *req;
	Trace(SetSysDateTime);

	LOG(PI_DBG_DLP, PI_DBG_LVL_INFO, "DLP Wrote Time: %s", ctime(&time));
	
	result = dlp_exec(sd, dlpFuncSetSysDateTime, 0x20, buf, 8, 0, 0);

	Expect(0);
	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	dlp_response_free(res);

/*  
   Begin struct SI
 
   Byte    number		byte(0)
   Byte    more			byte(1)
   Byte    unused		byte(2)
   Byte    count		byte(3)
   Byte    totalSize		byte(4)
   Byte    cardNo		byte(5)
   Word    cardVersion		short(6)
   DlpDateTimeType crDate	date(8)
   DWord   romSize		long(16)
   DWord   ramSize		long(20)
   DWord   freeRam		long(24)
   Byte    cardNameSize	byte(28)
   Byte    cardManufSize	byte(29)
   Char[0] cardNameAndManuf	byte(30)
   struct read access 
 */

#define SI_number(ptr)				get_byte((ptr)+0)
#define SI_more(ptr)				get_byte((ptr)+1)
#define SI_unused(ptr)				get_byte((ptr)+2)
#define SI_count(ptr)				get_byte((ptr)+3)
#define SI_totalSize(ptr)			get_byte((ptr)+4)
#define SI_cardNo(ptr)				get_byte((ptr)+5)
#define SI_cardVersion(ptr)			get_short((ptr)+6)
#define SI_crDate(ptr)				get_date((ptr)+8)
#define SI_romSize(ptr)				get_long((ptr)+16)
#define SI_ramSize(ptr)				get_long((ptr)+20)
#define SI_freeRam(ptr)				get_long((ptr)+24)
#define SI_cardNameSize(ptr)			get_byte((ptr)+28)
#define SI_cardManufSize(ptr)			get_byte((ptr)+29)
#define SI_cardNameAndManuf(ptr,idx)		get_byte((ptr)+30+1*(idx))
#define ptr_SI_cardNameAndManuf(ptr,idx)	((ptr)+30+1*(idx))
#define sizeof_SI		(30)
 /* end struct SI */

/* begin struct SIRequest
   Byte    cardNo	byte(0)
   Byte    unused	byte(1)
   struct write access 
 */
#define SIRequest_cardNo(ptr,val)		set_byte((ptr)+0,(val))
#define SIRequest_unused(ptr,val)		set_byte((ptr)+1,(val))
#define sizeof_SIRequest		(2)
 /* end struct SIRequest */


	return result;
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     DLP 1.0 ReadStorageInfo to read ROM/RAM regions
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int dlp_ReadStorageInfo(int sd, int cardno, struct CardInfo *c)

	SIRequest_cardNo(dlp_buf, cardno);
	SIRequest_unused(dlp_buf, 0);
		len1,
		len2;
	struct dlpRequest *req;
	LOG(PI_DBG_DLP, PI_DBG_LVL_INFO, "DLP Wrote Cardno: %d", cardno);
	req = dlp_request_new(dlpFuncReadStorageInfo, 1, 2);
	result = dlp_exec(sd, dlpFuncReadStorageInfo, 0x20, dlp_buf, 2, dlp_buf, 256 + 26);
	set_byte(DLP_REQUEST_DATA(req, 0, 0), cardno);
	c->more = 0;

	Expect(sizeof_SI);

	c->more 	= SI_more(dlp_buf) || (SI_count(dlp_buf) > 1);
	c->card 	= SI_cardNo(dlp_buf);
	c->version 	= SI_cardVersion(dlp_buf);
	c->creation 	= SI_crDate(dlp_buf);
	c->romSize 	= SI_romSize(dlp_buf);
	c->ramSize 	= SI_ramSize(dlp_buf);
	c->ramFree 	= SI_freeRam(dlp_buf);

	len1 = SI_cardNameSize(dlp_buf);
	memcpy(c->name, ptr_SI_cardNameAndManuf(dlp_buf, 0), len1);
	c->name[len1] = '\0';

	len2 = SI_cardManufSize(dlp_buf);
	memcpy(c->manufacturer, ptr_SI_cardNameAndManuf(dlp_buf, len1),
	       len2);
	c->manufacturer[len2] = '\0';

	LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
	    "DLP Read Cardno: %d, Card Version: %d, Creation time: %s",
	    c->card, c->version, ctime(&c->creation));
	LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
	    "  Total ROM: %lu, Total RAM: %lu, Free RAM: %lu\n",
	    c->romSize, c->ramSize, c->ramFree);
	LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
	    "  Card name: '%s'\n", c->name);
	LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
	    "  Manufacturer name: '%s'\n", c->manufacturer);
	LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
	    "  More: %s\n", c->more ? "Yes" : "No");
		    "  Manufacturer name: '%s'\n", c->manufacturer));
	}

	dlp_response_free (res);
	
	return result;
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Read the System Information (memory, battery, etc.) 
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	Trace(ReadSysInfo);

	req = dlp_request_new (dlpFuncReadSysInfo, 1, 4);

	set_short (DLP_REQUEST_DATA (req, 0, 0), 0x0001);
	set_short (DLP_REQUEST_DATA (req, 0, 2), 0x0003);
	
	result = dlp_exec_new(sd, req, &res);
	set_short (DLP_REQUEST_DATA (req, 0, 0), 0x0001);
	dlp_request_free (req);

	if (result >= 0) {
		s->romVersion = get_long (DLP_RESULT_DATA (res, 0, 0));
		s->locale = get_long (DLP_RESULT_DATA (res, 0, 4));
		/* The 8th byte is a filler byte */
		s->prodIDLength = get_byte (DLP_RESULT_DATA (res, 0, 9));
		memcpy(s->prodID, dlp_buf + 10, s->prodIDLength);

		if (req->argc > 1) {
			s->dlpMajorVersion = get_short (DLP_RESULT_DATA (res, 1, 0));
			s->dlpMinorVersion = get_short (DLP_RESULT_DATA (res, 1, 2));
			s->compatMajorVersion = get_short (DLP_RESULT_DATA (res, 1, 4));
			s->compatMinorVersion = get_short (DLP_RESULT_DATA (res, 1, 6));
		} else {
			s->dlpMajorVersion = 0;
			s->dlpMinorVersion = 0;
			s->compatMajorVersion = 0;
			s->compatMinorVersion = 0;
		}

		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadSysInfo ROM Version : 0x%8.8lX, Locale ID: 0x%8.8lX, ",
		    s->romVersion, s->locale);
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "Product ID: 0x%8.8lX\n", s->prodID);
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "  DLP Major Version : 0x%4.4lX, DLP Minor Version: 0x%4.4lX, ",
		    s->dlpMajorVersion, s->dlpMinorVersion);
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "Compat Major Version : 0x%4.4lX, Compat Minor Version: 0x%4.4lX\n",
		    s->compatMajorVersion, s->compatMinorVersion);		
	}
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
	dlp_response_free (res);
	
	}

	dlp_response_free (res);
	
	return result;
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Iterate through the list of databases on the Palm
 *
 * Parameters:  None
 *
 * Returns:	A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int

	dlp_buf[0] = (unsigned char) flags;
	dlp_buf[1] = (unsigned char) cardno;
	set_short(dlp_buf + 2, start);
{
	int 	result;
	struct dlpRequest *req;
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Cardno: %d, Start: %d, Flags:",
			cardno, start);
		if (flags & dlpDBListROM)
			fprintf(stderr, " ROM");
		if (flags & dlpDBListRAM)
			fprintf(stderr, " RAM");
		if (!flags)
			fprintf(stderr, " None");
		fprintf(stderr, " (0x%2.2X)\n", flags);
	}
#endif
	
	result = dlp_exec(sd, dlpFuncReadDBList, 0x20, dlp_buf, 4, dlp_buf, 48 + 32);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), (unsigned char) cardno);
	info->more = 0;

	Expect(48);

	info->more = get_byte(dlp_buf + 2);
	if (pi_version(sd) > 0x0100)	/* PalmOS 2.0 has additional flag */
		info->miscFlags = get_byte(dlp_buf + 5);
	else
		info->miscFlags = 0;
	info->flags 		= get_short(dlp_buf + 6);
	info->type 		= get_long(dlp_buf + 8);
	info->creator 		= get_long(dlp_buf + 12);
	info->version 		= get_short(dlp_buf + 16);
	info->modnum 		= get_long(dlp_buf + 18);
	info->createDate 	= get_date(dlp_buf + 22);
	info->modifyDate 	= get_date(dlp_buf + 30);
	info->backupDate 	= get_date(dlp_buf + 38);
	info->index 		= get_short(dlp_buf + 46);
	strncpy(info->name, (char *) dlp_buf + 48, 32);
	info->name[32] 		= '\0';

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			"  Read: Name: '%s', Version: %d, More: %s\n",
			info->name, info->version,
			info->more ? "Yes" : "No");
		fprintf(stderr, "        Creator: '%s'",
			printlong(info->creator));
		fprintf(stderr, ", Type: '%s', Flags:",
			printlong(info->type));
		if (info->flags & dlpDBFlagResource)
			fprintf(stderr, " Resource");
		if (info->flags & dlpDBFlagReadOnly)
			fprintf(stderr, " ReadOnly");
		if (info->flags & dlpDBFlagAppInfoDirty)
			fprintf(stderr, " AppInfoDirty");
		if (info->flags & dlpDBFlagBackup)
			fprintf(stderr, " Backup");
		if (info->flags & dlpDBFlagReset)
			fprintf(stderr, " Reset");
		if (info->flags & dlpDBFlagNewer)
			fprintf(stderr, " Newer");
		if (info->flags & dlpDBFlagCopyPrevention)
			fprintf(stderr, " CopyPrevention");
		if (info->flags & dlpDBFlagStream)
			fprintf(stderr, " Stream");
		if (info->flags & dlpDBFlagOpen)
			fprintf(stderr, " Open");
		if (!info->flags)
			fprintf(stderr, " None");
		fprintf(stderr, " (0x%2.2X)\n", info->flags);
		fprintf(stderr,
			"        Modnum: %ld, Index: %d, Creation date: %s",
			info->modnum, info->index,
			ctime(&info->createDate));
		fprintf(stderr, "        Modification date: %s",
			ctime(&info->modifyDate));
		fprintf(stderr, "        Backup date: %s",
			ctime(&info->backupDate));
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
#endif
		    " Modification date: %s", ctime(&info->modifyDate)));
	}

	dlp_response_free (res);

	return result;
}


/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Search for a database on the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_FindDBInfo(int sd, int cardno, int start, char *dbname,
	       unsigned long type, unsigned long creator,
	       struct DBInfo *info)
{
	int 	i;

	/* This function does not match any DLP layer function, but is
	   intended as a shortcut for programs looking for databases. It
	   uses a fairly byzantine mechanism for ordering the RAM databases
	   before the ROM ones.  You must feed the "index" slot from the
		while (dlp_ReadDBList(sd, cardno, 0x80, i, info) > 0) {

	if (start < 0x1000) {
		i = start;
		while (dlp_ReadDBList(sd, cardno, 0x80, i, info) >= 0) {
			if (((!dbname)
			     || (strcmp(info->name, dbname) == 0))
			    && ((!type) || (info->type == type))
			    && ((!creator)
				|| (info->creator == creator)))
				goto found;
			i = info->index + 1;
		}
	while (dlp_ReadDBList(sd, cardno, 0x40, i, info) > 0) {
	}

	i = start & 0xFFF;
	while (dlp_ReadDBList(sd, cardno, 0x40, i, info) >= 0) {
		if (((!dbname) || (strcmp(info->name, dbname) == 0))
		    && ((!type) || (info->type == type)) && ((!creator)
							     || (info->
								 creator ==
								 creator)))
		{
			info->index |= 0x1000;
			goto found;
		}
		i = info->index + 1;
	}

	return -1;

      found:
	
	return result;	
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Open the database for read/write/delete/mode
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
	unsigned char handle;

	dlp_buf[0] 	= (unsigned char) cardno;
	dlp_buf[1] 	= (unsigned char) mode;
	strcpy((char *) dlp_buf + 2, name);
{
	int 	result;
	struct dlpRequest *req;
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Cardno: %d, Name: '%s', Mode:",
			cardno, name);
		if (mode & dlpOpenRead)
			fprintf(stderr, " Read");
		if (mode & dlpOpenWrite)
			fprintf(stderr, " Write");
		if (mode & dlpOpenExclusive)
			fprintf(stderr, " Exclusive");
		if (mode & dlpOpenSecret)
			fprintf(stderr, " Secret");
		if (!mode)
			fprintf(stderr, " None");
		fprintf(stderr, " (0x%2.2X)\n", mode);
	}
#endif

	result =
	    dlp_exec(sd, dlpFuncOpenDB, 0x20, &dlp_buf[0], strlen(name) + 3,
		     &handle, 1);

	Expect(1);

	*dbhandle = (int) handle;
	dlp_request_free(req);
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, "  Read: Handle: %d\n", (int) handle);
		*dbhandle = get_byte(DLP_RESPONSE_DATA(res, 0, 0));
#endif

	}
	
	dlp_response_free(res);
	
	return result;
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Delete a given database on the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *

	dlp_buf[0] 	= (unsigned char) card;
	dlp_buf[1] 	= (unsigned char) 0;
	strcpy((char *) dlp_buf + 2, name);
{
	int 	result;
	struct dlpRequest *req;
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Cardno: %d, Name: '%s'\n", card,
			name);
	}
#endif

	result =
	    dlp_exec(sd, dlpFuncDeleteDB, 0x20, dlp_buf, 2 + strlen(name) + 1, 0, 0);

	Expect(0);

	
	dlp_request_free(req);
	dlp_response_free(res);
	
	return result;
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Create a database on the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
	unsigned char handle;

	set_long(dlp_buf, creator);
	set_long(dlp_buf + 4, type);
	set_byte(dlp_buf + 8, cardno);
	set_byte(dlp_buf + 9, 0);
	set_short(dlp_buf + 10, flags);
	set_short(dlp_buf + 12, version);
	strcpy((char *) dlp_buf + 14, name);
{
	int 	result;
	struct dlpRequest *req;
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			" Wrote: Card: %d, version: %d, name '%s'\n",
			cardno, version, name);
		fprintf(stderr, "        DB Flags:");
		if (flags & dlpDBFlagResource)
			fprintf(stderr, " Resource");
		if (flags & dlpDBFlagReadOnly)
			fprintf(stderr, " ReadOnly");
		if (flags & dlpDBFlagAppInfoDirty)
			fprintf(stderr, " AppInfoDirty");
		if (flags & dlpDBFlagBackup)
			fprintf(stderr, " Backup");
		if (flags & dlpDBFlagReset)
			fprintf(stderr, " Reset");
		if (flags & dlpDBFlagNewer)
			fprintf(stderr, " Newer");
		if (flags & dlpDBFlagCopyPrevention)
			fprintf(stderr, " CopyPrevention");
		if (flags & dlpDBFlagStream)
			fprintf(stderr, " Stream");
		if (flags & dlpDBFlagOpen)
			fprintf(stderr, " Open");
		if (!flags)
			fprintf(stderr, " None");
		fprintf(stderr, " (0x%2.2X), Creator: '%s'", flags,
			printlong(creator));
		fprintf(stderr, ", Type: '%s'\n", printlong(type));
	}
#endif

	result =
	    dlp_exec(sd, dlpFuncCreateDB, 0x20, dlp_buf, 14 + strlen(name) + 1,
		     &handle, 1);

	Expect(1);
	set_byte(DLP_REQUEST_DATA(req, 0, 9), 0);
	if (dbhandle)
		*dbhandle = (int) handle;
	dlp_request_free(req);
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, "  Read: Handle: %d\n", (int) handle);
		*dbhandle = get_byte(DLP_RESPONSE_DATA(res, 0, 0));
#endif
		    "DLP CreateDB Handle=%d\n", *dbhandle));
	}
	
	dlp_response_free(res);

	return result;
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Close the database opened on the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
	unsigned char handle = (unsigned char) dbhandle;
{
	int 	result;
	struct dlpRequest *req;
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d\n", dbhandle);
	}
#endif

	result = dlp_exec(sd, dlpFuncCloseDB, 0x20, &handle, 1, 0, 0);

	Expect(0);

	
	dlp_request_free(req);	
	dlp_response_free(res);
	
	return result;
}
{
	Trace(CloseDB_all);
	struct dlpRequest *req;
	result = dlp_exec(sd, dlpFuncCloseDB, 0x21, 0, 0, 0, 0);

	Expect(0);

	
	dlp_request_free(req);	
	dlp_response_free(res);
	
	return result;
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Call an application entry point via an action code
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int
dlp_CallApplication(int sd, unsigned long creator, unsigned long type,
	int 	result,	
		    void *retdata)
		data_len,
		version = pi_version(sd);
		set_long(dlp_buf + 0, creator);
		set_long(dlp_buf + 4, type);
		set_short(dlp_buf + 8, action);
		set_long(dlp_buf + 10, length);
		set_long(dlp_buf + 14, 0);
		set_long(dlp_buf + 18, 0);
		set_short(DLP_REQUEST_DATA(req, 0, 8), action);
		set_long(DLP_REQUEST_DATA(req, 0, 10), length);
		set_long(DLP_REQUEST_DATA(req, 0, 14), 0);
		set_long(DLP_REQUEST_DATA(req, 0, 18), 0);


		memcpy(dlp_buf + 22, data, length);

		Trace(CallApplicationV2);

#ifdef DLP_TRACE
		if (dlp_trace) {
			fprintf(stderr, " Wrote: Creator: '%s',",
				printlong(creator));
			fprintf(stderr,
				" Type: '%s', Action code: %d, and %d bytes of data:\n",
				printlong(type), action, length);
			dumpdata(data, length);
		}
#endif
			fprintf(stderr, "Data too large\n");
		result =
		    dlp_exec(sd, dlpFuncCallApplication, 0x21, dlp_buf, 22 + length, dlp_buf,
			     0xffff);
		}
		Expect(16);

		if (retcode)
			*retcode = get_long(dlp_buf);

		result -= 16;

		if (retlen)
			*retlen = result;
		if (retdata)
			memcpy(retdata, dlp_buf + 16,
			       result > maxretlen ? maxretlen : result);

#ifdef DLP_TRACE
		if (dlp_trace) {
			fprintf(stderr,
				"  Read: Result: %lu (0x%8.8lX), and %d bytes:\n",
				get_long(dlp_buf), get_long(dlp_buf + 4),
				result);
			dumpdata(dlp_buf + 16, result);
		}
#endif

		}
		
		dlp_response_free(res);
		set_long(dlp_buf + 0, creator);
		set_short(dlp_buf + 4, action);
		set_short(dlp_buf + 6, length);
		memcpy(dlp_buf + 6, data, length);
		return result;
		Trace(CallApplicationV10);
		
#ifdef DLP_TRACE
		if (dlp_trace) {
			fprintf(stderr,
				" Wrote: Creator: '%s', Action code: %d, and %d bytes of data:\n",
				printlong(creator), action, length);
			dumpdata(data, length);

#endif

		result =
		    dlp_exec(sd, dlpFuncCallApplication, 0x20, dlp_buf, 8, dlp_buf, 0xffff);
		}
		Expect(6);

		if (retcode)
			*retcode = get_short(dlp_buf + 2);

		result -= 6;

		if (retlen)
			*retlen = result;
		if (retdata)
			memcpy(retdata, dlp_buf + 6,
			       result > maxretlen ? maxretlen : result);

#ifdef DLP_TRACE
		if (dlp_trace) {
			fprintf(stderr,
				"  Read: Action: %d, Result: %d (0x%4.4X), and %d bytes:\n",
				get_short(dlp_buf), get_short(dlp_buf + 2),
				get_short(dlp_buf + 2), result);
			dumpdata(dlp_buf + 6, result);
		}
#endif
			      dumpdata(DLP_RESPONSE_DATA(res, 0, 6), data_len));
		}

		

		dlp_response_free(res);

		return result;
	}
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Reset the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
{
	Trace(ResetSystem);
	struct dlpRequest *req;
	result = dlp_exec(sd, dlpFuncResetSystem, 0, 0, 0, 0, 0);

	Expect(0);

	
	dlp_request_free(req);	
	dlp_response_free(res);
	
	return result;
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Add text to the Palm's synchronization log
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
{
	int 	result;
	struct dlpRequest *req;
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Entry:\n");
		dumpdata((unsigned char *) entry, strlen(entry));
	}
#endif
	
	result =
	    dlp_exec(sd, dlpFuncAddSyncLogEntry, 0x20, (unsigned char *) entry,
		     strlen(entry), 0, 0);

	Expect(0);
	
		    "DLP AddSyncLogEntry Entry: \n  %s\n", entry));
	
	dlp_response_free(res);
	
	return result;
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Read the number of records in the device database
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
	unsigned char buf[2];
{
	int 	result;
	struct dlpRequest *req;
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d\n", dbhandle);
	}
#endif

	set_byte(dlp_buf, (unsigned char) dbhandle);
	result = dlp_exec(sd, dlpFuncReadOpenDBInfo, 0x20, dlp_buf, 1, buf, 2);
	
	Expect(2);
	
	if (records)
		*records = get_short(buf);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, "  Read: %d records\n", get_short(buf));
		
#endif

	}
	
	dlp_response_free(res);
	
	return result;
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Move all records in a category to another category
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *

	set_byte(dlp_buf + 0, handle);
	set_byte(dlp_buf + 1, fromcat);
	set_byte(dlp_buf + 2, tocat);
	set_byte(dlp_buf + 3, 0);
{
	int 	result;
	struct dlpRequest *req;
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d, From: %d, To: %d\n",
			handle, fromcat, tocat);
	}
#endif

	result = dlp_exec(sd, dlpFuncMoveCategory, 0x20, dlp_buf, 4, 0, 0);
	set_byte(DLP_REQUEST_DATA(req, 0, 0), handle);
	Expect(0);
	set_byte(DLP_REQUEST_DATA(req, 0, 2), tocat);
	return result;
}
	dlp_response_free(res);
	
	return result;
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     This command is sent before each conduit is opened
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
{
	int 	result;
	struct dlpRequest *req;
	result = dlp_exec(sd, dlpFuncOpenConduit, 0, 0, 0, 0, 0);

	Expect(0);


	dlp_request_free(req);	
	dlp_response_free(res);
	
	return result;
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     End the sync with the given status
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(EndOfSync);

	set_short(dlp_buf, status);
	if (ps == 0)
	Trace(EndOfSync);

	result = dlp_exec(sd, dlpFuncEndOfSync, 0x20, dlp_buf, 2, 0, 0);

	Expect(0);

	set_short(DLP_REQUEST_DATA(req, 0, 0), status);

	result = dlp_exec(sd, req, &res);

	/* Messy code to set end-of-sync flag on socket 

	dlp_request_free(req);	
	dlp_response_free(res);
	
	return result;
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Enters a sync_aborted entry into the log
 *
 * Parameters:  None
 *
 * Returns:     Return value: A negative number on error, 0 otherwise
 *
 ***********************************************************************/
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			"DLP %d: AbortSync\nResult: Aborted Sync\n", sd);
	}
#endif
{
	struct 	pi_socket *ps;

	Trace(AbortSync);

	/* Pretend we sent the sync end */
	if ((ps = find_pi_socket(sd)))
		ps->state = PI_SOCK_CONEN;

	return 0;
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Write user information to the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *

	struct dlpRequest *req;
	struct dlpResponse *res;
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			" Wrote: UID: 0x%8.8lX, VID: 0x%8.8lX, PCID: 0x%8.8lX\n",
			User->userID, User->viewerID, User->lastSyncPC);
		fprintf(stderr, "        Last sync date: %s",
			ctime(&User->lastSyncDate));
		fprintf(stderr, "        User name '%s'\n",
			User->username);
	}
#endif

	set_long(dlp_buf, User->userID);
	set_long(dlp_buf + 4, User->viewerID);
	set_long(dlp_buf + 8, User->lastSyncPC);
	set_date(dlp_buf + 12, User->lastSyncDate);
	set_byte(dlp_buf + 20, 0xff);
	set_byte(dlp_buf + 21, strlen(User->username) + 1);
	strcpy((char *) dlp_buf + 22, User->username);

	result =
	    dlp_exec(sd, dlpFuncWriteUserInfo, 0x20, dlp_buf,
		     22 + strlen(User->username) + 1, NULL, 0);
	set_date(DLP_REQUEST_DATA(req, 0, 12), User->lastSyncDate);
	Expect(0);
	set_byte(DLP_REQUEST_DATA(req, 0, 21), len);

	dlp_request_free (req);
	dlp_response_free (res);
	
	return result;
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Read user information from the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise 
 *
 ***********************************************************************/
	struct dlpRequest *req;
	struct dlpResponse *res;
	
		userlen;
	
	req = dlp_request_new (dlpFuncReadUserInfo, 0);
	
	result = dlp_exec_new (sd, req, &res);
	
	dlp_request_free (req);
	
	if (result >= 0) {
		User->userID 		 = get_long(DLP_RESULT_DATA (res, 0, 0));
		User->viewerID 		 = get_long(DLP_RESULT_DATA (res, 0, 4));
		User->lastSyncPC 	 = get_long(DLP_RESULT_DATA (res, 0, 8));
		User->successfulSyncDate = get_date(DLP_RESULT_DATA (res, 0, 12));
		User->lastSyncDate 	 = get_date(DLP_RESULT_DATA (res, 0, 20));
		userlen                  = get_byte(DLP_RESULT_DATA (res, 0, 28));
		User->passwordLength 	 = get_byte(DLP_RESULT_DATA (res, 0, 29));
		memcpy(User->username, DLP_RESULT_DATA (res, 0, 30), userlen);
		memcpy(User->password, DLP_RESULT_DATA (res, 0, 30 + userlen),
		       User->passwordLength);

		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadUserInfo UID=0x%8.8lX VID=0x%8.8lX PCID=0x%8.8lX",
		    User->userID, User->viewerID, User->lastSyncPC);
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "  Last Sync=%s Last Successful Sync=%s",
		    ctime (&User->lastSyncDate), ctime (&User->successfulSyncDate));
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "  Username=%s",
		    User->username);
		    ctime (&User->lastSyncDate), ctime (&User->successfulSyncDate)));
	
	dlp_response_free (res);
	
	}
	
	dlp_response_free (res);
	
	return result;
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Read Network HotSync settings from the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
	int 	result,
		p;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(ReadNetSyncInfo);
	Trace(ReadNetSyncInfo);

	result = dlp_exec(sd, dlpFuncReadNetSyncInfo, 0x20, NULL, 0, dlp_buf, DLP_BUF_SIZE);

	Expect(24);

	i->lanSync = get_byte(dlp_buf);
	p = 24;
	
	i->hostName[0] = 0;
	memcpy(i->hostName, dlp_buf + p, get_short(dlp_buf + 18));
	p += get_short(dlp_buf + 18);

	i->hostAddress[0] = 0;
	memcpy(i->hostAddress, dlp_buf + p, get_short(dlp_buf + 20));
	p += get_short(dlp_buf + 20);

	i->hostSubnetMask[0] = 0;
	memcpy(i->hostSubnetMask, dlp_buf + p, get_short(dlp_buf + 22));
	p += get_short(dlp_buf + 22);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, "  Read: Active: %d\n", get_byte(dlp_buf));
		fprintf(stderr,
			"        PC hostname: '%s', address '%s', mask '%s'\n",
			i->hostName, i->hostAddress, i->hostSubnetMask);
		    "DLP ReadNetSyncInfo Active: %d\n", i->lanSync ? 1 : 0));
#endif

	}

	dlp_response_free(res);
	
	return result;
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Write Network HotSync settings to the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
		p;
	int 	result,
		str_offset = 24;
	struct dlpRequest *req;
	struct dlpResponse *res;

	if (pi_version(sd) < 0x0101)
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, "  Wrote: Active: %d\n",
			get_byte(dlp_buf));
		fprintf(stderr,
			"        PC hostname: '%s', address '%s', mask '%s'\n",
			i->hostName, i->hostAddress, i->hostSubnetMask);
	}
#endif
	    "DLP ReadNetSyncInfo Active: %d\n", i->lanSync ? 1 : 0));
	set_byte(dlp_buf, 0x80 | 0x40 | 0x20 | 0x10);	/* Change all settings */
	set_byte(dlp_buf + 1, i->lanSync);
	set_long(dlp_buf + 2, 0);			/* Reserved1 */
	set_long(dlp_buf + 6, 0);			/* Reserved2 */
	set_long(dlp_buf + 10, 0);			/* Reserved3 */
	set_long(dlp_buf + 14, 0);			/* Reserved4 */
	set_short(dlp_buf + 18, strlen(i->hostName) + 1);
	set_short(dlp_buf + 20, strlen(i->hostAddress) + 1);
	set_short(dlp_buf + 22, strlen(i->hostSubnetMask) + 1);
	p = 24;
	strcpy((char *) dlp_buf + p, i->hostName);
	p += strlen(i->hostName) + 1;
	strcpy((char *) dlp_buf + p, i->hostAddress);
	p += strlen(i->hostAddress) + 1;
	strcpy((char *) dlp_buf + p, i->hostSubnetMask);
	p += strlen(i->hostSubnetMask) + 1;
	str_offset += strlen(i->hostName) + 1;
	result =
	    dlp_exec(sd, dlpFuncWriteNetSyncInfo, 0x20, dlp_buf, p, dlp_buf, DLP_BUF_SIZE);

	Expect(0);
	str_offset += strlen(i->hostAddress) + 1;

	dlp_request_free(req);
	dlp_response_free(res);
	
	return result;
}

#ifdef _PILOT_SYSPKT_H
/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Remote Procedure Calls interface
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int dlp_RPC(int sd, struct RPC_params *p, unsigned long *result)
{
	int 	i,
	long 	D0 = 0,
		A0 = 0;
	unsigned char *c;
	unsigned char dlp_buf[DLP_BUF_SIZE];

	/* RPC through DLP breaks all the rules and isn't well documented to
	   boot */
	dlp_buf[0] = 0x2D;
	dlp_buf[1] = 1;
	dlp_buf[2] = 0;		/* Unknown filler */
	dlp_buf[3] = 0;

	InvertRPC(p);

	set_short(dlp_buf + 4, p->trap);
	set_long(dlp_buf + 6, D0);
	set_long(dlp_buf + 10, A0);
	set_short(dlp_buf + 14, p->args);

	c = dlp_buf + 16;
	for (i = p->args - 1; i >= 0; i--) {
		set_byte(c, p->param[i].byRef);
		c++;
		set_byte(c, p->param[i].size);
		c++;
		if (p->param[i].data)
			memcpy(c, p->param[i].data, p->param[i].size);
		c += p->param[i].size;
		if (p->param[i].size & 1)
			*c++ = 0;
	}

	pi_write(sd, dlp_buf, c - dlp_buf);

	err = 0;

	if (p->reply) {
		int l = pi_read(sd, dlp_buf, c - dlp_buf + 2);

		if (l < 0)
			err = l;
		else if (l < 6)
			err = -1;
		else if (dlp_buf[0] != 0xAD)
			err = -2;
		else if (get_short(dlp_buf + 2))
			err = -get_short(dlp_buf + 2);
		else {
			D0 = get_long(dlp_buf + 8);
			A0 = get_long(dlp_buf + 12);
			c = dlp_buf + 18;
			for (i = p->args - 1; i >= 0; i--) {
				if (p->param[i].byRef && p->param[i].data)
					memcpy(p->param[i].data, c + 2,
					       p->param[i].size);
				c += 2 + ((p->param[i].size + 1) & ~1);
			}
		}
	}

	UninvertRPC(p);

	if (result) {
		if (p->reply == RPC_PtrReply) {
			*result = A0;
		} else if (p->reply == RPC_IntReply) {
			*result = D0;
		}
	}

	return err;
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Read a feature from Feature Manager on the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise 
 *
 ***********************************************************************/
int
{
	int 	result;
		struct RPC_params p;
	if (pi_version(sd) < 0x0101) {
		struct RPC_params p;		
		int val;
		if (feature) {
			int val;
			unsigned long result;

#ifdef DLP_TRACE
			if (dlp_trace) {
				fprintf(stderr,
					" Wrote: Creator: '%s', Number: %d\n",
					printlong(creator), num);
			}
#endif
		Trace(ReadFeatureV1);
			*feature = 0x12345678;
		if (!feature)
			PackRPC(&p, 0xA27B, RPC_IntReply,
				RPC_Long(creator),
				RPC_Short((unsigned short) num),
				RPC_LongPtr(feature), RPC_End);
		} else if (result) {
			val = dlp_RPC(sd, &p, &result);

#ifdef DLP_TRACE
			if (dlp_trace) {
				if (val < 0)
					fprintf(stderr,
						"Result: Error: %s (%d)\n",
						dlp_errorlist[-val], val);
				else if (result)
					fprintf(stderr,
						"FtrGet error 0x%8.8lX\n",
						(unsigned long) result);
				else
					fprintf(stderr,
						"  Read: Feature: 0x%8.8lX\n",
						(unsigned long) *feature);
			}
#endif

			if (val < 0)
				return val;
			if (result)
				return (-(long) result);
		} else {

			    " DLP ReadFeature Feature: 0x%8.8lX\n",
	}
		}
	Trace(ReadFeatureV2);
		return 0;
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Creator: '%s', Number: %d\n",
			printlong(creator), num);
	}
#endif

	set_long(dlp_buf, creator);
	set_short(dlp_buf + 4, num);
		req = dlp_request_new(dlpFuncReadFeature, 1, 6);
	result =
	    dlp_exec(sd, dlpFuncReadFeature, 0x20, dlp_buf, 6, dlp_buf, DLP_BUF_SIZE);
		set_long(DLP_REQUEST_DATA(req, 0, 0), creator);
	Expect(4);
			    "DLP ReadFeature Feature: 0x%8.8lX\n",
	if (feature)
		*feature = (unsigned long) get_long(dlp_buf);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, "  Read: Feature: 0x%8.8lX\n",
			(unsigned long) get_long(dlp_buf));
		}
#endif

	return result;

		return result;
	}
	
}
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Reset the LastSyncPC ID so we can start again
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int dlp_ResetLastSyncPC(int sd)
{
	int 	err;

		return err;

	User.lastSyncPC = 0;

	return dlp_WriteUserInfo(sd, &User);
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Reset the modified records index
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(ResetRecordIndex);
	Trace(ResetDBIndex);
	if ((ps = find_pi_socket(sd)))
	/* FIXME: Specify the handle */

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d\n", dbhandle);
	}
#endif

	result = dlp_exec(sd, dlpFuncResetRecordIndex, 0x20, dlp_buf, 1, NULL, 0);

	Expect(0);


	dlp_request_free(req);	
	dlp_response_free(res);
	
	return result;
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Read in a list of RecordIDs from the Palm
 *
 * Parameters:  None
 *
 * Returns:	A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
	int 	result,
		i,
		ret;
	unsigned int nbytes;
	unsigned char *p;

	set_byte(dlp_buf, dbhandle);
	set_byte(dlp_buf + 1, sort ? 0x80 : 0);
	set_short(dlp_buf + 2, start);
	set_short(dlp_buf + 4, max);
{
	int 	result;
	struct dlpRequest *req;
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			" Wrote: Handle: %d, Sort: %s, Start: %d, Max: %d\n",
			dbhandle, sort ? "Yes" : "No", start, max);
	}
#endif
	set_byte(DLP_REQUEST_DATA(req, 0, 0), dbhandle);
	nbytes = max * 4 + 2;
	if (nbytes > DLP_BUF_SIZE)
		nbytes = DLP_BUF_SIZE;
	set_short(DLP_REQUEST_DATA(req, 0, 2), start);
	result = dlp_exec(sd, dlpFuncReadRecordIDList, 0x20, dlp_buf, 6, dlp_buf, nbytes);

	Expect(2);

	ret = get_short(dlp_buf);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Read: %d IDs:\n", ret);
		dumpdata(dlp_buf + 2, ret * 4);
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
#endif
		    "DLP ReadRecordIDList %d IDs:\n", ret));
	for (i = 0, p = dlp_buf + 2; i < ret; i++, p += 4)
		IDs[i] = get_long(p);

	if (count)
		*count = i;

	return nbytes;

	dlp_response_free(res);
	
	return result;
}

/***********************************************************************
 *
 * Function:    dlp_WriteRecord
 *
 * Parmeters:   None
 *		will create a new id and the variable the NewID pointer
 *		points to will be set to the new id.
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
	unsigned char buf[4];
{
	set_byte(dlp_buf, dbhandle);
	set_byte(dlp_buf + 1, 0);
	set_long(dlp_buf + 2, recID);
	set_byte(dlp_buf + 6, flags);
	set_byte(dlp_buf + 7, catID);
	struct dlpRequest *req;
	struct dlpResponse *res;



	if (length == -1)
		length = strlen((char *) data) + 1;


	memcpy(dlp_buf + 8, data, length);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
	Trace(WriteRecord);
	set_byte(DLP_REQUEST_DATA(req, 0, 6), flags);
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			" Wrote: Handle: %d, RecordID: 0x%8.8lX, Category: %d\n",
			dbhandle, (unsigned long) recID, catID);
		fprintf(stderr, "        Flags:");
		if (flags & dlpRecAttrDeleted)
			fprintf(stderr, " Deleted");
		if (flags & dlpRecAttrDirty)
			fprintf(stderr, " Dirty");
		if (flags & dlpRecAttrBusy)
			fprintf(stderr, " Busy");
		if (flags & dlpRecAttrSecret)
			fprintf(stderr, " Secret");
		if (flags & dlpRecAttrArchived)
			fprintf(stderr, " Archive");
		if (!flags)
			fprintf(stderr, " None");
		fprintf(stderr, " (0x%2.2X), and %d bytes of data: \n",
			flags, length);
		dumpdata(data, length);
	}
#endif

	result = dlp_exec(sd, dlpFuncWriteRecord, 0x20, dlp_buf, 8 + length, buf, 4);

	Expect(4);

	if (NewID) {
		if (result == 4) {
			*NewID = get_long(buf);	/* New record ID */
		} else {
			*NewID = 0;
		}

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, "  Read: Record ID: 0x%8.8lX\n",
			(unsigned long) get_long(buf));
	}
#endif

	}
	
	dlp_response_free(res);
	
	return result;
}

/***********************************************************************
 *
 * Function:    dlp_DeleteRecord
 * Parmeters:   None
 * Summary:     Deletes a record from the database or all records if the all
 *		flag is non-zero
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/

	set_byte(dlp_buf, dbhandle);
	set_byte(dlp_buf + 1, flags);
	set_long(dlp_buf + 2, recID);
	int 	result,
		flags = all ? 0x80 : 0;
	struct dlpRequest *req;
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			" Wrote: Handle: %d, RecordID: %8.8lX, All: %s\n",
			dbhandle, (unsigned long) recID,
			all ? "Yes" : "No");
	}
#endif

	result = dlp_exec(sd, dlpFuncDeleteRecord, 0x20, dlp_buf, 6, 0, 0);

	Expect(0);

	
	dlp_request_free(req);
	dlp_response_free(res);
	
	return result;
}

/***********************************************************************
 *
 * Function:    dlp_DeleteCategory
 * Parmeters:   None
 * Summary:     Delete all records in a category. The category name 
 *		is not changed.
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
	int 	result,
		flags = 0x40;
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	if (pi_version(sd) < 0x0101) {
		/* Emulate if not connected to PalmOS 2.0 */
		int i, r, cat, attr;
#ifdef DLP_TRACE
		if (dlp_trace) {
			fprintf(stderr,
				" Emulating with: Handle: %d, Category: %d\n",
				dbhandle, category & 0xff);
		}
#endif

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP DeleteCategory Emulating with: Handle: %d, Category: %d\n",
		    dbhandle, category & 0xff));

		for (i = 0;
		     dlp_ReadRecordByIndex(sd, dbhandle, i, NULL, &id,
					   NULL, &attr, &cat) >= 0; i++) {
			if ((cat != category) || (attr & dlpRecAttrDeleted)
			    || (attr & dlpRecAttrArchived))
				continue;
			r = dlp_DeleteRecord(sd, dbhandle, 0, id);
				return r;
	}
		set_byte(DLP_REQUEST_DATA(req, 0, 1), flags);
	set_byte(dlp_buf, dbhandle);
	set_byte(dlp_buf + 1, flags);
	set_long(dlp_buf + 2, category & 0xff);
		result = dlp_exec(sd, req, &res);
	Trace(DeleteCategoryV2);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d, Category: %d\n",
			dbhandle, category & 0xff);
		dlp_request_free(req);
#endif

	result = dlp_exec(sd, dlpFuncDeleteRecord, 0x20, dlp_buf, 6, 0, 0);

	Expect(0);

	return result;
		dlp_response_free(res);

		return result;
	}
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Read the record resources by ResourceID
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
	int 	result;

	set_byte(dlp_buf, fHandle);
	set_byte(dlp_buf + 1, 0x00);
	set_long(dlp_buf + 2, type);
	set_short(dlp_buf + 6, id);
	set_short(dlp_buf + 8, 0);				/* Offset into record 	*/
	set_short(dlp_buf + 10, buffer ? DLP_BUF_SIZE : 0);	/* length to return 	*/
	int 	result,
		data_len;
	struct dlpRequest *req;
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d, Type: '%s', ID: %d\n",
			fHandle, printlong(type), id);
	}
#endif
	set_long(DLP_REQUEST_DATA(req, 0, 2), type);
	result =
	    dlp_exec(sd, dlpFuncReadResource, 0x21, dlp_buf, 12, dlp_buf, DLP_BUF_SIZE);
	set_short(DLP_REQUEST_DATA(req, 0, 8), 0);
	Expect(10);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			"  Read: Type: '%s', ID: %d, Index: %d, and %d bytes:\n",
			printlong(type), id, get_short(dlp_buf + 6),
			result - 10);
		dumpdata(dlp_buf + 10, result - 10);
		CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, 
#endif
		      dumpdata(DLP_RESPONSE_DATA(res, 0, 10), data_len));
	if (index)
		*index = get_short(dlp_buf + 6);
	if (size)
		*size = get_short(dlp_buf + 8);
	if (buffer)
		memcpy(buffer, dlp_buf + 10, result - 10);

	return result - 10;

	dlp_response_free(res);
	
	return data_len;
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Read the record resources by index
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
	int 	result;

	set_byte(dlp_buf, fHandle);
	set_byte(dlp_buf + 1, 0x00);
	set_short(dlp_buf + 2, index);
	set_short(dlp_buf + 4, 0);				/* Offset into record 	*/
	set_short(dlp_buf + 6, buffer ? DLP_BUF_SIZE : 0);	/* length to return 	*/
	int 	result,
		data_len;
	struct dlpRequest *req;
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d, Index: %d\n", fHandle,
			index);
	}
#endif
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
	result =
	    dlp_exec(sd, dlpFuncReadResource, 0x20, dlp_buf, 8, dlp_buf, DLP_BUF_SIZE);
	set_short(DLP_REQUEST_DATA(req, 0, 4), 0);
	Expect(10);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			"  Read: Type: '%s', ID: %d, Index: %d, and %d bytes:\n",
			printlong(get_long(dlp_buf)),
			get_short(dlp_buf + 4), index, result - 10);
		dumpdata(dlp_buf + 10, result - 10);
		CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, 
#endif
		      dumpdata(DLP_RESPONSE_DATA(res, 0, 10), data_len));
	if (type)
		*type = get_long(dlp_buf);
	if (id)
		*id = get_short(dlp_buf + 4);
	if (size)
		*size = get_short(dlp_buf + 8);
	if (buffer)
		memcpy(buffer, dlp_buf + 10, result - 10);

	return result - 10;

	dlp_response_free(res);
	
	return data_len;
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Write a resource
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
	struct dlpRequest *req;
	set_byte(dlp_buf, dbhandle);
	set_byte(dlp_buf + 1, 0);
	set_long(dlp_buf + 2, type);
	set_short(dlp_buf + 6, id);
	set_short(dlp_buf + 8, length);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
	set_long(DLP_REQUEST_DATA(req, 0, 2), type);
	set_short(DLP_REQUEST_DATA(req, 0, 6), id);
	set_short(DLP_REQUEST_DATA(req, 0, 8), length);

		fprintf(stderr, "Data too large\n");
	memcpy(dlp_buf + 10, data, length);

	Trace(WriteResource);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			" Wrote: Type: '%s', ID: %d, and %d bytes:\n",
			printlong(type), id, length);
		dumpdata(data, length);
	}
#endif

	result = dlp_exec(sd, dlpFuncWriteResource, 0x20, dlp_buf, 10 + length, NULL, 0);
	}
	Expect(0);
	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	dlp_response_free(res);

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_DeleteResource
 * Parmeters:   None
 * Summary:     Delete a single resource from the database or all 
 *		resources if the all flag is non-zero
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_DeleteResource(int sd, int dbhandle, int all, unsigned long restype,

	set_byte(dlp_buf, dbhandle);
	set_byte(dlp_buf + 1, flags);
	set_long(dlp_buf + 2, restype);
	set_short(dlp_buf + 6, resID);
	int 	result,
		flags = all ? 0x80 : 0;
	struct dlpRequest *req;
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Type: '%s', ID: %d, All: %s\n",
			printlong(restype), resID, all ? "Yes" : "No");
	}
#endif

	result = dlp_exec(sd, dlpFuncDeleteResource, 0x20, dlp_buf, 8, NULL, 0);

	Expect(0);
	set_byte(DLP_REQUEST_DATA(req, 0, 0), dbhandle);
	
	dlp_request_free(req);
	dlp_response_free(res);
	
	return result;
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Read the AppInfo block that matches the database
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
	int 	result;

	set_byte(dlp_buf, fHandle);
	set_byte(dlp_buf + 1, 0x00);
	set_short(dlp_buf + 2, offset);
	set_short(dlp_buf + 4, dlen);
	int 	result,
		data_len;
	struct dlpRequest *req;
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			" Wrote: Handle: %d, Offset: %d, Max Length: %d\n",
			fHandle, offset, dlen);
	}
#endif
	set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
	result =
	    dlp_exec(sd, dlpFuncReadAppBlock, 0x20, dlp_buf, 6, dlp_buf, DLP_BUF_SIZE);
	set_short(DLP_REQUEST_DATA(req, 0, 2), offset);
	Expect(2);

	if (dbuf)
		memcpy(dbuf, dlp_buf + 2, result - 2);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, "  Read: %d bytes:\n", result - 2);
		dumpdata(dlp_buf + 2, result - 2);
		CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, 
#endif
		      dumpdata(DLP_RESPONSE_DATA(res, 0, 2), data_len));
	return result - 2;

	dlp_response_free(res);
	
	return data_len;
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Write the AppInfo block that matches the database
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
	struct dlpRequest *req;
	set_byte(dlp_buf, fHandle);
	set_byte(dlp_buf + 1, 0x00);
	set_short(dlp_buf + 2, length);
	
	if (length + 4 > DLP_BUF_SIZE) {
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
	set_short(DLP_REQUEST_DATA(req, 0, 2), length);

		fprintf(stderr, "Data too large\n");
	memcpy(dlp_buf + 4, data, length);

	Trace(WriteAppBlock);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d, %d bytes:\n", fHandle,
			length);
		dumpdata(data, length);
	}
#endif

	result = dlp_exec(sd, dlpFuncWriteAppBlock, 0x20, dlp_buf, length + 4, NULL, 0);
	}
	Expect(0);
	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	dlp_response_free(res);

	return result;
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Read the SortBlock that matches the database
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
	int 	result;

	set_byte(dlp_buf, fHandle);
	set_byte(dlp_buf + 1, 0x00);
	set_short(dlp_buf + 2, offset);
	set_short(dlp_buf + 4, dlen);
	int 	result,
		data_len;
	struct dlpRequest *req;
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			" Wrote: Handle: %d, Offset: %d, Max Length: %d\n",
			fHandle, offset, dlen);
	}
#endif
	set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
	result =
	    dlp_exec(sd, dlpFuncReadSortBlock, 0x20, dlp_buf, 6, dlp_buf, DLP_BUF_SIZE);
	set_short(DLP_REQUEST_DATA(req, 0, 2), offset);
	Expect(2)
#ifdef DLP_TRACE
	    if (dlp_trace) {
		fprintf(stderr, "  Read: %d bytes:\n", result - 2);
		dumpdata(dlp_buf + 2, result - 2);
		CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, 
#endif
		      dumpdata(DLP_RESPONSE_DATA(res, 0, 2), data_len));
	if (dbuf)
		memcpy(dbuf, dlp_buf + 2, result - 2);
	return result - 2;

	dlp_response_free(res);
	
	return data_len;
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Write the SortBlock that matches the database
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
	struct dlpRequest *req;
	set_byte(dlp_buf, fHandle);
	set_byte(dlp_buf + 1, 0x00);
	set_short(dlp_buf + 2, length);
	
	if (length + 4 > DLP_BUF_SIZE) {
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
	set_short(DLP_REQUEST_DATA(req, 0, 2), length);

		fprintf(stderr, "Data too large\n");
	memcpy(dlp_buf + 4, data, length);
	}
	Trace(WriteSortBlock);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d, %d bytes:\n", fHandle,
			length);
		dumpdata(data, length);
	}
#endif

	result = dlp_exec(sd, dlpFuncWriteSortBlock, 0x20, dlp_buf, length + 4, NULL, 0);

	Expect(0);
	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	dlp_response_free(res);

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_CleanUpDatabase
 * Parmeters:   None
 * Summary:     Deletes all records which are marked as archived or 
 *		deleted in the record database
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
	unsigned char handle = fHandle;
{
	int 	result;
	struct dlpRequest *req;
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d\n", fHandle);
	}
#endif

	result = dlp_exec(sd, dlpFuncCleanUpDatabase, 0x20, &handle, 1, NULL, 0);

	Expect(0);

	
	dlp_request_free(req);
	dlp_response_free(res);
	
	return result;
}

/***********************************************************************
 *
 * Function:    dlp_ResetSyncFlags
 * Parmeters:   None
 * Summary:     Clear all the sync flags (modified, deleted, etc) in 
 *		the pilot database
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
	unsigned char handle = fHandle;
{
	int 	result;
	struct dlpRequest *req;
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d\n", fHandle);
	}
#endif

	result = dlp_exec(sd, dlpFuncResetSyncFlags, 0x20, &handle, 1, NULL, 0);

	Expect(0);

	
	dlp_request_free(req);
	dlp_response_free(res);
	
	return result;
}

/***********************************************************************
 *
 * Function:    dlp_ReadNextRecInCategory
 * Parmeters:   None
 * Summary:     Iterate through all records in category returning 
 *		subsequent records on each call
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int
dlp_ReadNextRecInCategory(int sd, int fHandle, int incategory,

#ifdef DLP_TRACE
	int 	flags;
#endif
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	if (pi_version(sd) < 0x0101) {
		/* Emulate for PalmOS 1.0 */
		int 	cat,
			rec;
#ifdef DLP_TRACE
		if (dlp_trace) {
			fprintf(stderr,
				" Emulating with: Handle: %d, Category: %d\n",
				fHandle, incategory);
		}
#endif

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadNextRecInCategory Emulating with: Handle: %d, Category: %d\n",
		    fHandle, incategory));

		if ((ps = find_pi_socket(sd)) == 0)
			return -130;
						  ps->dlprecord, 0, 0, 0,
						  0, &cat);
			/* Fetch next modified record (in any category) */
			rec = dlp_ReadRecordByIndex(sd, fHandle,
						    ps->dlprecord, 0, 0, 0,
						    0, &cat);

			if (rec < 0)
				break;

			if (cat != incategory) {
				ps->dlprecord++;
						  ps->dlprecord, buffer,
						  id, size, attr, &cat);

			rec = dlp_ReadRecordByIndex(sd, fHandle,
						    ps->dlprecord, buffer,
						    id, size, attr, &cat);

			if (rec >= 0) {
				if (index)
					*index = ps->dlprecord;
				ps->dlprecord++;
			} else {
				/* If none found, reset modified pointer so
				   that another search on a different (or
				   the same!) category will work */

				/* Freeow! Do _not_ reset, as the Palm
				   itself does not!

				   ps->dlprecord = 0; */
			}

	}

	Trace(ReadNextRecInCategoryV2);
			      dumpdata(DLP_RESPONSE_DATA(res, 0, 10), data_len));
	set_byte(dlp_buf, fHandle);
	set_byte(dlp_buf + 1, incategory);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d, Category: %d\n",
			fHandle, incategory);
	}
#endif

	result =
	    dlp_exec(sd, dlpFuncReadNextRecInCategory, 0x20, dlp_buf, 2, dlp_buf, DLP_BUF_SIZE);

	Expect(10);

#ifdef DLP_TRACE
	if (dlp_trace) {
		flags = get_byte(dlp_buf + 8);
		fprintf(stderr,
			"  Read: ID: 0x%8.8lX, Index: %d, Category: %d\n        Flags:",
			(unsigned long) get_long(dlp_buf),
			get_short(dlp_buf + 4),
			(int) get_byte(dlp_buf + 9));
		if (flags & dlpRecAttrDeleted)
			fprintf(stderr, " Deleted");
		if (flags & dlpRecAttrDirty)
			fprintf(stderr, " Dirty");
		if (flags & dlpRecAttrBusy)
			fprintf(stderr, " Busy");
		if (flags & dlpRecAttrSecret)
			fprintf(stderr, " Secret");
		if (flags & dlpRecAttrArchived)
			fprintf(stderr, " Archive");
		if (!flags)
			fprintf(stderr, " None");
		fprintf(stderr, " (0x%2.2X), and %d bytes:\n", flags,
			result - 10);
		dumpdata(dlp_buf + 10, result - 10);

#endif

	if (id)
		*id = get_long(dlp_buf);
	if (index)
		*index = get_short(dlp_buf + 4);
	if (size)
		*size = get_short(dlp_buf + 6);
	if (attr)
		*attr = get_byte(dlp_buf + 8);
	if (buffer)
		memcpy(buffer, dlp_buf + 10, result - 10);

	return result - 10;
		dlp_response_free(res);
		
		return data_len;
	}
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Read application preference
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	if (pi_version(sd) < 0x0101) {
		/* Emulate on PalmOS 1.0 */
		int 	db,
#ifdef DLP_TRACE
		if (dlp_trace) {
			fprintf(stderr,
				"  Emulating with: Creator: '%s', Id: %d, Size: %d, Backup: %d\n",
				printlong(creator), id,
				buffer ? maxsize : 0, backup ? 0x80 : 0);
		}
#endif
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadAppPreference Emulating with: Creator: '%s', Id: %d, Size: %d, Backup: %d\n",
		    printlong(creator), id,
		    buffer ? maxsize : 0, backup ? 0x80 : 0));

		rec = dlp_OpenDB(sd, 0, dlpOpenRead, "System Preferences",
			       &db);
		if (rec < 0)
			return rec;

		rec = dlp_ReadResourceByType(sd, db, creator, id, buffer,
					   NULL, size);

		if (rec < 0) {
			dlp_CloseDB(sd, db);
			return rec;
		}

		if (size)
			*size -= 2;

		if (version)
			*version = get_short(buffer);

		if (rec > 2) {
			rec -= 2;
			memmove(buffer, ((char *) buffer) + 2, rec);
		} else {
			rec = 0;
		}
	}

	Trace(ReadAppPreferenceV2);

	set_long(dlp_buf, creator);
	set_short(dlp_buf + 4, id);
	set_short(dlp_buf + 6, buffer ? maxsize : 0);
	set_byte(dlp_buf + 8, backup ? 0x80 : 0);
	set_byte(dlp_buf + 9, 0);	/* Reserved */
			data_len = result;
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			" Wrote: Creator: '%s', Id: %d, Size: %d, Backup: %d\n",
			printlong(creator), id, buffer ? maxsize : 0,
			backup ? 0x80 : 0);
	}
#endif

	result =
	    dlp_exec(sd, dlpFuncReadAppPreference, 0x20, dlp_buf, 10, dlp_buf, DLP_BUF_SIZE);

	Expect(6);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			" Read: Version: %d, Total size: %d, Read %d bytes:\n",
			get_short(dlp_buf), get_short(dlp_buf + 2),
			get_short(dlp_buf + 4));
		dumpdata(dlp_buf + 6, get_short(dlp_buf + 4));
		
#endif

	if (version)
		*version = get_short(dlp_buf);
	if (size && !buffer)
		*size = get_short(dlp_buf + 2);	/* Total size */
	if (size && buffer)
		*size = get_short(dlp_buf + 4);	/* Size returned */
	if (buffer)
		memcpy(buffer, dlp_buf + 6, get_short(dlp_buf + 4));

	return get_short(dlp_buf + 4);
		dlp_response_free(res);

		return data_len;
	}
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Write application preference
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

		/* Emulate on PalmOS 1.0 */
		int 	db,
			rec;
#ifdef DLP_TRACE
		if (dlp_trace) {
			fprintf(stderr,
				" Wrote: Creator: '%s', Id: %d, Version: %d, Backup: %d, and %d bytes:\n",
				printlong(creator), id, version,
				backup ? 0x80 : 0, size);
			dumpdata(buffer, size);
		}
#endif

		unsigned char dlp_buf[DLP_BUF_SIZE];

		Trace(WriteAppPreferenceV1);

		rec = dlp_OpenDB(sd, 0, dlpOpenWrite, "System Preferences",
			       &db);
		if (rec < 0)
			return rec;

		if (buffer && size) {
			memcpy(dlp_buf + 2, buffer, size);
			set_short(dlp_buf, version);
			rec = dlp_WriteResource(sd, db, creator, id, dlp_buf,
					      size);
		} else
			rec = dlp_WriteResource(sd, db, creator, id, NULL,
					      0);
	}

	Trace(WriteAppPreferenceV2);
		set_short(DLP_REQUEST_DATA(req, 0, 6), version);
	set_long(dlp_buf, creator);
	set_short(dlp_buf + 4, id);
	set_short(dlp_buf + 6, version);
	set_short(dlp_buf + 8, size);
	set_byte(dlp_buf + 10, backup ? 0x80 : 0);
	set_byte(dlp_buf + 11, 0);	/* Reserved */

	if (size + 12 > DLP_BUF_SIZE) {
		fprintf(stderr, "Data too large\n");
		return -131;
	}
		}
	memcpy(dlp_buf + 12, buffer, size);
		result = dlp_exec (sd, req, &res);
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			" Wrote: Creator: '%s', Id: %d, Version: %d, Backup: %d, and %d bytes:\n",
			printlong(creator), id, version, backup ? 0x80 : 0,
			size);
		dumpdata(buffer, size);
		dlp_request_free(req);
#endif

	result = dlp_exec(sd, dlpFuncWriteAppPreference, 0x20, dlp_buf, 12 + size, NULL, 0);

	Expect(0);

	return result;
		dlp_response_free(res);

		return result;
	}
}

/***********************************************************************
 *
 * Function:    dlp_ReadNextModifiedRecInCategory
 * Parmeters:   None
 *              id         <-- Record ID of record on palm device.
 *              index      <-- Specifies record to get.
 *              size       <-- Size of data returned in buffer.
 *              attr       <-- Attributes from record on palm device.
 *		otherwise
 *
 * Parameter documentation by DHS.
 *
 ***********************************************************************/
int
dlp_ReadNextModifiedRecInCategory(int sd, int fHandle, int incategory,

#ifdef DLP_TRACE
	int 	flags;
#endif
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	if (pi_version(sd) < 0x0101) {
		/* Emulate for PalmOS 1.0 */
		int 	cat,
#ifdef DLP_TRACE
		if (dlp_trace) {
			fprintf(stderr,
				" Emulating with: Handle: %d, Category: %d\n",
				fHandle, incategory);
		}
#endif

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadNextModifiedRecInCategory Emulating with: Handle: %d, Category: %d\n",
		    fHandle, incategory));

		do {
			/* Fetch next modified record (in any category) */
			rec = dlp_ReadNextModifiedRec(sd, fHandle, buffer,
						    id, index, size, attr,
						    &cat);

			/* If none found, reset modified pointer so that another search on a different
			   (or the same!) category will start from the beginning */

			/* Working on same assumption as ReadNextRecInCat, elide this:
			   if (r < 0)
			   dlp_ResetDBIndex(sd, fHandle);
			 */

			/* Loop until we fail to get a record or a record is found in the proper category */
	}
			if (attr)
	Trace(ReadNextModifiedRecInCategoryV2);

	set_byte(dlp_buf, fHandle);
	set_byte(dlp_buf + 1, incategory);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d, Category: %d\n",
			fHandle, incategory);
		
#endif

	result =
	    dlp_exec(sd, dlpFuncReadNextModifiedRecInCategory, 0x20, dlp_buf, 2, dlp_buf, DLP_BUF_SIZE);

	Expect(10);

#ifdef DLP_TRACE
	if (dlp_trace) {
		flags = get_byte(dlp_buf + 8);
		fprintf(stderr,
			"  Read: ID: 0x%8.8lX, Index: %d, Category: %d\n        Flags:",
			(unsigned long) get_long(dlp_buf),
			get_short(dlp_buf + 4),
			(int) get_byte(dlp_buf + 9));
		if (flags & dlpRecAttrDeleted)
			fprintf(stderr, " Deleted");
		if (flags & dlpRecAttrDirty)
			fprintf(stderr, " Dirty");
		if (flags & dlpRecAttrBusy)
			fprintf(stderr, " Busy");
		if (flags & dlpRecAttrSecret)
			fprintf(stderr, " Secret");
		if (flags & dlpRecAttrArchived)
			fprintf(stderr, " Archive");
		if (!flags)
			fprintf(stderr, " None");
		fprintf(stderr, " (0x%2.2X), and %d bytes:\n", flags,
			result - 10);
		dumpdata(dlp_buf + 10, result - 10);
	}
#endif

	if (id)
		*id = get_long(dlp_buf);
	if (index)
		*index = get_short(dlp_buf + 4);
	if (size)
		*size = get_short(dlp_buf + 6);
	if (attr)
		*attr = get_byte(dlp_buf + 8);
	if (buffer)
		memcpy(buffer, dlp_buf + 10, result - 10);

	return result - 10;
		dlp_response_free(res);
		
		return data_len;
	}
}

/***********************************************************************
 *
 * Function:    dlp_ReadNextModifiedRec
 * Parmeters:   None
 * Summary:     Iterate through modified records in category, returning
 *		subsequent modified records on each call
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
	int 	result;
	unsigned char handle = fHandle;

#ifdef DLP_TRACE
	int flags;
#endif
	int 	result,
		data_len;
	
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d\n", fHandle);
	}
#endif
		if (category)
	result =
	    dlp_exec(sd, dlpFuncReadNextModifiedRec, 0x20, &handle, 1, dlp_buf, DLP_BUF_SIZE);

	Expect(10);

#ifdef DLP_TRACE
	if (dlp_trace) {
		flags = get_byte(dlp_buf + 8);
		fprintf(stderr,
			"  Read: ID: 0x%8.8lX, Index: %d, Category: %d\n        Flags:",
			(unsigned long) get_long(dlp_buf),
			get_short(dlp_buf + 4),
			(int) get_byte(dlp_buf + 9));
		if (flags & dlpRecAttrDeleted)
			fprintf(stderr, " Deleted");
		if (flags & dlpRecAttrDirty)
			fprintf(stderr, " Dirty");
		if (flags & dlpRecAttrBusy)
			fprintf(stderr, " Busy");
		if (flags & dlpRecAttrSecret)
			fprintf(stderr, " Secret");
		if (flags & dlpRecAttrArchived)
			fprintf(stderr, " Archive");
		if (!flags)
			fprintf(stderr, " None");
		fprintf(stderr, " (0x%2.2X), and %d bytes:\n", flags,
			result - 10);
		dumpdata(dlp_buf + 10, result - 10);

#endif

	if (id)
		*id = get_long(dlp_buf);
	if (index)
		*index = get_short(dlp_buf + 4);
	if (size)
		*size = get_short(dlp_buf + 6);
	if (attr)
		*attr = get_byte(dlp_buf + 8);
	if (category)
		*category = get_byte(dlp_buf + 9);
	if (buffer)
		memcpy(buffer, dlp_buf + 10, result - 10);

	return result - 10;
	
	dlp_response_free(res);
	
	return data_len;
}

/***********************************************************************
 *
 * Parmeters:   None
 *
 * Summary:     Searches device database for match on a record by id
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
	int 	result;

#ifdef DLP_TRACE
	int 	flags;
#endif

	set_byte(dlp_buf, fHandle);
	set_byte(dlp_buf + 1, 0x00);
	set_long(dlp_buf + 2, id);
	set_short(dlp_buf + 6, 0);	/* Offset into record */
	set_short(dlp_buf + 8, buffer ? DLP_BUF_SIZE : 0);	/* length to return */
	int 	result,
		data_len;
	struct dlpRequest *req;
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			" Wrote: Handle: %d, Record ID: 0x%8.8lX\n",
			fHandle, id);
	}
#endif
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
	result =
	    dlp_exec(sd, dlpFuncReadRecord, 0x20, dlp_buf, 10, dlp_buf, DLP_BUF_SIZE);
	set_short(DLP_REQUEST_DATA(req, 0, 6), 0); /* Offset into record */
	Expect(10);
		if (category)
#ifdef DLP_TRACE
	if (dlp_trace) {
		flags = get_byte(dlp_buf + 8);
		fprintf(stderr,
			"  Read: ID: 0x%8.8lX, Index: %d, Category: %d\n        Flags:",
			(unsigned long) get_long(dlp_buf),
			get_short(dlp_buf + 4),
			(int) get_byte(dlp_buf + 9));
		if (flags & dlpRecAttrDeleted)
			fprintf(stderr, " Deleted");
		if (flags & dlpRecAttrDirty)
			fprintf(stderr, " Dirty");
		if (flags & dlpRecAttrBusy)
			fprintf(stderr, " Busy");
		if (flags & dlpRecAttrSecret)
			fprintf(stderr, " Secret");
		if (flags & dlpRecAttrArchived)
			fprintf(stderr, " Archive");
		if (!flags)
			fprintf(stderr, " None");
		fprintf(stderr, " (0x%2.2X), and %d bytes:\n", flags,
			result - 10);
		dumpdata(dlp_buf + 10, result - 10);

#endif

	/*id = get_long(dlp_buf); */
	if (index)
		*index = get_short(dlp_buf + 4);
	if (size)
		*size = get_short(dlp_buf + 6);
	if (attr)
		*attr = get_byte(dlp_buf + 8);
	if (category)
		*category = get_byte(dlp_buf + 9);
	if (buffer)
		memcpy(buffer, dlp_buf + 10, result - 10);

	return result - 10;
	
	dlp_response_free(res);
	
	return data_len;
}

/***********************************************************************
 *
 * Parmeters:   None
 *              id       <-- Record ID of record on palm device.
 *              size     <-- Size of data returned in buffer.
 *              attr     <-- Attributes from record on palm device.
 * Turns this request for a particular record in the database into a
 * low-level dlp request.
 *
 * Parameter documentation by DHS.
 *
 ***********************************************************************/
	int 	result;

#ifdef DLP_TRACE
	int 	flags;
#endif

	set_byte(dlp_buf, fHandle);
	set_byte(dlp_buf + 1, 0x00);
	set_short(dlp_buf + 2, index);
	set_short(dlp_buf + 4, 0);	/* Offset into record */
	set_short(dlp_buf + 6, buffer ? DLP_BUF_SIZE : 0);	/* length to return */
	int 	result,
		data_len;
	struct dlpRequest *req;
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d, Index: %d\n", fHandle,
			index);
	}
#endif
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0x00);
	result =
	    dlp_exec(sd, dlpFuncReadRecord, 0x21, dlp_buf, 8, dlp_buf, DLP_BUF_SIZE);
	set_short(DLP_REQUEST_DATA(req, 0, 4), 0);	/* Offset into record */
	Expect(10);
		if (category)
#ifdef DLP_TRACE
	if (dlp_trace) {
		flags = get_byte(dlp_buf + 8);
		fprintf(stderr,
			"  Read: ID: 0x%8.8lX, Index: %d, Category: %d\n        Flags:",
			(unsigned long) get_long(dlp_buf),
			get_short(dlp_buf + 4),
			(int) get_byte(dlp_buf + 9));
		if (flags & dlpRecAttrDeleted)
			fprintf(stderr, " Deleted");
		if (flags & dlpRecAttrDirty)
			fprintf(stderr, " Dirty");
		if (flags & dlpRecAttrBusy)
			fprintf(stderr, " Busy");
		if (flags & dlpRecAttrSecret)
			fprintf(stderr, " Secret");
		if (flags & dlpRecAttrArchived)
			fprintf(stderr, " Archive");
		if (!flags)
			fprintf(stderr, " None");
		fprintf(stderr, " (0x%2.2X), and %d bytes:\n", flags,
			result - 10);
		dumpdata(dlp_buf + 10, result - 10);

#endif
	dlp_response_free(res);
	if (id)
		*id = get_long(dlp_buf);
	/*get_short(dlp_buf+4) == index */
	if (size)
		*size = get_short(dlp_buf + 6);
	if (attr)
		*attr = get_byte(dlp_buf + 8);
	if (category)
		*category = get_byte(dlp_buf + 9);
	if (buffer)
		memcpy(buffer, dlp_buf + 10, result - 10);
	int size, flags;
	return result - 10;
	    (flags & dlpRecAttrArchived) ? " Archive" : "",
	dlp_response_free (res);
	
	return result;
}
