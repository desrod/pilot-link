/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
/*                              
 * dlp.c:  Palm DLP protocol
 *
 * Copyright (c) 1996, 1997, Kenneth Albanowski
 * Copyright (c) 1998-2003, ???
 * Copyright (c) 2004, Florent Pillet
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

/***************************************************************************
 *
 * Documentation marked with DHS is my fault.
 *               -- David H. Silber <pilot@SilberSoft.com>
 *
 *  Key to documentation
 *
 *  Parameters marked with '-->' are data passed in to the function.
 *  Parameters marked with '<--' are data passed back from the function.
 *    If the argument is a NULL, no data is returned.
 *  Parameters marked with '<->' are used for data passed in and then
 *    (possibly modified) data passed back.
 *
 *  -- DHS
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
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
#include "pi-error.h"

#define get_date(ptr) (dlp_ptohdate((ptr)))

#define set_date(ptr,val) (dlp_htopdate((val),(ptr)))

#define	RequireDLPVersion(sd,major,minor)	\
	if (pi_version(sd) < (((major)<<8) | (minor))) \
		return dlpErrNotSupp

/* Define prototypes */
#ifdef PI_DEBUG
static void record_dump (unsigned long recID, unsigned int recIndex,
	int flags, int catID, char *data, int data_len);
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

#ifdef DLP_TRACE
static int dlp_trace = 0;
#endif
static int dlp_version_major = PI_DLP_VERSION_MAJOR;
static int dlp_version_minor = PI_DLP_VERSION_MINOR;

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

#ifdef PI_DEBUG
static void record_dump (unsigned long recID, unsigned int recIndex, int flags,
	int catID, char *data, int data_len)
{
	LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
	    "  ID: 0x%8.8lX, Index: %u, Category: %d\n"
	    "  Flags:%s%s%s%s%s%s (0x%2.2X), and %d bytes:\n",
	    (unsigned long) recID,
	    recIndex,
		catID,
	    (flags & dlpRecAttrDeleted) ? " Deleted" : "",
	    (flags & dlpRecAttrDirty) ? " Dirty" : "",
	    (flags & dlpRecAttrBusy) ? " Busy" : "",
	    (flags & dlpRecAttrSecret) ? " Secret" : "",
	    (flags & dlpRecAttrArchived) ? " Archive" : "",
	    (!flags) ? " None" : "",
	    flags, data_len));
	dumpdata(data, (size_t)data_len);
}
#endif

/***************************************************************************
 *
 * Function:	dlp_set_protocol_version
 *
 * Summary:	set the protocol version we should announce to the device when
 *		it connects (see pi-dlp.h for details). This should be done
 *		prior to connecting with device if you want to change from
 *		the defaults.
 *
 * Parameters:	major	--> protocol major version
 *		minor	--> protocol minor version
 *
 * Returns:     nothing
 *
 ***************************************************************************/
void
dlp_set_protocol_version(int major, int minor)
{
	dlp_version_major = major;
	dlp_version_minor = minor;
}

/***************************************************************************
 *
 * Function:	dlp_strerror
 *
 * Summary:	lookup text for dlp error
 *
 * Parameters:	error number
 *
 * Returns:     char* to error text string
 *
 ***************************************************************************/
char 
*dlp_strerror(int error)
{
	if (error < 0)
		error = -error;
	
	if ((unsigned int) error >= (sizeof(dlp_errorlist)/(sizeof(char *))))
		return "Unknown error";
	
	return dlp_errorlist[error];
}


/***************************************************************************
 *
 * Function:	dlp_arg_new
 *
 * Summary:	create a dlpArg instance
 *
 * Parameters:	id_, length of data
 *
 * Returns:     dlpArg* or NULL on failure
 *
 ***************************************************************************/
struct dlpArg
*dlp_arg_new (int id_, size_t len) 
{
	struct dlpArg *arg;
	
	arg = (struct dlpArg *)malloc(sizeof (struct dlpArg));

	if (arg != NULL) {
		arg->id_ = id_;
		arg->len = len;
		arg->data = NULL;

		if (len > 0) {
			arg->data = malloc (len);
			if (arg->data == NULL) {
				free(arg);
				arg = NULL;
			}
		}
	}	
	
	return arg;
}


/***************************************************************************
 *
 * Function:	dlp_arg_free
 *
 * Summary:	frees a dlpArg instance
 *
 * Parameters:	dlpArg*
 *
 * Returns:     void
 *
 ***************************************************************************/
void
dlp_arg_free (struct dlpArg *arg)
{
	if (arg != NULL) {
		if (arg->data != NULL)
			free (arg->data);
		free (arg);
	}
}


/***************************************************************************
 *
 * Function:	dlp_arg_len
 *
 * Summary:	computes aggregate length of data members associated with an
 *		array of dlpArg instances
 *
 * Parameters:	number of dlpArg instances, dlpArg**
 *
 * Returns:     aggregate length or -1 on error
 *
 ***************************************************************************/
int
dlp_arg_len (int argc, struct dlpArg **argv)
{
	int i, len = 0;

	for (i = 0; i < argc; i++) {
		struct dlpArg *arg = argv[i];
		
		/* FIXME: shapiro: should these be < or <= ??? */
		if (arg->len < PI_DLP_ARG_TINY_LEN &&
		    (arg->id_ & (PI_DLP_ARG_FLAG_SHORT | PI_DLP_ARG_FLAG_LONG)) == 0)
			len += 2;
		else if (arg->len < PI_DLP_ARG_SHORT_LEN &&
		         (arg->id_ & PI_DLP_ARG_FLAG_LONG) == 0)
			len += 4;
		else
			len += 6;

		len += arg->len;
	}

	return len;
}


/***************************************************************************
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
 ***************************************************************************/
struct dlpRequest*
dlp_request_new (enum dlpFunctions cmd, int argc, ...) 
{
	struct dlpRequest *req;
	va_list ap;
	int 	i,
		j;
	
	req = (struct dlpRequest *)malloc (sizeof (struct dlpRequest));

	if (req != NULL) {
		req->cmd = cmd;
		req->argc = argc;
		req->argv = NULL;

		if (argc) {
			req->argv = (struct dlpArg **) malloc (sizeof (struct dlpArg *) * argc);
			if (req->argv == NULL) {
				free(req);
				return NULL;
			}
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
				free(req->argv);
				free(req);
				req = NULL;
				break;
			}
		}
		va_end (ap);
	}
	
	return req;	
}


/***************************************************************************
 *
 * Function:	dlp_request_new_with_argid
 *
 * Summary:	creates a new dlpRequest instance with argid
 *
 * Parameters:	dlpFunction command, number of dlpArgs, argid, lengths of
 *		dlpArgs data member
 *
 * Returns:     dlpRequest* or NULL if failure
 *
 ***************************************************************************/
struct dlpRequest*
dlp_request_new_with_argid (enum dlpFunctions cmd, int argid, int argc, ...)
{
	struct dlpRequest *req;
	va_list ap;
	int	i,
		j;
	
	req = (struct dlpRequest *) malloc (sizeof (struct dlpRequest));

	if (req != NULL) {
		req->cmd = cmd;
		req->argc = argc;
		req->argv = NULL;

		if (argc) {
			req->argv = (struct dlpArg **) malloc (sizeof (struct dlpArg *) * argc);
			if (req->argv == NULL) {
				free(req);
				return NULL;
			}
		}

		va_start (ap, argc);
		for (i = 0; i < argc; i++) {
			size_t len;

			len = va_arg (ap, size_t);
			req->argv[i] = dlp_arg_new (argid + i, len);
			if (req->argv[i] == NULL) {
				for (j = 0; j < i; j++)
					dlp_arg_free(req->argv[j]);
				free(req->argv);
				free(req);
				req = NULL;
				break;
			}
		}
		va_end (ap);
	}

	return req;
}


/***************************************************************************
 *
 * Function:	dlp_response_new
 *
 * Summary:	creates a new dlpResponse instance 
 *
 * Parameters:	dlpFunction command, number of dlpArg instances
 *
 * Returns:     dlpResponse* or NULL if failure
 *
 ***************************************************************************/
struct dlpResponse
*dlp_response_new (enum dlpFunctions cmd, int argc) 
{
	struct dlpResponse *res;
	
	res = (struct dlpResponse *) malloc (sizeof (struct dlpResponse));

	if (res != NULL) {

		res->cmd = cmd;
		res->err = dlpErrNoError;
		res->argc = argc;
		res->argv = NULL;

		if (argc) {
			res->argv = (struct dlpArg **) malloc (sizeof (struct dlpArg *) * argc);
			if (res->argv == NULL) {
				free(res);
				return NULL;
			}
			/* zero-out argv so that in case of error during
			   response read, dlp_response_free() won't try to
			   free uninitialized ptrs */
			memset(res->argv, 0, sizeof (struct dlpArg *) * argc);
		}
	}
	
	return res;
}


/***************************************************************************
 *
 * Function:	dlp_response_read
 *
 * Summary:	reads dlp response
 *
 * Parameters:	dlpResonse**, sd
 *
 * Returns:     first dlpArg response length or -1 on error
 *
 ***************************************************************************/
ssize_t
dlp_response_read (struct dlpResponse **res, int sd)
{
	struct dlpResponse *response;
	unsigned char *buf;
	short argid;
	int i;
	ssize_t bytes;
	size_t len;
	pi_buffer_t *dlp_buf;
	
	dlp_buf = pi_buffer_new (DLP_BUF_SIZE);
	if (dlp_buf == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	bytes = pi_read (sd, dlp_buf, dlp_buf->allocated);      /* buffer will grow as needed */
	if (bytes < 0) {
		pi_buffer_free (dlp_buf);
		return bytes;
	}
	if (bytes < 4) {
		/* packet is probably incomplete */
#if DEBUG
		LOG((PI_DBG_DLP, PI_DBG_LVL_ERR,
			"dlp_response_read: response too short (%d bytes)\n",
			bytes));
		if (bytes)
			dumpdata(dlp_buf->data, (size_t)dlp_buf->used);
#endif
		return pi_set_error(sd, PI_ERR_DLP_COMMAND);
	}

	response = dlp_response_new (dlp_buf->data[0] & 0x7f, dlp_buf->data[1]);
	*res = response;

	/* note that in case an error occurs, we do not deallocate the response
	   since callers already do it under all circumstances */
	if (response == NULL) {
		pi_buffer_free (dlp_buf);
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);
	}

	response->err = get_short (&dlp_buf->data[2]);
	pi_set_palmos_error(sd, (int)response->err);

	/* FIXME: add bounds checking to make sure we don't access past
	 * the end of the buffer in case the data is corrupt */
	buf = dlp_buf->data + 4;
	for (i = 0; i < response->argc; i++) {
		argid = get_byte (buf) & 0x3f;
		if (get_byte(buf) & PI_DLP_ARG_FLAG_LONG) {
			if (pi_version(sd) < 0x0104) {
				/* we received a response from a device indicating that
				   it would have transmitted a >64k data block but DLP
				   versions prior to 1.4 don't have this capacity. In
				   this case (as observed on a T3), there is NO length
				   stored after the argid, it goes straigt to the data
				   contents. We need to report that the data is too large
				   to be transferred.
				*/
				pi_buffer_free (dlp_buf);
				return pi_set_error(sd, PI_ERR_DLP_DATASIZE);
			}
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
			pi_buffer_free (dlp_buf);
			return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);
		}
		memcpy (response->argv[i]->data, buf, len);
		buf += len;
	}

	pi_buffer_free (dlp_buf);

	return response->argc ? response->argv[0]->len : 0;
}


/***************************************************************************
 *
 * Function:	dlp_request_write
 *
 * Summary:	writes dlp request
 *
 * Parameters:	dlpRequest**, sd
 *
 * Returns:     response length or -1 on error
 *
 ***************************************************************************/
