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
#include "pilot-datebook-csv.h"

/* csv is a very flexible format, which is designed to be easily readable
 * by a spreadsheet program.
 */


/* Constants */
const char CSV_DEFAULT_FORMAT[]="uid,attributes,category,untimed,beginDate,beginTime,endDate,endTime,description,note,alarm,advance,advanceUnit,repeatType,repeatForever,repeatEnd,repeatFrequency,repeatDay,repeatWeekdays,repeatWeekstart";
const char CSV_OUTLOOK_FORMAT[]="description,beginDate,beginTime,endDate,endTime,untimed,note";
const char CSV_OUTLOOK_HEADER[]="\"Subject\",\"Start Date\",\"Start Time\",\"End Date\",\"End Time\",\"All day event\",\"Description\"";
const char CSV_OUTLOOK_NAME[]="Outlook";
const char CSV_HEADER_WILDCARD='*';


/* Public functions */

/* For init */

/* Initialize read data structure */
int
csv_init_read (struct csv_file_data * in_file)
{

  /* Debug */
  debug_message("Entering csv_init_read\n");

  /* Init own data structure */
  in_file->filename = NULL;
  in_file->file = NULL;
  in_file->file_is_open = FALSE;
  in_file->line_buffer = NULL;
  in_file->line_no = 0;
  in_file->num_recs = 0;
  in_file->next_rec = 0;

  in_file->valuenames = strdup(CSV_DEFAULT_FORMAT);
  in_file->header = in_file->valuenames;
  in_file->field_top = NULL;
  in_file->field_last = NULL;

  in_file->lines_read = 0;
  in_file->lines_written = 0;
  in_file->records_read = 0;
  in_file->records_written = 0;

  /* Debug */
  debug_message("Leaving csv_init_read\n");

  return TRUE;
}


/* Initialize write data structure */
int
csv_init_write (struct csv_file_data * out_file)
{

  /* Debug */
  debug_message("Entering csv_init_write\n");

  /* Init own data structure */
  out_file->filename = NULL;
  out_file->file = NULL;
  out_file->file_is_open = FALSE;
  out_file->line_buffer = NULL;
  out_file->line_no = 0;
  out_file->num_recs = 0;
  out_file->next_rec = 0;

  out_file->valuenames = strdup(CSV_DEFAULT_FORMAT);
  out_file->header = out_file->valuenames;
  out_file->field_top = NULL;
  out_file->field_last = NULL;

  out_file->lines_read = 0;
  out_file->lines_written = 0;
  out_file->records_read = 0;
  out_file->records_written = 0;

  /* Debug */
  debug_message("Leaving csv_init_write\n");

  return TRUE;
}


/* Destroy read data structure */
void
csv_exit_read (struct csv_file_data * in_file)
{
  struct csv_field_data * csv_field;


  /* Debug */
  debug_message("Entering csv_exit_read\n");

  /* Free memory */
  if (in_file->header == in_file->valuenames)
    /* Do not call free() twice */
    in_file->header = NULL;
  if (in_file->filename)
    free (in_file->filename);
  if (in_file->valuenames)
    free (in_file->valuenames);
  if (in_file->header)
    free (in_file->header);

  /* Now destroy all csv fields */
  csv_field = csvlist_get_first_field(in_file);
  while (csv_field != NULL) {
    debug_message("Removing csv field...\n");

    /* Remove csv field from in_file */
    csvlist_field_del(in_file, csv_field);

    /* Destroy csv field data structure */
    csv_free_field(csv_field);

    /* Next = now first (after removal of previous first) */
    csv_field = csvlist_get_first_field(in_file);
  } /* while */

  /* Init own data structure */
  in_file->filename = NULL;
  in_file->file = NULL;
  in_file->file_is_open = FALSE;
  in_file->line_buffer = NULL;
  in_file->line_no = 0;
  in_file->num_recs = 0;
  in_file->next_rec = 0;

  in_file->valuenames = NULL;
  in_file->header = NULL;
  in_file->field_top = NULL;
  in_file->field_last = NULL;

  in_file->lines_read = 0;
  in_file->lines_written = 0;
  in_file->records_read = 0;
  in_file->records_written = 0;

  /* Debug */
  debug_message("Leaving csv_exit_read\n");
}


