/*
 * Pilot Datebook processing utility
 * Matthias Hessler <pilot-datebook@mhessler.de> December 2000
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#ifndef _PILOT_DATEBOOK_UPDATE
#define _PILOT_DATEBOOK_UPDATE


#include "pilot-datebook-data.h"


/* Data structures */

/* Update assign structure */
struct update_assign_data {
  struct update_assign_data * next;
  struct assign_data assign;

  /* Statistics */
  unsigned long num_updated;
};




/* Update header structure */
struct update_data {
  /* Update assigns */
  struct update_assign_data * assign_top;
  struct update_assign_data * assign_last;

  /* For statistics */
  unsigned long records_updated;
};


/* Function definitions */

/* Public */

/* Init & destroy */
int update_init (struct update_data * updatelist, char * opt_arg);
void update_exit (struct update_data * updatelist);
int update_set_param (struct update_data * updatelist, char opt, char * opt_arg);

/* For reading & writing */
void update_row (struct update_data * updatelist, struct row_data * row);

/* For statistics */
void update_show_statistics (struct update_data * data);


/* Private */

/* Helper */
void updatelist_assign_add(struct update_data * updatelist, struct update_assign_data * add_update_assign);
void updatelist_assign_del(struct update_data * updatelist, struct update_assign_data * del_update_assign);
struct update_assign_data * updatelist_get_first_assign(struct update_data * updatelist);
struct update_assign_data * updatelist_get_next_assign(struct update_data * updatelist, struct update_assign_data * cur_update_assign);
struct update_assign_data * update_new_assign (void);
void update_free_assign (struct update_assign_data * update_assign);

void update_row_assign(struct update_assign_data * update_assign, struct row_data * update_row);

char * update_find_delimiter (char * pos1);


#endif
