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

/* We need to fix this for OSX and NeXT systems that 
   don't provide ENOMSG. Use EINVAL instead. */
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif /* HAVE_ERRNO_H */

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
#define DLP_RESPONSE_DATA(res, arg, offset) &res->argv[arg]->data[offset]

/* Define prototypes */
static void record_dump (unsigned char *data);



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
#define Trace(name)
#define Expect(count)
#endif

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
	if (argc)
		req->argv = malloc (sizeof (struct dlpArg *) * argc);
	else
		req->argv = NULL;
	
	va_start (ap, argc);
	for (i = 0; i < argc; i++) {
		int len;

		len = va_arg (ap, int);
		req->argv[i] = dlp_arg_new (PI_DLP_ARG_FIRST_ID + i, len);
	}
	va_end (ap);
	
	return req;	
}

struct dlpRequest *
dlp_request_new_with_argid (enum dlpFunctions cmd, int argid, int argc, ...)
{
	struct dlpRequest *req;
	va_list ap;
	int i;
	
	req = malloc (sizeof (struct dlpRequest));
	req->cmd = cmd;
	
	req->argc = argc;
	if (argc)
		req->argv = malloc (sizeof (struct dlpArg *) * argc);
	else
		req->argv = NULL;
	
	va_start (ap, argc);
	for (i = 0; i < argc; i++) {
		int len;

		len = va_arg (ap, int);
		req->argv[i] = dlp_arg_new (argid + i, len);
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
	if (argc)
		res->argv = malloc (sizeof (struct dlpArg *) * argc);
	else
		res->argv = NULL;
	
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
	for (i = 0, argid = PI_DLP_ARG_FIRST_ID; i < response->argc; i++, argid++) {
		if (get_byte (buf) == argid) {
			len = get_byte(&buf[1]);
			buf += 2;
		} else if ((get_byte (buf) & 0x7f) == argid) {
			len = get_long (&buf[2]);
			buf += 6;
		} else if ((get_short(buf) & 0x3FFF) == argid) {
			len = get_long (&buf[2]);
			buf += 6;
		} else if ((get_byte (buf) & PI_DLP_ARG_FLAG_SHORT) == PI_DLP_ARG_FLAG_SHORT) {
			return -1;
		} else {
			argid = get_byte(buf);
			len = get_byte(&buf[1]);
			buf += 2;
		}
		
		     
	return 0;
	if (response->argc == 0)
		return 0;
	else
		return response->argv[0]->len;
}

int
dlp_request_write (struct dlpRequest *req, int sd)
{
	unsigned char *exec_buf, *buf;
	int len, i;
	
	len = dlp_arg_len (req->argc, req->argv) + 2;
	exec_buf = (unsigned char *) malloc (sizeof (unsigned char) * len);
	
	set_byte (&exec_buf[PI_DLP_OFFSET_CMD], req->cmd);
	set_byte (&exec_buf[PI_DLP_OFFSET_ARGC], req->argc);

	buf = &exec_buf[PI_DLP_OFFSET_ARGV];	
	for (i = 0; i < req->argc; i++) {
		struct dlpArg *arg = req->argv[i];
		short argid = arg->id;
		
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
			return i;
			memcpy (&buf[6], arg->data, arg->len);
			buf += arg->len + 6;
		} else {
			goto cleanup;
		}
		return -1;

	if (pi_write(sd, exec_buf, len) < len) {

 cleanup:
	free (exec_buf);
	
	return i;
}

void
dlp_request_free (struct dlpRequest *req)
{
	int i;

	for (i = 0; i < req->argc; i++)
		dlp_arg_free (req->argv[i]);

	if (req->argv)
		free (req->argv);
	free (req);
}

void
dlp_response_free (struct dlpResponse *res) 
{
	int i;

	if (!res)
		return;
	
	for (i = 0; i < res->argc; i++)
		dlp_arg_free (res->argv[i]);
	
	if (res->argv)
		free (res->argv);
	free (res);	

int dlp_exec(int sd, struct dlpRequest *req, struct dlpResponse **res)
{
	int bytes;
	*res = NULL;
	
	if (dlp_request_write (req, sd) < req->argc) {
	if (dlp_response_read (res, sd) < 0) {
		return -1;
	}

	if ((bytes=dlp_response_read (res, sd)) < 0) {
		errno = -EIO;
		return -1;
		errno = -ENOMSG;
		return -1;
		if (req->cmd != dlpFuncVFSVolumeInfo || (*res)->cmd != dlpFuncVFSVolumeSize) {
			errno = -ENOMSG;
			return -1;
		}
	}

	/* Check to make sure there was no error  */
	if ((*res)->err != dlpErrNoError) {
	return 0;
		return -(*res)->err;
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
	   than it was supposed to report. You can verify this with the
	   following: 
		perl -e '$date=localtime(0x83D8FE00); print $date,"\n"'

	   return (time_t) 0x83D8FE00;	// Wed Dec 30 16:00:00 1903 GMT

	   Here are others, depending on what your system requirements are: 

	   return (time_t) 0x83D96E80;	// Thu Dec 31 00:00:00 1903 GMT
	   return (time_t) 0x00007080;	// Thu Jan  1 00:00:00 1970 GMT

	   Palm's own Conduit Development Kit references using 1/1/1904, so
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
 ***********************************************************************/
int dlp_GetSysDateTime(int sd, time_t * t)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(GetSysDateTime);

	req = dlp_request_new(dlpFuncGetSysDateTime, 0);
	
	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO, "DLP GetSysDateTime %s", ctime(t));
	if (result >= 0) {
		*t = dlp_ptohdate(DLP_RESPONSE_DATA (res, 0, 0));

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO, "DLP GetSysDateTime %s", ctime(t)));
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
 ***********************************************************************/
int dlp_SetSysDateTime(int sd, time_t time)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(SetSysDateTime);

	req = dlp_request_new(dlpFuncSetSysDateTime, 1, 8);
	
	dlp_htopdate(time, DLP_REQUEST_DATA(req, 0, 0));

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	dlp_response_free(res);

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
{
	int 	result,
		len1,
		len2;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(ReadStorageInfo);

	req = dlp_request_new(dlpFuncReadStorageInfo, 1, 2);
	
	set_byte(DLP_REQUEST_DATA(req, 0, 0), cardno);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);

	if (result >= 0) {
		c->more 	= get_byte(DLP_RESPONSE_DATA(res, 0, 0)) 
			|| (get_byte(DLP_RESPONSE_DATA(res, 0, 3)) > 1);
		c->card 	= get_byte(DLP_RESPONSE_DATA(res, 0, 5));
		c->version 	= get_byte(DLP_RESPONSE_DATA(res, 0, 6));
		c->creation 	= get_date(DLP_RESPONSE_DATA(res, 0, 8));
		c->romSize 	= get_long(DLP_RESPONSE_DATA(res, 0, 16));
		c->ramSize 	= get_long(DLP_RESPONSE_DATA(res, 0, 20));
		c->ramFree 	= get_long(DLP_RESPONSE_DATA(res, 0, 24));

		len1 = get_byte(DLP_RESPONSE_DATA(res, 0, 28));
		memcpy(c->name, DLP_RESPONSE_DATA(res, 0, 30), len1);
		c->name[len1] = '\0';

		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		memcpy(c->manufacturer, DLP_RESPONSE_DATA(res, 0, 30 + len1), len2);
		    c->card, c->version, ctime(&c->creation));
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    c->romSize, c->ramSize, c->ramFree);
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "  Card name: '%s'\n", c->name);
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "  Manufacturer name: '%s'\n", c->manufacturer);
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "  More: %s\n", c->more ? "Yes" : "No");
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "  Manufacturer name: '%s'\n", c->manufacturer));
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "  More: %s\n", c->more ? "Yes" : "No"));
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
 ***********************************************************************/