/* Destroy write data structure */
void
csv_exit_write (struct csv_file_data * out_file)
{
  struct csv_field_data * csv_field;


  /* Debug */
  debug_message("Entering csv_exit_write\n");

  /* Free memory */
  if (out_file->header == out_file->valuenames)
    /* Do not call free twice */
    out_file->header = NULL;
  if (out_file->filename)
    free (out_file->filename);
  if (out_file->valuenames)
    free (out_file->valuenames);
  if (out_file->header)
    free (out_file->header);

  /* Now destroy all csv fields */
  csv_field = csvlist_get_first_field(out_file);
  while (csv_field != NULL) {
    debug_message("Removing csv field...\n");

    /* Remove csv field from out_file */
    csvlist_field_del(out_file, csv_field);

    /* Destroy csv field data structure */
    csv_free_field(csv_field);

    /* Next = now first (after removal of previous first) */
    csv_field = csvlist_get_first_field(out_file);
  } /* while */

  /* Init own data structure */
  out_file->filename = NULL;
  out_file->file = NULL;
  out_file->file_is_open = FALSE;
  out_file->line_buffer = NULL;
  out_file->line_no = 0;
  out_file->num_recs = 0;
  out_file->next_rec = 0;

  out_file->valuenames = NULL;
  out_file->header = NULL;
  out_file->field_top = NULL;
  out_file->field_last = NULL;

  out_file->lines_read = 0;
  out_file->lines_written = 0;
  out_file->records_read = 0;
  out_file->records_written = 0;

  /* Debug */
  debug_message("Leaving csv_exit_write\n");
}


/* Set read command line option */
int
csv_set_read_option (struct csv_file_data * in_file, char opt, char * opt_arg)
{
  int rc = FALSE;
  int header_follows = FALSE;


  /* Debug */
  debug_message("Entering csv_set_read_option\n");

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

    case 'o':
      /* List of values to be printed */
      if (opt_arg != NULL
	  && *opt_arg != '\0') {
	if (strcmp(opt_arg, CSV_OUTLOOK_NAME) == 0) {
	  if (in_file->valuenames == in_file->header) {
	    if (in_file->valuenames)
	      free (in_file->valuenames);
	  }
	  else {
	    if (in_file->valuenames)
	      free (in_file->valuenames);
	    if (in_file->header)
	      free (in_file->header);
	  }
	  in_file->valuenames = strdup(CSV_OUTLOOK_FORMAT);
	  in_file->header = strdup(CSV_OUTLOOK_HEADER);
	}
	else {
	  header_follows = (in_file->valuenames == in_file->header);
	  if (in_file->valuenames)
	    free (in_file->valuenames);
	  in_file->valuenames = strdup(opt_arg);
	  /* Header follows value list, unless header is explicitly specified */
	  if (header_follows)
	    in_file->header = in_file->valuenames;
	}
	rc = TRUE;
      }
      break;

    case 't':
      /* Fileheader to be printed */
      if (opt_arg != NULL) {
	if (in_file->header
	    && in_file->header != in_file->valuenames)
	  /* Only free header if it is not also used as value list */
	  free (in_file->header);
	if (strcmp(opt_arg, CSV_OUTLOOK_NAME) == 0)
	  in_file->header = strdup(CSV_OUTLOOK_HEADER);
	else
	  in_file->header = strdup(opt_arg);
	rc = TRUE;
      }
      else {
	/* opt_arg may be empty string, to avoid printing header */
	if (in_file->header
	    && in_file->header != in_file->valuenames)
	  /* Only free header if it is not also used as value list */
	  free (in_file->header);
	in_file->header = NULL;
	rc = TRUE;
      }
      break;

    default:
      fprintf(stderr, "Can not process read option <%c> for input file\n",
	      opt);
    }

  /* Debug */
  debug_message("Leaving csv_set_read_option\n");

  return rc;
}


