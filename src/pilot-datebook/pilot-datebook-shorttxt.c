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
#include "pilot-datebook-shorttxt.h"

/* shorttxt is a loosing file format. Its main purpose is to allow for easy
   reading of non-repeating datebook events.
  
   It will drop/ignore the following fields:
   - uid
   - attributes
   - category
   - all repeat fields
   - note
  
   Untimed events are supported. They are assumed if either start or end
   date & time field is empty for a record.
  
   Please note that contrary to the original format for install-datebook,
   newlines and tabulators within the description are allowed, but will be
   quoted in printf matter as "\n" and "\t" in the output file to allow for
   proper reading.
 */


/* Read/write header format */
/* No header data will be written */

/* Read/write row format */
const char SHORTTXT_ROW_FORMAT[] 	= "%s\t%s\t%s\t%s\n";
const int SHORTTXT_ROW_MAX_INPUT_PARAM 	= 4;
const char SHORTTXT_ROW_ALARM_FORMAT[] 	= "%d%s";


/* Public functions */

/* For init */

/* Initialize read data structure */
int shorttxt_init_read(struct shorttxt_file_data *in_file)
{
	debug_message("Entering shorttxt_init_read\n");

	/* Init own data structure */
	in_file->filename 		= NULL;
	in_file->file 			= NULL;
	in_file->file_is_open 		= FALSE;
	in_file->line_buffer 		= NULL;
	in_file->line_no 		= 0;
	in_file->num_recs 		= 0;
	in_file->next_rec 		= 0;
	in_file->lines_read 		= 0;
	in_file->lines_written 		= 0;
	in_file->records_read 		= 0;
	in_file->records_written 	= 0;

	debug_message("Leaving shorttxt_init_read\n");

	return TRUE;
}


/* Initialize write data structure */
int shorttxt_init_write(struct shorttxt_file_data *out_file)
{
	debug_message("Entering shorttxt_init_write\n");

	/* Init own data structure */
	out_file->filename 		= NULL;
	out_file->file 			= NULL;
	out_file->file_is_open 		= FALSE;
	out_file->line_buffer 		= NULL;
	out_file->line_no 		= 0;
	out_file->num_recs 		= 0;
	out_file->next_rec 		= 0;
	out_file->lines_read 		= 0;
	out_file->lines_written 	= 0;
	out_file->records_read 		= 0;
	out_file->records_written 	= 0;

	debug_message("Leaving shorttxt_init_write\n");

	return TRUE;
}


/* Destroy read data structure */
void shorttxt_exit_read(struct shorttxt_file_data *in_file)
{
	debug_message("Entering shorttxt_exit_read\n");

	/* Free memory */
	if (in_file->filename)
		free(in_file->filename);

	debug_message("Leaving shorttxt_exit_read\n");
}


/* Destroy write data structure */
void shorttxt_exit_write(struct shorttxt_file_data *out_file)
{
	debug_message("Entering shorttxt_exit_write\n");

	/* Free memory */
	if (out_file->filename)
		free(out_file->filename);

	debug_message("Leaving shorttxt_exit_write\n");
}


/* Set read command line option */
int
shorttxt_set_read_option(struct shorttxt_file_data *in_file, char opt,
			 char *opt_arg)
{
	int rc = FALSE;

	debug_message("Entering shorttxt_set_read_option\n");

	switch (opt) {
	  case 'f':
		  /* Filename */
		  if (opt_arg != NULL && *opt_arg != '\0') {
			  if (in_file->filename)
				  free(in_file->filename);
			  in_file->filename = strdup(opt_arg);
			  rc = TRUE;
		  }
		  break;
	  default:
		  fprintf(stderr,
			  "Can not process read option <%c> for input file\n",
			  opt);
	}

	debug_message("Leaving shorttxt_set_read_option\n");

	return rc;
}


/* Set write command line option */
int
shorttxt_set_write_option(struct shorttxt_file_data *out_file, char opt,
			  char *opt_arg)
{
	int rc = FALSE;

	debug_message("Entering shorttxt_set_write_option\n");

	switch (opt) {
	  case 'f':
		  /* Filename */
		  if (opt_arg != NULL && *opt_arg != '\0') {
			  if (out_file->filename)
				  free(out_file->filename);
			  out_file->filename = strdup(opt_arg);
			  rc = TRUE;
		  }
		  break;
	  default:
		  fprintf(stderr,
			  "Can not process write option <%c> for output file\n",
			  opt);
	}

	debug_message("Leaving shorttxt_set_write_option\n");

	return rc;
}



