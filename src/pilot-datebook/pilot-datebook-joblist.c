/*
 * Pilot Datebook processing utility
 * Matthias Hessler <pilot-datebook@mhessler.de> December 2000
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

          
#include "pilot-datebook-joblist.h"

#include "pilot-datebook.h"

#include "pilot-datebook-io.h"
#include "pilot-datebook-sort.h"


/* Internally used 'global' variables
 * (joblist_abort_all() needs access to this to close all open files)
 */
struct job_list * joblist_joblist;



/* Public functions */

/* Init data structure */
void
joblist_init (struct job_list * joblist)
{

  /* Init */
  joblist->top = NULL;
  joblist->last = NULL;

  /* Store joblist for retrieval on joblist_abort_all() */
  joblist_joblist = joblist;
}


/* Destroy data structure */
void
joblist_exit (struct job_list * joblist)
{
  struct job * job;


  /* Debug */
  debug_message("Entering joblist_exit\n");

  /* Now process all jobs */
  job = joblist_get_first(joblist);
  while (job != NULL) {
    /* Process this job */
    debug_message("Exiting job type <%d>\n", job->type);

    /* Stop program if any job has not been properly terminated */
    if (job->state == JOB_EXITED
	|| job->state == JOB_ABORTED) {
      /* Do nothing, job has been properly terminated */
    }
    else if (job->state == JOB_STOPPED) {
      /* Destroy job */
      job_exit (job);
    }
    else {
      /* Something did not work properly, if we see any other job state */
      joblist_abort_all();
    }

    /* Remove job from joblist */
    joblist_del(joblist, job);

    /* Destroy job data structure */
    job_free(job);

    /* Next = now first (after removal of previous first) */
    job = joblist_get_first(joblist);
  } /* while */

  /* Destroy data structure */
  joblist->top = NULL;
  joblist->last = NULL;

  /* Debug */
  debug_message("Leaving joblist_exit\n");
}


/* Close all open files in case of an error */
void
joblist_abort_all (void)
{
  struct job * job;
  struct job_list * joblist;


  /* Debug */
  debug_message("Entering joblist_abort_all\n");

  /* Init from global data storage */
  joblist = joblist_joblist;

  /* Now process all jobs */
  job = joblist_get_first(joblist);
  while (job != NULL) {
    /* Process this job */
    debug_message("Aborting job type <%d>\n", job->type);

    /* Stop program if any job has not been properly terminated */
    if (job->state == JOB_EXITED
	|| job->state == JOB_ABORTED) {
      /* Do nothing, job has been properly terminated */
    }
    else if (job->state == JOB_INITIATED) {
      /* Do nothing, job has not yet started */
    }
    else {
      /* Destroy job */
      job_abort (job);
    }

    /* Remove job from joblist */
    joblist_del(joblist, job);

    /* Destroy job data structure */
    job_free(job);

    /* Next = now first (after removal of previous first) */
    job = joblist_get_first(joblist);
  } /* while */

  /* Destroy data structure */
  joblist->top = NULL;
  joblist->last = NULL;

  /* Debug */
  debug_message("Leaving joblist_abort_all\n");
}


/* Statistics */
void
joblist_statistics (struct job_list * joblist)
{
  struct job * job;


  /* Debug */
  debug_message("Entering joblist_statistics\n");

  /* Write general statistics */
  info_message("\n");
  info_message("Statistics:\n");

  /* Now process all jobs */
  job = joblist_get_first(joblist);
  while (job != NULL) {
    /* Process this job */
    debug_message("Writing statistics for job type <%d>\n", job->type);

    /* Write statistics if data structure not yet destroyed */
    if (job->state != JOB_EXITED
	&& job->state != JOB_ABORTED)
      job_statistics(job);

    info_message("\n");

    /* Next job */
    job = joblist_get_next(joblist, job);
  } /* while */

  /* Debug */
  debug_message("Leaving joblist_statistics\n");
}


