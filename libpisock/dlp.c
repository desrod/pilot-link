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

/*
 * Documentation marked with DHS is my fault.
 *               -- David H. Silber <pilot@SilberSoft.com>
 */

/***********************************************************************
 *  The functions in this file use the Desktop Link Protocol to interact
 *  with a connected Palm device.
 *
 *  The first section of this file contains utility functions.
 *
 *  Functions useful to people writing conduits are at the end.  (Search
 *  for '<Conduit>'.)
 *
 *  -- DHS
 ***********************************************************************/


/***********************************************************************
 *  Key to documentation
 *
 *  Parameters marked with '-->' are data passed in to the function.
 *  Parameters marked with '<--' are data passed back from the function.
 *    If the argument is a NULL, no data is returned.
 *  Parameters marked with '<->' are used for data passed in and then
 *    (possibly modified) data passed back.
 *
 *  -- DHS
 ***********************************************************************/


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef WIN32
#include <winsock.h>		/* for hton* */
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef HAVE_ERRNO_H
#include <errno.h>

/* For systems that don't provide ENOMSG. Use EINVAL instead. */
#ifndef        ENOMSG
#define ENOMSG EINVAL
#endif /* ENOMSG */

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

#define DLP_REQUEST_DATA(req, arg, offset) &req->argv[arg]->data[offset]
#define DLP_RESPONSE_DATA(res, arg, offset) &res->argv[arg]->data[offset]

#define	RequireDLPVersion(sd,major,minor)	\
	if (pi_version(sd) < (((major)<<8) | (minor))) \
		return dlpErrNotSupp

/* Define prototypes */
#ifdef PI_DEBUG
static void record_dump (char *data);
#endif

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

/* Look at "Error codes" in VFSMgr.h in the Palm SDK for their
   implementation */
char * vfs_errorlist[] = {
	"No error",
	"Buffer Overflow",
	"Generic file error",
	"File reference is invalid",
	"File still open",
	"Permission denied",
	"File or folder already exists",
	"FileEOF",
	"File not found",
	"volumereference is invalid",
	"Volume still mounted",
	"No filesystem",
	"Bad data",
	"Non-empty directory",
	"Invalid path or filename",
	"Volume full - not enough space",
	"Unimplemented",
	"Not a directory",
	"Is a directory",
	"Directory not found",
	"Name truncated"
};

/* Look at "Error codes" in ExpansionMgr.h in the Palm SDK for their
   implementation */
char * exp_errorlist[] = {
	"No error",
	"Unsupported Operation",
	"Not enough Power",
	"Card not present",
	"Invalid slotreference number",
	"Slot deallocated",
	"Card no sector read/write",
	"Card read only",
	"Card bad sector",
	"Protected sector",
	"Not open (slot driver)",
	"still open (slot driver)",
	"Unimplemented",
	"Enumeration empty",
	"Incompatible API version"
};

int dlp_trace = 0;

#ifdef PI_DEBUG
#define Trace(name) \
  LOG((PI_DBG_DLP, PI_DBG_LVL_INFO, "DLP %s sd: %d\n", #name, sd));
#define Expect(count)    \
  if (result < count) {  \
    if (result < 0) {    \
      LOG((PI_DBG_DLP, PI_DBG_LVL_ERR, "DLP Error  %s (%d)\n", dlp_errorlist[-result], result)); \
    } else {             \
      LOG((PI_DBG_DLP, PI_DBG_LVL_ERR, "DLP Read %d bytes, expected at least %d\n", result, count)); \
      result = -128;     \
    }                    \
    return result;       \
  } else                 \
      LOG((PI_DBG_DLP, PI_DBG_LVL_INFO, "DLP RX %d bytes\n", result));
#else
#define Trace(name)
#define Expect(count)
#endif

/***********************************************************************
 *
 * Function:	dlp_strerror
 *
 * Summary:	lookup text for dlp error
 *
 * Parameters:	error number
 *
 * Returns:     char* to error text string
 *
 ***********************************************************************/
char 
*dlp_strerror(int error)
{
	if (error < 0)
		error = -error;
	if ((unsigned int) error >= (sizeof(dlp_errorlist)/(sizeof(char *))))
		return "Unknown error";
	else
		return dlp_errorlist[error];
}


/***********************************************************************
 *
 * Function:	dlp_arg_new
 *
 * Summary:	create a dlpArg instance
 *
 * Parameters:	id, length of data
 *
 * Returns:     dlpArg* or NULL on failure
 *
 ***********************************************************************/
struct dlpArg
*dlp_arg_new (int id, size_t len) 
{
	struct dlpArg *arg;
	
	arg = malloc(sizeof (struct dlpArg));
	if (arg != NULL) {
		arg->id = id;
		arg->len = len;

		if (len > 0) {
			arg->data = malloc (len);
			if (arg->data == NULL) {
				free(arg);
				arg = NULL;
			}
		} else {
			arg->data = NULL;
		}
	}	
	
	return arg;
}


/***********************************************************************
 *
 * Function:	dlp_arg_free
 *
 * Summary:	frees a dlpArg instance
 *
 * Parameters:	dlpArg*
 *
 * Returns:     void
 *
 ***********************************************************************/
void
dlp_arg_free (struct dlpArg *arg)
{
	if (arg->data != NULL)
		free (arg->data);
	free (arg);
}


/***********************************************************************
 *
 * Function:	dlp_arg_len
 *
 * Summary:	computes aggregate length of data members associated with
 *		an array of dlpArg instances
 *
 * Parameters:	number of dlpArg instances, dlpArg**
 *
 * Returns:     aggregate length or -1 on error
 *
 ***********************************************************************/
int
dlp_arg_len (int argc, struct dlpArg **argv)
{
	int i, len = 0;
	
	for (i = 0; i < argc; i++) {
		struct dlpArg *arg = argv[i];
		
		if (arg->len < PI_DLP_ARG_TINY_LEN &&
		    (arg->id & (PI_DLP_ARG_FLAG_SHORT | PI_DLP_ARG_FLAG_LONG)) == 0)
			len += 2;
		else if (arg->len < PI_DLP_ARG_SHORT_LEN &&
		         (arg->id & PI_DLP_ARG_FLAG_LONG) == 0)
			len += 4;
		else if (arg->len < PI_DLP_ARG_LONG_LEN) 
			len += 6;
		else
			return -1;
		
		len += arg->len;
	}

	return len;
}


/***********************************************************************
 *
 * Function:	dlp_request_new
 *
 * Summary:	creates a new dlpRequest instance 
 *
 * Parameters:	dlpFunction command, number of dlpArgs, lengths of dlpArgs
 *		data member
 *
 * Returns:     dlpRequest* or NULL if failure
 *
 ***********************************************************************/
struct dlpRequest*
dlp_request_new (enum dlpFunctions cmd, int argc, ...) 
{
	struct dlpRequest *req;
	va_list ap;
	int 	i,
		j;
	
	req = malloc (sizeof (struct dlpRequest));

	if (req != NULL) {
		req->cmd = cmd;
		req->argc = argc;

		if (argc) {
			req->argv = malloc (sizeof (struct dlpArg *) * argc);
			if (req->argv == NULL) {
				free(req);
				req = NULL;
				goto done;
			}
		} else {
			req->argv = NULL;
		}
	
		va_start (ap, argc);
		for (i = 0; i < argc; i++) {
			size_t len;

			len = va_arg (ap, size_t);
			req->argv[i] = dlp_arg_new (PI_DLP_ARG_FIRST_ID + i,
				len);
			if (req->argv[i] == NULL) {
				for (j = 0; j < i; j++)
					dlp_arg_free(req->argv[j]);
				free(req);
				break;
			}
				
		}
		va_end (ap);
	}
	
done:
	return req;	
}


/***********************************************************************
 *
 * Function:	dlp_request_new_with_argid
 *
 * Summary:	creates a new dlpRequest instance with argid
 *
 * Parameters:	dlpFunction command, number of dlpArgs, argid,
 *		lengths of dlpArgs data member
 *
 * Returns:     dlpRequest* or NULL if failure
 *
 ***********************************************************************/
struct dlpRequest*
dlp_request_new_with_argid (enum dlpFunctions cmd, int argid, int argc, ...)
{
	struct dlpRequest *req;
	va_list ap;
	int	i,
		j;
	
	req = malloc (sizeof (struct dlpRequest));

	if (req != NULL) {

		req->cmd = cmd;
		req->argc = argc;
		if (argc) {
			req->argv = malloc (sizeof (struct dlpArg *) * argc);
			if (req->argv == NULL) {
				free(req);
				req = NULL;
				goto done;
			}
		} else {
			req->argv = NULL;
		}
	
		va_start (ap, argc);
		for (i = 0; i < argc; i++) {
			size_t len;

			len = va_arg (ap, size_t);
			req->argv[i] = dlp_arg_new (argid + i, len);
			if (req->argv[i] == NULL) {
				for (j = 0; j < i; j++)
					dlp_arg_free(req->argv[j]);
				free(req);
				break;
			}
		}
		va_end (ap);
	}

done:
	return req;
}


/***********************************************************************
 *
 * Function:	dlp_response_new
 *
 * Summary:	creates a new dlpResponse instance 
 *
 * Parameters:	dlpFunction command, number of dlpArg instances
 *
 * Returns:     dlpResponse* or NULL if failure
 *
 ***********************************************************************/
struct dlpResponse
*dlp_response_new (enum dlpFunctions cmd, int argc) 
{
	struct dlpResponse *res;
	
	res = malloc (sizeof (struct dlpResponse));

	if (res != NULL) {

		res->cmd = cmd;
		res->err = dlpErrNoError;
	
		res->argc = argc;
		if (argc) {
			res->argv = malloc (sizeof (struct dlpArg *) * argc);
			if (res->argv == NULL) {
				free(res);
				res = NULL;
			}
		} else {
			res->argv = NULL;
		}	
	}
	
	return res;
}


/***********************************************************************
 *
 * Function:	dlp_response_read
 *
 * Summary:	reads dlp response
 *
 * Parameters:	dlpResonse**, sd
 *
 * Returns:     first dlpArg response length or -1 on error
 *
 ***********************************************************************/
ssize_t
dlp_response_read (struct dlpResponse **res, int sd)
{
	struct dlpResponse *response;
	unsigned char *buf;
	short argid;
	int i,j;
	ssize_t bytes;
	size_t len;
	pi_buffer_t *dlp_buf;
	
	dlp_buf = pi_buffer_new (DLP_BUF_SIZE);
	if (dlp_buf == NULL) {
		errno = ENOMEM;
		return -1;
	}

	bytes = pi_read (sd, dlp_buf, dlp_buf->allocated);      /* buffer will grow as needed */
	if (bytes < 0) {
		pi_buffer_free (dlp_buf);
		return -1;
	}

	response = dlp_response_new (dlp_buf->data[0] & 0x7f, dlp_buf->data[1]);
	*res = response;

	if (response == NULL) {
		pi_buffer_free (dlp_buf);
		return -1;
	}

	response->err = get_short (&dlp_buf->data[2]);

	buf = dlp_buf->data + 4;
	for (i = 0; i < response->argc; i++) {
		argid = get_byte (buf) & 0x7f;
		if (get_byte(buf) & PI_DLP_ARG_FLAG_LONG) {
			len = get_long (&buf[2]);
			buf += 6;
		} else if (get_byte(buf) & PI_DLP_ARG_FLAG_SHORT) {
			len = get_short (&buf[2]);
			buf += 4;
		} else {
			argid = get_byte(buf);
			len = get_byte(&buf[1]);
			buf += 2;
		}
		
		response->argv[i] = dlp_arg_new (argid, len);
		if (response->argv[i] == NULL) {
			for (j = 0; j < i; j++)
				dlp_arg_free (response->argv[j]);
			free (response);
			pi_buffer_free (dlp_buf);
			return -1;
		}
		memcpy (response->argv[i]->data, buf, len);
		buf += len;
	}

	pi_buffer_free (dlp_buf);

	if (response->argc == 0)
		return 0;

	return response->argv[0]->len;
}


/***********************************************************************
 *
 * Function:	dlp_request_write
 *
 * Summary:	writes dlp request
 *
 * Parameters:	dlpRequest**, sd
 *
 * Returns:     response length or -1 on error
 *
 ***********************************************************************/
ssize_t
dlp_request_write (struct dlpRequest *req, int sd)
{
	unsigned char *exec_buf, *buf;
	int i;
	size_t len;
	
	len = dlp_arg_len (req->argc, req->argv) + 2;
	exec_buf = (unsigned char *) malloc (sizeof (unsigned char) * len);

	if (exec_buf == NULL)
		return -1;
	
	set_byte (&exec_buf[PI_DLP_OFFSET_CMD], req->cmd);
	set_byte (&exec_buf[PI_DLP_OFFSET_ARGC], req->argc);

	buf = &exec_buf[PI_DLP_OFFSET_ARGV];	
	for (i = 0; i < req->argc; i++) {
		struct dlpArg *arg = req->argv[i];
		short argid = arg->id;
		
		if (arg->len < PI_DLP_ARG_TINY_LEN &&
		    (argid & (PI_DLP_ARG_FLAG_SHORT | PI_DLP_ARG_FLAG_LONG)) == 0) {
			set_byte(&buf[0], argid | PI_DLP_ARG_FLAG_TINY);
			set_byte(&buf[1], arg->len);

			memcpy(&buf[2], arg->data, arg->len);
			buf += arg->len + 2;			
		} else if (arg->len < PI_DLP_ARG_SHORT_LEN &&
		           (argid & PI_DLP_ARG_FLAG_LONG) == 0) {
			set_byte(&buf[0], argid | PI_DLP_ARG_FLAG_SHORT);
			set_byte(&buf[1], 0);
			set_short(&buf[2], arg->len);

			memcpy (&buf[4], arg->data, arg->len);
			buf += arg->len + 4;			
		} else {
			set_byte (&buf[0], argid | PI_DLP_ARG_FLAG_LONG);
			set_byte(&buf[1], 0);
			set_long (&buf[2], arg->len);

			memcpy (&buf[6], arg->data, arg->len);
			buf += arg->len + 6;
		}
	}

	if (pi_write(sd, exec_buf, len) < (ssize_t)len) {
		errno = -EIO;
		i = -1;
	}

	free (exec_buf);

	return i;
}


/***********************************************************************
 *
 * Function:	dlp_request_free
 *
 * Summary:	frees a dlpRequest instance
 *
 * Parameters:	dlpRequest*
 *
 * Returns:     void
 *
 ***********************************************************************/
void
dlp_request_free (struct dlpRequest *req)
{
	int i;

	if (req == NULL)
		return;

	for (i = 0; i < req->argc; i++)
		if (req->argv[i] != NULL)
			dlp_arg_free (req->argv[i]);

	if (req->argv != NULL)
		free (req->argv);

	free (req);
}


/***********************************************************************
 *
 * Function:	dlp_response_free
 *
 * Summary:	frees a dlpResponse instance
 *
 * Parameters:	dlpResponse*
 *
 * Returns:     void
 *
 ***********************************************************************/
void
dlp_response_free (struct dlpResponse *res) 
{
	int i;

	if (res == NULL)
		return;
	
	for (i = 0; i < res->argc; i++)
		if (res->argv[i] != NULL)
			dlp_arg_free (res->argv[i]);
	
	if (res->argv != NULL)
		free (res->argv);

	free (res);	
}


/***********************************************************************
 *
 * Function:	dlp_exec
 *
 * Summary:	writes a dlp request and reads the response
 *
 * Parameters:	dlpResponse*
 *
 * Returns:     the number of response bytes, or -1 on error
 *
 ***********************************************************************/
int
dlp_exec(int sd, struct dlpRequest *req, struct dlpResponse **res)
{
	int bytes;
	*res = NULL;
	
	if (dlp_request_write (req, sd) < req->argc) {
		errno = -EIO;
		return -1;
	}

	if ((bytes=dlp_response_read (res, sd)) < 0) {
		errno = -EIO;
		return -1;
	}

	/* Check to make sure the response is for this command */
	if ((*res)->cmd != req->cmd) {
		
		/* The Tungsten T returns the wrong code for VFSVolumeInfo */
		if (req->cmd != dlpFuncVFSVolumeInfo ||
				(*res)->cmd != dlpFuncVFSVolumeSize) {
			errno = -ENOMSG;
			return -1;
		}
	}

	/* Check to make sure there was no error  */
	if ((*res)->err != dlpErrNoError) {
		errno = -ENOMSG;
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
 * Function:    dlp_ptohdate
 *
 * Summary:     Convert Palm format to time_t
 *
 * Parameters:  char* to time data buffer
 *
 * Returns:     time_t struct to mktime
 *
 ***********************************************************************/
static
time_t dlp_ptohdate(const char *data)
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

	if (data[0] == 0 && data[1] == 0) {

		/* This original calculation was wrong, and reported one day
		   earlier than it was supposed to report. You can verify
		   this with the following:

			perl -e '$date=localtime(0x83D8FE00); print $date,"\n"'

		return (time_t) 0x83D8FE00;	// Wed Dec 30 16:00:00 1903 GMT

		Here are others, depending on what your system requirements are: 

		return (time_t) 0x83D96E80;	// Thu Dec 31 00:00:00 1903 GMT
		return (time_t) 0x00007080;	// Thu Jan  1 00:00:00 1970 GMT

		Palm's own Conduit Development Kit references using 1/1/1904, 
		so that's what we'll use here until something else breaks
		it.
		*/

		return (time_t) 0x83DAC000;	/* Fri Jan  1 00:00:00 1904 GMT */
	}
	return mktime(&t);
}

/***********************************************************************
 *
 * Function:    dlp_htopdate
 *
 * Summary:     Convert time_t to Palm format
 *
 * Parameters:  time_t, char* to time data buffer
 *
 * Returns:     void
 *
 ***********************************************************************/
static void
dlp_htopdate(time_t time, char *data)
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

	return;
}


/***********************************************************************
 *  Functions for use by conduit programmers start here.   <Conduit>
 *
 *  -- DHS
 ***********************************************************************/


/***********************************************************************
 *
 * Function:    dlp_GetSysDateTime
 *
 * Summary:     DLP 1.0 GetSysDateTime function to get device date 
 *		and time
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int
dlp_GetSysDateTime(int sd, time_t * t)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(GetSysDateTime);

	req = dlp_request_new(dlpFuncGetSysDateTime, 0);
	
	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result >= 0) {
		*t = dlp_ptohdate(DLP_RESPONSE_DATA (res, 0, 0));

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			"DLP GetSysDateTime %s", ctime(t)));
	}

	dlp_response_free(res);
	
	return result;
}


/***********************************************************************
 *
 * Function:    dlp_SetSysDateTime
 *
 * Summary:     DLP 1.0 SetSysDateTime function to set the device date
 *		and time
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_SetSysDateTime(int sd, time_t time)
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
 * Function:    dlp_ReadStorageInfo
 *
 * Summary:     DLP 1.0 ReadStorageInfo to read ROM/RAM regions
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int
dlp_ReadStorageInfo(int sd, int cardno, struct CardInfo *c)
{
	int 	result;
	size_t len1, len2;
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

		len2 = get_byte(DLP_RESPONSE_DATA(res, 0, 29));
		memcpy(c->manufacturer, DLP_RESPONSE_DATA(res, 0, 30 + len1),
			len2);
		c->manufacturer[len2] = '\0';

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP Read Cardno: %d, Card Version: %d, Creation time: %s",
		    c->card, c->version, ctime(&c->creation)));
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "  Total ROM: %lu, Total RAM: %lu, Free RAM: %lu\n",
		    c->romSize, c->ramSize, c->ramFree));
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "  Card name: '%s'\n", c->name));
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
 * Function:    dlp_ReadSysInfo
 *
 * Summary:     Read the System Information (device DLP version, ROM
 *              version, product ID, maximum record/resource size)
 *
 * Parameters:  sd        --> socket descriptor
 *              s         <-- system information
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int
dlp_ReadSysInfo(int sd, struct SysInfo *s)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	Trace(ReadSysInfo);

	req = dlp_request_new (dlpFuncReadSysInfo, 1, 4);

	set_short (DLP_REQUEST_DATA (req, 0, 0), PI_DLP_VERSION_MAJOR);
	set_short (DLP_REQUEST_DATA (req, 0, 2), PI_DLP_VERSION_MINOR);
	
	result = dlp_exec(sd, req, &res);

	dlp_request_free (req);

	if (result >= 0) {
		s->romVersion = get_long (DLP_RESPONSE_DATA (res, 0, 0));
		s->locale = get_long (DLP_RESPONSE_DATA (res, 0, 4));
		/* The 8th byte is a filler byte */
		s->prodIDLength = get_byte (DLP_RESPONSE_DATA (res, 0, 9));
		memcpy(s->prodID, DLP_RESPONSE_DATA(res, 0, 10),
			 s->prodIDLength);

		if (res->argc > 1) {
			/* response added in DLP 1.2 */
			s->dlpMajorVersion =
				 get_short (DLP_RESPONSE_DATA (res, 1, 0));
			s->dlpMinorVersion =
				 get_short (DLP_RESPONSE_DATA (res, 1, 2));
			s->compatMajorVersion =
				 get_short (DLP_RESPONSE_DATA (res, 1, 4));
			s->compatMinorVersion =
				 get_short (DLP_RESPONSE_DATA (res, 1, 6));
			s->maxRecSize =
				get_long  (DLP_RESPONSE_DATA (res, 1, 8));

		} else {
			s->dlpMajorVersion = 0;
			s->dlpMinorVersion = 0;
			s->compatMajorVersion = 0;
			s->compatMinorVersion = 0;
			s->maxRecSize = 0;
		}

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadSysInfo ROM Ver=0x%8.8lX Locale=0x%8.8lX\n",
		    s->romVersion, s->locale));
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "  Product ID=0x%8.8lX\n", s->prodID));
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "  DLP Major Ver=0x%4.4lX DLP Minor Ver=0x%4.4lX\n",
		    s->dlpMajorVersion, s->dlpMinorVersion));
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "  Compat Major Ver=0x%4.4lX Compat Minor Vers=0x%4.4lX\n",
		    s->compatMajorVersion, s->compatMinorVersion));
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "  Max Rec Size=%ld\n", s->maxRecSize));

	}

	dlp_response_free (res);
	
	return result;
}


