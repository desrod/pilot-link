#ifndef _PILOT_FILE_H_
#define _PILOT_FILE_H_

#ifdef __cplusplus
extern "C" {
#endif

	/* For DBInfo */
#include "pi-dlp.h"

	typedef unsigned long pi_uid_t;

	typedef struct pi_file_entry {
		int 	offset,
			size,
			id_,
			attrs;
		unsigned long type;
		pi_uid_t uid;
	} pi_file_entry_t;

	typedef struct pi_file {
		int 	err,
			for_writing,	
			app_info_size,
			sort_info_size,
			next_record_list_id,
			resource_flag,
			ent_hdr_size,
			nentries,
			nentries_allocated,
			rbuf_size;
		FILE 	*f;
		FILE 	*tmpf;
		char 	*file_name;
		void 	*app_info,
			*sort_info,
			*rbuf;
		unsigned long unique_id_seed;
		struct 	DBInfo info;
		struct 	pi_file_entry *entries;


	} pi_file_t;

	/* progress callback definition (the number of records
	   can be obtained by looking at pf->nentries so we don't
	   need to repeat it in the parameters) 
	   Callback should return either PI_TRANSFER_STOP or
	   PI_TRANSFER_CONTINUE
	*/
	typedef int (*progress_func)(int socket, pi_file_t *pf,
		int total_bytes, int transferred_bytes,
		int transferred_records);

	#define PI_TRANSFER_STOP		0
	#define	PI_TRANSFER_CONTINUE	1


	/* most functions return -1 for error, 0 for ok */

	/* read-only open */
	extern pi_file_t *pi_file_open PI_ARGS((const char *name));

	/* closes read or write handle */
	extern int pi_file_close PI_ARGS((pi_file_t * pf));
	extern void pi_file_get_info
	    PI_ARGS((pi_file_t * pf, struct DBInfo * infop));
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
	extern int pi_file_read_record
	    PI_ARGS((pi_file_t * pf, int idx, void **bufp, size_t *sizep,
		     int *attrp, int *catp, pi_uid_t * uidp));
	extern void pi_file_get_entries
	    PI_ARGS((pi_file_t * pf, int *entries));
	extern int pi_file_read_record_by_id
	    PI_ARGS((pi_file_t * pf, pi_uid_t uid, void **bufp,
		     size_t *sizep, int *idxp, int *attrp, int *catp));
	extern int pi_file_id_used
	    PI_ARGS((pi_file_t * pf, pi_uid_t uid));
	extern pi_file_t *pi_file_create
	    PI_ARGS((const char *name, const struct DBInfo * info));

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

	extern unsigned long unix_time_to_pilot_time
	    PI_ARGS((time_t t));
	extern time_t pilot_time_to_unix_time
	    PI_ARGS((unsigned long raw_time));

#ifdef __cplusplus
}
#endif
#endif
