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
#include "pilot-datebook-windat.h"

/* windat is the file format of the Windows desktop application distributed
   with the Palm Pilot. Palm Computing has reserved the right to change this
   format without notice in the future.
 
   Analysis on the file format has been done for Palm Desktop 2.0 and 3.0. 
   The results have been almost identical, with the following exceptions:

	- position field: unused for desktop 3.0, used in desktop 2.0
	  (potentially only related to datebk3)

	- custom header: "100 16" for desktop 3.0, "100 3" for desktop 2.0.

	- desktop file location: is always dependent on the user & the PC
	  installation directory

   The written desktop file will follow the desktop 3.0 format.

   Since the desktop file location is not stored in the Palm pilot, or pdb,
   it can not be recovered.

   Since the desktop file does not know creation, modification, and backup
   dates, these will be set to the current time upon reading.

   Since the desktop file does not contain the modification number, it will
   be set to 1 upon reading.
 */


/* Constants */
const int WINDAT_FIELD_SCHEMA[] = {
	1,			/* Record ID       */
	1,			/* Status Field    */
	1,			/* Position        */
	3,			/* Start Time      */
	1,			/* End Time        */
	5,			/* Description     */
	1,			/* Duration        */
	5,			/* Note            */
	6,			/* Untimed         */
	6,			/* Private         */
	1,			/* Category        */
	6,			/* Alarm Set       */
	1,			/* Alarm Advance Units     */
	1,			/* Alarm Advance Type      */
	8			/* Repeat Event    */
};
const int WINDAT_FIELD_NUM =
    sizeof(WINDAT_FIELD_SCHEMA) / sizeof(WINDAT_FIELD_SCHEMA[0]);

/* Default desktop file location (is part of header data), assuming 'new_user'
 * as user name.
 */
const char WINDAT_DEFAULT_FILE_LOCATION[] =
    "c:\\Palm\\new_user\\datebook\\datebook.dat";


/* Can not use makelong, since it chokes/stops on '\0', yielding
 * incorrect results (it uses strlen to determine string length)
 */
unsigned long longtxt_expected_version =
    ('D' << 24) + ('B' << 16) + ('\1' << 8) + ('\0' << 0);

/* Class names:
 * (written only on the first written event per brand)
 *   CDayName = daily (brand 1)
 *   CWeekly = weekly (brand 2)
 *   CDayOfMonth = day of month (brand 3)
 *   CDateOfMonth = date of month (brand 4)
 *   CDateOfYear = yearly by date (brand 5)
 */
const char WINDAT_CLASSNAME_DAILY[] 		= "CDayName";
const char WINDAT_CLASSNAME_WEEKLY[] 		= "CWeekly";
const char WINDAT_CLASSNAME_MONTHLYBYDAY[] 	= "CDayOfMonth";
const char WINDAT_CLASSNAME_MONTHLYBYDATE[] 	= "CDateOfMonth";
const char WINDAT_CLASSNAME_YEARLY[] 		= "CDateOfYear";



/* Public functions */

/* For init */

/* Initialize read data structure */
int windat_init_read(struct windat_file_data *in_file)
{
	debug_message("Entering windat_init_read\n");

	/* Init own data structure */
	in_file->filename 			= NULL;
	in_file->file 				= NULL;
	in_file->file_is_open 			= FALSE;
	in_file->line_buffer 			= NULL;
	in_file->line_no 			= 0;
	in_file->num_recs 			= 0;
	in_file->next_rec 			= 0;

	in_file->fields_read 			= 0;
	in_file->fields_written 		= 0;
	in_file->records_read 			= 0;
	in_file->records_written 		= 0;

	in_file->desktopFileLocation 		= NULL;

	in_file->repeatEventFlagCount 		= 0;
	in_file->dailyRepeatEventFlag 		= 0;
	in_file->weeklyRepeatEventFlag 		= 0;
	in_file->monthlyByDayRepeatEventFlag 	= 0;
	in_file->monthlyByDateRepeatEventFlag 	= 0;
	in_file->yearlyRepeatEventFlag 		= 0;

	in_file->top 				= NULL;
	in_file->last 				= NULL;

	debug_message("Leaving windat_init_read\n");

	return TRUE;
}


/* Initialize write data structure */
int windat_init_write(struct windat_file_data *out_file)
{
	debug_message("Entering windat_init_write\n");

	/* Init own data structure */
	out_file->filename 			= NULL;
	out_file->file 				= NULL;
	out_file->file_is_open 			= FALSE;
	out_file->line_buffer 			= NULL;
	out_file->line_no 			= 0;
	out_file->num_recs 			= 0;
	out_file->next_rec 			= 0;

	out_file->fields_read 			= 0;
	out_file->fields_written 		= 0;
	out_file->records_read 			= 0;
	out_file->records_written 		= 0;

	out_file->desktopFileLocation =
	    strdup(WINDAT_DEFAULT_FILE_LOCATION);

	out_file->repeatEventFlagCount 		= 0;
	out_file->dailyRepeatEventFlag 		= 0;
	out_file->weeklyRepeatEventFlag 	= 0;
	out_file->monthlyByDayRepeatEventFlag 	= 0;
	out_file->monthlyByDateRepeatEventFlag 	= 0;
	out_file->yearlyRepeatEventFlag 	= 0;

	out_file->top 				= NULL;
	out_file->last 				= NULL;

	debug_message("Leaving windat_init_write\n");

	return TRUE;
}


/* Destroy read data structure */
void windat_exit_read(struct windat_file_data *in_file)
{
	struct windat_row_data *windat_row;

	debug_message("Entering windat_exit_read\n");

	/* Free memory */
	if (in_file->filename)
		free(in_file->filename);
	if (in_file->desktopFileLocation)
		free(in_file->desktopFileLocation);

	/* Destroy all windat rows */
	windat_row = windatlist_get_first(in_file);
	while (windat_row != NULL) {
		/* Remove windat row */
		windatlist_del(in_file, windat_row);

		/* Destroy windat row data structure */
		windat_free_row(windat_row);

		/* Next = now first (after removal of previous first) */
		windat_row = windatlist_get_first(in_file);
	}			/* while */

	/* Init own data structure */
	in_file->filename 			= NULL;
	in_file->file 				= NULL;
	in_file->file_is_open 			= FALSE;
	in_file->line_buffer 			= NULL;
	in_file->line_no 			= 0;
	in_file->num_recs 			= 0;
	in_file->next_rec 			= 0;

	in_file->fields_read 			= 0;
	in_file->fields_written 		= 0;
	in_file->records_read 			= 0;
	in_file->records_written 		= 0;

	in_file->desktopFileLocation 		= NULL;

	in_file->repeatEventFlagCount 		= 0;
	in_file->dailyRepeatEventFlag 		= 0;
	in_file->weeklyRepeatEventFlag 		= 0;
	in_file->monthlyByDayRepeatEventFlag 	= 0;
	in_file->monthlyByDateRepeatEventFlag 	= 0;
	in_file->yearlyRepeatEventFlag 		= 0;

	in_file->top 				= NULL;
	in_file->last 				= NULL;

	debug_message("Leaving windat_exit_read\n");
}


/* Destroy write data structure */
void windat_exit_write(struct windat_file_data *out_file)
{
	struct windat_row_data *windat_row;

	debug_message("Entering windat_exit_write\n");

	/* Free memory */
	if (out_file->filename)
		free(out_file->filename);
	if (out_file->desktopFileLocation)
		free(out_file->desktopFileLocation);

	/* Destroy all windat rows */
	windat_row = windatlist_get_first(out_file);
	while (windat_row != NULL) {
		/* Remove windat row */
		windatlist_del(out_file, windat_row);

		/* Destroy windat row data structure */
		windat_free_row(windat_row);

		/* Next = now first (after removal of previous first) */
		windat_row = windatlist_get_first(out_file);
	}			/* while */

	/* Init own data structure */
	out_file->filename 			= NULL;
	out_file->file 				= NULL;
	out_file->file_is_open 			= FALSE;
	out_file->line_buffer 			= NULL;
	out_file->line_no 			= 0;
	out_file->num_recs 			= 0;
	out_file->next_rec 			= 0;

	out_file->fields_read 			= 0;
	out_file->fields_written 		= 0;
	out_file->records_read 			= 0;
	out_file->records_written 		= 0;

	out_file->desktopFileLocation 		= NULL;

	out_file->repeatEventFlagCount 		= 0;
	out_file->dailyRepeatEventFlag 		= 0;
	out_file->weeklyRepeatEventFlag 	= 0;
	out_file->monthlyByDayRepeatEventFlag 	= 0;
	out_file->monthlyByDateRepeatEventFlag 	= 0;
	out_file->yearlyRepeatEventFlag 	= 0;

	out_file->top 				= NULL;
	out_file->last 				= NULL;

	debug_message("Leaving windat_exit_write\n");
}


