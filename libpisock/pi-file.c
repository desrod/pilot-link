/* 
 * Pilot File Interface Library
 * Pace Willisson <pace@blitz.com> December 1996
 * Additions by Kenneth Albanowski
 * Additions by Florent Pillet
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 *
 * the following is extracted from the combined wisdom of
 * PDB by Kevin L. Flynn
 * install-prc by Brian J. Swetland, D. Jeff Dionne and Kenneth Albanowski
 * makedoc7 by Pat Beirne, <patb@corel.com>
 * and the include files from the pilot SDK
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pi-debug.h"
#include "pi-source.h"
#include "pi-file.h"
#include "pi-error.h"

#undef FILEDEBUG
#define pi_mktag(c1,c2,c3,c4) (((c1)<<24)|((c2)<<16)|((c3)<<8)|(c4))

/*
   header:
   32		name
   2		flags
   2		version
   4		creation time
   4 		modification time
   4		backup time
   4		modification number
   4		app info offset 
   4		sort info offset
   4		type
   4		creator
   4		uniq id seed (I think it is just garbage)
   4		next record list id (normally 0, or ptr to extended hdr)
   2		num records for this header

   Hypothetically plus 2 more bytes if an extended or perhaps secondary
   header (not supported) (In practice, this value is never set, instead it
   usually indicates a damaged file.)
 
   if the low bit of attr is on, then next thing is a list of resource entry
   descriptors:
 
   resource entry header
   4		type
   2		id
   4		offset
 
   otherwise, if the low bit of attr is off, the next thing is a list of
   record entry decriptors:
 
   record entry header
   4		offset
   1		record attributes
   3		unique id
 
   then two bytes of unknown purpose, \0\0 seems safe
 
   next, the app_info, if any, then the sort_info, if any
 
   then the space used the data. Every offset is an offset from the
   beginning of the file, and will point until this area. Each block starts
   at the given offset and ends at the beginning of the next block. The last
   block ends at the end of the file.
 */

#define PI_HDR_SIZE 78
#define PI_RESOURCE_ENT_SIZE 10
#define PI_RECORD_ENT_SIZE 8

/* Declare prototypes */
static int pi_file_close_for_write(pi_file_t *pf);
static void pi_file_free(pi_file_t *pf);

/* this seems to work, but what about leap years? */
/*#define PILOT_TIME_DELTA (((unsigned)(1970 - 1904) * 365 * 24 * 60 * 60) + 1450800)*/

/* Exact value of "Jan 1, 1970 0:00:00 GMT" - "Jan 1, 1904 0:00:00 GMT" */
#define PILOT_TIME_DELTA (unsigned)(2082844800)


/* FIXME: These conversion functions apply no timezone correction. UNIX uses
   UTC for time_t's, while the Pilot uses local time for database backup
   time and appointments, etc. It is not particularly simple to convert
   between these in UNIX, especially since the Pilot's local time is
   unknown, and if syncing over political boundries, could easily be
   different then the local time on the UNIX box. Since the Pilot does not
   know what timezone it is in, there is no unambiguous way to correct for
   this.
   
   Worse, the creation date for a program is stored in the local time _of
   the computer which did the final linking of that program_. Again, the
   Pilot does not store the timezone information needed to reconstruct
   where/when this was.
   
   A better immediate tack would be to dissect these into struct tm's, and
   return those.
                                                                     --KJA
   */


/***********************************************************************
 *
 * Function:    pilot_time_to_unix_time
 *
 * Summary:     Convert the Palm time to Unix time
 *
 * Parameters:  palm time
 *
 * Returns:     unix time
 *
 ***********************************************************************/
time_t
pilot_time_to_unix_time(unsigned long raw_time)
{
	return (time_t) (raw_time - PILOT_TIME_DELTA);
}


/***********************************************************************
 *
 * Function:    unix_time_to_pilot_time
 *
 * Summary:     Convert Unix time to Palm time
 *
 * Parameters:  unix time
 *
 * Returns:     palm time
 *
 ***********************************************************************/
unsigned long
unix_time_to_pilot_time(time_t t)
{
	return (unsigned long) ((unsigned long) t + PILOT_TIME_DELTA);
}


/***********************************************************************
 *
 * Function:    pi_file
 *
 * Summary:     Open .prc or .pdb file for reading
 *
 * Parameters:  file name
 *
 * Returns:     pi_file_t *file handle or NULL if failed
 *
 ***********************************************************************/