/***********************************************************************
 *
 * Function:    dlp_ReadDBList
 *
 * Summary:     Iterate through the list of databases on the Palm
 *
 * Parameters:  sd	--> client socket
 *		cardno  --> card number (should be 0)
 *		flags   --> see enum dlpDBList in pi-dlp.h
 *		start   --> index of first database to list
 *		info    <-> buffer containing one or more DBInfo structs
 *			    depending on `flags' and the DLP version
 *			    running on the device.
 *
 * Returns:	A negative number on error, the number of bytes read
 *		otherwise. Use (info->used / sizeof(DBInfo)) to know how
 *		many database information blocks were returned
 *
 ***********************************************************************/
int
dlp_ReadDBList(int sd, int cardno, int flags, int start, pi_buffer_t *info)
{
	int 	result,
		i,
		count;
	struct dlpRequest *req;
	struct dlpResponse *res;
	unsigned char *p;
	struct DBInfo db;

	Trace(ReadDBList);

	pi_buffer_clear (info);

	/* `multiple' only supported in DLP 1.2 and above */
	if (pi_version(sd) < 0x0102)
		flags &= ~dlpDBListMultiple;

	req = dlp_request_new (dlpFuncReadDBList, 1, 4);
	
	set_byte(DLP_REQUEST_DATA(req, 0, 0), (unsigned char) flags);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), (unsigned char) cardno);
	set_short(DLP_REQUEST_DATA(req, 0, 2), start);

	result = dlp_exec (sd, req, &res);

	dlp_request_free(req);
	
	if (result > 0) {
		p = DLP_RESPONSE_DATA(res, 0, 0);
		db.more = get_byte(p + 2);
		count = get_byte(p + 3);

		for (i=0; i < count; i++) {
			/* PalmOS 2.0 has additional flag */
			if (pi_version(sd) > 0x0100)
				db.miscFlags = get_byte(p + 5);
			else
				db.miscFlags = 0;

			db.flags	= get_short(p + 6);
			db.type		= get_long(p + 8);
			db.creator      = get_long(p + 12);
			db.version      = get_short(p + 16);
			db.modnum       = get_long(p + 18);
			db.createDate   = get_date(p + 22);
			db.modifyDate   = get_date(p + 30);
			db.backupDate   = get_date(p + 38);
			db.index	= get_short(p + 46);

			strncpy(db.name, (char *)(p + 48), 32);
			db.name[32]     = '\0';

			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			    "DLP ReadDBList Name: '%s', Version: %d, More: %s\n",
			    db.name, db.version, db.more ? "Yes" : "No"));
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			    "  Creator: '%s'", printlong(db.creator)));
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			    " Type: '%s' Flags: %s%s%s%s%s%s%s%s%s%s",
			    printlong(db.type),
			    (db.flags & dlpDBFlagResource) ? "Resource " : "",
			    (db.flags & dlpDBFlagReadOnly) ? "ReadOnly " : "",
			    (db.flags & dlpDBFlagAppInfoDirty) ?
					 "AppInfoDirty " : "",
			    (db.flags & dlpDBFlagBackup) ? "Backup " : "",
			    (db.flags & dlpDBFlagReset) ? "Reset " : "",
			    (db.flags & dlpDBFlagNewer) ? "Newer " : "",
			    (db.flags & dlpDBFlagCopyPrevention) ?
					"CopyPrevention " : "",
			    (db.flags & dlpDBFlagStream) ? "Stream " : "",
			    (db.flags & dlpDBFlagOpen) ? "Open " : "",
			    (!db.flags) ? "None" : ""));
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO, " (0x%2.2X)\n", db.flags));
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			    "  Modnum: %ld, Index: %d, Creation date: %s",
			    db.modnum, db.index, ctime(&db.createDate)));
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			    " Modification date: %s", ctime(&db.modifyDate)));
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO, 
			    " Backup date: %s", ctime(&db.backupDate)));

			if (pi_buffer_append(info, &db, sizeof(db)) == NULL)
			{
				errno = ENOMEM;
				result = -1;
				break;
			}

			p += get_byte(p + 4);
		}
	}

	dlp_response_free (res);

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_FindDBInfo
 *
 * Summary:     Search for a database on the Palm
 *		FIXME: could read multiple databases at once using
 *		the dlpDBListMultiple flag
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_FindDBInfo(int sd, int cardno, int start, const char *dbname,
	       unsigned long type, unsigned long creator,
	       struct DBInfo *info)
{
	int 	i;
	pi_buffer_t *buf;

	/* This function does not match any DLP layer function, but is
	   intended as a shortcut for programs looking for databases. It
	   uses a fairly byzantine mechanism for ordering the RAM databases
	   before the ROM ones.  You must feed the "index" slot from the
	   returned info in as start the next time round. */

	buf = pi_buffer_new (sizeof (struct DBInfo));
	if (buf == NULL) {
		errno = ENOMEM;
		return -1;
	}

	if (start < 0x1000) {
		i = start;
		while (dlp_ReadDBList(sd, cardno, 0x80, i, buf) >= 0) {
			memcpy (info, buf->data, sizeof(struct DBInfo));
			if ((!dbname || strcmp(info->name, dbname) == 0)
			    && (!type || info->type == type)
			    && (!creator || info->creator == creator))
				goto found;
			i = info->index + 1;
		}
		start = 0x1000;
	}

	i = start & 0xFFF;
	while (dlp_ReadDBList(sd, cardno, 0x40, i, buf) >= 0) {
		memcpy (info, buf->data, sizeof(struct DBInfo));
		if ((!dbname || strcmp(info->name, dbname) == 0)
		    && (!type || info->type == type)
		    && (!creator || info->creator == creator))
		{
			info->index |= 0x1000;
			goto found;
		}
		i = info->index + 1;
	}

	pi_buffer_free (buf);
	return -1;

found:
	pi_buffer_free (buf);
	return 0;
}


/***********************************************************************
 *
 * Function:	dlp_FindDBByName
 *
 * Summary:	Search for a database on the Palm by explicit name
 *
 * Parameters:	None
 *
 * Returns:	Nothing
 *
 ***********************************************************************/
