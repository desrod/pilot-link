
#ifndef _PILOT_FILE_H_
#define _PILOT_FILE_H_

/* For DBInfo */
#include "dlp.h"

/* For time_t */
#include <time.h>

typedef unsigned long pi_uid_t;

struct pi_file; /* forward declaration */

/* most functions return -1 for error, 0 for ok */

/* read-only open */
struct pi_file *pi_file_open (char *name);

/* closes read or write handle */
int pi_file_close (struct pi_file *pf);

int pi_file_get_info (struct pi_file *pf, struct DBInfo * infop);
int pi_file_get_app_info (struct pi_file *pf, void **datap, int *sizep);
int pi_file_get_sort_info (struct pi_file *pf, void **dadtap, int *sizep);
int pi_file_read_resource (struct pi_file *pf, int idx,
			   void **bufp, int *sizep, unsigned long *type, int *idp);
int pi_file_read_record (struct pi_file *pf, int idx,
			 void **bufp, int *sizep, int *attrp, int * catp, pi_uid_t *uidp);
int pi_file_get_entries (struct pi_file *pf, int * entries);			 

struct pi_file *pi_file_create (char *name, struct DBInfo * info);

/* may call these any time before close (even multiple times) */
int pi_file_set_info (struct pi_file *pf, struct DBInfo * infop);
int pi_file_set_app_info (struct pi_file *pf, void *data, int size);
int pi_file_set_sort_info (struct pi_file *pf, void *data, int size);

int pi_file_append_resource (struct pi_file *pf, void *buf, int size,
			    unsigned long type, int id);
int pi_file_append_record (struct pi_file *pf, void *buf, int size,
			   int attr, int category, pi_uid_t uid);
			     
int pi_file_retrieve(struct pi_file * pf, int socket, int cardno);
int pi_file_install(struct pi_file * pf, int socket, int cardno);

#endif