pi_file_t
*pi_file_open(const char *name)
{
	int 	i,
		file_size;
	
	pi_file_t *pf;
	struct 	DBInfo *ip;
	pi_file_entry_t *entp;
		
	unsigned char buf[PI_HDR_SIZE];
	unsigned char *p;
	off_t offset, app_info_offset = 0, sort_info_offset = 0;

	if ((pf = calloc(1, sizeof (pi_file_t))) == NULL)
		return NULL;

	pf->for_writing = 0;

	if ((pf->f = fopen(name, "rb")) == NULL)
		goto bad;

	fseek(pf->f, 0, SEEK_END);
	file_size = ftell(pf->f);
	fseek(pf->f, 0, SEEK_SET);

	if (fread(buf, PI_HDR_SIZE, 1, pf->f) != (size_t) 1) {
		LOG ((PI_DBG_API, PI_DBG_LVL_ERR,
 		     "FILE OPEN %s: can't read header\n", name));
		goto bad;
	}

	p 	= buf;
	ip 	= &pf->info;

	memcpy(ip->name, p, 32);
	ip->flags 		= get_short(p + 32);
	ip->version 		= get_short(p + 34);
	ip->createDate 		= pilot_time_to_unix_time(get_long(p + 36));
	ip->modifyDate 		= pilot_time_to_unix_time(get_long(p + 40));
	ip->backupDate 		= pilot_time_to_unix_time(get_long(p + 44));
	ip->modnum 		= get_long(p + 48);
	app_info_offset 	= get_long(p + 52);
	sort_info_offset 	= get_long(p + 56);
	ip->type 		= get_long(p + 60);
	ip->creator 		= get_long(p + 64);
	pf->unique_id_seed 	= get_long(p + 68);

	/* record list header */
	pf->next_record_list_id = get_long(p + 72);
	pf->nentries 		= get_short(p + 76);

	LOG ((PI_DBG_API, PI_DBG_LVL_INFO,
	     "FILE OPEN Name: '%s' Flags: 0x%4.4X Version: %d\n",
	     ip->name, ip->flags, ip->version));
	LOG ((PI_DBG_API, PI_DBG_LVL_DEBUG,
	     "  Creation date: %s", ctime(&ip->createDate)));
	LOG ((PI_DBG_API, PI_DBG_LVL_DEBUG,
	     "  Modification date: %s", ctime(&ip->modifyDate)));
	LOG ((PI_DBG_API, PI_DBG_LVL_DEBUG,
	     "  Backup date: %s", ctime(&ip->backupDate)));
	LOG ((PI_DBG_API, PI_DBG_LVL_DEBUG,
	     "  Appinfo Size: %d Sortinfo Size: %d\n",
	     pf->app_info_size, pf->sort_info_size));
	LOG ((PI_DBG_API, PI_DBG_LVL_DEBUG,
	     "  Type: '%s'", printlong(ip->type)));
	LOG ((PI_DBG_API, PI_DBG_LVL_DEBUG,
	     "  Creator: '%s' Seed: 0x%8.8lX\n", printlong(ip->creator),
	     pf->unique_id_seed));

	if (pf->next_record_list_id != 0) {
		LOG ((PI_DBG_API, PI_DBG_LVL_ERR,
 		     "FILE OPEN %s: this file is probably damaged\n", name));
		goto bad;
	}

	if (ip->flags & dlpDBFlagResource) {
		pf->resource_flag = 1;
		pf->ent_hdr_size = PI_RESOURCE_ENT_SIZE;
	} else {
		pf->resource_flag = 0;
		pf->ent_hdr_size = PI_RECORD_ENT_SIZE;
	}

	if (pf->nentries < 0) {
		LOG ((PI_DBG_API, PI_DBG_LVL_ERR,
 		     "FILE OPEN %s: bad header\n", name));
		goto bad;
	}

	offset = file_size;

	if (pf->nentries) {
		if ((pf->entries =
		     calloc((size_t)pf->nentries,
				sizeof *pf->entries)) == NULL)
			goto bad;

		for (i = 0, entp = pf->entries; i < pf->nentries;
		     i++, entp++) {
			if (fread(buf, (size_t) pf->ent_hdr_size, 1, pf->f)
				!= (size_t) 1)
				goto bad;

			p = buf;
			if (pf->resource_flag) {
				entp->type 	= get_long(p);
				entp->id_ 	= get_short(p + 4);
				entp->offset 	= get_long(p + 6);

				LOG ((PI_DBG_API, PI_DBG_LVL_DEBUG,
				     "FILE OPEN Entry %d '%s' #%d @%X\n", i,
				       printlong(entp->type), entp->id_,
				       entp->offset));
			} else {
				entp->offset 	= get_long(p);
				entp->attrs 	= get_byte(p + 4);
				entp->uid 	= get_treble(p + 5);

				LOG ((PI_DBG_API, PI_DBG_LVL_DEBUG,
				 "FILE OPEN Entry %d UID: "
				 "0x%8.8X Attrs: %2.2X Offset: @%X\n", i,
				     (int) entp->uid, entp->attrs,
					 entp->offset));
			}
		}

		for (i = 0, entp = pf->entries + pf->nentries - 1;
		     i < pf->nentries; i++, entp--) {
			entp->size 	= offset - entp->offset;
			offset 		= entp->offset;

			LOG ((PI_DBG_API, PI_DBG_LVL_DEBUG,
			     "FILE OPEN Entry: %d Size: %d\n",
			     pf->nentries - i - 1, entp->size));

			if (entp->size < 0 ||
				(entp->offset + entp->size) > file_size) {
				LOG ((PI_DBG_API, PI_DBG_LVL_DEBUG,
				 "FILE OPEN %s: Entry %d corrupt,"
				 " giving up\n",
					name, pf->nentries - i - 1));
				goto bad;
			}
		}
	}

	if (sort_info_offset) {
		pf->sort_info_size = offset - sort_info_offset;
		offset = sort_info_offset;
	}

	if (app_info_offset) {
		pf->app_info_size = offset - app_info_offset;
		offset = app_info_offset;
	}

	if (pf->app_info_size < 0 ||
		(sort_info_offset + pf->sort_info_size) > file_size ||
		pf->sort_info_size < 0 ||
		(app_info_offset + pf->app_info_size) > file_size) {
		LOG ((PI_DBG_API, PI_DBG_LVL_ERR,
 		     "FILE OPEN %s: bad header "
			 "(app_info @ %d size %d, "
			 "sort_info @ %d size %d)\n", name,
			 app_info_offset, pf->app_info_size,
			 sort_info_offset, pf->sort_info_size));
		goto bad;
	}

	if (pf->app_info_size == 0)
		pf->app_info = NULL;
	else {
		if ((pf->app_info =
			malloc((size_t) pf->app_info_size)) == NULL)
			goto bad;
		fseek(pf->f, (long)app_info_offset, SEEK_SET);
		if (fread(pf->app_info, 1, (size_t) pf->app_info_size, pf->f)
			 != (size_t) pf->app_info_size)
			goto bad;
	}

	if (pf->sort_info_size == 0)
		pf->sort_info = NULL;
	else {
		if ((pf->sort_info = malloc((size_t)pf->sort_info_size))
			 == NULL)
			goto bad;
		fseek(pf->f, (long)sort_info_offset, SEEK_SET);
		if (fread(pf->sort_info, 1, (size_t) pf->sort_info_size,
			 pf->f) != (size_t) pf->sort_info_size)
			goto bad;
	}

	return pf;

bad:
	pi_file_close(pf);
	return NULL;
}


