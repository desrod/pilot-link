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

/* @+matchanyintegral@ 	*/
/* @-predboolint@	*/
/* @-boolops@		*/

#ifdef WIN32
#include <winsock.h>		/* for hton* */
#endif

#include <stdio.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-syspkt.h"

#define get_date(ptr) (dlp_ptohdate((ptr)))

#define set_date(ptr,val) (dlp_htopdate((val),(ptr)))

#define DLP_BUF_SIZE 0xffff
static /* @checked@ */ unsigned char dlp_buf[DLP_BUF_SIZE];
static /* @checked@ */ unsigned char exec_buf[DLP_BUF_SIZE];

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

char *dlp_strerror(int error)
{
	if (error < 0)
		error = -error;
	if ((unsigned int) error > sizeof(dlp_errorlist))
		return "Unknown error";
	else
		return dlp_errorlist[error];
}

int dlp_trace = 0;

#ifndef NO_DLP_TRACE
#define DLP_TRACE

#define Trace(name) \
  if (dlp_trace)    \
    fprintf(stderr, "DLP %d: %s\n", sd, #name);
#define Expect(count)    \
  if (result < count) {  \
    if (result < 0) {    \
      if (dlp_trace)     \
        fprintf(stderr, "Result: Error: %s (%d)\n", dlp_errorlist[-result], result); \
    } else {             \
      if (dlp_trace)     \
        fprintf(stderr, "Result: Read %d bytes, expected at least %d\n", result, count); \
      result = -128;     \
    }                    \
    return result;       \
  } else                 \
    if (dlp_trace)       \
      fprintf(stderr, "Result: No error, %d bytes\n", result);
#endif

#if 0
/***********************************************************************
 *
 * Function:    dlp_buffer
 *
 * Summary:     Code to dynamically allocate buffer
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void *dlp_buffer(int sd, int cmd, int arg, int arglen,
		 struct pi_socket *ps)
{
	unsigned char *buf;

	*ps = pi_find_socket(sd);
	if (*ps)
		return 0;

	len = 50 + msglen;
	if (ps->dlpbuflen < len) {
		ps->dlpbuflen = len + 1024;
		if (ps->dlpbuf)
			ps->dlpbuf = realloc(ps->dlpbuf, ps->dlpbuflen);
		else
			ps->dlpbuf = malloc(ps->dlpbuflen);
	}

	buf = ps->dlpbuf;

	buf[0] = (unsigned char) cmd;
buf[1] = etc.}
#endif

int
dlp_exec(int sd, int cmd, int arg, const unsigned char /* @null@ */ *msg,
	 int msglen, unsigned char
	 /* @out@ 	*/
	 /* @null@ 	*/
	 *result, int maxlen)
 /* @modifies *result, exec_buf;@ 	*/
 /* @-predboolint -boolops@ 		*/
{
	int i;
	int err;

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
 * Parmeters:   None
 *
 * Returns:     time_t struct to mktime
 *
 ***********************************************************************/
static time_t dlp_ptohdate(unsigned const char *data)
{
	struct tm t;

	t.tm_sec = (int) data[6];
	t.tm_min = (int) data[5];
	t.tm_hour = (int) data[4];
	t.tm_mday = (int) data[3];
	t.tm_mon = (int) data[2] - 1;
	t.tm_year = ((data[0] << 8) | data[1]) - 1900;
	t.tm_wday = 0;
	t.tm_yday = 0;
	t.tm_isdst = -1;

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
		return (time_t) 0x83D8FE00;	/* Dec 31, 1903 00:00:00 GMT */

	return mktime(&t);
}

/***********************************************************************
 *
 * Function:    dlp_htopdate
 *
 * Summary:     Convert time_t to Palm format
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void dlp_htopdate(time_t time, unsigned char *data)
{				/* @+ptrnegate@ */
	struct tm *t = localtime(&time);
	int y;

	if (!t)
		abort();

	y = t->tm_year + 1900;

	data[7] = (unsigned char) 0;	/* packing spacer */
	data[6] = (unsigned char) t->tm_sec;
	data[5] = (unsigned char) t->tm_min;
	data[4] = (unsigned char) t->tm_hour;
	data[3] = (unsigned char) t->tm_mday;
	data[2] = (unsigned char) (t->tm_mon + 1);
	data[0] = (unsigned char) ((y >> 8) & 0xff);
	data[1] = (unsigned char) ((y >> 0) & 0xff);

	return;
}

/***********************************************************************
 *
 * Function:    dlp_GetSysDateTime
 *
 * Summary:     DLP 1.0 GetSysDateTime function to get device date 
 *		and time
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int dlp_GetSysDateTime(int sd, time_t * t)
{
	unsigned char buf[8];
	int result;

	Trace(GetSysDateTime);

	result = dlp_exec(sd, 0x13, 0x20, 0, 0, buf, 8);

	Expect(8);

	*t = dlp_ptohdate(buf);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, "   Read: Time: %s", ctime(t));
	}
#endif

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_SetSysDateTime
 *
 * Summary:     DLP 1.0 SetSysDateTime function to set the device date
 *		and time
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int dlp_SetSysDateTime(int sd, time_t time)
{
	unsigned char buf[8];
	int result;

	dlp_htopdate(time, buf);

	Trace(ReadSysInfo);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, "  Wrote: Time: %s", ctime(&time));
	}
#endif

	result = dlp_exec(sd, 0x14, 0x20, buf, 8, 0, 0);

	Expect(0);

	return result;
}


/*  
 *  Begin struct SI
 *
 *  Byte    number		byte(0)
 *  Byte    more		byte(1)
 *  Byte    unused		byte(2)
 *  Byte    count		byte(3)
 *  Byte    totalSize		byte(4)
 *  Byte    cardNo		byte(5)
 *  Word    cardVersion		short(6)
 *  DlpDateTimeType crDate	date(8)
 *  DWord   romSize		long(16)
 *  DWord   ramSize		long(20)
 *  DWord   freeRam		long(24)
 *  Byte    cardNameSize	byte(28)
 *  Byte    cardManufSize	byte(29)
 *  Char[0] cardNameAndManuf	byte(30)
 * 
 *  struct read access 
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
 *  Byte    cardNo		byte(0)
 *  Byte    unused		byte(1)
 * struct write access */