int
dlp_FindDBByName (int sd, int cardno, PI_CONST char *name, unsigned long *localid,
	int *dbhandle, struct DBInfo *info, struct DBSizeInfo *size)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	int flags = 0;
	
	Trace(FindDBByName);

	if (pi_version(sd) < 0x0102)
		return -129;

	req = dlp_request_new(dlpFuncFindDB, 1, 2 + (strlen(name) + 1));

	if (localid || dbhandle || info)
		flags |= dlpFindDBOptFlagGetAttributes;
	if (size)
		flags |= dlpFindDBOptFlagGetSize;

	set_byte(DLP_REQUEST_DATA(req, 0, 0), flags);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), cardno);
	strcpy(DLP_REQUEST_DATA(req, 0, 2), name);

	result = dlp_exec(sd, req, &res);
	
	dlp_request_free(req);
	
	if (result >= 0) {
		if (localid)
			*localid = get_long(DLP_RESPONSE_DATA(res, 0, 2));
		if (dbhandle)
			*dbhandle = get_long(DLP_RESPONSE_DATA(res, 0, 6));

		if (info) {
			info->more = 0;
			info->miscFlags =
				get_byte(DLP_RESPONSE_DATA(res, 0, 11));
			info->flags = get_short(DLP_RESPONSE_DATA(res, 0, 12));
			info->type = get_long(DLP_RESPONSE_DATA(res, 0, 14));
			info->creator = get_long(DLP_RESPONSE_DATA(res, 0, 18));
			info->version =
				 get_short(DLP_RESPONSE_DATA(res, 0, 22));
			info->modnum = get_long(DLP_RESPONSE_DATA(res, 0, 24));
			info->createDate =
				 get_date(DLP_RESPONSE_DATA(res, 0, 28));
			info->modifyDate =
				 get_date(DLP_RESPONSE_DATA(res, 0, 36));
			info->backupDate =
				 get_date(DLP_RESPONSE_DATA(res, 0, 44));
			info->index =
				 get_short(DLP_RESPONSE_DATA(res, 0, 52));

			strncpy(info->name, DLP_RESPONSE_DATA(res, 0, 54), 32);
			info->name[32] 		= '\0';

			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			     "DLP ReadDBList Name: '%s', "
				 "Version: %d, More: %s\n",
			     info->name, info->version,
				 info->more ? "Yes" : "No"));
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			     "  Creator: '%s'", printlong(info->creator)));
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			     " Type: '%s' Flags: %s%s%s%s%s%s%s%s%s%s",
			     printlong(info->type),
			     (info->flags & dlpDBFlagResource) ?
				"Resource " : "",
			     (info->flags & dlpDBFlagReadOnly) ?
				"ReadOnly " : "",
			     (info->flags & dlpDBFlagAppInfoDirty) ?
				"AppInfoDirty " : "",
			     (info->flags & dlpDBFlagBackup) ?
				"Backup " : "",
			     (info->flags & dlpDBFlagReset) ?
				"Reset " : "",
			     (info->flags & dlpDBFlagNewer) ?
				"Newer " : "",
			     (info->flags & dlpDBFlagCopyPrevention) ?
				"CopyPrevention " : "",
			     (info->flags & dlpDBFlagStream) ?
				"Stream " : "",
			     (info->flags & dlpDBFlagOpen) ?
				"Open " : "",
			     (!info->flags) ? "None" : ""));
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
				" (0x%2.2X)\n", info->flags));
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			     	"  Modnum: %ld, Index: %d, "
				"Creation date: %s",
			    	info->modnum, info->index,
				ctime(&info->createDate)));
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			     	" Modification date: %s",
				ctime(&info->modifyDate)));
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO, 
			     " Backup date: %s",
				ctime(&info->backupDate)));
		}
		
		if (size) {
			size->numRecords =
				get_long(DLP_RESPONSE_DATA(res, 1, 0));
			size->totalBytes =
				get_long(DLP_RESPONSE_DATA(res, 1, 4));
			size->dataBytes =
				get_long(DLP_RESPONSE_DATA(res, 1, 8));
			size->appBlockSize =
				get_long(DLP_RESPONSE_DATA(res, 1, 12));
			size->sortBlockSize =
				get_long(DLP_RESPONSE_DATA(res, 1, 16));
			size->maxRecSize =
				get_long(DLP_RESPONSE_DATA(res, 1, 20));
		}
	}
	
	dlp_response_free(res);
	
	return result;	
}


/***********************************************************************
 *
 * Function:    dlp_FindDBByOpenHandle
 *
 * Summary:     Search for a database on the Palm by database handle
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
dlp_FindDBByOpenHandle (int sd, int dbhandle, int *cardno,
	 unsigned long *localid, struct DBInfo *info, struct DBSizeInfo *size)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	int flags = 0;
	
	Trace(FindDBByName);

	if (pi_version(sd) < 0x0102)
		return -129;

	req = dlp_request_new_with_argid(dlpFuncFindDB, 0x21, 1, 2);

	if (cardno || localid || info)
		flags |= dlpFindDBOptFlagGetAttributes;
	if (size)
		flags |= (dlpFindDBOptFlagGetSize | dlpFindDBOptFlagMaxRecSize);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), flags);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), dbhandle);

	result = dlp_exec(sd, req, &res);
	
	dlp_request_free(req);
	
	if (result >= 0) {
		if (cardno)
			*cardno = get_byte(DLP_RESPONSE_DATA(res, 0, 0));
		if (localid)
			*localid = get_long(DLP_RESPONSE_DATA(res, 0, 2));

		if (info) {
			info->more = 0;
			info->miscFlags =
				get_byte(DLP_RESPONSE_DATA(res, 0, 11));
			info->flags =
				 get_short(DLP_RESPONSE_DATA(res, 0, 12));
			info->type =
				 get_long(DLP_RESPONSE_DATA(res, 0, 14));
			info->creator =
				 get_long(DLP_RESPONSE_DATA(res, 0, 18));
			info->version =
				 get_short(DLP_RESPONSE_DATA(res, 0, 22));
			info->modnum =
				get_long(DLP_RESPONSE_DATA(res, 0, 24));
			info->createDate =
				get_date(DLP_RESPONSE_DATA(res, 0, 28));
			info->modifyDate =
				get_date(DLP_RESPONSE_DATA(res, 0, 36));
			info->backupDate =
				get_date(DLP_RESPONSE_DATA(res, 0, 44));
			info->index =
				 get_short(DLP_RESPONSE_DATA(res, 0, 52));
			strncpy(info->name, DLP_RESPONSE_DATA(res, 0, 54), 32);
			info->name[32] 		= '\0';

			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			     "DLP ReadDBList Name: '%s', "
				 "Version: %d, More: %s\n",
			     info->name, info->version,
				 info->more ? "Yes" : "No"));
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			     "  Creator: '%s'", printlong(info->creator)));
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			     " Type: '%s' Flags: %s%s%s%s%s%s%s%s%s%s",
			     printlong(info->type),
			     (info->flags & dlpDBFlagResource) ?
				 "Resource " : "",
			     (info->flags & dlpDBFlagReadOnly) ?
				 "ReadOnly " : "",
			     (info->flags & dlpDBFlagAppInfoDirty) ?
				 "AppInfoDirty " : "",
			     (info->flags & dlpDBFlagBackup) ?
				 "Backup " : "",
			     (info->flags & dlpDBFlagReset) ?
				 "Reset " : "",
			     (info->flags & dlpDBFlagNewer) ?
				 "Newer " : "",
			     (info->flags & dlpDBFlagCopyPrevention) ?
				 "CopyPrevention " : "",
			     (info->flags & dlpDBFlagStream) ?
				 "Stream " : "",
			     (info->flags & dlpDBFlagOpen) ?
				 "Open " : "",
			     (!info->flags) ?
				 "None" : ""));

			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
				 " (0x%2.2X)\n", info->flags));
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			     "  Modnum: %ld, Index: %d, "
				"Creation date: %s",
			     	info->modnum, info->index,
				ctime(&info->createDate)));
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			     " Modification date: %s",
				ctime(&info->modifyDate)));
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO, 
			     " Backup date: %s",
			 	ctime(&info->backupDate)));
		}
		
		if (size) {
			size->numRecords =
				 get_long(DLP_RESPONSE_DATA(res, 1, 0));
			size->totalBytes =
				 get_long(DLP_RESPONSE_DATA(res, 1, 4));
			size->dataBytes =
				 get_long(DLP_RESPONSE_DATA(res, 1, 8));
			size->appBlockSize =
				 get_long(DLP_RESPONSE_DATA(res, 1, 12));
			size->sortBlockSize =
				 get_long(DLP_RESPONSE_DATA(res, 1, 16));
			size->maxRecSize =
				 get_long(DLP_RESPONSE_DATA(res, 1, 20));
		}
	}
	
	dlp_response_free(res);
	
	return result;	
}


/***********************************************************************
 *
 * Function:    dlp_FindDBByTypeCreator
 *
 * Summary:     Search for a database on the Palm by CreatorID
 *
 * Parameters:  None
 *
 * Returns:     Creator ID in 'result'
 *
 ***********************************************************************/
int
dlp_FindDBByTypeCreator (int sd, unsigned long type, unsigned long creator,
 	int start, int latest, int *cardno, unsigned long *localid,
	int *dbhandle, struct DBInfo *info, struct DBSizeInfo *size)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	int flags = 0, search_flags = 0;
	
	Trace(FindDBByName);

	if (pi_version(sd) < 0x0102)
		return -129;

	req = dlp_request_new_with_argid(dlpFuncFindDB, 0x22, 1, 10);

	if (cardno || localid || dbhandle || info)
		flags |= dlpFindDBOptFlagGetAttributes;
	if (size)
		flags |= (dlpFindDBOptFlagGetSize |
			 dlpFindDBOptFlagMaxRecSize);

	if (start)
		search_flags |= dlpFindDBSrchFlagNewSearch;
	if (latest)
		search_flags |= dlpFindDBSrchFlagOnlyLatest;

	
	set_byte(DLP_REQUEST_DATA(req, 0, 0), flags);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), search_flags);
	set_long(DLP_REQUEST_DATA(req, 0, 2), type);
	set_long(DLP_REQUEST_DATA(req, 0, 6), creator);

	result = dlp_exec(sd, req, &res);
	
	dlp_request_free(req);
	
	if (result >= 0) {
		if (cardno)
			*cardno = get_byte(DLP_RESPONSE_DATA(res, 0, 0));
		if (localid)
			*localid = get_long(DLP_RESPONSE_DATA(res, 0, 2));
		if (dbhandle)
			*dbhandle = get_long(DLP_RESPONSE_DATA(res, 0, 6));
		
		if (info) {
			info->more = 0;
			info->miscFlags	=
				 get_byte(DLP_RESPONSE_DATA(res, 0, 11));
			info->flags =
				 get_short(DLP_RESPONSE_DATA(res, 0, 12));
			info->type =
				 get_long(DLP_RESPONSE_DATA(res, 0, 14));
			info->creator =
				 get_long(DLP_RESPONSE_DATA(res, 0, 18));
			info->version 	=
				 get_short(DLP_RESPONSE_DATA(res, 0, 22));
			info->modnum =
				 get_long(DLP_RESPONSE_DATA(res, 0, 24));
			info->createDate =
				 get_date(DLP_RESPONSE_DATA(res, 0, 28));
			info->modifyDate =
				 get_date(DLP_RESPONSE_DATA(res, 0, 36));
			info->backupDate =
				 get_date(DLP_RESPONSE_DATA(res, 0, 44));
			info->index =
				 get_short(DLP_RESPONSE_DATA(res, 0, 52));

			strncpy(info->name, DLP_RESPONSE_DATA(res, 0, 54), 32);
			info->name[32] 		= '\0';

			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			     "DLP ReadDBList Name: '%s', "
				 "Version: %d, More: %s\n",
			     info->name, info->version,
				 info->more ? "Yes" : "No"));
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			     "  Creator: '%s'", printlong(info->creator)));
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			     " Type: '%s' Flags: %s%s%s%s%s%s%s%s%s%s",
			     printlong(info->type),
			     (info->flags & dlpDBFlagResource) ?
				 "Resource " : "",
			     (info->flags & dlpDBFlagReadOnly) ?
				 "ReadOnly " : "",
			     (info->flags & dlpDBFlagAppInfoDirty) ?
				 "AppInfoDirty " : "",
			     (info->flags & dlpDBFlagBackup) ?
				 "Backup " : "",
			     (info->flags & dlpDBFlagReset) ?
				 "Reset " : "",
			     (info->flags & dlpDBFlagNewer) ?
				 "Newer " : "",
			     (info->flags & dlpDBFlagCopyPrevention) ?
				 "CopyPrevention " : "",
			     (info->flags & dlpDBFlagStream) ?
				 "Stream " : "",
			     (info->flags & dlpDBFlagOpen) ?
				 "Open " : "",
			     (!info->flags) ? "None" : ""));
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
				" (0x%2.2X)\n", info->flags));
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			     	"  Modnum: %ld, Index: %d, "
				"Creation date: %s",
			     	info->modnum, info->index,
				ctime(&info->createDate)));
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			     	" Modification date: %s",
				ctime(&info->modifyDate)));
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO, 
			     	" Backup date: %s",
				ctime(&info->backupDate)));
		}
		
		if (size) {
			size->numRecords = 
				get_long(DLP_RESPONSE_DATA(res, 1, 0));
			size->totalBytes =
				 get_long(DLP_RESPONSE_DATA(res, 1, 4));
			size->dataBytes =
				 get_long(DLP_RESPONSE_DATA(res, 1, 8));
			size->appBlockSize =
				 get_long(DLP_RESPONSE_DATA(res, 1, 12));
			size->sortBlockSize =
				 get_long(DLP_RESPONSE_DATA(res, 1, 16));
			size->maxRecSize =
				 get_long(DLP_RESPONSE_DATA(res, 1, 20));
		}	
	}
	
	dlp_response_free(res);
	
	return result;	
}


/***********************************************************************
 *
 * Function:    dlp_OpenDB
 *
 * Summary:     Open the database for read/write/delete/mode
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int
dlp_OpenDB(int sd, int cardno, int mode, PI_CONST char *name, int *dbhandle)
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
	
	if (result >= 0) {
		*dbhandle = get_byte(DLP_RESPONSE_DATA(res, 0, 0));

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP OpenDB Handle=%d\n", *dbhandle));
	}
	
	dlp_response_free(res);
	
	return result;
}


/***********************************************************************
 *
 * Function:    dlp_DeleteDB
 *
 * Summary:     Delete a given database on the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_DeleteDB(int sd, int card, const char *name)
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
 * Function:    dlp_CreateDB
 *
 * Summary:     Create a database on the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_CreateDB(int sd, unsigned long creator, unsigned long type, int cardno,
	 int flags, unsigned int version, const char *name, int *dbhandle)
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
	
	if (result >= 0 && dbhandle) {
		*dbhandle = get_byte(DLP_RESPONSE_DATA(res, 0, 0));

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP CreateDB Handle=%d\n", *dbhandle));
	}
	
	dlp_response_free(res);

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_CloseDB
 *
 * Summary:     Close the database opened on the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_CloseDB(int sd, int dbhandle)
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


/***********************************************************************
 *
 * Function:    dlp_CloseDB_All
 *
 * Summary:     Close all the databases opened on the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_CloseDB_All(int sd)
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
 * Function:    dlp_CallApplication
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
		    int action, size_t length, void *data,
		    unsigned long *retcode, size_t maxretlen, int *retlen,
		    void *retdata)
{
	int 	result,
		version = pi_version(sd);
	size_t	data_len;
	struct dlpRequest *req;
	struct dlpResponse *res;

	if (version >= 0x0101) {	/* PalmOS 2.0 call encoding */
		Trace(CallApplicationV1);

		req = dlp_request_new_with_argid(dlpFuncCallApplication,
			 0x21, 1, 22 + length);
		
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

		result = dlp_exec(sd, req, &res);

		dlp_request_free(req);

		if (result >= 0) {
			data_len = res->argv[0]->len - 16;
			
			if (retcode)
				*retcode =
				 get_long(DLP_RESPONSE_DATA(res, 0, 0));
			if (retlen)
				*retlen = data_len;
			if (retdata)
				memcpy(retdata, DLP_RESPONSE_DATA(res, 0, 16),
				data_len > maxretlen ? maxretlen : data_len);
			

			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			     "DLP CallApplication Result: %lu (0x%8.8lX), "
				 "and %d bytes:\n",
			     get_long(DLP_RESPONSE_DATA(res, 0, 0)), 
			     get_long(DLP_RESPONSE_DATA(res, 0, 4)),
			     data_len));
			CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, 
			      dumpdata(DLP_RESPONSE_DATA(res, 0, 16),
					(size_t)data_len));
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

		result = dlp_exec(sd, req, &res);

		dlp_request_free(req);

		if (result >= 0) {
			data_len = res->argv[0]->len - 6;
			if (retcode)
				*retcode =
				 get_short(DLP_RESPONSE_DATA(res, 0, 2));
			if (retlen)
				*retlen = data_len;
			
			if (retdata)
				memcpy(retdata, DLP_RESPONSE_DATA(res, 0, 6),
				data_len > maxretlen ? maxretlen : data_len);
			
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			     "DLP CallApplication Action: %d Result:"
				" %lu (0x%4.4lX), and %d bytes:\n",
			     get_short(DLP_RESPONSE_DATA(res, 0, 0)), 
			     get_short(DLP_RESPONSE_DATA(res, 0, 2)), 
			     get_short(DLP_RESPONSE_DATA(res, 0, 2)),
			     data_len));
			CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, 
			      dumpdata(DLP_RESPONSE_DATA(res, 0, 6),
				(size_t)data_len));
		}
		
		dlp_response_free(res);

		return result;
	}
}