ssize_t
dlp_request_write (struct dlpRequest *req, int sd)
{
	unsigned char *exec_buf, *buf;
	int i;
	size_t len;
	
	len = dlp_arg_len (req->argc, req->argv) + 2;
	exec_buf = (unsigned char *) malloc (sizeof (unsigned char) * len);
	if (exec_buf == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_byte (&exec_buf[PI_DLP_OFFSET_CMD], req->cmd);
	set_byte (&exec_buf[PI_DLP_OFFSET_ARGC], req->argc);

	buf = &exec_buf[PI_DLP_OFFSET_ARGV];	
	for (i = 0; i < req->argc; i++) {
		struct dlpArg *arg = req->argv[i];
		short argid = arg->id_;
		
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

	pi_flush(sd, PI_FLUSH_INPUT);
	if ((i = pi_write(sd, exec_buf, len)) < (ssize_t)len) {
		errno = -EIO;
		if (i >= 0 && i < (ssize_t)len)
			i = -1;
	}

	free (exec_buf);

	return i;
}


/***************************************************************************
 *
 * Function:	dlp_request_free
 *
 * Summary:	frees a dlpRequest instance
 *
 * Parameters:	dlpRequest*
 *
 * Returns:     void
 *
 ***************************************************************************/
void
dlp_request_free (struct dlpRequest *req)
{
	int i;

	if (req == NULL)
		return;

	if (req->argv != NULL) {
		for (i = 0; i < req->argc; i++) {
			if (req->argv[i] != NULL)
				dlp_arg_free (req->argv[i]);
		}
		free (req->argv);
	}

	free (req);
}


/***************************************************************************
 *
 * Function:	dlp_response_free
 *
 * Summary:	frees a dlpResponse instance
 *
 * Parameters:	dlpResponse*
 *
 * Returns:     void
 *
 ***************************************************************************/
void
dlp_response_free (struct dlpResponse *res) 
{
	int i;

	if (res == NULL)
		return;
	
	if (res->argv != NULL) {
		for (i = 0; i < res->argc; i++) {
			if (res->argv[i] != NULL)
				dlp_arg_free (res->argv[i]);
		}
		free (res->argv);
	}

	free (res);	
}


/***************************************************************************
 *
 * Function:	dlp_exec
 *
 * Summary:	writes a dlp request and reads the response
 *
 * Parameters:	dlpResponse*
 *
 * Returns:     the number of response bytes, or -1 on error
 *
 ***************************************************************************/
int
dlp_exec(int sd, struct dlpRequest *req, struct dlpResponse **res)
{
	int bytes, result;
	*res = NULL;

	if ((result = dlp_request_write (req, sd)) < req->argc) {
		LOG((PI_DBG_DLP, PI_DBG_LVL_ERR,
			    "DLP sd:%i dlp_request_write returned %i\n",
			    sd, result));
		errno = -EIO;
		return result;
	}

	if ((bytes = dlp_response_read (res, sd)) < 0) {
		LOG((PI_DBG_DLP, PI_DBG_LVL_ERR,
			    "DLP sd:%i dlp_response_read returned %i\n",
			    sd, bytes));
		errno = -EIO;
		return bytes;
	}

	/* Check to make sure the response is for this command */
	if ((*res)->cmd != req->cmd) {
		
		/* The Palm m130 and Tungsten T return the wrong code for VFSVolumeInfo */
		/* Tungsten T5 (and maybe Treo 650) return dlpFuncEndOfSync for dlpFuncWriteResource */
		if ((req->cmd != dlpFuncVFSVolumeInfo || (*res)->cmd != dlpFuncVFSVolumeSize)
			&& req->cmd != dlpFuncWriteResource)
		{
			errno = -ENOMSG;

			LOG((PI_DBG_DLP, PI_DBG_LVL_DEBUG,
				"dlp_exec: result CMD 0x%02x doesn't match requested cmd 0x%02x\n",
				(unsigned)((*res)->cmd), (unsigned)req->cmd));

			return pi_set_error(sd, PI_ERR_DLP_COMMAND);
		}
	}

	/* Check to make sure there was no error  */
	if ((*res)->err != dlpErrNoError) {
		errno = -ENOMSG;
		pi_set_palmos_error(sd, (int)((*res)->err));
		return pi_set_error(sd, PI_ERR_DLP_PALMOS);
	}

	return bytes;
}

/* These conversion functions are strictly for use within the DLP layer. 
   This particular date/time format does not occur anywhere else within the
   Palm or its communications. */

/* Notice: 
   The dates in the DLP protocol are expressed as absolute dates/times,
   without any time zone information. For example if a file was created
   on the device at 19:32:48, the time members will be 19, 32 and 48.
   This simplifies things a lot since we don't need to to time zone
   conversions. The functions below convert a breakdown DLP date to and
   from a time_t expressed in the machine's local timezone.
   -- FP */

/***************************************************************************
 *
 * Function:    dlp_ptohdate
 *
 * Summary:     Convert Palm format to time_t
 *
 * Parameters:  char* to time data buffer
 *
 * Returns:     time_t struct to mktime
 *
 ***************************************************************************/
time_t
dlp_ptohdate(const unsigned char *data)
{
	struct tm t;

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

	memset(&t, 0, sizeof(t));
	t.tm_sec 	= (int) data[6];
	t.tm_min 	= (int) data[5];
	t.tm_hour 	= (int) data[4];
	t.tm_mday 	= (int) data[3];
	t.tm_mon 	= (int) data[2] - 1;
	t.tm_year 	= (((int)data[0] << 8) | (int)data[1]) - 1900;
	t.tm_isdst 	= -1;

	return mktime(&t);
}

/***************************************************************************
 *
 * Function:    dlp_htopdate
 *
 * Summary:     Convert time_t to Palm format
 *
 * Parameters:  time_t, char* to time data buffer
 *
 * Returns:     void
 *
 ***************************************************************************/
void
dlp_htopdate(time_t ti, unsigned char *data)
{				/* @+ptrnegate@ */
	int 	year;
	const struct tm *t = localtime(&ti);

	ASSERT(t != NULL);

	year = t->tm_year + 1900;

	data[7] = (unsigned char) 0;	/* packing spacer */
	data[6] = (unsigned char) t->tm_sec;
	data[5] = (unsigned char) t->tm_min;
	data[4] = (unsigned char) t->tm_hour;
	data[3] = (unsigned char) t->tm_mday;
	data[2] = (unsigned char) (t->tm_mon + 1);
	data[0] = (unsigned char) (year >> 8);
	data[1] = (unsigned char) year;
}

/***************************************************************************
 *
 * Function:    dlp_GetSysDateTime
 *
 * Summary:     DLP 1.0 GetSysDateTime function to get device date and time
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***************************************************************************/
int
dlp_GetSysDateTime(int sd, time_t * t)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(GetSysDateTime);
	pi_reset_errors(sd);

	req = dlp_request_new(dlpFuncGetSysDateTime, 0);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);
	
	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result > 0) {
		*t = dlp_ptohdate((const unsigned char *)DLP_RESPONSE_DATA (res, 0, 0));

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			"DLP GetSysDateTime %s", ctime(t)));
	}

	dlp_response_free(res);
	
	return result;
}


/***************************************************************************
 *
 * Function:    dlp_SetSysDateTime
 *
 * Summary:     DLP 1.0 SetSysDateTime function to set the device date and
 *		time
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***************************************************************************/
int
dlp_SetSysDateTime(int sd, time_t t)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(SetSysDateTime);
	pi_reset_errors(sd);

	req = dlp_request_new(dlpFuncSetSysDateTime, 1, 8);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);
	
	dlp_htopdate(t, (unsigned char *)DLP_REQUEST_DATA(req, 0, 0));

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	dlp_response_free(res);

	return result;
}


/***************************************************************************
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
 ***************************************************************************/
int
dlp_ReadStorageInfo(int sd, int cardno, struct CardInfo *c)
{
	int 	result;
	size_t len1, len2;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(ReadStorageInfo);
	pi_reset_errors(sd);

	req = dlp_request_new(dlpFuncReadStorageInfo, 1, 2);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), cardno);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);

	if (result > 0) {
		c->more 	= get_byte(DLP_RESPONSE_DATA(res, 0, 0)) 
			|| (get_byte(DLP_RESPONSE_DATA(res, 0, 3)) > 1);
		c->card 	= get_byte(DLP_RESPONSE_DATA(res, 0, 5));
		c->version 	= get_byte(DLP_RESPONSE_DATA(res, 0, 6));
		c->creation 	= get_date((const unsigned char *)DLP_RESPONSE_DATA(res, 0, 8));
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


/***************************************************************************
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
 ***************************************************************************/
int
dlp_ReadSysInfo(int sd, struct SysInfo *s)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	Trace(ReadSysInfo);
	pi_reset_errors(sd);

	req = dlp_request_new (dlpFuncReadSysInfo, 1, 4);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_short (DLP_REQUEST_DATA (req, 0, 0), dlp_version_major);
	set_short (DLP_REQUEST_DATA (req, 0, 2), dlp_version_minor);
	
	result = dlp_exec(sd, req, &res);

	dlp_request_free (req);

	if (result > 0) {
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


/***************************************************************************
 *
 * Function:    dlp_ReadDBList
 *
 * Summary:     Iterate through the list of databases on the Palm
 *
 * Parameters:  sd	--> client socket
 *		cardno  --> card number (should be 0)
 *		flags   --> see enum dlpDBList in pi-dlp.h
 *		start   --> index of first database to list
 *		info	<-> buffer containing one or more DBInfo structs
 *			    depending on `flags' and the DLP version running
 *			    on the device.
 *
 * Returns:	A negative number on error, the number of bytes read otherwise.
 *		Use (info->used / sizeof(DBInfo)) to know how many database
 *		information blocks were returned
 *
 ***************************************************************************/
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
	pi_reset_errors(sd);

	req = dlp_request_new (dlpFuncReadDBList, 1, 4);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	pi_buffer_clear (info);
	
	/* `multiple' only supported in DLP 1.2 and above */
	if (pi_version(sd) < 0x0102)
		flags &= ~dlpDBListMultiple;

	set_byte(DLP_REQUEST_DATA(req, 0, 0), (unsigned char) flags);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), (unsigned char) cardno);
	set_short(DLP_REQUEST_DATA(req, 0, 2), start);

	result = dlp_exec (sd, req, &res);

	dlp_request_free(req);
	
	if (result > 0) {
		p = (unsigned char *)DLP_RESPONSE_DATA(res, 0, 0);
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

			memset(db.name, 0, sizeof(db.name));
			strncpy(db.name, (char *)(p + 48), 32);

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
			    "  Modnum: %ld, Index: %d, Creation date: 0x%08lx, %s",
			    db.modnum, db.index, db.createDate, ctime(&db.createDate)));
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			    " Modification date: 0x%08lx, %s", db.modifyDate, ctime(&db.modifyDate)));
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO, 
			    " Backup date: 0x%08lx, %s", db.backupDate, ctime(&db.backupDate)));

			if (pi_buffer_append(info, &db, sizeof(db)) == NULL) {
				result = pi_set_error(sd, PI_ERR_GENERIC_MEMORY);
				break;
			}

			p += get_byte(p + 4);
		}
	} else {
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
				"Error in dlp_ReadDBList: %d\n", result));
	}

	dlp_response_free (res);

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_FindDBInfo
 *
 * Summary:     Search for a database on the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***************************************************************************/
int
dlp_FindDBInfo(int sd, int cardno, int start, const char *dbname,
	       unsigned long type, unsigned long creator,
	       struct DBInfo *info)
{
	int 	i,
		j;
	pi_buffer_t *buf;

	/* This function does not match any DLP layer function, but is
	   intended as a shortcut for programs looking for databases. It
	   uses a fairly byzantine mechanism for ordering the RAM databases
	   before the ROM ones.  You must feed the "index" slot from the
	   returned info in as start the next time round. */

	pi_reset_errors(sd);

	buf = pi_buffer_new (sizeof (struct DBInfo));
	if (buf == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	if (start < 0x1000) {
		i = start;
		while (dlp_ReadDBList(sd, cardno, 0x80 | dlpDBListMultiple, i, buf) >= 0) {
			for (j=0; j < (int)(buf->used / sizeof(struct DBInfo)); j++) {
				memcpy (info, buf->data + j * sizeof(struct DBInfo), sizeof(struct DBInfo));
				if ((!dbname || strcmp(info->name, dbname) == 0)
					&& (!type || info->type == type)
					&& (!creator || info->creator == creator))
					goto found;
				i = info->index + 1;
			}
		}
		start = 0x1000;
	}

	i = start & 0xFFF;
	while (dlp_ReadDBList(sd, cardno, 0x40 | dlpDBListMultiple, i, buf) >= 0) {
		for (j=0; j < (int)(buf->used / sizeof(struct DBInfo)); j++) {
			memcpy (info, buf->data + j * sizeof(struct DBInfo), sizeof(struct DBInfo));
			if ((!dbname || strcmp(info->name, dbname) == 0)
				&& (!type || info->type == type)
				&& (!creator || info->creator == creator))
			{
				info->index |= 0x1000;
				goto found;
			}
			i = info->index + 1;
		}
	}

	pi_buffer_free (buf);
	return -1;

found:
	pi_buffer_free (buf);
	return 0;
}

/***************************************************************************
 *
 * Function:	dlp_decode_finddb_response
 *
 * Summary:	Response decoding for the three variants of dlp_FindDB
 *
 * Parameters:	None
 *
 * Returns:	Nothing
 *
 ***************************************************************************/
static void
dlp_decode_finddb_response(struct dlpResponse *res, int *cardno, unsigned long *localid,
	int *dbhandle, struct DBInfo *info, struct DBSizeInfo *size)
{
	int arg, argid;
	for (arg = 0; arg < res->argc; arg++) {
		argid = (res->argv[arg]->id_ & 0x7f) - PI_DLP_ARG_FIRST_ID;
		if (argid == 0) {
			if (cardno)
				*cardno = get_byte(DLP_RESPONSE_DATA(res, arg, 0));
			if (localid)
				*localid = get_long(DLP_RESPONSE_DATA(res, arg, 2));
			if (dbhandle)
				*dbhandle = get_long(DLP_RESPONSE_DATA(res, arg, 6));

			if (info) {
				info->more = 0;
				info->miscFlags =
					get_byte(DLP_RESPONSE_DATA(res, arg, 11));
				info->flags =
					get_short(DLP_RESPONSE_DATA(res, arg, 12));
				info->type =
					get_long(DLP_RESPONSE_DATA(res, arg, 14));
				info->creator =
					get_long(DLP_RESPONSE_DATA(res, arg, 18));
				info->version =
					 get_short(DLP_RESPONSE_DATA(res, arg, 22));
				info->modnum =
					get_long(DLP_RESPONSE_DATA(res, arg, 24));
				info->createDate =
					 get_date((const unsigned char *)DLP_RESPONSE_DATA(res, arg, 28));
				info->modifyDate =
					 get_date((const unsigned char *)DLP_RESPONSE_DATA(res, arg, 36));
				info->backupDate =
					 get_date((const unsigned char *)DLP_RESPONSE_DATA(res, arg, 44));
				info->index =
					 get_short(DLP_RESPONSE_DATA(res, arg, 52));

				strncpy(info->name, DLP_RESPONSE_DATA(res, arg, 54), 32);
				info->name[32] = '\0';

				LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
					 "DLP FindDB Name: '%s', "
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
		}
		else if (argid == 1) {
			if (size) {
				size->numRecords =
					get_long(DLP_RESPONSE_DATA(res, arg, 0));
				size->totalBytes =
					get_long(DLP_RESPONSE_DATA(res, arg, 4));
				size->dataBytes =
					get_long(DLP_RESPONSE_DATA(res, arg, 8));
				size->appBlockSize =
					get_long(DLP_RESPONSE_DATA(res, arg, 12));
				size->sortBlockSize =
					get_long(DLP_RESPONSE_DATA(res, arg, 16));
				size->maxRecSize =
					get_long(DLP_RESPONSE_DATA(res, arg, 20));
			}
		}
	}
}

/***************************************************************************
 *
 * Function:	dlp_FindDBByName
 *
 * Summary:	Search for a database on the Palm by explicit name
 *
 * Parameters:	None
 *
 * Returns:	Nothing
 *
 ***************************************************************************/
int
dlp_FindDBByName (int sd, int cardno, PI_CONST char *name, unsigned long *localid,
	int *dbhandle, struct DBInfo *info, struct DBSizeInfo *size)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	int flags = 0;
	
	Trace(FindDBByName);
	pi_reset_errors(sd);

	if (pi_version(sd) < 0x0102) {
		pi_set_error(sd, PI_ERR_DLP_UNSUPPORTED);
		return -129;
	}

	req = dlp_request_new(dlpFuncFindDB, 1, 2 + (strlen(name) + 1));
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	if (localid || dbhandle || info)
		flags |= dlpFindDBOptFlagGetAttributes;
	if (size)
		flags |= dlpFindDBOptFlagGetSize;

	set_byte(DLP_REQUEST_DATA(req, 0, 0), flags);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), cardno);
	strcpy(DLP_REQUEST_DATA(req, 0, 2), name);

	result = dlp_exec(sd, req, &res);
	
	dlp_request_free(req);

	if (result > 0)
		dlp_decode_finddb_response(res, NULL, localid, dbhandle, info, size);
	
	dlp_response_free(res);
	
	return result;	
}