/* Set read command line option */
int
windat_set_read_option(struct windat_file_data *in_file, char opt,
		       char *opt_arg)
{
	int rc = FALSE;

	debug_message("Entering windat_set_read_option\n");

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

	debug_message("Leaving windat_set_read_option\n");

	return rc;
}


/* Set write command line option */
int
windat_set_write_option(struct windat_file_data *out_file, char opt,
			char *opt_arg)
{
	int rc = FALSE;

	debug_message("Entering windat_set_write_option\n");

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

	  case 'l':
		  /* File location on desktop PC (to be written into header) */
		  if (opt_arg != NULL && *opt_arg != '\0') {
			  if (out_file->desktopFileLocation)
				  free(out_file->desktopFileLocation);
			  out_file->desktopFileLocation = strdup(opt_arg);
			  rc = TRUE;
		  }
		  break;

	  default:
		  fprintf(stderr,
			  "Can not process write option <%c> for input file\n",
			  opt);
	}

	debug_message("Leaving windat_set_write_option\n");

	return rc;
}



/* For opening & closing */

/* Open input data file for reading */
void
windat_open_read(struct windat_file_data *in_file,
		 struct header_data *header)
{
	debug_message("Entering windat_open_read\n");

	/* Parse file format */
	/* Parsing file format will be done when reading header */

	/* Open file */
	/* Read from stdin if file name not given */
	if (!in_file->filename) {
		/* No open is needed when writing to stdout */
		in_file->file = stdin;
	} else {
		in_file->file = fopen(in_file->filename, "rb");
		if (!in_file->file)
			error_message
			    ("Can not open file <%s> for reading\n",
			     in_file->filename);
		in_file->file_is_open = TRUE;
	}

	/* Init */
	in_file->line_buffer 			= NULL;
	in_file->line_no 			= 0;
	in_file->num_recs 			= 0;
	in_file->next_rec 			= 0;

	in_file->desktopFileLocation 		= NULL;

	in_file->repeatEventFlagCount 		= 0;
	in_file->dailyRepeatEventFlag 		= 0;
	in_file->weeklyRepeatEventFlag 		= 0;
	in_file->monthlyByDayRepeatEventFlag 	= 0;
	in_file->monthlyByDateRepeatEventFlag 	= 0;
	in_file->yearlyRepeatEventFlag 		= 0;

	in_file->top 				= NULL;
	in_file->last 				= NULL;


	/* Read header data */
	windat_read_header(in_file, header);

	debug_message("Leaving windat_open_read\n");
}


/* Open output data file for writing */
void
windat_open_write(struct windat_file_data *out_file,
		  struct header_data *header)
{
	debug_message("Entering windat_open_write\n");

	/* Open file */
	/* Write to stdout if file name not given or "-" */
	if (!out_file->filename) {
		/* No open is needed when writing to stdout */
		out_file->file = stdout;
	} else {
		out_file->file = fopen(out_file->filename, "wb");
		if (!out_file->file)
			error_message
			    ("Can not open file <%s> for writing\n",
			     out_file->filename);
		out_file->file_is_open = TRUE;
	}

	/* Init */
	out_file->line_buffer 			= NULL;
	out_file->line_no 			= 0;
	out_file->num_recs 			= 0;
	out_file->next_rec 			= 0;

	out_file->repeatEventFlagCount 		= 0;
	out_file->dailyRepeatEventFlag 		= 0;
	out_file->weeklyRepeatEventFlag 	= 0;
	out_file->monthlyByDayRepeatEventFlag 	= 0;
	out_file->monthlyByDateRepeatEventFlag 	= 0;
	out_file->yearlyRepeatEventFlag 	= 0;

	out_file->top 				= NULL;
	out_file->last 				= NULL;


	/* Header can not yet be written, need to know the total number of records */

	debug_message("Leaving windat_open_write\n");
}


/* Close input file at end of processing */
void
windat_close_read(struct windat_file_data *in_file,
		  struct header_data *header)
{
	debug_message("Entering windat_close_read\n");

	/* Close file */
	if (in_file->file_is_open == TRUE) {
		if (fclose(in_file->file))
			warn_message("Can not close input file\n");
		in_file->file_is_open = FALSE;
	}

	/* Update statistics */
	in_file->fields_read = in_file->line_no;

	/* clear data structures */
	in_file->file 				= NULL;
	in_file->line_buffer 			= NULL;
	in_file->line_no 			= 0;
	in_file->num_recs 			= 0;
	in_file->next_rec 			= 0;

	in_file->repeatEventFlagCount 		= 0;
	in_file->dailyRepeatEventFlag 		= 0;
	in_file->weeklyRepeatEventFlag 		= 0;
	in_file->monthlyByDayRepeatEventFlag 	= 0;
	in_file->monthlyByDateRepeatEventFlag 	= 0;
	in_file->yearlyRepeatEventFlag 		= 0;

	debug_message("Leaving windat_close_read\n");
}


/* Close output file at end of processing */
void
windat_close_write(struct windat_file_data *out_file,
		   struct header_data *header)
{
	struct windat_row_data *windat_row;

	debug_message("Entering windat_close_write\n");


	/* Only now we know the total number of records, needed to write header */
	out_file->num_recs = out_file->next_rec;

	/* Write header */
	windat_write_header(out_file, header);

	/* Write all rows */
	windat_row = windatlist_get_first(out_file);
	while (windat_row != NULL) {
		/* Write row into file */
		windat_store_row(out_file, header, &(windat_row->row));

		/* Next row */
		windat_row = windatlist_get_next(out_file, windat_row);
	}			/* while */

	/* Close file */
	if (out_file->file_is_open == TRUE) {
		if (fclose(out_file->file))
			warn_message("Can not close output file\n");
		out_file->file_is_open = FALSE;
	}

	/* Update statistics */
	out_file->fields_written = out_file->line_no;

	/* Clear data structures */
	out_file->file 				= NULL;
	out_file->line_buffer 			= NULL;
	out_file->line_no 			= 0;
	out_file->num_recs 			= 0;
	out_file->next_rec 			= 0;

	out_file->repeatEventFlagCount 		= 0;
	out_file->dailyRepeatEventFlag 		= 0;
	out_file->weeklyRepeatEventFlag 	= 0;
	out_file->monthlyByDayRepeatEventFlag 	= 0;
	out_file->monthlyByDateRepeatEventFlag 	= 0;
	out_file->yearlyRepeatEventFlag 	= 0;

	debug_message("Leaving windat_close_write\n");
}


/* Close input file in case of an error */
void windat_abort_read(struct windat_file_data *in_file)
{
	debug_message("Entering windat_abort_read\n");

	/* Close file */
	if (in_file->file_is_open == TRUE) {
		if (fclose(in_file->file))
			warn_message("Can not close input file\n");
		in_file->file_is_open = FALSE;
	}

	/* No special error processing needed */

	/* Update statistics */
	in_file->fields_read = in_file->line_no;

	/* clear data structures */
	in_file->file 				= NULL;
	in_file->line_buffer 			= NULL;
	in_file->line_no 			= 0;
	in_file->num_recs 			= 0;
	in_file->next_rec 			= 0;

	in_file->repeatEventFlagCount 		= 0;
	in_file->dailyRepeatEventFlag 		= 0;
	in_file->weeklyRepeatEventFlag 		= 0;
	in_file->monthlyByDayRepeatEventFlag 	= 0;
	in_file->monthlyByDateRepeatEventFlag 	= 0;
	in_file->yearlyRepeatEventFlag 		= 0;

	debug_message("Leaving windat_abort_read\n");
}


/* Close output file in case of an error */
void windat_abort_write(struct windat_file_data *out_file)
{
	debug_message("Entering windat_abort_write\n");

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
	out_file->fields_written = out_file->line_no;

	/* Clear data structures */
	out_file->file 				= NULL;
	out_file->line_buffer 			= NULL;
	out_file->line_no 			= 0;
	out_file->num_recs 			= 0;
	out_file->next_rec 			= 0;

	out_file->repeatEventFlagCount 		= 0;
	out_file->dailyRepeatEventFlag 		= 0;
	out_file->weeklyRepeatEventFlag 	= 0;
	out_file->monthlyByDayRepeatEventFlag 	= 0;
	out_file->monthlyByDateRepeatEventFlag 	= 0;
	out_file->yearlyRepeatEventFlag 	= 0;

	debug_message("Leaving windat_abort_write\n");
}