int dlp_ReadSysInfo(int sd, struct SysInfo *s)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	Trace(ReadSysInfo);

	req = dlp_request_new (dlpFuncReadSysInfo, 1, 4);

	set_short (DLP_REQUEST_DATA (req, 0, 0), 0x0001);
	set_short (DLP_REQUEST_DATA (req, 0, 2), 0x0003);
	
	result = dlp_exec(sd, req, &res);

	dlp_request_free (req);

	if (result >= 0) {
		s->romVersion = get_long (DLP_RESPONSE_DATA (res, 0, 0));
		s->locale = get_long (DLP_RESPONSE_DATA (res, 0, 4));
		if (req->argc > 1) {
		s->prodIDLength = get_byte (DLP_RESPONSE_DATA (res, 0, 9));
		memcpy(s->prodID, DLP_RESPONSE_DATA(res, 0, 10), s->prodIDLength);

		if (res->argc > 1) {
			s->dlpMajorVersion = get_short (DLP_RESPONSE_DATA (res, 1, 0));
			s->dlpMinorVersion = get_short (DLP_RESPONSE_DATA (res, 1, 2));
			s->compatMajorVersion = get_short (DLP_RESPONSE_DATA (res, 1, 4));
			s->compatMinorVersion = get_short (DLP_RESPONSE_DATA (res, 1, 6));
		} else {
			s->dlpMajorVersion = 0;
			s->dlpMinorVersion = 0;
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
			s->compatMinorVersion = 0;
		    s->romVersion, s->locale);
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "  Product ID=0x%8.8lX\n", s->prodID);
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		    s->romVersion, s->locale));
		    s->dlpMajorVersion, s->dlpMinorVersion);
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    s->compatMajorVersion, s->compatMinorVersion);
		    s->dlpMajorVersion, s->dlpMinorVersion));
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "  Compat Major Ver=0x%4.4lX Compat Minor Vers=0x%4.4lX\n",
		    s->compatMajorVersion, s->compatMinorVersion));
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
dlp_ReadDBList(int sd, int cardno, int flags, int start,
	       struct DBInfo *info)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(ReadDBList);

	req = dlp_request_new (dlpFuncReadDBList, 1, 4);
	
	set_byte(DLP_REQUEST_DATA(req, 0, 0), (unsigned char) flags);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), (unsigned char) cardno);
	set_short(DLP_REQUEST_DATA(req, 0, 2), start);

	result = dlp_exec (sd, req, &res);

	dlp_request_free(req);
	
	if (result >= 0) {
		info->more = get_byte(DLP_RESPONSE_DATA(res, 0, 2));
		if (pi_version(sd) > 0x0100)	/* PalmOS 2.0 has additional flag */
			info->miscFlags = get_byte(DLP_RESPONSE_DATA(res, 0, 5));
		else
			info->miscFlags = 0;
		info->flags 		= get_short(DLP_RESPONSE_DATA(res, 0, 6));
		info->type 		= get_long(DLP_RESPONSE_DATA(res, 0, 8));
		info->creator 		= get_long(DLP_RESPONSE_DATA(res, 0, 12));
		info->version 		= get_short(DLP_RESPONSE_DATA(res, 0, 16));
		info->modnum 		= get_long(DLP_RESPONSE_DATA(res, 0, 18));
		info->createDate 	= get_date(DLP_RESPONSE_DATA(res, 0, 22));
		info->modifyDate 	= get_date(DLP_RESPONSE_DATA(res, 0, 30));
		info->backupDate 	= get_date(DLP_RESPONSE_DATA(res, 0, 38));
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		strncpy(info->name, DLP_RESPONSE_DATA(res, 0, 48), 32);
		    info->name, info->version, info->more ? "Yes" : "No");
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "  Creator: '%s'", printlong(info->creator));
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		    info->name, info->version, info->more ? "Yes" : "No"));
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "  Creator: '%s'", printlong(info->creator)));
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    " Type: '%s' Flags: %s%s%s%s%s%s%s%s%s%s",
		    printlong(info->type),
		    (info->flags & dlpDBFlagResource) ? "Resource " : "",
		    (info->flags & dlpDBFlagReadOnly) ? "ReadOnly " : "",
		    (info->flags & dlpDBFlagAppInfoDirty) ? "AppInfoDirty " : "",
		    (info->flags & dlpDBFlagBackup) ? "Backup " : "",
		    (info->flags & dlpDBFlagReset) ? "Reset " : "",
		    (!info->flags) ? "None" : "");
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO, " (0x%2.2X)\n", info->flags);
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		    (info->flags & dlpDBFlagOpen) ? "Open " : "",
		    info->modnum, info->index, ctime(&info->createDate));
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		    " Modification date: %s", ctime(&info->modifyDate));
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO, 
		    " Backup date: %s", ctime(&info->backupDate));
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    " Modification date: %s", ctime(&info->modifyDate)));
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO, 
	
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
	   returned info in as start the next time round. */

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
		start = 0x1000;
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
 ***********************************************************************/
int dlp_OpenDB(int sd, int cardno, int mode, char *name, int *dbhandle)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(OpenDB);

	req = dlp_request_new(dlpFuncOpenDB, 1, 2 + (strlen(name) + 1));

	set_byte(DLP_REQUEST_DATA(req, 0, 0), cardno);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), mode);
	strcpy(DLP_REQUEST_DATA(req, 0, 2), name);

	result = dlp_exec(sd, req, &res);
	
	dlp_request_free(req);
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP OpenDB Handle=%d", *dbhandle);
		*dbhandle = get_byte(DLP_RESPONSE_DATA(res, 0, 0));

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP OpenDB Handle=%d\n", *dbhandle));
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
 ***********************************************************************/