/***********************************************************************
 *
 * Function:    dlp_ResetSystem
 *
 * Summary:     Reset the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_ResetSystem(int sd)
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
 * Function:    dlp_AddSyncLogEntry
 *
 * Summary:     Add text to the Palm's synchronization log
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_AddSyncLogEntry(int sd, char *entry)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(AddSyncLogEntry);

	req = dlp_request_new(dlpFuncAddSyncLogEntry, 1, strlen(entry) + 1);
	
	strcpy(DLP_REQUEST_DATA(req, 0, 0), entry);
	
	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);	

	if (result >= 0)
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP AddSyncLogEntry Entry: \n  %s\n", entry));
	
	dlp_response_free(res);
	
	return result;
}


/***********************************************************************
 *
 * Function:    dlp_ReadOpenDBInfo
 *
 * Summary:     Read the number of records in the device database
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int
dlp_ReadOpenDBInfo(int sd, int dbhandle, int *records)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(ReadOpenDBInfo);

	req = dlp_request_new(dlpFuncReadOpenDBInfo, 1, 1);
	
	set_byte(DLP_REQUEST_DATA(req, 0, 0), dbhandle);
	
	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);

	if (result >= 0) {
		if (records)
			*records = get_short(DLP_RESPONSE_DATA(res, 0, 0));
		
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadOpenDBInfo %d records\n", 
		    get_short(DLP_RESPONSE_DATA(res, 0, 0))));
	}
	
	dlp_response_free(res);
	
	return result;
}

/***********************************************************************
 *
 * Function:    dlp_SetDBInfo
 *
 * Summary:     Set the database info, passing 0 for a param leaves it
 *              unchanged.
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int
dlp_SetDBInfo (int sd, int dbhandle, int flags, int clearFlags,
	unsigned int version, time_t createDate, time_t modifyDate,
	time_t backupDate, unsigned long type, unsigned long creator)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
 	
	Trace(SetDBInfo);

	if (pi_version(sd) < 0x0102)
		return -129;

	req = dlp_request_new(dlpFuncSetDBInfo, 1, 40);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), dbhandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
	set_short(DLP_REQUEST_DATA(req, 0, 2), clearFlags);
	set_short(DLP_REQUEST_DATA(req, 0, 4), flags);
	set_short(DLP_REQUEST_DATA(req, 0, 6), version);
	set_date(DLP_REQUEST_DATA(req, 0, 8), createDate);
	set_date(DLP_REQUEST_DATA(req, 0, 16), modifyDate);
	set_date(DLP_REQUEST_DATA(req, 0, 24), backupDate);
	set_long(DLP_REQUEST_DATA(req, 0, 32), type);
	set_long(DLP_REQUEST_DATA(req, 0, 36), creator);
	
	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	dlp_response_free(res);
	
	return result;
}


/***********************************************************************
 *
 * Function:    dlp_MoveCategory
 *
 * Summary:     Move all records in a category to another category
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_MoveCategory(int sd, int handle, int fromcat, int tocat)
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

	dlp_request_free(req);	

	if (result >= 0)
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP MoveCategory Handle: %d, From: %d, To: %d\n",
		    handle, fromcat, tocat));
	
	dlp_response_free(res);
	
	return result;
}

/***********************************************************************
 *
 * Function:    dlp_OpenConduit
 *
 * Summary:     This command is sent before each conduit is opened
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_OpenConduit(int sd)
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
 * Function:    dlp_EndOfSync
 *
 * Summary:     End the sync with the given status
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_EndOfSync(int sd, int status)
{
	int 	result;
	pi_socket_t	*ps;
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
 * Function:    dlp_AbortSync
 *
 * Summary:     Enters a sync_aborted entry into the log
 *
 * Parameters:  None
 *
 * Returns:     Return value: A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_AbortSync(int sd)
{
	pi_socket_t	*ps;

	Trace(AbortSync);

	/* Pretend we sent the sync end */
	if ((ps = find_pi_socket(sd)))
		ps->state = PI_SOCK_CONEN;

	return 0;
}


/***********************************************************************
 *
 * Function:    dlp_WriteUserInfo
 *
 * Summary:     Write user information to the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_WriteUserInfo(int sd, struct PilotUser *User)
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
 * Function:    dlp_ReadUserInfo
 *
 * Summary:     Read user information from the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise 
 *
 ***********************************************************************/
int
dlp_ReadUserInfo(int sd, struct PilotUser *User)
{
	int 	result;
	size_t	userlen;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	Trace(ReadUserInfo);
	
	req = dlp_request_new (dlpFuncReadUserInfo, 0);
	
	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);

	if (result >= 0) {
		User->userID =
			 get_long(DLP_RESPONSE_DATA (res, 0, 0));
		User->viewerID =
			 get_long(DLP_RESPONSE_DATA (res, 0, 4));
		User->lastSyncPC =
			 get_long(DLP_RESPONSE_DATA (res, 0, 8));
		User->successfulSyncDate =
			 get_date(DLP_RESPONSE_DATA (res, 0, 12));
		User->lastSyncDate =
			 get_date(DLP_RESPONSE_DATA (res, 0, 20));
		userlen = 
			get_byte(DLP_RESPONSE_DATA (res, 0, 28));
		User->passwordLength  =
			 get_byte(DLP_RESPONSE_DATA (res, 0, 29));

		memcpy(User->username,
			 DLP_RESPONSE_DATA (res, 0, 30), userlen);
		memcpy(User->password,
			 DLP_RESPONSE_DATA (res, 0, 30 + userlen),
				User->passwordLength);

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    	"DLP ReadUserInfo UID=0x%8.8lX VID=0x%8.8lX "
			"PCID=0x%8.8lX\n",
			    User->userID, User->viewerID, User->lastSyncPC));
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			"  Last Sync=%s  Last Successful Sync=%s",
		    	ctime (&User->lastSyncDate),
			ctime (&User->successfulSyncDate)));
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		 	"  Username=%s\n", User->username));
	}
	
	dlp_response_free (res);
	
	return result;
}


/***********************************************************************
 *
 * Function:    dlp_ReadNetSyncInfo
 *
 * Summary:     Read Network HotSync settings from the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int
dlp_ReadNetSyncInfo(int sd, struct NetSyncInfo *i)
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
		size_t str_offset = 24;
		
		i->lanSync = get_byte(DLP_RESPONSE_DATA(res, 0, 0));
		
		i->hostName[0] = '\0';
		memcpy(i->hostName, DLP_RESPONSE_DATA(res, 0, str_offset), 
		       get_short(DLP_RESPONSE_DATA(res, 0, 18)));
		str_offset += get_short(DLP_RESPONSE_DATA(res, 0, 18));

		i->hostAddress[0] = '\0';
		memcpy(i->hostAddress, DLP_RESPONSE_DATA(res, 0, str_offset), 
		       get_short(DLP_RESPONSE_DATA(res, 0, 20)));
		str_offset += get_short(DLP_RESPONSE_DATA(res, 0, 20));

		i->hostSubnetMask[0] = '\0';
		memcpy(i->hostSubnetMask, DLP_RESPONSE_DATA(res, 0, str_offset), 
		       get_short(DLP_RESPONSE_DATA(res, 0, 22)));

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadNetSyncInfo Active: %d\n", i->lanSync ? 1 : 0));
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "  PC hostname: '%s', address '%s', mask '%s'\n",
		    i->hostName, i->hostAddress, i->hostSubnetMask));
	}

	dlp_response_free(res);
	
	return result;
}


/***********************************************************************
 *
 * Function:    dlp_WriteNetSyncInfo
 *
 * Summary:     Write Network HotSync settings to the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_WriteNetSyncInfo(int sd, struct NetSyncInfo *i)
{
	int 	result,
		str_offset = 24;
	struct dlpRequest *req;
	struct dlpResponse *res;

	if (pi_version(sd) < 0x0101)
		return -129;

	Trace(WriteNetSyncInfo);

	LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
	    "DLP ReadNetSyncInfo Active: %d\n", i->lanSync ? 1 : 0));
	LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
	    "  PC hostname: '%s', address '%s', mask '%s'\n",
	    i->hostName, i->hostAddress, i->hostSubnetMask));

	req = dlp_request_new(dlpFuncWriteNetSyncInfo, 1,
		24 + strlen(i->hostName) + 
		strlen(i->hostAddress) + strlen(i->hostSubnetMask) + 3);
	
	/* Change all settings */
	set_byte(DLP_REQUEST_DATA(req, 0, 0), 0x80 | 0x40 | 0x20 | 0x10);

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
 * Function:    dlp_RPC
 *
 * Summary:     Remote Procedure Calls interface
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_RPC(int sd, struct RPC_params *p, unsigned long *result)
{
	int 	i,
		err;
	long 	D0 = 0,
		A0 = 0;
	unsigned char *c;
	pi_buffer_t *dlp_buf;

	/* RPC through DLP breaks all the rules and isn't well documented to
	   boot */
	dlp_buf = pi_buffer_new (DLP_BUF_SIZE);
	if (dlp_buf == NULL) {
		errno = ENOMEM;
		return -1;
	}
	dlp_buf->data[0] = 0x2D;
	dlp_buf->data[1] = 1;
	dlp_buf->data[2] = 0;		/* Unknown filler */
	dlp_buf->data[3] = 0;

	InvertRPC(p);

	set_short(dlp_buf->data + 4, p->trap);
	set_long(dlp_buf->data + 6, D0);
	set_long(dlp_buf->data + 10, A0);
	set_short(dlp_buf->data + 14, p->args);

	c = dlp_buf->data + 16;
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

	pi_write(sd, dlp_buf->data, (size_t)(c - dlp_buf->data));

	err = 0;

	if (p->reply) {
		int l = pi_read(sd, dlp_buf, (size_t)(c - dlp_buf->data + 2));

		if (l < 0)
			err = l;
		else if (l < 6)
			err = -1;
		else if (dlp_buf->data[0] != 0xAD)
			err = -2;
		else if (get_short(dlp_buf->data + 2))
			err = -get_short(dlp_buf->data + 2);
		else {
			D0 = get_long(dlp_buf->data + 8);
			A0 = get_long(dlp_buf->data + 12);
			c = dlp_buf->data + 18;
			for (i = p->args - 1; i >= 0; i--) {
				if (p->param[i].byRef && p->param[i].data)
					memcpy(p->param[i].data, c + 2,
					       p->param[i].size);
				c += 2 + ((p->param[i].size + 1) & 
						(unsigned)~1);
			}
		}
	}

	pi_buffer_free (dlp_buf);

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
 * Function:    dlp_ReadFeature
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

		val = dlp_RPC(sd, &p, &result);

		if (val < 0) {
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			    "DLP ReadFeature Error: %s (%d)\n",
			    dlp_errorlist[-val], val));

			return val;
		} else if (result) {
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			    "DLP ReadFeature FtrGet error 0x%8.8lX\n",
			    (unsigned long) result));

			return (-(long) result);
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
		
		if (result >= 0) {
			if (feature)
				*feature = (unsigned long)
				 get_long(DLP_RESPONSE_DATA(res, 0, 0));
			
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			    "DLP ReadFeature Feature: 0x%8.8lX\n",
			    (unsigned long)
				get_long(DLP_RESPONSE_DATA(res, 0, 0))));
		}
		dlp_response_free(res);

		return result;
	}
	
}
#endif				/* IFDEF _PILOT_SYSPKT_H */


/***********************************************************************
 *
 * Function:    dlp_GetROMToken
 *
 * Summary:     Emulation of the SysGetROMToken function on the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise 
 *
 ***********************************************************************/
int
dlp_GetROMToken(int sd, unsigned long token, char *buffer, size_t *size)
{
	unsigned long result;

	struct RPC_params p;
	
	int val;
	unsigned long buffer_ptr;

	Trace(dlp_GetROMToken);
	
#ifdef DLP_TRACE
	if (dlp_trace) {
	  fprintf(stderr,
		  " Wrote: Token: '%s'\n",
		  printlong(token));
	}
#endif

	PackRPC(&p, 0xa340, RPC_IntReply,		// sysTrapHwrGetROMToken
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

	  PackRPC(&p, 0xa026, RPC_IntReply,		// sysTrapMemMove
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
 * Function:    dlp_ResetLastSyncPC
 *
 * Summary:     Reset the LastSyncPC ID so we can start again
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_ResetLastSyncPC(int sd)
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
 * Function:    dlp_ResetDBIndex
 *
 * Summary:     Reset the modified records index
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_ResetDBIndex(int sd, int dbhandle)
{
	int 	result;
	pi_socket_t	*ps;
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
 * Function:    dlp_ReadRecordIDList
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
			IDs[i] =
			 get_long(DLP_RESPONSE_DATA(res, 0, 2 + (i * 4)));

		if (count)
			*count = ret;

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadRecordIDList %d IDs:\n", ret));
		CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, 
		      dumpdata(DLP_RESPONSE_DATA(res, 0, 2), 
			(size_t)(ret * 4)));
	}

	dlp_response_free(res);
	
	return result;
}


/***********************************************************************
 *
 * Function:    dlp_WriteRecord
 *
 * Summary:     Writes a record to database. If recID is 0, the device
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
		int catID, void *data, size_t length, recordid_t * NewID)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(WriteRecord);

	if (length == (size_t)-1)
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
	set_byte(DLP_REQUEST_DATA(req, 0, 7), catID);

	memcpy(DLP_REQUEST_DATA(req, 0, 8), data, length);

	CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG,
		 record_dump(DLP_RESPONSE_DATA(req, 0, 0)));
	
	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result >= 0) {
		if (NewID)
			/* New record ID */
			*NewID = get_long(DLP_RESPONSE_DATA(res, 0, 0));

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
 *
 * Summary:     Deletes a record from the database or all records if the all
 *		flag is non-zero
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_DeleteRecord(int sd, int dbhandle, int all, recordid_t recID)
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
 *
 * Summary:     Delete all records in a category. The category name 
 *		is not changed.
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_DeleteCategory(int sd, int dbhandle, int category)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	if (pi_version(sd) < 0x0101) {
		/* Emulate if not connected to PalmOS 2.0 */
		int i, r, cat, attr;
		recordid_t id;

		Trace(DeleteCategoryV1);

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP DeleteCategory Emulating with: Handle: %d, "
			"Category: %d\n",
		    dbhandle, category & 0xff));

		for (i = 0;
		     dlp_ReadRecordByIndex(sd, dbhandle, i, NULL, &id,
					   &attr, &cat) >= 0; i++) {
			if ((cat != category) || (attr & dlpRecAttrDeleted)
			    || (attr & dlpRecAttrArchived))
				continue;
			r = dlp_DeleteRecord(sd, dbhandle, 0, id);
			if (r < 0)
				return r;
			i--; /* Sigh, deleting record moves it to the end. */
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
 * Function:    dlp_ReadResourceByType
 *
 * Summary:     Read the record resources by ResourceID
 *
 * Parameters:  buffer is emptied prior to reading data
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int
dlp_ReadResourceByType(int sd, int fHandle, unsigned long type, int id,
		       pi_buffer_t *buffer, int *index)
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
	set_short(DLP_REQUEST_DATA(req, 0, 10), buffer ? buffer->allocated : 0);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result >= 0) {
		data_len = res->argv[0]->len - 10;
		if (index)
			*index = get_short(DLP_RESPONSE_DATA(res, 0, 6));
		if (buffer) {
			pi_buffer_clear (buffer);
			pi_buffer_append (buffer, DLP_RESPONSE_DATA(res, 0, 10),
				(size_t)data_len);
		}
		
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadResourceByType  Type: '%s', ID: %d, "
			 "Index: %d, and %d bytes:\n",
		    printlong(type), id, 
		    get_short(DLP_RESPONSE_DATA(res, 0, 6)),(size_t)data_len));
		CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, 
		      dumpdata(DLP_RESPONSE_DATA(res, 0, 10),(size_t)data_len));
	} else {
		data_len = result;
	}

	dlp_response_free(res);
	
	return data_len;
}