/* For reading */

/* Identify end of file */
int windat_read_eof(struct windat_file_data *in_file)
{
	debug_message("Entering & Leaving windat_read_eof\n");

	/* Return on eof, or when exceeding expected record number */
	/* Have to compare greater or equal, because first record number is 0 */
	return (feof(in_file->file)
		|| in_file->next_rec >= in_file->num_recs);
}


/* Read routine for appointment in current record */
void
windat_read_row(struct windat_file_data *in_file, struct row_data *row)
{
	char buffer[0xffff];
	long l;
	struct tm tm;
	int attributes;

	debug_message("Entering windat_read_row\n");

	/* Safety check */
	if (feof(in_file->file))
		error_message
		    ("Read unexpected end of input file in record %d\n\n",
		     in_file->next_rec);

	/* Init data structure */
	rowInit(row);
	setRowIsValid(row, FALSE);

	do {
		/* Read all data items */

		/* Record ID, Type 1 = Integer */
		l = windat_read_field_integer(in_file);
		setRowUid(row, l);

		/* Status Field, Type 1 = Integer */
		l = windat_read_field_integer(in_file);
		attributes = 0;
		if (l & 0x01)	/* Add => Mapped to dirty, can be recognized on uid=0 */
			attributes |= dlpRecAttrDirty;
		if (l & 0x02)	/* Update */
			attributes |= dlpRecAttrDirty;
		if (l & 0x04)	/* Delete */
			attributes |= dlpRecAttrDeleted;
		if (l & 0x08)	/* Pending */
			attributes |= dlpRecAttrBusy;
		if (l & 0x80)	/* Archive */
			attributes |= dlpRecAttrArchived;
		setRowAttributes(row, attributes);
		/* attributes will be updated later, for private flag */

		/* Position, Type 1 = Integer */
		/* Was always set to INT_MAX for me (0x7fffffff) */
		l = windat_read_field_integer(in_file);

		/* Start Time, Type 3 = Date */
		l = windat_read_field_date(in_file);
		write_iso_full_time_str(l, buffer, sizeof(buffer));
		tm = *localtime(&l);
		setRowBegin(row, tm);

		/* End Time, Type 1 = Integer */
		/* Strange: why is this not also type 3 = date ? */
		l = windat_read_field_integer(in_file);
		write_iso_full_time_str(l, buffer, sizeof(buffer));
		tm = *localtime(&l);
		setRowEnd(row, tm);

		/* Description, Type 5 = CString */
		windat_read_field_string(in_file, buffer, sizeof(buffer));
		setRowDescription(row, buffer);

		/* Duration, Type 1 = Integer */
		/* Was always set to 0 for me */
		l = windat_read_field_integer(in_file);

		/* Note, Type 5 = CString */
		windat_read_field_string(in_file, buffer, sizeof(buffer));
		setRowNote(row, buffer);

		/* Untimed Field, Type 6 = Boolean */
		l = windat_read_field_boolean(in_file);
		setRowUntimed(row, l);

		/* Private Field, Type 6 = Boolean */
		/* Attributes have been written earlier, only update for private flag */
		attributes = getRowAttributes(row);
		l = windat_read_field_boolean(in_file);
		if (l)
			attributes |= dlpRecAttrSecret;
		setRowAttributes(row, attributes);

		/* Category Field, Type 1 = Integer */
		/* Set to 0 for category Unfiled (= default) */
		l = windat_read_field_integer(in_file);
		setRowCategory(row, l);

		/* Alarm Set Field, Type 6 = Boolean */
		l = windat_read_field_boolean(in_file);
		setRowAlarm(row, l);

		/* Alarm Advance Time Units Field, Type 1 = Integer */
		/* No Alarm => set to -1 after synchronized by Palm;
		 * Desktop seems to not initialize properly, but often set to 0 */
		l = windat_read_field_integer(in_file);
		if (l == -1)
			setRowAdvance(row, -1);
		else
			setRowAdvance(row, l);

		/* Alarm Advance Type Field, Type 1 = Integer */
		/* No Alarm => set to 0 after synchronized by Palm;
		 * Desktop seems to not initialize properly, but often set to -1 */
		l = windat_read_field_integer(in_file);
		if (l == -1)
			setRowAdvanceUnit(row, -1);
		else
			setRowAdvanceUnit(row, l);


		/* Repeat Event, Field Type 8 */
		/* Reading a repeat event is fairly complex, therefore directly coded
		 * here now, using the low-level read functions.
		 */
		{		/* start of reading repeat event data */
			long l;
			int exception_num;
			struct tm *exceptions;
			struct tm tm;
			int i;


			/* Read field type */
			l = windat_read_long(in_file);
			if (l != 8)
				error_message
				    ("Expected to read field type <8>, encountered <%lu>\n",
				     l);

			/* Read number of exceptions */
			exception_num = windat_read_short(in_file);

			if (exception_num == 0) {
				/* Store exceptions */
				setRowRepeatExceptionNum(row, 0);
				setRowRepeatException(row, NULL);
			} else {
				/* malloc */
				exceptions =
				    calloc(exception_num,
					   sizeof(struct tm));

				/* Read all exceptions */
				for (i = 0; i < exception_num; i++) {
					/* Read exception dates */
					l = windat_read_long(in_file);
					tm = *localtime(&l);
					write_iso_full_time_str(l, buffer,
								sizeof
								(buffer));
					exceptions[i] = tm;
				}

				/* Store exceptions */
				setRowRepeatExceptionNum(row,
							 exception_num);
				setRowRepeatException(row, exceptions);
				exceptions = NULL;
			}	/* else exception_num > 0 */

			/* Repeat Event Flag */
			l = windat_read_short(in_file);
			if (l == 0x0000) {
				/* No repeat event */
				setRowRepeatType(row, repeatNone);
			} else if ((l & 0xff00) == 0x8000) {

				/* On the first occurance of a repeat event, the class name is
				   written and this repeat event gets assigned a flag in the form
				   of 0x80yy, where yy is in the range of 0x00 to 0xff, depending
				   on which position in the input file the repeat event has been
				   encountered first.

				   The position is calculated such that it starts with 0, then it
				   is increased by 1 for any repeat event (0 for non-repeat event),
				   and another 1 after the first occurance of a repeat event.  (Who
				   knows why this silly logic is used, it does not even ensure
				   unique flags, since it wraps around after 0xff)
				  
				   Example1: file contains daily event followed by yearly event.
				   => daily = 0x8001, yearly = 0x8003
				  
				   Example2: file contains 2 daily events followed by yearly event.
				   => daily = 0x8001, yearly = 0x8004
				  
				   Example3: file contains non-repeat event, then 3 daily events
				   followed by yearly event.
				   => daily = 0x8001, yearly = 0x8005
				 */

				/* Read repeat event data */
				windat_read_repeatEventData(in_file, row);
			} else if (l == 0xffff) {
				/* Class entry follows (on first occurance of a repeat event) */

				/* Class entry constant = 1 (class entry version?) */
				l = windat_read_short(in_file);
				if (l != 1)
					error_message
					    ("Expected class entry constant <1>, encountered <%lu>\n",
					     l);

				/* Length of class name */
				l = windat_read_short(in_file);
				if (l >= sizeof(buffer))
					error_message
					    ("Length class name exceeds buffer size\n");

				/* Read class name */
				for (i = 0; i < l; i++) {
					buffer[i] =
					    windat_read_byte(in_file);
				}
				buffer[l] = '\0';

				/* Class names:
				   written only on the first written event per brand

				     CDayName 	  = daily (brand 1)
				     CWeekly      = weekly (brand 2)
				     CDayOfMonth  = day of month (brand 3)
				     CDateOfMonth = date of month (brand 4)
				     CDateOfYear  = yearly (brand 5)
				 */

				/* Read repeat event data */
				windat_read_repeatEventData(in_file, row);
			} else {
				error_message
				    ("Encountered unknown repeat event flag <%lx>\n",
				     l);
			}

		}		/* end of reading repeat event data */

		/* If we reach here, then we have successfully read a row */
		setRowIsValid(row, TRUE);


		/* Increment counters */
		in_file->next_rec++;

		/* Update statistics */
		in_file->records_read++;
	} while (FALSE);

	debug_message("Leaving windat_read_row\n");
}