/***********************************************************************
 *
 * Function:    pi_file_close
 *
 * Summary:     Close the open file handle
 *
 * Parameters:  file handle pi_file_t*
 *
 * Returns:     0 for success, negative otherwise
 *
 ***********************************************************************/
int
pi_file_close(pi_file_t *pf)
{
	int 	err;

	if (!pf)
		return -1;

	if (pf->for_writing) {
		if (pi_file_close_for_write(pf) < 0)
			pf->err = 1;
	}

	err = pf->err;

	pi_file_free(pf);

	return err ? -1 : 0;
}


/***********************************************************************
 *
 * Function:    pi_file_free
 *
 * Summary:     Flush and clean the file handles used
 *
 * Parameters:  file handle pi_file_t*
 *
 * Returns:     void
 *
 ***********************************************************************/
static
void pi_file_free(pi_file_t *pf)
{

	ASSERT (pf != NULL);

	if (pf->f != 0)
		fclose(pf->f);
	
	if (pf->app_info != NULL)
		free(pf->app_info);
	
	if (pf->sort_info != NULL)
		free(pf->sort_info);
	
	if (pf->entries != NULL)
		free(pf->entries);
	
	if (pf->file_name != NULL)
		free(pf->file_name);
	
	if (pf->rbuf != NULL)
		free(pf->rbuf);
	
	if (pf->tmpf != 0)
		fclose(pf->tmpf);
	
	free(pf);
}


/***********************************************************************
 *
 * Function:    pi_file_get_info
 *
 * Summary:	retrieve file pi_file info
 *
 * Parameters:  file handle pi_file_t*, DBInfo
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void
pi_file_get_info(pi_file_t *pf, struct DBInfo *infop)
{
	*infop = pf->info;
}


/***********************************************************************
 *
 * Function:    pi_file_get_app_info
 *
 * Summary:	retrieve pi_file app info
 *
 * Parameters:  file handle pi_file_t*, data buffer*, size*
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void
pi_file_get_app_info(pi_file_t *pf, void **datap, size_t *sizep)
{
	*datap = pf->app_info;
	*sizep = pf->app_info_size;
}


/***********************************************************************
 *
 * Function:    pi_file_get_sort_info
 *
 * Summary:	retrieves pi_file sort info
 *
 * Parameters:  file handle pi_file_t*, data*, size*
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void
pi_file_get_sort_info(pi_file_t *pf, void **datap, size_t *sizep)
{
	*datap = pf->sort_info;
	*sizep = pf->sort_info_size;
}


/***********************************************************************
 *
 * Function:    pi_file_set_rbuf_size
 *
 * Summary:	set pi_file rbuf size
 *
 * Parameters:  file handle pi_file_t*, rbuf size
 *
 * Returns:     0 for success, negative otherwise
 *
 ***********************************************************************/
static int
pi_file_set_rbuf_size(pi_file_t *pf, size_t size)
{
	size_t 	new_size;
	void 	*rbuf;

	if (size > (size_t)pf->rbuf_size) {
		if (pf->rbuf_size == 0) {
			new_size = size + 2048;
			rbuf = malloc(new_size);
		} else {
			new_size = size + 2048;
			rbuf = realloc(pf->rbuf, new_size);
		}

		if (rbuf == NULL)
			return PI_ERR_GENERIC_MEMORY;

		pf->rbuf_size = new_size;
		pf->rbuf = rbuf;
	}

	return 0;
}


/***********************************************************************
 *
 * Function:    pi_file_find_resource_by_type_id
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Negative on error, 0 if found
 *
 ***********************************************************************/
static int
pi_file_find_resource_by_type_id(pi_file_t *pf,
				 unsigned long type, int id_, int *idxp)
{
	int 	i;
	struct 	pi_file_entry *entp;

	if (!pf->resource_flag)
		return PI_ERR_FILE_INVALID;

	for (i = 0, entp = pf->entries; i < pf->nentries;
	     i++, entp++) {
		if (entp->type == type && entp->id_ == id_) {
			if (idxp)
				*idxp = i;
			return 0;
		}
	}
	return PI_ERR_FILE_NOT_FOUND;
}


/***********************************************************************
 *
 * Function:    pi_file_read_resource_by_type_id
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Negative on error (ie PI_ERR_FILE_RES_NOT_FOUND)
 *
 ***********************************************************************/
int
pi_file_read_resource_by_type_id(pi_file_t *pf, unsigned long type,
				 int id_, void **bufp, size_t *sizep,
				 int *idxp)
{
	int 	i,
		result;

	result = pi_file_find_resource_by_type_id(pf, type, id_, &i);
	if (result < 0)
		return result;

	if (idxp)
		*idxp = i;
	return pi_file_read_resource(pf, i, bufp, sizep, NULL, NULL);
}


/***********************************************************************
 *
 * Function:    pi_file_type_id_used
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_file_type_id_used(pi_file_t *pf, unsigned long type, int id_)
{
	return pi_file_find_resource_by_type_id(pf, type, id_, NULL) == 0;
}


/*********************************************************************** 
 * 
 * Function:    pi_file_read_resource
 *
 * Summary:     Returned buffer is valid until next call, or until
 *              pi_file_close
 *
 * Parameters:  None
 *
 * Returns:     Negative on error, 0 if no error  
 *
 ***********************************************************************/
