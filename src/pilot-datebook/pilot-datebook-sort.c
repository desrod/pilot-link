/*
 * Pilot Datebook processing utility
 * Matthias Hessler <pilot-datebook@mhessler.de> December 2000
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */


#include "pilot-datebook-sort.h"


/* Public functions */

/* Initialize sort data structure for writing */
int
sort_init (struct sort_data * sortlist, char * opt_arg)
{
  struct sort_field_data * sort_field;
  char * pos1;
  char * pos2;


  /* Debug */
  debug_message("Entering sort_init\n");

  /* Init */
  sortlist->top = NULL;
  sortlist->last = NULL;
  sortlist->field_top = NULL;
  sortlist->field_last = NULL;
  sortlist->records_compared = 0;
  sortlist->records_read = 0;
  sortlist->records_written = 0;

  /* Expected input parameter: sort order fields */

  /* Loop over all sort fields, delimited by ';' as delimiter */
  pos1 = opt_arg;
  pos2 = data_index(pos1, ';');
  while (pos1 != NULL
	 && *pos1 != '\0') {

    /* Alloc sort field data */
    sort_field = sort_new_field();

    /* Ascending or descending? */
    if (*pos1 == '+') {
      /* '+' before field name => ascending */
      sort_field->isAscending = TRUE;
      pos1++;
    }
    else if (*pos1 == '-') {
      /* '-' before field name => descending */
      sort_field->isAscending = FALSE;
      pos1++;
    }
    else {
      /* Default => ascending */
      sort_field->isAscending = TRUE;
    }

    /* Insert field data into sort field list */
    field_init(&(sort_field->field), pos1, pos2);
    sort_field->next = NULL;

    /* Insert sort field into sort structure */
    sortlist_field_add(sortlist, sort_field);

    /* Extract next sort field, skipping delimiter */
    if (pos2 != NULL) {
      pos1 = pos2 +1;
      pos2 = data_index(pos1, ';');
    }
    else {
      break;
    }
  }

  /* Debug */
  debug_message("Leaving sort_init\n");

  return TRUE;
}


/* Destroy sort data structure */
void
sort_exit (struct sort_data * sortlist)
{
  struct sort_row_data * sort_row;
  struct sort_field_data * sort_field;


  /* Debug */
  debug_message("Entering sort_exit\n");

  /* Now destroy all sort rows */
  sort_row = sortlist_get_first(sortlist);
  while (sort_row != NULL) {
    /* Remove sort row from sortlist */
    sortlist_del(sortlist, sort_row);

    /* Destroy sort row data structure */
    sort_free_row(sort_row);

    /* Next = now first (after removal of previous first) */
    sort_row = sortlist_get_first(sortlist);
  } /* while */


  /* Now destroy all sort fields */
  sort_field = sortlist_get_first_field(sortlist);
  while (sort_field != NULL) {
    debug_message("Removing sort field...\n");

    /* Free field data */
    field_exit(&(sort_field->field));

    /* Remove sort field from sortlist */
    sortlist_field_del(sortlist, sort_field);

    /* Destroy sort field data structure */
    sort_free_field(sort_field);

    /* Next = now first (after removal of previous first) */
    sort_field = sortlist_get_first_field(sortlist);
  } /* while */

  /* Init */
  sortlist->top = NULL;
  sortlist->last = NULL;
  sortlist->field_top = NULL;
  sortlist->field_last = NULL;
  sortlist->records_compared = 0;
  sortlist->records_read = 0;
  sortlist->records_written = 0;

  /* Debug */
  debug_message("Leaving sort_exit\n");
}



/* Set command line options for sort reading */
int
sort_set_param (struct sort_data * sortlist, char opt, char * opt_arg)
{
  /* Debug */
  debug_message("Entering sort_set_param\n");

  error_message("Currently command line options for sort are NOT supported!\n");

  /* Debug */
  debug_message("Leaving sort_set_param\n");

  return FALSE;
}


/* Read top-most sorted row from our sorted structure & remove it from there */
void
sort_read_row (struct sort_data * sortlist, struct row_data * row)
{
  struct sort_row_data * sort_row;


  /* Debug */
  debug_message("Entering sort_read_row\n");

  /* Get top-most sort row */
  sort_row = sortlist_get_first(sortlist);

  /* If empty then fail */
  if (sort_row == NULL)
    error_message("Trying to read too many sort rows\n");
  else
    *row = sort_row->row;

  /* Remove sort row from sortlist */
  sortlist_del(sortlist, sort_row);

  /* Destroy sort row data structure */
  sort_free_row (sort_row);

  /* Update statistics */
  sortlist->records_read++;

  /* Debug */
  debug_message("Leaving sort_read_row\n");
}