#define SIRequest_cardNo(ptr,val)		set_byte((ptr)+0,(val))
#define SIRequest_unused(ptr,val)		set_byte((ptr)+1,(val))
#define sizeof_SIRequest		(2)
 /* end struct SIRequest */

/***********************************************************************
 *
 * Function:    dlp_ReadStorageInfo
 *
 * Summary:     DLP 1.0 ReadStorageInfo to read ROM/RAM regions
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int dlp_ReadStorageInfo(int sd, int cardno, struct CardInfo *c)
{
	int result;
	int len1, len2;

	SIRequest_cardNo(dlp_buf, cardno);
	SIRequest_unused(dlp_buf, 0);

	Trace(ReadStorageInfo);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Cardno: %d\n", cardno);
	}
#endif

	result = dlp_exec(sd, 0x15, 0x20, dlp_buf, 2, dlp_buf, 256 + 26);

	c->more = 0;

	Expect(sizeof_SI);

	c->more = SI_more(dlp_buf) || (SI_count(dlp_buf) > 1);

	c->card = SI_cardNo(dlp_buf);
	c->version = SI_cardVersion(dlp_buf);
	c->creation = SI_crDate(dlp_buf);
	c->romSize = SI_romSize(dlp_buf);
	c->ramSize = SI_ramSize(dlp_buf);
	c->ramFree = SI_freeRam(dlp_buf);

	len1 = SI_cardNameSize(dlp_buf);
	memcpy(c->name, ptr_SI_cardNameAndManuf(dlp_buf, 0), len1);
	c->name[len1] = '\0';

	len2 = SI_cardManufSize(dlp_buf);
	memcpy(c->manufacturer, ptr_SI_cardNameAndManuf(dlp_buf, len1),
	       len2);
	c->manufacturer[len2] = '\0';

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			"  Read: Cardno: %d, Card Version: %d, Creation time: %s",
			c->card, c->version, ctime(&c->creation));
		fprintf(stderr,
			"        Total ROM: %lu, Total RAM: %lu, Free RAM: %lu\n",
			c->romSize, c->ramSize, c->ramFree);
		fprintf(stderr, "        Card name: '%s'\n", c->name);
		fprintf(stderr, "        Manufacturer name: '%s'\n",
			c->manufacturer);
		fprintf(stderr, "        More: %s\n",
			c->more ? "Yes" : "No");
	}
#endif

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_ReadSysInfo
 *
 * Summary:     Read the System Information (memory, battery, etc.) 
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int dlp_ReadSysInfo(int sd, struct SysInfo *s)
{
	int result;

	Trace(ReadSysInfo);

	result = dlp_exec(sd, 0x12, 0x20, NULL, 0, dlp_buf, 256);

	Expect(10);

	s->romVersion = get_long(dlp_buf);
	s->locale = get_long(dlp_buf + 4);
	/* dlp_buf+8 is a filler byte */
	s->nameLength = get_byte(dlp_buf + 9);
	memcpy(s->name, dlp_buf + 10, s->nameLength);
	s->name[s->nameLength] = '\0';

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			"  Read: ROM Version: 0x%8.8lX, Localization ID: 0x%8.8lX\n",
			(unsigned long) s->romVersion,
			(unsigned long) s->locale);
		fprintf(stderr, "        Name '%s'\n", s->name);
	}
#endif

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_ReadDBList
 *
 * Summary:     Iterate through the list of databases on the Palm
 *
 * Parmeters:   None
 *
 * Returns:	A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int
dlp_ReadDBList(int sd, int cardno, int flags, int start,
	       struct DBInfo *info)
{
	int result;

	dlp_buf[0] = (unsigned char) flags;
	dlp_buf[1] = (unsigned char) cardno;
	set_short(dlp_buf + 2, start);

	Trace(ReadDBList);

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

	result = dlp_exec(sd, 0x16, 0x20, dlp_buf, 4, dlp_buf, 48 + 32);

	info->more = 0;

	Expect(48);

	info->more = get_byte(dlp_buf + 2);
	if (pi_version(sd) > 0x0100)	/* PalmOS 2.0 has additional flag */
		info->miscFlags = get_byte(dlp_buf + 5);
	else
		info->miscFlags = 0;
	info->flags = get_short(dlp_buf + 6);
	info->type = get_long(dlp_buf + 8);
	info->creator = get_long(dlp_buf + 12);
	info->version = get_short(dlp_buf + 16);
	info->modnum = get_long(dlp_buf + 18);
	info->createDate = get_date(dlp_buf + 22);
	info->modifyDate = get_date(dlp_buf + 30);
	info->backupDate = get_date(dlp_buf + 38);
	info->index = get_short(dlp_buf + 46);
	strncpy(info->name, (char *) dlp_buf + 48, 32);
	info->name[32] = '\0';

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
	}
#endif

	return result;
}