int
pi_file_read_resource(pi_file_t *pf, int i,
		      void **bufp, size_t *sizep, unsigned long *type,
		      int *idp)
{
	pi_file_entry_t *entp;
	int result;

	if (pf->for_writing || !pf->resource_flag)
		return PI_ERR_FILE_INVALID;

	if (i < 0 || i >= pf->nentries)
		return PI_ERR_GENERIC_ARGUMENT;

	entp = &pf->entries[i];

	if (bufp) {
		if ((result = pi_file_set_rbuf_size(pf, (size_t) entp->size)) < 0)
			return result;
		fseek(pf->f, pf->entries[i].offset, SEEK_SET);
		if (fread(pf->rbuf, 1, (size_t) entp->size, pf->f) !=
				(size_t) entp->size)
			return PI_ERR_FILE_ERROR;
		*bufp = pf->rbuf;
	}

	if (sizep)
		*sizep = entp->size;
	if (type)
		*type = entp->type;
	if (idp)
		*idp = entp->id_;

	return 0;
}


/***********************************************************************
 *
 * Function:    pi_file_read_record
 *
 * Summary:     Returned buffer is valid until next call, or until
 *              pi_file_close
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_file_read_record(pi_file_t *pf, int i,
		    void **bufp, size_t *sizep, int *attrp, int *catp,
		    pi_uid_t * uidp)
{
	int result;
	pi_file_entry_t *entp;

	if (pf->for_writing || pf->resource_flag)
		return PI_ERR_FILE_INVALID;

	if (i < 0 || i >= pf->nentries)
		return PI_ERR_GENERIC_ARGUMENT;

	entp = &pf->entries[i];

	if (bufp) {
		if ((result = pi_file_set_rbuf_size(pf, (size_t) entp->size)) < 0) {
			LOG((PI_DBG_API, PI_DBG_LVL_ERR,
			    "FILE READ_RECORD Unable to set buffer size!\n"));
			return result;
		}

		fseek(pf->f, pf->entries[i].offset, SEEK_SET);

		if (fread(pf->rbuf, 1, (size_t) entp->size, pf->f) !=
		    (size_t) entp->size) {
			LOG((PI_DBG_API, PI_DBG_LVL_ERR,
			    "FILE READ_RECORD Unable to read record!\n"));
			return PI_ERR_FILE_ERROR;
		}
		
		*bufp = pf->rbuf;
	}

	LOG ((PI_DBG_API, PI_DBG_LVL_INFO,
	     "FILE READ_RECORD Record: %d Bytes: %d\n", i, entp->size));

	if (sizep)
		*sizep = entp->size;
	if (attrp)
		*attrp = entp->attrs & 0xf0;
	if (catp)
		*catp = entp->attrs & 0xf;
	if (uidp)
		*uidp = entp->uid;

	return 0;
}


/***********************************************************************
 *
 * Function:    pi_file_read_record_by_id
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_file_read_record_by_id(pi_file_t *pf, pi_uid_t uid,
			  void **bufp, size_t *sizep, int *idxp, int *attrp,
			  int *catp)
{
	int 	i;
	struct 	pi_file_entry *entp;

	for (i = 0, entp = pf->entries; i < pf->nentries;
	     i++, entp++) {
		if (entp->uid == uid) {
			if (idxp)
				*idxp = i;
			return (pi_file_read_record
				(pf, i, bufp, sizep, attrp, catp, &uid));
		}
	}

	return PI_ERR_FILE_NOT_FOUND;
}


/***********************************************************************
 *
 * Function:    pi_file_id_used
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_file_id_used(pi_file_t *pf, pi_uid_t uid)
{
	int 	i;
	struct 	pi_file_entry *entp;

	for (i = 0, entp = pf->entries; i < pf->nentries; i++, entp++) {
		if (entp->uid == uid)
			return 1;
	}
	return 0;
}


/***********************************************************************
 *
 * Function:    pi_file_create
 *
 * Summary:     
 *
 * Parameters:  None
 *
 * Returns:     NULL on error
 *
 ***********************************************************************/
pi_file_t *
pi_file_create(const char *name, const struct DBInfo *info)
{
	pi_file_t *pf;

	if ((pf = calloc(1, sizeof *pf)) == NULL)
		return NULL;

	if ((pf->file_name = strdup(name)) == NULL)
		goto bad;

	pf->for_writing = 1;
	pf->info = *info;

	if (info->flags & dlpDBFlagResource) {
		pf->resource_flag = 1;
		pf->ent_hdr_size = PI_RESOURCE_ENT_SIZE;
	} else {
		pf->resource_flag = 0;
		pf->ent_hdr_size = PI_RECORD_ENT_SIZE;
	}

	if ((pf->tmpf = tmpfile()) == NULL)
		goto bad;

	return (pf);

bad:
	pi_file_free(pf);
	return (NULL);
}


/***********************************************************************
 *
 * Function:    pi_file_set_info
 *
 * Summary:     May call these any time before close (even multiple
 *              times)
 *
 * Parameters:  None
 *
 * Returns:     Negative on error
 *
 ***********************************************************************/
int
pi_file_set_info(pi_file_t *pf, const struct DBInfo *ip)
{
	if (!pf->for_writing)
		return PI_ERR_FILE_INVALID;

	if ((ip->flags & dlpDBFlagResource) !=
	    (pf->info.flags & dlpDBFlagResource))
		return PI_ERR_FILE_INVALID;

	pf->info = *ip;

	return 0;
}


/***********************************************************************
 *
 * Function:    pi_file_set_app_info
 *
 * Summary:     
 *
 * Parameters:  None
 *
 * Returns:     Negative on error
 *
 ***********************************************************************/
int
pi_file_set_app_info(pi_file_t *pf, void *data, size_t size)
{
	void 	*p;

	if (!size) {
		if (pf->app_info)
			free(pf->app_info);
		pf->app_info_size = 0;
		return 0;
	}

	if ((p = malloc(size)) == NULL)
		return PI_ERR_GENERIC_MEMORY;

	memcpy(p, data, size);

	if (pf->app_info)
		free(pf->app_info);

	pf->app_info = p;
	pf->app_info_size = size;

	return 0;
}


