/*
 * Pilot Datebook processing utility
 * Matthias Hessler <pilot-datebook@mhessler.de> December 2000
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */


#include "pilot-datebook-compare.h"


/* Public functions */

/* Initialize data structure */
int
compare_init (struct compare_list_data * comparelist, char * opt_arg)
{
  struct compare_data * compare;
  char * pos1;
  char * pos2;


  /* Debug */
  debug_message("Entering compare_init\n");

  /* Init */
  comparelist->compare_top = NULL;
  comparelist->compare_last = NULL;
  comparelist->records_compared = 0;
  comparelist->records_success = 0;
  comparelist->records_failure = 0;

  /* Expected input parameter: if conditions */

  /* Loop over all if conditions, delimited by ';' as delimiter */
  pos1 = opt_arg;
  /* Skip leading delimiter (avoid check for backslash before it) */
  if (*pos1 == ';')
    pos2 = data_index(pos1 +1, ';');
  else
    pos2 = data_index(pos1, ';');
  /* Skip over quoted delimiters */
  while (pos2 != NULL
	 && *(pos2-1) == '\\')
    pos2 = data_index(pos2+1, ';');

  while (pos1 != NULL
	 && *pos1 != '\0') {

    /* Set pos2 properly on end of line */
    if (pos2 == NULL)
      pos2 = pos1 + strlen(pos1);

    /* Alloc if conditions data */
    compare = compare_new_compare();

    /* Insert condition data into if condition list */
    cond_init(&(compare->cond), pos1, pos2);
    compare->next = NULL;

    /* Insert if condition into if structure */
    comparelist_compare_add(comparelist, compare);

    /* Extract next if condition, skipping delimiter */
    if (pos2 != NULL
	&& *pos2 != '\0') {
      pos1 = pos2 +1;
      pos2 = data_index(pos1, ';');
      /* Skip over quoted delimiters */
      while (pos2 != NULL
	     && *(pos2-1) == '\\')
	pos2 = data_index(pos2+1, ';');
    }
    else {
      break;
    }
  }

  /* Debug */
  debug_message("Leaving compare_init\n");

  return TRUE;
}


/* Destroy if data structure */
void
compare_exit (struct compare_list_data * comparelist)
{
  struct compare_data * compare;


  /* Debug */
  debug_message("Entering compare_exit\n");

  /* Now destroy all if conditions */
  compare = comparelist_get_first_compare(comparelist);
  while (compare != NULL) {
    debug_message("Removing if compare...\n");

    /* Remove if compare from comparelist */
    comparelist_compare_del(comparelist, compare);

    /* Destroy if compare data structure */
    compare_free_compare(compare);

    /* Next = now first (after removal of previous first) */
    compare = comparelist_get_first_compare(comparelist);
  } /* while */

  /* Init */
  comparelist->compare_top = NULL;
  comparelist->compare_last = NULL;
  comparelist->records_compared = 0;
  comparelist->records_success = 0;
  comparelist->records_failure = 0;

  /* Debug */
  debug_message("Leaving compare_exit\n");
}



/* Set command line options for if reading */
int
compare_set_param (struct compare_list_data * comparelist, char opt, char * opt_arg)
{
  /* Debug */
  debug_message("Entering compare_set_param\n");

  error_message("Currently command line options for if are NOT supported!\n");

  /* Debug */
  debug_message("Leaving compare_set_param\n");

  return FALSE;
}


/* Check whether row passes condition, and set selection accordingly */
void
compare_row (struct compare_list_data * comparelist, struct row_data * row)
{
  struct compare_data * compare;
  int result = TRUE;


  /* Debug */
  debug_message("Entering compare_row\n");

  /* Compare a row completely according to compare data */
  compare = comparelist_get_first_compare(comparelist);
  while (compare != NULL) {
    /* Compare one condition in current row */
    result = compare_row_cond(compare, row);

    /* Next if successful (assume AND for multiple conditions) */
    if (result == FALSE)
      break;
    else
      compare = comparelist_get_next_compare(comparelist, compare);
  } /* while */

  /* Set selection */
  if (result)
    setRowIsSelected(row, DATEBOOK_SELECT_YES);
  else
    setRowIsSelected(row, DATEBOOK_SELECT_NO);

  /* Statistics */
  comparelist->records_compared++;
  if (result)
    comparelist->records_success++;
  else
    comparelist->records_failure++;

  /* Debug */
  debug_message("Leaving compare_row\n");
}