/* For writing */

/* Write an appointment record */
void
windat_write_row(struct windat_file_data *out_file,
		 struct header_data *header, struct row_data *row)
{
	struct windat_row_data *windat_row;

	debug_message("Entering windat_write_row\n");

	/* Don't write record, just store it - before actual writing we have
	   to know the total number of records (stored in the header)!
	 */

	/* Alloc windat row data */
	windat_row = windat_new_row();

	/* Insert row data into windat row */
	windat_row->row = *row;
	windat_row->next = NULL;

	/* Insert windat row into windat structure */
	windatlist_add(out_file, windat_row);

	/* Increase counters */
	out_file->next_rec++;

	/* Update statistics */
	out_file->records_written++;

	debug_message("Leaving windat_write_row\n");
}


/* Write an appointment record into a file */
void
windat_store_row(struct windat_file_data *out_file,
		 struct header_data *header, struct row_data *row)
{
	unsigned long l;
	struct tm tm;
	int attributes;

	debug_message("Entering windat_store_row\n");

	if (!getRowIsValid(row))
		error_message("Can not write invalid row.\n");

	/* Write a row */


	/* Record ID, Type 1 = Integer */
	l = getRowUid(row);
	windat_write_field_integer(out_file, l);

	/* Status Field, Type 1 = Integer */
	attributes = getRowAttributes(row);
	l = 0;
	if ((attributes & dlpRecAttrDirty)
	    && getRowUid(row) == 0)
		l |= 0x01;	/* Add */
	if (attributes & dlpRecAttrDirty && getRowUid(row) != 0)
		l |= 0x02;	/* Update */
	if (attributes & dlpRecAttrDeleted)
		l |= 0x04;	/* Delete */
	if (attributes & dlpRecAttrBusy)
		l |= 0x08;	/* Pending */
	if (attributes & dlpRecAttrArchived)
		l |= 0x80;	/* Archive */
	windat_write_field_integer(out_file, l);

	/* Position, Type 1 = Integer */

	/* Was always set to INT_MAX for me (0x7fffffff) for Palm Desktop
	   3.0, seemed to be in use for Palm Desktop 2.0, but could also be
	   related to my usage of datebk3 (now using datebk4).
	 */

	l = 0x7fffffff;
	windat_write_field_integer(out_file, l);

	/* Start Time, Type 3 = Date */
	tm = getRowBegin(row);
	l = mktime(&tm);
	windat_write_field_date(out_file, l);

	/* End Time, Type 1 = Integer */
	/* Strange: why is this not also type 3 = date ? */
	tm = getRowEnd(row);
	l = mktime(&tm);
	windat_write_field_integer(out_file, l);

	/* Description, Type 5 = CString */
	windat_write_field_string(out_file, getRowDescription(row));

	/* Duration, Type 1 = Integer */
	/* Was always set to 0 for me */
	windat_write_field_integer(out_file, 0);

	/* Note, Type 5 = CString */
	windat_write_field_string(out_file, getRowNote(row));

	/* Untimed Field, Type 6 = Boolean */
	windat_write_field_boolean(out_file, getRowUntimed(row));

	/* Private Field, Type 6 = Boolean */
	attributes = getRowAttributes(row);
	l = (attributes & dlpRecAttrSecret);
	windat_write_field_boolean(out_file, l);

	/* Category Field, Type 1 = Integer */
	/* Set to 0 for category Unfiled (= default) */
	l = getRowCategory(row);
	windat_write_field_integer(out_file, l);

	/* Alarm Set Field, Type 6 = Boolean */
	l = getRowAlarm(row);
	windat_write_field_boolean(out_file, l);

	/* Alarm Advance Time Units Field, Type 1 = Integer */
	if (getRowAdvance(row) == -1)
		l = -1;
	else
		l = getRowAdvance(row);
	windat_write_field_integer(out_file, l);

	/* Alarm Advance Type Field, Type 1 = Integer */
	if (getRowAdvanceUnit(row) == -1)
		l = -1;
	else
		l = getRowAdvanceUnit(row);
	windat_write_field_integer(out_file, l);


	/* Repeat Event, Field Type 8 */
	windat_write_repeatEventData(out_file, row);


	/* Update statistics */
	out_file->records_stored++;

	debug_message("Leaving windat_store_row\n");
}



/* For statistics */

/* Show input statistics */
void windat_show_read_statistics(struct windat_file_data *in_file)
{
	debug_message("Entering windat_show_read_statistics\n");

	info_message("Input file <%s>, format <%s>:\n",
		     (in_file->filename) ? in_file->filename : "stdin",
		     dataformat2txt(DATA_FORMAT_WINDAT));
	if (in_file->desktopFileLocation)
		info_message("Desktop file location read: <%s>\n",
			     in_file->desktopFileLocation);
	info_message("Fields read: %d\n", in_file->fields_read);
	info_message("Records read: %d\n", in_file->records_read);

	debug_message("Leaving windat_show_read_statistics\n");
}


/* Show output statistics */
void windat_show_write_statistics(struct windat_file_data *out_file)
{
	debug_message("Entering windat_show_write_statistics\n");

	info_message("Output file <%s>, format <%s>:\n",
		     (out_file->filename) ? out_file->filename : "stdout",
		     dataformat2txt(DATA_FORMAT_WINDAT));
	if (out_file->desktopFileLocation)
		info_message("Desktop file location written: <%s>\n",
			     out_file->desktopFileLocation);
	info_message("Fields written: %d\n", out_file->fields_written);
	info_message("Records received for writing: %d\n",
		     out_file->records_written);
	info_message("Records written to file: %d\n",
		     out_file->records_stored);

	debug_message("Leaving windat_show_write_statistics\n");
}



/* Private functions */

/* Read file header */
void
windat_read_header(struct windat_file_data *in_file,
		   struct header_data *header)
{
	unsigned long l;
	unsigned int s;

	char buffer[0xffff];

	int 	categoryNum,
		i,
		categoryIndex,
		lastUniqueID;

	debug_message("Entering windat_read_header\n");

	/* Init data structures, no header to read for this data format */
	memset(header, 0, sizeof(*header));
	header->isValid = FALSE;