/***********************************************************************
 *
 * Function:    pi_file_set_sort_info
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_file_set_sort_info(pi_file_t *pf, void *data, size_t size)
{
	void 	*p;

	if (!size) {
		if (pf->sort_info)
			free(pf->sort_info);
		pf->sort_info_size = 0;
		return 0;
	}

	if ((p = malloc(size)) == NULL)
		return PI_ERR_GENERIC_MEMORY;

	memcpy(p, data, size);

	if (pf->sort_info)
		free(pf->sort_info);

	pf->sort_info = p;
	pf->sort_info_size = size;

	return 0;
}


/***********************************************************************
 *
 * Function:    pi_file_append_entry
 *
 * Summary:     Internal function to extend entry list if necessary,
 *              and return a pointer to the next available slot
 *
 * Parameters:  None
 *
 * Returns:     NULL on allocation error
 *
 ***********************************************************************/
static pi_file_entry_t
*pi_file_append_entry(pi_file_t *pf)
{
	int 	new_count;
	size_t	new_size;
	struct 	pi_file_entry *new_entries;
	struct 	pi_file_entry *entp;

	if (pf->nentries >= pf->nentries_allocated) {
		if (pf->nentries_allocated == 0)
			new_count = 100;
		else
			new_count = pf->nentries_allocated * 3 / 2;
		new_size = new_count * sizeof *pf->entries;

		if (pf->entries == NULL)
			new_entries = malloc(new_size);
		else
			new_entries = realloc(pf->entries, new_size);

		if (new_entries == NULL)
			return NULL;

		pf->nentries_allocated = new_count;
		pf->entries = new_entries;
	}

	entp = &pf->entries[pf->nentries++];
	memset(entp, 0, sizeof *entp);
	return entp;
}


/***********************************************************************
 *
 * Function:    pi_file_append_resource
 *
 * Summary:
 *
 * Parameters:  None 
 *
 * Returns:     Negative on error, size written on success
 *
 ***********************************************************************/
int
pi_file_append_resource(pi_file_t *pf, void *buf, size_t size,
	unsigned long type, int id_)
{
	pi_file_entry_t *entp;

	if (!pf->for_writing || !pf->resource_flag)
		return PI_ERR_FILE_INVALID;

	entp = pi_file_append_entry(pf);

	if (size && fwrite(buf, size, 1, pf->tmpf) != 1) {
		pf->err = 1;
		return PI_ERR_FILE_ERROR;
	}

	entp->size 	= size;
	entp->type 	= type;
	entp->id_ 	= id_;

	return size;
}


/***********************************************************************
 *
 * Function:    pi_file_append_record  
 *
 * Summary:
 *
 * Parameters:  None 
 *
 * Returns:     Negative on error, size written on success
 *
 ***********************************************************************/
int
pi_file_append_record(pi_file_t *pf, void *buf, size_t size,
	int attrs, int category, pi_uid_t uid)
{
	pi_file_entry_t *entp;

	if (!pf->for_writing || pf->resource_flag)
		return PI_ERR_FILE_INVALID;

	entp = pi_file_append_entry(pf);

	if (size && fwrite(buf, size, 1, pf->tmpf) != 1) {
		pf->err = 1;
		return PI_ERR_FILE_ERROR;
	}

	entp->size 	= size;
	entp->attrs 	= (attrs & 0xf0) | (category & 0xf);
	entp->uid 	= uid;

	return size;
}


/***********************************************************************
 *
 * Function:    pi_file_get_entries
 *
 * Summary:     
 *
 * Parameters:  None   
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void
pi_file_get_entries(pi_file_t *pf, int *entries)
{
	*entries = pf->nentries;
}


/***********************************************************************
 *
 * Function:    pi_file_close_for_write 
 *
 * Summary:     
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int
pi_file_close_for_write(pi_file_t *pf)
{
	int 	i,
		offset,
		c;
	FILE 	*f;
	
	struct 	DBInfo *ip;
	struct 	pi_file_entry *entp;
		
	unsigned char buf[512];
	unsigned char *p;

	ip = &pf->info;
	if (pf->nentries >= 64 * 1024) {
		LOG((PI_DBG_API, PI_DBG_LVL_ERR,
			 "pi_file_close_for_write: too many entries "
			 "for this implentation of pi-file: %d\n",
			 pf->nentries));
		return PI_ERR_FILE_INVALID;
	}

	if ((f = fopen(pf->file_name, "wb")) == NULL)
		return PI_ERR_FILE_ERROR;

	ip = &pf->info;

	offset = PI_HDR_SIZE + pf->nentries * pf->ent_hdr_size + 2;

	p = buf;
	memcpy(p, ip->name, 32);
	set_short(p + 32, ip->flags);
	set_short(p + 34, ip->version);
	set_long(p + 36, unix_time_to_pilot_time(ip->createDate));
	set_long(p + 40, unix_time_to_pilot_time(ip->modifyDate));
	set_long(p + 44, unix_time_to_pilot_time(ip->backupDate));
	set_long(p + 48, ip->modnum);
	set_long(p + 52, pf->app_info_size ? offset : 0);
	offset += pf->app_info_size;
	set_long(p + 56, pf->sort_info_size ? offset : 0);
	offset += pf->sort_info_size;
	set_long(p + 60, ip->type);
	set_long(p + 64, ip->creator);
	set_long(p + 68, pf->unique_id_seed);
	set_long(p + 72, pf->next_record_list_id);
	set_short(p + 76, pf->nentries);

	if (fwrite(buf, PI_HDR_SIZE, 1, f) != 1)
		goto bad;

	for (i = 0, entp = pf->entries; i < pf->nentries; i++, entp++) {
		entp->offset = offset;

		p = buf;
		if (pf->resource_flag) {
			set_long(p, entp->type);
			set_short(p + 4, entp->id_);
			set_long(p + 6, entp->offset);
		} else {
			set_long(p, entp->offset);
			set_byte(p + 4, entp->attrs);
			set_treble(p + 5, entp->uid);
		}

		if (fwrite(buf, (size_t) pf->ent_hdr_size, 1, f) != 1)
			goto bad;

		offset += entp->size;
	}

	/* This may just be packing */
	fwrite("\0\0", 1, 2, f);

	if (pf->app_info
	    && (fwrite(pf->app_info, 1,(size_t) pf->app_info_size, f) !=
		(size_t) pf->app_info_size))
		goto bad;

	if (pf->sort_info
	    && (fwrite(pf->sort_info, 1, (size_t) pf->sort_info_size, f) !=
		(size_t) pf->sort_info_size))
		goto bad;


	rewind(pf->tmpf);
	do {
		c = fread (buf, 1, sizeof (buf), pf->tmpf);
		fwrite (buf, 1, (size_t)c, f);
	} while (c == sizeof (buf));
	fflush(f);

	if (ferror(f) || feof(f) || !feof(pf->tmpf))
		goto bad;

	fclose(f);
	return 0;