/* Check for end of reading */
int
sort_read_eof (struct sort_data * sortlist)
{

  /* Debug */
  debug_message("Entering & leaving sort_read_eof\n");

  if (sortlist == NULL)
    return TRUE;
  else
    return (sortlist_get_first(sortlist) == NULL);
}


/* Write a row to be inserted into our sorted structure */
void
sort_write_row (struct sort_data * sortlist, struct header_data * header, struct row_data * row)
{
  struct sort_row_data * sort_row;


  /* Debug */
  debug_message("Entering sort_write_row\n");

  /* Alloc sort row data */
  sort_row = sort_new_row();

  /* Insert row data into sort row */
  sort_row->row = *row;
  sort_row->next = NULL;

  /* Insert sort row into sort structure */
  sortlist_add(sortlist, sort_row);

  /* Update statistics */
  sortlist->records_written++;

  /* Debug */
  debug_message("Leaving sort_write_row\n");
}


/* For statistics */

/* Show statistics */
void
sort_show_statistics (struct sort_data * sortlist)
{
  struct sort_field_data * sort_field;


  /* Debug */
  debug_message("Entering sort_show_statistics\n");

  info_message("Sorting:\n");
  info_message("Records compared: %ld\n",
	       sortlist->records_compared);
  info_message("Records written: %d\n",
	       sortlist->records_written);
  info_message("Records read: %d\n",
	       sortlist->records_read);

  /* Statistics for all sort fields */
  sort_field = sortlist_get_first_field(sortlist);
  while (sort_field != NULL) {
    /* Show statistics for this sort field */
    info_message("Sort field <%s, %s> was compared <%ld> times\n",
		 sort_field->field.name,
		 sort_field->isAscending ? "ascending" : "descending",
		 sort_field->num_compared);

    /* Next sort field */
    sort_field = sortlist_get_next_field(sortlist, sort_field);
  } /* while */

  /* Debug */
  debug_message("Leaving sort_show_statistics\n");
}



/* Private functions */

/* Helper */

/* For now use unsorted storage in linked list, since it is easier to
 * implement than a tree.
 * However, linked list has horrible performance => fix me!
 */

/* Append one sort_row to sortlist */
void
sortlist_add(struct sort_data * sortlist, struct sort_row_data * add_sort_row)
{

  /* Add to list */
  if (add_sort_row == NULL) {
    /* No sort_row provided => nothing to add */
  }
  else if (sortlist == NULL) {
    /* No sortlist provided => nothing to add */
  }
  /* All parameters present */
  /* Empty list? */
  else if (sortlist->last == NULL
	   || sortlist->top == NULL) {
    /* List is empty */
    add_sort_row->next = NULL;
    sortlist->last = add_sort_row;
    sortlist->top = add_sort_row;
  }
  /* Last entry into list? (Keep unsorted order by default) */
  else if (sort_cmp_whole_row(sortlist, add_sort_row, sortlist->last) >= 0) {
    /* Add after last */
    add_sort_row->next = NULL;
    sortlist->last->next = add_sort_row;
    sortlist->last = add_sort_row;
  }
  /* First entry into list? */
  else if (sort_cmp_whole_row(sortlist, add_sort_row, sortlist->top) <= 0) {
    /* Add before first */
    add_sort_row->next = sortlist->top;
    sortlist->top = add_sort_row;
  }
  else {
    /* Add in middle */

    /* Now here we get into horrible performance with a sorted list...
     * Fix me to use a tree or so...
     */

    /* Find first sort item before which we want to insert.
     * At least two have to exist, otherwise we would have added at the end
     * or at the beginning
     */
    struct sort_row_data * row;

    row = sortlist_get_first(sortlist);
    while (sort_cmp_whole_row(sortlist, add_sort_row, row->next) > 0) {
      row = sortlist_get_next(sortlist, row);
    } /* while */
    /* Insert at found location */
    add_sort_row->next = row->next;
    row->next = add_sort_row;
  }
}


/* Remove one sort_row from sortlist */
void
sortlist_del(struct sort_data * sortlist, struct sort_row_data * del_sort_row)
{

  /* Remove from list */
  if (del_sort_row == NULL) {
    /* No sort_row provided => nothing to remove */
  }
  else if (sortlist == NULL) {
    /* No sortlist provided => nothing to remove */
  }
  else if (sortlist->last == NULL) {
    /* Nothing in list => nothing to remove */
  }
  else {
    if (sortlist->last == del_sort_row) {
      /* Last in list => iterate through list to find new last */
      if (sortlist->top != del_sort_row)
	debug_message("Removing last - inefficient use of sortlist_del\n");
      sortlist->last = sortlist->top;
      while (sortlist->last->next != NULL
	     && sortlist->last->next != del_sort_row) {
	sortlist->last = sortlist->last->next;
      } /* while */
    }
    if (sortlist->top == del_sort_row) {
      /* First in list => remove */
      sortlist->top = del_sort_row->next;
    }
  }

  /* Remove linkage */
  del_sort_row->next = NULL;
}


