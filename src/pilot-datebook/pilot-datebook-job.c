/*
 * Pilot Datebook processing utility
 * Matthias Hessler <pilot-datebook@mhessler.de> December 2000
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

          
#include "pilot-datebook.h"
#include "pilot-datebook-data.h"

#include "pilot-datebook-io.h"
#include "pilot-datebook-sort.h"
#include "pilot-datebook-update.h"



/* Public functions */

/* Alloc new job data */
struct job *
job_new (enum JOB_TYPE job_type)
{
  struct job * new_job;


  /* Malloc */
  new_job = (struct job *) malloc (sizeof(struct job));

  /* Init */
  if (new_job != NULL) {
    new_job->type = job_type;
    new_job->state = JOB_INITIATED;
    new_job->record_num = 0;
  }
  else
    error_message("Not enough memory to create new job!\n");

  return new_job;
}

/* De-alloc job data */
void
job_free (struct job * job)
{
  free ((void *) job);
}


/* Initiate job data structures */
struct job *
job_init (enum JOB_TYPE job_type, char * opt_arg)
{
  int rc = FALSE;
  struct job * job;


  /* Debug */
  debug_message("Entering job_init\n");

  /* Alloc & Init */
  job = job_new(job_type);

  switch(job_type)
    {
    case COMPARE_JOB:
      rc = compare_init(&(job->job_data.compare), opt_arg);
      break;
    case READ_JOB:
      rc = io_init_read(&(job->job_data.io), opt_arg);
      break;
    case WRITE_JOB:
      rc = io_init_write(&(job->job_data.io), opt_arg);
      break;
    case SORT_JOB:
      rc = sort_init(&(job->job_data.sort), opt_arg);
      break;
    case UPDATE_JOB:
      rc = update_init(&(job->job_data.update), opt_arg);
      break;
    case DEL_JOB:
      /* Nothing to do to initiate delete job */
      rc = TRUE;
      break;
    default:
      error_message("Can not create job type <%d>\n", job_type);
    }

  /* Remove job on failure */
  if (rc == FALSE) {
    job_free(job);
    job = NULL;
  }

  return job;
}


/* Set job parameters */
int
job_set_param(struct job * job, char opt, char * opt_arg)
{
  int rc = FALSE;


  /* Debug */
  debug_message("Entering job_set_param\n");

  switch(job->type)
    {
    case COMPARE_JOB:
      rc = compare_set_param(&(job->job_data.compare), opt, opt_arg);
      break;
    case READ_JOB:
      rc = io_set_read_option(&(job->job_data.io), opt, opt_arg);
      break;
    case WRITE_JOB:
      rc = io_set_write_option(&(job->job_data.io), opt, opt_arg);
      break;
    case SORT_JOB:
      rc = sort_set_param(&(job->job_data.sort), opt, opt_arg);
      break;
    case UPDATE_JOB:
      rc = update_set_param(&(job->job_data.update), opt, opt_arg);
      break;
    case DEL_JOB:
      /* Can not set parameter for delete job */
      rc = FALSE;
      break;
    default:
      error_message("Can not set option for job type <%d>\n", job->type);
    }

  /* No new job state */

  return rc;
}


/* Start job, process header */
void
job_start(struct job * job, struct header_data * header)
{
  int rc = FALSE;


  /* Debug */
  debug_message("Entering job_start\n");

  switch(job->type)
    {
    case COMPARE_JOB:
      /* Nothing to do to open condition */
      rc = TRUE;
      break;
    case READ_JOB:
      io_open_read(&(job->job_data.io), header);
      rc = TRUE;
      break;
    case WRITE_JOB:
      io_open_write(&(job->job_data.io), header);
      rc = TRUE;
      break;
    case SORT_JOB:
      /* Nothing to do to open sort */
      rc = TRUE;
      break;
    case UPDATE_JOB:
      /* Nothing to do to open update */
      rc = TRUE;
      break;
    case DEL_JOB:
      /* Nothing to do to open delete job */
      rc = TRUE;
      break;
    default:
      error_message("Can not start job type <%d>\n", job->type);
    }

  /* New job state if successful */
  if (rc)
    job->state = JOB_STARTED;
  else
    job->state = JOB_ABORTED;
}


