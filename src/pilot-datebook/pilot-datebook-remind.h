/*
 * Pilot Datebook processing utility
 * Matthias Hessler <pilot-datebook@mhessler.de> December 2000
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */
          

#ifndef _PILOT_DATEBOOK_REMIND
#define _PILOT_DATEBOOK_REMIND


#include "pilot-datebook-data.h"



/* data structures */

/* file data */
struct remind_file_data {
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
int remind_init_write (struct remind_file_data * in_file);
void remind_exit_write (struct remind_file_data * out_file);
int remind_set_write_option (struct remind_file_data * out_file, char opt, char * opt_arg);

/* For opening & closing */
void remind_open_write (struct remind_file_data * out_file, struct header_data * header);
void remind_close_write (struct remind_file_data * out_file, struct header_data * header);
void remind_abort_write (struct remind_file_data * out_file);

/* For writing */
void remind_write_row (struct remind_file_data * out_file, struct header_data * header, struct row_data * row);

/* For statistics */
void remind_show_write_statistics (struct remind_file_data * out_file);


/* Private */
void remind_write_header (struct remind_file_data * out_file, struct header_data * header);

/* For IO */
void remind_write_str (struct remind_file_data * out_file, const char * out_string);
void remind_write (struct remind_file_data * out_file, const char * format, ...);


#endif