/***********************************************************************
 *
 * Function:    dlp_FindDBInfo
 *
 * Summary:     Search for a database on the Palm
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_FindDBInfo(int sd, int cardno, int start, char *dbname,
	       unsigned long type, unsigned long creator,
	       struct DBInfo *info)
{
	int i;

	/* This function does not match any DLP layer function, but is
	   intended as a shortcut for programs looking for databases. It
	   uses a fairly byzantine mechanism for ordering the RAM databases
	   before the ROM ones.  You must feed the "index" slot from the
	   returned info in as start the next time round. */

	if (start < 0x1000) {
		i = start;
		while (dlp_ReadDBList(sd, cardno, 0x80, i, info) > 0) {
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
	while (dlp_ReadDBList(sd, cardno, 0x40, i, info) > 0) {
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

	return 0;
}

/***********************************************************************
 *
 * Function:    dlp_OpenDB
 *
 * Summary:     Open the database for read/write/delete/mode
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int dlp_OpenDB(int sd, int cardno, int mode, char *name, int *dbhandle)
{
	unsigned char handle;
	int result;

	dlp_buf[0] = (unsigned char) cardno;
	dlp_buf[1] = (unsigned char) mode;
	strcpy((char *) dlp_buf + 2, name);

	Trace(OpenDB);

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
	    dlp_exec(sd, 0x17, 0x20, &dlp_buf[0], strlen(name) + 3,
		     &handle, 1);

	Expect(1);

	*dbhandle = (int) handle;

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, "  Read: Handle: %d\n", (int) handle);
	}
#endif

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_DeleteDB
 *
 * Summary:     Delete a given database on the Palm
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int dlp_DeleteDB(int sd, int card, const char *name)
{
	int result;

	dlp_buf[0] = (unsigned char) card;
	dlp_buf[1] = (unsigned char) 0;
	strcpy((char *) dlp_buf + 2, name);

	Trace(DeleteDB);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Cardno: %d, Name: '%s'\n", card,
			name);
	}
#endif

	result =
	    dlp_exec(sd, 0x1A, 0x20, dlp_buf, 2 + strlen(name) + 1, 0, 0);

	Expect(0);

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_CreateDB
 *
 * Summary:     Create a database on the Palm
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_CreateDB(int sd, long creator, long type, int cardno, int flags,
	     int version, const char *name, int *dbhandle)
{
	unsigned char handle;
	int result;

	set_long(dlp_buf, creator);
	set_long(dlp_buf + 4, type);
	set_byte(dlp_buf + 8, cardno);
	set_byte(dlp_buf + 9, 0);
	set_short(dlp_buf + 10, flags);
	set_short(dlp_buf + 12, version);
	strcpy((char *) dlp_buf + 14, name);

	Trace(CreateDB);

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
	    dlp_exec(sd, 0x18, 0x20, dlp_buf, 14 + strlen(name) + 1,
		     &handle, 1);

	Expect(1);

	if (dbhandle)
		*dbhandle = (int) handle;

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, "  Read: Handle: %d\n", (int) handle);
	}
#endif

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_CloseDB
 *
 * Summary:     Close the database opened on the Palm
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int dlp_CloseDB(int sd, int dbhandle)
{
	unsigned char handle = (unsigned char) dbhandle;
	int result;

	Trace(CloseDB);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d\n", dbhandle);
	}
#endif

	result = dlp_exec(sd, 0x19, 0x20, &handle, 1, 0, 0);

	Expect(0);

	return result;
}

int dlp_CloseDB_All(int sd)
{
	int result;

	Trace(CloseDB_all);

	result = dlp_exec(sd, 0x19, 0x21, 0, 0, 0, 0);

	Expect(0);

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_CallApplication
 *
 * Summary:     Call an application entry point via an action code
 *
 * Parmeters:   None
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
	int result;
	int version = pi_version(sd);

	if (version >= 0x0101) {	/* PalmOS 2.0 call encoding */
		set_long(dlp_buf + 0, creator);
		set_long(dlp_buf + 4, type);
		set_short(dlp_buf + 8, action);
		set_long(dlp_buf + 10, length);
		set_long(dlp_buf + 14, 0);
		set_long(dlp_buf + 18, 0);

		if (length + 22 > DLP_BUF_SIZE) {
			fprintf(stderr, "Data too large\n");
			return -131;
		}

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

		result =
		    dlp_exec(sd, 0x28, 0x21, dlp_buf, 22 + length, dlp_buf,
			     0xffff);

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

		return result;

	} else {		/* PalmOS 1.0 call encoding */
		set_long(dlp_buf + 0, creator);
		set_short(dlp_buf + 4, action);
		set_short(dlp_buf + 6, length);
		memcpy(dlp_buf + 6, data, length);

		Trace(CallApplicationV10);

#ifdef DLP_TRACE
		if (dlp_trace) {
			fprintf(stderr,
				" Wrote: Creator: '%s', Action code: %d, and %d bytes of data:\n",
				printlong(creator), action, length);
			dumpdata(data, length);
		}
#endif

		result =
		    dlp_exec(sd, 0x28, 0x20, dlp_buf, 8, dlp_buf, 0xffff);

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

		return result;

	}

}

/***********************************************************************
 *
 * Function:    dlp_ResetSystem
 *
 * Summary:     Reset the Palm
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int dlp_ResetSystem(int sd)
{
	int result;

	Trace(ResetSystem);

	result = dlp_exec(sd, 0x29, 0, 0, 0, 0, 0);

	Expect(0);

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_AddSyncLogEntry
 *
 * Summary:     Add text to the Palm's synchronization log
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int dlp_AddSyncLogEntry(int sd, char *entry)
{
	int result;

	Trace(AddSyncLogEntry);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Entry:\n");
		dumpdata((unsigned char *) entry, strlen(entry));
	}
#endif

	result =
	    dlp_exec(sd, 0x2A, 0x20, (unsigned char *) entry,
		     strlen(entry), 0, 0);

	Expect(0);

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_ReadOpenDBInfo
 *
 * Summary:     Read the number of records in the device database
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int dlp_ReadOpenDBInfo(int sd, int dbhandle, int *records)
{
	unsigned char buf[2];
	int result;

	Trace(ReadOpenDBInfo);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d\n", dbhandle);
	}
#endif

	set_byte(dlp_buf, (unsigned char) dbhandle);
	result = dlp_exec(sd, 0x2B, 0x20, dlp_buf, 1, buf, 2);

	Expect(2);

	if (records)
		*records = get_short(buf);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, "  Read: %d records\n", get_short(buf));
	}
#endif

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_MoveCategory
 *
 * Summary:     Move all records in a category to another category
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int dlp_MoveCategory(int sd, int handle, int fromcat, int tocat)
{
	int result;

	set_byte(dlp_buf + 0, handle);
	set_byte(dlp_buf + 1, fromcat);
	set_byte(dlp_buf + 2, tocat);
	set_byte(dlp_buf + 3, 0);

	Trace(MoveCategory);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d, From: %d, To: %d\n",
			handle, fromcat, tocat);
	}
#endif

	result = dlp_exec(sd, 0x2C, 0x20, dlp_buf, 4, 0, 0);

	Expect(0);

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_OpenConduit
 *
 * Summary:     This command is sent before each conduit is opened
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int dlp_OpenConduit(int sd)
{
	int result;

	Trace(OpenConduit);

	result = dlp_exec(sd, 0x2E, 0, 0, 0, 0, 0);

	Expect(0);

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_EndOfSync
 *
 * Summary:     End the sync with the given status
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int dlp_EndOfSync(int sd, int status)
{
	int result;
	struct pi_socket *ps;

	ps = find_pi_socket(sd);
	if (ps == 0)
		return 1;	/* General system error */

	if ((ps->broken) || (!(ps->connected & 1)) || (ps->connected & 2))
		return 1;	/* Don't end sync on an unavailable socket */

	set_short(dlp_buf, status);

	Trace(EndOfSync);

	result = dlp_exec(sd, 0x2F, 0x20, dlp_buf, 2, 0, 0);

	Expect(0);

	/* Messy code to set end-of-sync flag on socket 
	   so pi_close won't do it for us */
	if (result == 0)
		ps->connected |= 2;

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_AbortSync
 *
 * Summary:     Enters a sync_aborted entry into the log
 *
 * Parmeters:   None
 *
 * Returns:     Return value: A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int dlp_AbortSync(int sd)
{
	struct pi_socket *ps;

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			"DLP %d: AbortSync\nResult: Aborted Sync\n", sd);
	}
