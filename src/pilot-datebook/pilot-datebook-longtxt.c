/*
 * Pilot Datebook processing utility
 *
 * (c) 2000, Matthias Hessler <pilot-datebook@mhessler.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
 
#include <string.h>
#include "pilot-datebook-longtxt.h"

/* longtxt is a lossless file format, and designed to be human-readable.
 * Since it may change over time, it is not guaranteed to read in files
 * written in earlier versions of longtxt.
 *
 * It is actually the best output format, as it will capture even more data
 * than can be stored in pdb files (fields index, misc_flag).
 * Therefore best results for reading are achieved when reading via hotsync
 * and writing longtxt.
 */


/* Read/write header format */
/* Due to usage of scanf, can not suround strings with any delimiters
 * other than white space or end of line
 */
const char LONGTXT_HEADER_NAME_FORMAT[] = "name=%s\n";
const char LONGTXT_HEADER_FLAGS_FORMAT[] = "flags=0x%x%s%s%s%s%s%s%s\n";
const char LONGTXT_HEADER_MISC_FLAGS_FORMAT[] = "misc_flags=0x%x\n";
const char LONGTXT_HEADER_TYPE_FORMAT[] = "type=%s\n";
const char LONGTXT_HEADER_CREATOR_FORMAT[] = "creator=%s\n";
const char LONGTXT_HEADER_VERSION_FORMAT[] = "version=%d\n";
const char LONGTXT_HEADER_MODIFICATION_FORMAT[] = "modification_number=%ld\n";
const char LONGTXT_HEADER_CREATE_TIME_FORMAT[] = "creation_time=%s%s%s%s\n";
const char LONGTXT_HEADER_MODIFY_TIME_FORMAT[] = "modified_time=%s%s%s%s\n";
const char LONGTXT_HEADER_BACKUP_TIME_FORMAT[] = "backup_time=%s%s%s%s\n";
const char LONGTXT_HEADER_INDEX_FORMAT[] = "index=%u\n";
const char LONGTXT_HEADER_CATEGORY_HEADER1_FORMAT[] = "Categories\n";
const char LONGTXT_HEADER_CATEGORY_HEADER2_FORMAT[] = "#\tID\tRenamed\tName\n";
const char LONGTXT_HEADER_CATEGORY_ROW_FORMAT[] = "%d\t%hhu\t%s\t%s%s\n";
const char LONGTXT_HEADER_RENAMED_YES[] = "Yes";
const char LONGTXT_HEADER_RENAMED_NO[] = "No";
const char LONGTXT_HEADER_RENAMED_UNUSED[] = "(unused)";

const char LONGTXT_HEADER_LAST_UNIQUE_ID_FORMAT[] = "Last unique ID=%hhu\n";
const char LONGTXT_HEADER_START_OF_WEEK_FORMAT[] = "Start of week: %d\n";
const char LONGTXT_HEADER_NO_APP_INFO[] = "No application info found.\n";
const char LONGTXT_HEADER_NO_SORT_INFO[] = "No sort info found.\n";


/* Read/write row format */
/* Due to more manual processing, data fields are surrounded by
 * greater/less than signs, like this: <data_field>
 */
const char LONGTXT_ROW_UNTIMED1_FORMAT[] = "UNTIMED_EVENT <";
const char LONGTXT_ROW_UNTIMED2_FORMAT[] = "> on <";
const char LONGTXT_ROW_UNTIMED3_FORMAT[] = ">:";
const char LONGTXT_ROW_APPOINTMENT1_FORMAT[] = "APPOINTMENT <";
const char LONGTXT_ROW_APPOINTMENT2_FORMAT[] = "> from <";
const char LONGTXT_ROW_APPOINTMENT3_FORMAT[] = "> to <";
const char LONGTXT_ROW_APPOINTMENT4_FORMAT[] = ">:";
const char LONGTXT_ROW_ATTRIBUTES_FORMAT[] = "attributes=0x%x%s%s%s%s%s\n";
const char LONGTXT_ROW_CATEGORY_FORMAT[] = "category=%d (%s)\n";

const char LONGTXT_ROW_REPEAT1_FORMAT[] = "REPEAT from <";
const char LONGTXT_ROW_REPEAT2_FORMAT[] = "> until <";
const char LONGTXT_ROW_REPEAT3_FORMAT[] = ">: <";
const char LONGTXT_ROW_REPEAT4_FORMAT[] = ">";
const char LONGTXT_ROW_REPEAT_FOREVER_CONST[] = "forever";
const char LONGTXT_ROW_REPEAT_DAILY_CONST[] = "daily";
const char LONGTXT_ROW_REPEAT_WEEKLY_CONST[] = "weekly";
const char LONGTXT_ROW_REPEAT_MONTHLY_LAST_WEEK_CONST[] = "monthly/last_weekday";
const char LONGTXT_ROW_REPEAT_MONTHLY_WEEKDAY_CONST[] = "monthly/weekday";
const char LONGTXT_ROW_REPEAT_MONTHLY_CONST[] = "monthly";
const char LONGTXT_ROW_REPEAT_YEARLY_CONST[] = "yearly";
const char LONGTXT_ROW_REPEAT_UNKNOWN_CONST[] = "unknown";

const char LONGTXT_ROW_REPEAT_WEEKLY_FORMAT[] = "%s%s%s%s%s%s%s";
const char LONGTXT_ROW_REPEAT_MONTHLY_FORMAT[] = "%d";
const char LONGTXT_ROW_REPEAT_MONTHLY_LAST_WEEK_FORMAT[] = "%s %d";
const char LONGTXT_ROW_REPEAT_MONTHLY_WEEKDAY_FORMAT[] = "%s ge %d";

const char LONGTXT_ROW_REPEAT_YEARLY_FORMAT[] = "%s %d";
const char LONGTXT_ROW_REPEAT_PARAM1_FORMAT[] = " on <";
const char LONGTXT_ROW_REPEAT_PARAM2_FORMAT[] = ">";
const char LONGTXT_ROW_REPEAT_WEEKSTART1_FORMAT[] = " week starts <";
const char LONGTXT_ROW_REPEAT_WEEKSTART2_FORMAT[] = ">";

const char LONGTXT_ROW_REPEAT_FREQUENCY1_FORMAT[] = " every <";
const char LONGTXT_ROW_REPEAT_FREQUENCY2_FORMAT[] = "> times";

const char LONGTXT_ROW_REPEAT_OMIT1_FORMAT[] = "  OMIT: <";
const char LONGTXT_ROW_REPEAT_OMIT2_FORMAT[] = "> <";
const char LONGTXT_ROW_REPEAT_OMIT3_FORMAT[] = ">";

const char LONGTXT_ROW_ALARM1_FORMAT[] = "ALARM <";
const char LONGTXT_ROW_ALARM2_FORMAT[] = "%d %s before>";
const char LONGTXT_ROW_ALARM_MIN_CONST[] = "min";
const char LONGTXT_ROW_ALARM_HOURS_CONST[] = "hours";
const char LONGTXT_ROW_ALARM_DAYS_CONST[] = "days";
const char LONGTXT_ROW_ALARM_UNKNOWN_CONST[] = "unknown_unit";

const char LONGTXT_ROW_NOTE_FORMAT[] = "NOTE:\n";



/* Public functions */

/* For init */

/* Initialize read data structure */
int
longtxt_init_read (struct longtxt_file_data * in_file)
{

  /* Debug */
  debug_message("Entering longtxt_init_read\n");

  /* Init own data structure */
  in_file->filename = NULL;
  in_file->file = NULL;
  in_file->file_is_open = FALSE;
  in_file->line_buffer = NULL;
  in_file->line_no = 0;
  in_file->num_recs = 0;
  in_file->next_rec = 0;
  in_file->lines_read = 0;
  in_file->lines_written = 0;
  in_file->records_read = 0;
  in_file->records_written = 0;

  /* Debug */
  debug_message("Leaving longtxt_init_read\n");

  return TRUE;
}


/* Initialize write data structure */
int
longtxt_init_write (struct longtxt_file_data * out_file)
{

  /* Debug */
  debug_message("Entering longtxt_init_write\n");

  /* Init own data structure */
  out_file->filename = NULL;
  out_file->file = NULL;
  out_file->file_is_open = FALSE;
  out_file->line_buffer = NULL;
  out_file->line_no = 0;
  out_file->num_recs = 0;
  out_file->next_rec = 0;
  out_file->lines_read = 0;
  out_file->lines_written = 0;
  out_file->records_read = 0;
  out_file->records_written = 0;

  /* Debug */
  debug_message("Leaving longtxt_init_write\n");

  return TRUE;
}


/* Destroy read data structure */
void
longtxt_exit_read (struct longtxt_file_data * in_file)
{

  /* Debug */
  debug_message("Entering longtxt_exit_read\n");

  /* Free memory */
  if (in_file->filename)
    free (in_file->filename);

  /* Debug */
  debug_message("Leaving longtxt_exit_read\n");
}


/* Destroy write data structure */
void
longtxt_exit_write (struct longtxt_file_data * out_file)
{

  /* Debug */
  debug_message("Entering longtxt_exit_write\n");

  /* Free memory */
  if (out_file->filename)
    free (out_file->filename);

  /* Debug */
  debug_message("Leaving longtxt_exit_write\n");
}


/* Set read command line option */
int
longtxt_set_read_option (struct longtxt_file_data * in_file, char opt, char * opt_arg)
{
  int rc = FALSE;


  /* Debug */
  debug_message("Entering longtxt_set_read_option\n");

  switch (opt)
    {
    case 'f':
      /* Filename */
      if (opt_arg != NULL
	  && *opt_arg != '\0') {
	if (in_file->filename)
	  free (in_file->filename);
	in_file->filename = strdup(opt_arg);
	rc = TRUE;
      }
      break;
    default:
      fprintf(stderr, "Can not process read option <%c> for input file\n",
	      opt);
    }

  /* Debug */
  debug_message("Leaving longtxt_set_read_option\n");

  return rc;
}


/* Set write command line option */
int
longtxt_set_write_option (struct longtxt_file_data * out_file, char opt, char * opt_arg)
{
  int rc = FALSE;


  /* Debug */
  debug_message("Entering longtxt_set_write_option\n");

  switch (opt)
    {
    case 'f':
      /* Filename */
      if (opt_arg != NULL
	  && *opt_arg != '\0') {
	if (out_file->filename)
	  free (out_file->filename);
	out_file->filename = strdup(opt_arg);
	rc = TRUE;
      }
      break;
    default:
      fprintf(stderr, "Can not process write option <%c> for input file\n",
	      opt);
    }

  /* Debug */
  debug_message("Leaving longtxt_set_write_option\n");

  return rc;
}



/* For opening & closing */