/* Read command line, build job list */
int
joblist_set_param (struct job_list * joblist, int argc, char **argv)
{
  char * progname;


  /* Debug */
  debug_message("Entering joblist_set_param\n");

  /* Init */
  joblist_init(joblist);

  /* Under which name have we been called */
  progname = argv[0];

  /* Setting default processing options */
  /* verbose level 2 => show errors + warnings + information */
  set_message_verbose(MESSAGE_VERBOSE_INFO);

  /* Parse all command line options */
  joblist_parse_param(joblist, argc, argv, progname);

  /* Debug */
  debug_message("Leaving joblist_set_param\n");

  /* exit */
  return TRUE;
}


/* Parse command line options */
void
joblist_parse_param (struct job_list * joblist, int argc, char **argv, char * progname)
{
  struct job * cur_job;
  char c;
  int optind2;


  /* This function can be called multiple times for parameter files! */

  /* Debug */
  debug_message("Entering joblist_parse_param\n");

  /* Read all command line options */
  cur_job = NULL;
  optind = 0;
  while ((c = getopt (argc, argv, "-qvr:w:s:u:di:h::?f:o:l:t:")) != EOF) {

    switch (c) {
      /* General options */
    case 'q':
      set_message_verbose(MESSAGE_VERBOSE_WARN);
      break;
    case 'v':
      set_message_verbose(MESSAGE_VERBOSE_DEBUG);
      break;
    case '?':
      usage (progname, "");
      break;
    case 'h':
      usage (progname, optarg);
      break;

      /* Commands */
    case 'd':
      /* Update job */
      joblist_add(joblist, cur_job);
      cur_job = job_init(DEL_JOB, optarg);
      if (cur_job == NULL)
	usage (progname, "");
      break;
    case 'i':
      /* Compare job */
      joblist_add(joblist, cur_job);
      cur_job = job_init(COMPARE_JOB, optarg);
      if (cur_job == NULL)
	usage (progname, "");
      break;
    case 'r':
      /* Read job */
      joblist_add(joblist, cur_job);
      cur_job = job_init(READ_JOB, optarg);
      if (cur_job == NULL)
	usage (progname, "");
      break;
    case 's':
      /* Sort job */
      joblist_add(joblist, cur_job);
      cur_job = job_init(SORT_JOB, optarg);
      if (cur_job == NULL)
	usage (progname, "");
      break;
    case 'u':
      /* Update job */
      joblist_add(joblist, cur_job);
      cur_job = job_init(UPDATE_JOB, optarg);
      if (cur_job == NULL)
	usage (progname, "");
      break;
    case 'w':
      /* Write job */
      /* Uncomment the following lines to buffer all input reading by default.
       * After adding an unsorted sort_job, writing will only take place after
       * all reading has been finished. This behavior can also be enforced
       * by providing  -s ""  as command line option between reading
       * and writing.
       joblist_add(joblist, cur_job);
       cur_job = job_init(SORT_JOB, "");
       */

      joblist_add(joblist, cur_job);
      cur_job = job_init(WRITE_JOB, optarg);
      if (cur_job == NULL)
	usage (progname, "");
      break;

      /* Command specific options */
    case 'f':
      if (cur_job == NULL) {
	error_message("-f can only be used together with -r or -w\n");
      }
      else {
	/* Filename (command specific option) */
	if (optarg[0] == '-'
	    && optarg[1] == '\0') {
	  /* Read from stdin/write to stdout */
	  job_set_param(cur_job, c, "");
	}
	else {
	  job_set_param(cur_job, c, optarg);
	}
      }
      break;
    case 'o':
      if (cur_job == NULL) {
	error_message("-o can only be used together with -r or -w for csv output\n");
      }
      else {
	/* Filename (command specific option) */
	job_set_param(cur_job, c, optarg);
      }
      break;
    case 'l':
      if (cur_job == NULL) {
	error_message("-l can only be used together with -r or -w for windat output\n");
      }
      else {
	/* Filename (command specific option) */
	job_set_param(cur_job, c, optarg);
      }
      break;
    case 't':
      if (cur_job == NULL) {
	error_message("-%c can only be used together with -r or -w for csv output\n", c);
      }
      else {
	/* Filename (command specific option) */
	job_set_param(cur_job, c, optarg);
      }
      break;

    case 1:
      if (optarg[0] == '@') {
	joblist_add(joblist, cur_job);
	cur_job=NULL;
	/* Have to store optind, since it is a global variable, and
	 * joblist_read_param_file will recursively call this function!
	 */
	optind2=optind;
	joblist_read_param_file(joblist, optarg +1, progname);
	optind=optind2;
      }
      else {
	fprintf (stderr, "Do not understand option <%s>!\n", optarg);
	usage (progname, "");
      }
      break;

    default:
      fprintf (stderr, "Unknown option <%c>!\n", c);
      usage (progname, "");
    }
  } /* while */
  /* Add last job */
  joblist_add(joblist, cur_job);
  cur_job = NULL;

  /* Not enough parameters? */
  if (optind > argc)
    {
      fprintf (stderr, "Got too many input options!\n");
      usage (progname, "");
    }

  while (optind < argc) {
    /* Got extra (non-option) parameter */

    /* Check whether we should read in a parameter file */
    if (argv[optind][0] == '@') {
      /* Have to store optind, since it is a global variable, and
       * joblist_read_param_file will recursively call this function!
       */
      optind2=optind;
      joblist_read_param_file(joblist, argv[optind2] +1, progname);
      optind=optind2;
    }
    else {
      usage (progname, "");
    }

    /* Next remaining parameter */
    optind++;
  } /* while */

  /* Do we understand all passed options */
  if (optind != argc)
    usage (progname, "");

  /* Debug */
  debug_message("Leaving joblist_parse_param\n");
}