#endif

	/* Set end-of-sync flag on socket so pi_close won't do a dlp_EndOfSync */
	if ((ps = find_pi_socket(sd)))
		ps->connected |= 2;

	return pi_close(sd);
}

/***********************************************************************
 *
 * Function:    dlp_WriteUserInfo
 *
 * Summary:     Write user information to the Palm
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int dlp_WriteUserInfo(int sd, struct PilotUser *User)
{
	int result;

	Trace(WriteUserInfo);

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
	    dlp_exec(sd, 0x11, 0x20, dlp_buf,
		     22 + strlen(User->username) + 1, NULL, 0);

	Expect(0);

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_ReadUserInfo
 *
 * Summary:     Read user information from the Palm
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise 
 *
 ***********************************************************************/
int dlp_ReadUserInfo(int sd, struct PilotUser *User)
{
	int result;
	int userlen;

	Trace(ReadUserInfo);

	result = dlp_exec(sd, 0x10, 0x00, NULL, 0, dlp_buf, DLP_BUF_SIZE);

	Expect(30);

	userlen = get_byte(dlp_buf + 28);

	User->userID = get_long(dlp_buf);
	User->viewerID = get_long(dlp_buf + 4);
	User->lastSyncPC = get_long(dlp_buf + 8);
	User->successfulSyncDate = get_date(dlp_buf + 12);
	User->lastSyncDate = get_date(dlp_buf + 20);
	User->passwordLength = get_byte(dlp_buf + 29);
	memcpy(User->username, dlp_buf + 30, userlen);
	User->username[userlen] = '\0';
	memcpy(User->password, dlp_buf + 30 + userlen,
	       User->passwordLength);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			"  Read: UID: 0x%8.8lX, VID: 0x%8.8lX, PCID: 0x%8.8lX\n",
			User->userID, User->viewerID, User->lastSyncPC);
		fprintf(stderr, "        Last sync date: %s",
			ctime(&User->lastSyncDate));
		fprintf(stderr, "        Successful sync date: %s",
			ctime(&User->successfulSyncDate));
		fprintf(stderr, "        User name '%s'", User->username);
		if (User->passwordLength) {
			fprintf(stderr, ", Password of %d bytes:\n",
				User->passwordLength);
			dumpdata((unsigned char *) User->password,
				 User->passwordLength);
		} else
			fprintf(stderr, ", No password\n");
	}
#endif

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_ReadNetSyncInfo
 *
 * Summary:     Read Network HotSync settings from the Palm
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int dlp_ReadNetSyncInfo(int sd, struct NetSyncInfo *i)
{
	int result;
	int p;

	if (pi_version(sd) < 0x0101)
		return -129;	/* This call only functions under PalmOS 2.0 */

	Trace(ReadNetSyncInfo);

	result = dlp_exec(sd, 0x36, 0x20, NULL, 0, dlp_buf, DLP_BUF_SIZE);

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
	}
#endif

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_WriteNetSyncInfo
 *
 * Summary:     Write Network HotSync settings to the Palm
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int dlp_WriteNetSyncInfo(int sd, struct NetSyncInfo *i)
{
	int result;
	int p;

	if (pi_version(sd) < 0x0101)
		return -129;

	Trace(WriteNetSyncInfo);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, "  Wrote: Active: %d\n",
			get_byte(dlp_buf));
		fprintf(stderr,
			"        PC hostname: '%s', address '%s', mask '%s'\n",
			i->hostName, i->hostAddress, i->hostSubnetMask);
	}
#endif

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

	result =
	    dlp_exec(sd, 0x37, 0x20, dlp_buf, p, dlp_buf, DLP_BUF_SIZE);

	Expect(0);

	return result;
}

#ifdef _PILOT_SYSPKT_H
/***********************************************************************
 *
 * Function:    dlp_RPC
 *
 * Summary:     Remote Procedure Calls interface
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int dlp_RPC(int sd, struct RPC_params *p, unsigned long *result)
{
	int i;
	unsigned char *c;

	/* RPC through DLP breaks all the rules and isn't well documented to
	   boot */

	long D0 = 0, A0 = 0;
	int err;

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
 * Function:    dlp_ReadFeature
 *
 * Summary:     Read a feature from Feature Manager on the Palm
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise 
 *
 ***********************************************************************/
int
dlp_ReadFeature(int sd, unsigned long creator, unsigned int num,
		unsigned long *feature)
{
	int result;

	if (pi_version(sd) < 0x0101) {
		struct RPC_params p;

		Trace(ReadFeatureV1);

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

			*feature = 0x12345678;

			PackRPC(&p, 0xA27B, RPC_IntReply,
				RPC_Long(creator),
				RPC_Short((unsigned short) num),
				RPC_LongPtr(feature), RPC_End);

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
		}

		return 0;
	}

	Trace(ReadFeatureV2);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Creator: '%s', Number: %d\n",
			printlong(creator), num);
	}
#endif

	set_long(dlp_buf, creator);
	set_short(dlp_buf + 4, num);

	result =
	    dlp_exec(sd, 0x38, 0x20, dlp_buf, 6, dlp_buf, DLP_BUF_SIZE);

	Expect(4);

	if (feature)
		*feature = (unsigned long) get_long(dlp_buf);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, "  Read: Feature: 0x%8.8lX\n",
			(unsigned long) get_long(dlp_buf));
	}
#endif

	return result;
}
#endif				/* IFDEF _PILOT_SYSPKT_H */