/***************************************************************************
 *
 * Function:    dlp_FindDBByOpenHandle
 *
 * Summary:     Search for a database on the Palm by database handle
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***************************************************************************/
int
dlp_FindDBByOpenHandle (int sd, int dbhandle, int *cardno,
	 unsigned long *localid, struct DBInfo *info, struct DBSizeInfo *size)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	int flags = 0;
	
	Trace(FindDBByName);
	pi_reset_errors(sd);

	if (pi_version(sd) < 0x0102) {
		pi_set_error(sd, PI_ERR_DLP_UNSUPPORTED);
		return -129;
	}

	req = dlp_request_new_with_argid(dlpFuncFindDB, 0x21, 1, 2);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	/* Note: there is a bug in HotSync -- requesting the maxRecSize
	 * crashes the device, so we don't. This is supposed to work only
	 * for this variant of FindDB anyway.
	 */
	if (cardno || localid || info)
		flags |= dlpFindDBOptFlagGetAttributes;
	if (size)
		flags |= dlpFindDBOptFlagGetSize;

	set_byte(DLP_REQUEST_DATA(req, 0, 0), flags);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), dbhandle);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result > 0)
		dlp_decode_finddb_response(res, cardno, localid, NULL, info, size);
	
	dlp_response_free(res);
	
	return result;	
}


/***************************************************************************
 *
 * Function:    dlp_FindDBByTypeCreator
 *
 * Summary:     Search for a database on the Palm by CreatorID
 *
 * Parameters:  None
 *
 * Returns:     Creator ID in 'result'
 *
 ***************************************************************************/
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
	pi_reset_errors(sd);

	if (pi_version(sd) < 0x0102) {
		pi_set_error(sd, PI_ERR_DLP_UNSUPPORTED);
		return -129;
	}

	req = dlp_request_new_with_argid(dlpFuncFindDB, 0x22, 1, 10);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

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
	
	if (result > 0)
		dlp_decode_finddb_response(res, cardno, localid, dbhandle, info, size);
	
	dlp_response_free(res);
	
	return result;	
}


/***************************************************************************
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
 ***************************************************************************/
int
dlp_OpenDB(int sd, int cardno, int mode, PI_CONST char *name, int *dbhandle)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(OpenDB);
	pi_reset_errors(sd);

	req = dlp_request_new(dlpFuncOpenDB, 1, 2 + strlen(name) + 1);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), cardno);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), mode);
	strcpy(DLP_REQUEST_DATA(req, 0, 2), name);

	result = dlp_exec(sd, req, &res);
	
	dlp_request_free(req);
	
	if (result > 0) {
		*dbhandle = get_byte(DLP_RESPONSE_DATA(res, 0, 0));

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP OpenDB Handle=%d\n", *dbhandle));
	}
	
	dlp_response_free(res);
	
	return result;
}


/***************************************************************************
 *
 * Function:    dlp_DeleteDB
 *
 * Summary:     Delete a given database on the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***************************************************************************/
int
dlp_DeleteDB(int sd, int card, const char *name)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(DeleteDB);
	pi_reset_errors(sd);

	req = dlp_request_new(dlpFuncDeleteDB, 1, 2 + (strlen(name) + 1));
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), card);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
	strcpy(DLP_REQUEST_DATA(req, 0, 2), name);

	result = dlp_exec(sd, req, &res);
	
	dlp_request_free(req);
	dlp_response_free(res);
	
	return result;
}


/***************************************************************************
 *
 * Function:    dlp_CreateDB
 *
 * Summary:     Create a database on the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***************************************************************************/
int
dlp_CreateDB(int sd, unsigned long creator, unsigned long type, int cardno,
	 int flags, unsigned int version, const char *name, int *dbhandle)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(CreateDB);
	pi_reset_errors(sd);

	req = dlp_request_new(dlpFuncCreateDB, 1, 14 + (strlen(name) + 1));
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_long(DLP_REQUEST_DATA(req, 0, 0), creator);
	set_long(DLP_REQUEST_DATA(req, 0, 4), type);
	set_byte(DLP_REQUEST_DATA(req, 0, 8), cardno);
	set_byte(DLP_REQUEST_DATA(req, 0, 9), 0);
	set_short(DLP_REQUEST_DATA(req, 0, 10), flags);
	set_short(DLP_REQUEST_DATA(req, 0, 12), version);
	strcpy(DLP_REQUEST_DATA(req, 0, 14), name);

	result = dlp_exec(sd, req, &res);
	
	dlp_request_free(req);
	
	if (result > 0 && dbhandle) {
		*dbhandle = get_byte(DLP_RESPONSE_DATA(res, 0, 0));

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP CreateDB Handle=%d\n", *dbhandle));
	}
	
	dlp_response_free(res);

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_CloseDB
 *
 * Summary:     Close the database opened on the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***************************************************************************/
int
dlp_CloseDB(int sd, int dbhandle)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(CloseDB);
	pi_reset_errors(sd);

	req = dlp_request_new(dlpFuncCloseDB, 1, 1);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), (unsigned char) dbhandle);

	result = dlp_exec(sd, req, &res);
	
	dlp_request_free(req);	
	dlp_response_free(res);
	
	return result;
}


/***************************************************************************
 *
 * Function:    dlp_CloseDB_All
 *
 * Summary:     Close all the databases opened on the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***************************************************************************/
int
dlp_CloseDB_All(int sd)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(CloseDB_All);
	pi_reset_errors(sd);

	req = dlp_request_new_with_argid(dlpFuncCloseDB, 0x21, 0);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	result = dlp_exec(sd, req, &res);
	
	dlp_request_free(req);	
	dlp_response_free(res);
	
	return result;
}

/***************************************************************************
 *
 * Function:    dlp_CallApplication
 *
 * Summary:     Call an application entry point via an action code
 *
 * Parameters:	retbuf is emptied prior to receiving data
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***************************************************************************/
int
dlp_CallApplication(int sd, unsigned long creator, unsigned long type,
		    int action, size_t length, const void *data,
		    unsigned long *retcode, pi_buffer_t *retbuf)
{
	int 	result,
		version = pi_version(sd);
	size_t	data_len;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(dlp_CallApplication);
	pi_reset_errors(sd);
	if (retbuf)
		pi_buffer_clear(retbuf);

	if (version >= 0x0101) {	/* PalmOS 2.0 call encoding */

		if (length + 22 > DLP_BUF_SIZE) {
			LOG((PI_DBG_DLP, PI_DBG_LVL_ERR,
			     "DLP CallApplication: data too large (>64k)"));
			pi_set_error(sd, PI_ERR_DLP_DATASIZE);
			return -131;
		}

		req = dlp_request_new_with_argid(
				dlpFuncCallApplication, 0x21, 1, 22 + length);
		if (req == NULL)
			return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

		set_long(DLP_REQUEST_DATA(req, 0, 0), creator);
		set_long(DLP_REQUEST_DATA(req, 0, 4), type);
		set_short(DLP_REQUEST_DATA(req, 0, 8), action);
		set_long(DLP_REQUEST_DATA(req, 0, 10), length);
		set_long(DLP_REQUEST_DATA(req, 0, 14), 0);
		set_long(DLP_REQUEST_DATA(req, 0, 18), 0);
		if (length)
			memcpy(DLP_REQUEST_DATA(req, 0, 22), data, length);

		result = dlp_exec(sd, req, &res);

		dlp_request_free(req);

		if (result > 0) {
			data_len = res->argv[0]->len - 16;
			
			if (retcode)
				*retcode = get_long(DLP_RESPONSE_DATA(res, 0, 0));
			if (retbuf)
				pi_buffer_append(retbuf, DLP_RESPONSE_DATA(res, 0, 16), data_len);

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

	} else {		/* PalmOS 1.0 call encoding */

		if (length + 8 > DLP_BUF_SIZE) {
			LOG((PI_DBG_DLP, PI_DBG_LVL_ERR,
			     "DLP CallApplication: data too large (>64k)"));
			pi_set_error(sd, PI_ERR_DLP_DATASIZE);
			return -131;
		}

		req = dlp_request_new (dlpFuncCallApplication, 1, 8 + length);
		if (req == NULL)
			return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

		set_long(DLP_REQUEST_DATA(req, 0, 0), creator);
		set_short(DLP_REQUEST_DATA(req, 0, 4), action);
		set_short(DLP_REQUEST_DATA(req, 0, 6), length);
		memcpy(DLP_REQUEST_DATA(req, 0, 8), data, length);

		result = dlp_exec(sd, req, &res);

		dlp_request_free(req);

		if (result > 0) {
			data_len = res->argv[0]->len - 6;
			if (retcode)
				*retcode =
				 get_short(DLP_RESPONSE_DATA(res, 0, 2));
			if (retbuf)
				pi_buffer_append(retbuf, DLP_RESPONSE_DATA(res, 0, 6), data_len);
			
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
	}

	dlp_response_free(res);
	return result;
}


/***************************************************************************
 *
 * Function:    dlp_ResetSystem
 *
 * Summary:     Reset the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***************************************************************************/
int
dlp_ResetSystem(int sd)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(ResetSystems);
	pi_reset_errors(sd);

	req = dlp_request_new(dlpFuncResetSystem, 0);

	result = dlp_exec(sd, req, &res);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	dlp_request_free(req);	
	dlp_response_free(res);
	
	return result;
}


/***************************************************************************
 *
 * Function:    dlp_AddSyncLogEntry
 *
 * Summary:     Add text to the Palm's synchronization log
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***************************************************************************/
int
dlp_AddSyncLogEntry(int sd, char *entry)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(AddSyncLogEntry);
	pi_reset_errors(sd);

	req = dlp_request_new(dlpFuncAddSyncLogEntry, 1, strlen(entry) + 1);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	strcpy(DLP_REQUEST_DATA(req, 0, 0), entry);
	
	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);	
	dlp_response_free(res);

	if (result > 0) {
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP AddSyncLogEntry Entry: \n  %s\n", entry));
	}

	return result;
}


/***************************************************************************
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
 ***************************************************************************/
int
dlp_ReadOpenDBInfo(int sd, int dbhandle, int *records)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(ReadOpenDBInfo);
	pi_reset_errors(sd);

	req = dlp_request_new(dlpFuncReadOpenDBInfo, 1, 1);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), dbhandle);
	
	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);

	if (result > 0) {
		if (records)
			*records = get_short(DLP_RESPONSE_DATA(res, 0, 0));
		
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadOpenDBInfo %d records\n", 
		    get_short(DLP_RESPONSE_DATA(res, 0, 0))));
	}
	
	dlp_response_free(res);
	
	return result;
}

/***************************************************************************
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
 ***************************************************************************/
int
dlp_SetDBInfo (int sd, int dbhandle, int flags, int clearFlags,
	unsigned int version, time_t createDate, time_t modifyDate,
	time_t backupDate, unsigned long type, unsigned long creator)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
 	
	Trace(SetDBInfo);
	pi_reset_errors(sd);

	if (pi_version(sd) < 0x0102) {
		pi_set_error(sd, PI_ERR_DLP_UNSUPPORTED);
		return -129;
	}

	req = dlp_request_new(dlpFuncSetDBInfo, 1, 40);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), dbhandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
	set_short(DLP_REQUEST_DATA(req, 0, 2), clearFlags);
	set_short(DLP_REQUEST_DATA(req, 0, 4), flags);
	set_short(DLP_REQUEST_DATA(req, 0, 6), version);
	set_date((unsigned char *)DLP_REQUEST_DATA(req, 0, 8), createDate);
	set_date((unsigned char *)DLP_REQUEST_DATA(req, 0, 16), modifyDate);
	set_date((unsigned char *)DLP_REQUEST_DATA(req, 0, 24), backupDate);
	set_long(DLP_REQUEST_DATA(req, 0, 32), type);
	set_long(DLP_REQUEST_DATA(req, 0, 36), creator);
	
	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	dlp_response_free(res);
	
	return result;
}


/***************************************************************************
 *
 * Function:    dlp_MoveCategory
 *
 * Summary:     Move all records in a category to another category
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***************************************************************************/
int
dlp_MoveCategory(int sd, int handle, int fromcat, int tocat)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(MoveCategory);
	pi_reset_errors(sd);

	req = dlp_request_new(dlpFuncMoveCategory, 1, 4);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), handle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), fromcat);
	set_byte(DLP_REQUEST_DATA(req, 0, 2), tocat);
	set_byte(DLP_REQUEST_DATA(req, 0, 3), 0);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);	
	dlp_response_free(res);

	if (result >= 0) {
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP MoveCategory Handle: %d, From: %d, To: %d\n",
		    handle, fromcat, tocat));
	}

	return result;
}

/***************************************************************************
 *
 * Function:    dlp_OpenConduit
 *
 * Summary:     This command is sent before each conduit is opened
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***************************************************************************/
int
dlp_OpenConduit(int sd)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(OpenConduit);
	pi_reset_errors(sd);
	
	req = dlp_request_new(dlpFuncOpenConduit, 0);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);	
	dlp_response_free(res);
	
	/* if this was not done yet, this will read and cache the DLP version
	   that the Palm is running. We need this when reading responses during
	   record/resource transfers */
	if (result >= 0)
		pi_version(sd);

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_EndOfSync
 *
 * Summary:     End the sync with the given status
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***************************************************************************/
int
dlp_EndOfSync(int sd, int status)
{
	int 	result;
	pi_socket_t	*ps;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(EndOfSync);
	pi_reset_errors(sd);

	ps = find_pi_socket(sd);
	if (ps == NULL) {
		errno = ESRCH;
		return PI_ERR_SOCK_INVALID;
	}

	req = dlp_request_new(dlpFuncEndOfSync, 1, 2);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_short(DLP_REQUEST_DATA(req, 0, 0), status);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);	
	dlp_response_free(res);

	/* Messy code to set end-of-sync flag on socket 
	   so pi_close won't do it for us */
	if (result == 0)
		ps->state = PI_SOCK_CONEN;

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_AbortSync
 *
 * Summary:     Enters a sync_aborted entry into the log
 *
 * Parameters:  None
 *
 * Returns:     Return value: A negative number on error, 0 otherwise
 *
 ***************************************************************************/
int
dlp_AbortSync(int sd)
{
	pi_socket_t	*ps;

	Trace(AbortSync);
	pi_reset_errors(sd);

	/* Pretend we sent the sync end */
	if ((ps = find_pi_socket(sd)) == NULL) {
		errno = ESRCH;
		return PI_ERR_SOCK_INVALID;
	}

	ps->state = PI_SOCK_CONEN;

	return 0;
}


/***************************************************************************
 *
 * Function:    dlp_WriteUserInfo
 *
 * Summary:     Write user information to the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***************************************************************************/