/* For statistics */

/* Show statistics */
void
compare_show_statistics (struct compare_list_data * comparelist)
{
  struct compare_data * compare;


  /* Debug */
  debug_message("Entering compare_show_statistics\n");

  info_message("Comparing:\n");
  info_message("Records compared: %ld  (%ld success, %ld failure)\n",
	       comparelist->records_compared,
	       comparelist->records_success,
	       comparelist->records_failure);

  /* Statistics for all if compares */
  compare = comparelist_get_first_compare(comparelist);
  while (compare != NULL) {
    /* Show statistics for this if compare */
    info_message("Comparison <%s> was executed %ld times (%ld success, %ld failure)\n",
		 compare->cond.name,
		 compare->num_compared,
		 compare->num_success,
		 compare->num_failure);

    /* Next if compare */
    compare = comparelist_get_next_compare(comparelist, compare);
  } /* while */

  /* Debug */
  debug_message("Leaving compare_show_statistics\n");
}



/* Private functions */

/* Helper */

/* Append one condition to comparelist */
void
comparelist_compare_add(struct compare_list_data * comparelist, struct compare_data * add_compare)
{

  /* Add to list */
  if (add_compare == NULL) {
    /* No compare provided => nothing to add */
  }
  else if (comparelist == NULL) {
    /* No comparelist provided => nothing to add */
  }
  else if (comparelist->compare_last == NULL) {
    /* First entry into list */
    add_compare->next = NULL;
    comparelist->compare_last = add_compare;
    comparelist->compare_top = add_compare;
  }
  else {
    /* other entries exist already */
    add_compare->next = NULL;
    comparelist->compare_last->next = add_compare;
    comparelist->compare_last = add_compare;
  }
}


/* Remove one condition from comparelist */
void
comparelist_compare_del(struct compare_list_data * comparelist, struct compare_data * del_compare)
{

  /* Remove from list */
  if (del_compare == NULL) {
    /* No compare provided => nothing to remove */
  }
  else if (comparelist == NULL) {
    /* No comparelist provided => nothing to remove */
  }
  else if (comparelist->compare_last == NULL) {
    /* Nothing in list => nothing to remove */
  }
  else {
    if (comparelist->compare_last == del_compare) {
      /* Last in list => iterate through list to find new last */
      if (comparelist->compare_top != del_compare)
	debug_message("Removing last - inefficient use of comparelist_del\n");
      comparelist->compare_last = comparelist->compare_top;
      while (comparelist->compare_last->next != NULL
	     && comparelist->compare_last->next != del_compare) {
	comparelist->compare_last = comparelist->compare_last->next;
      } /* while */
    }
    if (comparelist->compare_top == del_compare) {
      /* First in list => remove */
      comparelist->compare_top = del_compare->next;
    }
  }

  /* Remove linkage */
  del_compare->next = NULL;
}


/* Get first condition from comparelist */
struct compare_data *
comparelist_get_first_compare(struct compare_list_data * comparelist)
{
  if (comparelist == NULL)
    return NULL;
  else
    return comparelist->compare_top;
}


/* Get next condition from comparelist */
struct compare_data *
comparelist_get_next_compare(struct compare_list_data * comparelist, struct compare_data * cur_compare)
{
  if (cur_compare == NULL)
    return NULL;
  else
    return cur_compare->next;
}


/* Alloc new condition data */
struct compare_data *
compare_new_compare (void)
{
  return (struct compare_data *) malloc (sizeof(struct compare_data));
}


/* De-alloc condition data */
void
compare_free_compare (struct compare_data * compare)
{
  cond_exit(&(compare->cond));

  free ((void *) compare);
}



/* If one compare of a row */
int
compare_row_cond(struct compare_data * compare, struct row_data * row)
{
  int result;


  /* Debug */
  debug_message("Entering compare_row_cond\n");

  /* No if condition? Then both are equal. */
  if (compare == NULL)
    return FALSE;

  /* Safety net */
  if (compare_row == NULL)
    return FALSE;

  /* Check current condition for this row */
  result = cond_row(&(compare->cond), row);

  /* Statistics */
  compare->num_compared++;
  if (result)
    compare->num_success++;
  else
    compare->num_failure++;

  /* Debug */
  debug_message("Leaving compare_row_cond\n");

  return result;
}
