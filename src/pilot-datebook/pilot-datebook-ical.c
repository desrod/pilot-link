/*
 * Pilot Datebook processing utility
 * Matthias Hessler <pilot-datebook@mhessler.de> December 2000
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <string.h>
#include "pilot-datebook-ical.h"




/* Constants */

/* Read/write header format */
const char ICAL_HEADER[] = "calendar cal $ical(calendar)\n";
const char ICAL_FOOTER1[] = "cal save [cal main]\n";
const char ICAL_FOOTER2[] = "exit\n";

/* Read/write row format */
/* Row format constants  have not been defined, since neither will
 * reading-in for the ical file format be implemented, nor would the
 * source code be easier to read */



/* Public functions */

/* For init */

/* Initialize write data structure */
int
ical_init_write (struct ical_file_data * out_file)
{

  /* Debug */
  debug_message("Entering ical_init_write\n");

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
  debug_message("Leaving ical_init_write\n");

  return TRUE;
}


/* Destroy write data structure */
void
ical_exit_write (struct ical_file_data * out_file)
{

  /* Debug */
  debug_message("Entering ical_exit_write\n");

  /* Free memory */
  if (out_file->filename)
    free (out_file->filename);

  /* Debug */
  debug_message("Leaving ical_exit_write\n");
}


/* Set write command line option */
int
ical_set_write_option (struct ical_file_data * out_file, char opt, char * opt_arg)
{
  int rc = FALSE;


  /* Debug */
  debug_message("Entering ical_set_write_option\n");

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
  debug_message("Leaving ical_set_write_option\n");

  return rc;
}



/* For opening & closing */

/* Open output data file for writing */
void
ical_open_write (struct ical_file_data * out_file, struct header_data * header)
{

  /* Debug */
  debug_message("Entering ical_open_write\n");

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
  ical_write_header (out_file, header);

  /* Debug */
  debug_message("Leaving ical_open_write\n");
}


/* Close output file at end of processing */
void
ical_close_write (struct ical_file_data * out_file, struct header_data * header)
{

  /* Debug */
  debug_message("Entering ical_close_write\n");

  /* Write footer */
  ical_write_str(out_file, ICAL_FOOTER1);
  ical_write_str(out_file, ICAL_FOOTER2);

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
  debug_message("Leaving ical_close_write\n");
}


/* Close output file in case of an error */
void
ical_abort_write (struct ical_file_data * out_file)
{

  /* Debug */
  debug_message("Entering ical_abort_write\n");

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
  debug_message("Leaving ical_abort_write\n");
}



/* For writing */