	/* Read general database header data */
	do {

		/* Get version */
		header->info.version = windat_read_long(in_file);
		if (header->info.version != longtxt_expected_version)
			error_message
			    ("Expected version <%lu>, got <%lu>\n",
			     longtxt_expected_version,
			     header->info.version);

		/* File name of desktop file */
		windat_read_cstring(in_file, buffer, sizeof(buffer));
		in_file->desktopFileLocation = strdup(buffer);

		/* Custom show header */
		/* Always "100 16" for desktop 3.0 (but "100 3" for desktop 2.0?) */
		windat_read_cstring(in_file, buffer, sizeof(buffer));

		/* Next free category id for desktop */
		/* Set to 128 for desktop (should be set to highest category number, if
		 * one is greater than 128)
		 */
		/* Derive last unique id for Palm from highest present category number
		 * which is less than 128, but set to 0 since it seems to be not used.
		 */
		l = windat_read_long(in_file);
		lastUniqueID = 0;

		/* Category count (without Unfiled category) */
		categoryNum = windat_read_long(in_file);

		/* Only set up categories, if category entries are present */
		if (categoryNum > 0) {
			/* Set up "Unfiled" category */
			header->aai.category.renamed[0] = 0;
			strncpy(header->aai.category.name[0],
				"Unfiled",
				sizeof(header->aai.category.name) - 1);
			header->aai.category.
			    name[0][sizeof(header->aai.category.name) -
				    1] = '\0';
			header->aai.category.ID[0] = 0;

			/* Categories */
			/* Relying on the fact that not-used categories have already been
			 * initialized by memset, since categoryIndex will jump over unused
			 * category entries
			 */
			categoryIndex = 0;
			for (i = 1; i <= categoryNum; i++) {
				/* Category index */
				l = windat_read_long(in_file);
				if (l <= categoryIndex)
					warn_message
					    ("Expected category entry greater than <%d>, encountered <%lu>\n",
					     categoryIndex, l);
				if (l >= DATEBOOK_MAX_CATEGORIES)
					error_message
					    ("Category index <%lu> exceeds maximum category number <%d>\n",
					     l,
					     DATEBOOK_MAX_CATEGORIES - 1);
				categoryIndex = l;

				/* ID */
				header->aai.category.ID[categoryIndex] =
				    windat_read_long(in_file);

				/* Search for last unique ID */
				if (header->aai.category.
				    ID[categoryIndex] > lastUniqueID)
					lastUniqueID =
					    header->aai.category.
					    ID[categoryIndex];

				/* Dirty Flag */
				header->aai.category.
				    renamed[categoryIndex] =
				    windat_read_long(in_file);

				/* Long Name */
				windat_read_cstring(in_file, buffer,
						    sizeof(buffer));
				strncpy(header->aai.category.
					name[categoryIndex], buffer,
					sizeof(header->aai.category.
					       name[0]) - 1);
				header->aai.category.
				    name[categoryIndex][sizeof
							(header->aai.
							 category.
							 name[0]) - 1] =
				    '\0';

				/* Short Name (truncated after 8 characters?) */

				/* Since the pdb format does not have support for short name vs. 
				   long name, the Pilot also displays longer names, and datebook
				   category are not supported by the desktop application anyway, a
				   basic solution is used here: only long name is taken, and on
				   writing windat, the long name is truncated to 8 characters.
				 */
				windat_read_cstring(in_file, buffer,
						    sizeof(buffer));
			}	/* for */
		}



		/* if categoryNum > 0 */
		/* Set last unique ID */
		/* Derive last unique id for Palm from highest present category number
		   * which is less than 128
		 */
		/* For me, lastUniqueID was always 0 */
		header->aai.category.lastUniqueID = 0;
		/* header->aai.category.lastUniqueID = lastUniqueID; */

		/* Resource ID */
		l = windat_read_long(in_file);
		if (l != 54)
			error_message
			    ("Expected Resource ID <54>, encountered <%lu>\n",
			     l);

		/* Fields per Row */
		l = windat_read_long(in_file);
		if (l != WINDAT_FIELD_NUM)
			error_message
			    ("Expected <%d> fields per row, encountered <%lu>\n",
			     WINDAT_FIELD_NUM, l);

		/* Record ID Position */
		l = windat_read_long(in_file);
		if (l != 0)
			error_message
			    ("Expected to have Record ID in field <0>, encountered <%lu>\n",
			     l);

		/* Record Status Position */
		l = windat_read_long(in_file);
		if (l != 1)
			error_message
			    ("Expected to have Record Status in field <1>, encountered <%lu>\n",
			     l);

		/* Placement Position */
		l = windat_read_long(in_file);
		if (l != 2)
			error_message
			    ("Expected to have Placement Position in field <2>, encountered <%lu>\n",
			     l);

		/* Schema Field Count */
		s = windat_read_short(in_file);
		if (s != WINDAT_FIELD_NUM)
			error_message
			    ("Expected to have <%d> fields in schema, encountered <%lu>\n",
			     WINDAT_FIELD_NUM, s);

		/* Field Entries */
		for (i = 0; i < WINDAT_FIELD_NUM; i++) {
			/* Check field type against expected field type */
			s = windat_read_short(in_file);
			if (s != WINDAT_FIELD_SCHEMA[i])
				error_message
				    ("Expected field type <%d> for field# <%d>, encountered <%lu>\n",
				     WINDAT_FIELD_SCHEMA[i], i, s);
		}		/* for */

		/* Num Entries */
		l = windat_read_long(in_file);
		in_file->num_recs = l / WINDAT_FIELD_NUM;
		in_file->next_rec = 0;

		/* Set missing DBInfo data to default values */
		header->info.more = 0;
		/* AppInfo and data are both dirty and require backup */
		header->info.flags =
		    dlpDBFlagAppInfoDirty | dlpDBFlagBackup;
		header->info.miscFlags = 0;
		header->info.type = makelong("DATA");
		header->info.creator = makelong("date");
		header->info.version = 0;
		/* This is at least the first modification */
		header->info.modnum = 1;
		/* Creation, modification, and backup set to NOW */
		header->info.createDate = time(NULL);
		header->info.modifyDate = time(NULL);
		header->info.backupDate = time(NULL);
		header->info.index = 0;
		strcpy(header->info.name, "DatebookDB");

		/* Set size of application information block */
		header->app_info_size = sizeof(header->aai);

		/* No sort block */
		header->sort_info = NULL;
		header->sort_info_size = 0;

		/* If we reach here then we successfully read the header */
		header->isValid = TRUE;

	} while (FALSE);

	debug_message("Leaving windat_read_header\n");
}


/* Write datebook file header */
void
windat_write_header(struct windat_file_data *out_file,
		    struct header_data *header)
{
	int lastUniqueID;
	int categoryNum;
	int categoryIndex;
	char short_name[8 + 1];
	int i;

	debug_message("Entering windat_write_header\n");

	/* Version of desktop file whose format we will write */
	windat_write_long(out_file, longtxt_expected_version);

	/* File name of desktop file */
	windat_write_cstring(out_file, out_file->desktopFileLocation);

	/* Custom show header */
	/* Always "100 16" for desktop 3.0 ("100 3" for desktop2.0?) */
	windat_write_cstring(out_file, "100 16");

	/* Analyse categories */
	lastUniqueID = 128;
	categoryNum = 0;
	for (i = 0; i < DATEBOOK_MAX_CATEGORIES; i++) {
		/* Search for last unique ID */
		if (header->aai.category.ID[i] > lastUniqueID)
			lastUniqueID = header->aai.category.ID[i];

		/* Count number of used categories */
		if (header->aai.category.ID[i] > 0)
			categoryNum++;
	}			/* categoryNum */

	/* Next free category id for desktop */
	/* Set to 128 for desktop (should be set to highest category number, if
	 * one is greater than 128)
	 */
	windat_write_long(out_file, lastUniqueID);

	/* Category count (without Unfiled category) */
	windat_write_long(out_file, categoryNum);

	/* Only write categories, if category entries are present */
	if (categoryNum > 0) {
		/* Skip "Unfiled" category */

		/* Categories */
		categoryIndex = 0;
		for (i = 0; i < DATEBOOK_MAX_CATEGORIES; i++) {
			/* Skip unused categories */
			if (header->aai.category.ID[i] != 0) {

				/* Category index */
				categoryIndex++;
				windat_write_long(out_file, i);

				/* ID */
				windat_write_long(out_file,
						  header->aai.category.
						  ID[i]);

				/* Dirty Flag */
				windat_write_long(out_file,
						  header->aai.category.
						  renamed[i]);

				/* Long Name */
				windat_write_cstring(out_file,
						     header->aai.category.
						     name[i]);

				/* Short Name (truncated after 8 characters - is this correct?) */
				strncpy(short_name,
					header->aai.category.name[i], 8);
				short_name[8] = '\0';
				windat_write_cstring(out_file, short_name);
			}	/* if non-empty category */
		}		/* for */
	}

	/* if categoryNum > 0 */
	/* Resource ID */
	windat_write_long(out_file, 54);

	/* Fields per Row */
	windat_write_long(out_file, WINDAT_FIELD_NUM);

	/* Record ID Position (0 => is in first field) */
	windat_write_long(out_file, 0);

	/* Record Status Position (1 => is in second field) */
	windat_write_long(out_file, 1);

	/* Placement Position (2 => is in third field) */
	windat_write_long(out_file, 2);

	/* Schema Field Count */
	windat_write_short(out_file, WINDAT_FIELD_NUM);

	/* Field Entries */
	for (i = 0; i < WINDAT_FIELD_NUM; i++) {
		/* Write field type */
		windat_write_short(out_file, WINDAT_FIELD_SCHEMA[i]);
	}			/* for */

	/* Num Entries */
	/* This entry forces us to store all record numbers beforehand,
	 * to know the number of records when writing this header.
	 */
	windat_write_long(out_file, out_file->num_recs * WINDAT_FIELD_NUM);

	debug_message("Leaving windat_write_header\n");
}



/* For IO */

/* Write one single string to output file
 *
 * Especially useful for printing multi-line text, to properly count
 * line numbers
 */
void
windat_write_str(struct windat_file_data *out_file, const char *out_string)
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
windat_write(struct windat_file_data *out_file, const char *format, ...)
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


/* Helper */

/* For windat file format */

/* High level read/write functions */

/* Read integer field */
unsigned long windat_read_field_integer(struct windat_file_data *in_file)
{
	const unsigned long expected_type = 1;
	unsigned long l;


	/* Read field type */
	l = windat_read_long(in_file);
	if (l != expected_type)
		error_message
		    ("Expected to read field type <%d>, encountered <%ld>\n",
		     expected_type, l);

	/* Read value */
	l = windat_read_long(in_file);

	return l;
}