/***********************************************************************
 *
 * Function:    dlp_ReadResourceByIndex
 *
 * Summary:     Read the record resources by index
 *
 * Parameters:  buffer is emptied prior to reading data
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int
dlp_ReadResourceByIndex(int sd, int fHandle, int index, pi_buffer_t *buffer,
			unsigned long *type, int *id)
{
	int 	result,
		data_len,
		large = 0;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(ReadResourceByIndex);

	/* TapWave (DLP 1.4) implements a `large' version of dlpFuncReadResource,
	 * which can return records >64k
	 */
	if (pi_version(sd) >= 0x0104) {
		req = dlp_request_new (dlpFuncReadResourceEx, 1, 12);
		set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
		set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
		set_short(DLP_REQUEST_DATA(req, 0, 2), index);
		set_long(DLP_REQUEST_DATA(req, 0, 4), 0);
		set_long(DLP_REQUEST_DATA(req, 0, 8), pi_maxrecsize(sd));
		large = 1;
	} else {
		req = dlp_request_new (dlpFuncReadResource, 1, 8);
		set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
		set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
		set_short(DLP_REQUEST_DATA(req, 0, 2), index);
		set_long(DLP_REQUEST_DATA(req, 0, 4), DLP_BUF_SIZE);
	}

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result > 0) {
		data_len = res->argv[0]->len - (large ? 12 : 10);
		if (type)
			*type = get_long(DLP_RESPONSE_DATA(res, 0, 0));
		if (id)
			*id = get_short(DLP_RESPONSE_DATA(res, 0, 4));
		if (buffer) {
			pi_buffer_clear (buffer);
			pi_buffer_append (buffer, DLP_RESPONSE_DATA(res, 0, large ? 12 : 10),
				(size_t)data_len);
		}

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadResourceByIndex Type: '%s', ID: %d, "
			"Index: %d, and %d bytes:\n",
		    printlong(get_long(DLP_RESPONSE_DATA(res, 0, 0))),
		    get_short(DLP_RESPONSE_DATA(res, 0, 4)),
		    index, data_len));
		CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, 
		      dumpdata(DLP_RESPONSE_DATA(res, 0, (large ? 12 : 10)),
			 (size_t)data_len));
	} else {
		data_len = result;
	}

	dlp_response_free(res);
	
	return data_len;
}


/***********************************************************************
 *
 * Function:    dlp_WriteResource
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
		  const void *data, size_t length)
{
	int 	result,
		large = 0;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(WriteResource);

	/* TapWave (DLP 1.4) implements a `large' version of dlpFuncWriteResource,
	 * which can store records >64k
	 */
	if (pi_version(sd) >= 0x0104) {
		req = dlp_request_new_with_argid(dlpFuncWriteResourceEx,
			PI_DLP_ARG_FIRST_ID | PI_DLP_ARG_FLAG_LONG, 1, 12 + length);
		large = 1;
	} else {
		if (length > 0xffff)
			length = 0xffff;
		req = dlp_request_new(dlpFuncWriteResource, 1, 10 + length);
	}

	set_byte(DLP_REQUEST_DATA(req, 0, 0), dbhandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
	set_long(DLP_REQUEST_DATA(req, 0, 2), type);
	set_short(DLP_REQUEST_DATA(req, 0, 6), id);
	if (large)
		set_long (DLP_REQUEST_DATA(req, 0, 8), 0);      /* device doesn't want length here (it computes it) */
	else
		set_short(DLP_REQUEST_DATA(req, 0, 8), length);

	memcpy(DLP_REQUEST_DATA(req, 0, large ? 12 : 10), data, length);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	dlp_response_free(res);

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_DeleteResource
 *
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
 * Function:    dlp_ReadAppBlock
 *
 * Summary:     Read the AppInfo block that matches the database
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int
dlp_ReadAppBlock(int sd, int fHandle, int offset, void *dbuf, int dlen)
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
		data_len = res->argv[0]->len - 2;
		if (dbuf)
			memcpy (dbuf, DLP_RESPONSE_DATA(res, 0, 2),
				(size_t)data_len);
		
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadAppBlock %d bytes\n", data_len));
		CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, 
		      dumpdata(DLP_RESPONSE_DATA(res, 0, 2),
			(size_t)data_len));
	} else {
		data_len = result;
	}

	dlp_response_free(res);
	
	return data_len;
}


/***********************************************************************
 *
 * Function:    dlp_WriteAppBlock
 *
 * Summary:     Write the AppInfo block that matches the database
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_WriteAppBlock(int sd, int fHandle, const /* @unique@ */ void *data,
      size_t length)
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
 * Function:    dlp_ReadSortBlock
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
		data_len = res->argv[0]->len - 2;
		if (dbuf)
			memcpy(dbuf, DLP_RESPONSE_DATA(res, 0, 2), 
				(size_t)data_len);
		
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadSortBlock %d bytes\n", data_len));
		CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, 
		      dumpdata(DLP_RESPONSE_DATA(res, 0, 2),
			(size_t)data_len));
	} else {
		data_len = result;
	}

	dlp_response_free(res);
	
	return data_len;
}


/***********************************************************************
 *
 * Function:    dlp_WriteSortBlock
 *
 * Summary:     Write the SortBlock that matches the database
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_WriteSortBlock(int sd, int fHandle, const /* @unique@ */ void *data,
		       size_t length)
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
 *
 * Summary:     Deletes all records which are marked as archived or 
 *		deleted in the record database
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_CleanUpDatabase(int sd, int fHandle)
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
 *
 * Summary:     Clear all the sync flags (modified, deleted, etc) in 
 *		the pilot database
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_ResetSyncFlags(int sd, int fHandle)
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
 *
 * Summary:     Iterate through all records in category returning 
 *		subsequent records on each call
 *
 * Parameters:  buffer is emptied prior to reading data
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int
dlp_ReadNextRecInCategory(int sd, int fHandle, int incategory,
			  pi_buffer_t *buffer, recordid_t * id, int *index,
			  int *attr)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	if (pi_version(sd) < 0x0101) {
		/* Emulate for PalmOS 1.0 */
		int 	cat,
			rec;
		pi_socket_t *ps;

		Trace(ReadNextRecInCategoryV1);

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadNextRecInCategory Emulating with: Handle: %d, "
			 "Category: %d\n",
		    fHandle, incategory));

		if ((ps = find_pi_socket(sd)) == 0)
			return -130;

		for (;;) {
			/* Fetch next modified record (in any category) */
			rec = dlp_ReadRecordByIndex(sd, fHandle,
						    ps->dlprecord, 0, 0,
						    0, &cat);

			if (rec < 0)
				break;

			if (cat != incategory) {
				ps->dlprecord++;
				continue;
			}

			rec = dlp_ReadRecordByIndex(sd, fHandle,
						    ps->dlprecord, buffer,
						    id, attr, &cat);

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
			if (id) *id =
				 get_long(DLP_RESPONSE_DATA(res, 0, 0));
			if (index) *index =
				 get_short(DLP_RESPONSE_DATA(res, 0, 4));
			if (attr) *attr =
				 get_byte(DLP_RESPONSE_DATA(res, 0, 8));
			if (buffer) {
				pi_buffer_clear (buffer);
				pi_buffer_append (buffer, DLP_RESPONSE_DATA(res, 0, 10),
					(size_t)data_len);
			}

			flags = get_byte(DLP_RESPONSE_DATA(res, 0, 8));
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			    "DLP ReadNextRecInCategory ID: 0x%8.8lX, "
				 "Index: %d, Category: %d\n"
			    "  Flags: %s%s%s%s%s%s (0x%2.2X) and %d bytes:\n",
			    (unsigned long) get_long(DLP_RESPONSE_DATA(res,
					0, 0)),
			    get_short(DLP_RESPONSE_DATA(res, 0, 4)),
			    (int) get_byte(DLP_RESPONSE_DATA(res, 0, 9)),
			    (flags & dlpRecAttrDeleted) ? " Deleted" : "",
			    (flags & dlpRecAttrDirty) ? " Dirty" : "",
			    (flags & dlpRecAttrBusy) ? " Busy" : "",
			    (flags & dlpRecAttrSecret) ? " Secret" : "",
			    (flags & dlpRecAttrArchived) ? " Archive" : "",
			    (!flags) ? " None" : "",
			    flags, data_len));
			CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, 
			      dumpdata(DLP_RESPONSE_DATA(res, 0, 10),
				(size_t)data_len));
		} else {
			data_len = result;
		}

		dlp_response_free(res);
		
		return data_len;
	}
}


/***********************************************************************
 *
 * Function:    dlp_ReadAppPreference
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
		      int maxsize, void *buffer, size_t *size, int *version)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	if (pi_version(sd) < 0x0101) {
		/* Emulate on PalmOS 1.0 */
		int 	db,
			rec;
		pi_buffer_t *buf;

		Trace(ReadAppPreferenceV1);

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadAppPreference Emulating with: Creator: '%s', "
			"Id: %d, Size: %d, Backup: %d\n",
		    printlong(creator), id,
		    buffer ? maxsize : 0, backup ? 0x80 : 0));

		rec = dlp_OpenDB(sd, 0, dlpOpenRead, "System Preferences",
			       &db);
		if (rec < 0)
			return rec;

		buf = pi_buffer_new (1024);
		
		rec = dlp_ReadResourceByType(sd, db, creator, id, buf,NULL);
		
		if (rec < 0) {
			pi_buffer_free (buf);
			dlp_CloseDB(sd, db);
			return rec;
		}

		if (size)
			*size = buf->used - 2;

		if (version)
			*version = get_short(buf->data);

		if (rec > 2) {
			rec -= 2;
			memmove(buffer, buf->data + 2, (size_t)rec);
		} else {
			rec = 0;
		}

		pi_buffer_free (buf);
		
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
			if (version) *version =
				 get_short(DLP_RESPONSE_DATA(res, 0, 0));
			if (size && !buffer) *size =
			 get_short(DLP_RESPONSE_DATA(res, 0, 2)); /* Total sz */
			if (size && buffer)
				*size = data_len;	/* Size returned */
			if (buffer)
				memcpy(buffer, DLP_RESPONSE_DATA(res, 0, 6),
					(size_t)data_len);

			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			    "DLP ReadAppPref Version: %d, "
				"Total size: %d, Read %d bytes:\n",
			    get_short(DLP_RESPONSE_DATA(res, 0, 0)), 
			    get_short(DLP_RESPONSE_DATA(res, 0, 2)),
			    get_short(DLP_RESPONSE_DATA(res, 0, 4))));
			CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, 
			      dumpdata(DLP_RESPONSE_DATA(res, 0, 6),
				(size_t)data_len));
		} else {
			data_len = result;
		}
		
		dlp_response_free(res);

		return data_len;
	}
}


/***********************************************************************
 *
 * Function:    dlp_WriteAppPreference
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
		       int version, void *buffer, size_t size)
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
 *
 * Summary:     Iterate through modified records in category, returning 
 *		subsequent modifieded records on each call
 *
 * Parameters:  sd         --> Socket descriptor as returned by
 *                             pilot_connect().
 *              fHandle    --> Database handle as returned by dlp_OpenDB().
 *              incategory --> Category to fetch records from.
 *              buffer     <-- Data from specified record. emptied prior to reading data
 *              id         <-- Record ID of record on palm device.
 *              index      <-- Specifies record to get.
 *              size       <-- Size of data returned in buffer.
 *              attr       <-- Attributes from record on palm device.
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 * Parameter documentation by DHS.
 *
 ***********************************************************************/