/* Open input data file for reading */
void
longtxt_open_read (struct longtxt_file_data * in_file, struct header_data * header)
{

  /* Debug */
  debug_message("Entering longtxt_open_read\n");

  /* Open file */
  /* Read from stdin if file name not given */
  if (!in_file->filename) {
    /* No open is needed when writing to stdout */
    in_file->file = stdin;
  }
  else {
    in_file->file = fopen (in_file->filename, "r");
    if (!in_file->file)
      error_message("Can not open file <%s> for reading\n",
		    in_file->filename);
    in_file->file_is_open = TRUE;
  }

  /* Init */
  in_file->line_buffer = NULL;
  in_file->line_no = 0;
  in_file->num_recs = 0;
  in_file->next_rec = 0;

  /* Read header data */
  longtxt_read_header (in_file, header);

  /* Debug */
  debug_message("Leaving longtxt_open_read\n");
}


/* Open output data file for writing */
void
longtxt_open_write (struct longtxt_file_data * out_file, struct header_data * header)
{

  /* Debug */
  debug_message("Entering longtxt_open_write\n");

  /* Open file */
  /* Write to stdout if file name not given or "-" */
  if (!out_file->filename) {
    /* No open is needed when writing to stdout */
    out_file->file = stdout;
  }
  else {
    out_file->file = fopen(out_file->filename, "w");
    if (!out_file->file)
      error_message("Can not open file <%s> for writing\n",
		    out_file->filename);
    out_file->file_is_open = TRUE;
  }

  /* Init */
  out_file->line_buffer = NULL;
  out_file->line_no = 0;
  out_file->num_recs = 0;
  out_file->next_rec = 0;

  /* Write header */
  longtxt_write_header (out_file, header);

  /* Debug */
  debug_message("Leaving longtxt_open_write\n");
}


/* Close input file at end of processing */
void
longtxt_close_read (struct longtxt_file_data * in_file, struct header_data * header)
{

  /* Debug */
  debug_message("Entering longtxt_close_read\n");

  /* Close file */
  if (in_file->file_is_open == TRUE) {
    if (fclose (in_file->file))
      warn_message("Can not close input file\n");
    in_file->file_is_open = FALSE;
  }

  /* Update statistics */
  in_file->lines_read = in_file->line_no;

  /* clear data structures */
  in_file->file = NULL;
  in_file->line_buffer = NULL;
  in_file->line_no = 0;
  in_file->num_recs = 0;
  in_file->next_rec = 0;

  /* Debug */
  debug_message("Leaving longtxt_close_read\n");
}


/* Close output file at end of processing */
void
longtxt_close_write (struct longtxt_file_data * out_file, struct header_data * header)
{

  /* Debug */
  debug_message("Entering longtxt_close_write\n");

  /* Close file */
  if (out_file->file_is_open == TRUE) {
    if (fclose (out_file->file))
      warn_message("Can not close output file\n");
    out_file->file_is_open = FALSE;
  }

  /* Update statistics */
  out_file->lines_written = out_file->line_no;

  /* Clear data structures */
  out_file->file = NULL;
  out_file->line_buffer = NULL;
  out_file->line_no = 0;
  out_file->num_recs = 0;
  out_file->next_rec = 0;

  /* Debug */
  debug_message("Leaving longtxt_close_write\n");
}


/* Close input file in case of an error */
void
longtxt_abort_read (struct longtxt_file_data * in_file)
{

  /* Debug */
  debug_message("Entering longtxt_abort_read\n");

  /* Close file */
  if (in_file->file_is_open == TRUE) {
    if (fclose (in_file->file))
      warn_message("Can not close input file\n");
    in_file->file_is_open = FALSE;
  }

  /* No special error processing needed */

  /* Update statistics */
  in_file->lines_read = in_file->line_no;

  /* clear data structures */
  in_file->file = NULL;
  in_file->line_buffer = NULL;
  in_file->line_no = 0;
  in_file->num_recs = 0;
  in_file->next_rec = 0;

  /* Debug */
  debug_message("Leaving longtxt_abort_read\n");
}


/* Close output file in case of an error */
void
longtxt_abort_write (struct longtxt_file_data * out_file)
{

  /* Debug */
  debug_message("Entering longtxt_abort_write\n");

  /* Close file */
  if (out_file->file_is_open == TRUE) {
    if (fclose (out_file->file))
      warn_message("Can not close output file\n");
    out_file->file_is_open = FALSE;
  }

  /* Remove incompletely written file */
  if (out_file->filename) {
    info_message("Removing incompletely written output file <%s>\n",
		 out_file->filename);
    if (out_file->filename)
      unlink(out_file->filename);
  }

  /* Update statistics */
  out_file->lines_written = out_file->line_no;

  /* Clear data structures */
  out_file->file = NULL;
  out_file->line_buffer = NULL;
  out_file->line_no = 0;
  out_file->num_recs = 0;
  out_file->next_rec = 0;

  /* Debug */
  debug_message("Leaving longtxt_abort_write\n");
}



/* For reading */

/* Identify end of file */
int
longtxt_read_eof (struct longtxt_file_data * in_file)
{

  /* Debug */
  debug_message("Entering & Leaving longtxt_read_eof\n");

  return feof(in_file->file);
}