/* Set write command line option */
int
csv_set_write_option (struct csv_file_data * out_file, char opt, char * opt_arg)
{
  int rc = FALSE;
  int header_follows = FALSE;


  /* Debug */
  debug_message("Entering csv_set_write_option\n");

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

    case 'o':
      /* List of values to be printed */
      if (opt_arg != NULL
	  && *opt_arg != '\0') {
	if (strcmp(opt_arg, CSV_OUTLOOK_NAME) == 0) {
	  if (out_file->valuenames == out_file->header) {
	    if (out_file->valuenames)
	      free (out_file->valuenames);
	  }
	  else {
	    if (out_file->valuenames)
	      free (out_file->valuenames);
	    if (out_file->header)
	      free (out_file->header);
	  }
	  out_file->valuenames = strdup(CSV_OUTLOOK_FORMAT);
	  out_file->header = strdup(CSV_OUTLOOK_HEADER);
	}
	else {
	  header_follows = (out_file->valuenames == out_file->header);
	  if (out_file->valuenames)
	    free (out_file->valuenames);
	  out_file->valuenames = strdup(opt_arg);
	  /* Header follows value list, unless header is explicitly specified */
	  if (header_follows)
	    out_file->header = out_file->valuenames;
	}
	rc = TRUE;
      }
      break;

    case 't':
      /* Fileheader to be written */
      if (opt_arg != NULL) {
	if (out_file->header
	    && out_file->header != out_file->valuenames)
	  /* Only free header if it is not also used as value list */
	  free (out_file->header);
	if (strcmp(opt_arg, CSV_OUTLOOK_NAME) == 0)
	  out_file->header = strdup(CSV_OUTLOOK_HEADER);
	else
	  out_file->header = strdup(opt_arg);
	rc = TRUE;
      }
      else {
	/* opt_arg may be empty string, to avoid printing header */
	if (out_file->header
	    && out_file->header != out_file->valuenames)
	  /* Only free header if it is not also used as value list */
	  free (out_file->header);
	out_file->header = NULL;
	rc = TRUE;
      }
      break;

    default:
      fprintf(stderr, "Can not process write option <%c> for input file\n",
	      opt);
    }

  /* Debug */
  debug_message("Leaving csv_set_write_option\n");

  return rc;
}



/* For opening & closing */

/* Open input data file for reading */
void
csv_open_read (struct csv_file_data * in_file, struct header_data * header)
{

  /* Debug */
  debug_message("Entering csv_open_read\n");

  /* Parse file format */
  /* Parsing file format will be done when reading header */

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
  csv_read_header (in_file, header);

  /* Debug */
  debug_message("Leaving csv_open_read\n");
}


/* Open output data file for writing */
void
csv_open_write (struct csv_file_data * out_file, struct header_data * header)
{

  /* Debug */
  debug_message("Entering csv_open_write\n");

  /* Parse file format */
  if (out_file->valuenames)
    csv_parse_valuenames(out_file, out_file->valuenames);
  else
    error_message("Please specify csv output format for file <%s>!\n",
		  out_file->filename);

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
  csv_write_header (out_file, header);

  /* Debug */
  debug_message("Leaving csv_open_write\n");
}


/* Close input file at end of processing */
void
csv_close_read (struct csv_file_data * in_file, struct header_data * header)
{

  /* Debug */
  debug_message("Entering csv_close_read\n");

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
  debug_message("Leaving csv_close_read\n");
}


/* Close output file at end of processing */
void
csv_close_write (struct csv_file_data * out_file, struct header_data * header)
{

  /* Debug */
  debug_message("Entering csv_close_write\n");

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
  debug_message("Leaving csv_close_write\n");
}


/* Close input file in case of an error */
void
csv_abort_read (struct csv_file_data * in_file)
{

  /* Debug */
  debug_message("Entering csv_abort_read\n");

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
  debug_message("Leaving csv_abort_read\n");
}


/* Close output file in case of an error */
void
csv_abort_write (struct csv_file_data * out_file)
{

  /* Debug */
  debug_message("Entering csv_abort_write\n");

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
  debug_message("Leaving csv_abort_write\n");
}



/* For reading */

/* Identify end of file */
int
csv_read_eof (struct csv_file_data * in_file)
{

  /* Debug */
  debug_message("Entering & Leaving csv_read_eof\n");

  return feof(in_file->file);
}