/* Work on job for one row */
void
job_process(struct job * job, struct header_data * header, struct row_data * row)
{
  int rc = FALSE;


  /* Debug */
  debug_message("Entering job_process\n");

  switch(job->type)
    {
    case COMPARE_JOB:
      /* Statistics */
      if (!getRowIsValid(row)) {
	/* End of condition processing if no more rows to process */
	rc = TRUE;
      }
      else {
	job->record_num++;

	/* Update (OR is assumed for multiple comparisons) */
	if (getRowIsSelected(row) != DATEBOOK_SELECT_YES)
	  compare_row (&(job->job_data.compare), row);

	/* Continue with processing */
	rc = FALSE;
      }
      break;

    case READ_JOB:
      if (getRowIsValid(row)) {
	/* Inform user */
	debug_message("Skipping reading of input record, since input record is already valid\n");

	/* Continue with processing */
	rc = FALSE;
      }
      else if (io_read_eof(&(job->job_data.io))) {
	/* Inform user */
	debug_message("Skipping reading of input record, since no input record is present\n");

	/* Stop processing */
	rc = TRUE;
      }
      else {
	/* Inform user */
	debug_message("Reading input record #%d\n", job->record_num);

	/* Initialize row: destroy all existing row data */
	rowInit(row);

	/* Get new row */
	io_read_row(&(job->job_data.io), row);

	/* Update selection */
	setRowIsSelected(row, DATEBOOK_SELECT_UNKNOWN);

	/* Statistics */
	if (getRowIsValid(row))
	  job->record_num++;

	/* End of processing reached ? */
	rc = io_read_eof(&(job->job_data.io));
      }
      break;

    case WRITE_JOB:
      /* if record present => write, else skip */
      if (getRowIsValid(row)) {
	/* Inform user */
	info_message("Writing output record #%d\r", job->record_num);
	debug_message("\n");

	/* Write row to output file */
	if (getRowIsSelected(row) != DATEBOOK_SELECT_NO)
	  io_write_row(&(job->job_data.io), header, row);

	/* Update selection */
	setRowIsSelected(row, DATEBOOK_SELECT_UNKNOWN);

	/* Statistics */
	job->record_num++;

	/* End of processing not yet reached */
	rc = FALSE;
      }
      else {
	/* End of processing reached if no record found */
	debug_message("End of writing reached, received no record.\n");
	rc = TRUE;
      }
      break;

    case SORT_JOB:
      /* if record present => write, else read */
      if (getRowIsValid(row)) {
	/* Inform user */
	info_message("Sorting record #%d\r", job->record_num);
	debug_message("\n");

	/* Statistics */
	job->record_num++;

	/* Sort (ignore but conserve selection) */
	sort_write_row (&(job->job_data.sort), header, row);

	/* Now signal that record has been processed already */
	setRowIsValid(row, FALSE);
      }
      else {
	/* Inform user */
	debug_message("Reading sorted record #%d\n", job->record_num);

	/* Statistics */
	job->record_num++;

	/* get next row if not yet end of sort reading */
	rc = sort_read_eof(&(job->job_data.sort));
	if (!rc)
	  sort_read_row (&(job->job_data.sort), row);
      }
      break;

    case UPDATE_JOB:
      /* Inform user */
      debug_message("Updating record #%d\n", job->record_num);

      if (!getRowIsValid(row)) {
	/* End of update processing if no more rows to process */
	rc = TRUE;
      }
      else {
	/* Statistics */
	job->record_num++;

	/* Update */
	if (getRowIsSelected(row) != DATEBOOK_SELECT_NO)
	  update_row (&(job->job_data.update), row);

	/* Update selection */
	setRowIsSelected(row, DATEBOOK_SELECT_UNKNOWN);

	/* Continue with processing */
	rc = FALSE;
      }
      break;

    case DEL_JOB:
      /* Inform user */
      debug_message("Deleting record #%d\n", job->record_num);

      if (!getRowIsValid(row)) {
	/* End of update processing if no more rows to process */
	rc = TRUE;
      }
      else {
	/* Statistics */
	job->record_num++;

	/* Delete unless not selected */
	if (getRowIsSelected(row) != DATEBOOK_SELECT_NO) {
	  /* Delete row */
	  setRowIsValid(row, FALSE);

	  /* Update statistics */
	  job->job_data.del.records_deleted++;
	}
	else {
	  /* Update selection */
	  setRowIsSelected(row, DATEBOOK_SELECT_UNKNOWN);
	}

	/* Continue with processing */
	rc = FALSE;
      }
      break;

    default:
      error_message("Can not process job type <%d>\n", job->type);
    }

  /* New job state if successful */
  if (rc)
    job->state = JOB_PROCESSED;
}