/***********************************************************************
 *
 * Function:    dlp_ResetLastSyncPC
 *
 * Summary:     Reset the LastSyncPC ID so we can start again
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int dlp_ResetLastSyncPC(int sd)
{
	struct PilotUser U;
	int err;

	if ((err = dlp_ReadUserInfo(sd, &U)) < 0)
		return err;
	U.lastSyncPC = 0;
	return dlp_WriteUserInfo(sd, &U);
}

/***********************************************************************
 *
 * Function:    dlp_ResetDBIndex
 *
 * Summary:     Reset the modified records index
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int dlp_ResetDBIndex(int sd, int dbhandle)
{
	int result;
	struct pi_socket *ps;

	if ((ps = find_pi_socket(sd)))
		ps->dlprecord = 0;

	Trace(ResetDBIndex);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d\n", dbhandle);
	}
#endif

	result = dlp_exec(sd, 0x30, 0x20, dlp_buf, 1, NULL, 0);

	Expect(0);

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_ReadRecordIDList
 *
 * Summary:     Read in a list of RecordIDs from the Palm
 *
 * Parmeters:   None
 *
 * Returns:	A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int
dlp_ReadRecordIDList(int sd, int dbhandle, int sort, int start, int max,
		     recordid_t * IDs, int *count)
{
	int result, i, ret;
	unsigned int nbytes;
	unsigned char *p;

	set_byte(dlp_buf, dbhandle);
	set_byte(dlp_buf + 1, sort ? 0x80 : 0);
	set_short(dlp_buf + 2, start);
	set_short(dlp_buf + 4, max);

	Trace(ReadRecordIDList);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			" Wrote: Handle: %d, Sort: %s, Start: %d, Max: %d\n",
			dbhandle, sort ? "Yes" : "No", start, max);
	}
#endif

	nbytes = max * 4 + 2;
	if (nbytes > DLP_BUF_SIZE)
		nbytes = DLP_BUF_SIZE;

	result = dlp_exec(sd, 0x31, 0x20, dlp_buf, 6, dlp_buf, nbytes);

	Expect(2);

	ret = get_short(dlp_buf);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Read: %d IDs:\n", ret);
		dumpdata(dlp_buf + 2, ret * 4);
	}
#endif

	for (i = 0, p = dlp_buf + 2; i < ret; i++, p += 4)
		IDs[i] = get_long(p);

	if (count)
		*count = i;

	return nbytes;
}

/***********************************************************************
 *
 * Function:    dlp_WriteRecord
 *
 * Summary:     Writes a record to database. If recID is 0, the device
 *		will create a new id and the variable the NewID pointer
 *		points to will be set to the new id.
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_WriteRecord(int sd, int dbhandle, int flags, recordid_t recID,
		int catID, void *data, int length, recordid_t * NewID)
{
	unsigned char buf[4];
	int result;

	set_byte(dlp_buf, dbhandle);
	set_byte(dlp_buf + 1, 0);
	set_long(dlp_buf + 2, recID);
	set_byte(dlp_buf + 6, flags);
	set_byte(dlp_buf + 7, catID);

	if (length == -1)
		length = strlen((char *) data) + 1;

	if (length + 8 > DLP_BUF_SIZE) {
		fprintf(stderr, "Data too large\n");
		return -131;
	}

	memcpy(dlp_buf + 8, data, length);

	Trace(WriteRecord);

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

	result = dlp_exec(sd, 0x21, 0x20, dlp_buf, 8 + length, buf, 4);

	Expect(4);

	if (NewID) {
		if (result == 4) {
			*NewID = get_long(buf);	/* New record ID */
		} else {
			*NewID = 0;
		}
	}
#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, "  Read: Record ID: 0x%8.8lX\n",
			(unsigned long) get_long(buf));
	}
#endif

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_DeleteRecord
 *
 * Summary:     Deletes a record from the database or all records if the all
 *		flag is non-zero
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int dlp_DeleteRecord(int sd, int dbhandle, int all, recordid_t recID)
{
	int result;
	int flags = all ? 0x80 : 0;

	set_byte(dlp_buf, dbhandle);
	set_byte(dlp_buf + 1, flags);
	set_long(dlp_buf + 2, recID);

	Trace(DeleteRecord);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			" Wrote: Handle: %d, RecordID: %8.8lX, All: %s\n",
			dbhandle, (unsigned long) recID,
			all ? "Yes" : "No");
	}
#endif

	result = dlp_exec(sd, 0x22, 0x20, dlp_buf, 6, 0, 0);

	Expect(0);

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_DeleteCategory
 *
 * Summary:     Delete all records in a category. The category name 
 *		is not changed.
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int dlp_DeleteCategory(int sd, int dbhandle, int category)
{
	int result;
	int flags = 0x40;

	if (pi_version(sd) < 0x0101) {
		/* Emulate if not connected to PalmOS 2.0 */
		int i, r, cat, attr;
		recordid_t id;

		Trace(DeleteCategoryV1);

#ifdef DLP_TRACE
		if (dlp_trace) {
			fprintf(stderr,
				" Emulating with: Handle: %d, Category: %d\n",
				dbhandle, category & 0xff);
		}
#endif

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
	}

	set_byte(dlp_buf, dbhandle);
	set_byte(dlp_buf + 1, flags);
	set_long(dlp_buf + 2, category & 0xff);

	Trace(DeleteCategoryV2);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d, Category: %d\n",
			dbhandle, category & 0xff);
	}
#endif

	result = dlp_exec(sd, 0x22, 0x20, dlp_buf, 6, 0, 0);

	Expect(0);

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_ReadResourceByType
 *
 * Summary:     Read the record resources by ResourceID
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int
dlp_ReadResourceByType(int sd, int fHandle, unsigned long type, int id,
		       void *buffer, int *index, int *size)
{
	int result;

	set_byte(dlp_buf, fHandle);
	set_byte(dlp_buf + 1, 0x00);
	set_long(dlp_buf + 2, type);
	set_short(dlp_buf + 6, id);
	set_short(dlp_buf + 8, 0);				/* Offset into record 	*/
	set_short(dlp_buf + 10, buffer ? DLP_BUF_SIZE : 0);	/* length to return 	*/

	Trace(ReadResourceByType);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d, Type: '%s', ID: %d\n",
			fHandle, printlong(type), id);
	}
