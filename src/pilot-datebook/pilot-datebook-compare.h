/*
 * Pilot Datebook processing utility
 * Matthias Hessler <pilot-datebook@mhessler.de> December 2000
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#ifndef _PILOT_DATEBOOK_COMPARE
#define _PILOT_DATEBOOK_COMPARE


#include "pilot-datebook-data.h"


/* Data structures */

/* Compare structure */
struct compare_data {
  struct compare_data * next;
  struct cond_data cond;

  /* Statistics */
  unsigned long num_compared;
  unsigned long num_success;
  unsigned long num_failure;
};




/* If header structure */
struct compare_list_data {
  /* If conds */
  struct compare_data * compare_top;
  struct compare_data * compare_last;

  /* For statistics */
  unsigned long records_compared;
  unsigned long records_success;
  unsigned long records_failure;
};


/* Function definitions */

/* Public */

/* Init & destroy */
int compare_init (struct compare_list_data * condlist, char * opt_arg);
void compare_exit (struct compare_list_data * condlist);
int compare_set_param (struct compare_list_data * condlist, char opt, char * opt_arg);

/* For reading & writing */
void compare_row (struct compare_list_data * condlist, struct row_data * row);

/* For statistics */
void compare_show_statistics (struct compare_list_data * condlist);


/* Private */

/* Helper */
void comparelist_compare_add(struct compare_list_data * comparelist, struct compare_data * add_compare);
void comparelist_compare_del(struct compare_list_data * comparelist, struct compare_data * del_compare);
struct compare_data * comparelist_get_first_compare(struct compare_list_data * comparelist);
struct compare_data * comparelist_get_next_compare(struct compare_list_data * comparelist, struct compare_data * cur_compare);
struct compare_data * compare_new_compare (void);
void compare_free_compare (struct compare_data * compare);

int compare_row_cond(struct compare_data * compare, struct row_data * compare_row);


#endif