/* read routine for appointment in current record */
void
longtxt_read_row (struct longtxt_file_data * in_file, struct row_data * row)
{
  int len = 0;
  int i = 0;
  int j = 0;

  const char * p1 = NULL;
  char * b1 = NULL;
  char * b2 = NULL;

  int num_read = 0;
  int mday = 0;
  int empty_lines = 0;

  int repeat_last_week = 0;
  char repeatparam[0xffff];

  char date_buffer[50];

  char buffer[0xffff];
  char buffer1[0xffff];
  char buffer2[0xffff];
  char buffer3[0xffff];
  char buffer4[0xffff];
  char buffer5[0xffff];

  time_t t;
  struct tm tm;
  struct Appointment a;
  int attributes = 0;
  int category = 0;
  unsigned long uid = 0;
  int record_num = 0;



  /* Debug */
  debug_message("Entering longtxt_read_row\n");

  /* Ideally, this should be reprogrammed to a real parser (maybe using
   * yacc or so). A parser would be much faster and safer.
   * Unfortunately, I don't know how to do that, so I had
   * to sweat through this hell here.
   * If someone else knows how to do it better...
   */


  /* Init data structures */
  memset(&a, 0, sizeof(a));
  memset(&tm, 0, sizeof(tm));


  /* Read record */

  /* Skip empty lines */
  longtxt_read_line(buffer, sizeof(buffer), in_file);
  while (buffer[0] == '\n') {
    longtxt_read_line(buffer, sizeof(buffer), in_file);
  }
  if (feof(in_file->file))
    error_message("Read unexpected end of input file at line %d\n\n",
		  in_file->line_no);

  /* Now parse; per default assume that reading will fail */
  setRowIsValid(row, FALSE);
  do {
    /* Now expect UNTIMED_EVENT or APPOINTMENT */
    b1 = buffer;
    p1 = LONGTXT_ROW_UNTIMED1_FORMAT;
    len = sizeof(LONGTXT_ROW_UNTIMED1_FORMAT) -1;
    if(!strncmp(b1, p1, len)) {
      b1 += len;

      /* Found UNTIMED_EVENT */
      a.event = 1;

      /* Read UID */
      uid = strtoul(b1, &b2, 0);
      b1 = b2;

      /* Find event date */
      p1 = LONGTXT_ROW_UNTIMED2_FORMAT;
      len = sizeof(LONGTXT_ROW_UNTIMED2_FORMAT) -1;
      if(strncmp(b1, p1, len)) {
	warn_message("Can not find event date from input file line %d\n\n",
		     in_file->line_no);
	break;
      }
      b1 += len;

      /* Read event date
       * (assuming here that the first character of the next text string will
       * always contain the separator)
       */
      b2 = data_index(b1, LONGTXT_ROW_UNTIMED3_FORMAT[0]);
      len = b2-b1;
      if (len >= sizeof(date_buffer))
	error_message("Date_buffer size: Can not parse event date from input file line %d\n\n",
		      in_file->line_no);
      strncpy(date_buffer, b1, len);
      date_buffer[len] = '\0';
      t = read_iso_date_str1 (date_buffer);
      if (t == -1) {
	warn_message("Can not parse start date from input file line %d\n\n",
		     in_file->line_no);
	break;
      }
      b1 = b2;
      a.begin = *localtime(&t);

      /* Find end of line */
      p1 = LONGTXT_ROW_UNTIMED3_FORMAT;
      len = sizeof(LONGTXT_ROW_UNTIMED3_FORMAT) -1;
      if(strncmp(b1, p1, len)) {
	warn_message("Unexpected end of line reading from input file line %d\n\n",
		     in_file->line_no);
	break;
      }
      b1+=len;
    }


    else {
      /* Check for Appointment */
      b1 = buffer;
      p1 = LONGTXT_ROW_APPOINTMENT1_FORMAT;
      len = sizeof(LONGTXT_ROW_APPOINTMENT1_FORMAT) -1;
      if(strncmp(b1, p1, len)) {
	warn_message("Could not parse input file line %d, expected begin of appointment\n\n",
		     in_file->line_no);
	break;
      }
      else {
	b1 += len;

	/* Found APPOINTMENT */
	a.event = 0;

	/* Read UID */
	uid = strtoul(b1, &b2, 0);
	b1 = b2;

	/* Find event start date & time */
	p1 = LONGTXT_ROW_APPOINTMENT2_FORMAT;
	len = sizeof(LONGTXT_ROW_APPOINTMENT2_FORMAT) -1;
	if(strncmp(b1, p1, len)) {
	  warn_message("Can not find event start date & time from input file line %d\n\n",
		       in_file->line_no);
	  break;
	}
	b1 += len;

	/* Read event start date & time
	 * (assuming here that the first character of the next text string will
	 * always contain the separator)
	 */
	b2 = data_index(b1, LONGTXT_ROW_APPOINTMENT3_FORMAT[0]);
	len = b2-b1;
	if (len >= sizeof(date_buffer))
	  error_message("Date_buffer size: Can not read event start date & time from input file line %d\n\n",
			in_file->line_no);
	strncpy(date_buffer, b1, len);
	date_buffer[len] = '\0';
	t = read_iso_time_str1 (date_buffer);
	if (t == -1) {
	  warn_message("Can not parse event start date & time time from input file line %d\n\n",
		       in_file->line_no);
	  break;
	}
	b1 = b2;
	a.begin = *localtime(&t);


	/* Find event end date & time */
	p1 = LONGTXT_ROW_APPOINTMENT3_FORMAT;
	len = sizeof(LONGTXT_ROW_APPOINTMENT3_FORMAT) -1;
	if(strncmp(b1, p1, len)) {
	  warn_message("Can not find event end date & time from input file line %d\n\n",
		       in_file->line_no);
	  break;
	}
	b1 += len;

	/* Read event end date & time
	 * (assuming here that the first character of the next text string will
	 * always contain the separator)
	 */
	b2 = data_index(b1, LONGTXT_ROW_APPOINTMENT4_FORMAT[0]);
	len = b2-b1;
	if (len >= sizeof(date_buffer))
	  error_message("Date_buffer size: Can not read event end date & time from input file line %d\n\n",
			in_file->line_no);
	strncpy(date_buffer, b1, len);
	date_buffer[len] = '\0';
	t = read_iso_time_str1 (date_buffer);
	if (t == -1) {
	  warn_message("Can not parse event end date & time from input file line %d\n\n",
		       in_file->line_no);
	  break;
	}
	b1 = b2;
	a.end = *localtime(&t);

	/* Find end of line */
	p1 = LONGTXT_ROW_APPOINTMENT4_FORMAT;
	len = sizeof(LONGTXT_ROW_APPOINTMENT4_FORMAT) -1;
	if(strncmp(b1, p1, len)) {
	  warn_message("Unexpected end of line reading from input file line %d\n\n",
		       in_file->line_no);
	  break;
	}
	b1+=len;
      } /* else Appointment */

    } /* else UNTIMED_EVENT or Appointment */
    
    /* Read attributes, if present */
    longtxt_read_line(buffer, sizeof(buffer), in_file);
    num_read = sscanf (buffer, LONGTXT_ROW_ATTRIBUTES_FORMAT,
		       &attributes,
		       buffer1,
		       buffer2,
		       buffer3,
		       buffer4,
		       buffer5);
    if (num_read > 0) {
      /* Get next line, if we successfully processed this line */
      longtxt_read_line(buffer, sizeof(buffer), in_file);
    }
    else {
      /* Could not read attributes? Then assume default */
      attributes = 0;
    }

    /* Read category, if present */
    num_read = sscanf (buffer, LONGTXT_ROW_CATEGORY_FORMAT,
		       &category,
		       buffer1);
    /* Ignoring buffer1 for now, which contains the category name
     * plus the closing paranthesis - could rebuild or verify category
     * index table from encountered category id and name
     */
    if (num_read > 0) {
      /* Get next line, if we successfully processed this line */
      longtxt_read_line(buffer, sizeof(buffer), in_file);
    }
    else {
      /* Could not read category? Then assume default */
      category = 0;
    }


    /* Read description until encountering REPEAT, ALARM, NOTE,
     * UNTIMED_EVENT, APPOINTMENT, or two empty lines.
     */
    buffer1[0] = '\0';
    empty_lines = 0;
    while (!feof(in_file->file)
	   && empty_lines < 2) {
      /* Test for REPEAT */
      b1 = buffer;
      p1 = LONGTXT_ROW_REPEAT1_FORMAT;
      len = sizeof(LONGTXT_ROW_REPEAT1_FORMAT) -1;
      if(!strncmp(b1, p1, len)) {
	break;
      }

      /* Test for ALARM */
      b1 = buffer;
      p1 = LONGTXT_ROW_ALARM1_FORMAT;
      len = sizeof(LONGTXT_ROW_ALARM1_FORMAT) -1;
      if(!strncmp(b1, p1, len)) {
	break;
      }

      /* Test for NOTE */
      b1 = buffer;
      p1 = LONGTXT_ROW_NOTE_FORMAT;
      len = sizeof(LONGTXT_ROW_NOTE_FORMAT) -1;
      if(!strncmp(b1, p1, len)) {
	break;
      }

      /* Test for UNTIMED_EVENT */
      b1 = buffer;
      p1 = LONGTXT_ROW_UNTIMED1_FORMAT;
      len = sizeof(LONGTXT_ROW_UNTIMED1_FORMAT) -1;
      if(!strncmp(b1, p1, len)) {
	break;
      }

      /* Test for APOINTMENT */
      b1 = buffer;
      p1 = LONGTXT_ROW_APPOINTMENT1_FORMAT;
      len = sizeof(LONGTXT_ROW_APPOINTMENT1_FORMAT) -1;
      if(!strncmp(b1, p1, len)) {
	break;
      }

      /* If we are here, then this line is still part of the description */

      /* Check for empty line
       * (check is done after exit tests, to allow to remove trailing
       * newlines)
       */
      if (buffer[0] == '\n') {
	empty_lines++;
      }
      else {
	empty_lines = 0;
      }

      /* Add line to description */
      strncat(buffer1, buffer, sizeof(buffer1)-strlen(buffer1)-1);

      /* Read next line */
      longtxt_read_line(buffer, sizeof(buffer), in_file);
    } /* while */
    /* Remove up to 2 trailing newlines from description
     */
    /* Less then 2 empty lines? Then end without newline */ 
    if (empty_lines < 2)
      *(buffer1 + strlen(buffer1) - empty_lines -1) = '\0';
    else
      *(buffer1 + strlen(buffer1) - empty_lines) = '\0';

    if (strlen(buffer1)<=0) {
      warn_message("Description was empty when reading from input file line %d\n\n",
		   in_file->line_no);
      break;
    }
    a.description = strdup(buffer1);
    if (a.description == NULL)
      error_message("Out of memory: strdup failed when reading description from input file line %d\n\n",
		    in_file->line_no);


    /* Test for REPEAT */
    b1 = buffer;
    p1 = LONGTXT_ROW_REPEAT1_FORMAT;
    len = sizeof(LONGTXT_ROW_REPEAT1_FORMAT) -1;
    if (feof(in_file->file)
	|| strncmp(b1, p1, len)) {
      /* No REPEAT */
      a.repeatType = (enum repeatTypes) 0;
      a.repeatForever = 1;
      a.repeatFrequency = 0;
      a.repeatDay = (enum DayOfMonthType) 0;
      for(i=0;i<7;i++)
	a.repeatDays[i] = 0;
      a.repeatWeekstart = 0;
    }
    else {
      /* Found REPEAT */
      b1 += len;

      /* Init repeat data */
      a.repeatType = (enum repeatTypes) 0;
      a.repeatForever = 1;
      a.repeatFrequency = 0;
      a.repeatDay = (enum DayOfMonthType) 0;
      for(i=0;i<7;i++)
	a.repeatDays[i] = 0;
      a.repeatWeekstart = 0;

      /* Read repeat start date
       * (assuming here that the first character of the next text string will
       * always contain the separator)
       */
      b2 = data_index(b1, LONGTXT_ROW_REPEAT2_FORMAT[0]);
      len = b2-b1;
      if (len >= sizeof(date_buffer))
	error_message("Date_buffer size: Can not read repeat start date from input file line %d\n\n",
		      in_file->line_no);
      strncpy(date_buffer, b1, len);
      date_buffer[len] = '\0';
      t = read_iso_date_str1 (date_buffer);
      if (t == -1) {
	warn_message("Can not parse repeat start date from input file line %d\n\n",
		     in_file->line_no);
	break;
      }
      b1 = b2;
      tm = *localtime(&t);
      if (tm.tm_year != a.begin.tm_year
	  || tm.tm_mon != a.begin.tm_mon
	  || tm.tm_mday != a.begin.tm_mday) {
	warn_message("Repeat start date deviates from event start date, input file line %d\n\n",
		     in_file->line_no);
	break;
      }

      /* Find repeat end date */
      p1 = LONGTXT_ROW_REPEAT2_FORMAT;
      len = sizeof(LONGTXT_ROW_REPEAT2_FORMAT) -1;
      if(strncmp(b1, p1, len)) {
	warn_message("Can not find repeat end date from input file line %d\n\n",
		     in_file->line_no);
	break;
      }
      b1 += len;

      /* Read repeat end date
       * (assuming here that the first character of the next text string will
       * always contain the separator)
       */
      b2 = data_index(b1, LONGTXT_ROW_REPEAT3_FORMAT[0]);
      len = b2-b1;
      if (len >= sizeof(date_buffer))
	error_message("Date_buffer size: Can not parse repeat end date from input file line %d\n\n",
		      in_file->line_no);
      if (!strncmp(b1, LONGTXT_ROW_REPEAT_FOREVER_CONST, len)) {
	/* Repeat forever */
	a.repeatForever = 1;
	b1 += len;
      }
      else {
	/* Repeat until end date */
	a.repeatForever = 0;
	strncpy(date_buffer, b1, len);
	date_buffer[len] = '\0';
	t = read_iso_date_str1 (date_buffer);
	if (t == -1) {
	  warn_message("Can not parse repeat end date from input file line %d\n\n",
		       in_file->line_no);
	  break;
	}
	b1 = b2;
	a.repeatEnd = *localtime(&t);
      }

      /* Find repeat type */
      p1 = LONGTXT_ROW_REPEAT3_FORMAT;
      len = sizeof(LONGTXT_ROW_REPEAT3_FORMAT) -1;
      if(strncmp(b1, p1, len)) {
	warn_message("Can not find repeat type in input file line %d\n\n",
		     in_file->line_no);
	break;
      }
      b1 += len;

      /* Read repeat type
       * (assuming here that the first character of the next text string will
       * always contain the separator)
       */
      if (!strncmp(b1, LONGTXT_ROW_REPEAT_DAILY_CONST,
		   sizeof(LONGTXT_ROW_REPEAT_DAILY_CONST) -1)) {
	/* Repeat daily */
	a.repeatType = repeatDaily;
	b1 += sizeof(LONGTXT_ROW_REPEAT_DAILY_CONST) -1;
      }
      else if (!strncmp(b1, LONGTXT_ROW_REPEAT_WEEKLY_CONST,
			sizeof(LONGTXT_ROW_REPEAT_WEEKLY_CONST) -1)) {
	/* Repeat weekly */
	a.repeatType = repeatWeekly;
	b1 += sizeof(LONGTXT_ROW_REPEAT_WEEKLY_CONST) -1;
      }
      else if (!strncmp(b1, LONGTXT_ROW_REPEAT_MONTHLY_LAST_WEEK_CONST,
			sizeof(LONGTXT_ROW_REPEAT_MONTHLY_LAST_WEEK_CONST) -1)) {
	/* Repeat monthly on a fixed weekday in the last week of the month */
	a.repeatType = repeatMonthlyByDay;
	repeat_last_week = TRUE;
	b1 += sizeof(LONGTXT_ROW_REPEAT_MONTHLY_LAST_WEEK_CONST) -1;
      }
      else if (!strncmp(b1, LONGTXT_ROW_REPEAT_MONTHLY_WEEKDAY_CONST,
			sizeof(LONGTXT_ROW_REPEAT_MONTHLY_WEEKDAY_CONST) -1)) {
	/* Repeat monthly on a fixed weekday */
	a.repeatType = repeatMonthlyByDay;
	repeat_last_week = FALSE;
	b1 += sizeof(LONGTXT_ROW_REPEAT_MONTHLY_WEEKDAY_CONST) -1;
      }
      else if (!strncmp(b1, LONGTXT_ROW_REPEAT_MONTHLY_CONST,
			sizeof(LONGTXT_ROW_REPEAT_MONTHLY_CONST) -1)) {
	/* Repeat monthly */
	a.repeatType = repeatMonthlyByDate;
	b1 += sizeof(LONGTXT_ROW_REPEAT_MONTHLY_CONST) -1;
      }
      else if (!strncmp(b1, LONGTXT_ROW_REPEAT_YEARLY_CONST,
			sizeof(LONGTXT_ROW_REPEAT_YEARLY_CONST) -1)) {
	/* Repeat yearly */
	a.repeatType = repeatYearly;
	b1 += sizeof(LONGTXT_ROW_REPEAT_YEARLY_CONST) -1;
      }
      else {
	warn_message("Encountered unknown repeat type from input file line %d\n\n",
		     in_file->line_no);
	break;
      }

      /* Find end of repeat type */
      p1 = LONGTXT_ROW_REPEAT4_FORMAT;
      len = sizeof(LONGTXT_ROW_REPEAT4_FORMAT) -1;
      if(strncmp(b1, p1, len)) {
	warn_message("Can not find end of repeat type from input file line %d\n\n",
		     in_file->line_no);
	break;
      }
      b1 += len;


      /* Read repeat type specific parameters */
      /* Repeat daily
       *
       * Means on proceeding days:
       * Today, tomorrow,...
       */
      if(a.repeatType == repeatDaily) {
	/* On the specified day... */
	/* No parameters to read */
      }
      else {
	/* All other repeat types have repeat type specific data */
	/* Find start of repeat type specific data */
	p1 = LONGTXT_ROW_REPEAT_PARAM1_FORMAT;
	len = sizeof(LONGTXT_ROW_REPEAT_PARAM1_FORMAT) -1;
	if(strncmp(b1, p1, len)) {
	  warn_message("Can not find repeat type specific data from input file line %d\n\n",
		       in_file->line_no);
	  break;
	}
	b1 += len;

	b2 = data_index(b1, LONGTXT_ROW_REPEAT_PARAM2_FORMAT[0]);
	len = b2-b1;
	if (len >= sizeof(buffer1)) {
	  warn_message("buffer1 size: Can not parse repeat type specific data from input file line %d\n\n",
		       in_file->line_no);
	  break;
	}
	strncpy(repeatparam, b1, len);
	repeatparam[len] = '\0';
	b1 = b2;

	/* Find end of repeatparams */
	p1 = LONGTXT_ROW_REPEAT_PARAM2_FORMAT;
	len = sizeof(LONGTXT_ROW_REPEAT_PARAM2_FORMAT) -1;
	if(strncmp(b1, p1, len)) {
	  warn_message("Unexpected end of line reading from input file line %d\n\n",
		       in_file->line_no);
	  break;
	}
	b1 += len;
	/* Now repeatparam contains the repeat type specific data */


	/* Repeat monthly by date
	 *
	 * Means always on the same date number (e.g. 17) in every month:
	 * 17th of January, 17th of February, 17th of March,...
	 */
	if(a.repeatType == repeatMonthlyByDate) {
	  /* On the x of every month */

	  /* Parsing repeat type specific data */
	  num_read = sscanf (repeatparam,
			     LONGTXT_ROW_REPEAT_MONTHLY_FORMAT,
			     &mday);
	  if (num_read != 1) {
	    warn_message("Can not parse repeat type specific data from input file line %d\n\n",
			 in_file->line_no);
	    break;
	  }
	  if (mday != a.begin.tm_mday) {
	    warn_message("Monthly repeat day differs from start day reading from input file line %d\n\n",
			 in_file->line_no);
	    break;
	  }
	}


	/* Repeat weekly
	 *
	 * Means always on the same weekday in every week:
	 * Tuesday this week, Tuesday next week,...
	 */
	else if(a.repeatType == repeatWeekly)
	  /* On the chosen days of the week */
	  {
	    int k;

	    /* Parsing repeat type specific data */
	    b2 = repeatparam;
	    len = strlen(repeatparam);
	    /* Init weekdays */
	    for(k=0;k<7;k++) {
	      a.repeatDays[k] = 0;
	    }
	    /* process whole repeat param string */
	    while (len > 0) {
	      if (isspace(*b2)
		  || *b2 == ',') {
		/* skip separator */
		b2++;
		len--;
		continue;
	      }
	      /* Found week day name, now trying to find index */
	      k = weekday2int(b2);
	      if (k>=0) {
		/* Found index of week day */
		a.repeatDays[k] = 1;
		b2 += WEEKDAY_LEN;
		len -= WEEKDAY_LEN;
		continue;
	      }
	      else {
		/* Unknown week day */
		strncpy(buffer1, b2, WEEKDAY_LEN);
		buffer1[WEEKDAY_LEN]= '\0';
		warn_message("Unknown week day <%s> found reading from input file line %d\n\n",
			     buffer1,
			     in_file->line_no);
		break;
	      }
	    } /* while */
	  } /* else repeat weekly */

	  
	/* Repeat monthly by weekday
	 *
	 * Means always on the same weekday in every month.
	 * Comes in two flavors:
	 * - weekday of last week
	 * - weekday of first, second, third, or forth week
	 */
	else if(a.repeatType == repeatMonthlyByDay) {
	  int day;
	  int weekday;

	  /* Repeat monthly by weekday for last week
	   *
	   * Means always on the same weekday in the last week in every month:
	   * Last Tuesday this month, last Tuesday next month,...
	   */
	  if(repeat_last_week == TRUE) {
	    num_read = sscanf (repeatparam,
			       LONGTXT_ROW_REPEAT_MONTHLY_LAST_WEEK_FORMAT,
			       (char *)&buffer1,
			       &day);
	    if (num_read < 1
		|| num_read > 2) {
	      warn_message("Can not parse weekday repeat specific data from input file line %d\n\n",
			   in_file->line_no);
	      break;
	    }
	    weekday = weekday2int(buffer1);
	    if (weekday < 0) {
	      /* Unknown week day */
	      strncpy(buffer1, b2, WEEKDAY_LEN);
	      buffer1[WEEKDAY_LEN]= '\0';
	      warn_message("Unknown week day <%s> found reading from input file line %d\n\n",
			   buffer1,
			   in_file->line_no);
	      break;
	    }
	    /* day should be 1, since only the last week is possible,
	     * but can not be bothered to check for this...
	     */
	    a.repeatDay = domLastSun + weekday;
	  }


	  /* Repeat monthly by weekday not in last week
	   *
	   * Means always on the same weekday in the same week in every month:
	   * Second Tuesday this month, second Tuesday next month,...
	   */
	  else {
	    num_read = sscanf (repeatparam,
			       LONGTXT_ROW_REPEAT_MONTHLY_WEEKDAY_FORMAT,
			       &buffer1,
			       &day);
	    if (num_read != 2) {
	      warn_message("Can not parse weekday repeat specific data from input file line %d\n\n",
			   in_file->line_no);
	      break;
	    }
	    weekday = weekday2int(buffer1);
	    if (weekday < 0) {
	      /* Unknown week day */
	      strncpy(buffer1, b2, WEEKDAY_LEN);
	      buffer1[WEEKDAY_LEN]= '\0';
	      warn_message("Unknown week day <%s> found reading from input file line %d\n\n",
			   buffer1,
			   in_file->line_no);
	      break;
	    }
	    if (day <1 || day > 4) {
	      warn_message("Unknown week number <%d> found reading from input file line %d\n\n",
			   day,
			   in_file->line_no);
	      break;
	    }
	    a.repeatDay = (day -1)*7 + weekday;
	  } /* else repeat monthly by weekday not in last week */
	} /* else repeat monthly by weekday */


	/* Repeat yearly
	 *
	 * Means always on the same date in every year:
	 * 27th October this year, 27th October next year,...
	 */
	else if(a.repeatType == repeatYearly) {
	  /* On one day each year */
	  t = read_iso_date_str1 (repeatparam);
	  if (t == -1) {
	    warn_message("Can not parse yearly repeat date from input file line %d\n\n",
			 in_file->line_no);
	    break;
	  }
	  tm = *localtime(&t);
	  if (tm.tm_mon != a.begin.tm_mon
	      || tm.tm_mday != a.begin.tm_mday) {
	    warn_message("Yearly repeat date deviates from event start date, input file line %d\n\n",
			 in_file->line_no);
	    break;
	  }
	} /* else repeat yearly */

      } /* end else for repeat types with specific data */


      /* Test for repeat frequency */
      p1 = LONGTXT_ROW_REPEAT_FREQUENCY1_FORMAT;
      len = sizeof(LONGTXT_ROW_REPEAT_FREQUENCY1_FORMAT) -1;
      if(strncmp(b1, p1, len)) {
	/* No frequency found */
	/* Set default: frequency = 1 */
	a.repeatFrequency = 1;
      }
      else {
	/* Found frequency */
	b1 += len;

	/* Read frequency */
	a.repeatFrequency = strtol(b1, &b2, 0);
	b1 = b2;

	/* Find end of repeat frequency */
	p1 = LONGTXT_ROW_REPEAT_FREQUENCY2_FORMAT;
	len = sizeof(LONGTXT_ROW_REPEAT_FREQUENCY2_FORMAT) -1;
	if(strncmp(b1, p1, len)) {
	  warn_message("Unexpected end of line reading frequency from input file line %d\n\n",
		       in_file->line_no);
	  break;
	}
	b1 += len;
      }


      /* Test for repeat weekstart */
      p1 = LONGTXT_ROW_REPEAT_WEEKSTART1_FORMAT;
      len = sizeof(LONGTXT_ROW_REPEAT_WEEKSTART1_FORMAT) -1;
      if(strncmp(b1, p1, len)) {
	/* No weekstart found */
	/* Set default: weekstart = 0 */
	a.repeatWeekstart = 0;
      }
      else {
	/* Found weekstart */
	b1 += len;

	/* Read weekstart */
	a.repeatWeekstart = strtol(b1, &b2, 0);
	b1 = b2;


	b2 = data_index(b1, LONGTXT_ROW_REPEAT_PARAM2_FORMAT[0]);
	len = b2-b1;
	if (len >= sizeof(buffer1))
	  error_message("buffer1 size: Can not parse repeat type specific data from input file line %d\n\n",
			in_file->line_no);
	/* Found week day name, now trying to find index */
	a.repeatWeekstart = weekday2int(b1);
	if (a.repeatWeekstart<0) {
	  /* Unknown week day */
	  strncpy(buffer1, b2, WEEKDAY_LEN);
	  buffer1[WEEKDAY_LEN]= '\0';
	  warn_message("Unknown week day <%s> found reading from input file line %d\n\n",
		       buffer1,
		       in_file->line_no);
	  break;
	}
	b1 = b2;


	/* Find end of repeat weekstart */
	p1 = LONGTXT_ROW_REPEAT_WEEKSTART2_FORMAT;
	len = sizeof(LONGTXT_ROW_REPEAT_WEEKSTART2_FORMAT) -1;
	if(strncmp(b1, p1, len)) {
	  warn_message("Unexpected end of line reading weekstart from input file line %d\n\n",
		       in_file->line_no);
	  break;
	}
	b1 += len;
      }



      /* Test for repeat exceptions (omissions) */
      p1 = LONGTXT_ROW_REPEAT_OMIT1_FORMAT;
      len = sizeof(LONGTXT_ROW_REPEAT_OMIT1_FORMAT) -1;
      if(strncmp(b1, p1, len)) {
	/* No exceptions for this event */
	a.exceptions = 0;
	a.exception = NULL;
      }
      else {
	/* Found exceptions */
	b1 += len;

	/* Find out how many exceptions for proper malloc */
	j = 0;
	/* Look for separator */
	b2 = data_index(b1, LONGTXT_ROW_REPEAT_OMIT2_FORMAT[0]);
	while (b2 != NULL
	       && strlen(b2)>0
	       && *b2 != '\n') {
	  /* Found end separator */
	  /* Only count as success when end separator was found */
	  j++;

	  /* More exceptions to be found ? */
	  p1 = LONGTXT_ROW_REPEAT_OMIT2_FORMAT;
	  len = sizeof(LONGTXT_ROW_REPEAT_OMIT2_FORMAT) -1;
	  if(!strncmp(b2, p1, len)) {
	    /* Found another exception */

	    /* Now look for its end separator */
	    b2 = data_index(b2 +1, LONGTXT_ROW_REPEAT_OMIT2_FORMAT[0]);
	    if (b2 == NULL)
	      /* No more separator */
	      break;
	  }
	  else {
	    /* No more exceptions */
	    break;
	  }
	} /* while */

	/* malloc */
	a.exceptions = j;
	a.exception = calloc(a.exceptions, sizeof(struct tm));

	/* Now really read exceptions */
	b2 = data_index(b1, LONGTXT_ROW_REPEAT_OMIT2_FORMAT[0]);
	j = 0;

	/* Read exception dates */
	while (b2 != NULL
	       && strlen(b2)>0
	       && *b2 != '\n') {
	  /* Found end separator */

	  /* Read omission date */
	  len = b2-b1;
	  if (len >= sizeof(date_buffer))
	    error_message("Date_buffer size: Can not read omit date from input file line %d\n\n",
			  in_file->line_no);
	  strncpy(date_buffer, b1, len);
	  date_buffer[len] = '\0';
	  t = read_iso_date_str1 (date_buffer);
	  if (t == -1) {
	    warn_message("Can not parse omit date <%s> from input file line %d\n\n",
			 date_buffer,
			 in_file->line_no);
	    break;
	  }
	  b1 = b2;
	  a.exception[j] = *localtime(&t);

	  /* Only count as success when successfully processed */
	  j++;

	  /* More exceptions to be found ? */
	  p1 = LONGTXT_ROW_REPEAT_OMIT2_FORMAT;
	  len = sizeof(LONGTXT_ROW_REPEAT_OMIT2_FORMAT) -1;
	  if(!strncmp(b2, p1, len)) {
	    /* Found another exception */
	    b1 += len;

	    /* Now look for its end separator */
	    b2 = data_index(b2 +1, LONGTXT_ROW_REPEAT_OMIT2_FORMAT[0]);
	    if (b2 == NULL)
	      /* No more separator */
	      break;
	  }
	  else {
	    /* No more exceptions */
	    break;
	  } /* if found one exception */
	} /* while */

	/* Find end of repeat exceptions */
	p1 = LONGTXT_ROW_REPEAT_OMIT3_FORMAT;
	len = sizeof(LONGTXT_ROW_REPEAT_OMIT3_FORMAT) -1;
	if(strncmp(b1, p1, len)) {
	  warn_message("Unexpected end of line reading omit dates from input file line %d\n\n",
		       in_file->line_no);
	  break;
	}
	b1 += len;
	/* should be at end of line now */
      } /* End of test for repeat exceptions */


      /* Read next line */
      longtxt_read_line(buffer, sizeof(buffer), in_file);
    } /* end else for REPEAT */


    /* Test for ALARM */
    b1 = buffer;
    p1 = LONGTXT_ROW_ALARM1_FORMAT;
    len = sizeof(LONGTXT_ROW_ALARM1_FORMAT) -1;
    if(feof(in_file->file)
       || strncmp(b1, p1, len)) {
      /* No ALARM */
      a.alarm = 0;
      a.advance = 0;
      a.advanceUnits = 0;
    }
    else {
      /* Found ALARM */
      b1 += len;

      a.alarm = 1;

      /* alarm how much before */
      num_read = sscanf (b1,
			 LONGTXT_ROW_ALARM2_FORMAT,
			 &a.advance,
			 &buffer1);
      if (num_read != 2) {
	warn_message("Can not read alarm advance options from input file line %d\n\n",
		     in_file->line_no);
	break;
      }
      b2 = buffer1;
      p1 = LONGTXT_ROW_ALARM_MIN_CONST;
      len = sizeof(LONGTXT_ROW_ALARM_MIN_CONST) -1;
      if(!strncmp(b2, p1, len)) {
	a.advanceUnits = advMinutes;
      }
      else {
	p1 = LONGTXT_ROW_ALARM_HOURS_CONST;
	len = sizeof(LONGTXT_ROW_ALARM_HOURS_CONST) -1;
	if(!strncmp(b2, p1, len)) {
	  a.advanceUnits = advHours;
	}
	else {
	  p1 = LONGTXT_ROW_ALARM_DAYS_CONST;
	  len = sizeof(LONGTXT_ROW_ALARM_DAYS_CONST) -1;
	  if(!strncmp(b2, p1, len)) {
	    a.advanceUnits = advDays;
	  }
	  else {
	    warn_message("Can not understand alarm advance unit <%s> from input file line %d\n\n",
			 b2,
			 in_file->line_no);
	    break;
	  } /* advance days */
	} /* advance hours */
      } /* advance minutes */


      /* Read next line */
      longtxt_read_line(buffer, sizeof(buffer), in_file);
    } /* ALARM */


    /* Find NOTE */
    p1 = LONGTXT_ROW_NOTE_FORMAT;
    len = sizeof(LONGTXT_ROW_NOTE_FORMAT) -1;
    if(feof(in_file->file)
       || strncmp(buffer, p1, len)) {
      /* No NOTE */
      a.note = NULL;
    }
    else {
      /* Found NOTE */
      /* should be at end of line now */

      /* Read next line */
      longtxt_read_line(buffer, sizeof(buffer), in_file);

      /* Read note until encountering two empty lines or
       * next UNTIMED_EVENT, or APPOINTMENT
       */
      buffer1[0] = '\0';
      empty_lines = 0;
      while (!feof(in_file->file)
	     && empty_lines < 2) {
	/* Test for UNTIMED_EVENT */
	b1 = buffer;
	p1 = LONGTXT_ROW_UNTIMED1_FORMAT;
	len = sizeof(LONGTXT_ROW_UNTIMED1_FORMAT) -1;
	if(!strncmp(b1, p1, len)) {
	  break;
	}

	/* Test for APPOINTMENT */
	b1 = buffer;
	p1 = LONGTXT_ROW_APPOINTMENT1_FORMAT;
	len = sizeof(LONGTXT_ROW_APPOINTMENT1_FORMAT) -1;
	if(!strncmp(b1, p1, len)) {
	  break;
	}

	/* If we are here, then this line is still part of the note */

	/* Check for empty line
	 * (check is done after exit tests, to allow to remove trailing
	 * newlines)
	 */
	if (buffer[0] == '\n') {
	  empty_lines++;
	}
	else {
	  empty_lines = 0;
	}

	/* Add line to note */
	strncat(buffer1, buffer, sizeof(buffer1)-strlen(buffer1)-1);

	/* Read next line */
	longtxt_read_line(buffer, sizeof(buffer), in_file);
      }

      /* Always remove at max two trailing newlines, since two newlines
       * separate the note from the following data.
       */
      /* Less then 2 empty lines? Then end without newline */ 
      if (empty_lines < 2)
	*(buffer1 + strlen(buffer1) - empty_lines -1) = '\0';
      else
	*(buffer1 + strlen(buffer1) - empty_lines) = '\0';

      if (strlen(buffer1)<=0) {
	warn_message("Note was empty when reading from input file line %d\n\n",
		     in_file->line_no);
	break;
      }
      a.note = strdup(buffer1);
      if (a.note == NULL)
	error_message("Out of memory: strdup failed when reading note from input file line %d\n\n",
		      in_file->line_no);
    } /* Find NOTE */

    /* Skip empty lines to position for next row */
    while (!feof(in_file->file)
	   && buffer[0] == '\n') {
      longtxt_read_line(buffer, sizeof(buffer), in_file);
    }

    /* Put last line back, to be read on next read attempt */
    if (!feof(in_file->file))
      longtxt_pushback_line(buffer, in_file);



    /* Set datebook row data */
    setRowRecordNum(row, record_num);
    setRowUid(row, uid);
    setRowAttributes(row, attributes);
    setRowCategory(row, category);
    setRowAppointment(row, a);
    setRowIsValid(row, TRUE);

    /* increment counters */
    in_file->num_recs++;
    in_file->next_rec++;

    /* Update statistics */
    in_file->records_read++;
  } while (FALSE);

  /* Debug */
  debug_message("Leaving longtxt_read_row\n");
}