/* Get first sort_row from sortlist */
struct sort_row_data *
sortlist_get_first(struct sort_data * sortlist)
{
  if (sortlist == NULL)
    return NULL;
  else
    return sortlist->top;
}


/* Get next sort_row from sortlist */
struct sort_row_data *
sortlist_get_next(struct sort_data * sortlist, struct sort_row_data * cur_sort_row)
{
  if (cur_sort_row == NULL)
    return NULL;
  else
    return cur_sort_row->next;
}


/* Alloc new sort row data */
struct sort_row_data *
sort_new_row (void)
{
  return (struct sort_row_data *) malloc (sizeof(struct sort_row_data));
}


/* De-alloc sort row data */
void
sort_free_row (struct sort_row_data * sort_row)
{
  free ((void *) sort_row);
}




/* Append one sort_field to sortlist */
void
sortlist_field_add(struct sort_data * sortlist, struct sort_field_data * add_sort_field)
{

  /* Add to list */
  if (add_sort_field == NULL) {
    /* No sort_field provided => nothing to add */
  }
  else if (sortlist == NULL) {
    /* No sortlist provided => nothing to add */
  }
  else if (sortlist->field_last == NULL) {
    /* First entry into list */
    add_sort_field->next = NULL;
    sortlist->field_last = add_sort_field;
    sortlist->field_top = add_sort_field;
  }
  else {
    /* other entries exist already */
    add_sort_field->next = NULL;
    sortlist->field_last->next = add_sort_field;
    sortlist->field_last = add_sort_field;
  }
}


/* Remove one sort_field from sortlist */
void
sortlist_field_del(struct sort_data * sortlist, struct sort_field_data * del_sort_field)
{

  /* Remove from list */
  if (del_sort_field == NULL) {
    /* No sort_field provided => nothing to remove */
  }
  else if (sortlist == NULL) {
    /* No sortlist provided => nothing to remove */
  }
  else if (sortlist->field_last == NULL) {
    /* Nothing in list => nothing to remove */
  }
  else {
    if (sortlist->field_last == del_sort_field) {
      /* Last in list => iterate through list to find new last */
      if (sortlist->field_top != del_sort_field)
	debug_message("Removing last - inefficient use of sortlist_del\n");
      sortlist->field_last = sortlist->field_top;
      while (sortlist->field_last->next != NULL
	     && sortlist->field_last->next != del_sort_field) {
	sortlist->field_last = sortlist->field_last->next;
      } /* while */
    }
    if (sortlist->field_top == del_sort_field) {
      /* First in list => remove */
      sortlist->field_top = del_sort_field->next;
    }
  }

  /* Remove linkage */
  del_sort_field->next = NULL;
}


/* Get first sort_field from sortlist */
struct sort_field_data *
sortlist_get_first_field(struct sort_data * sortlist)
{
  if (sortlist == NULL)
    return NULL;
  else
    return sortlist->field_top;
}


/* Get next sort_field from sortlist */
struct sort_field_data *
sortlist_get_next_field(struct sort_data * sortlist, struct sort_field_data * cur_sort_field)
{
  if (cur_sort_field == NULL)
    return NULL;
  else
    return cur_sort_field->next;
}


/* Alloc new sort field data */
struct sort_field_data *
sort_new_field (void)
{
  return (struct sort_field_data *) malloc (sizeof(struct sort_field_data));
}


/* De-alloc sort field data */
void
sort_free_field (struct sort_field_data * sort_field)
{
  free ((void *) sort_field);
}


/* Compare two rows completely according to sort field data */
int
sort_cmp_whole_row(struct sort_data * sortlist, struct sort_row_data * sort_row1, struct sort_row_data * sort_row2)
{

  /* Statistics */
  sortlist->records_compared++;

  return sort_cmp_row(sortlist->field_top, sort_row1, sort_row2);
}


/* Compare one field of two rows */
int
sort_cmp_row(struct sort_field_data * sort_field, struct sort_row_data * sort_row1, struct sort_row_data * sort_row2)
{
  int result;


  /* No sort field? Then both are equal. */
  if (sort_field == NULL)
    return 0;

  /* Safety net */
  if (sort_row1 == NULL
      || sort_row2 == NULL)
    return 0;

  /* Statistics */
  sort_field->num_compared++;

  /* Compare one field */
  result = field_cmp_rows(&(sort_field->field),
			  &(sort_row1->row),
			  &(sort_row2->row));

  /* Invert result on descending sort order */
  if (!sort_field->isAscending)
    result *= (-1);

  /* Compare with next sort field, if not yet decided */
  if (result == 0)
    result = sort_cmp_row(sort_field->next, sort_row1, sort_row2);

  return result;
}