/* Write an appointment record */
void
ical_write_row (struct ical_file_data * out_file, struct header_data * header, struct row_data * row)
{
  int j;
  struct AppointmentAppInfo aai;
  struct Appointment a;
  int attributes;
  int category;
  unsigned long uid;
  int record_num;

  /* TODO: would need to read this as parameter from command line arguments
   * to be fully compatible to read-ical. However, since the option -p was
   * not even mentioned in the manpage for read-ical (Options: None), this
   * is considered an undocumented and less important feature, and for now
   * disabled.
   * Will be covered by a more general update logic, once it is implemented.
   */
  char *pubtext = NULL;

  char id_buf[255];


  /* Debug */
  debug_message("Entering ical_write_row\n");

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

  if (!getRowIsValid(row))
    error_message("Can not write empty ical output record.\n");

  /* Write datebook row data */

  /* Skip deleted records */
  if((attributes & dlpRecAttrDeleted) || (attributes & dlpRecAttrArchived)) {
    /* Found deleted or archived record => skip */
  }
  else {
    /* Found record for ical output */

    /* UNTIMED_EVENT */
    if (a.event) {
      ical_write_str(out_file, "set i [notice]\n");
    }

    /* APPOINTMENT */
    else {
      int start,end;

      start = a.begin.tm_hour*60 + a.begin.tm_min;
      end   = a.end.tm_hour*60 + a.end.tm_min;

      ical_write_str(out_file, "set i [appointment]\n");
      ical_write(out_file, "$i starttime %d\n",
		 start);
      ical_write(out_file, "$i length %d\n",
		 end-start);
    }
 
    /* Don't hilight private (secret) records */
    if (attributes & dlpRecAttrSecret) {
      ical_write_str(out_file, "$i hilite never\n");
    }
 
    /* Handle alarms */
    if (a.alarm) {
      if (a.event) { 
	if (a.advanceUnits == advDays ) {
	  ical_write(out_file, "$i earlywarning %d\n",
		     a.advance);
	} else {
	  info_message("Minute or hour alarm on untimed event ignored: %s\n",
		       a.description);
	}
      } else {
	switch (a.advanceUnits) {
	case advMinutes:
	  ical_write(out_file, "$i alarms { %d }\n",
		     a.advance);
	  break;
	case advHours:
	  ical_write(out_file, "$i alarms { %d }\n",
		     a.advance*60);
	  break;
	case advDays:
	  ical_write(out_file, "$i earlywarning %d\n",
		     a.advance);
	  break;
	}
      }
    }

    /* '\x95' is the "bullet" character */
    ical_write_str(out_file, "$i text ");
    ical_write_str(out_file, tclquote((pubtext && a.description[0] != '\x95')
				      ? pubtext : a.description));
    ical_write_str(out_file, "\n");

    ical_write(out_file, "set begin [date make %d %d %d]\n",
	       a.begin.tm_mday,
	       a.begin.tm_mon+1,
	       a.begin.tm_year+1900);

    if (a.repeatFrequency) {
      if (a.repeatType == repeatDaily) {
	ical_write(out_file, "$i dayrepeat %d $begin\n",
		   a.repeatFrequency);
      } else if(a.repeatType == repeatMonthlyByDate) {
	ical_write(out_file, "$i month_day %d $begin %d\n",
		   a.begin.tm_mon+1,
		   a.repeatFrequency);
      } else if(a.repeatType == repeatMonthlyByDay) {
	if (a.repeatDay>=domLastSun) {
	  ical_write(out_file, "$i month_last_week_day %d 1 $begin %d\n",
		     a.repeatDay % 7 + 1,
		     a.repeatFrequency);
	} else {
	  ical_write(out_file, "$i month_week_day %d %d $begin %d\n",
		     a.repeatDay % 7 + 1,
		     a.repeatDay / 7 + 1,
		     a.repeatFrequency);
	}
      } else if(a.repeatType == repeatWeekly) {
	/*
	 * Handle the case where the user said weekly repeat, but
	 * really meant daily repeat every n*7 days.  Note: We can't
	 * do days of the week and a repeat-frequency > 1, so do the
	 * best we can and go on.
	 */
	if (a.repeatFrequency > 1) {
	  int ii, found;
	  for (ii = 0, found = 0; ii < 7; ii++) {
	    if (a.repeatDays[ii])
	      found++;
	  }
	  if (found > 1)
	    info_message("Incomplete translation of %s\n",
			 a.description);
	  ical_write(out_file, "$i dayrepeat %d $begin\n",
		     a.repeatFrequency * 7);
	} else {
	  int ii;
	  ical_write(out_file, "$i weekdays ");
	  for (ii=0;ii<7;ii++)
	    if (a.repeatDays[ii])
	      ical_write(out_file, "%d ", ii+1);
	  ical_write_str(out_file, "\n");
	}
      } else if(a.repeatType == repeatYearly) {
	ical_write(out_file, "$i monthrepeat %d $begin\n",
		   12 * a.repeatFrequency);
      }
      ical_write_str(out_file, "$i start $begin\n");
      if (!a.repeatForever)
	ical_write(out_file, "$i finish [date make %d %d %d]\n",
		   a.repeatEnd.tm_mday,
		   a.repeatEnd.tm_mon+1, 
		   a.repeatEnd.tm_year+1900);
      if (a.exceptions)
	for (j=0;j<a.exceptions;j++)
	  ical_write(out_file, "$i deleteon [date make %d %d %d]\n",
		     a.exception[j].tm_mday,
		     a.exception[j].tm_mon+1,
		     a.exception[j].tm_year+1900);
    } else
      ical_write_str(out_file, "$i date $begin\n");

    /* Uid */
    snprintf(id_buf, sizeof(id_buf),
	     "%lx",
	     uid);
    ical_write(out_file, "$i option PilotRecordId %s\n",
	       id_buf);

    /* End of row */
    ical_write_str(out_file, "cal add $i\n");


    /* Increase counters */
    out_file->num_recs++;
    out_file->next_rec++;

    /* Update statistics */
    out_file->records_written++;
  } /* else deleted record found */

  /* Debug */
  debug_message("Leaving ical_write_row\n");
}



/* For statistics */

/* Show output statistics */
void
ical_show_write_statistics (struct ical_file_data * out_file)
{

  /* Debug */
  debug_message("Entering ical_show_write_statistics\n");

  info_message("Output file <%s>, format <%s>:\n",
	       (out_file->filename) ? out_file->filename : "stdout",
	       dataformat2txt(DATA_FORMAT_ICAL));
  info_message("Lines written: %d\n",
	       out_file->lines_written);
  info_message("Records written: %d\n",
	       out_file->records_written);

  /* Debug */
  debug_message("Leaving ical_show_write_statistics\n");
}



/* Private functions */

/* Write datebook file header */
void
ical_write_header (struct ical_file_data * out_file, struct header_data * header)
{

  /* Debug */
  debug_message("Entering ical_write_header\n");


  /* No header data information will be written */


  /* Print leading ical information */
  ical_write_str(out_file, ICAL_HEADER);

  /* Debug */
  debug_message("Leaving ical_write_header\n");
}



/* For IO */

/* Write one single string to output file
 *
 * Especially useful for printing multi-line text, to properly count
 * line numbers
 */
void
ical_write_str (struct ical_file_data * out_file, const char * out_string)
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
ical_write (struct ical_file_data * out_file, const char * format, ...)
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


/* Quote & skip dangerous characters */
char *
tclquote(char * in)
{
  static char * buffer = 0;
  char * out;
  char * pos;
  int len;
  
  /* Skip leading bullet (and any whitespace after) */
  if (in[0] == '\x95') {
    ++in;
    while(in[0] == ' ' || in[0] == '\t') {
      ++in;
    }
  }

  len = 3;
  pos = in;
  while(*pos) {
    if((*pos == '\\') || (*pos == '"') || (*pos == '[') || (*pos == '{') || (*pos == '$'))
      len++;
    len++;
    pos++;
  }
  
  if (buffer)
    free(buffer);
  buffer = (char*)malloc(len);
  out = buffer;

  pos = in;
  *out++ = '"';
  while(*pos) {
    if((*pos == '\\') || (*pos == '"') || (*pos == '[') || (*pos == '{') || (*pos == '$'))
      *out++ = '\\';
    *out++=*pos++;
  }
  *out++ = '"';
  *out++ = '\0';
  
  return buffer;
}