/* For writing */

/* Write an appointment record */
void
longtxt_write_row (struct longtxt_file_data * out_file, struct header_data * header, struct row_data * row)
{
  int j;
  struct AppointmentAppInfo aai;
  struct Appointment a;
  int attributes;
  int category;
  unsigned long uid;
  int record_num;

  char date1_buffer[50];
  char time1_buffer[50];
  char date2_buffer[50];
  char time2_buffer[50];
  char repeatType[30];
  char repeatparam[30];
  char alarmparam[80];
  int repeatparam_len = 0;


  /* Debug */
  debug_message("Entering longtxt_write_row\n");

  if (!getRowIsValid(row))
    error_message("Can not write invalid row.\n");

  /* Get datebook header data */
  aai = header->aai;

  /* Get datebook row data */
  record_num = getRowRecordNum(row);
  uid = getRowUid(row);
  attributes = getRowAttributes(row);
  category = getRowCategory(row);
  a = getRowAppointment(row);


  /* Write datebook row data */
  /* Event type/time */
  if(a.event) {
    write_human_date_str (mktime(&a.begin), date1_buffer, sizeof(date1_buffer));
    longtxt_write(out_file, "%s%lu%s%s%s",
		  LONGTXT_ROW_UNTIMED1_FORMAT,
		  uid,
		  LONGTXT_ROW_UNTIMED2_FORMAT,
		  date1_buffer,
		  LONGTXT_ROW_UNTIMED3_FORMAT);
    longtxt_write_str(out_file, "\n");
  }
  else {
    write_human_full_time_str (mktime(&a.begin), time1_buffer, sizeof(time1_buffer));
    write_human_full_time_str (mktime(&a.end), time2_buffer, sizeof(time2_buffer));
    longtxt_write(out_file, "%s%lu%s%s%s%s%s",
		  LONGTXT_ROW_APPOINTMENT1_FORMAT,
		  uid,
		  LONGTXT_ROW_APPOINTMENT2_FORMAT,
		  time1_buffer,
		  LONGTXT_ROW_APPOINTMENT3_FORMAT,
		  time2_buffer,
		  LONGTXT_ROW_APPOINTMENT4_FORMAT);
    longtxt_write_str(out_file, "\n");
  }

  /* Attributes */
  if (attributes > 0) {
    longtxt_write(out_file, LONGTXT_ROW_ATTRIBUTES_FORMAT,
		  attributes,
		  (attributes & dlpRecAttrDeleted) ? " DELETED" : "",
		  (attributes & dlpRecAttrDirty) ? " DIRTY" : "",
		  (attributes & dlpRecAttrBusy) ? " LOCKED" : "",
		  (attributes & dlpRecAttrSecret) ? " PRIVATE" : "",
		  (attributes & dlpRecAttrArchived) ? " ARCHIVED" : "");
  }

  /* Category */
  if (category > 0) {
    if (header->isValid)
      longtxt_write(out_file, LONGTXT_ROW_CATEGORY_FORMAT,
		    category,
		    aai.category.name[category]);
    else
      longtxt_write(out_file, LONGTXT_ROW_CATEGORY_FORMAT,
		    category,
		    "");
  }

  /* Description */
  /* Print it this way to properly count lines, and description is non-empty */
  if (a.description)
    longtxt_write_str(out_file, a.description);
  else
    longtxt_write_str(out_file, " ");
  longtxt_write_str(out_file, "\n");

  /* Repeat stuff */
  if(a.repeatType == repeatNone) {
    /* One-time event (no repeats) */
  }		                           
      
  else {
    write_human_date_str (mktime(&a.begin), date1_buffer, sizeof(date1_buffer));

    if (a.repeatForever)
      strncpy(date2_buffer, LONGTXT_ROW_REPEAT_FOREVER_CONST, sizeof(date2_buffer));
    else
      write_human_date_str (mktime(&a.repeatEnd), date2_buffer, sizeof(date2_buffer));

    if(a.repeatType == repeatDaily) {
      strncpy(repeatType, LONGTXT_ROW_REPEAT_DAILY_CONST, sizeof(repeatType));
    } else if(a.repeatType == repeatWeekly) {
      strncpy(repeatType, LONGTXT_ROW_REPEAT_WEEKLY_CONST, sizeof(repeatType));
    } else if(a.repeatType == repeatMonthlyByDay) {
      if(a.repeatDay>=domLastSun) {
	strncpy(repeatType, LONGTXT_ROW_REPEAT_MONTHLY_LAST_WEEK_CONST, sizeof(repeatType));
      } else {
	strncpy(repeatType, LONGTXT_ROW_REPEAT_MONTHLY_WEEKDAY_CONST, sizeof(repeatType));
      }
    } else if(a.repeatType == repeatMonthlyByDate) {
      strncpy(repeatType, LONGTXT_ROW_REPEAT_MONTHLY_CONST, sizeof(repeatType));
    } else if(a.repeatType == repeatYearly) {
      strncpy(repeatType, LONGTXT_ROW_REPEAT_YEARLY_CONST, sizeof(repeatType));
    } else {
      strncpy(repeatType, LONGTXT_ROW_REPEAT_UNKNOWN_CONST, sizeof(repeatType));
    }

    longtxt_write(out_file, "%s%s%s%s%s%s%s",
		  LONGTXT_ROW_REPEAT1_FORMAT,
		  date1_buffer,
		  LONGTXT_ROW_REPEAT2_FORMAT,
		  date2_buffer,
		  LONGTXT_ROW_REPEAT3_FORMAT,
		  repeatType,
		  LONGTXT_ROW_REPEAT4_FORMAT);

    /* Repeat daily */
    if(a.repeatType == repeatDaily) {
      /* On the specified day... */
      /* No more parameters */

      /* Repeat monthly by date */
    } else if(a.repeatType == repeatMonthlyByDate) {
      /* On the x of every month */
      snprintf (repeatparam, sizeof(repeatparam),
		LONGTXT_ROW_REPEAT_MONTHLY_FORMAT,
		a.begin.tm_mday);
      longtxt_write(out_file, "%s%s%s",
		    LONGTXT_ROW_REPEAT_PARAM1_FORMAT,
		    repeatparam,
		    LONGTXT_ROW_REPEAT_PARAM2_FORMAT);

      /* Repeat weekly */
    } else if(a.repeatType == repeatWeekly) {
      int k;
      /* On the chosen days of the week */
      {
	repeatparam[0] ='\0';
	repeatparam_len = 0;
	for(k=0;k<7;k++) {
	  if(a.repeatDays[k]) {
	    /* Use comma as separator between weekdays */
	    if (repeatparam_len > 0) {
	      repeatparam[repeatparam_len] = ',';
	      repeatparam_len++;
	      repeatparam[repeatparam_len] = '\0';
	    }
	    /* Add weekday */
	    strncat(repeatparam + repeatparam_len,
		    int2weekday(k),
		    sizeof(repeatparam)-repeatparam_len-1);
	    repeatparam_len = strlen(repeatparam);
	  }
	} /* for */
      }
      longtxt_write(out_file, "%s%s%s",
		    LONGTXT_ROW_REPEAT_PARAM1_FORMAT,
		    repeatparam,
		    LONGTXT_ROW_REPEAT_PARAM2_FORMAT);

      /* Repeat monthly by weekday */
    } else if(a.repeatType == repeatMonthlyByDay) {
      int day;
      int weekday;
			
      if(a.repeatDay>=domLastSun) {
	day = 1;
	weekday = a.repeatDay % 7;
	snprintf (repeatparam, sizeof(repeatparam),
		  LONGTXT_ROW_REPEAT_MONTHLY_LAST_WEEK_FORMAT,
		  int2weekday(weekday),
		  day);
      } else {
	/* day contains the week number */
	day = a.repeatDay / 7 +1;
	weekday = a.repeatDay % 7;
	snprintf (repeatparam, sizeof(repeatparam),
		  LONGTXT_ROW_REPEAT_MONTHLY_WEEKDAY_FORMAT,
		  int2weekday(weekday),
		  day);
      }
      longtxt_write(out_file, "%s%s%s",
		    LONGTXT_ROW_REPEAT_PARAM1_FORMAT,
		    repeatparam,
		    LONGTXT_ROW_REPEAT_PARAM2_FORMAT);

      /* Repeat yearly */
    } else if(a.repeatType == repeatYearly) {
      /* On one day each year */
      snprintf (repeatparam, sizeof(repeatparam),
		LONGTXT_ROW_REPEAT_YEARLY_FORMAT,
		int2month(a.begin.tm_mon),
		a.begin.tm_mday);
      longtxt_write(out_file, "%s%s%s",
		    LONGTXT_ROW_REPEAT_PARAM1_FORMAT,
		    repeatparam,
		    LONGTXT_ROW_REPEAT_PARAM2_FORMAT);
    }

    /* Repeat frequency */
    if(a.repeatFrequency != 1) {
      longtxt_write(out_file, "%s%d%s",
		    LONGTXT_ROW_REPEAT_FREQUENCY1_FORMAT,
		    a.repeatFrequency,
		    LONGTXT_ROW_REPEAT_FREQUENCY2_FORMAT);
    }


    /* Repeat week start
     *
     * The user-decided day which starts the week
     * (0 = Sunday, 1 = Monday).
     * For some reason only being used for weekly events?
     */
    if(a.repeatWeekstart != 0) {
      longtxt_write(out_file, "%s%s%s",
		    LONGTXT_ROW_REPEAT_WEEKSTART1_FORMAT,
		    int2weekday(a.repeatWeekstart),
		    LONGTXT_ROW_REPEAT_WEEKSTART2_FORMAT);
    }


    /* List all exceptions of a repeating event */
    if (a.exceptions) {
      write_human_date_str (mktime(&a.exception[0]),
			    date1_buffer, sizeof(date1_buffer));
      longtxt_write(out_file, "%s%s",
		    LONGTXT_ROW_REPEAT_OMIT1_FORMAT,
		    date1_buffer);
      /* Start from 1, since first exception has already been printed */
      for(j=1;j<a.exceptions;j++) {
	write_human_date_str (mktime(&a.exception[j]),
			      date1_buffer, sizeof(date1_buffer));
	longtxt_write(out_file, "%s%s",
		      LONGTXT_ROW_REPEAT_OMIT2_FORMAT,
		      date1_buffer);
      } /* for */
      longtxt_write_str(out_file, LONGTXT_ROW_REPEAT_OMIT3_FORMAT);
    } /* if exceptions */
    longtxt_write_str(out_file, "\n");
  } /* if REPEAT */
	
  /* Alarm stuff */
  if (a.alarm) {
    /* alarm how much before */
    snprintf(alarmparam, sizeof(alarmparam), LONGTXT_ROW_ALARM2_FORMAT,
	     a.advance,
	     (a.advanceUnits == advMinutes) ? LONGTXT_ROW_ALARM_MIN_CONST :
	     (a.advanceUnits == advHours) ? LONGTXT_ROW_ALARM_HOURS_CONST :
	     (a.advanceUnits == advDays) ? LONGTXT_ROW_ALARM_DAYS_CONST :
	     LONGTXT_ROW_ALARM_UNKNOWN_CONST);
    longtxt_write(out_file, "%s%s",
		  LONGTXT_ROW_ALARM1_FORMAT,
		  alarmparam);
    longtxt_write_str(out_file, "\n");
  }

  /* Note */
  if(a.note) {
    /* Print it this way to properly count lines */
    longtxt_write_str(out_file, LONGTXT_ROW_NOTE_FORMAT);
    longtxt_write_str(out_file, a.note);
    longtxt_write_str(out_file, "\n");
  }

  /* Have at least one empty line between rows */
  longtxt_write_str(out_file, "\n");

  /* Increase counters */
  out_file->num_recs++;
  out_file->next_rec++;

  /* Update statistics */
  out_file->records_written++;

  /* Debug */
  debug_message("Leaving longtxt_write_row\n");
}