/* Read routine for appointment in current record */
void
csv_read_row (struct csv_file_data * in_file, struct row_data * row)
{
  char buffer[0xffff];
  char * pos1;
  char * pos2;
  struct csv_field_data * csv_field;


  /* Debug */
  debug_message("Entering csv_read_row\n");

  /* Skip empty lines */
  csv_read_line(buffer, sizeof(buffer), in_file);
  while (buffer[0] == '\n') {
    csv_read_line(buffer, sizeof(buffer), in_file);
  }
  if (feof(in_file->file))
    error_message("Read unexpected end of input file at line %d\n\n",
		  in_file->line_no);

  /* Now parse; per default assume that reading will fail */
  rowInit(row);

  do {
    /* Init data structure */
    rowInit(row);

    /* Read all data items, delimited by ',' as delimiter */
    pos1 = buffer;

    /* Truncate trailing newline */
    if (buffer[strlen(buffer)-1] == '\n')
      buffer[strlen(buffer)-1] = '\0';

    /* Process line */

    /* Check for quoted text */
    pos2 = data_find_delimiter(pos1, ',');

    /* Read a row completely according to csv field data */
    csv_field = csvlist_get_first_field(in_file);
    while (pos1 != NULL
	   && csv_field != NULL) {

      /* Set pos2 properly before end of line */
      if (pos2 == NULL)
	pos2 = pos1 + strlen(pos1);

      /* Read one field in current row */
      csv_row_read(in_file, csv_field, row, pos1, pos2);

      /* Next csv field */
      csv_field = csvlist_get_next_field(in_file, csv_field);

      /* Find next data item */
      if (pos2 != NULL
	  && *pos2 != '\0') {
	/* End of line not yet reached */
	/* This will jump over one single trailing comma which has been
	 * provided by accident (format error) without complaining, but
	 * this should hopefully not be a big issue...
	 */
	pos1 = pos2 +1;
	pos2 = data_find_delimiter(pos1, ',');
      }
      else if (pos1 != pos2) {
	/* Hit end of line for first time, process last field */
	pos1 = pos2;
      }
      else {
	/* Hit end of line for second time, now exit */
	break;
      }
    }

    /* Check whether all expected fields had really been provided */
    if (csv_field != NULL)
      warn_message("Field <%s> has not been provided in file <%s> line <%d>\n",
		    csv_field->field.name,
		    in_file->filename,
		    in_file->line_no);

    /* Check whether whole row had been processed */
    if (pos1 != NULL
	&& *pos1 != '\0')
      warn_message("Could not process trailing input <%s> from file <%s> line <%d>\n",
		   pos1,
		   in_file->filename,
		   in_file->line_no);

    /* If we reach here, then we have successfully read a row */
    setRowIsValid(row, TRUE);

    /* Find next non-empty line to position for next read */
    buffer[0] = '\n';
    while (!feof(in_file->file)
	   && buffer[0] == '\n') {
      csv_read_line(buffer, sizeof(buffer), in_file);
    }

    /* Put last line back, to be read on next read attempt */
    if (!feof(in_file->file))
      csv_pushback_line(buffer, in_file);

    /* Increment counters */
    in_file->num_recs++;
    in_file->next_rec++;

    /* Update statistics */
    in_file->records_read++;
  } while (FALSE);

  /* Debug */
  debug_message("Leaving csv_read_row\n");
}



/* For writing */

/* Write an appointment record */
void
csv_write_row (struct csv_file_data * out_file, struct header_data * header, struct row_data * row)
{
  struct csv_field_data * csv_field;


  /* Debug */
  debug_message("Entering csv_write_row\n");

  if (!getRowIsValid(row))
    error_message("Can not write invalid row.\n");

  /* Write a row completely according to csv field data */
  csv_field = csvlist_get_first_field(out_file);
  while (csv_field != NULL) {
    /* Write one field in current row */
    csv_row_write(out_file, csv_field, row);

    /* Next csv assign */
    csv_field = csvlist_get_next_field(out_file, csv_field);
    if (csv_field != NULL)
      /* Write field separator */
      csv_write_str(out_file, ",");
  } /* while */

  /* Write end of line */
  csv_write_str(out_file, "\n");

  /* Increase counters */
  out_file->num_recs++;
  out_file->next_rec++;

  /* Update statistics */
  out_file->records_written++;

  /* Debug */
  debug_message("Leaving csv_write_row\n");
}



/* For statistics */

