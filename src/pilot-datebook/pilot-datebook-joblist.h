/*
 * Pilot Datebook processing utility
 * Matthias Hessler <pilot-datebook@mhessler.de> December 2000
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#ifndef _PILOT_DATEBOOK_JOBLIST
#define _PILOT_DATEBOOK_JOBLIST


#include "pilot-datebook-job.h"


/* Data structures */

struct job_list {
  struct job * top;
  struct job * last;
};



/* Function definitions */

/* Public */
/* For joblist handling */
void joblist_init (struct job_list * joblist);
int joblist_set_param (struct job_list * joblist, int argc, char **argv);
void joblist_parse_param (struct job_list * joblist, int argc, char **argv, char * progname);
void joblist_read_param_file(struct job_list * joblist, char * filename, char * progname);
void joblist_add(struct job_list * joblist, struct job * add_job);
void joblist_del(struct job_list * joblist, struct job * del_job);
struct job * joblist_get_first(struct job_list * joblist);
struct job * joblist_get_next(struct job_list * joblist, struct job * cur_job);
void joblist_process (struct job_list * joblist);
void joblist_statistics (struct job_list * joblist);
void joblist_exit (struct job_list * joblist);
void joblist_abort_all (void);


#endif