/* Stop job at end of processing */
void
job_stop(struct job * job, struct header_data * header)
{
  int rc = FALSE;


  switch(job->type)
    {
    case COMPARE_JOB:
      /* Nothing to do to close condition */
      rc = TRUE;
      break;
    case READ_JOB:
      io_close_read(&(job->job_data.io), header);
      rc = TRUE;
      break;
    case WRITE_JOB:
      io_close_write(&(job->job_data.io), header);
      rc = TRUE;
      break;
    case SORT_JOB:
      /* Nothing to do to close sort */
      rc = TRUE;
      break;
    case UPDATE_JOB:
      /* Nothing to do to close update */
      rc = TRUE;
      break;
    case DEL_JOB:
      /* Nothing to do to close delete job */
      rc = TRUE;
      break;
    default:
      error_message("Can not stop job type <%d>\n", job->type);
    }

  /* New job state if successful */
  if (rc)
    job->state = JOB_STOPPED;
  else
    job->state = JOB_ABORTED;
}


/* Destroy job data structure */
void
job_exit(struct job * job)
{
  int rc = FALSE;


  /* Debug */
  debug_message("Entering job_exit\n");

  switch(job->type)
    {
    case COMPARE_JOB:
      compare_exit(&(job->job_data.compare));
      rc = TRUE;
      break;
    case READ_JOB:
      io_exit_read(&(job->job_data.io));
      rc = TRUE;
      break;
    case WRITE_JOB:
      io_exit_write(&(job->job_data.io));
      rc = TRUE;
      break;
    case SORT_JOB:
      sort_exit(&(job->job_data.sort));
      rc = TRUE;
      break;
    case UPDATE_JOB:
      update_exit(&(job->job_data.update));
      rc = TRUE;
      break;
    case DEL_JOB:
      /* Nothing to do to exit delete job */
      rc = TRUE;
      break;
    default:
      error_message("Can not exit job type <%d>\n", job->type);
    }

  /* New job state if successful */
  if (rc)
    job->state = JOB_EXITED;
  else
    job->state = JOB_ABORTED;
}


/* Stop job in case of error */
void
job_abort(struct job * job)
{
  int rc = FALSE;


  /* Debug */
  debug_message("Entering job_abort\n");

  switch(job->type)
    {
    case COMPARE_JOB:
      /* Nothing to do to abort condition */
      rc = TRUE;
      break;
    case READ_JOB:
      io_abort_read(&(job->job_data.io));
      rc = TRUE;
      break;
    case WRITE_JOB:
      io_abort_write(&(job->job_data.io));
      rc = TRUE;
      break;
    case SORT_JOB:
      /* Nothing to do to abort sort */
      rc = TRUE;
      break;
    case UPDATE_JOB:
      /* Nothing to do to abort update */
      rc = TRUE;
      break;
    case DEL_JOB:
      /* Nothing to do to abort delete job */
      rc = TRUE;
      break;
    default:
      error_message("Can not abort job type <%d>\n", job->type);
    }

  /* New job state */
  job->state = JOB_ABORTED;
}


/* Destroy job data structure */
void
job_statistics(struct job * job)
{
  int rc = FALSE;


  /* Debug */
  debug_message("Entering job_statistics\n");

  switch(job->type)
    {
    case COMPARE_JOB:
      compare_show_statistics(&(job->job_data.compare));
      rc = TRUE;
      break;
    case READ_JOB:
      io_show_read_statistics(&(job->job_data.io));
      rc = TRUE;
      break;
    case WRITE_JOB:
      io_show_write_statistics(&(job->job_data.io));
      rc = TRUE;
      break;
    case SORT_JOB:
      sort_show_statistics(&(job->job_data.sort));
      rc = TRUE;
      break;
    case UPDATE_JOB:
      update_show_statistics(&(job->job_data.update));
      rc = TRUE;
      break;
    case DEL_JOB:
      /* Show statistics */
      info_message("Deleting:\n");
      info_message("Records deleted: %ld\n",
		   job->job_data.del.records_deleted);
      rc = TRUE;
      break;
    default:
      error_message("Can not show statistics on job type <%d>\n", job->type);
    }
}



/* Helper functions */

/* Alloc new row data */
struct row_data *
new_row (void)
{
  struct row_data * new_row;


  new_row = (struct row_data *) malloc (sizeof(struct row_data));
  if (new_row != NULL)
    setRowIsValid(new_row, FALSE);

  /* rowInit() has to be called separately */

  return new_row;
}


/* de-alloc row data */
void
free_row (struct row_data * row)
{
  rowExit(row);
  free ((void *) row);
}