/* For opening & closing */

/* Open input data file for reading */
void
shorttxt_open_read(struct shorttxt_file_data *in_file,
		   struct header_data *header)
{
	debug_message("Entering shorttxt_open_read\n");

	/* Open file */
	/* Read from stdin if file name not given */
	if (!in_file->filename) {
		/* No open is needed when writing to stdout */
		in_file->file = stdin;
	} else {
		in_file->file = fopen(in_file->filename, "r");
		if (!in_file->file)
			error_message
			    ("Can not open file <%s> for reading\n",
			     in_file->filename);
		in_file->file_is_open = TRUE;
	}

	/* Init */
	in_file->line_buffer 	= NULL;
	in_file->line_no 	= 0;
	in_file->num_recs 	= 0;
	in_file->next_rec 	= 0;

	/* No header to read for this file format */
	header->isValid = FALSE;

	debug_message("Leaving shorttxt_open_read\n");
}


/* Open output data file for writing */
void
shorttxt_open_write(struct shorttxt_file_data *out_file,
		    struct header_data *header)
{
	debug_message("Entering shorttxt_open_write\n");

	/* Open file */
	/* Write to stdout if file name not given or "-" */
	if (!out_file->filename) {
		/* No open is needed when writing to stdout */
		out_file->file = stdout;
	} else {
		out_file->file = fopen(out_file->filename, "w");
		if (!out_file->file)
			error_message("Can not open %s for writing\n",
				      out_file->filename);
		out_file->file_is_open = TRUE;
	}

	/* Init */
	out_file->line_buffer 	= NULL;
	out_file->line_no 	= 0;
	out_file->num_recs 	= 0;
	out_file->next_rec 	= 0;

	/* No header to write for this file format */

	debug_message("Leaving shorttxt_open_write\n");
}


/* Close input file at end of processing */
void
shorttxt_close_read(struct shorttxt_file_data *in_file,
		    struct header_data *header)
{
	debug_message("Entering shorttxt_close_read\n");

	/* Close file */
	if (in_file->file_is_open == TRUE) {
		if (fclose(in_file->file))
			warn_message("Can not close input file\n");
		in_file->file_is_open = FALSE;
	}

	/* Update statistics */
	in_file->lines_read = in_file->line_no;

	/* clear data structures */
	in_file->file 		= NULL;
	in_file->line_buffer 	= NULL;
	in_file->line_no 	= 0;
	in_file->num_recs 	= 0;
	in_file->next_rec 	= 0;

	debug_message("Leaving shorttxt_close_read\n");
}


/* Close output file at end of processing */
void
shorttxt_close_write(struct shorttxt_file_data *out_file,
		     struct header_data *header)
{
	debug_message("Entering shorttxt_close_write\n");

	/* No footer */

	/* Close file */
	if (out_file->file_is_open == TRUE) {
		if (fclose(out_file->file))
			warn_message("Can not close output file\n");
		out_file->file_is_open = FALSE;
	}

	/* Update statistics */
	out_file->lines_written = out_file->line_no;

	/* Clear data structures */
	out_file->file 		= NULL;
	out_file->line_buffer 	= NULL;
	out_file->line_no 	= 0;
	out_file->num_recs 	= 0;
	out_file->next_rec 	= 0;

	debug_message("Leaving shorttxt_close_write\n");
}


/* Close input file in case of an error */
void shorttxt_abort_read(struct shorttxt_file_data *in_file)
{
	debug_message("Entering shorttxt_abort_read\n");

	/* Close file */
	if (in_file->file_is_open == TRUE) {
		if (fclose(in_file->file))
			warn_message("Can not close input file\n");
		in_file->file_is_open = FALSE;
	}

	/* No special error processing needed */

	/* Update statistics */
	in_file->lines_read = in_file->line_no;

	/* Clear data structures */
	in_file->file 		= NULL;
	in_file->line_buffer 	= NULL;
	in_file->line_no 	= 0;
	in_file->num_recs 	= 0;
	in_file->next_rec 	= 0;

	debug_message("Leaving shorttxt_abort_read\n");
}


