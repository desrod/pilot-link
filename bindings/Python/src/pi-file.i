// -*- C -*-

extern struct pi_file *pi_file_open (const char *name);
extern PIERROR pi_file_close (pi_file_t *pf);
extern void pi_file_get_info (pi_file_t *pf, struct DBInfo *OUTPUT);
extern void pi_file_get_app_info  (pi_file_t *pf, void **OUTBUF,
					      size_t *OUTBUFLEN);
extern void pi_file_get_sort_info (pi_file_t *pf, void **OUTBUF,
					      size_t *OUTBUFLEN);
extern PIERROR pi_file_read_resource (pi_file_t *pf, int idx, void **OUTBUF,
				  size_t *OUTBUFLEN, unsigned long *OUTSTR4,
				  int *OUTPUT);
extern PIERROR pi_file_read_resource_by_type_id (pi_file_t *pf,
					     unsigned long STR4, int id,
					     void **OUTBUF, size_t *OUTBUFLEN, int *OUTPUT);
extern PIERROR pi_file_type_id_used (pi_file_t *pf, unsigned long STR4, int id);
extern PIERROR pi_file_read_record (pi_file_t *pf, int idx, void **OUTBUF, size_t *OUTBUFLEN,
				    int *OUTPUT, int *OUTPUT, recordid_t *OUTPUT);
extern void pi_file_get_entries (pi_file_t *pf, int *OUTPUT);
extern PIERROR pi_file_read_record_by_id (pi_file_t *pf, recordid_t INPUT, void **OUTBUF,
					  size_t *OUTBUFLEN, int *OUTPUT, int *OUTPUT,
					  int *OUTPUT);
extern PIERROR pi_file_id_used (pi_file_t *pf, recordid_t INPUT);
extern pi_file_t *pi_file_create (const char *name, const struct DBInfo *INPUT);
extern PIERROR pi_file_set_info (pi_file_t *pf, const struct DBInfo *INPUT);
extern PIERROR pi_file_set_app_info (pi_file_t *pf, void *INBUF, size_t INBUFLEN);
extern PIERROR pi_file_set_sort_info (pi_file_t *pf, void *INBUF, size_t INBUFLEN);
extern PIERROR pi_file_append_resource (pi_file_t *pf, void *INBUF, size_t INBUFLEN,
					unsigned long STR4, int id);
extern PIERROR pi_file_append_record (pi_file_t *pf, void *INBUF, size_t INBUFLEN,
				      int attr, int category, recordid_t INPUT);
extern PIERROR pi_file_retrieve (pi_file_t *pf, int socket, int cardno, 
				 progress_func PROGRESSFUNC);
extern PIERROR pi_file_install (pi_file_t *pf, int socket, int cardno,
				progress_func PROGRESSFUNC);
extern PIERROR pi_file_merge (pi_file_t *pf, int socket, int cardno,
			      progress_func PROGRESSFUNC);

// pi-inet / pi-inetserial
// pi-padp
// pi-serial
// pi-slp
// pi-sync