int
dlp_ReadNextModifiedRecInCategory(int sd, int fHandle, int incategory,
				  pi_buffer_t *buffer, recordid_t * id,
				  int *index, int *attr)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	if (pi_version(sd) < 0x0101) {
		/* Emulate for PalmOS 1.0 */
		int 	cat,
			rec;

		Trace(ReadNextModifiedRecInCategoryV1);

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadNextModifiedRecInCategory"
			" Emulating with: Handle: %d, Category: %d\n",
		    fHandle, incategory));

		do {
			/* Fetch next modified record (in any category) */
			rec = dlp_ReadNextModifiedRec(sd, fHandle, buffer,
						    id, index, attr,
						    &cat);

			/* If none found, reset modified pointer so that
			 another search on a different (or the same!) category
			 will start from the beginning */

			/* Working on same assumption as ReadNextRecInCat,
				elide this:
			   if (r < 0)
			   dlp_ResetDBIndex(sd, fHandle);
			 */

			/* Loop until we fail to get a record or a record
			 is found in the proper category */
		}
		while ((rec >= 0) && (cat != incategory));

		return rec;
	} else {
		int data_len;
		
		Trace(ReadNextModifiedRecInCategoryV2);

		req = dlp_request_new(dlpFuncReadNextModifiedRecInCategory,
			1, 2);

		set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
		set_byte(DLP_REQUEST_DATA(req, 0, 1), incategory);

		result = dlp_exec(sd, req, &res);
		
		dlp_request_free(req);
		
		if (result >= 0) {
			data_len = res->argv[0]->len - 10;
			
			if (id) *id =
				 get_long(DLP_RESPONSE_DATA(res, 0, 0));
			if (index) *index =
				 get_short(DLP_RESPONSE_DATA(res, 0, 4));
			if (attr) *attr =
				 get_byte(DLP_RESPONSE_DATA(res, 0, 8));

			if (buffer) {
				pi_buffer_clear (buffer);
				pi_buffer_append (buffer, DLP_RESPONSE_DATA(res, 0, 10),
					(size_t)data_len);
			}

			CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG,
				 record_dump(DLP_RESPONSE_DATA(res, 0, 0)));
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
 *
 * Summary:     Iterate through modified records in category, returning
 *		subsequent modified records on each call
 *
 * Parameters:  buffer is emptied prior to reading data
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int
dlp_ReadNextModifiedRec(int sd, int fHandle, pi_buffer_t *buffer, recordid_t * id,
			int *index, int *attr, int *category)
{
	int 	result,
		data_len;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(ReadNextModifiedRec);
	
	req = dlp_request_new (dlpFuncReadNextModifiedRec, 1, 1);
	
	set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
	
	result = dlp_exec (sd, req, &res);

	dlp_request_free(req);
	
	if (result >= 0) {
		data_len = res->argv[0]->len -10;
		if (id) *id =
			 get_long(DLP_RESPONSE_DATA(res, 0, 0));
		if (index) *index =
			 get_short(DLP_RESPONSE_DATA(res, 0, 4));
		if (attr) *attr =
			 get_byte(DLP_RESPONSE_DATA(res, 0, 8));
		if (category) *category =
			 get_byte(DLP_RESPONSE_DATA(res, 0, 9));

		if (buffer) {
			pi_buffer_clear (buffer);
			pi_buffer_append (buffer, DLP_RESPONSE_DATA(res, 0, 10),
				(size_t)data_len);
		}

		CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG,
			 record_dump(DLP_RESPONSE_DATA(res, 0, 0)));
	} else {
		data_len = result;
	}
	
	dlp_response_free(res);
	
	return data_len;
}


/***********************************************************************
 *
 * Function:    dlp_ReadRecordById
 *
 * Summary:     Searches device database for match on a record by id
 *
 * Parameters:  buffer is emptied prior to reading data
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int
dlp_ReadRecordById(int sd, int fHandle, recordid_t id, pi_buffer_t *buffer,
		   int *index, int *attr, int *category)
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
	set_short(DLP_REQUEST_DATA(req, 0, 8), buffer ? buffer->allocated : 0);	/* length to return */

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result >= 0) {
		data_len = res->argv[0]->len - 10;
		if (index)
			*index = get_short(DLP_RESPONSE_DATA(res, 0, 4));
		if (attr)
			*attr = get_byte(DLP_RESPONSE_DATA(res, 0, 8));
		if (category)
			*category = get_byte(DLP_RESPONSE_DATA(res, 0, 9));
		if (buffer) {
			pi_buffer_clear (buffer);
			pi_buffer_append (buffer, DLP_RESPONSE_DATA(res, 0, 10),
				(size_t)data_len);
		}

		CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG,
			 record_dump(DLP_RESPONSE_DATA(res, 0, 0)));
	} else {
		data_len = result;
	}
	
	dlp_response_free(res);
	
	return data_len;
}


/***********************************************************************
 *
 * Function:    dlp_ReadRecordByIndex
 *
 * Summary:     Searches device database for match on a record by index
 *
 * Parameters:  sd       --> Socket descriptor as returned by pilot_connect().
 *              fHandle  --> Database handle as returned by dlp_OpenDB().
 *              index    --> Specifies record to get.
 *              buffer   <-- Data from specified record. emptied prior to reading data
 *              id       <-- Record ID of record on palm device.
 *              size     <-- Size of data returned in buffer.
 *              attr     <-- Attributes from record on palm device.
 *              category <-- Category from record on palm device.
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 * Turns this request for a particular record in the database into a
 * low-level dlp request.
 *
 * Parameter documentation by DHS.
 *
 ***********************************************************************/
int
dlp_ReadRecordByIndex(int sd, int fHandle, int index, pi_buffer_t *buffer,
	recordid_t * id, int *attr, int *category)
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
	set_short(DLP_REQUEST_DATA(req, 0, 4), 0); /* Offset into record */
	set_short(DLP_REQUEST_DATA(req, 0, 6), buffer ? buffer->allocated : 0);	/* length to return */

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result >= 0) {
		data_len = res->argv[0]->len - 10;
		if (id)
			*id = get_long(DLP_RESPONSE_DATA(res, 0, 0));
		if (attr)
			*attr = get_byte(DLP_RESPONSE_DATA(res, 0, 8));
		if (category)
			*category = get_byte(DLP_RESPONSE_DATA(res, 0, 9));
		if (buffer) {
			pi_buffer_clear (buffer);
			pi_buffer_append (buffer, DLP_RESPONSE_DATA(res, 0, 10),
				(size_t)data_len);
		}

		CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG,
			 record_dump(DLP_RESPONSE_DATA(res, 0, 0)));
	} else {
		data_len = result;
	}

	dlp_response_free(res);
	
	return data_len;
}

#ifdef PI_DEBUG
static void record_dump (char *data)
{
	size_t size, flags;
	
	size = get_short(&data[6]);
	flags = get_byte(&data[8]);

	LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
	    "  ID: 0x%8.8lX, Index: %d, Category: %d\n"
	    "  Flags: %s%s%s%s%s%s (0x%2.2X), and %d bytes:\n",
	    (unsigned long) get_long(&data[0]),
	    get_short(&data[4]), get_byte(&data[9]),
	    (flags & dlpRecAttrDeleted) ? " Deleted" : "",
	    (flags & dlpRecAttrDirty) ? " Dirty" : "",
	    (flags & dlpRecAttrBusy) ? " Busy" : "",
	    (flags & dlpRecAttrSecret) ? " Secret" : "",
	    (flags & dlpRecAttrArchived) ? " Archive" : "",
	    (!flags) ? " None" : "",
	    flags, size));
	dumpdata((char *)&data[10], size);
}
#endif

/***********************************************************************
 *
 * Function:    dlp_ExpSlotEnumerate
 *
 * Summary:     Get the number of expansion slots on the device
 * 				Available only in version 1.3 and later of the protocol
 * 				(Palm OS 4.0 and later)
 *
 * Parameters:  sd			--> Socket descriptor as returned by pilot_connect().
 * 				numSlots	<-> on input, the maximum number of slot refs that
 * 								can be stored in the slotRefs array.
 * 								On output, the number of slots on the device
 * 				slotRefs	<-- an array of int with the ref of each slot
 * 
 * Returns:     -1		dlp error (see errno)
 *				0		no error
 *				>0		Palm OS error code
 *
 ***********************************************************************/
int
dlp_ExpSlotEnumerate(int sd, int *numSlots, int *slotRefs)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_ExpSlotEnumerate);
	
	req = dlp_request_new(dlpFuncExpSlotEnumerate, 0);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result >= 0) {
		int slots, i;

		slots = get_short(DLP_RESPONSE_DATA (res, 0, 0));

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			 "DLP ExpSlotEnumerate %d\n", slots));

		if (slots) {
			for (i = 0; i < slots && i < *numSlots; i++) {
				slotRefs[i] =
			  	get_short(DLP_RESPONSE_DATA (res, 0,
					 2 + (2 * i)));

				LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
				 "  %d Slot-Refnum %d\n", i, slotRefs[i]));
			}
		}

		*numSlots = slots;
	}
	 
	dlp_response_free(res);

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)
		result = 0;				/* no error */

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_ExpCardPresent
 *
 * Summary:     Check whether there is an expansion card in a slot
 * 				Available only in version 1.3 and later of the protocol
 * 				(Palm OS 4.0 and later)
 *
 * Parameters:  sd		--> socket descriptor as returned by pilot_connect().
 * 				slotRef	--> slot ref as returned by dlp_ExpCardInfo()
 * 
 * Returns:     -1		dlp error (see errno)
 *				0		no error, card is present
 *				>0		Palm OS error code
 *
 ***********************************************************************/
int
dlp_ExpCardPresent(int sd, int slotRef)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	RequireDLPVersion(sd,1,2);
	Trace(dlp_ExpCardPresent);

	req = dlp_request_new (dlpFuncExpCardPresent, 1, 2);

	set_short(DLP_REQUEST_DATA(req, 0, 0), slotRef);

	result = dlp_exec (sd, req, &res);

	dlp_request_free(req);
	dlp_response_free(res);

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)
		result = 0;				/* no error */

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_ExpCardInfo
 *
 * Summary:     Return information about one connected expansion card
 *
 * Parameters:  sd			--> socket descriptor 
 * 				slotRef		--> slot ref as returned by dlp_ExpCardInfo()
 *				flags		<-- card capabilities (see ExpansionMgr.h in
 *								Palm OS headers)
 *				numStrings  <-- number of packed information strings
 *				strings		<-- if strings was not NULL and numStrings>0,
 *								the function returns a new buffer with
 *								the packed strings. It is the caller's
 *								responsibility to free() the buffer.
 * 
 * Returns:     -1		dlp error (see errno)
 *				0		no error
 *				>0		Palm OS error code
 *				strings buffer can be NULL in case of malloc error
 *
 ***********************************************************************/
int
dlp_ExpCardInfo(int sd, int SlotRef, unsigned long *flags, int *numStrings,
				char **strings)
{
	int result;
	struct dlpRequest* req;
	struct dlpResponse* res;

	RequireDLPVersion(sd,1,2);
	Trace(dlp_ExpCardInfo);

	req = dlp_request_new (dlpFuncExpCardInfo, 1, 2);

	set_short(DLP_REQUEST_DATA(req, 0, 0), SlotRef);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result >= 0) {
		*flags = get_long(DLP_RESPONSE_DATA (res, 0, 0));
		*numStrings = get_byte(DLP_RESPONSE_DATA (res, 0, 4));

		if (strings && *numStrings) {
			int i, len, sz = 0;
			char *p = DLP_RESPONSE_DATA (res, 0, 8);

			for (i=0; i < *numStrings; i++, sz+=len, p+=len)
				len = strlen (p) + 1;

			*strings = (char *) malloc (sz);
			if (*strings)
				memcpy (*strings, DLP_RESPONSE_DATA (res, 0, 8), sz);
		}

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			 "DLP ExpCardInfo flags: 0x%08lx numStrings: %d\n",
			 *flags, *numStrings));
	}

	dlp_response_free(res);

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)
		result = 0;				/* no error */

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_VFSGetDefaultDir
 *
 * Summary:		Return the default location of the given volume for files
 *				of a particular type
 *
 * Parameters:  sd			--> socket descriptor
 *				volRefNum   --> volume reference number returned from
 *								dlp_VFSVolumeEnumerate()
 *				type		--> file type. can be either a MIME type
 *								(ie "image/jpeg") or a file extension
 *								(ie ".jpg")
 *				dir			<-> a buffer to hold the returned path
 *				len			<-> on input, the maximum size of the dir
 *								buffer. on output, the effective size
 *								(counting the nul terminator)
 * 
 * Returns:     -2		dir buffer size < required size
 *				-1		dlp error (see errno)
 *				0		no error
 *				>0		Palm OS error code
 *
 ***********************************************************************/
int
dlp_VFSGetDefaultDir(int sd, int volRefNum, const char *type, char *dir,
	int *len)
{
	int result, buflen;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSGetDefaultDir);
	
	req = dlp_request_new(dlpFuncVFSGetDefaultDir,
		1, 2 + (strlen(type) + 1));

	set_short(DLP_REQUEST_DATA(req, 0, 0), volRefNum);
	strcpy(DLP_REQUEST_DATA(req, 0, 2), type);
	
	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result >= 0) {
		buflen = get_short(DLP_RESPONSE_DATA (res, 0, 0));
		
		if (*len < buflen + 1)
			return -2;
		
		if (buflen)
			strncpy(dir, DLP_RESPONSE_DATA (res, 0, 2), 
				(size_t)buflen);
		else
			dir[0] = '\0';
		
		*len = buflen;
		
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		     "Default dir is %s\n", dir));
	}
	
	dlp_response_free(res);
	
	if (result < -2)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)
		result = 0;				/* no error */

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_VFSImportDatabaseFromFile
 *
 * Summary:     
 *
 * Parameters:  FIXME
 * 
 * Returns:     
 *
 ***********************************************************************/
int
dlp_VFSImportDatabaseFromFile(int sd, int volRefNum, const char *path,
	int *cardno, unsigned long *localid)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSImportDatabaseFromFile);

	LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		"Import file <%s>%d\n", path));

	req = dlp_request_new(dlpFuncVFSImportDatabaseFromFile,
		1, 2 + (strlen(path) + 1));

	set_short(DLP_REQUEST_DATA(req, 0, 0), volRefNum);
	strcpy(DLP_REQUEST_DATA(req, 0, 2), path);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);

	if (result >= 0) {
		if (cardno)
			*cardno = get_short(DLP_RESPONSE_DATA (res, 0, 0));
		if (localid)
			*localid = get_short(DLP_RESPONSE_DATA (res, 0, 2)); 

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		     "Database imported as: cardNo:%d dbID:%d\n", 
		     get_short(DLP_RESPONSE_DATA (res, 0, 0)), 
		     get_short(DLP_RESPONSE_DATA (res, 0, 2))));
	}
	
	dlp_response_free(res);

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)		/* no error */
		result = 0;

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_VFSExportDatabaseToFile
 *
 * Summary:     
 *
 * Parameters:  FIXME
 * 
 * Returns:     
 *
 ***********************************************************************/
int
dlp_VFSExportDatabaseToFile(int sd, int volRefNum, const char *path, 
	int cardno, unsigned int localid)  
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSExportDatabaseToFile);
	
	req = dlp_request_new(dlpFuncVFSExportDatabaseToFile,
		1, 8 + (strlen(path) + 1));
	
	set_short(DLP_REQUEST_DATA(req, 0, 0), volRefNum);
	set_short(DLP_REQUEST_DATA(req, 0, 2), cardno);
	set_long(DLP_REQUEST_DATA(req, 0, 4), localid);
	strcpy(DLP_REQUEST_DATA(req, 0, 8), path);
	
	result = dlp_exec(sd, req, &res);
	
	dlp_request_free(req);
	dlp_response_free(res);

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)		/* no error */
		result = 0;

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_VFSFileCreate
 *
 * Summary:     Create a new file
 *
 * Parameters:  sd			--> socket descriptor
 *				volRefNum   --> volume reference number returned from
 *								dlp_VFSVolumeEnumerate()
 *				path		--> full path of the file to create
 * 
 * Returns:     -1		dlp error (see errno)
 *				0		no error
 *				>0		Palm OS error code
 *
 ***********************************************************************/