bad:
	fclose(f);
	return PI_ERR_FILE_ERROR;
}


/***********************************************************************
 *
 * Function:    pi_file_retrieve
 *
 * Summary:     Fetch a file from the device. If caller wants to be
 *				notified of download progress, it should pass a
 *				progress_func ptr.
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_file_retrieve(pi_file_t *pf, int socket, int cardno,
	progress_func report_progress)
{
	int 	db = -1,
		j,
		written 	= 0,
		total_size,
		result;
	struct DBSizeInfo size_info;
	pi_buffer_t *buffer = NULL;

	pi_reset_errors(socket);

	if ((result = dlp_FindDBByName(socket, cardno, pf->info.name,
			NULL, NULL, NULL, &size_info)) < 0)
		goto fail;

	total_size = size_info.totalBytes + size_info.appBlockSize;

	if ((result = dlp_OpenDB (socket, cardno, dlpOpenRead | dlpOpenSecret,
			pf->info.name, &db)) < 0)
		goto fail;

	buffer = pi_buffer_new (DLP_BUF_SIZE);
	if (buffer == NULL) {
		result = pi_set_error(socket, PI_ERR_GENERIC_MEMORY);
		goto fail;
	}

	if (size_info.appBlockSize) {
		result = dlp_ReadAppBlock(socket, db, 0, DLP_BUF_SIZE, buffer);
		if (result < 0)
			goto fail;
		if (result > 0) {
			pi_file_set_app_info(pf, buffer->data, (size_t)result);
			written = result;
			if (report_progress
				&& report_progress(socket, pf, total_size, 
					written, 0) == PI_TRANSFER_STOP) {
				result = PI_ERR_FILE_ABORTED;
				goto fail;
			}
		}
	}

	if (pf->info.flags & dlpDBFlagResource) {
		for (j = 0; j < size_info.numRecords; j++) {
			int 	id_;
			unsigned long type;

			if ((result = dlp_ReadResourceByIndex(socket, db, j, buffer,
					&type, &id_)) < 0)
				goto fail;

			if ((result = pi_file_append_resource (pf, buffer->data, buffer->used,
					type, id_)) < 0) {
				pi_set_error(socket, result);
				goto fail;
			}

			written += buffer->used;

			if (report_progress
				&& report_progress(socket, pf, total_size,
					written, j+1) == PI_TRANSFER_STOP) {
				result = pi_set_error(socket, PI_ERR_FILE_ABORTED);
				goto fail;
			}
		}
	} else for (j = 0; j < size_info.numRecords; j++) {
		int 	attr,
			category;
		unsigned long id_;

		if ((result = dlp_ReadRecordByIndex(socket, db, j, buffer, &id_, &attr,
				&category)) < 0)
			goto fail;

		written += buffer->used;

		if (report_progress
			&& report_progress(socket, pf, total_size,
				written, j+1) == PI_TRANSFER_STOP) {
			result = pi_set_error(socket, PI_ERR_FILE_ABORTED);
			goto fail;
		}

		/* There is no way to restore records with these
		   attributes, so there is no use in backing them up
		 */
		if (attr &
		    (dlpRecAttrArchived | dlpRecAttrDeleted))
			continue;
		if ((result = pi_file_append_record(pf, buffer->data, buffer->used,
				attr, category, id_)) < 0) {
			pi_set_error(socket, result);
			goto fail;
		}
	}

	pi_buffer_free(buffer);

	return dlp_CloseDB(socket, db);

fail:
	if (db != -1 && pi_socket_connected(socket)) {
		int err = pi_error(socket);			/* make sure we keep last error code */
		int palmoserr = pi_palmos_error(socket);
		
		dlp_CloseDB(socket, db);
		
		pi_set_error(socket, err);			/* then restore it afterwards */
		pi_set_palmos_error(socket, palmoserr);
	}

	if (buffer != NULL)
		pi_buffer_free (buffer);

	if (result >= 0) {
		/* one of our pi_file* calls failed */
		result = pi_set_error(socket, PI_ERR_FILE_ERROR);
	}
	return result;
}


