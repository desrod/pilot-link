/*
 * Pilot Datebook processing utility
 * Matthias Hessler <pilot-datebook@mhessler.de> December 2000
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */
          

#ifndef _PILOT_DATEBOOK_LONGTXT
#define _PILOT_DATEBOOK_LONGTXT


#include "pilot-datebook-data.h"



/* data structures */

/* file data */
struct longtxt_file_data {
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
int longtxt_init_read (struct longtxt_file_data * in_file);
int longtxt_init_write (struct longtxt_file_data * in_file);
void longtxt_exit_read (struct longtxt_file_data * in_file);
void longtxt_exit_write (struct longtxt_file_data * out_file);
int longtxt_set_read_option (struct longtxt_file_data * in_file, char opt, char * opt_arg);
int longtxt_set_write_option (struct longtxt_file_data * out_file, char opt, char * opt_arg);

/* For opening & closing */
void longtxt_open_read (struct longtxt_file_data * in_file, struct header_data * header);
void longtxt_open_write (struct longtxt_file_data * out_file, struct header_data * header);
void longtxt_close_read (struct longtxt_file_data * in_file, struct header_data * header);
void longtxt_close_write (struct longtxt_file_data * out_file, struct header_data * header);
void longtxt_abort_read (struct longtxt_file_data * in_file);
void longtxt_abort_write (struct longtxt_file_data * out_file);

/* For reading */
int longtxt_read_eof (struct longtxt_file_data * in_file);
void longtxt_read_row (struct longtxt_file_data * in_file, struct row_data * row);

/* For writing */
void longtxt_write_row (struct longtxt_file_data * out_file, struct header_data * header, struct row_data * row);

/* For statistics */
void longtxt_show_read_statistics (struct longtxt_file_data * in_file);
void longtxt_show_write_statistics (struct longtxt_file_data * out_file);


/* Private */
void longtxt_read_header (struct longtxt_file_data * in_file, struct header_data * header);
void longtxt_write_header (struct longtxt_file_data * out_file, struct header_data * header);

/* For IO */
void longtxt_read_line (char * buffer, int buffer_size, struct longtxt_file_data * in_file);
void longtxt_pushback_line (char * buffer, struct longtxt_file_data * in_file);
void longtxt_write_str (struct longtxt_file_data * out_file, const char * out_string);
void longtxt_write (struct longtxt_file_data * out_file, const char * format, ...);


#endif