int dlp_DeleteDB(int sd, int card, const char *name)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(DeleteDB);

	req = dlp_request_new(dlpFuncDeleteDB, 1, 2 + (strlen(name) + 1));

	set_byte(DLP_REQUEST_DATA(req, 0, 0), card);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
	strcpy(DLP_REQUEST_DATA(req, 0, 2), name);

	result = dlp_exec(sd, req, &res);
	
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
dlp_CreateDB(int sd, long creator, long type, int cardno, int flags,
	     int version, const char *name, int *dbhandle)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(CreateDB);

	req = dlp_request_new(dlpFuncCreateDB, 1, 14 + (strlen(name) + 1));

	set_long(DLP_REQUEST_DATA(req, 0, 0), creator);
	set_long(DLP_REQUEST_DATA(req, 0, 4), type);
	set_byte(DLP_REQUEST_DATA(req, 0, 8), cardno);
	set_byte(DLP_REQUEST_DATA(req, 0, 9), 0);
	set_short(DLP_REQUEST_DATA(req, 0, 10), flags);
	set_short(DLP_REQUEST_DATA(req, 0, 12), version);
	strcpy(DLP_REQUEST_DATA(req, 0, 14), name);

	result = dlp_exec(sd, req, &res);
	
	dlp_request_free(req);
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP CreateDB Handle=%d", *dbhandle);
		*dbhandle = get_byte(DLP_RESPONSE_DATA(res, 0, 0));

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
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
 ***********************************************************************/
int dlp_CloseDB(int sd, int dbhandle)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(CloseDB);

	req = dlp_request_new(dlpFuncCloseDB, 1, 1);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), (unsigned char) dbhandle);

	result = dlp_exec(sd, req, &res);
	
	dlp_request_free(req);	
	dlp_response_free(res);
	
	return result;
}

int dlp_CloseDB_All(int sd)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(CloseDB_All);

	req = dlp_request_new_with_argid(dlpFuncCloseDB, 0x21, 0);

	result = dlp_exec(sd, req, &res);
	
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
		    int action, int length, void *data,
		    unsigned long *retcode, int maxretlen, int *retlen,
		    void *retdata)
{
	int 	result,
		data_len,
		version = pi_version(sd);
	struct dlpRequest *req;
	struct dlpResponse *res;

	if (version >= 0x0101) {	/* PalmOS 2.0 call encoding */
		Trace(CallApplicationV1);

		req = dlp_request_new_with_argid(dlpFuncCallApplication, 0x21, 1, 22 + length);
		
		set_long(DLP_REQUEST_DATA(req, 0, 0), creator);
		set_long(DLP_REQUEST_DATA(req, 0, 4), type);
		set_short(DLP_REQUEST_DATA(req, 0, 8), action);
		set_long(DLP_REQUEST_DATA(req, 0, 10), length);
		set_long(DLP_REQUEST_DATA(req, 0, 14), 0);
		set_long(DLP_REQUEST_DATA(req, 0, 18), 0);


		if (length + 22 > DLP_BUF_SIZE) {
			fprintf(stderr, "Data too large\n");
			return -131;
		}
		memcpy(DLP_REQUEST_DATA(req, 0, 22), data, length);

		data_len = res->argv[0]->len - 16;
			     data_len));
		if (retcode)
			*retcode = get_long(DLP_RESPONSE_DATA(res, 0, 0));
		if (retlen)
			*retlen = data_len;
		if (retdata)
			memcpy(retdata, DLP_RESPONSE_DATA(res, 0, 16),
			       data_len > maxretlen ? maxretlen : data_len);


		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP CallApplication Result: %lu (0x%8.8lX), and %d bytes:\n",
		    get_long(DLP_RESPONSE_DATA(res, 0, 0)), 
		    get_long(DLP_RESPONSE_DATA(res, 0, 4)),
		    data_len);
		CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, 
		      dumpdata(DLP_RESPONSE_DATA(res, 0, 16), data_len));

			CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, 
			      dumpdata(DLP_RESPONSE_DATA(res, 0, 16), data_len));
		}
		
		dlp_response_free(res);
		
		return result;

	} else {		/* PalmOS 1.0 call encoding */
		Trace(CallApplicationV10);

		req = dlp_request_new (dlpFuncCallApplication, 1, 8 + length);
		
		set_long(DLP_REQUEST_DATA(req, 0, 0), creator);
		set_short(DLP_REQUEST_DATA(req, 0, 4), action);
		set_short(DLP_REQUEST_DATA(req, 0, 6), length);


		if (length + 8 > DLP_BUF_SIZE) {
			fprintf(stderr, "Data too large\n");
			return -131;
		}
		memcpy(DLP_REQUEST_DATA(req, 0, 8), data, length);

		data_len = res->argv[0]->len - 6;
		if (retcode)
			*retcode = get_short(DLP_RESPONSE_DATA(res, 0, 2));
		if (retlen)
			*retlen = data_len;

		if (retdata)
			memcpy(retdata, DLP_RESPONSE_DATA(res, 0, 6),
			       data_len > maxretlen ? maxretlen : data_len);

		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP CallApplication Action: %d Result: %lu (0x%4.4lX), and %d bytes:\n",
		    get_short(DLP_RESPONSE_DATA(res, 0, 0)), 
		    get_short(DLP_RESPONSE_DATA(res, 0, 2)), 
		    get_short(DLP_RESPONSE_DATA(res, 0, 2)),
		    data_len);
		CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, 
		      dumpdata(DLP_RESPONSE_DATA(res, 0, 6), data_len));
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
 ***********************************************************************/
int dlp_ResetSystem(int sd)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(ResetSystems);

	req = dlp_request_new(dlpFuncResetSystem, 0);

	result = dlp_exec(sd, req, &res);
	
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
 ***********************************************************************/
int dlp_AddSyncLogEntry(int sd, char *entry)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(AddSyncLogEntry);

	req = dlp_request_new(dlpFuncAddSyncLogEntry, 1, strlen(entry) + 1);
	
	strcpy(DLP_REQUEST_DATA(req, 0, 0), entry);
	
	result = dlp_exec(sd, req, &res);
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP AddSyncLogEntry Entry: \n  %s\n", entry);

	if (result >= 0)
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
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
 ***********************************************************************/
int dlp_ReadOpenDBInfo(int sd, int dbhandle, int *records)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(ReadOpenDBInfo);

	req = dlp_request_new(dlpFuncReadOpenDBInfo, 1, 1);
	
	set_byte(DLP_REQUEST_DATA(req, 0, 0), dbhandle);
	
	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);

		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		if (records)
		    get_short(DLP_RESPONSE_DATA(res, 0, 0)));
		
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadOpenDBInfo %d records\n", 
		    get_short(DLP_RESPONSE_DATA(res, 0, 0))));
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
 ***********************************************************************/