int
dlp_WriteUserInfo(int sd, struct PilotUser *User)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	int len;
	
	Trace(WriteUserInfo);
	pi_reset_errors(sd);

	len = strlen (User->username) + 1;
	
	req = dlp_request_new (dlpFuncWriteUserInfo, 1, 22 + len);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_long(DLP_REQUEST_DATA(req, 0, 0), User->userID);
	set_long(DLP_REQUEST_DATA(req, 0, 4), User->viewerID);
	set_long(DLP_REQUEST_DATA(req, 0, 8), User->lastSyncPC);
	set_date((unsigned char *)DLP_REQUEST_DATA(req, 0, 12), User->lastSyncDate);
	set_byte(DLP_REQUEST_DATA(req, 0, 20), 0xff);
	set_byte(DLP_REQUEST_DATA(req, 0, 21), len);
	strcpy(DLP_REQUEST_DATA(req, 0, 22), User->username);

	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);
	dlp_response_free (res);
	
	return result;
}


/***************************************************************************
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
 ***************************************************************************/
int
dlp_ReadUserInfo(int sd, struct PilotUser *User)
{
	int 	result;
	size_t	userlen;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	Trace(ReadUserInfo);
	pi_reset_errors(sd);

	req = dlp_request_new (dlpFuncReadUserInfo, 0);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);
	
	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);

	if (result > 0) {
		User->userID =
			 get_long(DLP_RESPONSE_DATA (res, 0, 0));
		User->viewerID =
			 get_long(DLP_RESPONSE_DATA (res, 0, 4));
		User->lastSyncPC =
			 get_long(DLP_RESPONSE_DATA (res, 0, 8));
		User->successfulSyncDate =
			 get_date((const unsigned char *)DLP_RESPONSE_DATA (res, 0, 12));
		User->lastSyncDate =
			 get_date((const unsigned char *)DLP_RESPONSE_DATA (res, 0, 20));
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


/***************************************************************************
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
 ***************************************************************************/
int
dlp_ReadNetSyncInfo(int sd, struct NetSyncInfo *i)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(ReadNetSyncInfo);
	pi_reset_errors(sd);

	if (pi_version(sd) < 0x0101) {
		pi_set_error(sd, PI_ERR_DLP_UNSUPPORTED);
		return -129;	/* This call only functions under PalmOS 2.0 */
	}

	req = dlp_request_new(dlpFuncReadNetSyncInfo, 0);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

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


/***************************************************************************
 *
 * Function:    dlp_WriteNetSyncInfo
 *
 * Summary:     Write Network HotSync settings to the Palm
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***************************************************************************/
int
dlp_WriteNetSyncInfo(int sd, struct NetSyncInfo *i)
{
	int 	result,
		str_offset = 24;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(WriteNetSyncInfo);
	pi_reset_errors(sd);

	if (pi_version(sd) < 0x0101) {
		pi_set_error(sd, PI_ERR_DLP_UNSUPPORTED);
		return -129;
	}

	LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
	    "DLP ReadNetSyncInfo Active: %d\n", i->lanSync ? 1 : 0));
	LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
	    "  PC hostname: '%s', address '%s', mask '%s'\n",
	    i->hostName, i->hostAddress, i->hostSubnetMask));

	req = dlp_request_new(dlpFuncWriteNetSyncInfo, 1,
		24 + strlen(i->hostName) + 
		strlen(i->hostAddress) + strlen(i->hostSubnetMask) + 3);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

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
/***************************************************************************
 *
 * Function:    dlp_RPC
 *
 * Summary:     Remote Procedure Calls interface
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***************************************************************************/
int
dlp_RPC(int sd, struct RPC_params *p, unsigned long *result)
{
	int 	i,
		err = 0;
	long 	D0 = 0,
		A0 = 0;
	unsigned char *c;
	pi_buffer_t *dlp_buf;

	Trace(dlp_RPC);
	pi_reset_errors(sd);

	/* RPC through DLP breaks all the rules and isn't well documented to
	   boot */
	dlp_buf = pi_buffer_new (DLP_BUF_SIZE);
	if (dlp_buf == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

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

	if (pi_write(sd, dlp_buf->data, (size_t)(c - dlp_buf->data)) > 0) {
		err = 0;
		if (p->reply) {
			int l = pi_read(sd, dlp_buf, (size_t)(c - dlp_buf->data + 2));

			if (l < 0)
				err = l;
			else if (l < 6)
				err = -1;
			else if (dlp_buf->data[0] != 0xAD)
				err = -2;
			else if (get_short(dlp_buf->data + 2)) {
				err = -get_short(dlp_buf->data + 2);
				pi_set_palmos_error(sd, -err);
			} else {
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


/***************************************************************************
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
 ***************************************************************************/
int
dlp_ReadFeature(int sd, unsigned long creator, unsigned int num,
		unsigned long *feature)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(dlp_ReadFeature);
	pi_reset_errors(sd);

	if (pi_version(sd) < 0x0101) {
		struct RPC_params p;		
		int val;
		unsigned long errCode;

		if (feature == NULL)
			return 0;

		*feature = 0x12345678;

		PackRPC(&p, 0xA27B, RPC_IntReply,
			RPC_Long(creator),
			RPC_Short((unsigned short) num),
			RPC_LongPtr(feature), RPC_End);

		val = dlp_RPC(sd, &p, &errCode);

		if (val < 0) {
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			    "DLP ReadFeature Error: %s (%d)\n",
			    dlp_errorlist[-val], val));

			return val;
		}
		
		if (errCode) {
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			    "DLP ReadFeature FtrGet error 0x%8.8lX\n",
			    res));
			pi_set_palmos_error(sd, (int)errCode);
			return pi_set_error(sd, PI_ERR_DLP_PALMOS);
		}

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    " DLP ReadFeature Feature: 0x%8.8lX\n",
		    (unsigned long) *feature));
		
		return 0;
	}

	Trace(dlp_ReadFeatureV2);

	req = dlp_request_new(dlpFuncReadFeature, 1, 6);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_long(DLP_REQUEST_DATA(req, 0, 0), creator);
	set_short(DLP_REQUEST_DATA(req, 0, 4), num);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result > 0) {
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
#endif				/* IFDEF _PILOT_SYSPKT_H */


/***************************************************************************
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
 ***************************************************************************/
int
dlp_GetROMToken(int sd, unsigned long token, char *buffer, size_t *size)
{
	unsigned long result;

	struct RPC_params p;
	
	int val;
	unsigned long buffer_ptr;

	Trace(dlp_GetROMToken);
	pi_reset_errors(sd);
	
#ifdef DLP_TRACE
	if (dlp_trace) {
	  fprintf(stderr,
		  " Wrote: Token: '%s'\n",
		  printlong(token));
	}
#endif

	PackRPC(&p, 0xa340, RPC_IntReply,		/* sysTrapHwrGetROMToken */
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

	if (buffer) {
		buffer[*size] = 0;

		PackRPC(&p, 0xa026, RPC_IntReply,		/* sysTrapMemMove */
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
		return -((int)result);

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_ResetLastSyncPC
 *
 * Summary:     Reset the LastSyncPC ID so we can start again
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***************************************************************************/
int
dlp_ResetLastSyncPC(int sd)
{
	int 	err;
	struct 	PilotUser User;

	Trace(dlp_ResetLastSyncPC);

	if ((err = dlp_ReadUserInfo(sd, &User)) < 0)
		return err;

	User.lastSyncPC = 0;

	return dlp_WriteUserInfo(sd, &User);
}


/***************************************************************************
 *
 * Function:    dlp_ResetDBIndex
 *
 * Summary:     Reset the modified records index
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***************************************************************************/
int
dlp_ResetDBIndex(int sd, int dbhandle)
{
	int 	result;
	pi_socket_t	*ps;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(dlp_ResetDBIndex);
	pi_reset_errors(sd);

	if ((ps = find_pi_socket(sd)) == NULL) {
		errno = ESRCH;
		return PI_ERR_SOCK_INVALID;
	}

	ps->dlprecord = 0;

	req = dlp_request_new(dlpFuncResetRecordIndex, 1, 1);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), dbhandle);
	
	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);	
	dlp_response_free(res);
	
	return result;
}


/***************************************************************************
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
 ***************************************************************************/
int
dlp_ReadRecordIDList(int sd, int dbhandle, int sort, int start, int max,
		     recordid_t * IDs, int *count)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(dlp_ReadRecordIDList);
	pi_reset_errors(sd);

	req = dlp_request_new(dlpFuncReadRecordIDList, 1, 6);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), dbhandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), sort ? 0x80 : 0);
	set_short(DLP_REQUEST_DATA(req, 0, 2), start);
	set_short(DLP_REQUEST_DATA(req, 0, 4), max);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);

	if (result > 0) {
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


/***************************************************************************
 *
 * Function:    dlp_WriteRecord
 *
 * Summary:     Writes a record to database. If recID is 0, the device will
 *		create a new id and the variable the NewID pointer points to
 *		will be set to the new id. If length is -1, function will
 *		consider data as a string and write a data block of (strlen
 *		+ 1).
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***************************************************************************/
int
dlp_WriteRecord(int sd, int dbhandle, int flags, recordid_t recID,
		int catID, void *data, size_t length, recordid_t *pNewRecID)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(dlp_WriteRecord);
	pi_reset_errors(sd);

	if (length == (size_t)-1)
		length = strlen((char *) data) + 1;

	if (pi_version(sd) >= 0x0104) {
		req = dlp_request_new(dlpFuncWriteRecordEx, 1, 12 + length);
		if (req == NULL)
			return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

		set_byte(DLP_REQUEST_DATA(req, 0, 0), dbhandle);
		set_byte(DLP_REQUEST_DATA(req, 0, 1), 0x80);	/* "data included" */
		set_long(DLP_REQUEST_DATA(req, 0, 2), recID);
		set_byte(DLP_REQUEST_DATA(req, 0, 6), flags);
		set_byte(DLP_REQUEST_DATA(req, 0, 7), catID);
		set_long(DLP_REQUEST_DATA(req, 0, 8), 0);

		memcpy(DLP_REQUEST_DATA(req, 0, 12), data, length);
	} else {
		if ((length + 8) > DLP_BUF_SIZE) {
			LOG((PI_DBG_DLP, PI_DBG_LVL_ERR,
			     "DLP WriteRecord: data too large (>64k)"));
			return PI_ERR_DLP_DATASIZE;
		}

		req = dlp_request_new(dlpFuncWriteRecord, 1, 8 + length);
		if (req == NULL)
			return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

		set_byte(DLP_REQUEST_DATA(req, 0, 0), dbhandle);
		set_byte(DLP_REQUEST_DATA(req, 0, 1), 0x80);	/* "data included" */
		set_long(DLP_REQUEST_DATA(req, 0, 2), recID);
		set_byte(DLP_REQUEST_DATA(req, 0, 6), flags);
		set_byte(DLP_REQUEST_DATA(req, 0, 7), catID);

		memcpy(DLP_REQUEST_DATA(req, 0, 8), data, length);
	}

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);

	if (result > 0) {
		if (pNewRecID)
			*pNewRecID = get_long(DLP_RESPONSE_DATA(res, 0, 0));

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP WriteRecord Record ID: 0x%8.8lX\n",
		    get_long(DLP_RESPONSE_DATA(res, 0, 0))));

		CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG,
			record_dump(
				get_long(DLP_RESPONSE_DATA(res, 0, 0)), /* recID */
				0xffff,									/* index */
				flags,
				catID,
				data, (int)length));
	}
	
	dlp_response_free(res);
	
	return result;
}


/***************************************************************************
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
 ***************************************************************************/
int
dlp_DeleteRecord(int sd, int dbhandle, int all, recordid_t recID)
{
	int 	result,
		flags = all ? 0x80 : 0;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(dlp_DeleteRecord);
	pi_reset_errors(sd);

	req = dlp_request_new(dlpFuncDeleteRecord, 1, 6);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), dbhandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), flags);
	set_long(DLP_REQUEST_DATA(req, 0, 2), recID);

	result = dlp_exec(sd, req, &res);
	
	dlp_request_free(req);
	dlp_response_free(res);
	
	return result;
}

/***************************************************************************
 *
 * Function:    dlp_DeleteCategory
 *
 * Summary:     Delete all records in a category. The category name is not
 *		changed.
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***************************************************************************/
int
dlp_DeleteCategory(int sd, int dbhandle, int category)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(dlp_DeleteCategory);
	pi_reset_errors(sd);

	if (pi_version(sd) < 0x0101) {
		/* Emulate if not connected to PalmOS 2.0 */
		int i, cat, attr;
		recordid_t id_;

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP DeleteCategory Emulating with: Handle: %d, "
			"Category: %d\n",
		    dbhandle, category & 0xff));

		for (i = 0;
		     (result = dlp_ReadRecordByIndex(sd, dbhandle, i, NULL, &id_,
					   &attr, &cat)) >= 0; i++) {
			if (cat != category
				|| (attr & dlpRecAttrDeleted)
			    || (attr & dlpRecAttrArchived))
				continue;
			result = dlp_DeleteRecord(sd, dbhandle, 0, id_);
			if (result < 0)
				break;
			i--; /* Sigh, deleting record moves it to the end. */
		}

		return result;
	} else {
		int flags = 0x40;
		
		req = dlp_request_new(dlpFuncDeleteRecord, 1, 6);
		if (req == NULL)
			return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);
		
		set_byte(DLP_REQUEST_DATA(req, 0, 0), dbhandle);
		set_byte(DLP_REQUEST_DATA(req, 0, 1), flags);
		set_long(DLP_REQUEST_DATA(req, 0, 2), category & 0xff);
		
		result = dlp_exec(sd, req, &res);

		dlp_request_free(req);
		dlp_response_free(res);

		return result;
	}
}


/***************************************************************************
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
 ***************************************************************************/
int
dlp_ReadResourceByType(int sd, int fHandle, unsigned long type, int id_,
		       pi_buffer_t *buffer, int *resindex)
{
	int 	result,
		data_len;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(dlp_ReadResourceByType);
	pi_reset_errors(sd);

	req = dlp_request_new_with_argid(dlpFuncReadResource, 0x21, 1, 12);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
	set_long(DLP_REQUEST_DATA(req, 0, 2), type);
	set_short(DLP_REQUEST_DATA(req, 0, 6), id_);
	set_short(DLP_REQUEST_DATA(req, 0, 8), 0);
	set_short(DLP_REQUEST_DATA(req, 0, 10), buffer ? DLP_BUF_SIZE : 0);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result > 0) {
		data_len = res->argv[0]->len - 10;
		if (resindex)
			*resindex = get_short(DLP_RESPONSE_DATA(res, 0, 6));
		if (buffer) {
			pi_buffer_clear (buffer);
			pi_buffer_append (buffer, DLP_RESPONSE_DATA(res, 0, 10),
				(size_t)data_len);
		}
		
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadResourceByType  Type: '%s', ID: %d, "
			 "Index: %d, and %d bytes:\n",
		    printlong(type), id_, 
		    get_short(DLP_RESPONSE_DATA(res, 0, 6)),(size_t)data_len));
		CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, 
		      dumpdata(DLP_RESPONSE_DATA(res, 0, 10),(size_t)data_len));
	} else {
		data_len = result;
	}

	dlp_response_free(res);
	
	return data_len;
}


