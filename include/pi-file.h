/* 
 * Pilot File Interface Library
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
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
#ifndef _PILOT_FILE_H_
#define _PILOT_FILE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "pi-dlp.h"		/* For DBInfo */

typedef unsigned long pi_uid_t;

typedef struct pi_file_entry {
	int 	offset;
	int	size;
	int	id_;
	int	attrs;
	unsigned long type;
	pi_uid_t uid;
} pi_file_entry_t;

typedef struct pi_file {
	int 	err;
	int	for_writing;
	int	app_info_size;
	int	sort_info_size;
	int	next_record_list_id;
	int	resource_flag;
	int	ent_hdr_size;
	int	nentries;
	int	nentries_allocated;
	int	rbuf_size;
	FILE 	*f;
	FILE 	*tmpf;
	char 	*file_name;
	void 	*app_info;
	void	*sort_info;
	void	*rbuf;
	unsigned long unique_id_seed;
	struct 	DBInfo info;
	struct 	pi_file_entry *entries;
} pi_file_t;

/** @brief Transfer progress callback structure */
typedef struct {
	int type;				/**< Transfer type (see #piProgressType enum) */
	int transferred_bytes;			/**< Current transferred bytes */
	void *userinfo;				/**< Provided by and passed back to client (not used for now, will be in a future release) */
	union {
		struct {
			pi_file_t *pf;		/**< May be NULL */
			struct DBSizeInfo size;	/**< Size information */
			int transferred_records;/**< Number of records or resources */
		} db;

		struct {
			char *path;		/**< VFS file path */
			long total_bytes;	/**< File size in bytes */
		} vfs;
	} data;
} pi_progress_t;

/** @brief Progress callback function definition
 *
 * Callback should return either #PI_TRANSFER_STOP or
 * #PI_TRANSFER_CONTINUE
 */
typedef int (*progress_func)(int socket, pi_progress_t *progress);

/** @brief Transfer progress types for the @a type member of pi_progress_t */
enum piProgressType {
	PI_PROGRESS_SEND_DB	= 1,		/**< Sending a database */
	PI_PROGRESS_RECEIVE_DB,			/**< Receiving a database */
	PI_PROGRESS_SEND_VFS,			/**< Sending a VFS file */
	PI_PROGRESS_RECEIVE_VFS			/**< Receiving a VFS file */
};


#define PI_TRANSFER_STOP	0		/**< Returned by progress callback to stop the transfer */
#define	PI_TRANSFER_CONTINUE	1		/**< Returned by progress callback to continue the transfer */

/* read-only open */
extern pi_file_t *pi_file_open
	PI_ARGS((const char *name));

extern pi_file_t *pi_file_create
    PI_ARGS((const char *name, const struct DBInfo *INPUT));

/* closes read or write handle */
extern int pi_file_close PI_ARGS((pi_file_t * pf));

extern void pi_file_get_info
    PI_ARGS((pi_file_t * pf, struct DBInfo *OUTPUT));
extern void pi_file_get_app_info
    PI_ARGS((pi_file_t * pf, void **datap, size_t *sizep));
extern void pi_file_get_sort_info
    PI_ARGS((pi_file_t * pf, void **dadtap, size_t *sizep));

extern int pi_file_read_resource
    PI_ARGS((pi_file_t * pf, int idx, void **bufp, size_t *sizep,
	     unsigned long *type, int *idp));
extern int pi_file_read_resource_by_type_id
    PI_ARGS((pi_file_t * pf, unsigned long type, int id_,
	     void **bufp, size_t *sizep, int *idxp));

extern int pi_file_type_id_used
    PI_ARGS((pi_file_t * pf, unsigned long type, int id_));
extern int pi_file_id_used
    PI_ARGS((pi_file_t * pf, pi_uid_t uid));

extern int pi_file_read_record
    PI_ARGS((pi_file_t * pf, int idx, void **bufp, size_t *sizep,
	     int *attrp, int *catp, pi_uid_t * uidp));
extern int pi_file_read_record_by_id
    PI_ARGS((pi_file_t * pf, pi_uid_t uid, void **bufp,
	     size_t *sizep, int *idxp, int *attrp, int *catp));

extern void pi_file_get_entries
    PI_ARGS((pi_file_t * pf, int *entries));

/* may call these any time before close (even multiple times) */
extern int pi_file_set_info
    PI_ARGS((pi_file_t * pf, const struct DBInfo * infop));
extern int pi_file_set_app_info
    PI_ARGS((pi_file_t * pf, void *data, size_t size));
extern int pi_file_set_sort_info
    PI_ARGS((pi_file_t * pf, void *data, size_t size));

extern int pi_file_append_resource
    PI_ARGS((pi_file_t * pf, void *buf, size_t size,
	     unsigned long type, int id_));
extern int pi_file_append_record
    PI_ARGS((pi_file_t * pf, void *buf, size_t size, int attr,
	     int category, pi_uid_t uid));

extern int pi_file_retrieve
    PI_ARGS((pi_file_t * pf, int socket, int cardno,
		progress_func report_progress));
extern int pi_file_install
    PI_ARGS((pi_file_t * pf, int socket, int cardno,
		progress_func report_progress));
extern int pi_file_merge
    PI_ARGS((pi_file_t * pf, int socket, int cardno,
		progress_func report_progress));

/** @name Utilities */
/*@{*/
	extern unsigned long unix_time_to_pilot_time
	    PI_ARGS((time_t t));
	extern time_t pilot_time_to_unix_time
	    PI_ARGS((unsigned long raw_time));
/*@}*/

#ifdef __cplusplus
}
#endif
#endif
