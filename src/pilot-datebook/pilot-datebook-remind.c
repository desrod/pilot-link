/*
 * Pilot Datebook processing utility
 * Matthias Hessler <pilot-datebook@mhessler.de> December 2000
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <string.h>
#include "pilot-datebook-remind.h"




/* Read/write header format */
const char REMIND_PUSH_CONTEXT[] = "PUSH-OMIT-CONTEXT\n";
const char REMIND_CLEAR_CONTEXT[] = "CLEAR-OMIT-CONTEXT\n";
const char REMIND_POP_CONTEXT[] = "POP-OMIT-CONTEXT\n";

/* Read/write row format */
const char REMIND_ROW_OMIT_FORMAT[] = "OMIT %d %s %d\n";
const char REMIND_ROW_ADVANCE_FORMAT[] = "AT %2.2d:%2.2d +%d ";
const char REMIND_ROW_REPEAT_END_FORMAT[] = "UNTIL %d %s %d ";
const char REMIND_ROW_DAILY_FORMAT[] = "REM %d %s %d ";
const char REMIND_ROW_DAILY_FREQUENCY[] = "*%d ";
const char REMIND_ROW_MONTLY_FORMAT[] = "REM %d ";
const char REMIND_ROW_MONTLY_FREQUENCY[] = "SATISFY \
[(trigdate()>=date(%d,%d,%d)) && \
(!isomitted(trigdate())) && \
(((monnum(trigdate())-1+year(trigdate())*12)%%%d) == ((%d+%d*12)%%%d))] ";
const char REMIND_ROW_MONTLY_NO_FREQUENCY[] = "SATISFY [(trigdate()>=date(%d,%d,%d)) && (!isomitted(trigdate()))] ";
const char REMIND_ROW_WEEKLY_FORMAT1[] = "REM ";
const char REMIND_ROW_WEEKLY_FORMAT2[] = "%s ";
const char REMIND_ROW_WEEKLY_FREQUENCY[] = "SATISFY \
[(trigdate()>=date(%d,%d,%d)) &&\
(!isomitted(trigdate())) &&\
(((coerce(\"int\",trigdate())/7)%%%d) == ((coerce(\"int\",date(%d,%d,%d))/7)%%%d))] ";
const char REMIND_ROW_WEEKLY_NO_FREQUENCY[] = "SATISFY [(trigdate()>=date(%d,%d,%d))  && (!isomitted(trigdate()))] ";
const char REMIND_ROW_MONTHLY_LAST_WEEK_FORMAT[] = "REM %s %d -7 ";
const char REMIND_ROW_MONTHLY_WEEKDAY_FORMAT[] = "REM %s %d ";
const char REMIND_ROW_MONTHLY_WEEK_FREQUENCY[] = "SATISFY \
[(trigdate()>=date(%d,%d,%d)) && \
(!isomitted(trigdate())) && \
(((monnum(trigdate())-1+year(trigdate())*12)%%%d) == ((%d+%d*12)%%%d))] ";
const char REMIND_ROW_MONTHLY_WEEK_NO_FREQUENCY[] = "SATISFY [(trigdate()>=date(%d,%d,%d))  && (!isomitted(trigdate()))] ";
const char REMIND_ROW_YEARLY_FORMAT[] = "REM %d %s ";
const char REMIND_ROW_YEARLY_FREQUENCY[] = "SATISFY \
[(trigdate()>=date(%d,%d,%d)) &&\
(!isomitted(trigdate())) &&\
((year(trigdate())%%%d) == (%d%%%d))] ";
const char REMIND_ROW_YEARLY_NO_FREQUENCY[] = "SATISFY [(trigdate()>=date(%d,%d,%d)) && (!isomitted(trigdate()))]";
const char REMIND_ROW_NO_REPEAT_FORMAT[] = "REM %d %s %d ";
const char REMIND_ROW_NOTE_FORMAT1[] = " (";
const char REMIND_ROW_NOTE_FORMAT2[] = ")";
const char REMIND_ROW_DESCRIPTION_FORMAT1[] = "MSG ";
const char REMIND_ROW_DESCRIPTION_FORMAT2[] = " %a";
const char REMIND_ROW_FROM_TO_FORMAT[] = " from %2.2d:%2.2d to %2.2d:%2.2d";