int dlp_MoveCategory(int sd, int handle, int fromcat, int tocat)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(MoveCategory);

	req = dlp_request_new(dlpFuncMoveCategory, 1, 4);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), handle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), fromcat);
	set_byte(DLP_REQUEST_DATA(req, 0, 2), tocat);
	set_byte(DLP_REQUEST_DATA(req, 0, 3), 0);

	result = dlp_exec(sd, req, &res);
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
	dlp_request_free(req);	
		    handle, fromcat, tocat);
	if (result >= 0)
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP MoveCategory Handle: %d, From: %d, To: %d\n",
		    handle, fromcat, tocat));
	
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
 ***********************************************************************/
int dlp_OpenConduit(int sd)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(OpenConduit);

	req = dlp_request_new(dlpFuncOpenConduit, 0);

	result = dlp_exec(sd, req, &res);

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
int dlp_EndOfSync(int sd, int status)
{
	int 	result;
	struct 	pi_socket *ps;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(EndOfSync);

	ps = find_pi_socket(sd);
	if (ps == 0)
		return 1;	/* General system error */

	req = dlp_request_new(dlpFuncEndOfSync, 1, 2);

	set_short(DLP_REQUEST_DATA(req, 0, 0), status);

	result = dlp_exec(sd, req, &res);

	/* Messy code to set end-of-sync flag on socket 
	   so pi_close won't do it for us */
	if (result == 0)
		ps->state = PI_SOCK_CONEN;

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
int dlp_AbortSync(int sd)
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
 ***********************************************************************/
int dlp_WriteUserInfo(int sd, struct PilotUser *User)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	int len;
	
	Trace(WriteUserInfo);

	len = strlen (User->username) + 1;
	
	req = dlp_request_new (dlpFuncWriteUserInfo, 1, 22 + len);

	set_long(DLP_REQUEST_DATA(req, 0, 0), User->userID);
	set_long(DLP_REQUEST_DATA(req, 0, 4), User->viewerID);
	set_long(DLP_REQUEST_DATA(req, 0, 8), User->lastSyncPC);
	set_date(DLP_REQUEST_DATA(req, 0, 12), User->lastSyncDate);
	set_byte(DLP_REQUEST_DATA(req, 0, 20), 0xff);
	set_byte(DLP_REQUEST_DATA(req, 0, 21), len);
	strcpy(DLP_REQUEST_DATA(req, 0, 22), User->username);

	result = dlp_exec (sd, req, &res);

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
int dlp_ReadUserInfo(int sd, struct PilotUser *User)
{
	int 	result,
		userlen;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	Trace(ReadUserInfo);
	
	req = dlp_request_new (dlpFuncReadUserInfo, 0);
	
	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);

	if (result >= 0) {
		User->userID 		 = get_long(DLP_RESPONSE_DATA (res, 0, 0));
		User->viewerID 		 = get_long(DLP_RESPONSE_DATA (res, 0, 4));
		User->lastSyncPC 	 = get_long(DLP_RESPONSE_DATA (res, 0, 8));
		User->successfulSyncDate = get_date(DLP_RESPONSE_DATA (res, 0, 12));
		User->lastSyncDate 	 = get_date(DLP_RESPONSE_DATA (res, 0, 20));
		userlen                  = get_byte(DLP_RESPONSE_DATA (res, 0, 28));
		User->passwordLength 	 = get_byte(DLP_RESPONSE_DATA (res, 0, 29));
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		memcpy(User->password, DLP_RESPONSE_DATA (res, 0, 30 + userlen),
		    User->userID, User->viewerID, User->lastSyncPC);
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    ctime (&User->lastSyncDate), ctime (&User->successfulSyncDate));
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    User->username);
		    ctime (&User->lastSyncDate), ctime (&User->successfulSyncDate)));
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "  Username=%s\n",
		    User->username));
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
 *
 ***********************************************************************/
int dlp_ReadNetSyncInfo(int sd, struct NetSyncInfo *i)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(ReadNetSyncInfo);

	if (pi_version(sd) < 0x0101)
		return -129;	/* This call only functions under PalmOS 2.0 */

	req = dlp_request_new(dlpFuncReadNetSyncInfo, 0);
	
	result = dlp_exec (sd, req, &res);

	dlp_request_free(req);

	if (result >= 0) {
		int str_offset = 24;
		
		i->lanSync = get_byte(DLP_RESPONSE_DATA(res, 0, 0));
		
		i->hostName[0] = '\0';
		memcpy(i->hostName, DLP_RESPONSE_DATA(res, 0, str_offset), 
		       get_short(DLP_RESPONSE_DATA(res, 0, 18)));
		str_offset += get_short(DLP_RESPONSE_DATA(res, 0, 18));

		i->hostAddress[0] = '\0';
		memcpy(i->hostAddress, DLP_RESPONSE_DATA(res, 0, str_offset), 
		       get_short(DLP_RESPONSE_DATA(res, 0, 20)));
		str_offset += get_short(DLP_RESPONSE_DATA(res, 0, 20));

		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadNetSyncInfo Active: %d\n", i->lanSync ? 1 : 0);
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,

		    i->hostName, i->hostAddress, i->hostSubnetMask);
		    "DLP ReadNetSyncInfo Active: %d\n", i->lanSync ? 1 : 0));
		    i->hostName, i->hostAddress, i->hostSubnetMask));
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
 ***********************************************************************/