#endif

	result =
	    dlp_exec(sd, 0x23, 0x21, dlp_buf, 12, dlp_buf, DLP_BUF_SIZE);

	Expect(10);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			"  Read: Type: '%s', ID: %d, Index: %d, and %d bytes:\n",
			printlong(type), id, get_short(dlp_buf + 6),
			result - 10);
		dumpdata(dlp_buf + 10, result - 10);
	}
#endif

	if (index)
		*index = get_short(dlp_buf + 6);
	if (size)
		*size = get_short(dlp_buf + 8);
	if (buffer)
		memcpy(buffer, dlp_buf + 10, result - 10);

	return result - 10;
}

/***********************************************************************
 *
 * Function:    dlp_ReadResourceByIndex
 *
 * Summary:     Read the record resources by index
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int
dlp_ReadResourceByIndex(int sd, int fHandle, int index, void *buffer,
			unsigned long *type, int *id, int *size)
{
	int result;

	set_byte(dlp_buf, fHandle);
	set_byte(dlp_buf + 1, 0x00);
	set_short(dlp_buf + 2, index);
	set_short(dlp_buf + 4, 0);				/* Offset into record 	*/
	set_short(dlp_buf + 6, buffer ? DLP_BUF_SIZE : 0);	/* length to return 	*/

	Trace(ReadResourceByIndex);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d, Index: %d\n", fHandle,
			index);
	}
#endif

	result =
	    dlp_exec(sd, 0x23, 0x20, dlp_buf, 8, dlp_buf, DLP_BUF_SIZE);

	Expect(10);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			"  Read: Type: '%s', ID: %d, Index: %d, and %d bytes:\n",
			printlong(get_long(dlp_buf)),
			get_short(dlp_buf + 4), index, result - 10);
		dumpdata(dlp_buf + 10, result - 10);
	}
#endif

	if (type)
		*type = get_long(dlp_buf);
	if (id)
		*id = get_short(dlp_buf + 4);
	if (size)
		*size = get_short(dlp_buf + 8);
	if (buffer)
		memcpy(buffer, dlp_buf + 10, result - 10);

	return result - 10;
}

/***********************************************************************
 *
 * Function:    dlp_WriteResource
 *
 * Summary:     Write a resource
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_WriteResource(int sd, int dbhandle, unsigned long type, int id,
		  const void *data, int length)
{
	int result;

	set_byte(dlp_buf, dbhandle);
	set_byte(dlp_buf + 1, 0);
	set_long(dlp_buf + 2, type);
	set_short(dlp_buf + 6, id);
	set_short(dlp_buf + 8, length);

	if (length + 10 > DLP_BUF_SIZE) {
		fprintf(stderr, "Data too large\n");
		return -131;
	}

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

	result = dlp_exec(sd, 0x24, 0x20, dlp_buf, 10 + length, NULL, 0);

	Expect(0);

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_DeleteResource
 *
 * Summary:     Delete a single resource from the database or all 
 *		resources if the all flag is non-zero
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_DeleteResource(int sd, int dbhandle, int all, unsigned long restype,
		   int resID)
{
	int result;
	int flags = all ? 0x80 : 0;

	set_byte(dlp_buf, dbhandle);
	set_byte(dlp_buf + 1, flags);
	set_long(dlp_buf + 2, restype);
	set_short(dlp_buf + 6, resID);

	Trace(DeleteResource);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Type: '%s', ID: %d, All: %s\n",
			printlong(restype), resID, all ? "Yes" : "No");
	}
#endif

	result = dlp_exec(sd, 0x25, 0x20, dlp_buf, 8, NULL, 0);

	Expect(0);

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_ReadAppBlock
 *
 * Summary:     Read the AppInfo block that matches the database
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int dlp_ReadAppBlock(int sd, int fHandle, int offset, void *dbuf, int dlen)
{
	int result;

	set_byte(dlp_buf, fHandle);
	set_byte(dlp_buf + 1, 0x00);
	set_short(dlp_buf + 2, offset);
	set_short(dlp_buf + 4, dlen);

	Trace(ReadAppBlock);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			" Wrote: Handle: %d, Offset: %d, Max Length: %d\n",
			fHandle, offset, dlen);
	}
#endif

	result =
	    dlp_exec(sd, 0x1b, 0x20, dlp_buf, 6, dlp_buf, DLP_BUF_SIZE);

	Expect(2);

	if (dbuf)
		memcpy(dbuf, dlp_buf + 2, result - 2);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, "  Read: %d bytes:\n", result - 2);
		dumpdata(dlp_buf + 2, result - 2);
	}
#endif

	return result - 2;
}

/***********************************************************************
 *
 * Function:    dlp_WriteAppBlock
 *
 * Summary:     Write the AppInfo block that matches the database
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int dlp_WriteAppBlock(int sd, int fHandle, const /* @unique@ */ void *data,
		      int length)
{
	int result;

	set_byte(dlp_buf, fHandle);
	set_byte(dlp_buf + 1, 0x00);
	set_short(dlp_buf + 2, length);

	if (length + 4 > DLP_BUF_SIZE) {
		fprintf(stderr, "Data too large\n");
		return -131;
	}

	memcpy(dlp_buf + 4, data, length);

	Trace(WriteAppBlock);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d, %d bytes:\n", fHandle,
			length);
		dumpdata(data, length);
	}
#endif

	result = dlp_exec(sd, 0x1c, 0x20, dlp_buf, length + 4, NULL, 0);

	Expect(0);

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_ReadSortBlock
 *
 * Summary:     Read the SortBlock that matches the database
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int
dlp_ReadSortBlock(int sd, int fHandle, int offset, void *dbuf, int dlen)
{
	int result;

	set_byte(dlp_buf, fHandle);
	set_byte(dlp_buf + 1, 0x00);
	set_short(dlp_buf + 2, offset);
	set_short(dlp_buf + 4, dlen);

	Trace(ReadSortBlock);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			" Wrote: Handle: %d, Offset: %d, Max Length: %d\n",
			fHandle, offset, dlen);
	}
#endif

	result =
	    dlp_exec(sd, 0x1d, 0x20, dlp_buf, 6, dlp_buf, DLP_BUF_SIZE);

	Expect(2)
#ifdef DLP_TRACE
	    if (dlp_trace) {
		fprintf(stderr, "  Read: %d bytes:\n", result - 2);
		dumpdata(dlp_buf + 2, result - 2);
	}