/***************************************************************************
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
 ***************************************************************************/
int
dlp_ReadResourceByIndex(int sd, int fHandle, int resindex, pi_buffer_t *buffer,
			unsigned long *type, int *id_)
{
	int 	result,
		data_len,
		large = 0;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(dlp_ReadResourceByIndex);
	pi_reset_errors(sd);

	/* TapWave (DLP 1.4) implements a `large' version of dlpFuncReadResource,
	 * which can return resources >64k
	 */
	if (pi_version(sd) >= 0x0104) {
		req = dlp_request_new (dlpFuncReadResourceEx, 1, 12);
		if (req == NULL)
			return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

		set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
		set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
		set_short(DLP_REQUEST_DATA(req, 0, 2), resindex);
		set_long(DLP_REQUEST_DATA(req, 0, 4), 0);
		set_long(DLP_REQUEST_DATA(req, 0, 8), pi_maxrecsize(sd));
		large = 1;
	} else {
		req = dlp_request_new (dlpFuncReadResource, 1, 8);
		if (req == NULL)
			return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

		set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
		set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
		set_short(DLP_REQUEST_DATA(req, 0, 2), resindex);
		set_long(DLP_REQUEST_DATA(req, 0, 4), DLP_BUF_SIZE);
	}

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result > 0) {
		data_len = res->argv[0]->len - (large ? 12 : 10);
		if (type)
			*type = get_long(DLP_RESPONSE_DATA(res, 0, 0));
		if (id_)
			*id_ = get_short(DLP_RESPONSE_DATA(res, 0, 4));
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
		    resindex, data_len));
		CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, 
		      dumpdata(DLP_RESPONSE_DATA(res, 0, (large ? 12 : 10)),
			 (size_t)data_len));
	} else {
		data_len = result;
	}

	dlp_response_free(res);
	
	return data_len;
}


/***************************************************************************
 *
 * Function:    dlp_WriteResource
 *
 * Summary:     Write a resource
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***************************************************************************/
int
dlp_WriteResource(int sd, int dbhandle, unsigned long type, int id_,
		  const void *data, size_t length)
{
	int 	result,
		large = 0;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(dlp_WriteResource);
	pi_reset_errors(sd);

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
	if (req == NULL) {
		LOG((PI_DBG_DLP, PI_DBG_LVL_ERR,
			    "DLP sd:%i large:%i dlp_request_new failed\n",
			    sd, large));
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);
	}

	set_byte(DLP_REQUEST_DATA(req, 0, 0), dbhandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
	set_long(DLP_REQUEST_DATA(req, 0, 2), type);
	set_short(DLP_REQUEST_DATA(req, 0, 6), id_);
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


/***************************************************************************
 *
 * Function:    dlp_DeleteResource
 *
 * Summary:     Delete a single resource from the database or all resources
 *		if the all flag is non-zero
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***************************************************************************/
int
dlp_DeleteResource(int sd, int dbhandle, int all, unsigned long restype,
		   int resID)
{
	int 	result,
		flags = all ? 0x80 : 0;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(dlp_DeleteResource);
	pi_reset_errors(sd);

	req = dlp_request_new(dlpFuncDeleteResource, 1, 8);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), dbhandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), flags);
	set_long(DLP_REQUEST_DATA(req, 0, 2), restype);
	set_short(DLP_REQUEST_DATA(req, 0, 6), resID);

	result = dlp_exec(sd, req, &res);
	
	dlp_request_free(req);
	dlp_response_free(res);
	
	return result;
}


/***************************************************************************
 *
 * Function:    dlp_ReadAppBlock
 *
 * Summary:     Read the AppInfo block that matches the database
 *
 * Parameters:  retbuf is emptied prior to receiving data
 *				reqbytes should be -1 to read from 'offset' 'till the end
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***************************************************************************/
int
dlp_ReadAppBlock(int sd, int fHandle, int offset, int reqbytes, pi_buffer_t *retbuf)
{
	int 	result,
		data_len;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(dlp_ReadAppBlock);
	pi_reset_errors(sd);

	if (retbuf)
		pi_buffer_clear(retbuf);

	req = dlp_request_new(dlpFuncReadAppBlock, 1, 6);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
	set_short(DLP_REQUEST_DATA(req, 0, 2), offset);
	set_short(DLP_REQUEST_DATA(req, 0, 4), reqbytes);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result >= 0) {
		if (result < 2)
			data_len = PI_ERR_DLP_COMMAND;
		else {
			data_len = res->argv[0]->len - 2;
			if (retbuf && data_len)
				pi_buffer_append(retbuf, DLP_RESPONSE_DATA(res, 0, 2),
					(size_t)data_len);
			
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
				"DLP ReadAppBlock %d bytes\n", data_len));
			CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, 
				  dumpdata(DLP_RESPONSE_DATA(res, 0, 2),
				(size_t)data_len));
		}
	} else {
		data_len = result;
	}

	dlp_response_free(res);
	
	return data_len;
}


/***************************************************************************
 *
 * Function:    dlp_WriteAppBlock
 *
 * Summary:     Write the AppInfo block that matches the database
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***************************************************************************/
int
dlp_WriteAppBlock(int sd, int fHandle, const /* @unique@ */ void *data,
      size_t length)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(dlp_WriteAppBlock);
	pi_reset_errors(sd);

	req = dlp_request_new(dlpFuncWriteAppBlock, 1, 4 + length);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);
	
	set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
	set_short(DLP_REQUEST_DATA(req, 0, 2), length);

	if (length + 10 > DLP_BUF_SIZE) {
		LOG((PI_DBG_DLP, PI_DBG_LVL_ERR,
		     "DLP WriteAppBlock: data too large (>64k)"));
		pi_set_error(sd, PI_ERR_DLP_DATASIZE);
		return -131;
	}
	memcpy(DLP_REQUEST_DATA(req, 0, 4), data, length);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	dlp_response_free(res);

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_ReadSortBlock
 *
 * Summary:     Read the SortBlock that matches the database
 *
 * Parameters:  retbuf emptied before receiving data
 *				reqbytes: use -1 to get all data from offset on.
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***************************************************************************/
int
dlp_ReadSortBlock(int sd, int fHandle, int offset, int reqbytes, pi_buffer_t *retbuf)
{
	int 	result,
		data_len;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(dlp_ReadSortBlock);
	pi_reset_errors(sd);

	if (retbuf)
		pi_buffer_clear(retbuf);

	req = dlp_request_new(dlpFuncReadSortBlock, 1, 6);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
	set_short(DLP_REQUEST_DATA(req, 0, 2), offset);
	set_short(DLP_REQUEST_DATA(req, 0, 4), reqbytes);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result >= 0) {
		if (result < 2)
			data_len = PI_ERR_DLP_COMMAND;
		else {
			data_len = res->argv[0]->len - 2;
			if (retbuf)
				pi_buffer_append(retbuf, DLP_RESPONSE_DATA(res, 0, 2), 
					(size_t)data_len);
			
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
				"DLP ReadSortBlock %d bytes\n", data_len));
			CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG, 
				  dumpdata(DLP_RESPONSE_DATA(res, 0, 2),
				(size_t)data_len));
		}
	} else {
		data_len = result;
	}

	dlp_response_free(res);
	
	return data_len;
}


/***************************************************************************
 *
 * Function:    dlp_WriteSortBlock
 *
 * Summary:     Write the SortBlock that matches the database
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***************************************************************************/
int
dlp_WriteSortBlock(int sd, int fHandle, const /* @unique@ */ void *data,
		       size_t length)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(dlp_WriteSortBlock);
	pi_reset_errors(sd);

	req = dlp_request_new(dlpFuncWriteSortBlock, 1, 4 + length);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
	set_short(DLP_REQUEST_DATA(req, 0, 2), length);

	if (length + 10 > DLP_BUF_SIZE) {
		LOG((PI_DBG_DLP, PI_DBG_LVL_ERR,
		     "DLP WriteSortBlock: data too large (>64k)"));
		pi_set_error(sd, PI_ERR_DLP_DATASIZE);
		return -131;
	}
	memcpy(DLP_REQUEST_DATA(req, 0, 4), data, length);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	dlp_response_free(res);

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_CleanUpDatabase
 *
 * Summary:     Deletes all records which are marked as archived or deleted
 *		in the record database
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***************************************************************************/
int
dlp_CleanUpDatabase(int sd, int fHandle)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(dlp_CleanUpDatabase);
	pi_reset_errors(sd);

	req = dlp_request_new(dlpFuncCleanUpDatabase, 1, 1);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);

	result = dlp_exec(sd, req, &res);
	
	dlp_request_free(req);
	dlp_response_free(res);
	
	return result;
}


/***************************************************************************
 *
 * Function:    dlp_ResetSyncFlags
 *
 * Summary:     Clear all the sync flags (modified, deleted, etc) in the
 *		pilot database
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***************************************************************************/
int
dlp_ResetSyncFlags(int sd, int fHandle)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(dpl_ResetSyncFlags);
	pi_reset_errors(sd);

	req = dlp_request_new(dlpFuncResetSyncFlags, 1, 1);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);

	result = dlp_exec(sd, req, &res);
	
	dlp_request_free(req);
	dlp_response_free(res);
	
	return result;
}


/***************************************************************************
 *
 * Function:    dlp_ReadNextRecInCategory
 *
 * Summary:     Iterate through all records in category returning subsequent
 *		records on each call
 *
 * Parameters:  buffer is emptied prior to reading data
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***************************************************************************/
int
dlp_ReadNextRecInCategory(int sd, int fHandle, int incategory,
			  pi_buffer_t *buffer, recordid_t *recuid, int *recindex,
			  int *attr)
{
	int 	result,
		data_len,
		flags;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(dlp_ReadNextRecInCategory);
	pi_reset_errors(sd);

	if (pi_version(sd) < 0x0101) {
		/* Emulate for PalmOS 1.0 */
		int 	cat,
			rec;
		pi_socket_t *ps;

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadNextRecInCategory Emulating with: Handle: %d, "
			 "Category: %d\n",
		    fHandle, incategory));

		if ((ps = find_pi_socket(sd)) == 0) {
			errno = ESRCH;
			return -130;
		}

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
						    recuid, attr, &cat);

			if (rec >= 0) {
				if (recindex)
					*recindex = ps->dlprecord;
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
	}
	
	req = dlp_request_new(dlpFuncReadNextRecInCategory, 1, 2);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), incategory);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);

	if (result > 0) {
		data_len = res->argv[0]->len - 10;
		if (recuid)
			*recuid = get_long(DLP_RESPONSE_DATA(res, 0, 0));
		if (recindex)
			*recindex = get_short(DLP_RESPONSE_DATA(res, 0, 4));
		if (attr)
			*attr = get_byte(DLP_RESPONSE_DATA(res, 0, 8));
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


/***************************************************************************
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
 ***************************************************************************/
int
dlp_ReadAppPreference(int sd, unsigned long creator, int id_, int backup,
		      int maxsize, void *buffer, size_t *size, int *version)
{
	int 	result,
		data_len;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(dlp_ReadAppPreference);
	pi_reset_errors(sd);

	if (pi_version(sd) < 0x0101) {
		/* Emulate on PalmOS 1.0 */
		int 	db;
		pi_buffer_t *buf;

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadAppPreference Emulating with: Creator: '%s', "
			"Id: %d, Size: %d, Backup: %d\n",
		    printlong(creator), id_,
		    buffer ? maxsize : 0, backup ? 0x80 : 0));

		result = dlp_OpenDB(sd, 0, dlpOpenRead, "System Preferences",
			       &db);
		if (result < 0)
			return result;

		buf = pi_buffer_new (1024);
		
		result = dlp_ReadResourceByType(sd, db, creator, id_, buf,NULL);

		if (result < 0) {
			/* have to keep the previous error codes to properly return it */
			int err1 = pi_error(sd);
			int err2 = pi_palmos_error(sd);

			pi_buffer_free (buf);
			if (err1 != PI_ERR_SOCK_DISCONNECTED)
				dlp_CloseDB(sd, db);

			pi_set_error(sd, err1);
			pi_set_palmos_error(sd, err2);
			return result;
		}

		if (size)
			*size = buf->used - 2;

		if (version)
			*version = get_short(buf->data);

		if (result > 2) {
			result -= 2;
			memcpy(buffer, buf->data + 2, (size_t)result);
		} else {
			result = 0;
		}

		pi_buffer_free (buf);
		dlp_CloseDB(sd, db);
		return result;
	}
	
	req = dlp_request_new(dlpFuncReadAppPreference, 1, 10);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_long(DLP_REQUEST_DATA(req, 0, 0), creator);
	set_short(DLP_REQUEST_DATA(req, 0, 4), id_);
	set_short(DLP_REQUEST_DATA(req, 0, 6), buffer ? maxsize : 0);
	set_byte(DLP_REQUEST_DATA(req, 0, 8), backup ? 0x80 : 0);
	set_byte(DLP_REQUEST_DATA(req, 0, 9), 0); /* Reserved */

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result > 0) {
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


/***************************************************************************
 *
 * Function:    dlp_WriteAppPreference
 *
 * Summary:     Write application preference
 *
 * Parameters:  None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***************************************************************************/
int
dlp_WriteAppPreference(int sd, unsigned long creator, int id_, int backup,
		       int version, void *buffer, size_t size)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(dlp_ReadAppPreference);
	pi_reset_errors(sd);

	if (pi_version(sd) < 0x0101) {
		/* Emulate on PalmOS 1.0 */
		int 	db,
			err1,
			err2;

		if ((result = dlp_OpenDB(sd, 0, dlpOpenWrite, "System Preferences",
			       &db)) < 0)
			return result;

		if (buffer && size) {
			unsigned char dlp_buf[DLP_BUF_SIZE];
			memcpy(dlp_buf + 2, buffer, size);
			set_short(dlp_buf, version);
			result = dlp_WriteResource(sd, db, creator, id_, dlp_buf,
						size);
		} else {
			result = dlp_WriteResource(sd, db, creator, id_, NULL,
						0);
		}
		err1 = pi_error(sd);
		err2 = pi_palmos_error(sd);

		if (err1 != PI_ERR_SOCK_DISCONNECTED)
			dlp_CloseDB(sd, db);

		if (result < 0) {
			/* restore previous error after DB close */
			pi_set_error(sd, err1);
			pi_set_palmos_error(sd, err2);
		}
		return result;
	}

	req = dlp_request_new(dlpFuncWriteAppPreference, 1, 12 + size);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_long(DLP_REQUEST_DATA(req, 0, 0), creator);
	set_short(DLP_REQUEST_DATA(req, 0, 4), id_);
	set_short(DLP_REQUEST_DATA(req, 0, 6), version);
	set_short(DLP_REQUEST_DATA(req, 0, 8), size);
	set_byte(DLP_REQUEST_DATA(req, 0, 10), backup ? 0x80 : 0);
	set_byte(DLP_REQUEST_DATA(req, 0, 11), 0); 	/* Reserved */

	if ((size + 12) > DLP_BUF_SIZE) {
		LOG((PI_DBG_DLP, PI_DBG_LVL_ERR,
			 "DLP WriteAppPreferenceV2: data too large (>64k)"));
		return PI_ERR_DLP_DATASIZE;
	}
	memcpy(DLP_REQUEST_DATA(req, 0, 12), buffer, size);

	result = dlp_exec (sd, req, &res);

	dlp_request_free(req);
	dlp_response_free(res);

	return result;
}


