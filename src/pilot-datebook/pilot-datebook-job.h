/*
 * Pilot Datebook processing utility
 * Matthias Hessler <pilot-datebook@mhessler.de> December 2000
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#ifndef _PILOT_DATEBOOK_JOB
#define _PILOT_DATEBOOK_JOB

#include "pilot-datebook-data.h"

#include "pilot-datebook-compare.h"
#include "pilot-datebook-io.h"
#include "pilot-datebook-sort.h"
#include "pilot-datebook-update.h"


/* Data structures */

/* Job handling */
enum JOB_TYPE {
  COMPARE_JOB = 0,
  READ_JOB = 1,
  WRITE_JOB = 2,
  SORT_JOB = 3,
  UPDATE_JOB = 4,
  DEL_JOB = 5
};


enum JOB_STATE {
  JOB_INITIATED = 0,
  JOB_STARTED = 1,
  JOB_PROCESSED = 2,
  JOB_STOPPED = 3,
  JOB_EXITED = 4,
  JOB_ABORTED = 5
};


struct job {
  enum JOB_TYPE type;
  union {
    struct compare_list_data compare;
    struct file_data io;
    struct sort_data sort;
    struct update_data update;
    struct {
      int records_deleted;
    } del;
  } job_data;

  enum JOB_STATE state;
  int record_num;


  /* store in linked list */
  struct job * next;
};



/* Function definitions */

/* Public */

/* For job handling */
struct job * job_new (enum JOB_TYPE job_type);
void job_free (struct job * job);

struct job * job_init (enum JOB_TYPE job_type, char * opt_arg);
int job_set_param(struct job * job, char opt, char * opt_arg);
void job_start(struct job * job, struct header_data * header);
void job_process(struct job * job, struct header_data * header, struct row_data * row);
void job_stop(struct job * job, struct header_data * header);
void job_exit(struct job * job);
void job_abort(struct job * job);
void job_statistics(struct job * job);

/* Helper functions */
struct row_data * new_row (void);
void free_row (struct row_data * row);


#endif