/* Read date field */
unsigned long windat_read_field_date(struct windat_file_data *in_file)
{
	const int expected_type = 3;
	unsigned long l;


	/* Read field type */
	l = windat_read_long(in_file);
	if (l != expected_type)
		error_message
		    ("Expected to read field type <%d>, encountered <%lu>\n",
		     expected_type, l);

	/* Read value */
	l = windat_read_long(in_file);

	return l;
}


/* Read string field */
void
windat_read_field_string(struct windat_file_data *in_file, char *buffer,
			 int buffer_size)
{
	const int expected_type = 5;
	unsigned long l;


	/* Read field type */
	l = windat_read_long(in_file);
	if (l != expected_type)
		error_message
		    ("Expected to read field type <%d>, encountered <%lu>\n",
		     expected_type, l);

	/* Read field padding */
	l = windat_read_long(in_file);
	if (l != 0)
		error_message
		    ("Expected to read padding <0>, encountered <%lu>\n",
		     l);

	/* Read value */
	windat_read_cstring(in_file, buffer, buffer_size);
}


/* Read boolean field */
unsigned long windat_read_field_boolean(struct windat_file_data *in_file)
{
	const int expected_type = 6;
	unsigned long l;


	/* Read field type */
	l = windat_read_long(in_file);
	if (l != expected_type)
		error_message
		    ("Expected to read field type <%d>, encountered <%lu>\n",
		     expected_type, l);

	/* Read value */
	l = windat_read_long(in_file);

	return l;
}


/* Read repeat event data */
void
windat_read_repeatEventData(struct windat_file_data *in_file,
			    struct row_data *row)
{
	char buffer[0xffff];
	unsigned long brand;
	long l;
	struct tm tm;
	int weekstart;


	/* Brand/type of repeat event */
	brand = windat_read_long(in_file);
	if (brand < repeatDaily || brand > repeatYearly)
		error_message("Encountered invalid brand <%lu>\n", brand);
	setRowRepeatType(row, brand);

	/* Interval/frequency of repeat event */
	l = windat_read_long(in_file);
	setRowRepeatFrequency(row, l);

	/* End Date */
	l = windat_read_long(in_file);
	if (l >= 1956520799) {	/* 0x749e235f = 2031-12-31 22:59:59 (?) */
		/* No end date */
		write_iso_full_time_str(l, buffer, sizeof(buffer));
		l = 0;
		tm = *localtime(&l);
		setRowRepeatForever(row, 1);
		setRowRepeatEnd(row, tm);
	} else {
		/* End date encountered */
		tm = *localtime(&l);
		write_iso_full_time_str(mktime(&tm), buffer,
					sizeof(buffer));
		setRowRepeatForever(row, 0);
		setRowRepeatEnd(row, tm);
	}

	/* First Day of Week */
	/* Only set for weekly events (for me to 1 = Monday), otherwise 0 */
	weekstart = windat_read_long(in_file);
	if (weekstart < 0 || weekstart > 6)
		error_message
		    ("Encountered invalid first day of week <%d>\n",
		     weekstart);
	setRowRepeatWeekstart(row, weekstart);

	/* Init (safety net) */
	setRowRepeatWeekdays(row, 0);
	setRowRepeatDay(row, 0);

	/* Brand data */
	switch (brand) {
	  case repeatDaily:
		  /* Day Index */
		  tm = getRowBegin(row);
		  l = windat_read_long(in_file);
		  if (l != tm.tm_wday)
			  error_message
			      ("Expected daily weekday <%d>, encountered day <%lu>\n",
			       tm.tm_wday, l);
		  /* On the specified day, nothing to set */
		  break;

	  case repeatWeekly:
		  /* Day Index */
		  tm = getRowBegin(row);
		  l = windat_read_long(in_file);
		  if (l != weekstart)
			  error_message
			      ("Expected weekstart day <%d>, encountered day <%lu>\n",
			       weekstart, l);

		  /* Days Mask */
		  l = windat_read_byte(in_file);

		  /* Set weekday repeat mask */
		  setRowRepeatWeekdays(row, l);
		  break;

	  case repeatMonthlyByDay:
	  {
		  int weekday;
		  int weeknum;

		  /* Weekday Index */
		  /* 0 = Sun, 1 = Mon, ... 6 = Sat */
		  weekday = windat_read_long(in_file);

		  /* Week Index */
		  /* 0-3 means week number, 4 means last week */
		  weeknum = windat_read_long(in_file);

		  /* Calculate & set repeat day */
		  setRowRepeatDay(row, weeknum * 7 + weekday);
	  }
		  break;

	  case repeatMonthlyByDate:
		  /* Day Index */
		  tm = getRowBegin(row);
		  l = windat_read_long(in_file);
		  if (l != tm.tm_mday)
			  error_message
			      ("Expected monthlybyday day <%d>, encountered day <%lu>\n",
			       tm.tm_mday, l);
		  /* On the specified day, nothing to set */
		  break;

	  case repeatYearly:
		  /* Day Number */
		  tm = getRowBegin(row);
		  l = windat_read_long(in_file);
		  if (l != tm.tm_mday)
			  error_message
			      ("Expected yearly day <%d>, encountered day <%lu>\n",
			       tm.tm_mday, l);

		  /* Month Index */
		  l = windat_read_long(in_file);
		  if (l != tm.tm_mon)
			  error_message
			      ("Expected yearly month <%d>, encountered month <%lu>\n",
			       tm.tm_mon, l);
		  /* On the specified day, nothing to set */
		  break;

	  default:
		  error_message("Encountered invalid brand <%lu>\n",
				brand);
	}
}



/* Write integer field */
void
windat_write_field_integer(struct windat_file_data *in_file,
			   unsigned long l)
{

	/* Write field type */
	windat_write_long(in_file, 1);

	/* Write value */
	windat_write_long(in_file, l);
}


/* Write date field */
void
windat_write_field_date(struct windat_file_data *in_file, unsigned long l)
{

	/* Write field type */
	windat_write_long(in_file, 3);

	/* Write value */
	windat_write_long(in_file, l);
}


/* Write string field */
void
windat_write_field_string(struct windat_file_data *in_file, char *buffer)
{

	/* Write field type */
	windat_write_long(in_file, 5);

	/* Write field padding */
	windat_write_long(in_file, 0);

	/* Write value */
	windat_write_cstring(in_file, buffer);
}


/* Write boolean field */
void
windat_write_field_boolean(struct windat_file_data *in_file,
			   unsigned long l)
{

	/* Write field type */
	windat_write_long(in_file, 6);

	/* Write value */
	windat_write_long(in_file, l);
}


/* Write repeat event data */
void
windat_write_repeatEventData(struct windat_file_data *out_file,
			     struct row_data *row)
{
	unsigned long brand;
	int exception_num;
	struct tm *exceptions;
	unsigned long l;
	unsigned int i;
	struct tm tm;


	/* Get repeat type */
	brand = getRowRepeatType(row);

	/* Field type for repeat event */
	windat_write_long(out_file, 8);