int dlp_WriteNetSyncInfo(int sd, struct NetSyncInfo *i)
{
	int 	result,
		str_offset = 24;
	struct dlpRequest *req;
	struct dlpResponse *res;

	if (pi_version(sd) < 0x0101)
	LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
	    "DLP ReadNetSyncInfo Active: %d\n", i->lanSync ? 1 : 0);
	LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,

	    i->hostName, i->hostAddress, i->hostSubnetMask);
	    "DLP ReadNetSyncInfo Active: %d\n", i->lanSync ? 1 : 0));
	LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
	    "  PC hostname: '%s', address '%s', mask '%s'\n",
	    i->hostName, i->hostAddress, i->hostSubnetMask));

	req = dlp_request_new(dlpFuncWriteNetSyncInfo, 1, 24 + strlen(i->hostName) + 
			      strlen(i->hostAddress) + strlen(i->hostSubnetMask) + 3);
	
	set_byte(DLP_REQUEST_DATA(req, 0, 0), 0x80 | 0x40 | 0x20 | 0x10);	/* Change all settings */
	set_byte(DLP_REQUEST_DATA(req, 0, 1), i->lanSync);
	set_long(DLP_REQUEST_DATA(req, 0, 2), 0);  /* Reserved1 */
	set_long(DLP_REQUEST_DATA(req, 0, 6), 0);  /* Reserved2 */
	set_long(DLP_REQUEST_DATA(req, 0, 10), 0); /* Reserved3 */
	set_long(DLP_REQUEST_DATA(req, 0, 14), 0); /* Reserved4 */
	set_short(DLP_REQUEST_DATA(req, 0, 18), strlen(i->hostName) + 1);
	set_short(DLP_REQUEST_DATA(req, 0, 20), strlen(i->hostAddress) + 1);
	set_short(DLP_REQUEST_DATA(req, 0, 22), strlen(i->hostSubnetMask) + 1);

	strcpy(DLP_REQUEST_DATA(req, 0, str_offset), i->hostName);
	str_offset += strlen(i->hostName) + 1;
	strcpy(DLP_REQUEST_DATA(req, 0, str_offset), i->hostAddress);
	str_offset += strlen(i->hostAddress) + 1;
	strcpy(DLP_REQUEST_DATA(req, 0, str_offset), i->hostSubnetMask);

	result = dlp_exec(sd, req, &res);

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
		err;
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
dlp_ReadFeature(int sd, unsigned long creator, unsigned int num,
		unsigned long *feature)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	if (pi_version(sd) < 0x0101) {
		struct RPC_params p;		
		int val;
		unsigned long result;

		Trace(ReadFeatureV1);

		if (!feature)
			return 0;

		*feature = 0x12345678;

		PackRPC(&p, 0xA27B, RPC_IntReply,
			RPC_Long(creator),
			RPC_Short((unsigned short) num),
			RPC_LongPtr(feature), RPC_End);
			LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		val = dlp_RPC(sd, &p, &result);
			    dlp_errorlist[-val], val);
		if (val < 0) {
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			    "DLP ReadFeature Error: %s (%d)\n",
			LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,

			    (unsigned long) result);
		} else if (result) {
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			    "DLP ReadFeature FtrGet error 0x%8.8lX\n",
			LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,

			    (unsigned long) *feature);
		} else {
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			    " DLP ReadFeature Feature: 0x%8.8lX\n",
			    (unsigned long) *feature));
		}
		
		return 0;
	} else {

		Trace(ReadFeatureV2);

		req = dlp_request_new(dlpFuncReadFeature, 1, 6);

		set_long(DLP_REQUEST_DATA(req, 0, 0), creator);
		set_short(DLP_REQUEST_DATA(req, 0, 4), num);

		result = dlp_exec(sd, req, &res);

		dlp_request_free(req);
		
			LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
			if (feature)
			    (unsigned long) get_long(DLP_RESPONSE_DATA(res, 0, 0)));
			
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			    "DLP ReadFeature Feature: 0x%8.8lX\n",
			    (unsigned long) get_long(DLP_RESPONSE_DATA(res, 0, 0))));
		}
		dlp_response_free(res);

		return result;
	}
	
}
#endif				/* IFDEF _PILOT_SYSPKT_H */

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
dlp_GetROMToken(int sd, unsigned long token, char *buffer, unsigned int *size)
{
	unsigned long result;

	struct RPC_params p;
	unsigned long buffer_ptr;

	Trace(GetROMToken);
	
#ifdef DLP_TRACE
	if (dlp_trace) {
	  fprintf(stderr,
		  " Wrote: Token: '%s'\n",
		  printlong(token));
	}
#endif

	PackRPC(&p, 41792, RPC_IntReply,
		RPC_Short(0),
		RPC_Long(token),
		RPC_LongPtr(&buffer_ptr),
		RPC_ShortPtr(size), RPC_End);
	
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
		    "  Read: Buffer Ptr: 0x%8.8lX Size: %d\n",
		    (unsigned long) buffer_ptr, *size);
	}
#endif		
	
	if( buffer ) {
	  buffer[*size] = 0;

	  PackRPC(&p, 0xa026, RPC_IntReply,
		  RPC_Ptr(buffer, *size),
		  RPC_Long(buffer_ptr),
		  RPC_Long((unsigned long) *size), 
		  RPC_End);
	  
	  val = dlp_RPC(sd, &p, &result);
	}
	
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
		    "  Read: Buffer: %s\n", buffer);
	}
#endif	

	if (val < 0)
	  return val;
	
	if (result)
	  return (-(long) result);
	
	return result;
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
	struct 	PilotUser User;

	if ((err = dlp_ReadUserInfo(sd, &User)) < 0)
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
int dlp_ResetDBIndex(int sd, int dbhandle)
{
	int 	result;
	struct 	pi_socket *ps;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(ResetRecordIndex);

	if ((ps = find_pi_socket(sd)))
		ps->dlprecord = 0;

	req = dlp_request_new(dlpFuncResetRecordIndex, 1, 1);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), dbhandle);
	
	result = dlp_exec(sd, req, &res);

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
int
dlp_ReadRecordIDList(int sd, int dbhandle, int sort, int start, int max,
		     recordid_t * IDs, int *count)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(ReadRecordIDList);

	req = dlp_request_new(dlpFuncReadRecordIDList, 1, 6);
		
	set_byte(DLP_REQUEST_DATA(req, 0, 0), dbhandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), sort ? 0x80 : 0);
	set_short(DLP_REQUEST_DATA(req, 0, 2), start);
	set_short(DLP_REQUEST_DATA(req, 0, 4), max);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result >= 0) {
		int ret, i;
		
		ret = get_short(DLP_RESPONSE_DATA(res, 0, 0));
		for (i = 0; i < ret; i++)
			IDs[i] = get_long(DLP_RESPONSE_DATA(res, 0, 2 + (i * 4)));
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadRecordIDList %d IDs:\n", ret);
			*count = ret;

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadRecordIDList %d IDs:\n", ret));
		CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, 
		      dumpdata(DLP_RESPONSE_DATA(res, 0, 2), ret * 4));
	}

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
dlp_WriteRecord(int sd, int dbhandle, int flags, recordid_t recID,
		int catID, void *data, int length, recordid_t * NewID)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;


	if (length == -1)
		length = strlen((char *) data) + 1;


	if (length + 8 > DLP_BUF_SIZE) {
		fprintf(stderr, "Data too large\n");
		return -131;
	}
	req = dlp_request_new(dlpFuncWriteRecord, 1, 8 + length);
	
	set_byte(DLP_REQUEST_DATA(req, 0, 0), dbhandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
	set_long(DLP_REQUEST_DATA(req, 0, 2), recID);
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

	memcpy(DLP_REQUEST_DATA(req, 0, 8), data, length);

	CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, record_dump(DLP_RESPONSE_DATA(req, 0, 0)));
	
	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		if (NewID)
		    get_long(DLP_RESPONSE_DATA(res, 0, 0)));

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP WriteRecord Record ID: 0x%8.8lX\n",
		    get_long(DLP_RESPONSE_DATA(res, 0, 0))));
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
int dlp_DeleteRecord(int sd, int dbhandle, int all, recordid_t recID)
{
	int 	result,
		flags = all ? 0x80 : 0;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(DeleteRecord);

	req = dlp_request_new(dlpFuncDeleteRecord, 1, 6);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), dbhandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), flags);
	set_long(DLP_REQUEST_DATA(req, 0, 2), recID);

	result = dlp_exec(sd, req, &res);
	
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
 *
 ***********************************************************************/
