/*
 * Pilot Datebook processing utility
 * Matthias Hessler <pilot-datebook@mhessler.de> December 2000
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */
          

#ifndef _PILOT_DATEBOOK_SHORTTXT
#define _PILOT_DATEBOOK_SHORTTXT


#include "pilot-datebook-data.h"



/* data structures */

/* file data */
struct shorttxt_file_data {
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
int shorttxt_init_read (struct shorttxt_file_data * in_file);
int shorttxt_init_write (struct shorttxt_file_data * in_file);
void shorttxt_exit_read (struct shorttxt_file_data * in_file);
void shorttxt_exit_write (struct shorttxt_file_data * out_file);
int shorttxt_set_read_option (struct shorttxt_file_data * in_file, char opt, char * opt_arg);
int shorttxt_set_write_option (struct shorttxt_file_data * out_file, char opt, char * opt_arg);

/* For opening & closing */
void shorttxt_open_read (struct shorttxt_file_data * in_file, struct header_data * header);
void shorttxt_open_write (struct shorttxt_file_data * out_file, struct header_data * header);
void shorttxt_close_read (struct shorttxt_file_data * in_file, struct header_data * header);
void shorttxt_close_write (struct shorttxt_file_data * out_file, struct header_data * header);
void shorttxt_abort_read (struct shorttxt_file_data * in_file);
void shorttxt_abort_write (struct shorttxt_file_data * out_file);

/* For reading */
int shorttxt_read_eof (struct shorttxt_file_data * in_file);
void shorttxt_read_row (struct shorttxt_file_data * in_file, struct row_data * row);

/* For writing */
void shorttxt_write_row (struct shorttxt_file_data * out_file, struct header_data * header, struct row_data * row);

/* For statistics */
void shorttxt_show_read_statistics (struct shorttxt_file_data * in_file);
void shorttxt_show_write_statistics (struct shorttxt_file_data * out_file);


/* Private */
/* For IO */
void shorttxt_read_line (char * buffer, int buffer_size, struct shorttxt_file_data * in_file);
void shorttxt_pushback_line (char * buffer, struct shorttxt_file_data * in_file);
void shorttxt_write_str (struct shorttxt_file_data * out_file, const char * out_string);
void shorttxt_write (struct shorttxt_file_data * out_file, const char * format, ...);


#endif