int
dlp_VFSFileCreate(int sd, int volRefNum, const char *name)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileCreate);

	req = dlp_request_new (dlpFuncVFSFileCreate, 1, 2 + (strlen(name) + 1));

	set_short (DLP_REQUEST_DATA (req, 0, 0), volRefNum);
	strcpy (DLP_REQUEST_DATA (req, 0, 2), name);

	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);
	dlp_response_free (res);

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)		/* no error */
		result = 0;

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_VFSFileOpen
 *
 * Summary:     Open a file or directory and return a FileRef to it
 *
 * Parameters:  sd			--> socket descriptor
 *				volRefNum   --> volume reference number returned from
 *								dlp_VFSVolumeEnumerate()
 *				path		--> access path to the file or directory
 *				openMode	--> open mode, use constants from Palm SDK
 *				fileRef		<-- returned file reference
 * 
 * Returns:     -1		dlp error (see errno)
 *				0		no error
 *				>0		Palm OS error code
 *
 ***********************************************************************/
int
dlp_VFSFileOpen(int sd, int volRefNum, const char *path, int openMode, 
	    FileRef *fileRef)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileOpen);

	LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		"Open File %s Mode: %x VFSRef 0x%x\n",
		path, openMode,volRefNum));
	
	req = dlp_request_new (dlpFuncVFSFileOpen, 1, 4 + (strlen (path) + 1));

	set_short (DLP_REQUEST_DATA (req, 0, 0), volRefNum);
	set_short (DLP_REQUEST_DATA (req, 0, 2), openMode);
	strcpy (DLP_REQUEST_DATA (req, 0, 4), path);

	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);
	
	if (result >= 0) {
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		     "OpenFileRef: 0x%x\n",
			get_long (DLP_RESPONSE_DATA (res, 0, 0))));

		*fileRef = get_long(DLP_RESPONSE_DATA (res, 0, 0));
	}
	
	dlp_response_free(res);

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)		/* no error */
		result = 0;

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_VFSFileClose
 *
 * Summary:     Close an open file or directory
 *
 * Parameters:  sd			--> socket descriptor
 *				fileRef		--> file or directory reference obtained
 *								from dlp_VFSFileOpen()
 * 
 * Returns:     -1		dlp error (see errno)
 *				0		no error
 *				>0		Palm OS error code
 *
 ***********************************************************************/
int
dlp_VFSFileClose(int sd, FileRef fileRef)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileClose);

	req = dlp_request_new (dlpFuncVFSFileClose, 1, 4);

	set_long (DLP_REQUEST_DATA (req, 0, 0), fileRef);

	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);
	dlp_response_free (res);
	
	LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		"Closed FileRef: %x\n", fileRef));

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)		/* no error */
		result = 0;

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_VFSFileWrite
 *
 * Summary:     Write data to an open file
 *
 * Parameters:  sd			--> socket descriptor
 *				fileRef		--> file reference obtained from dlp_VFSFileOpen()
 *				data		--> data buffer to write
 *				len			--> number of bytes to write
 *
 * Returns:     -1		dlp error (see errno)
 *				0		no error
 *				>0		Palm OS error code
 *
 ***********************************************************************/
int
dlp_VFSFileWrite(int sd, FileRef fileRef, unsigned char *data, size_t len)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res = NULL;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileWrite);

	LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		"Write to FileRef: %x bytes %d\n", fileRef, len));
	
	req = dlp_request_new (dlpFuncVFSFileWrite, 1, 8);

	set_long (DLP_REQUEST_DATA (req, 0, 0), fileRef);
	set_long (DLP_REQUEST_DATA (req, 0, 4), len);

	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);

	if (result >= 0) {
		result = pi_write (sd, data, len);
		if (result < (int)len) {
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			     "send failed %d\n", result));
		} else {
			dlp_response_free (res);
			res = NULL;

			result = dlp_response_read (&res, sd);
			
			if (result >= 0) {
				result = get_short(DLP_RESPONSE_DATA (res, 0, 2));
				LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
					"send success (%d) res %d!\n", len, result));
			}
		} 
	}

	dlp_response_free (res);

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)		/* no error */
		result = 0;

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_VFSFileRead
 *
 * Summary:     Read data from an open file
 *
 * Parameters:  sd			--> socket descriptor
 *				fileRef		--> file reference obtained from dlp_VFSFileOpen()
 *				data		<-> buffer to hold the data, emptied first
 *				len			--> on input, number of bytes to read
 * 
 * Returns:     -1		dlp error (see errno)
 *				0		no error
 *				>0		Palm OS error code
 *
 ***********************************************************************/
int
dlp_VFSFileRead(int sd, FileRef fileRef, pi_buffer_t *data, size_t len)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileRead);

	req = dlp_request_new (dlpFuncVFSFileRead, 1, 8);

	set_long (DLP_REQUEST_DATA (req, 0, 0), fileRef);
	set_long (DLP_REQUEST_DATA (req, 0, 4), len);

	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);

	pi_buffer_clear (data);

	if (result >= 0) {
		do {
			result = pi_read(sd, data, len);
			if (result <= 0)
				break;
			len -= result;
		} while (len > 0);

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		     "Readbytes: %d\n", len));
	}

	dlp_response_free(res);

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)		/* no error */
		result = 0;

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_VFSFileDelete
 *
 * Summary:     Delete a closed file or directory
 *
 * Parameters:  sd			--> socket descriptor
 *				volRefNum   --> as returned by dlp_VFSVolumeEnumerate()
 *				path		--> complete file access path
 * 
 * Returns:     -1		dlp error (see errno)
 *				0		no error
 *				>0		Palm OS error code
 * 
 ***********************************************************************/
int
dlp_VFSFileDelete(int sd, int volRefNum, const char *path)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileDelete);
	
	req = dlp_request_new (dlpFuncVFSFileDelete, 1, 2 + (strlen (path) + 1));
	
	set_short (DLP_REQUEST_DATA (req, 0, 0), volRefNum);
	strcpy (DLP_REQUEST_DATA (req, 0, 2), path);
	
	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);
	dlp_response_free (res);

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)		/* no error */
		result = 0;

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_VFSFileRename
 *
 * Summary:     Rename an existing file or directory
 *
 * Parameters:  sd			--> socket descriptor
 *				volRefNum   --> as returned by dlp_VFSVolumeEnumerate()
 *				path		--> complete file access path
 *				newname		--> new file name (without the access path)
 * 
 * Returns:     -1		dlp error (see errno)
 *				0		no error
 *				>0		Palm OS error code
 *
 ***********************************************************************/
int
dlp_VFSFileRename(int sd, int volRefNum, const char *path, 
		      const char *newname)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileRename);
	
	LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		"Rename file %s to %s\n", path, newname));
	
	req = dlp_request_new (dlpFuncVFSFileRename,
		1, 4 + (strlen (path) + 1) + (strlen (newname) + 1));
	
	set_short (DLP_REQUEST_DATA (req, 0, 0), volRefNum);
	set_short (DLP_REQUEST_DATA (req, 0, 2), 2);
	strcpy (DLP_REQUEST_DATA (req, 0, 4), path);
	strcpy (DLP_REQUEST_DATA (req, 0, 4 + (strlen(path) + 1)), newname);
	
	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);
	dlp_response_free (res);

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)		/* no error */
		result = 0;

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_VFSFileEOF
 *
 * Summary:     Checks whether the current position is at the end of file
 *
 * Parameters:  sd			--> socket descriptor
 *				fileRef		--> file reference obtained from dlp_VFSFileOpen()
 * 
 * Returns:     -1		dlp error (see errno)
 *				0		no error
 *				>0		Palm OS error code
 *
 ***********************************************************************/
int
dlp_VFSFileEOF(int sd, FileRef fileRef)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileEOF);

	req = dlp_request_new (dlpFuncVFSFileEOF, 1, 4);
	
	set_long (DLP_REQUEST_DATA (req, 0, 0), fileRef);
	
	result = dlp_exec (sd, req, &res);
	
	dlp_request_free (req);
	dlp_response_free (res);

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)		/* no error */
		result = 0;

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_VFSFileTell
 *
 * Summary:     Return the current seek position in an open file
 *
 * Parameters:  sd			--> socket descriptor
 *				fileRef		--> file reference obtained from dlp_VFSFileOpen()
 *				position	<-- current position
 * 
 * Returns:     -1		dlp error (see errno)
 *				0		no error
 *				>0		Palm OS error code
 *
 ***********************************************************************/
int
dlp_VFSFileTell(int sd, FileRef fileRef,int *position)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileTell);
	
	req = dlp_request_new(dlpFuncVFSFileTell, 1, 4);

	set_long (DLP_REQUEST_DATA (req, 0, 0), fileRef);

	result = dlp_exec (sd, req, &res);
	
	dlp_request_free (req);

	if (result >= 0) {
		*position = get_long (DLP_RESPONSE_DATA (res, 0, 0));
	}
	
	dlp_response_free (res);

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)		/* no error */
		result = 0;

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_VFSFileGetAttributes
 *
 * Summary:     Return the attributes of an open file
 *
 * Parameters:  sd			--> socket descriptor
 *				fileRef		--> file reference obtained from dlp_VFSFileOpen()
 *				attributes	<-- file attributes (see Palm SDK's VFSMgr.h)
 * 
 * Returns:     -1		dlp error (see errno)
 *				0		no error
 *				>0		Palm OS error code
 *
 ***********************************************************************/
int
dlp_VFSFileGetAttributes (int sd, FileRef fileRef, unsigned long *attributes)
{
	int	result;
	struct dlpRequest *req; 
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileGetAttributes);
	
	req = dlp_request_new (dlpFuncVFSFileGetAttributes, 1, 4);
	
	set_long (DLP_REQUEST_DATA (req, 0, 0), fileRef);
	
	result = dlp_exec (sd, req, &res);
	
	dlp_request_free (req);

	if (result >= 0) {
		*attributes = get_long (DLP_RESPONSE_DATA (res, 0, 0));
	}

	dlp_response_free(res);	

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)		/* no error */
		result = 0;

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_VFSFileSetAttributes
 *
 * Summary:     Change the attributes of an open file
 *
 * Parameters:  sd			--> socket descriptor
 *				fileRef		--> file reference obtained from dlp_VFSFileOpen()
 *				attributes	--> new file attributes (see Palm SDK's VFSMgr.h)
 * 
 * Returns:     -1		dlp error (see errno)
 *				0		no error
 *				>0		Palm OS error code
 *
 ***********************************************************************/
int
dlp_VFSFileSetAttributes(int sd, FileRef fileRef, unsigned long attributes)
{
	int	result;
	struct dlpRequest *req; 
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileSetAttributes);
	
	req = dlp_request_new (dlpFuncVFSFileSetAttributes, 1, 8);
	
	set_long (DLP_REQUEST_DATA (req, 0, 0), fileRef);
	set_long (DLP_REQUEST_DATA (req, 0, 4), attributes);
	
	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);
	dlp_response_free (res);

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)		/* no error */
		result = 0;

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_VFSFileGetDate
 *
 * Summary:     Return one of the dates associated with an open file or
 *				directory (creation, modification or last access)
 *
 * Parameters:  sd			--> socket descriptor
 *				fileRef		--> file reference obtained from dlp_VFSFileOpen()
 *				whichDate   --> which date you want (vfsFileDateCreated,
 *								vfsFileDateModified or vfsFileDateAccessed)
 *				date		<-- the date
 * 
 * Returns:     -1		dlp error (see errno)
 *				0		no error
 *				>0		Palm OS error code
 *
 ***********************************************************************/
int
dlp_VFSFileGetDate(int sd, FileRef fileRef, int which, time_t *date)
{
	int	result;
	struct dlpRequest *req; 
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileGetDate);
	
	req = dlp_request_new (dlpFuncVFSFileGetDate, 1, 6);

	set_long (DLP_REQUEST_DATA (req, 0, 0), fileRef);
	set_short (DLP_REQUEST_DATA (req, 0, 4), which);

	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);
	
	if (result >= 0) {
		*date = get_long (DLP_RESPONSE_DATA (res, 0, 0)) - 2082852000;
	
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		     "Requested date(%d): %d / %x calc %d / %x\n",which,
		     get_long(DLP_RESPONSE_DATA (res, 0, 0)),
		     get_long(DLP_RESPONSE_DATA (res, 0, 0)),
		     *date,*date));
	}
	
	dlp_response_free (res);

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)		/* no error */
		result = 0;

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_VFSFileSetDate
 *
 * Summary:     Change one of the dates for an open file or directory
 *				(created, modified or last access)
 *
 * Parameters:  sd			--> socket descriptor
 *				fileRef		--> file reference obtained from dlp_VFSFileOpen()
 *				whichDate   --> which date you want (vfsFileDateCreated,
 *								vfsFileDateModified or vfsFileDateAccessed)
 *				date		--> the date
 * 
 * Returns:     -1		dlp error (see errno)
 *				0		no error
 *				>0		Palm OS error code
 *
 ***********************************************************************/
int
dlp_VFSFileSetDate(int sd, FileRef fileRef, int which, time_t date)
{
	int	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileSetDate);

	LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		"Set date(%d): %d / %x calc %d / %x\n", which,
		date, date,
		date + 2082852000,
		date + 2082852000));
	
	req = dlp_request_new(dlpFuncVFSFileSetDate, 1, 10);

	set_long (DLP_REQUEST_DATA(req, 0, 0), fileRef);
	set_short (DLP_REQUEST_DATA(req, 0, 4), which);
	set_long (DLP_REQUEST_DATA(req, 0, 6), date + 2082852000);

	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);
	dlp_response_free (res);	

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)		/* no error */
		result = 0;

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_VFSDirCreate
 *
 * Summary:     Create a new directory on a VFS volume
 *
 * Parameters:  sd			--> socket descriptor 
 *				volRefNum   --> as returned by dlp_VFSVolumeEnumerate()
 *				path		--> full path of directory to create
 *
 * Returns:     -1		dlp error (see errno)
 *				0		no error
 *				>0		Palm OS error code
 *
 ***********************************************************************/