/* Show input statistics */
void
csv_show_read_statistics (struct csv_file_data * in_file)
{
  struct csv_field_data * csv_field;


  /* Debug */
  debug_message("Entering csv_show_read_statistics\n");

  info_message("Input file <%s>, format <%s>:\n",
	       (in_file->filename) ? in_file->filename : "stdin",
	       dataformat2txt(DATA_FORMAT_CSV));
  if (in_file->valuenames != NULL)
    info_message("Input file format <%s>\n",
		 in_file->valuenames);
  if (in_file->valuenames != NULL)
    info_message("Input file header <%s>\n",
		 in_file->header);
  info_message("Lines read: %d\n",
	       in_file->lines_read);
  info_message("Records read: %d\n",
	       in_file->records_read);

  /* Statistics for all csv fields */
  csv_field = csvlist_get_first_field(in_file);
  while (csv_field != NULL) {
    /* Show statistics for this csv field */
    info_message("Csv field <%s> was read %ld times\n",
                 csv_field->field.name,
                 csv_field->num_read);

    /* Next csv field */
    csv_field = csvlist_get_next_field(in_file, csv_field);
  } /* while */

  /* Debug */
  debug_message("Leaving csv_show_read_statistics\n");
}


/* Show output statistics */
void
csv_show_write_statistics (struct csv_file_data * out_file)
{
  struct csv_field_data * csv_field;


  /* Debug */
  debug_message("Entering csv_show_write_statistics\n");

  info_message("Output file <%s>, format <%s>:\n",
	       (out_file->filename) ? out_file->filename : "stdout",
	       dataformat2txt(DATA_FORMAT_CSV));
  if (out_file->valuenames != NULL)
    info_message("Output file format <%s>\n",
		 out_file->valuenames);
  if (out_file->header != NULL)
    info_message("Output file header <%s>\n",
		 out_file->header);
  info_message("Lines written: %d\n",
	       out_file->lines_written);
  info_message("Records written: %d\n",
	       out_file->records_written);

  /* Statistics for all csv fields */
  csv_field = csvlist_get_first_field(out_file);
  while (csv_field != NULL) {
    /* Show statistics for this csv field */
    info_message("Csv field <%s> was written %ld times\n",
                 csv_field->field.name,
                 csv_field->num_written);

    /* Next csv field */
    csv_field = csvlist_get_next_field(out_file, csv_field);
  } /* while */

  /* Debug */
  debug_message("Leaving csv_show_write_statistics\n");
}



/* Private functions */