/* Close output file in case of an error */
void shorttxt_abort_write(struct shorttxt_file_data *out_file)
{
	debug_message("Entering shorttxt_abort_write\n");

	/* Close file */
	if (out_file->file_is_open == TRUE) {
		if (fclose(out_file->file))
			warn_message("Can not close output file\n");
		out_file->file_is_open = FALSE;
	}

	/* Remove incompletely written file */
	if (out_file->filename) {
		info_message
		    ("Removing incompletely written output file <%s>\n",
		     out_file->filename);
		if (out_file->filename)
			unlink(out_file->filename);
	}

	/* Update statistics */
	out_file->lines_written = out_file->line_no;

	/* Clear data structures */
	out_file->file 		= NULL;
	out_file->line_buffer 	= NULL;
	out_file->line_no 	= 0;
	out_file->num_recs 	= 0;
	out_file->next_rec 	= 0;

	debug_message("Leaving shorttxt_abort_write\n");
}



/* For reading */

/* Identify end of file */
int shorttxt_read_eof(struct shorttxt_file_data *in_file)
{
	debug_message("Entering & Leaving shorttxt_read_eof\n");

	return feof(in_file->file);
}


/* Read appointment in current record */
void
shorttxt_read_row(struct shorttxt_file_data *in_file, struct row_data *row)
{
	char buffer[0xffff];
	char buffer1[0xffff];

	struct Appointment a;

	int 	attributes	= 0,
		category	= 0,
		record_num	= 0;

	unsigned long uid 	= 0;

	char *cPtr;

	/* ANSI C forbids variable-size array fields, this #define below will fix
	   it. Since it's declared as type 'int' on line 41 here, we should define
	   it

	   #define SHORTTXT_ROW_MAX_INPUT_PARAM 4
	 */

#define SHORTTXT_ROW_MAX_INPUT_PARAM 4

	char *fields[SHORTTXT_ROW_MAX_INPUT_PARAM];
	int fieldno;

	debug_message("Entering shorttxt_read_row\n");

	/* Init data structures */
	memset(&a, 0, sizeof(a));
	memset(&fields, 0, sizeof(fields));

	setRowIsValid(row, FALSE);
	do {
		/* Skip empty lines */
		shorttxt_read_line(buffer, sizeof(buffer), in_file);
		while (buffer[0] == '\n') {
			shorttxt_read_line(buffer, sizeof(buffer),
					   in_file);
		}
		if (feof(in_file->file))
			error_message
			    ("Read unexpected end of input file at line %d\n\n",
			     in_file->line_no);

		/* Identify all parameters */
		cPtr = buffer;
		fieldno = 0;
		fields[fieldno++] = cPtr;
		while (*cPtr != '\0') {
			/* Separate on tabulator */
			if (*cPtr == '\t') {
				if (fieldno >=
				    SHORTTXT_ROW_MAX_INPUT_PARAM) {
					if (fieldno ==
					    SHORTTXT_ROW_MAX_INPUT_PARAM)
						warn_message
						    ("Too many fields in input file at line %d\n\n",
						     in_file->line_no);
					fieldno++;
					cPtr++;
					continue;
				}
				/* replace tab with terminator */
				*cPtr++ = '\0';
				fields[fieldno++] = cPtr;
			}

			/* Stop at end of line */
			else if (*cPtr == '\n') {
				/* replace cr with terminator */
				*cPtr++ = '\0';
				break;
			}

			/* Continue for normal characters */
			else {
				cPtr++;
			}
		}		/* while */
		/* Now the four input strings should be stored in fields[] */
		if (fieldno != SHORTTXT_ROW_MAX_INPUT_PARAM) {
			warn_message
			    ("Not enough fields in input file at line %d\n\n",
			     in_file->line_no);
			fieldno = 0;
			fields[fieldno++] = cPtr;
			break;
		}

		/* Now process fields */
		fieldno = 0;
		if (fields[0][0] == '\0' || fields[1][0] == '\0') {
			/* no start time or no end time => UNTIMED_EVENT */
			a.event = 1;
		} else {
			/* start and end time => APPOINTMENT */
			a.event = 0;
		}

		/* First field: begin time & date */
		if (fields[0][0] != '\0') {
			time_t t;

			t = read_iso_time_str1(fields[0]);
			if (t == -1) {
				warn_message
				    ("Invalid start date or time <%s> in input file at line %d\n\n",
				     fields[0], in_file->line_no);
				break;
			}
			a.begin = *localtime(&t);
			/* Clear time fields for UNTIMED event */
			if (a.event == 1) {
				a.begin.tm_hour = 0;
				a.begin.tm_min 	= 0;
				a.begin.tm_sec 	= 0;
				a.end 		= a.begin;
			}
		}

		/* Second field: end time & date */
		if (fields[1][0] != '\0') {
			time_t t;

			t = read_iso_time_str1(fields[1]);
			if (t == -1) {
				warn_message
				    ("Invalid end date or time <%s> in input file at line %d\n\n",
				     fields[1], in_file->line_no);
				break;
			}
			a.end = *localtime(&t);
			/* Clear time fields for UNTIMED event */
			if (a.event == 1) {
				a.end.tm_hour 	= 0;
				a.end.tm_min 	= 0;
				a.end.tm_sec 	= 0;
				a.begin 	= a.end;
			}
		}

		/* Third field: advance alarm settings */
		if (fields[2][0] != '\0') {
			a.alarm = 1;
			a.advance = atoi(fields[2]);
			if (strchr(fields[2], 'm'))
				a.advanceUnits = advMinutes;
			else if (strchr(fields[2], 'h'))
				a.advanceUnits = advHours;
			else if (strchr(fields[2], 'd'))
				a.advanceUnits = advDays;
			else {
				warn_message
				    ("Did not understand alarm advance <%s> in input file at line %d\n\n",
				     fields[2], in_file->line_no);
				break;
			}
		} else {
			a.alarm = 0;
			a.advance = 0;
			a.advanceUnits = 0;
		}

		/* No Repeat events are supported */
		a.repeatType 		= repeatNone;
		a.repeatForever 	= 0;
		a.repeatEnd.tm_mday 	= 0;
		a.repeatEnd.tm_mon 	= 0;
		a.repeatEnd.tm_wday 	= 0;
		a.repeatFrequency 	= 0;
		a.repeatWeekstart 	= 0;
		a.exceptions 		= 0;
		a.exception 		= NULL;

		/* Fourth field: description */
		/* Un-quote description text to allow for dangerous characters */
		text_unquote(fields[3], buffer1, sizeof(buffer1));
		a.description = strdup(buffer1);

		/* No note */
		a.note = NULL;


		/* Set record number */
		record_num = in_file->next_rec;


		/* Set datebook row data */
		setRowRecordNum(row, record_num);
		setRowUid(row, uid);
		setRowAttributes(row, attributes);
		setRowCategory(row, category);
		setRowAppointment(row, a);
		setRowIsValid(row, TRUE);

		/* Increment counters */
		in_file->num_recs++;
		in_file->next_rec++;

		/* Update statistics */
		in_file->records_read++;

		/* Allow for exit on error */
	} while (FALSE);

	/* Reading ahead is suboptimal, since this involves a malloc overhead
	 * for pushing back the last read line.
	 * However, it is a reliable way to detect the end of file properly.
	 */

	/* Skip empty lines */
	shorttxt_read_line(buffer, sizeof(buffer), in_file);
	while (buffer[0] == '\n') {
		shorttxt_read_line(buffer, sizeof(buffer), in_file);
	}

	/* Put last line back, to be read on next read attempt */
	shorttxt_pushback_line(buffer, in_file);

	debug_message("Leaving shorttxt_read_row\n");
}