/***************************************************************************
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
 *              id_        <-- Record ID of record on palm device.
 *              recindex   <-- Specifies record to get.
 *              size       <-- Size of data returned in buffer.
 *              attr       <-- Attributes from record on palm device.
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***************************************************************************/
int
dlp_ReadNextModifiedRecInCategory(int sd, int fHandle, int incategory,
				  pi_buffer_t *buffer, recordid_t * id_,
				  int *recindex, int *attr)
{
	int 	result,
		data_len;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(dlp_ReadNextModifiedRecInCategory);
	pi_reset_errors(sd);

	if (pi_version(sd) < 0x0101) {
		/* Emulate for PalmOS 1.0 */
		int 	cat;

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		    "DLP ReadNextModifiedRecInCategory"
			" Emulating with: Handle: %d, Category: %d\n",
		    fHandle, incategory));

		do {
			/* Fetch next modified record (in any category) */
			result = dlp_ReadNextModifiedRec(sd, fHandle, buffer,
						    id_, recindex, attr, &cat);

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
		while (result >= 0 && cat != incategory);
		
		return result;
	}

	req = dlp_request_new(dlpFuncReadNextModifiedRecInCategory, 1, 2);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), incategory);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);

	if (result > 0) {
		data_len = res->argv[0]->len - 10;

		if (id_)
			*id_ = get_long(DLP_RESPONSE_DATA(res, 0, 0));
		if (recindex)
			*recindex = get_short(DLP_RESPONSE_DATA(res, 0, 4));
		if (attr)
			*attr = get_byte(DLP_RESPONSE_DATA(res, 0, 8));

		if (buffer) {
			pi_buffer_clear (buffer);
			pi_buffer_append (buffer, DLP_RESPONSE_DATA(res, 0, 10),
				(size_t)data_len);
		}

		CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG,
			 record_dump(
				get_long(DLP_RESPONSE_DATA(res, 0, 0)),			/* recID */
				get_short(DLP_RESPONSE_DATA(res, 0, 4)),		/* index */
				get_byte(DLP_RESPONSE_DATA(res, 0, 8)),			/* flags */
				get_byte(DLP_RESPONSE_DATA(res, 0, 9)),			/* catID */
				DLP_RESPONSE_DATA(res, 0, 10), data_len));
	} else {
		data_len = result;
	}

	dlp_response_free(res);

	return data_len;
}


/***************************************************************************
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
 ***************************************************************************/
int
dlp_ReadNextModifiedRec(int sd, int fHandle, pi_buffer_t *buffer, recordid_t * id_,
			int *recindex, int *attr, int *category)
{
	int 	result,
		data_len;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(dlp_ReadNextModifiedRec);
	pi_reset_errors(sd);
	
	req = dlp_request_new (dlpFuncReadNextModifiedRec, 1, 1);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
	
	result = dlp_exec (sd, req, &res);

	dlp_request_free(req);
	
	if (result >= 0) {
		data_len = res->argv[0]->len -10;
		if (id_)
			*id_ = get_long(DLP_RESPONSE_DATA(res, 0, 0));
		if (recindex)
			*recindex = get_short(DLP_RESPONSE_DATA(res, 0, 4));
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
			 record_dump(
				get_long(DLP_RESPONSE_DATA(res, 0, 0)),			/* recID */
				get_short(DLP_RESPONSE_DATA(res, 0, 4)),		/* index */
				get_byte(DLP_RESPONSE_DATA(res, 0, 8)),			/* flags */
				get_byte(DLP_RESPONSE_DATA(res, 0, 9)),			/* catID */
				DLP_RESPONSE_DATA(res, 0, 10), data_len));
	} else {
		data_len = result;
	}

	dlp_response_free(res);
	
	return data_len;
}


/***************************************************************************
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
 ***************************************************************************/
int
dlp_ReadRecordById(int sd, int fHandle, recordid_t id_, pi_buffer_t *buffer,
		   int *recindex, int *attr, int *category)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(dlp_ReadRecordById);
	pi_reset_errors(sd);

	req = dlp_request_new(dlpFuncReadRecord, 1, 10);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
	set_byte(DLP_REQUEST_DATA(req, 0, 1), 0);
	set_long(DLP_REQUEST_DATA(req, 0, 2), id_); 
	set_short(DLP_REQUEST_DATA(req, 0, 6), 0); /* Offset into record */
	set_short(DLP_REQUEST_DATA(req, 0, 8), buffer ? DLP_BUF_SIZE : 0);	/* length to return */

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result > 0) {
		result = res->argv[0]->len - 10;
		if (recindex)
			*recindex = get_short(DLP_RESPONSE_DATA(res, 0, 4));
		if (attr)
			*attr = get_byte(DLP_RESPONSE_DATA(res, 0, 8));
		if (category)
			*category = get_byte(DLP_RESPONSE_DATA(res, 0, 9));
		if (buffer) {
			pi_buffer_clear (buffer);
			pi_buffer_append (buffer, DLP_RESPONSE_DATA(res, 0, 10),
				(size_t)result);
		}

		CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG,
			 record_dump(
				get_long(DLP_RESPONSE_DATA(res, 0, 0)),			/* recID */
				get_short(DLP_RESPONSE_DATA(res, 0, 4)),		/* index */
				get_byte(DLP_RESPONSE_DATA(res, 0, 8)),			/* flags */
				get_byte(DLP_RESPONSE_DATA(res, 0, 9)),			/* catID */
				DLP_RESPONSE_DATA(res, 0, 10), result));
	}
	
	dlp_response_free(res);
	
	return result;
}


/***************************************************************************
 *
 * Function:    dlp_ReadRecordByIndex
 *
 * Summary:     Searches device database for match on a record by index
 *
 * Parameters:  sd       --> Socket descriptor as returned by pilot_connect().
 *              fHandle  --> Database handle as returned by dlp_OpenDB().
 *              recindex --> Specifies record to get.
 *              buffer   <-- Data from specified record. emptied prior to reading data
 *              recuid   <-- ptr to Record ID of record on palm device (can be NULL).
 *              attr     <-- ptr to Attributes from record on palm device (can be NULL).
 *              category <-- ptr to Category from record on palm device (can be NULL).
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 * Turns this request for a particular record in the database into a
 * low-level dlp request.
 *
 ***************************************************************************/
int
dlp_ReadRecordByIndex(int sd, int fHandle, int recindex, pi_buffer_t *buffer,
	recordid_t * recuid, int *attr, int *category)
{
	int 	result,
		large = 0;
	struct dlpRequest *req;
	struct dlpResponse *res;

	Trace(dlp_ReadRecordByIndex);
	pi_reset_errors(sd);

	/* TapWave (DLP 1.4) implements a `large' version of dlpFuncReadRecord,
	 * which can return records >64k
	 */
	if (pi_version(sd) >= 0x0104) {
		req = dlp_request_new_with_argid(dlpFuncReadRecordEx, 0x21, 1, 12);
		if (req == NULL)
			return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

		set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
		set_byte(DLP_REQUEST_DATA(req, 0, 1), 0x00);
		set_short(DLP_REQUEST_DATA(req, 0, 2), recindex);
		set_long(DLP_REQUEST_DATA(req, 0, 4), 0); /* Offset into record */
		set_long(DLP_REQUEST_DATA(req, 0, 8), pi_maxrecsize(sd));	/* length to return */
		large = 1;
	} else {
		req = dlp_request_new_with_argid(dlpFuncReadRecord, 0x21, 1, 8);
		if (req == NULL)
			return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

		set_byte(DLP_REQUEST_DATA(req, 0, 0), fHandle);
		set_byte(DLP_REQUEST_DATA(req, 0, 1), 0x00);
		set_short(DLP_REQUEST_DATA(req, 0, 2), recindex);
		set_short(DLP_REQUEST_DATA(req, 0, 4), 0); /* Offset into record */
		set_short(DLP_REQUEST_DATA(req, 0, 6), buffer ? DLP_BUF_SIZE : 0);	/* length to return */
	}
	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result > 0) {
		result = res->argv[0]->len - (large ? 14 : 10);
		if (recuid)
			*recuid = get_long(DLP_RESPONSE_DATA(res, 0, 0));
		if (attr)
			*attr = get_byte(DLP_RESPONSE_DATA(res, 0, large ? 12 : 8));
		if (category)
			*category = get_byte(DLP_RESPONSE_DATA(res, 0, large ? 13 : 9));
		if (buffer) {
			pi_buffer_clear (buffer);
			pi_buffer_append (buffer, DLP_RESPONSE_DATA(res, 0, large ? 14 : 10),
				(size_t)result);
		}

		CHECK(PI_DBG_DLP, PI_DBG_LVL_DEBUG,
			 record_dump(
				get_long(DLP_RESPONSE_DATA(res, 0, 0)),			/* recUID */
				get_short(DLP_RESPONSE_DATA(res, 0, 4)),		/* index */
				get_byte(DLP_RESPONSE_DATA(res, 0, large ? 12 : 8)),	/* flags */
				get_byte(DLP_RESPONSE_DATA(res, 0, large ? 13 : 9)),	/* catID */
				DLP_RESPONSE_DATA(res, 0, large ? 14 : 10), result));
	}

	dlp_response_free(res);
	
	return result;
}

/***************************************************************************
 *
 * Function:    dlp_ExpSlotEnumerate
 *
 * Summary:     Get the number of expansion slots on the device Available
 * 		only in version 1.3 and later of the protocol (Palm OS 4.0
 * 		and later)
 *
 * Parameters:  sd		--> Socket descriptor as returned by pilot_connect().
 * 		numSlots	<-> on input, the maximum number of slot refs
 * 				    that can be stored in the slotRefs array. 
 * 				    On output, the number of slots on the device
 * 		slotRefs	<-- an array of int with the ref of each slot
 * 
 * Returns:     negative number on error
 *
 ***************************************************************************/
int
dlp_ExpSlotEnumerate(int sd, int *numSlots, int *slotRefs)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_ExpSlotEnumerate);
	pi_reset_errors(sd);

	req = dlp_request_new(dlpFuncExpSlotEnumerate, 0);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result > 0) {
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

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_ExpCardPresent
 *
 * Summary:     Check whether there is an expansion card in a slot Available
 * 		only in version 1.3 and later of the protocol (Palm OS 4.0
 * 		and later)
 *
 * Parameters:  sd	--> socket descriptor as returned by pilot_connect().
 *		slotRef	--> slot ref as returned by dlp_ExpCardInfo()
 * 
 * Returns:     negative number on error
 *
 ***************************************************************************/
int
dlp_ExpCardPresent(int sd, int slotRef)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	RequireDLPVersion(sd,1,2);
	Trace(dlp_ExpCardPresent);
	pi_reset_errors(sd);

	req = dlp_request_new (dlpFuncExpCardPresent, 1, 2);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_short(DLP_REQUEST_DATA(req, 0, 0), slotRef);

	result = dlp_exec (sd, req, &res);

	dlp_request_free(req);
	dlp_response_free(res);

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_ExpCardInfo
 *
 * Summary:     Return information about one connected expansion card
 *
 * Parameters:  sd		--> socket descriptor 
 * 		slotRef		--> slot ref as returned by dlp_ExpCardInfo()
 *		flags		<-- card capabilities (see ExpansionMgr.h in
 *				    Palm OS headers)
 *		numStrings	<-- number of packed information strings
 *		strings		<-- if strings was not NULL and numStrings>0,
 *				    the function returns a new buffer with the
 *				    packed strings. It is the caller's
 *				    responsibility to free() the buffer.
 * 
 * Returns:     negative number on error
 *
 ***************************************************************************/
int
dlp_ExpCardInfo(int sd, int SlotRef, unsigned long *flags, int *numStrings,
				char **strings)
{
	int result;
	struct dlpRequest* req;
	struct dlpResponse* res;

	RequireDLPVersion(sd,1,2);
	Trace(dlp_ExpCardInfo);
	pi_reset_errors(sd);

	req = dlp_request_new (dlpFuncExpCardInfo, 1, 2);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_short(DLP_REQUEST_DATA(req, 0, 0), SlotRef);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result > 0) {
		*flags = get_long(DLP_RESPONSE_DATA (res, 0, 0));
		*numStrings = get_byte(DLP_RESPONSE_DATA (res, 0, 4));

		if (strings && *numStrings) {
			int i, len, sz = 0;
			char *p = DLP_RESPONSE_DATA (res, 0, 8);

			for (i=0; i < *numStrings; i++, sz+=len, p+=len)
				len = strlen (p) + 1;

			*strings = (char *) malloc ((size_t)sz);
			if (*strings)
				memcpy (*strings, DLP_RESPONSE_DATA (res, 0, 8), (size_t)sz);
			else
				result = pi_set_error(sd, PI_ERR_GENERIC_MEMORY);
		}

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			 "DLP ExpCardInfo flags: 0x%08lx numStrings: %d\n",
			 *flags, *numStrings));
	}

	dlp_response_free(res);

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_VFSGetDefaultDir
 *
 * Summary:	Return the default location of the given volume for files of
 *		a particular type
 *
 * Parameters:  sd		--> socket descriptor
 *		volRefNum	--> volume reference number returned from
 *	    			    dlp_VFSVolumeEnumerate()
 *		type		--> file type. can be either a MIME type (ie
 *		    		    "image/jpeg") or a file extension (ie
 *		                    ".jpg")
 *		dir		<-> a buffer to hold the returned path
 *		len		<-> on input, the maximum size of the dir
 *			            buffer. on output, the effective size
 *				    (counting the nul terminator)
 * 
 * Returns:     negative number on error
 *
 ***************************************************************************/