/* Read in parameter file */
void
joblist_read_param_file(struct job_list * joblist, char * filename, char * progname)
{
  FILE *in_file;
  int line_no;
  char buffer[0xffff];

  char * pos1;
  int found_option;
  int argc1;
  int argc2;
  char ** argv;


  /* Debug */
  debug_message("Entering joblist_read_param_file\n");

  debug_message("Reading param file <%s>\n", filename);

  in_file = fopen(filename, "r");
  if (!in_file)
    error_message("Can not open param file <%s> for reading\n",
		  filename);

  /* Read first line */
  line_no = 0;
  fgets(buffer, sizeof(buffer), in_file);

  /* Process each line */
  while (!feof(in_file)) {
    /* Update line number */
    line_no++;

    /* Have we read a terminating newline? */
    if (buffer[strlen(buffer) -1] != '\n')
      error_message("Line <%d> in param file <%s> exceeds buffer size\n",
		    line_no,
		    filename);

    /* Skip comments and empty lines */
    if (buffer[0] != '\0'
	&& buffer[0] != '\n'
	&& buffer[0] != '#') {

      /* Process line */
      debug_message("Processing line <%d> from param file <%s>\n",
		    line_no,
		    filename);

      /* Count number of options */
      argc1 = 1;
      pos1 = buffer;
      found_option = FALSE;
      while (*pos1 != '\0') {
	if (isspace(*pos1)) {
	  /* Space */
	  if (found_option == TRUE)
	    found_option = FALSE;
	}
	else {
	  /* Non-space */
	  if (found_option == FALSE) {
	    /* New option found */
	    argc1++;
	    found_option = TRUE;

	    if (*pos1 == '"') {
	      /* Skip over double quotes, take backslash into account */
	      pos1++;
	      while (*pos1 != '\0'
		     && (*pos1 != '"'
			 || *(pos1-1) == '\\'))
		pos1++;
	    }
	    else if (*pos1 == '\'') {
	      /* Skip over single quotes, take backslash into account */
	      pos1++;
	      while (*pos1 != '\0'
		     && (*pos1 != '\''
			 || *(pos1-1) == '\\'))
		pos1++;
	    }
	  } /* if new option found */
	  else {
	    /* No new option found */
	    if (*pos1 == '"') {
	      /* Skip over double quotes, take backslash into account */
	      pos1++;
	      while (*pos1 != '\0'
		     && (*pos1 != '"'
			 || *(pos1-1) == '\\'))
		pos1++;
	    }
	    else if (*pos1 == '\'') {
	      /* Skip over single quotes, take backslash into account */
	      pos1++;
	      while (*pos1 != '\0'
		     && (*pos1 != '\''
			 || *(pos1-1) == '\\'))
		pos1++;
	    }
	  } /* else no new option found */
	} /* else non-space */

	/* Next character */
	pos1++;
      } /* while */

      /* Now allocate enough memory to hold all options
       * (malloc introduces a certain overhead here, but parameter file
       * reading is not a performance critical task; the advantage is
       * that we can have arbitrary many options on one line)
       */
      argv=calloc(argc1, sizeof(char *));
      argv[0]=progname;

      /* Read options into argv */
      argc2 = 1;
      pos1 = buffer;
      found_option = FALSE;
      while (*pos1 != '\0') {
	if (isspace(*pos1)) {
	  /* Space */
	  if (found_option == TRUE) {
	    found_option = FALSE;
	    /* Terminate option string */
	    *pos1='\0';
	  }
	}
	else {
	  /* Non-space */
	  if (found_option == FALSE) {
	    /* New option found */
	    found_option = TRUE;
	    if (argc2 == argc1)
	      error_message("argc process count exceeds argc count in line <%d> of param file <%s>:<%s>,%d,%d\n",
			    line_no,
			    filename, pos1, argc1,argc2);

	    /* Handle quoted options */
	    if (*pos1 == '"') {
	      /* Skip over double quotes */
	      pos1++;

	      /* Store position after begin of quote */
	      argv[argc2]=pos1;
	      argc2++;

	      /* Terminate string at end of quote */
	      while (*pos1 != '\0'
		     && (*pos1 != '"'
			 || *(pos1-1) == '\\'))
		pos1++;
	      if (*pos1 == '"')
		*pos1='\0';
	    }
	    else if (*pos1 == '\'') {
	      /* Skip over single quotes */
	      pos1++;

	      /* Store position after begin of quote */
	      argv[argc2]=pos1;
	      argc2++;

	      /* Terminate string at end of quote */
	      while (*pos1 != '\0'
		     && (*pos1 != '\''
			 || *(pos1-1) == '\\'))
		pos1++;
	      if (*pos1 == '\'')
		*pos1='\0';
	    }
	    else {
	      /* Store position */
	      argv[argc2]=pos1;
	      argc2++;
	    }
	  } /* if new option found */
	  else {
	    /* No new option found */
	    if (*pos1 == '"') {
	      /* Skip over double quotes */
	      pos1++;
	      while (*pos1 != '\0'
		     && (*pos1 != '"'
			 || *(pos1-1) == '\\'))
		pos1++;
	    }
	    if (*pos1 == '\'') {
	      /* Skip over single quotes */
	      pos1++;
	      while (*pos1 != '\0'
		     && (*pos1 != '\''
			 || *(pos1-1) == '\\'))
		pos1++;
	    }
	  } /* else no new option found */
	} /* else non-space */

	/* Next character */
	pos1++;
      } /* while */

      /* Safety check */
      if (argc2 != argc1)
	error_message("argc count differs from process count in line <%d> of param file <%s>\n",
		      line_no,
		      filename);

      /* Parse all options read from param file */
      joblist_parse_param(joblist, argc1, argv, progname);

      /* Free memory */
      free(argv);
      argv = NULL;
    }
    else {
      debug_message("Skipping line <%d> from param file <%s>\n",
		    line_no,
		    filename);
    }

    /* Read next line */
    buffer[0]='\0';
    fgets(buffer, sizeof(buffer), in_file);
  }

  if (fclose(in_file))
    warn_message("Can not close param file <%s> after reading\n",
		 filename);

  /* Debug */
  debug_message("Leaving joblist_read_param_file\n");
}