/* For writing */

/* Write an appointment record */
void
shorttxt_write_row(struct shorttxt_file_data *out_file,
		   struct header_data *header, struct row_data *row)
{
	struct AppointmentAppInfo aai;
	struct Appointment a;

	int 	attributes,
		category,
		record_num;

	unsigned long uid;

	char time1_buffer[50];
	char time2_buffer[50];
	char buffer[0xffff];
	char alarmparam[80];

	debug_message("Entering shorttxt_write_row\n");

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

	/* Format begin time&date */
	write_human_full_time_str(mktime(&a.begin), time1_buffer,
				  sizeof(time1_buffer));

	/* Format end time&date */
	write_human_full_time_str(mktime(&a.end), time2_buffer,
				  sizeof(time2_buffer));

	/* Format alarm settings */
	if (a.advance) {
		/* alarm is how much before */
		snprintf(alarmparam, sizeof(alarmparam),
			 SHORTTXT_ROW_ALARM_FORMAT,
			 a.advance,
			 (a.advanceUnits == advMinutes) ? "m" :
			 (a.advanceUnits == advHours) ? "h" :
			 (a.advanceUnits == advDays) ? "d" : "");
	} else {
		/* no alarm */
		alarmparam[0] = '\0';
	}

	/* Format description */
	/* Quote description text to allow for dangerous characters */
	text_quote(a.description, buffer, sizeof(buffer));