int
dlp_VFSGetDefaultDir(int sd, int volRefNum, const char *type, char *dir,
	int *len)
{
	int result, buflen;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSGetDefaultDir);
	pi_reset_errors(sd);
	
	req = dlp_request_new(dlpFuncVFSGetDefaultDir,
		1, 2 + (strlen(type) + 1));
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_short(DLP_REQUEST_DATA(req, 0, 0), volRefNum);
	strcpy(DLP_REQUEST_DATA(req, 0, 2), type);
	
	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);
	
	if (result > 0) {
		buflen = get_short(DLP_RESPONSE_DATA (res, 0, 0));
		
		if (*len < buflen + 1)
			result = pi_set_error(sd, PI_ERR_DLP_BUFSIZE);
		else {
			if (buflen)
				strncpy(dir, DLP_RESPONSE_DATA (res, 0, 2), 
					(size_t)buflen);
			else
				dir[0] = '\0';
			
			*len = buflen;

			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
				 "Default dir is %s\n", dir));
		}
	}
	
	dlp_response_free(res);
	
	return result;
}


/***************************************************************************
 *
 * Function:    dlp_VFSImportDatabaseFromFile
 *
 * Summary:     
 *
 * Parameters:  FIXME
 * 
 * Returns:     negative number on error
 *
 ***************************************************************************/
int
dlp_VFSImportDatabaseFromFile(int sd, int volRefNum, const char *path,
	int *cardno, unsigned long *localid)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSImportDatabaseFromFile);
	pi_reset_errors(sd);

	LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		"Import file <%s>%d\n", path));

	req = dlp_request_new(dlpFuncVFSImportDatabaseFromFile,
		1, 2 + (strlen(path) + 1));
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_short(DLP_REQUEST_DATA(req, 0, 0), volRefNum);
	strcpy(DLP_REQUEST_DATA(req, 0, 2), path);

	result = dlp_exec(sd, req, &res);

	dlp_request_free(req);

	if (result > 0) {
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

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_VFSExportDatabaseToFile
 *
 * Summary:     
 *
 * Parameters:  FIXME
 * 
 * Returns:     negative number on error
 *
 ***************************************************************************/
int
dlp_VFSExportDatabaseToFile(int sd, int volRefNum, const char *path, 
	int cardno, unsigned int localid)  
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSExportDatabaseToFile);
	pi_reset_errors(sd);
	
	req = dlp_request_new(dlpFuncVFSExportDatabaseToFile,
		1, 8 + (strlen(path) + 1));
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_short(DLP_REQUEST_DATA(req, 0, 0), volRefNum);
	set_short(DLP_REQUEST_DATA(req, 0, 2), cardno);
	set_long(DLP_REQUEST_DATA(req, 0, 4), localid);
	strcpy(DLP_REQUEST_DATA(req, 0, 8), path);
	
	result = dlp_exec(sd, req, &res);
	
	dlp_request_free(req);
	dlp_response_free(res);

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_VFSFileCreate
 *
 * Summary:     Create a new file
 *
 * Parameters:  sd	        --> socket descriptor
 *		volRefNum       --> volume reference number returned from
 *			            dlp_VFSVolumeEnumerate()
 *		path		--> full path of the file to create
 * 
 * Returns:     negative number on error
 *
 ***************************************************************************/
int
dlp_VFSFileCreate(int sd, int volRefNum, const char *name)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileCreate);
	pi_reset_errors(sd);

	req = dlp_request_new (dlpFuncVFSFileCreate, 1, 2 + (strlen(name) + 1));
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_short (DLP_REQUEST_DATA (req, 0, 0), volRefNum);
	strcpy (DLP_REQUEST_DATA (req, 0, 2), name);

	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);
	dlp_response_free (res);

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_VFSFileOpen
 *
 * Summary:     Open a file or directory and return a FileRef to it
 *
 * Parameters:  sd      	--> socket descriptor
 *		volRefNum       --> volume reference number returned from
 *				    dlp_VFSVolumeEnumerate()
 *		path	        --> access path to the file or directory
 *		openMode	--> open mode, use constants from Palm SDK
 *		fileRef		<-- returned file reference
 * 
 * Returns:     negative number on error
 *
 ***************************************************************************/
int
dlp_VFSFileOpen(int sd, int volRefNum, const char *path, int openMode, 
	    FileRef *fileRef)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		"Open File %s Mode: %x VFSRef 0x%x\n",
		path, openMode,volRefNum));
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileOpen);
	pi_reset_errors(sd);

	req = dlp_request_new (dlpFuncVFSFileOpen, 1, 4 + (strlen (path) + 1));
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_short (DLP_REQUEST_DATA (req, 0, 0), volRefNum);
	set_short (DLP_REQUEST_DATA (req, 0, 2), openMode);
	strcpy (DLP_REQUEST_DATA (req, 0, 4), path);

	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);
	
	if (result > 0) {
		*fileRef = get_long(DLP_RESPONSE_DATA (res, 0, 0));

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		     "OpenFileRef: 0x%x\n", *fileRef));
	}
	
	dlp_response_free(res);

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_VFSFileClose
 *
 * Summary:     Close an open file or directory
 *
 * Parameters:  sd	--> socket descriptor
 *		fileRef	--> file or directory reference obtained from
 *			    dlp_VFSFileOpen()
 * 
 * Returns:     negative number on error
 *
 ***************************************************************************/
int
dlp_VFSFileClose(int sd, FileRef fileRef)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileClose);
	pi_reset_errors(sd);

	req = dlp_request_new (dlpFuncVFSFileClose, 1, 4);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_long (DLP_REQUEST_DATA (req, 0, 0), fileRef);

	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);
	dlp_response_free (res);
	
	LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		"Closed FileRef: %x\n", fileRef));

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_VFSFileWrite
 *
 * Summary:     Write data to an open file
 *
 * Parameters:  sd	--> socket descriptor
 *		fileRef	--> file reference obtained from dlp_VFSFileOpen()
 *		data	--> data buffer to write
 *		len	--> number of bytes to write
 *
 * Returns:     negative number on error, number of bytes written on success
 *
 ***************************************************************************/
int
dlp_VFSFileWrite(int sd, FileRef fileRef, unsigned char *data, size_t len)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res = NULL;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileWrite);
	pi_reset_errors(sd);

	LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		"Write to FileRef: %x bytes %d\n", fileRef, len));
	
	req = dlp_request_new (dlpFuncVFSFileWrite, 1, 8);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_long (DLP_REQUEST_DATA (req, 0, 0), fileRef);
	set_long (DLP_REQUEST_DATA (req, 0, 4), len);

	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);

	if (result >= 0) {
		int bytes = pi_write (sd, data, len);
		result = bytes;
		if (result < (int)len) {
			LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			     "send failed %d\n", result));
		} else {
			dlp_response_free (res);
			res = NULL;

			result = dlp_response_read (&res, sd);

			if (result > 0) {
				pi_set_palmos_error(sd, get_short(DLP_RESPONSE_DATA (res, 0, 2)));
				LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
					"send success (%d) res 0x%04x!\n", len, pi_palmos_error(sd)));
				result = bytes;
			}
		} 
	}

	dlp_response_free (res);

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_VFSFileRead
 *
 * Summary:     Read data from an open file
 *
 * Parameters:  sd	--> socket descriptor
 *		fileRef	--> file reference obtained from dlp_VFSFileOpen()
 *		data	<-> buffer to hold the data, emptied first
 *		len	--> on input, number of bytes to read
 * 
 * Returns:     negative number on error, total number of bytes read otherwise
 *
 ***************************************************************************/
int
dlp_VFSFileRead(int sd, FileRef fileRef, pi_buffer_t *data, size_t len)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	size_t bytes = 0;

	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileRead);
	pi_reset_errors(sd);

	req = dlp_request_new (dlpFuncVFSFileRead, 1, 8);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_long (DLP_REQUEST_DATA (req, 0, 0), fileRef);
	set_long (DLP_REQUEST_DATA (req, 0, 4), len);

	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);

	pi_buffer_clear (data);

	if (result >= 0) {
		do {
			result = pi_read(sd, data, len);
			if (result > 0) {
				len -= result;
				bytes += result;
			}
		} while (result > 0 && len > 0);

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		     "dlp_VFSFileRead: read %u bytes (result=%d)\n",
			(unsigned)bytes, result));

		if (result >= 0)
			result = bytes;
	}

	dlp_response_free(res);

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_VFSFileDelete
 *
 * Summary:     Delete a closed file or directory
 *
 * Parameters:  sd		--> socket descriptor
 *		volRefNum	--> as returned by dlp_VFSVolumeEnumerate()
 *		path		--> complete file access path
 * 
 * Returns:     negative number on error
 * 
 ***************************************************************************/
int
dlp_VFSFileDelete(int sd, int volRefNum, const char *path)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileDelete);
	pi_reset_errors(sd);

	req = dlp_request_new (dlpFuncVFSFileDelete, 1, 2 + (strlen (path) + 1));
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);
	
	set_short (DLP_REQUEST_DATA (req, 0, 0), volRefNum);
	strcpy (DLP_REQUEST_DATA (req, 0, 2), path);
	
	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);
	dlp_response_free (res);

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_VFSFileRename
 *
 * Summary:     Rename an existing file or directory
 *
 * Parameters:  sd		--> socket descriptor
 *		volRefNum	--> as returned by dlp_VFSVolumeEnumerate()
 *		path		--> complete file access path
 *		newname		--> new file name (without the access path)
 * 
 * Returns:     negative number on error
 *
 ***************************************************************************/
int
dlp_VFSFileRename(int sd, int volRefNum, const char *path, 
		      const char *newname)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		"Rename file %s to %s\n", path, newname));
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileRename);
	pi_reset_errors(sd);

	req = dlp_request_new (dlpFuncVFSFileRename,
		1, 4 + (strlen (path) + 1) + (strlen (newname) + 1));
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_short (DLP_REQUEST_DATA (req, 0, 0), volRefNum);
	set_short (DLP_REQUEST_DATA (req, 0, 2), 2);
	strcpy (DLP_REQUEST_DATA (req, 0, 4), path);
	strcpy (DLP_REQUEST_DATA (req, 0, 4 + (strlen(path) + 1)), newname);
	
	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);
	dlp_response_free (res);

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_VFSFileEOF
 *
 * Summary:     Checks whether the current position is at the end of file
 *
 * Parameters:  sd	--> socket descriptor
 *		fileRef	--> file reference obtained from dlp_VFSFileOpen()
 * 
 * Returns:     negative number on error
 *
 ***************************************************************************/
int
dlp_VFSFileEOF(int sd, FileRef fileRef)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileEOF);
	pi_reset_errors(sd);

	req = dlp_request_new (dlpFuncVFSFileEOF, 1, 4);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);
	
	set_long (DLP_REQUEST_DATA (req, 0, 0), fileRef);
	
	result = dlp_exec (sd, req, &res);
	
	dlp_request_free (req);
	dlp_response_free (res);

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_VFSFileTell
 *
 * Summary:     Return the current seek position in an open file
 *
 * Parameters:  sd		--> socket descriptor
 *		fileRef		--> file reference obtained from dlp_VFSFileOpen()
 *		position	<-- current position
 * 
 * Returns:     negative number on error
 *
 ***************************************************************************/
int
dlp_VFSFileTell(int sd, FileRef fileRef,int *position)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileTell);
	pi_reset_errors(sd);

	req = dlp_request_new(dlpFuncVFSFileTell, 1, 4);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_long (DLP_REQUEST_DATA (req, 0, 0), fileRef);

	result = dlp_exec (sd, req, &res);
	
	dlp_request_free (req);

	if (result > 0) {
		*position = get_long (DLP_RESPONSE_DATA (res, 0, 0));
	}
	
	dlp_response_free (res);

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_VFSFileGetAttributes
 *
 * Summary:     Return the attributes of an open file
 *
 * Parameters:  sd	        --> socket descriptor
 *		fileRef 	--> file reference obtained from dlp_VFSFileOpen()
 *		attributes	<-- file attributes (see Palm SDK's VFSMgr.h)
 * 
 * Returns:     negative number on error
 *
 ***************************************************************************/
int
dlp_VFSFileGetAttributes (int sd, FileRef fileRef, unsigned long *attributes)
{
	int	result;
	struct dlpRequest *req; 
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileGetAttributes);
	pi_reset_errors(sd);
	
	req = dlp_request_new (dlpFuncVFSFileGetAttributes, 1, 4);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_long (DLP_REQUEST_DATA (req, 0, 0), fileRef);
	
	result = dlp_exec (sd, req, &res);
	
	dlp_request_free (req);

	if (result > 0) {
		*attributes = get_long (DLP_RESPONSE_DATA (res, 0, 0));
	}

	dlp_response_free(res);	

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_VFSFileSetAttributes
 *
 * Summary:     Change the attributes of an open file
 *
 * Parameters:  sd		--> socket descriptor
 *		fileRef		--> file reference obtained from dlp_VFSFileOpen()
 *		attributes	--> new file attributes (see Palm SDK's VFSMgr.h)
 * 
 * Returns:     negative number on error
 *
 ***************************************************************************/
int
dlp_VFSFileSetAttributes(int sd, FileRef fileRef, unsigned long attributes)
{
	int	result;
	struct dlpRequest *req; 
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileSetAttributes);
	pi_reset_errors(sd);

	req = dlp_request_new (dlpFuncVFSFileSetAttributes, 1, 8);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_long (DLP_REQUEST_DATA (req, 0, 0), fileRef);
	set_long (DLP_REQUEST_DATA (req, 0, 4), attributes);
	
	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);
	dlp_response_free (res);

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_VFSFileGetDate
 *
 * Summary:     Return one of the dates associated with an open file or
 *				directory (creation, modification or last access)
 *
 * Parameters:  sd		--> socket descriptor
 *		fileRef		--> file reference obtained from dlp_VFSFileOpen()
 *		whichDate	--> which date you want (vfsFileDateCreated,
 *				    vfsFileDateModified or vfsFileDateAccessed)
 *		date		<-- the date
 * 
 * Returns:     negative number on error
 *
 ***************************************************************************/
int
dlp_VFSFileGetDate(int sd, FileRef fileRef, int which, time_t *date)
{
	int	result;
	struct dlpRequest *req; 
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileGetDate);
	pi_reset_errors(sd);

	req = dlp_request_new (dlpFuncVFSFileGetDate, 1, 6);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_long (DLP_REQUEST_DATA (req, 0, 0), fileRef);
	set_short (DLP_REQUEST_DATA (req, 0, 4), which);

	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);
	
	if (result > 0) {
		*date = get_long (DLP_RESPONSE_DATA (res, 0, 0)) - 2082852000;
	
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		     "Requested date(%d): %d / %x calc %d / %x\n",which,
		     get_long(DLP_RESPONSE_DATA (res, 0, 0)),
		     get_long(DLP_RESPONSE_DATA (res, 0, 0)),
		     *date,*date));
	}
	
	dlp_response_free (res);

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_VFSFileSetDate
 *
 * Summary:     Change one of the dates for an open file or directory
 *		(created, modified or last access)
 *
 * Parameters:  sd		--> socket descriptor
 *		fileRef		--> file reference obtained from dlp_VFSFileOpen()
 *		whichDate       --> which date you want (vfsFileDateCreated,
 *				    vfsFileDateModified or vfsFileDateAccessed)
 *		date		--> the date
 * 
 * Returns:     negative number on error
 *
 ***************************************************************************/