/* Append one job to joblist */
void
joblist_add(struct job_list * joblist, struct job * add_job)
{

  /* Debug */
  debug_message("Entering joblist_add\n");

  /* Add to list */
  if (add_job == NULL) {
    /* No job provided => nothing to add */
  }
  else if (joblist == NULL) {
    /* No joblist provided => nothing to add */
  }
  else if (joblist->last == NULL) {
    /* First entry into list */
    add_job->next = NULL;
    joblist->last = add_job;
    joblist->top = add_job;
  }
  else {
    /* other entries exist already */
    add_job->next = NULL;
    joblist->last->next = add_job;
    joblist->last = add_job;
  }

  /* Debug */
  debug_message("Leaving joblist_add\n");
}


/* Remove one job from joblist */
void
joblist_del(struct job_list * joblist, struct job * del_job)
{

  /* Debug */
  debug_message("Entering joblist_del\n");

  /* Remove from list */
  if (del_job == NULL) {
    /* No job provided => nothing to remove */
  }
  else if (joblist == NULL) {
    /* No joblist provided => nothing to remove */
  }
  else if (joblist->last == NULL) {
    /* Nothing in list => nothing to remove */
  }
  else {
    if (joblist->last == del_job) {
      /* Last in list => iterate through list to find new last */
      if (joblist->top != del_job)
	debug_message("Removing last - inefficient use of joblist_del\n");
      joblist->last = joblist->top;
      while (joblist->last->next != NULL
	     && joblist->last->next != del_job) {
	joblist->last = joblist->last->next;
      } /* while */
    }
    if (joblist->top == del_job) {
      /* First in list => remove */
      joblist->top = del_job->next;
    }
  }

  /* Remove linkage */
  del_job->next = NULL;

  /* Debug */
  debug_message("Leaving joblist_del\n");
}