/* For statistics */

/* Show input statistics */
void
longtxt_show_read_statistics (struct longtxt_file_data * in_file)
{

  /* Debug */
  debug_message("Entering longtxt_show_read_statistics\n");

  info_message("Input file <%s>, format <%s>:\n",
	       (in_file->filename) ? in_file->filename : "stdin",
	       dataformat2txt(DATA_FORMAT_LONGTXT));
  info_message("Lines read: %d\n",
	       in_file->lines_read);
  info_message("Records read: %d\n",
	       in_file->records_read);

  /* Debug */
  debug_message("Leaving longtxt_show_read_statistics\n");
}


/* Show output statistics */
void
longtxt_show_write_statistics (struct longtxt_file_data * out_file)
{

  /* Debug */
  debug_message("Entering longtxt_show_write_statistics\n");

  info_message("Output file <%s>, format <%s>:\n",
	       (out_file->filename) ? out_file->filename : "stdout",
	       dataformat2txt(DATA_FORMAT_LONGTXT));
  info_message("Lines written: %d\n",
	       out_file->lines_written);
  info_message("Records written: %d\n",
	       out_file->records_written);

  /* Debug */
  debug_message("Leaving longtxt_show_write_statistics\n");
}



/* Private functions */

/* Read datebook file header */
void
longtxt_read_header (struct longtxt_file_data * in_file, struct header_data * header)
{
  struct DBInfo ip;
  struct AppointmentAppInfo aai;
  int app_info_size;
  void * sort_info;
  int sort_info_size;

  int num_read = 0;

  char buffer[0xffff];
  char buffer1[0xffff];
  char buffer2[0xffff];
  char buffer3[0xffff];
  char buffer4[0xffff];
  char buffer5[0xffff];
  char buffer6[0xffff];
  char buffer7[0xffff];

  char long_buffer[50];
  time_t t;

  int j = 0;



  /* Debug */
  debug_message("Entering longtxt_read_header\n");

  /* Init data structures */
  memset(&ip, 0, sizeof(ip));
  memset(&aai, 0, sizeof(aai));

  /* Read general database header data */
  do {
    /* Skip empty lines */
    longtxt_read_line(buffer, sizeof(buffer), in_file);
    while (buffer[0] == '\n') {
      longtxt_read_line(buffer, sizeof(buffer), in_file);
    }
    if (feof(in_file->file))
      error_message("Read unexpected end of input file at line %d\n\n",
		    in_file->line_no);

    /* Check for UNTIMED_EVENT or APPOINTMENT */
    if(!strncmp(buffer,
		LONGTXT_ROW_UNTIMED1_FORMAT,
		sizeof(LONGTXT_ROW_UNTIMED1_FORMAT) -1)) {
      /* Found UNTIMED_EVENT => No header present */
      longtxt_pushback_line(buffer, in_file);
      break;
    }
    else if(!strncmp(buffer,
		     LONGTXT_ROW_APPOINTMENT1_FORMAT,
		     sizeof(LONGTXT_ROW_APPOINTMENT1_FORMAT) -1)) {
      /* Found APPOINTMENT => No header present */
      longtxt_pushback_line(buffer, in_file);
      break;
    }

    /* Read database name */
    num_read = sscanf (buffer, LONGTXT_HEADER_NAME_FORMAT,
		       ip.name);
    if (num_read != 1) {
      warn_message("Can not read header name from input file line %d\n\n",
		   in_file->line_no);
      break;
    }


    /* Read database flags */
    longtxt_read_line(buffer, sizeof(buffer), in_file);
    num_read = sscanf (buffer, LONGTXT_HEADER_FLAGS_FORMAT,
		       &ip.flags,
		       buffer1,
		       buffer2,
		       buffer3,
		       buffer4,
		       buffer5,
		       buffer6,
		       buffer7);
    if (num_read <= 0) {
      warn_message("Can not read flags from input file line %d\n\n",
		   in_file->line_no);
      break;
    }
    if (ip.flags & dlpDBFlagResource) {
      warn_message("Input file is not a Datebook file, resource flag is set!\n\n");
      break;
    }


    /* Read database misc flags */
    longtxt_read_line(buffer, sizeof(buffer), in_file);
    num_read = sscanf (buffer, LONGTXT_HEADER_MISC_FLAGS_FORMAT,
		       &ip.miscFlags);
    if (num_read != 1) {
      warn_message("Can not read misc flags from input file line %d\n\n",
		   in_file->line_no);
      break;
    }


    /* Read database type */
    longtxt_read_line(buffer, sizeof(buffer), in_file);
    num_read = sscanf (buffer, LONGTXT_HEADER_TYPE_FORMAT,
		       long_buffer);
    if (num_read != 1) {
      warn_message("Can not read database type from input file line %d\n\n",
		   in_file->line_no);
      break;
    }
    ip.type = makelong(long_buffer);


    /* Read database creator */
    longtxt_read_line(buffer, sizeof(buffer), in_file);
    num_read = sscanf (buffer, LONGTXT_HEADER_CREATOR_FORMAT,
		       long_buffer);
    if (num_read != 1) {
      warn_message("Can not read creator from input file line %d\n\n",
		   in_file->line_no);
      break;
    }
    ip.creator = makelong(long_buffer);


    /* Read database version */
    longtxt_read_line(buffer, sizeof(buffer), in_file);
    num_read = sscanf (buffer, LONGTXT_HEADER_VERSION_FORMAT,
		       &ip.version);
    if (num_read != 1) {
      warn_message("Can not read version from input file line %d\n\n",
		   in_file->line_no);
      break;
    }


    /* Read database modification number */
    longtxt_read_line(buffer, sizeof(buffer), in_file);
    num_read = sscanf (buffer, LONGTXT_HEADER_MODIFICATION_FORMAT,
		       &ip.modnum);
    if (num_read != 1) {
      warn_message("Can not read modification number from input file line %d\n\n",
		   in_file->line_no);
      break;
    }


    /* Read database creation time */
    longtxt_read_line(buffer, sizeof(buffer), in_file);
    num_read = sscanf (buffer, LONGTXT_HEADER_CREATE_TIME_FORMAT,
		       &buffer1, &buffer2, &buffer3, &buffer4);
    if (num_read < 2 || num_read > 4) {
      warn_message("Can not read creation time from input file line %d\n\n",
		   in_file->line_no);
      break;
    }
    t = read_iso_time_str4n (num_read, buffer1, buffer2, buffer3, buffer4);
    if (t == -1)
      info_message("Can not parse creation time from input file line %d\n\n",
		   in_file->line_no);
    ip.createDate = t;


    /* Read database modification time */
    longtxt_read_line(buffer, sizeof(buffer), in_file);
    num_read = sscanf (buffer, LONGTXT_HEADER_MODIFY_TIME_FORMAT,
		       &buffer1, &buffer2, &buffer3, &buffer4);
    if (num_read < 2 || num_read > 4) {
      warn_message("Can not read modification time from input file line %d\n\n",
		   in_file->line_no);
      break;
    }
    t = read_iso_time_str4n (num_read, buffer1, buffer2, buffer3, buffer4);
    if (t == -1)
      info_message("Can not parse modification time from input file line %d\n\n",
		   in_file->line_no);
    ip.modifyDate = t;


    /* Read database backup time */
    longtxt_read_line(buffer, sizeof(buffer), in_file);
    num_read = sscanf (buffer, LONGTXT_HEADER_BACKUP_TIME_FORMAT,
		       &buffer1, &buffer2, &buffer3, &buffer4);
    if (num_read < 2 || num_read > 4) {
      warn_message("Can not read backup time from input file line %d\n\n",
		   in_file->line_no);
      break;
    }
    t = read_iso_time_str4n (num_read, buffer1, buffer2, buffer3, buffer4);
    if (t == -1)
      info_message("Can not parse backup time from input file line %d\n\n",
		   in_file->line_no);
    ip.backupDate = t;


    /* Read database index */
    longtxt_read_line(buffer, sizeof(buffer), in_file);
    num_read = sscanf (buffer, LONGTXT_HEADER_INDEX_FORMAT,
		       &ip.index);
    if (num_read != 1) {
      warn_message("Can not read index from input file line %d\n\n",
		   in_file->line_no);
      break;
    }



    /* Read datebook application information header data */

    /* Skip empty lines, then check for category header */
    longtxt_read_line(buffer, sizeof(buffer), in_file);
    while (buffer[0] == '\n') {
      longtxt_read_line(buffer, sizeof(buffer), in_file);
    }
    if (!strncmp(buffer, LONGTXT_HEADER_NO_APP_INFO,
		 sizeof(LONGTXT_HEADER_NO_APP_INFO))) {
      /* No application header found */
      app_info_size = 0;
    }
    else {
      /* Found application header */
      if (strncmp(buffer, LONGTXT_HEADER_CATEGORY_HEADER1_FORMAT,
		  sizeof(LONGTXT_HEADER_CATEGORY_HEADER1_FORMAT))) {
	warn_message("Can not read category header 1 from input file line %d\n\n",
		     in_file->line_no);
	break;
      }
      longtxt_read_line(buffer, sizeof(buffer), in_file);
      if (strncmp(buffer, LONGTXT_HEADER_CATEGORY_HEADER2_FORMAT,
		  sizeof(LONGTXT_HEADER_CATEGORY_HEADER2_FORMAT))) {
	warn_message("Can not read category header 2 from input file line %d\n\n",
		     in_file->line_no);
	break;
      }

      /* Read category rows */
      for(j=0;j<DATEBOOK_MAX_CATEGORIES;j++) {
	longtxt_read_line(buffer, sizeof(buffer), in_file);
	num_read = sscanf (buffer, LONGTXT_HEADER_CATEGORY_ROW_FORMAT,
			   (int *)buffer1,
			   &((aai.category).ID[j]),
			   buffer2,
			   (char *)&((aai.category).name[j]),
			   buffer3);
	if (num_read != 4) {
	  /* Allow category 0 to be without name (Unfiled) */
	  if (j != 0 || num_read != 3) {
	    warn_message("Can not read category row %d from input file line %d\n\n",
			 j, in_file->line_no);
	    break;
	  }
	}
	if (strncmp(buffer2, LONGTXT_HEADER_RENAMED_YES, sizeof(buffer2)))
	  (aai.category).renamed[j] = 0;
	else
	  (aai.category).renamed[j] = 1;
	if (!strncmp((aai.category).name[j],
		     LONGTXT_HEADER_RENAMED_UNUSED,
		     sizeof(LONGTXT_HEADER_RENAMED_UNUSED))) {
	  memset((aai.category).name[j], 0, sizeof((aai.category).name[j]));
	}
      } /* for all categories */

      /* Skip empty lines, then read last unique category id */
      longtxt_read_line(buffer, sizeof(buffer), in_file);
      while (buffer[0] == '\n') {
	longtxt_read_line(buffer, sizeof(buffer), in_file);
      }
      num_read = sscanf (buffer, LONGTXT_HEADER_LAST_UNIQUE_ID_FORMAT,
			 &((aai.category).lastUniqueID));
      if (num_read != 1) {
	warn_message("Can not read last unique category id from input file line %d\n\n",
		     in_file->line_no);
	break;
      }


      /* Read start of week */
      longtxt_read_line(buffer, sizeof(buffer), in_file);
      num_read = sscanf (buffer, LONGTXT_HEADER_START_OF_WEEK_FORMAT,
			 &aai.startOfWeek);
      if (num_read != 1) {
	warn_message("Can not read start of Week from input file line %d\n\n",
		     in_file->line_no);
	break;
      }

      /* Set size of application information header data */
      app_info_size = sizeof(header->aai);
    } /* else (no application information) */


    /* Read datebook sort information header data */
    sort_info = NULL;
    sort_info_size = 0;

    /* Skip empty lines, then read 'no sort info'
     * TODO: read in sort info according to hex dump written by write_header
     */
    longtxt_read_line(buffer, sizeof(buffer), in_file);
    while (buffer[0] == '\n') {
      longtxt_read_line(buffer, sizeof(buffer), in_file);
    }
    if (strncmp(buffer, LONGTXT_HEADER_NO_SORT_INFO,
		sizeof(LONGTXT_HEADER_NO_SORT_INFO))) {
      warn_message("Can not read hint on no sort info from input file line %d\n\n",
		   in_file->line_no);
      break;
    }


    /* Skip empty lines to find begin of row data */
    longtxt_read_line(buffer, sizeof(buffer), in_file);
    while (buffer[0] == '\n') {
      longtxt_read_line(buffer, sizeof(buffer), in_file);
    }

    /* Put last line back, to be read on next read attempt */
    longtxt_pushback_line(buffer, in_file);


    /* Set datebook header data if header has not already been read.
     * If header has already been read earlier, then the longtxt read
     * header information is ignored - but we are properly positioned
     * for reading row data now.
     */
    if (header->isValid) {
      info_message("Skip reading header data, since header has already been read.\n");
    }
    else {
      header->info = ip;
      header->aai = aai;
      header->app_info_size = app_info_size;
      header->sort_info = sort_info;
      header->sort_info_size = sort_info_size;
      header->isValid = TRUE;
    }
  } while (FALSE);

  /* Debug */
  debug_message("Leaving longtxt_read_header\n");
}