	/* Write row according to event type */
	if (a.event == 1) {
		/* UNTIMED_EVENT */
		/* No end time = signal for UNTIMED_EVENT */
		shorttxt_write(out_file, SHORTTXT_ROW_FORMAT,
			       time1_buffer, "", alarmparam, buffer);
	} else {
		/* APPOINTMENT */
		shorttxt_write(out_file, SHORTTXT_ROW_FORMAT,
			       time1_buffer,
			       time2_buffer, alarmparam, buffer);
	}


	/* Increase counters */
	out_file->num_recs++;
	out_file->next_rec++;

	/* Update statistics */
	out_file->records_written++;

	debug_message("Leaving shorttxt_write_row\n");
}



/* For statistics */

/* Show input statistics */
void shorttxt_show_read_statistics(struct shorttxt_file_data *in_file)
{
	debug_message("Entering shorttxt_show_read_statistics\n");

	info_message("Input file <%s>, format <%s>:\n",
		     (in_file->filename) ? in_file->filename : "stdin",
		     dataformat2txt(DATA_FORMAT_SHORTTXT));
	info_message("Lines read: %d\n", in_file->lines_read);
	info_message("Records read: %d\n", in_file->records_read);

	debug_message("Leaving shorttxt_show_read_statistics\n");
}


/* Show output statistics */
void shorttxt_show_write_statistics(struct shorttxt_file_data *out_file)
{
	debug_message("Entering shorttxt_show_write_statistics\n");

	info_message("Output file <%s>, format <%s>:\n",
		     (out_file->filename) ? out_file->filename : "stdout",
		     dataformat2txt(DATA_FORMAT_SHORTTXT));
	info_message("Lines written: %d\n", out_file->lines_written);
	info_message("Records written: %d\n", out_file->records_written);

	debug_message("Leaving shorttxt_show_write_statistics\n");
}



/* Private functions */

/* For IO */

/* Read one line from input file */
void
shorttxt_read_line(char *buffer, int buffer_size,
		   struct shorttxt_file_data *in_file)
{
	/* Check for unexpected end of file */
	if (feof(in_file->file))
		error_message("Unexpected end of input file, line %d\n\n",
			      in_file->line_no);

	/* Read one line */
	if (in_file->line_buffer != NULL) {
		/* Use pushback buffer, if filled */
		strncpy(buffer, in_file->line_buffer, buffer_size);
		free(in_file->line_buffer);
		in_file->line_buffer = NULL;
		in_file->line_no++;
	} else {
		/* Return empty string if fgets fails */
		buffer[0] = '\0';

		/* Read from file */
		fgets(buffer, buffer_size, in_file->file);
		if (!feof(in_file->file)) {
			in_file->line_no++;

			/* Check whether entire line could be read */
			if (buffer[strlen(buffer) - 1] != '\n')
				error_message
				    ("Line <%d> in input file <%s> exceeds buffer size\n",
				     in_file->line_no,
				     (in_file->filename ==
				      NULL) ? "stdin" : in_file->filename);
		}
	}
}


/* Push pack one line into input file */
void
shorttxt_pushback_line(char *buffer, struct shorttxt_file_data *in_file)
{
	/* Pushback line buffer to allow for later re-reading */
	in_file->line_buffer = strdup(buffer);
	in_file->line_no--;
}


/* Write one single string to output file
 
  Especially useful for printing multi-line text, to properly count line
  numbers
 +/
void
shorttxt_write_str(struct shorttxt_file_data *out_file,
		   const char *out_string)
{
	char *newline_pos;


	/* Write text */
	fprintf(out_file->file, "%s", out_string);

	/* Count printed lines */
	newline_pos = data_index(out_string, '\n');
	while (newline_pos != NULL) {
		out_file->line_no++;
		newline_pos = data_index(newline_pos + 1, '\n');
	}
}


/* Write to output file */
void
shorttxt_write(struct shorttxt_file_data *out_file, const char *format,
	       ...)
{
	va_list printf_args;
	char *newline_pos;

	/* Get variable arguments */
	va_start(printf_args, format);

	/* Write text */
	vfprintf(out_file->file, format, printf_args);

	/* No more processing of variable arguments */
	va_end(printf_args);

	/* Count printed lines
	 * (only accurate if newlines can be found in format parameter) */
	newline_pos = data_index(format, '\n');
	while (newline_pos != NULL) {
		out_file->line_no++;
		newline_pos = data_index(newline_pos + 1, '\n');
	}
}