#endif

	if (dbuf)
		memcpy(dbuf, dlp_buf + 2, result - 2);
	return result - 2;
}

/***********************************************************************
 *
 * Function:    dlp_WriteSortBlock
 *
 * Summary:     Write the SortBlock that matches the database
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int dlp_WriteSortBlock(int sd, int fHandle, const /* @unique@ */ void *data,
		       int length)
{
	int result;

	set_byte(dlp_buf, fHandle);
	set_byte(dlp_buf + 1, 0x00);
	set_short(dlp_buf + 2, length);

	if (length + 4 > DLP_BUF_SIZE) {
		fprintf(stderr, "Data too large\n");
		return -131;
	}

	memcpy(dlp_buf + 4, data, length);

	Trace(WriteSortBlock);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d, %d bytes:\n", fHandle,
			length);
		dumpdata(data, length);
	}
#endif

	result = dlp_exec(sd, 0x1e, 0x20, dlp_buf, length + 4, NULL, 0);

	Expect(0);

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_CleanUpDatabase
 *
 * Summary:     Deletes all records which are marked as archived or 
 *		deleted in the record database
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int dlp_CleanUpDatabase(int sd, int fHandle)
{
	int result;
	unsigned char handle = fHandle;

	Trace(CleanUpDatabase);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d\n", fHandle);
	}
#endif

	result = dlp_exec(sd, 0x26, 0x20, &handle, 1, NULL, 0);

	Expect(0);

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_ResetSyncFlags
 *
 * Summary:     Clear all the sync flags (modified, deleted, etc) in 
 *		the pilot database
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int dlp_ResetSyncFlags(int sd, int fHandle)
{
	int result;
	unsigned char handle = fHandle;

	Trace(ResetSyncFlags);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d\n", fHandle);
	}
#endif

	result = dlp_exec(sd, 0x27, 0x20, &handle, 1, NULL, 0);

	Expect(0);

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_ReadNextRecInCategory
 *
 * Summary:     Iterate through all records in category returning 
 *		subsequent records on each call
 *
 * Parmeters:   None
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
	int result;

#ifdef DLP_TRACE
	int flags;
#endif

	if (pi_version(sd) < 0x0101) {
		/* Emulate for PalmOS 1.0 */
		int cat;
		int r;
		struct pi_socket *ps;

		Trace(ReadNextRecInCategoryV1);

#ifdef DLP_TRACE
		if (dlp_trace) {
			fprintf(stderr,
				" Emulating with: Handle: %d, Category: %d\n",
				fHandle, incategory);
		}
#endif

		if ((ps = find_pi_socket(sd)) == 0)
			return -130;

		for (;;) {
			/* Fetch next modified record (in any category) */
			r = dlp_ReadRecordByIndex(sd, fHandle,
						  ps->dlprecord, 0, 0, 0,
						  0, &cat);

			if (r < 0)
				break;

			if (cat != incategory) {
				ps->dlprecord++;
				continue;
			}

			r = dlp_ReadRecordByIndex(sd, fHandle,
						  ps->dlprecord, buffer,
						  id, size, attr, &cat);

			if (r >= 0) {
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

		return r;
	}

	Trace(ReadNextRecInCategoryV2);

	set_byte(dlp_buf, fHandle);
	set_byte(dlp_buf + 1, incategory);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d, Category: %d\n",
			fHandle, incategory);
	}
#endif

	result =
	    dlp_exec(sd, 0x32, 0x20, dlp_buf, 2, dlp_buf, DLP_BUF_SIZE);

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
}

/***********************************************************************
 *
 * Function:    dlp_ReadAppPreference
 *
 * Summary:     Read application preference
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int
dlp_ReadAppPreference(int sd, unsigned long creator, int id, int backup,
		      int maxsize, void *buffer, int *size, int *version)
{
	int result;

	if (pi_version(sd) < 0x0101) {
		/* Emulate on PalmOS 1.0 */
		int db;
		int r;

		Trace(ReadAppPreferenceV1);

#ifdef DLP_TRACE
		if (dlp_trace) {
			fprintf(stderr,
				"  Emulating with: Creator: '%s', Id: %d, Size: %d, Backup: %d\n",
				printlong(creator), id,
				buffer ? maxsize : 0, backup ? 0x80 : 0);
		}
#endif

		r = dlp_OpenDB(sd, 0, dlpOpenRead, "System Preferences",
			       &db);
		if (r < 0)
			return r;

		r = dlp_ReadResourceByType(sd, db, creator, id, buffer,
					   NULL, size);

		if (r < 0) {
			dlp_CloseDB(sd, db);
			return r;
		}

		if (size)
			*size -= 2;

		if (version)
			*version = get_short(buffer);

		if (r > 2) {
			r -= 2;
			memmove(buffer, ((char *) buffer) + 2, r);
		} else {
			r = 0;
		}

		dlp_CloseDB(sd, db);

		return r;
	}

	Trace(ReadAppPreferenceV2);

	set_long(dlp_buf, creator);
	set_short(dlp_buf + 4, id);
	set_short(dlp_buf + 6, buffer ? maxsize : 0);
	set_byte(dlp_buf + 8, backup ? 0x80 : 0);
	set_byte(dlp_buf + 9, 0);	/* Reserved */

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			" Wrote: Creator: '%s', Id: %d, Size: %d, Backup: %d\n",
			printlong(creator), id, buffer ? maxsize : 0,
			backup ? 0x80 : 0);
	}
#endif

	result =
	    dlp_exec(sd, 0x34, 0x20, dlp_buf, 10, dlp_buf, DLP_BUF_SIZE);

	Expect(6);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			" Read: Version: %d, Total size: %d, Read %d bytes:\n",
			get_short(dlp_buf), get_short(dlp_buf + 2),
			get_short(dlp_buf + 4));
		dumpdata(dlp_buf + 6, get_short(dlp_buf + 4));
	}
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
}

/***********************************************************************
 *
 * Function:    dlp_WriteAppPreference
 *
 * Summary:     Write application preference
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, 0 otherwise
 *
 ***********************************************************************/