int
dlp_VFSDirCreate(int sd, int volRefNum, const char *path)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSDirCreate);

	req = dlp_request_new (dlpFuncVFSDirCreate, 1, 2 + (strlen(path) + 1));

	set_short (DLP_REQUEST_DATA (req, 0, 0), volRefNum);
	strcpy (DLP_REQUEST_DATA (req, 0, 2), path);
	
	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);
	dlp_response_free (res);

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)		/* no error */
		result = 0;

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_VFSDirEntryEnumerate
 *
 * Summary:     Iterate through the entries in a directory
 *
 * Parameters:  sd			--> socket descriptor 
 *				dirRefNum   --> FileRef obtained from dlp_VFSFileOpen()
 *				dirIterator <-> iterator we use to navigate in the dirs.
 *								start with vfsIteratorStart
 *				maxDirItems	<-> on input, the max. number of VFSDirInfo
 *								structs that can be filled at once.
 *								on output, the actual number of items
 *				data		<-- `maxDirItems' directory items
 * 
 * Returns:     -1		dlp error (see errno)
 *				0		no error
 *				>0		Palm OS error code
 *
 ***********************************************************************/
int
dlp_VFSDirEntryEnumerate(int sd, FileRef dirRefNum, 
	unsigned long *dirIterator, int *maxDirItems, struct VFSDirInfo *data)
{
	int result,
		entries,
		from,
		at,
		slen,
		count;
	struct dlpRequest *req;
	struct dlpResponse *res;

	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSDirEntryEnumerate);

	req = dlp_request_new (dlpFuncVFSDirEntryEnumerate, 1, 12);
	
	set_long (DLP_REQUEST_DATA (req, 0, 0), dirRefNum);
	set_long (DLP_REQUEST_DATA (req, 0, 4), *dirIterator);
	/*  FP: (DLP_BUF_SIZE - 99). this is the max return buffer size that
		we are passing for the device to send its response, but I'm not
		sure whether this is a magic value that shouldn't be changed.
		If DLP_BUF_SIZE changes, the value below may become too large! */
	set_long (DLP_REQUEST_DATA (req, 0, 8), 0xFF9C);

	result = dlp_exec (sd, req, &res);
	
	dlp_request_free (req);
	
	if (result >= 0) {
		*dirIterator = get_long (DLP_RESPONSE_DATA (res, 0, 0));
		if (result) {
			entries = get_long (DLP_RESPONSE_DATA (res, 0, 4));
		} else {
			entries = 0;
		}
	
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		     "%d results returnd (ilterator: %d)\n", entries,
		     *dirIterator));
	
		from  = 8;
		count = 0;
	
		for (at = 0; at < entries; at++) {
			if ((*maxDirItems) > at) {
				data[at].attr = 
					get_long(DLP_RESPONSE_DATA (res,
					 0, 0) + from);
				strncpy (data[at].name,
					 DLP_RESPONSE_DATA(res, 0, from + 4),
					vfsMAXFILENAME);
				data[at].name[vfsMAXFILENAME-1] = 0;
				count++;
			}
	
			/* Zero terminated string. Strings that have an
			 even length will be null terminated and have a
			 pad byte. */
			slen = strlen (DLP_RESPONSE_DATA(res, 0, from + 4)) + 1;
			if (slen & 1)
				slen++;	/* make even stringlen + NULL */
	
			/* 6 = 4 (attr) + 1 (NULL)  -+ 1 (PADDING) */
			from += slen + 4;
		}
		*maxDirItems = count;
	}
	
	dlp_response_free (res);

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)		/* no error */
		result = 0;

	return result;  
}


/***********************************************************************
 *
 * Function:    dlp_VFSVolumeFormat
 *
 * Summary:     
 *
 * Parameters:  FIXME
 * 
 * Returns:     
 *
 ***********************************************************************/
int
dlp_VFSVolumeFormat(int sd, unsigned char flags,
	int fsLibRef, struct VFSSlotMountParamTag *param)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(VFSVolumeFormat);

	LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		"VFSVolumeFormat Ver-check %x != 0101 \n",
			pi_version(sd)));
	
	req = dlp_request_new(dlpFuncVFSVolumeFormat, 1, 4);
/* FIXME check sizes, list the mount params properly */
	set_short(DLP_REQUEST_DATA(req, 0, 0), fsLibRef);
	set_short(DLP_REQUEST_DATA(req, 0, 2),
		 sizeof(struct VFSSlotMountParamTag));
	set_byte(DLP_REQUEST_DATA(req, 0, 4), flags);
	set_byte(DLP_REQUEST_DATA(req, 0, 4), 0); /* unused */

	set_short(DLP_REQUEST_DATA(req, 0, 6), param->vfsMountParam.volRefNum);
	set_short(DLP_REQUEST_DATA(req, 0, 8), param->vfsMountParam.reserved); 
	set_long(DLP_REQUEST_DATA(req, 0, 10), param->vfsMountParam.mountClass);
	set_short(DLP_REQUEST_DATA(req, 0, 14), param->slotLibRefNum);
	set_short(DLP_REQUEST_DATA(req, 0, 16), param->slotRefNum);   
	
	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	dlp_response_free(res);

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)		/* no error */
		result = 0;

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_VFSVolumeEnumerate
 *
 * Summary:     Returns a list of connected VFS volumes
 *
 * Parameters:  sd			--> socket descriptor 
 *				numVols		<-> on input, the maximum number of volume
 *								references that can be returned. On output,
 *								the actual number of volume references
 *				volRefs		<-- an array of volume references
 * 
 * Returns:     -1		dlp error (see errno)
 *				0		no error
 *				>0		Palm OS error code
 *
 ***********************************************************************/
int
dlp_VFSVolumeEnumerate(int sd, int *numVols, int *volRefs)
{
	int	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSVolumeEnumerate);

	req = dlp_request_new (dlpFuncVFSVolumeEnumerate, 0);
	
	result = dlp_exec (sd, req, &res);
	
	dlp_request_free (req);

	if (result >= 0) {
		int vols, i;
		
		vols = get_short (DLP_RESPONSE_DATA (res, 0, 0));

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			"DLP VFSVolumeEnumerate %d\n", vols));
		if (vols) {
			for (i = 0; i < vols && i < *numVols; i++) {
				volRefs[i] =
				  get_short (DLP_RESPONSE_DATA (res,
					 0, 2 + (2 * i)));

				LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
				 "  %d Volume-Refnum %d\n", i, volRefs[i]));
			}
		}

		*numVols = vols;
	}

	dlp_response_free (res);

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)		/* no error */
		result = 0;

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_VFSVolumeInfo
 *
 * Summary:     Returns information about one VFS volume
 *
 * Parameters:  sd			--> socket descriptor 
 *				volRefNum   --> as returned by dlp_VFSVolumeEnumerate()
 *				volInfo		<-- volume information
 * 
 * Returns:     -1		dlp error (see errno)
 *				0		no error
 *				>0		Palm OS error code
 *
 ***********************************************************************/
int
dlp_VFSVolumeInfo(int sd, int volRefNum, struct VFSInfo *volInfo)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSVolumeInfo);

	req = dlp_request_new (dlpFuncVFSVolumeInfo, 1, 2);

	set_short (DLP_REQUEST_DATA(req, 0, 0), volRefNum);

	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);
	
	if (result >= 0) {
		volInfo->attributes		= get_long (DLP_RESPONSE_DATA (res, 0, 0));
		volInfo->fsType			= get_long (DLP_RESPONSE_DATA (res, 0, 4));  
		volInfo->fsCreator		= get_long (DLP_RESPONSE_DATA (res, 0, 8));
		volInfo->mountClass		= get_long (DLP_RESPONSE_DATA (res, 0, 12));
		volInfo->slotLibRefNum  = get_short (DLP_RESPONSE_DATA (res, 0, 16));
		volInfo->slotRefNum		= get_short (DLP_RESPONSE_DATA (res, 0, 18));
		volInfo->mediaType		= get_long (DLP_RESPONSE_DATA (res, 0, 20));
		volInfo->reserved		= get_long (DLP_RESPONSE_DATA (res, 0, 24));      

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		     "VFSVolumeInfo: fstype '%s' ", printlong(volInfo->fsType)));
		
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		     "fscreator: '%s'\nSlotlibref %d Slotref %d\n", 
		     printlong(volInfo->fsCreator),
		     volInfo->slotLibRefNum,
		     volInfo->slotRefNum));
		
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		     "Media: '%s'\n", printlong(volInfo->mediaType)));
	}
	
	dlp_response_free(res);
	
	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)		/* no error */
		result = 0;

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_VFSVolumeGetLabel
 *
 * Summary:     Return the label (name) of a VFS volume
 *
 * Parameters:  sd			--> socket descriptor 
 *				volRefNum   --> as returned by dlp_VFSVolumeEnumerate()
 *				len			<-> on input, the maximum size of the name
 *								buffer. on output, the used size
 *				name		<-- the volume name
 * 
 * Returns:     -1		dlp error (see errno)
 *				0		no error
 *				>0		Palm OS error code
 *
 ***********************************************************************/
int
dlp_VFSVolumeGetLabel(int sd, int volRefNum, int *len, char *name)
{
	int 	result;
	struct dlpRequest *req; 
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSVolumeGetLabel);

	req = dlp_request_new (dlpFuncVFSVolumeGetLabel, 1, 2);

	set_short (DLP_REQUEST_DATA (req, 0, 0), volRefNum);
	
	result = dlp_exec (sd, req, &res);
	
	dlp_request_free (req);

	if (result >= 0) {
		strncpy(name, DLP_RESPONSE_DATA(res, 0, 0),
			 (size_t)(*len - 1));
		*len = strlen(name);

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			 "DLP VFSVolumeGetLabel %s\n", name));
	}
	
	dlp_response_free(res);

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)		/* no error */
		result = 0;

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_VFSVolumeSetLabel
 *
 * Summary:     Change the label (name) of a VFS volume
 *
 * Parameters:  sd			--> socket descriptor 
 *				volRefNum   --> as returned by dlp_VFSVolumeEnumerate()
 *				name		--> the new volume name
 * 
 * Returns:     -1		dlp error (see errno)
 *				0		no error
 *				>0		Palm OS error code
 *
 ***********************************************************************/
int
dlp_VFSVolumeSetLabel(int sd, int volRefNum, const char *name)
{
	int 	result;
	struct dlpRequest *req; 
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSVolumeSetLabel);

	req = dlp_request_new (dlpFuncVFSVolumeSetLabel, 1,
			2 + (strlen(name) + 1));

	set_short (DLP_REQUEST_DATA (req, 0, 0), volRefNum);
	strcpy (DLP_REQUEST_DATA (req, 0, 2), name);

	result = dlp_exec (sd, req, &res);

	dlp_response_free (res);
	dlp_request_free (req);

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)		/* no error */
		result = 0;

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_VFSVolumeSize
 *
 * Summary:     Return the total and used size of a VFS volume
 *
 * Parameters:  sd			--> socket descriptor 
 *				volRefNum   --> as returned by dlp_VFSVolumeEnumerate()
 *				volSizeUsed <-- number of bytes used on the volume
 *				volSizeTotal<-- total size of the volume
 * 
 * Returns:     -1		dlp error (see errno)
 *				0		no error
 *				>0		Palm OS error code
 *
 ***********************************************************************/
int
dlp_VFSVolumeSize(int sd, int volRefNum, long *volSizeUsed,
	long *volSizeTotal)
{
	int 	result;
	struct dlpRequest *req; 
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSVolumeSize);

	req = dlp_request_new (dlpFuncVFSVolumeSize, 1, 2);

	set_short (DLP_REQUEST_DATA (req, 0, 0), volRefNum);

	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);
	
	if (result >= 0) {
		*volSizeUsed = get_long (DLP_RESPONSE_DATA (res, 0, 0));
		*volSizeTotal = get_long (DLP_RESPONSE_DATA (res, 0, 4));
	
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		     "DLP VFS Volume Size total: %d used: %d\n",
		     *volSizeTotal, *volSizeUsed));
	}

	dlp_response_free (res);

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)		/* no error */
		result = 0;

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_VFSFileSeek
 *
 * Summary:     Change the current seek position in an open file
 *
 * Parameters:  sd			--> socket descriptor
 *				fileRef		--> file reference obtained from dlp_VFSFileOpen()
 *				origin		--> seek origin (use vfsOriginXXX from pi-dlp.h)
 *				offset		--> offset relative to the chosen origin
 * 
 * Returns:     -1		dlp error (see errno)
 *				0		no error
 *				>0		Palm OS error code
 *
 ***********************************************************************/
int
dlp_VFSFileSeek(int sd, FileRef fileRef, int origin, int offset)
{
	int 	result;
	struct dlpRequest *req; 
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileSeek);

	LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		"Seek %x to offset %d from origin %d\n",
			fileRef,offset,origin));
	
	req = dlp_request_new (dlpFuncVFSFileSeek, 1, 10);
	
	set_long (DLP_REQUEST_DATA (req, 0, 0), fileRef);
	set_short (DLP_REQUEST_DATA (req, 0, 4), origin);
	set_long (DLP_REQUEST_DATA (req, 0, 6), offset); 

	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);
	dlp_response_free (res);

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)		/* no error */
		result = 0;

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_VFSFileResize
 *
 * Summary:     Resize an open file
 *
 * Parameters:  sd			--> socket descriptor
 *				fileRef		--> file reference obtained from dlp_VFSFileOpen()
 *				newSize		--> the new file size
 * 
 * Returns:     
 *
 ***********************************************************************/
int
dlp_VFSFileResize(int sd, FileRef fileRef, int newSize)
{
	int 	result;
	struct dlpRequest *req; 
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1, 3);
	Trace(dlp_VFSFileResize);

	LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		"Resize %x to %d bytes\n", fileRef, newSize));
	
	req = dlp_request_new(dlpFuncVFSFileResize, 1, 8);
	
	set_long(DLP_REQUEST_DATA(req, 0, 0), fileRef);
	set_long(DLP_REQUEST_DATA(req, 0, 4), newSize);
	
	result = dlp_exec(sd, req, &res);

	dlp_request_free (req);
	dlp_response_free (res);
	
	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)		/* no error */
		result = 0;

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_VFSFileSize
 *
 * Summary:     Return the size of an open file
 *
 * Parameters:  sd			--> socket descriptor
 *				fileRef		--> file reference obtained from dlp_VFSFileOpen()
 *				size		<-- the size of the file
 * 
 * Returns:     -1		dlp error (see errno)
 *				0		no error
 *				>0		Palm OS error code
 *
 ***********************************************************************/
int
dlp_VFSFileSize(int sd, FileRef fileRef, int *size)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1, 3);
	Trace (dlp_VFSFileSize);
	
	req = dlp_request_new (dlpFuncVFSFileSize, 1, 4);
	
	set_long (DLP_REQUEST_DATA (req, 0, 0), fileRef);
	
	result = dlp_exec (sd, req, &res);
	
	dlp_request_free (req);
	
	if (result >= 0) {
		*size = get_long (DLP_RESPONSE_DATA (res, 0, 0));

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			"DLP VFS File Size: %d\n", *size));
	}
	
	dlp_response_free (res);

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)		/* no error */
		result = 0;

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_ExpSlotMediaType
 *
 * Summary:     Return the type of media supported by an expansion slot
 *				(DLP 1.4 only)
 *
 * Parameters:  sd          --> socket descriptor
 *              slotNum     --> slot to query (1...n)
 *              mediaType   <-- media type
 *
 * Returns:     -1          dlp error (see errno)
 *              0           no error
 *              >0          Palm OS error code
 *
 ***********************************************************************/
int
dlp_ExpSlotMediaType(int sd, int slotNum, unsigned long *mediaType)
{
	int     result;
	struct dlpRequest *req;
	struct dlpResponse *res;
 
	RequireDLPVersion(sd,1, 4);
	Trace (dlp_ExpSlotMediaType);

	req = dlp_request_new (dlpFuncExpSlotMediaType, 1, 2);

	set_short (DLP_REQUEST_DATA (req, 0, 0), slotNum);

	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);

	if (result >= 0) {
		*mediaType = get_long (DLP_RESPONSE_DATA (res, 0, 0));

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			"DLP Media Type for slot %d: %4.4s\n", 
			slotNum, mediaType));
	}

	dlp_response_free (res);

	if (result < -1)			/* negated Palm OS error code */
		result = -result;
	else if (result > 0)		/* no error */
		result = 0;

	return result;
}