int dlp_DeleteCategory(int sd, int dbhandle, int category)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	if (pi_version(sd) < 0x0101) {
		/* Emulate if not connected to PalmOS 2.0 */
		int i, r, cat, attr;
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,

		    dbhandle, category & 0xff);

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
			if (r < 0)
				return r;
			i--;	/* Sigh, deleting the record moves it to the end. */
		}

		return 0;
	} else {
		int flags = 0x40;
		
		Trace(DeleteCategoryV2);

		req = dlp_request_new(dlpFuncDeleteRecord, 1, 6);
		
		set_byte(DLP_REQUEST_DATA(req, 0, 0), dbhandle);
		set_byte(DLP_REQUEST_DATA(req, 0, 1), flags);
		set_long(DLP_REQUEST_DATA(req, 0, 2), category & 0xff);
		
		result = dlp_exec(sd, req, &res);

		dlp_request_free(req);
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
int
dlp_ReadResourceByType(int sd, int fHandle, unsigned long type, int id,
		       void *buffer, int *index, int *size)
{
	int 	result,
		data_len;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(ReadResourceByType);

	req = dlp_request_new_with_argid(dlpFuncReadResource, 0x21, 1, 12);
	
	set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
	set_long(DLP_REQUEST_DATA(req, 0, 2), type);
	set_short(DLP_REQUEST_DATA(req, 0, 6), id);
	set_short(DLP_REQUEST_DATA(req, 0, 8), 0);
	set_short(DLP_REQUEST_DATA(req, 0, 10), buffer ? DLP_BUF_SIZE : 0);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result >= 0) {
		data_len = res->argv[0]->len - 10;
		if (index)
			*index = get_short(DLP_RESPONSE_DATA(res, 0, 6));
		if (size)
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		if (buffer)
			memcpy(buffer, DLP_RESPONSE_DATA(res, 0, 10), data_len);
		    get_short(DLP_RESPONSE_DATA(res, 0, 6)), data_len);
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadResourceByType  Type: '%s', ID: %d, Index: %d, and %d bytes:\n",
		    printlong(type), id, 
		    get_short(DLP_RESPONSE_DATA(res, 0, 6)), data_len));
		CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, 
		      dumpdata(DLP_RESPONSE_DATA(res, 0, 10), data_len));
	} else {
		data_len = result;
	}

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
int
dlp_ReadResourceByIndex(int sd, int fHandle, int index, void *buffer,
			unsigned long *type, int *id, int *size)
{
	int 	result,
		data_len;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(ReadResourceByIndex);

	req = dlp_request_new(dlpFuncReadResource, 1, 8);
	
	set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
	set_short(DLP_REQUEST_DATA(req, 0, 2), index);
	set_short(DLP_REQUEST_DATA(req, 0, 4), 0);
	set_short(DLP_REQUEST_DATA(req, 0, 6), buffer ? DLP_BUF_SIZE : 0);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result >= 0) {
		data_len = res->argv[0]->len - 10;
		if (type)
			*type = get_long(DLP_RESPONSE_DATA(res, 0, 0));
		if (id)
			*id = get_short(DLP_RESPONSE_DATA(res, 0, 4));
		if (size)
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		if (buffer)
			memcpy(buffer, DLP_RESPONSE_DATA(res, 0, 10), data_len);
		
		    index, data_len);
		    "DLP ReadResourceByIndex Type: '%s', ID: %d, Index: %d, and %d bytes:\n",
		    printlong(get_long(DLP_RESPONSE_DATA(res, 0, 0))),
		    get_short(DLP_RESPONSE_DATA(res, 0, 4)),
		    index, data_len));
		CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, 
		      dumpdata(DLP_RESPONSE_DATA(res, 0, 10), data_len));
	} else {
		data_len = result;
	}

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
dlp_WriteResource(int sd, int dbhandle, unsigned long type, int id,
		  const void *data, int length)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(WriteResource);

	req = dlp_request_new(dlpFuncWriteResource, 1, 10 + length);
	
	set_byte(DLP_REQUEST_DATA(req, 0, 0), dbhandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
	set_long(DLP_REQUEST_DATA(req, 0, 2), type);
	set_short(DLP_REQUEST_DATA(req, 0, 6), id);
	set_short(DLP_REQUEST_DATA(req, 0, 8), length);

	if (length + 10 > DLP_BUF_SIZE) {
		fprintf(stderr, "Data too large\n");
		return -131;
	}
	memcpy(DLP_REQUEST_DATA(req, 0, 10), data, length);

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
		   int resID)
{
	int 	result,
		flags = all ? 0x80 : 0;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(DeleteResource);

	req = dlp_request_new(dlpFuncDeleteResource, 1, 8);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), dbhandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), flags);
	set_long(DLP_REQUEST_DATA(req, 0, 2), restype);
	set_short(DLP_REQUEST_DATA(req, 0, 6), resID);

	result = dlp_exec(sd, req, &res);
	
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
 *
 ***********************************************************************/
int dlp_ReadAppBlock(int sd, int fHandle, int offset, void *dbuf, int dlen)
{
	int 	result,
		data_len;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(ReadAppBlock);

	req = dlp_request_new(dlpFuncReadAppBlock, 1, 6);
	
	set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
	set_short(DLP_REQUEST_DATA(req, 0, 2), offset);
	set_short(DLP_REQUEST_DATA(req, 0, 4), dlen);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result >= 0) {
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadAppBlock %d bytes\n", data_len);
			memcpy(dbuf, DLP_RESPONSE_DATA(res, 0, 2), data_len);
		
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadAppBlock %d bytes\n", data_len));
		CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, 
		      dumpdata(DLP_RESPONSE_DATA(res, 0, 2), data_len));
	} else {
		data_len = result;
	}

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
int dlp_WriteAppBlock(int sd, int fHandle, const /* @unique@ */ void *data,
		      int length)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(WriteAppBlock);

	req = dlp_request_new(dlpFuncWriteAppBlock, 1, 4 + length);
	
	set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
	set_short(DLP_REQUEST_DATA(req, 0, 2), length);

	if (length + 10 > DLP_BUF_SIZE) {
		fprintf(stderr, "Data too large\n");
		return -131;
	}
	memcpy(DLP_REQUEST_DATA(req, 0, 4), data, length);

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
 ***********************************************************************/