int
dlp_WriteAppPreference(int sd, unsigned long creator, int id, int backup,
		       int version, void *buffer, int size)
{
	int result;

	if (pi_version(sd) < 0x0101) {
		/* Emulate on PalmOS 1.0 */
		int db;
		int r;

		Trace(WriteAppPreferenceV1);

#ifdef DLP_TRACE
		if (dlp_trace) {
			fprintf(stderr,
				" Wrote: Creator: '%s', Id: %d, Version: %d, Backup: %d, and %d bytes:\n",
				printlong(creator), id, version,
				backup ? 0x80 : 0, size);
			dumpdata(buffer, size);
		}
#endif

		r = dlp_OpenDB(sd, 0, dlpOpenWrite, "System Preferences",
			       &db);
		if (r < 0)
			return r;

		if (buffer && size) {
			memcpy(dlp_buf + 2, buffer, size);
			set_short(dlp_buf, version);
			r = dlp_WriteResource(sd, db, creator, id, dlp_buf,
					      size);
		} else
			r = dlp_WriteResource(sd, db, creator, id, NULL,
					      0);

		dlp_CloseDB(sd, db);

		return r;
	}

	Trace(WriteAppPreferenceV2);

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

	memcpy(dlp_buf + 12, buffer, size);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			" Wrote: Creator: '%s', Id: %d, Version: %d, Backup: %d, and %d bytes:\n",
			printlong(creator), id, version, backup ? 0x80 : 0,
			size);
		dumpdata(buffer, size);
	}
#endif

	result = dlp_exec(sd, 0x35, 0x20, dlp_buf, 12 + size, NULL, 0);

	Expect(0);

	return result;
}

/***********************************************************************
 *
 * Function:    dlp_ReadNextModifiedRecInCategory
 *
 * Summary:     Iterate through modified records in category, returning 
 *		subsequent modifieded records on each call
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int
dlp_ReadNextModifiedRecInCategory(int sd, int fHandle, int incategory,
				  void *buffer, recordid_t * id,
				  int *index, int *size, int *attr)
{
	int result;

#ifdef DLP_TRACE
	int flags;
#endif

	if (pi_version(sd) < 0x0101) {
		/* Emulate for PalmOS 1.0 */
		int cat;
		int r;

		Trace(ReadNextModifiedRecInCategoryV1);

#ifdef DLP_TRACE
		if (dlp_trace) {
			fprintf(stderr,
				" Emulating with: Handle: %d, Category: %d\n",
				fHandle, incategory);
		}
#endif

		do {
			/* Fetch next modified record (in any category) */
			r = dlp_ReadNextModifiedRec(sd, fHandle, buffer,
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
		while ((r >= 0) && (cat != incategory));

		return r;
	}

	Trace(ReadNextModifiedRecInCategoryV2);

	set_byte(dlp_buf, fHandle);
	set_byte(dlp_buf + 1, incategory);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d, Category: %d\n",
			fHandle, incategory);
	}
#endif

	result =
	    dlp_exec(sd, 0x33, 0x20, dlp_buf, 2, dlp_buf, DLP_BUF_SIZE);

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
}

/***********************************************************************
 *
 * Function:    dlp_ReadNextModifiedRec
 *
 * Summary:     Iterate through modified records in category, returning
 *		subsequent modified records on each call
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int
dlp_ReadNextModifiedRec(int sd, int fHandle, void *buffer, recordid_t * id,
			int *index, int *size, int *attr, int *category)
{
	unsigned char handle = fHandle;
	int result;

#ifdef DLP_TRACE
	int flags;
#endif

	Trace(ReadNextModifiedRec);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d\n", fHandle);
	}
#endif

	result =
	    dlp_exec(sd, 0x1f, 0x20, &handle, 1, dlp_buf, DLP_BUF_SIZE);

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
	if (category)
		*category = get_byte(dlp_buf + 9);
	if (buffer)
		memcpy(buffer, dlp_buf + 10, result - 10);

	return result - 10;
}

/***********************************************************************
 *
 * Function:    dlp_ReadRecordById
 *
 * Summary:     Searches device database for match on a record by id
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int
dlp_ReadRecordById(int sd, int fHandle, recordid_t id, void *buffer,
		   int *index, int *size, int *attr, int *category)
{
	int result;

#ifdef DLP_TRACE
	int flags;
#endif

	set_byte(dlp_buf, fHandle);
	set_byte(dlp_buf + 1, 0x00);
	set_long(dlp_buf + 2, id);
	set_short(dlp_buf + 6, 0);	/* Offset into record */
	set_short(dlp_buf + 8, buffer ? DLP_BUF_SIZE : 0);	/* length to return */

	Trace(ReadRecordById);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr,
			" Wrote: Handle: %d, Record ID: 0x%8.8lX\n",
			fHandle, id);
	}
#endif

	result =
	    dlp_exec(sd, 0x20, 0x20, dlp_buf, 10, dlp_buf, DLP_BUF_SIZE);

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
}

/***********************************************************************
 *
 * Function:    dlp_ReadRecordByIndex
 *
 * Summary:     Searches device database for match on a record by index
 *
 * Parmeters:   None
 *
 * Returns:     A negative number on error, the number of bytes read
 *		otherwise
 *
 ***********************************************************************/
int
dlp_ReadRecordByIndex(int sd, int fHandle, int index, void *buffer,
		      recordid_t * id, int *size, int *attr, int *category)
{
	int result;

#ifdef DLP_TRACE
	int flags;
#endif

	set_byte(dlp_buf, fHandle);
	set_byte(dlp_buf + 1, 0x00);
	set_short(dlp_buf + 2, index);
	set_short(dlp_buf + 4, 0);	/* Offset into record */
	set_short(dlp_buf + 6, buffer ? DLP_BUF_SIZE : 0);	/* length to return */

	Trace(ReadRecordByIndex);

#ifdef DLP_TRACE
	if (dlp_trace) {
		fprintf(stderr, " Wrote: Handle: %d, Index: %d\n", fHandle,
			index);
	}
#endif

	result =
	    dlp_exec(sd, 0x20, 0x21, dlp_buf, 8, dlp_buf, DLP_BUF_SIZE);

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
	/*get_short(dlp_buf+4) == index */
	if (size)
		*size = get_short(dlp_buf + 6);
	if (attr)
		*attr = get_byte(dlp_buf + 8);
	if (category)
		*category = get_byte(dlp_buf + 9);
	if (buffer)
		memcpy(buffer, dlp_buf + 10, result - 10);

	return result - 10;
}