	/* Repeat event? */
	if (brand == repeatNone) {
		/* No repeat event */

		/* Number of exceptions */
		exception_num = 0;
		windat_write_short(out_file, exception_num);

		/* Repeat Event Flag */
		windat_write_short(out_file, 0x0000);
	} else {
		/* Repeat event */

		/* Number of exceptions */
		exception_num = getRowRepeatExceptionNum(row);
		windat_write_short(out_file, exception_num);

		/* Write all exceptions */
		if (exception_num > 0) {
			exceptions = getRowRepeatException(row);
			for (i = 0; i < exception_num; i++) {
				/* Write exception date */
				l = mktime(&(exceptions[i]));
				windat_write_long(out_file, l);
			}	/* for */
		}



		/* if exception_num > 0 */
		/* Repeat Event Flag */
		/* On the first occurance of a repeat event, the class name is
		   * written (with the flag 0xffff), and this repeat event gets
		   * assigned a flag in the form of 0x80yy, where yy is in the range
		   * of 0x00 to 0xff, depending on which position in the input file
		   * the repeat event has been encountered first.
		   * The position is calculated such that it starts with 0, then
		   * it is increased by 1 for any repeat event (0 for non-repeat
		   * event), and another 1 after the first occurance of a repeat event.
		   * (Who knows why this silly logic is used, it does not even ensure
		   * unique flags, since it wraps around after 0xff)
		   *
		   * Example1: file contains daily event followed by yearly event.
		   * => daily = 0x8001, yearly = 0x8003
		   *
		   * Example2: file contains 2 daily events followed by yearly event.
		   * => daily = 0x8001, yearly = 0x8004
		   *
		   * Example3: file contains non-repeat event, then 3 daily events
		   * followed by yearly event.
		   * => daily = 0x8001, yearly = 0x8005
		 */
		/* Update event position/index for every repeat event */
		out_file->repeatEventFlagCount++;
		switch (brand) {
		  case repeatDaily:
			  if (out_file->dailyRepeatEventFlag == 0) {
				  /* First occurance */

				  /* Determine daily repeat event flag */
				  out_file->dailyRepeatEventFlag =
				      0x8000 | (out_file->
						repeatEventFlagCount &
						0xff);

				  /* Update event position/index for a newly encountered event */
				  out_file->repeatEventFlagCount++;

				  /* Repeat Event Flag */
				  windat_write_short(out_file, 0xffff);

				  /* Write class name */
				  windat_write_classname(out_file,
							 WINDAT_CLASSNAME_DAILY);
			  } else {
				  /* Not first occurance */

				  /* Repeat Event Flag */
				  windat_write_short(out_file,
						     out_file->
						     dailyRepeatEventFlag);
			  }
			  break;

		  case repeatWeekly:
			  if (out_file->weeklyRepeatEventFlag == 0) {
				  /* First occurance */

				  /* Determine weekly repeat event flag */
				  out_file->weeklyRepeatEventFlag =
				      0x8000 | (out_file->
						repeatEventFlagCount &
						0xff);

				  /* Update event position/index for a newly encountered event */
				  out_file->repeatEventFlagCount++;

				  /* Repeat Event Flag */
				  windat_write_short(out_file, 0xffff);

				  /* Write class name */
				  windat_write_classname(out_file,
							 WINDAT_CLASSNAME_WEEKLY);
			  } else {
				  /* Not first occurance */

				  /* Repeat Event Flag */
				  windat_write_short(out_file,
						     out_file->
						     weeklyRepeatEventFlag);
			  }
			  break;

		  case repeatMonthlyByDay:
			  if (out_file->monthlyByDayRepeatEventFlag == 0) {
				  /* First occurance */

				  /* Determine monthlyByDay repeat event flag */
				  out_file->monthlyByDayRepeatEventFlag =
				      0x8000 | (out_file->
						repeatEventFlagCount &
						0xff);

				  /* Update event position/index for a newly encountered event */
				  out_file->repeatEventFlagCount++;

				  /* Repeat Event Flag */
				  windat_write_short(out_file, 0xffff);

				  /* Write class name */
				  windat_write_classname(out_file,
							 WINDAT_CLASSNAME_MONTHLYBYDAY);
			  } else {
				  /* Not first occurance */

				  /* Repeat Event Flag */
				  windat_write_short(out_file,
						     out_file->
						     monthlyByDayRepeatEventFlag);
			  }
			  break;

		  case repeatMonthlyByDate:
			  if (out_file->monthlyByDateRepeatEventFlag == 0) {
				  /* First occurance */

				  /* Determine monthlyByDate repeat event flag */
				  out_file->monthlyByDateRepeatEventFlag =
				      0x8000 | (out_file->
						repeatEventFlagCount &
						0xff);

				  /* Update event position/index for a newly encountered event */
				  out_file->repeatEventFlagCount++;

				  /* Repeat Event Flag */
				  windat_write_short(out_file, 0xffff);

				  /* Write class name */
				  windat_write_classname(out_file,
							 WINDAT_CLASSNAME_MONTHLYBYDATE);
			  } else {
				  /* Not first occurance */

				  /* Repeat Event Flag */
				  windat_write_short(out_file,
						     out_file->
						     monthlyByDateRepeatEventFlag);
			  }
			  break;

		  case repeatYearly:
			  if (out_file->yearlyRepeatEventFlag == 0) {
				  /* First occurance */

				  /* Determine yearly repeat event flag */
				  out_file->yearlyRepeatEventFlag =
				      0x8000 | (out_file->
						repeatEventFlagCount &
						0xff);

				  /* Update event position/index for a newly encountered event */
				  out_file->repeatEventFlagCount++;

				  /* Repeat Event Flag */
				  windat_write_short(out_file, 0xffff);

				  /* Write class name */
				  windat_write_classname(out_file,
							 WINDAT_CLASSNAME_YEARLY);
			  } else {
				  /* Not first occurance */

				  /* Repeat Event Flag */
				  windat_write_short(out_file,
						     out_file->
						     yearlyRepeatEventFlag);
			  }
			  break;

		  default:
			  error_message
			      ("Encountered unknown brand type <%d>\n",
			       brand);
		}

		/* Brand/type of repeat event */
		windat_write_long(out_file, brand);

		/* Interval/frequency of repeat event */
		l = getRowRepeatFrequency(row);
		windat_write_long(out_file, l);

		/* Repeat end Date */
		if (getRowRepeatForever(row)) {
			/* Repeat forever */
			/* 1956520799 = 0x749e235f = 2031-12-31 22:59:59 (?) */
			windat_write_long(out_file, 1956520799);
		} else {
			/* Repeat until repeat end date */
			tm = getRowRepeatEnd(row);
			l = mktime(&tm);
			windat_write_long(out_file, l);
		}

		/* First Day of Week */
		/* Only set for weekly events (for me to 1 = Monday), otherwise 0
		 * (for records synchronized with Palm).
		 * Records newly added by desktop 3.x follow different rules, however:
		 * the field is always properly set for repeat events.
		 */
		l = getRowRepeatWeekstart(row);
		windat_write_long(out_file, l);

		/* Brand data */
		switch (brand) {
		  case repeatDaily:
			  /* Day Index */
			  tm = getRowBegin(row);
			  windat_write_long(out_file, tm.tm_wday);
			  break;

		  case repeatWeekly:
			  /* (Weekstart) Day Index */
			  l = getRowRepeatWeekstart(row);
			  windat_write_long(out_file, l);

			  /* (Weekday Repeat) Days Mask */
			  i = getRowRepeatWeekdays(row);
			  windat_write_byte(out_file, i);
			  break;

		  case repeatMonthlyByDay:
		  {
			  int repeatDay;
			  int weekday;
			  int weeknum;

			  /* Get repeat day */
			  repeatDay = getRowRepeatDay(row);

			  /* Weekday Index */
			  /* 0 = Sun, 1 = Mon, ... 6 = Sat */
			  weekday = repeatDay % 7;
			  windat_write_long(out_file, weekday);

			  /* Week Index */
			  /* 0-3 means week number, 4 means last week */
			  weeknum = repeatDay / 7;
			  windat_write_long(out_file, weeknum);
		  }
			  break;

		  case repeatMonthlyByDate:
			  /* Day Index */
			  tm = getRowBegin(row);
			  windat_write_long(out_file, tm.tm_mday);
			  break;

		  case repeatYearly:
			  /* Day Number */
			  tm = getRowBegin(row);
			  windat_write_long(out_file, tm.tm_mday);

			  /* Month Index */
			  windat_write_long(out_file, tm.tm_mon);
			  break;

		  default:
			  error_message
			      ("Encountered invalid brand <%lu>\n", brand);
		}		/* switch brand */
	}			/* else repeat event */
}


/* Write class name */
void
windat_write_classname(struct windat_file_data *out_file,
		       const char *classname)
{
	long len;
	int i;


	/* Class entry constant = 1  (version number?) */
	windat_write_short(out_file, 1);

	/* Length of class name */
	len = strlen(classname);
	if (len > 0xff)
		error_message("Class name too long\n");
	windat_write_short(out_file, len);

	/* Class name */
	for (i = 0; i < len; i++) {
		windat_write_byte(out_file, classname[i]);
	}			/* for */
}


/* Low level read/write functions */