/* Public functions */

/* For init */

/* Initialize write data structure */
int
remind_init_write (struct remind_file_data * out_file)
{

  /* Debug */
  debug_message("Entering remind_init_write\n");

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
  debug_message("Leaving remind_init_write\n");

  return TRUE;
}


/* Destroy write data structure */
void
remind_exit_write (struct remind_file_data * out_file)
{

  /* Debug */
  debug_message("Entering remind_exit_write\n");

  /* Free memory */
  if (out_file->filename)
    free (out_file->filename);

  /* Debug */
  debug_message("Leaving remind_exit_write\n");
}


/* Set write command line option */
int
remind_set_write_option (struct remind_file_data * out_file, char opt, char * opt_arg)
{
  int rc = FALSE;


  /* Debug */
  debug_message("Entering remind_set_write_option\n");

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
      fprintf(stderr, "Can not process write option <%c> for output file\n",
	      opt);
    }

  /* Debug */
  debug_message("Leaving remind_set_write_option\n");

  return rc;
}



/* For opening & closing */

/* Open output data file for writing */
void
remind_open_write (struct remind_file_data * out_file, struct header_data * header)
{

  /* Debug */
  debug_message("Entering remind_open_write\n");

  /* Open file */
  /* Write to stdout if file name not given or "-" */
  if (!out_file->filename) {
    /* No open is needed when writing to stdout */
    out_file->file = stdout;
  }
  else {
    out_file->file = fopen(out_file->filename, "w");
    if (!out_file->file)
      error_message("Can not open %s for writing\n",
		    out_file->filename);
    out_file->file_is_open = TRUE;
  }

  /* Init */
  out_file->line_buffer = NULL;
  out_file->line_no = 0;
  out_file->num_recs = 0;
  out_file->next_rec = 0;

  /* Write header */
  remind_write_header (out_file, header);

  /* Debug */
  debug_message("Leaving remind_open_write\n");
}


/* Close output file at end of processing */
void
remind_close_write (struct remind_file_data * out_file, struct header_data * header)
{

  /* Debug */
  debug_message("Entering remind_close_write\n");

  /* End omit context */
  remind_write_str(out_file, REMIND_POP_CONTEXT);

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
  debug_message("Leaving remind_close_write\n");
}


/* Close output file in case of an error */
void
remind_abort_write (struct remind_file_data * out_file)
{

  /* Debug */
  debug_message("Entering remind_abort_write\n");

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
  debug_message("Leaving remind_abort_write\n");
}



/* For writing */