/* Write datebook file header */
void
longtxt_write_header (struct longtxt_file_data * out_file, struct header_data * header)
{
  struct DBInfo * ip;
  struct AppointmentAppInfo * aai;
  void * sort_info;
  int sort_info_size;

  char time_buffer[50];

  int j;


  /* Debug */
  debug_message("Entering longtxt_write_header\n");

  if (header->isValid) {
    /* Get datebook header data */
    ip = &(header->info);
    aai = &(header->aai);
    sort_info = header->sort_info;
    sort_info_size = header->sort_info_size;


    /* Print database header flags */
    longtxt_write(out_file, LONGTXT_HEADER_NAME_FORMAT,
		  ip->name);
    longtxt_write(out_file, LONGTXT_HEADER_FLAGS_FORMAT, ip->flags,
		  (ip->flags & dlpDBFlagResource) ? " RESOURCE" : "",
		  (ip->flags & dlpDBFlagReadOnly) ? " READ_ONLY" : "",
		  (ip->flags & dlpDBFlagAppInfoDirty) ? " APP-INFO-DIRTY" : "",
		  (ip->flags & dlpDBFlagBackup) ? " BACKUP" : "",
		  (ip->flags & dlpDBFlagOpen) ? " OPEN" : "",
		  (ip->flags & dlpDBFlagNewer) ? " NEWER" : "",
		  (ip->flags & dlpDBFlagReset) ? " RESET" : "");
    longtxt_write(out_file, LONGTXT_HEADER_MISC_FLAGS_FORMAT,
		  ip->miscFlags);
    longtxt_write(out_file, LONGTXT_HEADER_TYPE_FORMAT,
		  printlong(ip->type));
    longtxt_write(out_file, LONGTXT_HEADER_CREATOR_FORMAT,
		  printlong(ip->creator));
    longtxt_write(out_file, LONGTXT_HEADER_VERSION_FORMAT,
		  ip->version);
    longtxt_write(out_file, LONGTXT_HEADER_MODIFICATION_FORMAT,
		  ip->modnum);

    write_human_full_time_str (ip->createDate, time_buffer, sizeof(time_buffer));
    longtxt_write(out_file, LONGTXT_HEADER_CREATE_TIME_FORMAT,
		  time_buffer, "", "", "");

    write_human_full_time_str (ip->modifyDate, time_buffer, sizeof(time_buffer));
    longtxt_write(out_file, LONGTXT_HEADER_MODIFY_TIME_FORMAT,
		  time_buffer, "", "", "");

    write_human_full_time_str (ip->backupDate, time_buffer, sizeof(time_buffer));
    longtxt_write(out_file, LONGTXT_HEADER_BACKUP_TIME_FORMAT,
		  time_buffer, "", "", "");

    longtxt_write(out_file, LONGTXT_HEADER_INDEX_FORMAT,
		  ip->index);
    longtxt_write_str(out_file, "\n");


    /* Print datebook application header */
    if (header->app_info_size <= 0) {
      longtxt_write_str(out_file, "\n");
      longtxt_write_str(out_file, LONGTXT_HEADER_NO_APP_INFO);
    }
    else {
      longtxt_write_str(out_file, "\n");
      longtxt_write_str(out_file, LONGTXT_HEADER_CATEGORY_HEADER1_FORMAT);
      longtxt_write_str(out_file, LONGTXT_HEADER_CATEGORY_HEADER2_FORMAT);
      for(j=0;j<DATEBOOK_MAX_CATEGORIES;j++) {
	longtxt_write(out_file, LONGTXT_HEADER_CATEGORY_ROW_FORMAT,
		      j,
		      (aai->category).ID[j],
		      (aai->category).renamed[j]
		      ? LONGTXT_HEADER_RENAMED_YES : LONGTXT_HEADER_RENAMED_NO,
		      (aai->category).name[j],
		      (j>0 && (aai->category).ID[j] == 0)
		      ? LONGTXT_HEADER_RENAMED_UNUSED : "");
      }
      longtxt_write(out_file, LONGTXT_HEADER_LAST_UNIQUE_ID_FORMAT,
		    (aai->category).lastUniqueID);
      longtxt_write(out_file, LONGTXT_HEADER_START_OF_WEEK_FORMAT,
		    aai->startOfWeek);
      longtxt_write_str(out_file, "\n");
    }


    /* Print datebook sort header */
    if (sort_info_size > 0)
      {
	/* Does this ever happen for a datebook database?
	 * At least currently we will not be able to read the hexdump
	 * back in.
	 */
	longtxt_write(out_file, "sort_info_size %d\n", sort_info_size);
	write_dump (out_file->file, sort_info, sort_info_size);
	longtxt_write_str(out_file, "\n");
      }
    else
      {
	longtxt_write_str(out_file, LONGTXT_HEADER_NO_SORT_INFO);
	longtxt_write_str(out_file, "\n");
      }
    /* Visually separate header from body */
    longtxt_write_str(out_file, "\n");
  }
  else {
    debug_message("No valid header present => skip writing header.\n");
  }

  /* Debug */
  debug_message("Leaving longtxt_write_header\n");
}



