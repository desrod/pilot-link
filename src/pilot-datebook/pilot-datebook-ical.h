/*
 * Pilot Datebook processing utility
 * Matthias Hessler <pilot-datebook@mhessler.de> December 2000
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */
          

#ifndef _PILOT_DATEBOOK_ICAL
#define _PILOT_DATEBOOK_ICAL


#include "pilot-datebook-data.h"



/* data structures */

/* file data */
struct ical_file_data {
  char * filename;

  FILE * file;

  int file_is_open;
  char * line_buffer;
  int line_no;
  int num_recs; /* number of records in file */
  int next_rec; /* record number of next record (record number starts with 0)
		   (before reading first record: next_rec = 0) */

  /* Statistics */
  int lines_read;
  int lines_written;
  int records_read;
  int records_written;
};




/* Function definitions */

/* Public */

/* For init */
int ical_init_write (struct ical_file_data * in_file);
void ical_exit_write (struct ical_file_data * out_file);
int ical_set_write_option (struct ical_file_data * out_file, char opt, char * opt_arg);

/* For opening & closing */
void ical_open_write (struct ical_file_data * out_file, struct header_data * header);
void ical_close_write (struct ical_file_data * out_file, struct header_data * header);
void ical_abort_write (struct ical_file_data * out_file);

/* For writing */
void ical_write_row (struct ical_file_data * out_file, struct header_data * header, struct row_data * row);

/* For statistics */
void ical_show_write_statistics (struct ical_file_data * out_file);


/* Private */
void ical_write_header (struct ical_file_data * out_file, struct header_data * header);

/* For IO */
void ical_write_str (struct ical_file_data * out_file, const char * out_string);
void ical_write (struct ical_file_data * out_file, const char * format, ...);
char * tclquote(char * in);


#endif