/***********************************************************************
 *
 * Function:    pi_file_install
 *
 * Summary:     
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_file_install(pi_file_t *pf, int socket, int cardno,
	progress_func report_progress)
{
	int 	db = -1,
		j,
		reset 		= 0,
		flags,
		version,
		freeai 		= 0,
		result,
		err1,
		err2;
	size_t	l,
		size = 0,
		total_size;
	void 	*buffer;

	version = pi_version(socket);

	/* compute total size for progress reporting, and check that
	   either records are 64k or less, or the handheld can accept
	   large records. we do this prior to starting the install,
	   to avoid messing the device up if we have to fail. */
	total_size = pf->app_info_size;
	for (j = 0; j < pf->nentries; j++) {
		result =  (pf->info.flags & dlpDBFlagResource) ?
			pi_file_read_resource(pf, j, 0, &size, 0, 0) :
			pi_file_read_record(pf, j, 0, &size, 0, 0, 0);
		if (result < 0) {
			LOG((PI_DBG_API, PI_DBG_LVL_ERR,
				"FILE INSTALL can't read all records/resources\n"));
			goto fail;
		}
		if (size > 65536 && version < 0x0104) {
			LOG((PI_DBG_API, PI_DBG_LVL_ERR,
				"FILE INSTALL Database contains"
				" record/resource over 64K!\n"));
			goto fail;
		}
		total_size += size;
	}

	/* Delete DB if it already exists */
	dlp_DeleteDB(socket, cardno, pf->info.name);

	 /* Judd - 25Nov99 - Graffiti hack We want to make sure that these 2
	    flags get set for this one */
	if (pf->info.creator == pi_mktag('g', 'r', 'a', 'f')) {
		flags |= dlpDBFlagNewer;
		flags |= dlpDBFlagReset;
	}

	/* Set up DB flags */
	flags = pf->info.flags;

	if (strcmp(pf->info.name, "Graffiti ShortCuts ") == 0) {
		flags |= 0x8000;	/* Rewrite an open DB */
		reset = 1;		/* To be on the safe side */
	}
	LOG((PI_DBG_API, PI_DBG_LVL_INFO,
	    "FILE INSTALL Name: %s Flags: %8.8X\n", pf->info.name, flags));

	/* Create DB */
	if ((result = dlp_CreateDB
	    (socket, pf->info.creator, pf->info.type, cardno, flags,
	     pf->info.version, pf->info.name, &db)) < 0) {
		int retry = 0;

		/* Judd - 25Nov99 - Graffiti hack

		   The dlpDBFlagNewer specifies that if a DB is open and
		   cannot be deleted then it can be overwritten by a DB with
		   a different name.  The creator ID of "graf" is what
		   really identifies a DB, not the name.  We could call it
		   JimBob and the palm would still find it and use it. */

		if (strcmp(pf->info.name, "Graffiti ShortCuts ") == 0) {
			strcpy(pf->info.name, "Graffiti ShortCuts");
			retry = 1;
		} else if (strcmp(pf->info.name, "Graffiti ShortCuts") ==
			   0) {
			strcpy(pf->info.name, "Graffiti ShortCuts ");
			retry = 1;
		} else if (pf->info.creator ==
			   pi_mktag('g', 'r', 'a', 'f')) {
			/* Yep, someone has named it JimBob */
			strcpy(pf->info.name, "Graffiti ShortCuts");
			retry = 1;
		}

		if (retry) {
			/* Judd - 25Nov99 - Graffiti hack
			   We changed the name, now we can try to write it
			   again */
			if ((result = dlp_CreateDB
			    (socket, pf->info.creator, pf->info.type,
			     cardno, flags, pf->info.version,
			     pf->info.name, &db)) < 0) {
				return result;
			}
		} else {
			return result;
		}
	}

	pi_file_get_app_info(pf, &buffer, &l);

	/* Compensate for bug in OS 2.x Memo */
	if (version > 0x0100
		&& strcmp(pf->info.name, "MemoDB") == 0
		&& l > 0
		&& l < 282) {
		/* Justification: The appInfo structure was accidentally
		   lengthend in OS 2.0, but the Memo application does not
		   check that it is long enough, hence the shorter block
		   from OS 1.x will cause the 2.0 Memo application to lock
		   up if the sort preferences are modified. This code
		   detects the installation of a short app info block on a
		   2.0 machine, and lengthens it. This transformation will
		   never lose information. */
		void *b2 = calloc(1, 282);
		memcpy(b2, buffer, (size_t)l);
		buffer = b2;
		total_size += 282 - l;
		l = 282;
		freeai = 1;
	}

	/* All system updates seen to have the 'ptch' type, so trigger a
	   reboot on those */
	if (pf->info.creator == pi_mktag('p', 't', 'c', 'h'))
		reset = 1;

	if (pf->info.flags & dlpDBFlagReset)
		reset = 1;

	/* Upload appInfo block */
	if (l > 0) {
		if ((result = dlp_WriteAppBlock(socket, db, buffer, l)) < 0) {
			if (freeai)
				free(buffer);
			goto fail;
		}
		
		if (freeai)
			free(buffer);

		if (report_progress
			&& report_progress(socket, pf, (int)total_size,
				(int)l, 0) == PI_TRANSFER_STOP) {
			result = pi_set_error(socket, PI_ERR_FILE_ABORTED);
			goto fail;
		}
	}

	/* Upload resources / records */
	if (pf->info.flags & dlpDBFlagResource) {
		for (j = 0; j < pf->nentries; j++) {
			int 	id_;
			unsigned long type;

			if ((result = pi_file_read_resource(pf, j, &buffer, &size,
					&type,	&id_)) < 0)
				goto fail;

			/* Skip empty resource, it cannot be installed */
			if (size == 0)
				continue;

			if ((result = dlp_WriteResource(socket, db, type, id_, buffer,
					size)) < 0)
				goto fail;

			l += size;

			if (report_progress
				&& report_progress(socket, pf, (int)total_size,
					(int)l, (int)j) == PI_TRANSFER_STOP) {
				result = pi_set_error(socket, PI_ERR_FILE_ABORTED);
				goto fail;
			}
			
			/* If we see a 'boot' section, regardless of file
			   type, require reset */
			if (type == pi_mktag('b', 'o', 'o', 't'))
				reset = 1;
		}
	} else {
		for (j = 0; j < pf->nentries; j++) {
			int 	attr,
				category;
			unsigned long id_;

			if ((result = pi_file_read_record(pf, j, &buffer, &size, &attr,
					&category, &id_)) < 0)
				goto fail;

			/* Old OS version cannot install deleted records, so
			   don't even try */
			if ((attr & (dlpRecAttrArchived | dlpRecAttrDeleted))
			    && version < 0x0101)
				continue;

			if ((result = dlp_WriteRecord(socket, db, attr, id_, category,
					buffer, size, 0)) < 0)
				goto fail;

			l += size;

			if (report_progress
				&& report_progress(socket, pf, (int)total_size,
					(int)l, (int)j) == PI_TRANSFER_STOP) {
				result = pi_set_error(socket, PI_ERR_FILE_ABORTED);
				goto fail;
			}
		}
	}

	if (reset)
		dlp_ResetSystem(socket);

	return dlp_CloseDB(socket, db);