/* Read file header */
void
csv_read_header (struct csv_file_data * in_file, struct header_data * header)
{
  char buffer[0xffff];


  /* Debug */
  debug_message("Entering csv_read_header\n");

  /* Init data structures, no header to read for this data format */
  header->isValid = FALSE;

  /* Read general database header data */
  do {
    /* Skip empty lines */
    csv_read_line(buffer, sizeof(buffer), in_file);
    while (buffer[0] == '\n') {
      csv_read_line(buffer, sizeof(buffer), in_file);
    }
    if (feof(in_file->file))
      error_message("Read unexpected end of input file at line %d\n\n",
		    in_file->line_no);

    /* Check expected data format is present */
    if ((in_file->header != NULL
	 && *in_file->header != '\0')
	&& (in_file->valuenames == NULL
	    || *in_file->valuenames == '\0')) {
      /* Can not provide header without value list */
      error_message("Please provide csv file format.\n");
    }

    if ((in_file->header == NULL
	 || *in_file->header == '\0')
	&& (in_file->valuenames == NULL
	    || *in_file->valuenames == '\0')) {
      /* Neither header nor valuenames present => use first line of file */

      /* Remove trailing newline */
      if (buffer[strlen(buffer) -1] == '\n')
	buffer[strlen(buffer) -1] = '\0';

      if (in_file->header
	  && in_file->header != in_file->valuenames)
	/* Only free header if it is not also used as value list */
	free (in_file->header);
      if (in_file->valuenames)
	free (in_file->valuenames);
      in_file->valuenames = strdup(buffer);
      in_file->header = in_file->valuenames;

      /* Skip empty lines to find begin of row data */
      csv_read_line(buffer, sizeof(buffer), in_file);
      while (buffer[0] == '\n') {
	csv_read_line(buffer, sizeof(buffer), in_file);
      }
    } /* if no file format present */
    else if ((in_file->header == NULL
	      || *in_file->header == '\0')
	     && (in_file->valuenames != NULL
		 && *in_file->valuenames != '\0')) {
      /* File format has been provided */

      /* Empty header, but valuenames was given
       * => use valuenames, file contains no header
       */
    }
    else {
      /* Valuenames/header were given, therefore first line of file
       * has to contain a header
       */

      /* Remove trailing newline */
      if (buffer[strlen(buffer) -1] == '\n')
	buffer[strlen(buffer) -1] = '\0';

      if (in_file->header
	  && csv_verify_header(buffer, in_file->header) == TRUE) {
	/* Header matches => use given valuenames */

	/* Skip empty lines to find begin of row data */
	csv_read_line(buffer, sizeof(buffer), in_file);
	while (buffer[0] == '\n') {
	  csv_read_line(buffer, sizeof(buffer), in_file);
	}
      }
      /* Header does not match: try matching default format */
      else if (csv_verify_header(buffer, CSV_DEFAULT_FORMAT) == TRUE) {
	/* Default matches => use default as header & valuenames */

	if (in_file->header
	    && in_file->header != in_file->valuenames)
	  /* Only free header if it is not also used as value list */
	  free (in_file->header);
	if (in_file->valuenames)
	  free (in_file->valuenames);
	in_file->valuenames = strdup(CSV_DEFAULT_FORMAT);
	in_file->header = in_file->valuenames;

	/* Skip empty lines to find begin of row data */
	csv_read_line(buffer, sizeof(buffer), in_file);
	while (buffer[0] == '\n') {
	  csv_read_line(buffer, sizeof(buffer), in_file);
	}
      } /* if default matches */
      else {
	/* Default does not match => use first line from file */

	/* Remove trailing newline */
	if (buffer[strlen(buffer) -1] == '\n')
	  buffer[strlen(buffer) -1] = '\0';

	if (in_file->header
	    && in_file->header != in_file->valuenames)
	  /* Only free header if it is not also used as value list */
	  free (in_file->header);
	if (in_file->valuenames)
	  free (in_file->valuenames);
	in_file->valuenames = strdup(buffer);
	in_file->header = in_file->valuenames;

	/* Skip empty lines to find begin of row data */
	csv_read_line(buffer, sizeof(buffer), in_file);
	while (buffer[0] == '\n') {
	  csv_read_line(buffer, sizeof(buffer), in_file);
	}
      } /* else default does not match */
    } /* else valuenames/header were given */

    /* Parse file format */
    if (in_file->valuenames != NULL)
      csv_parse_valuenames(in_file, in_file->valuenames);
    else
      error_message("Can not process empty valuenames\n");

    /* Put last line back, to be read on next read attempt */
    csv_pushback_line(buffer, in_file);

  } while (FALSE);

  /* Debug */
  debug_message("Leaving csv_read_header\n");
}


/* Write datebook file header */
void
csv_write_header (struct csv_file_data * out_file, struct header_data * header)
{

  /* Debug */
  debug_message("Entering csv_write_header\n");

  /* Write header */
  if (out_file->header != NULL) {
    csv_write_str(out_file, out_file->header);
    csv_write_str(out_file, "\n");
  }

  /* Debug */
  debug_message("Leaving csv_write_header\n");
}


/* Verify whether expected header has been found */
int
csv_verify_header (const char * buffer, const char * header)
{
  const char * pos1;
  const char * pos2;
  struct value_data value1;
  struct value_data value2;
  int rc;


  /* Wild card match for whole header */
  if (*header == CSV_HEADER_WILDCARD
      && *(header +1) == '\0')
    return TRUE;

  rc = FALSE;
  while (buffer != NULL
	 && *buffer != '\0'
	 && header != NULL
	 && *header != '\0') {
    /* Get field name from buffer */
    pos1 = data_find_delimiter(buffer, ',');
    if (pos1 == NULL)
      pos1 = buffer + strlen(buffer);
    value1.type = DATEBOOK_FIELD_STR;
    value_init(&value1, buffer, pos1);

    /* Get field name from header */
    pos2 = data_find_delimiter(header, ',');
    if (pos2 == NULL)
      pos2 = header + strlen(header);
    value2.type = DATEBOOK_FIELD_STR;
    value_init(&value2, header, pos2);

    /* Header field name = wildcard? Then success */
    if (*value2.literal.lit_str != CSV_HEADER_WILDCARD
	|| *(value2.literal.lit_str +1) != '\0')
      /* No wildcard, therefore compare fields */
      if (strcmp(value1.literal.lit_str, value2.literal.lit_str) != 0)
	break;

    buffer = pos1;
    header = pos2;
    /* Skip delimiter */
    if (*buffer == ',')
      buffer++;
    if (*header == ',')
      header++;
  } /* while */

  /* Success, if we managed to compare everything */
  if (buffer != NULL
      && *buffer == '\0'
      && header != NULL
      && *header == '\0')
    rc = TRUE;

  return rc;
}