int
dlp_VFSFileSetDate(int sd, FileRef fileRef, int which, time_t date)
{
	int	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		"Set date(%d): %d / %x calc %d / %x\n", which,
		date, date,
		date + 2082852000,
		date + 2082852000));
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileSetDate);
	pi_reset_errors(sd);

	req = dlp_request_new(dlpFuncVFSFileSetDate, 1, 10);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_long (DLP_REQUEST_DATA(req, 0, 0), fileRef);
	set_short (DLP_REQUEST_DATA(req, 0, 4), which);
	set_long (DLP_REQUEST_DATA(req, 0, 6), date + 2082852000);

	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);
	dlp_response_free (res);	

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_VFSDirCreate
 *
 * Summary:     Create a new directory on a VFS volume
 *
 * Parameters:  sd		--> socket descriptor 
 *		volRefNum	--> as returned by dlp_VFSVolumeEnumerate()
 *		path		--> full path of directory to create
 *
 * Returns:     negative number on error
 *
 ***************************************************************************/
int
dlp_VFSDirCreate(int sd, int volRefNum, const char *path)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSDirCreate);
	pi_reset_errors(sd);

	req = dlp_request_new (dlpFuncVFSDirCreate, 1, 2 + (strlen(path) + 1));
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_short (DLP_REQUEST_DATA (req, 0, 0), volRefNum);
	strcpy (DLP_REQUEST_DATA (req, 0, 2), path);
	
	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);
	dlp_response_free (res);

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_VFSDirEntryEnumerate
 *
 * Summary:     Iterate through the entries in a directory
 *
 * Parameters:  sd		--> socket descriptor 
 *		dirRefNum	--> FileRef obtained from dlp_VFSFileOpen()
 *		dirIterator	<-> iterator we use to navigate in the dirs.
 *		    		    start with vfsIteratorStart
 *		maxDirItems	<-> on input, the max. number of VFSDirInfo
 *				    structs that can be filled at once.  on
 *				    output, the actual number of items
 *		data		<-- "maxDirItems" directory items
 * 
 * Returns:     negative number on error
 *
 ***************************************************************************/
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
	pi_reset_errors(sd);

	req = dlp_request_new (dlpFuncVFSDirEntryEnumerate, 1, 12);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_long (DLP_REQUEST_DATA (req, 0, 0), dirRefNum);
	set_long (DLP_REQUEST_DATA (req, 0, 4), *dirIterator);
	/*  FP: (DLP_BUF_SIZE - 99). this is the max return buffer size that
		we are passing for the device to send its response, but I'm not
		sure whether this is a magic value that shouldn't be changed.
		If DLP_BUF_SIZE changes, the value below may become too large! */
	set_long (DLP_REQUEST_DATA (req, 0, 8), 0xFF9C);

	result = dlp_exec (sd, req, &res);
	
	dlp_request_free (req);
	
	if (result > 0) {
		if (result) {
			*dirIterator = get_long (DLP_RESPONSE_DATA (res, 0, 0));
			entries = get_long (DLP_RESPONSE_DATA (res, 0, 4));
		} else {
			*dirIterator = vfsIteratorStop;
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
					get_long(DLP_RESPONSE_DATA (res, 0, from));

				/* fix for Sony sims (and probably devices too): they return
				   the attributes in the high word of attr instead of the low
				   word. We can safely shift it since the high 16 bits are not
				   used for VFS flags */
				if ((data[at].attr & 0x0000FFFF) == 0 &&
					(data[at].attr & 0xFFFF0000) != 0)
					data[at].attr >>= 16;

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

	return result;  
}


/***************************************************************************
 *
 * Function:    dlp_VFSVolumeFormat
 *
 * Summary:     
 *
 * Parameters:  FIXME
 * 
 * Returns:     negative number on error
 *
 ***************************************************************************/
int
dlp_VFSVolumeFormat(int sd, unsigned char flags,
	int fsLibRef, struct VFSSlotMountParamTag *param)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		"VFSVolumeFormat Ver-check %x != 0101 \n",
			pi_version(sd)));
	
	RequireDLPVersion(sd,1,2);
	Trace(VFSVolumeFormat);
	pi_reset_errors(sd);

	req = dlp_request_new(dlpFuncVFSVolumeFormat, 1, 4);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

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

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_VFSVolumeEnumerate
 *
 * Summary:     Returns a list of connected VFS volumes
 *
 * Parameters:  sd              --> socket descriptor 
 *		numVols		<-> on input, the maximum number of volume
 *				    references that can be returned. On output,
 *				    the actual number of volume references
 *		volRefs		<-- an array of volume references
 * 
 * Returns:     negative number on error
 *
 ***************************************************************************/
int
dlp_VFSVolumeEnumerate(int sd, int *numVols, int *volRefs)
{
	int	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSVolumeEnumerate);
	pi_reset_errors(sd);

	req = dlp_request_new (dlpFuncVFSVolumeEnumerate, 0);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	result = dlp_exec (sd, req, &res);
	
	dlp_request_free (req);

	if (result > 0) {
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
	else
		*numVols = 0;

	dlp_response_free (res);

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_VFSVolumeInfo
 *
 * Summary:     Returns information about one VFS volume
 *
 * Parameters:  sd      	--> socket descriptor 
 *		volRefNum	--> as returned by dlp_VFSVolumeEnumerate()
 *		volInfo		<-- volume information
 * 
 * Returns:     negative number on error
 *
 ***************************************************************************/
int
dlp_VFSVolumeInfo(int sd, int volRefNum, struct VFSInfo *volInfo)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;

	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSVolumeInfo);
	pi_reset_errors(sd);

	req = dlp_request_new (dlpFuncVFSVolumeInfo, 1, 2);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_short (DLP_REQUEST_DATA(req, 0, 0), volRefNum);

	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);
	
	if (result > 0) {
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
	
	return result;
}


/***************************************************************************
 *
 * Function:    dlp_VFSVolumeGetLabel
 *
 * Summary:     Return the label (name) of a VFS volume
 *
 * Parameters:  sd		--> socket descriptor 
 *		volRefNum       --> as returned by dlp_VFSVolumeEnumerate()
 *		len     	<-> on input, the maximum size of the name
 *				    buffer (including the ending nul byte). on
 *				    output, the used size
 *		name		<-- the volume name
 * 
 * Returns:     negative number on error
 *
 ***************************************************************************/
int
dlp_VFSVolumeGetLabel(int sd, int volRefNum, int *len, char *name)
{
	int 	result;
	struct dlpRequest *req; 
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSVolumeGetLabel);
	pi_reset_errors(sd);

	req = dlp_request_new (dlpFuncVFSVolumeGetLabel, 1, 2);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_short (DLP_REQUEST_DATA (req, 0, 0), volRefNum);
	
	result = dlp_exec (sd, req, &res);
	
	dlp_request_free (req);

	if (result > 0) {
		strncpy(name, DLP_RESPONSE_DATA(res, 0, 0),
			 (size_t)(*len - 1));
		*len = strlen(name);

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			 "DLP VFSVolumeGetLabel %s\n", name));
	}
	
	dlp_response_free(res);

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_VFSVolumeSetLabel
 *
 * Summary:     Change the label (name) of a VFS volume
 *
 * Parameters:  sd		--> socket descriptor 
 *		volRefNum       --> as returned by dlp_VFSVolumeEnumerate()
 *		name	        --> the new volume name
 * 
 * Returns:     negative number on error
 *
 ***************************************************************************/
int
dlp_VFSVolumeSetLabel(int sd, int volRefNum, const char *name)
{
	int 	result;
	struct dlpRequest *req; 
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSVolumeSetLabel);
	pi_reset_errors(sd);

	req = dlp_request_new (dlpFuncVFSVolumeSetLabel, 1,
			2 + (strlen(name) + 1));
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_short (DLP_REQUEST_DATA (req, 0, 0), volRefNum);
	strcpy (DLP_REQUEST_DATA (req, 0, 2), name);

	result = dlp_exec (sd, req, &res);

	dlp_response_free (res);
	dlp_request_free (req);

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_VFSVolumeSize
 *
 * Summary:     Return the total and used size of a VFS volume
 *
 * Parameters:  sd		--> socket descriptor 
 *		volRefNum   	--> as returned by dlp_VFSVolumeEnumerate()
 *		volSizeUsed 	<-- number of bytes used on the volume
 *		volSizeTotal	<-- total size of the volume
 * 
 * Returns:     negative number on error
 *
 ***************************************************************************/
int
dlp_VFSVolumeSize(int sd, int volRefNum, long *volSizeUsed,
	long *volSizeTotal)
{
	int 	result;
	struct dlpRequest *req; 
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSVolumeSize);
	pi_reset_errors(sd);

	req = dlp_request_new (dlpFuncVFSVolumeSize, 1, 2);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_short (DLP_REQUEST_DATA (req, 0, 0), volRefNum);

	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);
	
	if (result > 0) {
		*volSizeUsed = get_long (DLP_RESPONSE_DATA (res, 0, 0));
		*volSizeTotal = get_long (DLP_RESPONSE_DATA (res, 0, 4));
	
		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		     "DLP VFS Volume Size total: %d used: %d\n",
		     *volSizeTotal, *volSizeUsed));
	}

	dlp_response_free (res);

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_VFSFileSeek
 *
 * Summary:     Change the current seek position in an open file
 *
 * Parameters:  sd	        --> socket descriptor
 *		fileRef		--> file reference obtained from dlp_VFSFileOpen()
 *		origin		--> seek origin (use vfsOriginXXX from pi-dlp.h)
 *		offset		--> offset relative to the chosen origin
 * 
 * Returns:     negative number on error
 *
 ***************************************************************************/
int
dlp_VFSFileSeek(int sd, FileRef fileRef, int origin, int offset)
{
	int 	result;
	struct dlpRequest *req; 
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileSeek);
	pi_reset_errors(sd);

	LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		"Seek %x to offset %d from origin %d\n",
			fileRef,offset,origin));
	
	req = dlp_request_new (dlpFuncVFSFileSeek, 1, 10);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_long (DLP_REQUEST_DATA (req, 0, 0), fileRef);
	set_short (DLP_REQUEST_DATA (req, 0, 4), origin);
	set_long (DLP_REQUEST_DATA (req, 0, 6), offset); 

	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);
	dlp_response_free (res);

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_VFSFileResize
 *
 * Summary:     Resize an open file
 *
 * Parameters:  sd		--> socket descriptor
 *		fileRef	        --> file reference obtained from dlp_VFSFileOpen()
 *		newSize	        --> the new file size
 * 
 * Returns:     negative number on error
 *
 ***************************************************************************/
int
dlp_VFSFileResize(int sd, FileRef fileRef, int newSize)
{
	int 	result;
	struct dlpRequest *req; 
	struct dlpResponse *res;
	
	LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
		"Resize %x to %d bytes\n", fileRef, newSize));
	
	RequireDLPVersion(sd,1,2);
	Trace(dlp_VFSFileResize);
	pi_reset_errors(sd);

	req = dlp_request_new(dlpFuncVFSFileResize, 1, 8);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_long(DLP_REQUEST_DATA(req, 0, 0), fileRef);
	set_long(DLP_REQUEST_DATA(req, 0, 4), newSize);
	
	result = dlp_exec(sd, req, &res);

	dlp_request_free (req);
	dlp_response_free (res);
	
	return result;
}


/***************************************************************************
 *
 * Function:    dlp_VFSFileSize
 *
 * Summary:     Return the size of an open file
 *
 * Parameters:  sd		--> socket descriptor
 *		fileRef	        --> file reference obtained from dlp_VFSFileOpen()
 *		size	        <-- the size of the file
 * 
 * Returns:     negative number on error
 *
 ***************************************************************************/
int
dlp_VFSFileSize(int sd, FileRef fileRef, int *size)
{
	int 	result;
	struct dlpRequest *req;
	struct dlpResponse *res;
	
	RequireDLPVersion(sd,1,2);
	Trace (dlp_VFSFileSize);
	pi_reset_errors(sd);

	req = dlp_request_new (dlpFuncVFSFileSize, 1, 4);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_long (DLP_REQUEST_DATA (req, 0, 0), fileRef);
	
	result = dlp_exec (sd, req, &res);
	
	dlp_request_free (req);
	
	if (result > 0) {
		*size = get_long (DLP_RESPONSE_DATA (res, 0, 0));

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			"DLP VFS File Size: %d\n", *size));
	}
	
	dlp_response_free (res);

	return result;
}


/***************************************************************************
 *
 * Function:    dlp_ExpSlotMediaType
 *
 * Summary:     Return the type of media supported by an expansion slot (DLP
 *				1.4 only)
 *
 * Parameters:  sd		--> socket descriptor
 *              slotNum		--> slot to query (1...n)
 *              mediaType	<-- media type
 *
 * Returns:     negative number on error
 *
 ***************************************************************************/
int
dlp_ExpSlotMediaType(int sd, int slotNum, unsigned long *mediaType)
{
	int     result;
	struct dlpRequest *req;
	struct dlpResponse *res;
 
	RequireDLPVersion(sd,1, 4);
	Trace (dlp_ExpSlotMediaType);
	pi_reset_errors(sd);

	req = dlp_request_new (dlpFuncExpSlotMediaType, 1, 2);
	if (req == NULL)
		return pi_set_error(sd, PI_ERR_GENERIC_MEMORY);

	set_short (DLP_REQUEST_DATA (req, 0, 0), slotNum);

	result = dlp_exec (sd, req, &res);

	dlp_request_free (req);

	if (result > 0) {
		*mediaType = get_long (DLP_RESPONSE_DATA (res, 0, 0));

		LOG((PI_DBG_DLP, PI_DBG_LVL_INFO,
			"DLP Media Type for slot %d: %4.4s\n", 
			slotNum, mediaType));
	}

	dlp_response_free (res);

	return result;
}