fail:
	/* save error codes then restore them after
	   closing/deleting the DB */
	err1 = pi_error(socket);
	err2 = pi_palmos_error(socket);

	LOG((PI_DBG_API, PI_DBG_LVL_ERR, "FILE INSTALL error: pilot-link "
		    "0x%04x, PalmOS 0x%04x\n", err1, err2));
	if (db != -1 && pi_socket_connected(socket))
		dlp_CloseDB(socket, db);
	if (pi_socket_connected(socket))
		dlp_DeleteDB(socket, cardno, pf->info.name);

	pi_set_error(socket, err1);
	pi_set_palmos_error(socket, err2);

	if (result >= 0)
		result = pi_set_error(socket, PI_ERR_FILE_ERROR);
	return result;
}


/***********************************************************************
 *
 * Function:    pi_file_merge
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_file_merge(pi_file_t *pf, int socket, int cardno,
	progress_func report_progress)
{
	int 	db = -1,
		j,
		reset 	= 0,
		version,
		total_size = 0,
		bytes_written = 0,
		result;
	void 	*buffer;
	size_t	size;
	
	version = pi_version(socket);

	if (dlp_OpenDB
	    (socket, cardno, dlpOpenReadWrite | dlpOpenSecret,
	     pf->info.name, &db) < 0)
		return pi_file_install(pf, socket, cardno, report_progress);

	/* compute total size for progress reporting, and check that
	   either records are 64k or less, or the handheld can accept
	   large records. we do this prior to starting the install,
	   to avoid messing the device up if we have to fail. */
	for (j = 0; j < pf->nentries; j++) {
		result =  (pf->info.flags & dlpDBFlagResource) ?
			pi_file_read_resource(pf, j, 0, &size, 0, 0) :
			pi_file_read_record(pf, j, 0, &size, 0, 0, 0);
		if (result < 0) {
			LOG((PI_DBG_API, PI_DBG_LVL_ERR,
				"FILE INSTALL can't read all records/resources\n"));
			goto fail;
		}
		if (size > 65536 && version < 0x0104) {
			LOG((PI_DBG_API, PI_DBG_LVL_ERR,
				"FILE INSTALL Database contains"
				" record/resource over 64K!\n"));
			result = pi_set_error(socket, PI_ERR_DLP_DATASIZE);
			goto fail;
		}
		total_size += size;
	}

	/* All system updates seen to have the 'ptch' type, so trigger a
	   reboot on those */
	if (pf->info.creator == pi_mktag('p', 't', 'c', 'h'))
		reset = 1;

	if (pf->info.flags & dlpDBFlagReset)
		reset = 1;

	/* Upload resources / records */
	if (pf->info.flags & dlpDBFlagResource) {
		for (j = 0; j < pf->nentries; j++) {
			int 	id_;
			unsigned long type;

			if ((result = pi_file_read_resource
			    (pf, j, &buffer, &size, &type, &id_)) < 0)
				goto fail;

			if (size == 0)
				continue;

			if ((result = dlp_WriteResource
			    (socket, db, type, id_, buffer, size)) < 0)
				goto fail;

			bytes_written += size;

			if (report_progress
				&& report_progress(socket, pf, total_size,
					bytes_written, j) == PI_TRANSFER_STOP) {
				result = pi_set_error(socket, PI_ERR_FILE_ABORTED);
				goto fail;
			}

			/* If we see a 'boot' section, regardless of file
			   type, require reset */
			if (type == pi_mktag('b', 'o', 'o', 't'))
				reset = 1;
		}
	} else {
		for (j = 0; j < pf->nentries; j++) {
			int	attr,
				category;
			unsigned long id_;

			if ((result = pi_file_read_record(pf, j, &buffer, &size,
					&attr, &category, &id_)) < 0)
				goto fail;

			/* Old OS version cannot install deleted records, so
			   don't even try */
			if ((attr & (dlpRecAttrArchived | dlpRecAttrDeleted))
			    && version < 0x0101)
				continue;

			if ((result = dlp_WriteRecord(socket, db, attr, 0, category,
					buffer, size, 0)) < 0)
				goto fail;

			bytes_written += size;

			if (report_progress
				&& report_progress(socket, pf, total_size,
					bytes_written, j) == PI_TRANSFER_STOP) {
				result = PI_ERR_FILE_ABORTED;
				goto fail;
			}
		}
	}

	if (reset)
		dlp_ResetSystem(socket);

	return dlp_CloseDB(socket, db);

fail:
	if (db != -1 && pi_socket_connected(socket)) {
		int err1 = pi_error(socket);
		int err2 = pi_palmos_error(socket);

		dlp_CloseDB(socket, db);

		pi_set_error(socket, err1);
		pi_set_palmos_error(socket, err2);
	}
	if (result >= 0)
		result = pi_set_error(socket, PI_ERR_FILE_ERROR);
	return result;
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