/* Read CString */
void
windat_read_cstring(struct windat_file_data *in_file, char *buffer,
		    unsigned int buffer_size)
{
	unsigned char c;
	unsigned int len;
	unsigned int i;


	if (feof(in_file->file))
		error_message
		    ("Unexpected end of file before reading CString\n");

	/* Get string length */
	c = 0;
	if (in_file->file) {
		c = fgetc(in_file->file);
		if (feof(in_file->file))
			error_message
			    ("Unexpected end of file while reading CString length\n");
	}
	len = c;
	if (len == 0xff) {
		/* Little endian format */
		c = fgetc(in_file->file);
		if (feof(in_file->file))
			error_message
			    ("Unexpected end of file while reading byte 1 of CString length\n");
		len = c;
		c = fgetc(in_file->file);
		if (feof(in_file->file))
			error_message
			    ("Unexpected end of file while reading byte 2 of CString length\n");
		len += (c << 8);
	}

	/* String fits into buffer, including trailing zero? */
	if (len >= buffer_size)
		error_message
		    ("String of length <%u> does not fit into buffer with length <%u>\n",
		     len, buffer_size);

	/* Get string */
	for (i = 0; i < len; i++) {
		buffer[i] = fgetc(in_file->file);
		if (feof(in_file->file))
			error_message
			    ("Unexpected end of file while reading CString\n");
	}
	buffer[len] = '\0';

	/* Safety check */
	if (i != len)
		error_message
		    ("Could only read <%d> bytes for CString, expected <%d>\n",
		     i, len);
}


/* Read Long */
unsigned long windat_read_long(struct windat_file_data *in_file)
{
	unsigned long u;


	/* Get long */
	if (feof(in_file->file))
		error_message
		    ("Unexpected end of file before reading long\n");

	/* Little endian format */
	u = (fgetc(in_file->file));
	if (feof(in_file->file))
		error_message
		    ("Unexpected end of file while reading byte 1 of long\n");

	u += (fgetc(in_file->file) << 8);
	if (feof(in_file->file))
		error_message
		    ("Unexpected end of file while reading byte 2 of long\n");

	u += (fgetc(in_file->file) << 16);
	if (feof(in_file->file))
		error_message
		    ("Unexpected end of file while reading byte 3 of long\n");

	u += (fgetc(in_file->file) << 24);
	if (feof(in_file->file))
		error_message
		    ("Unexpected end of file while reading byte 4 of long\n");

	return u;
}


/* Read short */
unsigned int windat_read_short(struct windat_file_data *in_file)
{
	unsigned int i;


	/* Get short */
	if (feof(in_file->file))
		error_message
		    ("Unexpected end of file before reading short\n");

	/* Little endian format */
	i = fgetc(in_file->file);
	if (feof(in_file->file))
		error_message
		    ("Unexpected end of file while reading byte 1 of short\n");

	i += (fgetc(in_file->file) << 8);
	if (feof(in_file->file))
		error_message
		    ("Unexpected end of file while reading byte 2 of short\n");

	/* Unfortunately, no makeshort function available */

	return i;
}


/* Read byte */
unsigned int windat_read_byte(struct windat_file_data *in_file)
{
	unsigned int i;


	/* Get short */
	if (feof(in_file->file))
		error_message
		    ("Unexpected end of file before reading byte\n");

	i = fgetc(in_file->file);
	if (feof(in_file->file))
		error_message
		    ("Unexpected end of file while reading byte\n");

	return i;
}


/* Write CString */
void windat_write_cstring(struct windat_file_data *out_file, char *buffer)
{
	unsigned int len;


	/* Get string length */
	if (buffer == NULL)
		len = 0;
	else
		len = strlen(buffer);

	/* Write string length */
	if (len < 0xff) {
		/* For small string, write length into first byte */
		fputc(len & 0xff, out_file->file);
		if (ferror(out_file->file))
			error_message
			    ("Unexpected error while writing short CString length\n");
	} else {
		/* For long string, write 0xff, followed by string length in second and
		 * third byte
		 */
		if (len > 0xffff)
			error_message
			    ("String <%s> exceeds maximum length\n",
			     buffer);

		/* Little endian format */
		fputc(0xff, out_file->file);
		if (ferror(out_file->file))
			error_message
			    ("Unexpected error while writing long CString indicator\n");

		fputc(len & 0xff, out_file->file);
		if (ferror(out_file->file))
			error_message
			    ("Unexpected error while writing byte 1 of CString length\n");

		fputc((len >> 8) & 0xff, out_file->file);
		if (ferror(out_file->file))
			error_message
			    ("Unexpected error while writing byte 2 of CString length\n");
	}

	/* Write string */
	if (len > 0) {
		fputs(buffer, out_file->file);
		if (ferror(out_file->file))
			error_message
			    ("Unexpected error while writing string of CString\n");
	}
}


/* Write long */
void windat_write_long(struct windat_file_data *out_file, unsigned long u)
{

	/* Little endian format */
	fputc(u & 0xff, out_file->file);
	if (ferror(out_file->file))
		error_message
		    ("Unexpected error while writing byte 1 of long\n");

	fputc((u >> 8) & 0xff, out_file->file);
	if (ferror(out_file->file))
		error_message
		    ("Unexpected error while writing byte 2 of long\n");

	fputc((u >> 16) & 0xff, out_file->file);
	if (ferror(out_file->file))
		error_message
		    ("Unexpected error while writing byte 3 of long\n");

	fputc((u >> 24) & 0xff, out_file->file);
	if (ferror(out_file->file))
		error_message
		    ("Unexpected error while writing byte 4 of long\n");
}


/* Write short */
void windat_write_short(struct windat_file_data *out_file, unsigned int i)
{

	/* Little endian format */
	fputc(i & 0xff, out_file->file);
	if (ferror(out_file->file))
		error_message
		    ("Unexpected error while writing byte 1 of short\n");

	fputc((i >> 8) & 0xff, out_file->file);
	if (ferror(out_file->file))
		error_message
		    ("Unexpected error while writing byte 2 of short\n");
}


/* Write byte */
void windat_write_byte(struct windat_file_data *out_file, unsigned int i)
{

	/* Little endian format */
	fputc(i & 0xff, out_file->file);
	if (ferror(out_file->file))
		error_message("Unexpected error while writing byte\n");
}




/* Helper functions to locally store windat data records */

/* Append one windat_row */
void
windatlist_add(struct windat_file_data *windatlist,
	       struct windat_row_data *add_windat_row)
{

	/* Add to list */
	if (add_windat_row == NULL) {
		/* No windat_row provided => nothing to add */
	} else if (windatlist == NULL) {
		/* No list data provided => nothing to add */
	}
	/* All parameters present */
	/* Empty list? */
	else if (windatlist->last == NULL || windatlist->top == NULL) {
		/* List is empty */
		add_windat_row->next = NULL;
		windatlist->last = add_windat_row;
		windatlist->top = add_windat_row;
	}
	/* Enter as last entry into list (Keep unsorted order by default) */
	else {
		/* Add after last */
		add_windat_row->next = NULL;
		windatlist->last->next = add_windat_row;
		windatlist->last = add_windat_row;
	}
}


/* Remove one windat_row */
void
windatlist_del(struct windat_file_data *windatlist,
	       struct windat_row_data *del_windat_row)
{

	/* Remove from list */
	if (del_windat_row == NULL) {
		/* No windat_row provided => nothing to remove */
	} else if (windatlist == NULL) {
		/* No list data provided => nothing to remove */
	} else if (windatlist->last == NULL) {
		/* Nothing in list => nothing to remove */
	} else {
		if (windatlist->last == del_windat_row) {
			/* Last in list => iterate through list to find new last */
			if (windatlist->top != del_windat_row)
				debug_message
				    ("Removing last - inefficient use of windatlist_del\n");
			windatlist->last = windatlist->top;
			while (windatlist->last->next != NULL
			       && windatlist->last->next !=
			       del_windat_row) {
				windatlist->last = windatlist->last->next;
			}	/* while */
		}
		if (windatlist->top == del_windat_row) {
			/* First in list => remove */
			windatlist->top = del_windat_row->next;
		}
	}

	/* Remove linkage */
	del_windat_row->next = NULL;
}


/* Get first windat_row */
struct windat_row_data *windatlist_get_first(struct windat_file_data
					     *windatlist)
{
	if (windatlist == NULL)
		return NULL;
	else
		return windatlist->top;
}


/* Get next windat_row */
struct windat_row_data *windatlist_get_next(struct windat_file_data
					    *windatlist,
					    struct windat_row_data
					    *cur_windat_row)
{
	if (cur_windat_row == NULL)
		return NULL;
	else
		return cur_windat_row->next;
}


/* Alloc new windat row data */
struct windat_row_data *windat_new_row(void)
{
	return (struct windat_row_data *)
	    malloc(sizeof(struct windat_row_data));
}


/* De-alloc windat row data */
void windat_free_row(struct windat_row_data *windat_row)
{
	free((void *) windat_row);
}