int
dlp_ReadSortBlock(int sd, int fHandle, int offset, void *dbuf, int dlen)
{
	int 	result,
		data_len;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(ReadSortBlock);

	req = dlp_request_new(dlpFuncReadSortBlock, 1, 6);
	
	set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
	set_short(DLP_REQUEST_DATA(req, 0, 2), offset);
	set_short(DLP_REQUEST_DATA(req, 0, 4), dlen);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result >= 0) {
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadSortBlock %d bytes\n", data_len);
			memcpy(dbuf, DLP_RESPONSE_DATA(res, 0, 2), data_len);
		
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadSortBlock %d bytes\n", data_len));
		CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, 
		      dumpdata(DLP_RESPONSE_DATA(res, 0, 2), data_len));
	} else {
		data_len = result;
	}

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
int dlp_WriteSortBlock(int sd, int fHandle, const /* @unique@ */ void *data,
		       int length)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(WriteSortBlock);

	req = dlp_request_new(dlpFuncWriteSortBlock, 1, 4 + length);
	
	set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
	set_short(DLP_REQUEST_DATA(req, 0, 2), length);

	if (length + 10 > DLP_BUF_SIZE) {
		fprintf(stderr, "Data too large\n");
		return -131;
	}
	memcpy(DLP_REQUEST_DATA(req, 0, 4), data, length);

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
 ***********************************************************************/
int dlp_CleanUpDatabase(int sd, int fHandle)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(CleanUpDatabase);

	req = dlp_request_new(dlpFuncCleanUpDatabase, 1, 1);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);

	result = dlp_exec(sd, req, &res);
	
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
 ***********************************************************************/