/* For IO */

/* Read one line from input file */
void
csv_read_line (char * buffer, int buffer_size, struct csv_file_data * in_file)
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
csv_pushback_line (char * buffer, struct csv_file_data * in_file)
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
csv_write_str (struct csv_file_data * out_file, const char * out_string)
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
csv_write (struct csv_file_data * out_file, const char * format, ...)
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


/* For field handling */

/* Parse values list */
void
csv_parse_valuenames (struct csv_file_data * file, char * valuenames)
{
  struct csv_field_data * csv_field;
  char * pos1;
  char * pos2;


  /* Debug */
  debug_message("Entering csv_parse_valuenames\n");

  /* Expected input parameter: csv order fields */

  /* Loop over all csv fields, delimited by ',' as delimiter */
  pos1 = valuenames;
  pos2 = data_index(pos1, ',');
  while (pos1 != NULL
         && *pos1 != '\0') {

    /* Alloc csv field data */
    csv_field = csv_new_field();

    /* Init data structure */
    csv_field->num_read = 0;
    csv_field->num_written = 0;

    /* Insert field data into csv field list */
    field_init(&(csv_field->field), pos1, pos2);
    csv_field->next = NULL;

    /* Insert csv field into csv structure */
    csvlist_field_add(file, csv_field);

    /* Extract next csv field, skipping delimiter */
    if (pos2 != NULL) {
      pos1 = pos2 +1;
      pos2 = data_index(pos1, ',');
    }
    else {
      break;
    }
  }

  /* Debug */
  debug_message("Entering csv_parse_valuenames\n");
}



/* Helper */

/* Append one csv_field to file */
void
csvlist_field_add(struct csv_file_data * file, struct csv_field_data * add_csv_field)
{

  /* Add to list */
  if (add_csv_field == NULL) {
    /* No csv_field provided => nothing to add */
  }
  else if (file == NULL) {
    /* No file provided => nothing to add */
  }
  else if (file->field_last == NULL) {
    /* First entry into list */
    add_csv_field->next = NULL;
    file->field_last = add_csv_field;
    file->field_top = add_csv_field;
  }
  else {
    /* other entries exist already */
    add_csv_field->next = NULL;
    file->field_last->next = add_csv_field;
    file->field_last = add_csv_field;
  }
}


/* Remove one csv_field from file */
void
csvlist_field_del(struct csv_file_data * file, struct csv_field_data * del_csv_field)
{

  /* Remove from list */
  if (del_csv_field == NULL) {
    /* No csv_field provided => nothing to remove */
  }
  else if (file == NULL) {
    /* No file provided => nothing to remove */
  }
  else if (file->field_last == NULL) {
    /* Nothing in list => nothing to remove */
  }
  else {
    if (file->field_last == del_csv_field) {
      /* Last in list => iterate through list to find new last */
      if (file->field_top != del_csv_field)
	debug_message("Removing last - inefficient use of csvlist_del\n");
      file->field_last = file->field_top;
      while (file->field_last->next != NULL
	     && file->field_last->next != del_csv_field) {
	file->field_last = file->field_last->next;
      } /* while */
    }
    if (file->field_top == del_csv_field) {
      /* First in list => remove */
      file->field_top = del_csv_field->next;
    }
  }

  /* Remove linkage */
  del_csv_field->next = NULL;
}


/* Get first csv_field from file */
struct csv_field_data *
csvlist_get_first_field(struct csv_file_data * file)
{
  if (file == NULL)
    return NULL;
  else
    return file->field_top;
}


/* Get next csv_field from file */
struct csv_field_data *
csvlist_get_next_field(struct csv_file_data * file, struct csv_field_data * cur_csv_field)
{
  if (cur_csv_field == NULL)
    return NULL;
  else
    return cur_csv_field->next;
}


/* Alloc new csv field data */
struct csv_field_data *
csv_new_field (void)
{
  return (struct csv_field_data *) malloc (sizeof(struct csv_field_data));
}


/* De-alloc csv field data */
void
csv_free_field (struct csv_field_data * csv_field)
{
  free ((void *) csv_field);
}