/* Write an appointment record */
void
remind_write_row (struct remind_file_data * out_file, struct header_data * header, struct row_data * row)
{
  int j;
  struct AppointmentAppInfo aai;
  struct Appointment a;
  int attributes;
  int category;
  unsigned long uid;
  int record_num;

  char delta[80];
  char satisfy[256];


  /* Debug */
  debug_message("Entering remind_write_row\n");

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

  /* Skip deleted records */
  if((attributes & dlpRecAttrDeleted) || (attributes & dlpRecAttrArchived)) {
    /* Found deleted or archived record => skip */
  }
  else {
    /* Found record for reminders output */
    strcpy(delta, "+7 ");
    satisfy[0] = 0;

    if (a.exceptions) {
      remind_write(out_file, REMIND_PUSH_CONTEXT);
      for(j=0;j<a.exceptions;j++) {
	remind_write(out_file, REMIND_ROW_OMIT_FORMAT,
		     a.exception[j].tm_mday,
		     int2month(a.exception[j].tm_mon),
		     a.exception[j].tm_year+1900);
      }
    }

    if (a.advance) {
      snprintf(delta + strlen(delta), sizeof(delta) -strlen(delta) -1,
	       REMIND_ROW_ADVANCE_FORMAT,
	       a.begin.tm_hour,
	       a.begin.tm_min,
	       a.advance * (
			    (a.advanceUnits == advMinutes) ? 1 :
			    (a.advanceUnits == advHours) ? 60 :
			    (a.advanceUnits == advDays) ? 60*24 :
			    0));
    }

    if (!a.repeatForever) {
      snprintf(delta + strlen(delta), sizeof(delta) -strlen(delta) -1,
	       REMIND_ROW_REPEAT_END_FORMAT,
	       a.repeatEnd.tm_mday,
	       int2month(a.repeatEnd.tm_mon),
	       a.repeatEnd.tm_year+1900);
    }

    if(a.repeatFrequency) {
      if(a.repeatType == repeatDaily) {
	/* On the specified day... */
	remind_write(out_file, REMIND_ROW_DAILY_FORMAT,
		     a.begin.tm_mday,
		     int2month(a.begin.tm_mon),
		     a.begin.tm_year+1900);
	if(a.repeatFrequency > 1) {
	  /* And every x days afterwords */
	  remind_write(out_file, REMIND_ROW_DAILY_FREQUENCY,
		       a.repeatFrequency);
	}
      } else if(a.repeatType == repeatMonthlyByDate) {
	/* On the x of every month */
	remind_write(out_file, REMIND_ROW_MONTLY_FORMAT,
		     a.begin.tm_mday);

	if(a.repeatFrequency>1) {
	  /* if the month is equal to the starting month mod x */
	  snprintf(satisfy, sizeof(satisfy) -1,
		   REMIND_ROW_MONTLY_FREQUENCY,
		   a.begin.tm_year+1900,
		   a.begin.tm_mon+1,
		   a.begin.tm_mday,
		   a.repeatFrequency,
		   a.begin.tm_year+1900,
		   a.begin.tm_mon,
		   a.repeatFrequency);
	} else {
	  snprintf(satisfy, sizeof(satisfy) -1,
		   REMIND_ROW_MONTLY_NO_FREQUENCY,
		   a.begin.tm_year+1900,
		   a.begin.tm_mon+1,
		   a.begin.tm_mday);
	}
      } else if(a.repeatType == repeatWeekly) {
	int k;
	/* On the chosen days of the week */
	remind_write(out_file, REMIND_ROW_WEEKLY_FORMAT1);
	for(k=0;k<7;k++)
	  if(a.repeatDays[k])
	    remind_write(out_file, REMIND_ROW_WEEKLY_FORMAT2,
			 int2weekday(k));

	if(a.repeatFrequency>1) {
	  /* if the week is equal to the starting week mod x */
	  snprintf(satisfy, sizeof(satisfy) -1,
		   REMIND_ROW_WEEKLY_FREQUENCY,
		   a.begin.tm_year+1900,
		   a.begin.tm_mon+1,
		   a.begin.tm_mday,
		   a.repeatFrequency,
		   a.begin.tm_year+1900,
		   a.begin.tm_mon+1,
		   a.begin.tm_mday,
		   a.repeatFrequency);
	} else {
	  snprintf(satisfy, sizeof(satisfy) -1,
		   REMIND_ROW_WEEKLY_NO_FREQUENCY,
		   a.begin.tm_year+1900,
		   a.begin.tm_mon+1,
		   a.begin.tm_mday);
	}
      } else if(a.repeatType == repeatMonthlyByDay) {
	int day;
	int weekday;

	if(a.repeatDay>=domLastSun) {
	  day = 1;
	  weekday = a.repeatDay % 7;
	  remind_write(out_file, REMIND_ROW_MONTHLY_LAST_WEEK_FORMAT,
		       int2weekday(weekday),
		       day);
	} else {
	  day = a.repeatDay / 7 * 7 + 1;
	  weekday = a.repeatDay % 7;
	  remind_write(out_file, REMIND_ROW_MONTHLY_WEEKDAY_FORMAT,
		       int2weekday(weekday),
		       day);
	}

	if( a.repeatFrequency > 1) {

	  snprintf(satisfy, sizeof(satisfy) -1,
		   REMIND_ROW_MONTHLY_WEEK_FREQUENCY,
		   a.begin.tm_year+1900,
		   a.begin.tm_mon+1,
		   a.begin.tm_mday,
		   a.repeatFrequency,
		   a.begin.tm_year+1900,
		   a.begin.tm_mon,
		   a.repeatFrequency);
	} else {
	  snprintf(satisfy, sizeof(satisfy) -1,
		   REMIND_ROW_MONTHLY_WEEK_NO_FREQUENCY,
		   a.begin.tm_year+1900,
		   a.begin.tm_mon+1,
		   a.begin.tm_mday);
	}
      } else if(a.repeatType == repeatYearly) {
	/* On one day each year */
	remind_write(out_file, REMIND_ROW_YEARLY_FORMAT,
		     a.begin.tm_mday,
		     int2month(a.begin.tm_mon));
	if(a.repeatFrequency>1) {
	  /* if the year is equal to the starting year, mod x */
	  snprintf(satisfy, sizeof(satisfy) -1,
		   REMIND_ROW_YEARLY_FREQUENCY,
		   a.begin.tm_year+1900,
		   a.begin.tm_mon+1,
		   a.begin.tm_mday,
		   a.repeatFrequency,
		   a.begin.tm_year+1900,
		   a.repeatFrequency);
	} else {
	  snprintf(satisfy, sizeof(satisfy) -1,
		   REMIND_ROW_YEARLY_NO_FREQUENCY,
		   a.begin.tm_year+1900,
		   a.begin.tm_mon+1,
		   a.begin.tm_mday);
	}
      }

    } else {
      /* On that one day */
      remind_write(out_file, REMIND_ROW_NO_REPEAT_FORMAT,
		   a.begin.tm_mday,
		   int2month(a.begin.tm_mon),
		   a.begin.tm_year+1900);
    }

    remind_write(out_file, "%s%s",
		 delta,
		 satisfy);

    /* Description */
    /* Print it this way to properly count lines */
    remind_write_str(out_file, REMIND_ROW_DESCRIPTION_FORMAT1);
    remind_write_str(out_file, a.description);
    remind_write_str(out_file, REMIND_ROW_DESCRIPTION_FORMAT2);

    /* Note */
    if(a.note) {
      /* Print it this way to properly count lines */
      remind_write_str(out_file, REMIND_ROW_NOTE_FORMAT1);
      remind_write_str(out_file, a.note);
      remind_write_str(out_file, REMIND_ROW_NOTE_FORMAT2);
    }

    if(!a.event) {
      remind_write(out_file, REMIND_ROW_FROM_TO_FORMAT,
		   a.begin.tm_hour,
		   a.begin.tm_min,
		   a.end.tm_hour,
		   a.end.tm_min);
    }
    remind_write(out_file, "\n");

    if (a.exceptions)
      remind_write(out_file, REMIND_POP_CONTEXT);



    /* Increase counters */
    out_file->num_recs++;
    out_file->next_rec++;

    /* Update statistics */
    out_file->records_written++;
  } /* else deleted record found */

  /* Debug */
  debug_message("Leaving remind_write_row\n");
}