/* For IO */

/* Read one line from input file */
void
longtxt_read_line (char * buffer, int buffer_size, struct longtxt_file_data * in_file)
{

  /* Check for unexpected end of file */
  if (feof(in_file->file))
    error_message("Unexpected end of input file, line %d\n\n",
		  in_file->line_no);

  /* Read one line */
  if (in_file->line_buffer != NULL) {
    /* Use pushback buffer, if filled */
    strncpy(buffer, in_file->line_buffer, buffer_size);
    free (in_file->line_buffer);
    in_file->line_buffer = NULL;
    in_file->line_no++;
  }
  else {
    /* Return empty string if fgets fails */
    buffer[0] = '\0';

    /* Read from file */
    fgets(buffer, buffer_size, in_file->file);
    if (!feof(in_file->file)) {
      in_file->line_no++;

      /* Check whether entire line could be read */
      if (buffer[strlen(buffer) -1] != '\n')
	error_message("Line <%d> in input file <%s> exceeds buffer size\n",
		      in_file->line_no,
		      (in_file->filename==NULL) ? "stdin" : in_file->filename);
    }
  }
}


/* Push pack one line into input file */
void
longtxt_pushback_line (char * buffer, struct longtxt_file_data * in_file)
{
  /* Pushback line buffer to allow for later re-reading */
  in_file->line_buffer = strdup(buffer);
  in_file->line_no--;
}


/* Write one single string to output file
 *
 * Especially useful for printing multi-line text, to properly count
 * line numbers
 */
void
longtxt_write_str (struct longtxt_file_data * out_file, const char * out_string)
{
  char * newline_pos;


  /* Write text */
  fprintf (out_file->file, "%s", out_string);

  /* Count printed lines */
  newline_pos = data_index(out_string, '\n');
  while (newline_pos != NULL) {
    out_file->line_no++;
    newline_pos = data_index(newline_pos+1, '\n');
  }
}


/* Write to output file */
void
longtxt_write (struct longtxt_file_data * out_file, const char * format, ...)
{
  va_list printf_args;
  char * newline_pos;


  /* Get variable arguments */
  va_start(printf_args, format);

  /* Write text */
  vfprintf (out_file->file, format, printf_args);

  /* No more processing of variable arguments */
  va_end(printf_args);

  /* Count printed lines
   * (only accurate if newlines can be found in format parameter) */
  newline_pos = data_index(format, '\n');
  while (newline_pos != NULL) {
    out_file->line_no++;
    newline_pos = data_index(newline_pos+1, '\n');
  }
}