/* Get first job from joblist */
struct job *
joblist_get_first(struct job_list * joblist)
{
  if (joblist == NULL)
    return NULL;
  else
    return joblist->top;
}


/* Get next job from joblist */
struct job *
joblist_get_next(struct job_list * joblist, struct job * cur_job)
{

  if (cur_job == NULL)
    return NULL;
  else
    return cur_job->next;
}


/* Process all jobs contained in joblist until state STOPPED */
void
joblist_process (struct job_list * joblist)
{
  struct header_data header;
  struct row_data row;
  struct job * job;


  /* Debug */
  debug_message("Entering joblist_process\n");

  /* Init */
  header.isValid = FALSE;
  setRowIsValid(&row, FALSE);

  /* Now process all jobs */
  job = joblist_get_first(joblist);
  while (job != NULL) {
    /* Process this job */
    debug_message("Processing type <%d>, state <%d>, rowIsValid <%d>, headerIsValid <%d>\n",
		  job->type, job->state, row.isValid, header.isValid);

    /* Start up job until job_process has been called at least once,
     * then decide whether to pass row data on to next job, or start
     * to read from first not-yet finished job.
     */
    switch (job->state)
      {
      case JOB_INITIATED:
	job_start(job, &header);
	/* Start up current job before processing */
	break;
      case JOB_STARTED:
	job_process(job, &header, &row);
	/* If row is valid and next job available, go to next job;
	 * else erase row and go to first job */
	if (getRowIsValid(&row)
	    && joblist_get_next(joblist, job) != NULL) {
	  job = joblist_get_next(joblist, job);
	}
	else {
	  /* Row got consumed or could not be created
	   * => start again with first job
	   */
	  setRowIsValid(&row, FALSE);
	  job = joblist_get_first(joblist);
	}
	break;
      case JOB_PROCESSED:
	/* Close down current job */
	job_stop(job, &header);
	break;
      case JOB_STOPPED:
	/* Skip; need to show statistics before exiting */
	job = joblist_get_next(joblist, job);
	break;
      case JOB_EXITED:
	/* Skip; process has already successfully terminated */
	job = joblist_get_next(joblist, job);
	break;
      case JOB_ABORTED:
	/* Stop program if any job was aborted */
	joblist_abort_all();
	break;
      default:
	error_message("Encountered invalid job state <%d>\n", job->state);
      }
  } /* while */

  /* Destroy row data structure */
  if (getRowIsValid(&row))
    rowExit(&row);

  /* Debug */
  debug_message("Leaving joblist_process\n");
}