/* For statistics */

/* Show output statistics */
void
remind_show_write_statistics (struct remind_file_data * out_file)
{

  /* Debug */
  debug_message("Entering remind_show_write_statistics\n");

  info_message("Output file <%s>, format <%s>:\n",
	       (out_file->filename) ? out_file->filename : "stdout",
	       dataformat2txt(DATA_FORMAT_REMIND));
  info_message("Lines written: %d\n",
	       out_file->lines_written);
  info_message("Records written: %d\n",
	       out_file->records_written);

  /* Debug */
  debug_message("Leaving remind_show_write_statistics\n");
}



/* Private functions */

/* Write datebook file header */
void
remind_write_header (struct remind_file_data * out_file, struct header_data * header)
{

  /* Debug */
  debug_message("Entering remind_write_header\n");


  /* No header data information will be written */


  /* Print leading reminders information */
  remind_write(out_file, REMIND_PUSH_CONTEXT);
  remind_write(out_file, REMIND_CLEAR_CONTEXT);

  /* Debug */
  debug_message("Leaving remind_write_header\n");
}



/* For IO */

/* Write one single string to output file
 *
 * Especially useful for printing multi-line text, to properly count
 * line numbers
 */
void
remind_write_str (struct remind_file_data * out_file, const char * out_string)
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
remind_write (struct remind_file_data * out_file, const char * format, ...)
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