int dlp_ResetSyncFlags(int sd, int fHandle)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(ResetSyncFlags);

	req = dlp_request_new(dlpFuncResetSyncFlags, 1, 1);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);

	result = dlp_exec(sd, req, &res);
	
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
			  void *buffer, recordid_t * id, int *index,
			  int *size, int *attr)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	if (pi_version(sd) < 0x0101) {
		/* Emulate for PalmOS 1.0 */
		int 	cat,
			rec;
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,

		    fHandle, incategory);

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadNextRecInCategory Emulating with: Handle: %d, Category: %d\n",
		    fHandle, incategory));

		if ((ps = find_pi_socket(sd)) == 0)
			return -130;

		for (;;) {
			/* Fetch next modified record (in any category) */
			rec = dlp_ReadRecordByIndex(sd, fHandle,
						    ps->dlprecord, 0, 0, 0,
						    0, &cat);

			if (rec < 0)
				break;

			if (cat != incategory) {
				ps->dlprecord++;
				continue;
			}

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

			break;
		}

		return rec;
	} else {
		int data_len, flags;
		
		Trace(ReadNextRecInCategoryV2);

		req = dlp_request_new(dlpFuncReadNextRecInCategory, 1, 2);
		
		set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
		set_byte(DLP_REQUEST_DATA(req, 0, 1), incategory);

		result = dlp_exec(sd, req, &res);

		dlp_request_free(req);

		if (result >= 0) {
			data_len = res->argv[0]->len - 10;
			if (id)
				*id = get_long(DLP_RESPONSE_DATA(res, 0, 0));
			if (index)
				*index = get_short(DLP_RESPONSE_DATA(res, 0, 4));
			if (size)
				*size = get_short(DLP_RESPONSE_DATA(res, 0, 6));
			if (attr)
				*attr = get_byte(DLP_RESPONSE_DATA(res, 0, 8));
			LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
				memcpy(buffer, DLP_RESPONSE_DATA(res, 0, 10), data_len);

			flags = get_byte(DLP_RESPONSE_DATA(res, 0, 8));
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			    "DLP ReadNextRecInCategory ID: 0x%8.8lX, Index: %d, Category: %d\n"
			    "  Flags: %s%s%s%s%s%s (0x%2.2X) and %d bytes:\n",
			    (unsigned long) get_long(DLP_RESPONSE_DATA(res, 0, 0)),
			    get_short(DLP_RESPONSE_DATA(res, 0, 4)),
			    (int) get_byte(DLP_RESPONSE_DATA(res, 0, 9)),
			    (flags & dlpRecAttrDeleted) ? " Deleted" : "",
			    (flags & dlpRecAttrDirty) ? " Dirty" : "",
			    flags, data_len);
			    (flags & dlpRecAttrSecret) ? " Secret" : "",
			    (flags & dlpRecAttrArchived) ? " Archive" : "",
			    (!flags) ? " None" : "",
			    flags, data_len));
			CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, 
			      dumpdata(DLP_RESPONSE_DATA(res, 0, 10), data_len));
		} else {
			data_len = result;
		}

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
dlp_ReadAppPreference(int sd, unsigned long creator, int id, int backup,
		      int maxsize, void *buffer, int *size, int *version)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	if (pi_version(sd) < 0x0101) {
		/* Emulate on PalmOS 1.0 */
		int 	db,
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,

		Trace(ReadAppPreferenceV1);
		    buffer ? maxsize : 0, backup ? 0x80 : 0);
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

		dlp_CloseDB(sd, db);

		return rec;
	} else {
		int data_len;
		
		Trace(ReadAppPreferenceV2);

		req = dlp_request_new(dlpFuncReadAppPreference, 1, 10);
		
		set_long(DLP_REQUEST_DATA(req, 0, 0), creator);
		set_short(DLP_REQUEST_DATA(req, 0, 4), id);
		set_short(DLP_REQUEST_DATA(req, 0, 6), buffer ? maxsize : 0);
		set_byte(DLP_REQUEST_DATA(req, 0, 8), backup ? 0x80 : 0);
		set_byte(DLP_REQUEST_DATA(req, 0, 9), 0); /* Reserved */

		result = dlp_exec(sd, req, &res);

		dlp_request_free(req);
		
		if (result >= 0) {
			data_len = get_short(DLP_RESPONSE_DATA(res, 0, 4));
			if (version)
				*version = get_short(DLP_RESPONSE_DATA(res, 0, 0));
			if (size && !buffer)
				*size = get_short(DLP_RESPONSE_DATA(res, 0, 2));/* Total size */
			if (size && buffer)
			LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
			if (buffer)
				memcpy(buffer, DLP_RESPONSE_DATA(res, 0, 6), data_len);

			    get_short(DLP_RESPONSE_DATA(res, 0, 4)));
			    "DLP ReadAppPref Version: %d, Total size: %d, Read %d bytes:\n",
			    get_short(DLP_RESPONSE_DATA(res, 0, 0)), 
			    get_short(DLP_RESPONSE_DATA(res, 0, 2)),
			    get_short(DLP_RESPONSE_DATA(res, 0, 4))));
			CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, 
			      dumpdata(DLP_RESPONSE_DATA(res, 0, 6), data_len));
		} else {
			data_len = result;
		}
		
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
dlp_WriteAppPreference(int sd, unsigned long creator, int id, int backup,
		       int version, void *buffer, int size)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	if (pi_version(sd) < 0x0101) {
		/* Emulate on PalmOS 1.0 */
		int 	db,
			rec;
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

		dlp_CloseDB(sd, db);

		return rec;
	} else {
		Trace(WriteAppPreferenceV2);

		req = dlp_request_new(dlpFuncWriteAppPreference, 1, 12 + size);
		
		set_long(DLP_REQUEST_DATA(req, 0, 0), creator);
		set_short(DLP_REQUEST_DATA(req, 0, 4), id);
		set_short(DLP_REQUEST_DATA(req, 0, 6), version);
		set_short(DLP_REQUEST_DATA(req, 0, 8), size);
		set_byte(DLP_REQUEST_DATA(req, 0, 10), backup ? 0x80 : 0);
		set_byte(DLP_REQUEST_DATA(req, 0, 11), 0); 	/* Reserved */


		if (size + 12 > DLP_BUF_SIZE) {
			fprintf(stderr, "Data too large\n");
			return -131;
		}
		memcpy(DLP_REQUEST_DATA(req, 0, 12), buffer, size);

		result = dlp_exec (sd, req, &res);

		dlp_request_free(req);
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
				  void *buffer, recordid_t * id,
				  int *index, int *size, int *attr)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	if (pi_version(sd) < 0x0101) {
		/* Emulate for PalmOS 1.0 */
		int 	cat,
		LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,

		    fHandle, incategory);

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
		while ((rec >= 0) && (cat != incategory));

		return rec;
	} else {
		int data_len;
		
		Trace(ReadNextModifiedRecInCategoryV2);

		req = dlp_request_new(dlpFuncReadNextModifiedRecInCategory, 1, 2);

		set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
		set_byte(DLP_REQUEST_DATA(req, 0, 1), incategory);

		result = dlp_exec(sd, req, &res);
		
		dlp_request_free(req);
		
		if (result >= 0) {
			data_len = res->argv[0]->len - 10;
			
			if (id)
				*id = get_long(DLP_RESPONSE_DATA(res, 0, 0));
			if (index)
				*index = get_short(DLP_RESPONSE_DATA(res, 0, 4));
			if (size)
				*size = get_short(DLP_RESPONSE_DATA(res, 0, 6));
			if (attr)
				*attr = get_byte(DLP_RESPONSE_DATA(res, 0, 8));
			if (buffer)
				memcpy(buffer, DLP_RESPONSE_DATA(res, 0, 10), data_len);

			CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, record_dump(DLP_RESPONSE_DATA(res, 0, 0)));
		} else {
			data_len = result;
		}
		
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
int
dlp_ReadNextModifiedRec(int sd, int fHandle, void *buffer, recordid_t * id,
			int *index, int *size, int *attr, int *category)
{
	int 	result,
		data_len;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(ReadNextModifiedRec);
	
	req = dlp_request_new (dlpFuncReadNextModifiedRec, 1, 1);
	
	result = dlp_exec (sd, req, &res);

	dlp_request_free(req);
	
	if (result >= 0) {
		data_len = res->argv[0]->len -10;
		if (id)
			*id = get_long(DLP_RESPONSE_DATA(res, 0, 0));
		if (index)
			*index = get_short(DLP_RESPONSE_DATA(res, 0, 4));
		if (size)
			*size = get_short(DLP_RESPONSE_DATA(res, 0, 6));
		if (attr)
			*attr = get_byte(DLP_RESPONSE_DATA(res, 0, 8));
		if (category)
			*category = get_byte(DLP_RESPONSE_DATA(res, 0, 9));
		if (buffer)
			memcpy(buffer, DLP_RESPONSE_DATA(res, 0, 10), data_len);

		CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, record_dump(DLP_RESPONSE_DATA(res, 0, 0)));
	} else {
		data_len = result;
	}
	
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
int
dlp_ReadRecordById(int sd, int fHandle, recordid_t id, void *buffer,
		   int *index, int *size, int *attr, int *category)
{
	int 	result,
		data_len;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(ReadRecordById);

	req = dlp_request_new(dlpFuncReadRecord, 1, 10);
	
	set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
	set_long(DLP_REQUEST_DATA(req, 0, 2), id); 
	set_short(DLP_REQUEST_DATA(req, 0, 6), 0); /* Offset into record */
	set_short(DLP_REQUEST_DATA(req, 0, 8), buffer ? DLP_BUF_SIZE : 0);	/* length to return */

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result >= 0) {
		data_len = res->argv[0]->len - 10;
		if (index)
			*index = get_short(DLP_RESPONSE_DATA(res, 0, 4));
		if (size)
			*size = get_short(DLP_RESPONSE_DATA(res, 0, 6));
		if (attr)
			*attr = get_byte(DLP_RESPONSE_DATA(res, 0, 8));
		if (category)
			*category = get_byte(DLP_RESPONSE_DATA(res, 0, 9));
		if (buffer)
			memcpy(buffer, DLP_RESPONSE_DATA(res, 0, 10), data_len);

		CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, record_dump(DLP_RESPONSE_DATA(res, 0, 0)));
	} else {
		data_len = result;
	}
	
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
int
dlp_ReadRecordByIndex(int sd, int fHandle, int index, void *buffer,
		      recordid_t * id, int *size, int *attr, int *category)
{
	int 	result,
		data_len;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(ReadRecordByIndex);

	req = dlp_request_new_with_argid(dlpFuncReadRecord, 0x21, 1, 8);
	
	set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0x00);
	set_short(DLP_REQUEST_DATA(req, 0, 2), index);
	set_short(DLP_REQUEST_DATA(req, 0, 4), 0);	/* Offset into record */
	set_short(DLP_REQUEST_DATA(req, 0, 6), buffer ? DLP_BUF_SIZE : 0);	/* length to return */

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result >= 0) {
		data_len = res->argv[0]->len - 10;
		if (id)
			*id = get_long(DLP_RESPONSE_DATA(res, 0, 0));
		if (size)
			*size = get_short(DLP_RESPONSE_DATA(res, 0, 6));
		if (attr)
			*attr = get_byte(DLP_RESPONSE_DATA(res, 0, 8));
		if (category)
			*category = get_byte(DLP_RESPONSE_DATA(res, 0, 9));
		if (buffer)
			memcpy(buffer, DLP_RESPONSE_DATA(res, 0, 10), data_len);

		data_len = result;
	}

	dlp_response_free(res);
	return data_len;
}

#ifdef PI_DEBUG
static void record_dump (unsigned char *data)
{
	int size, flags;
	LOG(PI_DBG_DLP, PI_DBG_LVL_INFO,
	size = get_short(&data[6]);
	flags = get_byte(&data[8]);

	LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
	    "  ID: 0x%8.8lX, Index: %d, Category: %d\n"
	    "  Flags: %s%s%s%s%s%s (0x%2.2X), and %d bytes:\n",
	    (unsigned long) get_long(&data[0]),
	    get_short(&data[4]), get_byte(&data[9]),
	    (flags & dlpRecAttrDeleted) ? " Deleted" : "",
	    (flags & dlpRecAttrDirty) ? " Dirty" : "",
	    flags, size);
	    (flags & dlpRecAttrSecret) ? " Secret" : "",
	    (flags & dlpRecAttrArchived) ? " Archive" : "",

	dlp_response_free (res);
	
	return result;
}
