/*
 * Pilot Datebook processing utility
 * Matthias Hessler <pilot-datebook@mhessler.de> December 2000
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#ifndef _PILOT_DATEBOOK_SORT
#define _PILOT_DATEBOOK_SORT


#include "pilot-datebook-data.h"


/* Data structures */

/* Sort field structure */
struct sort_field_data {
  struct sort_field_data * next;
  struct field_data field;
  int isAscending;

  /* Statistics */
  unsigned long num_compared;
};


/* Sort row structure */
struct sort_row_data {
  struct sort_row_data * next;
  struct row_data row;
};


/* Sort header structure */
struct sort_data {
  /* Sort order fields */
  struct sort_field_data * field_top;
  struct sort_field_data * field_last;

  /* Sorted data */
  struct sort_row_data * top;
  struct sort_row_data * last;

  /* For statistics */
  unsigned long records_compared;
  int records_read;
  int records_written;
};


/* Function definitions */

/* Public */

/* Init & destroy */
int sort_init (struct sort_data * sortlist, char * opt_arg);
void sort_exit (struct sort_data * sortlist);
int sort_set_param (struct sort_data * sortlist, char opt, char * opt_arg);

/* For reading & writing */
void sort_read_row (struct sort_data * sortlist, struct row_data * row);
int sort_read_eof (struct sort_data * sortlist);
void sort_write_row (struct sort_data * sortlist, struct header_data * header, struct row_data * row);

/* For statistics */
void sort_show_statistics (struct sort_data * data);


/* Private */

/* Helper */
void sortlist_add(struct sort_data * sortlist, struct sort_row_data * add_sort_row);
void sortlist_del(struct sort_data * sortlist, struct sort_row_data * del_sort_row);
struct sort_row_data * sortlist_get_first(struct sort_data * sortlist);
struct sort_row_data * sortlist_get_next(struct sort_data * sortlist, struct sort_row_data * cur_sort_row);
struct sort_row_data * sort_new_row (void);
void sort_free_row (struct sort_row_data * sort_row);

void sortlist_field_add(struct sort_data * sortlist, struct sort_field_data * add_sort_field);
void sortlist_field_del(struct sort_data * sortlist, struct sort_field_data * del_sort_field);
struct sort_field_data * sortlist_get_first_field(struct sort_data * sortlist);
struct sort_field_data * sortlist_get_next_field(struct sort_data * sortlist, struct sort_field_data * cur_sort_field);
struct sort_field_data * sort_new_field (void);
void sort_free_field (struct sort_field_data * sort_field);

int sort_cmp_whole_row(struct sort_data * sortlist, struct sort_row_data * sort_row1, struct sort_row_data * sort_row2);
int sort_cmp_row(struct sort_field_data * sort_field, struct sort_row_data * sort_row1, struct sort_row_data * sort_row2);


#endif