/* Write one field for current row */
void
csv_row_write(struct csv_file_data * file, struct csv_field_data * csv_field, struct row_data * row)
{
  struct value_data value;
  char buffer[100];


  /* Debug */
  debug_message("Entering csv_row_write\n");

  /* No csv field? Then do nothing. */
  if (csv_field == NULL)
    return;

  /* Safety net */
  if (row == NULL)
    return;

  /* Get field data */
  value = row_get_field(row, &(csv_field->field));
  if (value.type != csv_field->field.type)
    error_message("Result type of field <%s> does not match expected type\n",
		  csv_field->field.name);

  /* Print result data */
  buffer[0] = '\0';
  switch (csv_field->field.type) {
  case DATEBOOK_FIELD_LONG:
    snprintf(buffer, sizeof(buffer), "%lu", value.literal.lit_long);
    csv_write_str(file, buffer);
    csv_field->num_written++;
    break;

  case DATEBOOK_FIELD_INT:
    snprintf(buffer, sizeof(buffer), "%d", value.literal.lit_int);
    csv_write_str(file, buffer);
    csv_field->num_written++;
    break;

  case DATEBOOK_FIELD_TIME:
    {
      const char DateOnlyName[] = "Date";
      const char * pos1;
      time_t t;

      buffer[0] = '\0';
      t = mktime(&value.literal.lit_time);
      if (t > 0) {
	/* Only print if date is really present */
	pos1 = csv_field->field.name
	  - strlen(DateOnlyName)
	  + strlen(csv_field->field.name);
	/* Write date & time, or only date? */
	if (pos1 >= csv_field->field.name
	    && strcmp(pos1, DateOnlyName) == 0)
	  write_iso_date_str(t,
			     buffer,
			     sizeof(buffer));
	else
	  write_iso_full_time_str(t,
				  buffer,
				  sizeof(buffer));
	csv_write_str(file, buffer);
	csv_field->num_written++;
      }
    }
    break;

  case DATEBOOK_FIELD_SECONDS:
    write_iso_gmtime_str(value.literal.lit_seconds, buffer, sizeof(buffer));
    csv_write_str(file, buffer);
    csv_field->num_written++;
    break;

  case DATEBOOK_FIELD_STR:
    /* Write strings within double quotes */
    if (value.literal.lit_str == NULL
	|| *value.literal.lit_str == '\0') {
      csv_write_str(file, "\"\"");
    }
    else {
      text_quote(value.literal.lit_str, buffer, sizeof(buffer));
      csv_write(file, "\"%s\"", buffer);
      csv_field->num_written++;
    }
    break;

  default:
    break;
  }

  /* Statistics: see above */

  /* Debug */
  debug_message("Entering csv_row_write\n");
}


/* Read one field for current row */
void
csv_row_read(struct csv_file_data * file, struct csv_field_data * csv_field, struct row_data * row, char * in_start, char * in_end)
{
  struct value_data value;


  /* Debug */
  debug_message("Entering csv_row_read\n");

  /* No csv field? Then do nothing. */
  if (csv_field == NULL)
    return;

  /* Safety net */
  if (row == NULL)
    return;

  /* Read data */
  value.type = csv_field->field.type;
  if (in_start != in_end) {
    /* Non-empty data values */
    value_init(&value, in_start, in_end);
  }
  else {
    /* Empty data values are allowed for csv format */
    switch(value.type) {
    case DATEBOOK_FIELD_LONG:
      value.literal.lit_long = 0;
      break;

    case DATEBOOK_FIELD_INT:
      value.literal.lit_int = 0;
      break;

    case DATEBOOK_FIELD_TIME:
      {
	time_t t;

	t = 0;
	value.literal.lit_time = *localtime(&t);
      }
      break;

    case DATEBOOK_FIELD_SECONDS:
      value.literal.lit_seconds = 0;
      break;

    case DATEBOOK_FIELD_STR:
      value.literal.lit_str = NULL;
      break;

    default:
      error_message("Can not handle unknown field data type\n");
    } /* switch */
  } /* else empty values */

  if (value.type != csv_field->field.type)
    error_message("Result type of field <%s> does not match read type\n",
		  csv_field->field.name);

  /* Set field data */
  row_set_field(row, &(csv_field->field), &value);

  /* Statistics */
  csv_field->num_read++;

  /* Debug */
  debug_message("Leaving csv_row_read\n");
}
