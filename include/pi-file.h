#ifndef _PILOT_FILE_H_
#define _PILOT_FILE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* For DBInfo */
#include "pi-dlp.h"

/* For time_t */
#include <time.h>

typedef unsigned long pi_uid_t;

struct pi_file; /* forward declaration */

/* most functions return -1 for error, 0 for ok */

/* read-only open */
extern struct pi_file *pi_file_open (char *name);

/* closes read or write handle */
extern int pi_file_close (struct pi_file *pf);

extern int pi_file_get_info (struct pi_file *pf, struct DBInfo * infop);
extern int pi_file_get_app_info (struct pi_file *pf, void **datap, int *sizep);
extern int pi_file_get_sort_info (struct pi_file *pf, void **dadtap, int *sizep);
extern int pi_file_read_resource (struct pi_file *pf, int idx,
			   void **bufp, int *sizep, unsigned long *type, int *idp);
extern int pi_file_read_record (struct pi_file *pf, int idx,
			 void **bufp, int *sizep, int *attrp, int * catp, pi_uid_t *uidp);
extern int pi_file_get_entries (struct pi_file *pf, int * entries);			 
extern int pi_file_read_record_by_id (struct pi_file *pf, pi_uid_t uid,
			       void **bufp, int *sizep, int *idxp,
			       int *attrp, int * catp);
extern int pi_file_id_used (struct pi_file *pf, pi_uid_t uid);

extern struct pi_file *pi_file_create (char *name, struct DBInfo * info);

/* may call these any time before close (even multiple times) */
extern int pi_file_set_info (struct pi_file *pf, struct DBInfo * infop);
extern int pi_file_set_app_info (struct pi_file *pf, void *data, int size);
extern int pi_file_set_sort_info (struct pi_file *pf, void *data, int size);

extern int pi_file_append_resource (struct pi_file *pf, void *buf, int size,
			    unsigned long type, int id);
extern int pi_file_append_record (struct pi_file *pf, void *buf, int size,
			   int attr, int category, pi_uid_t uid);
			     
extern int pi_file_retrieve(struct pi_file * pf, int socket, int cardno);
extern int pi_file_install(struct pi_file * pf, int socket, int cardno);

#ifdef __cplusplus
}
#endif

#endif
