/*
 * Pilot Datebook processing utility
 * Matthias Hessler <pilot-datebook@mhessler.de> December 2000
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */
          

#ifndef _PILOT_DATEBOOK_CSV
#define _PILOT_DATEBOOK_CSV


#include "pilot-datebook-data.h"



/* Data structures */

/* Csv field structure */
struct csv_field_data {
  struct csv_field_data * next;
  struct field_data field;

  /* Statistics */
  unsigned long num_read;
  unsigned long num_written;
};


/* File data */
struct csv_file_data {
  char * filename;

  FILE * file;

  int file_is_open;
  char * line_buffer;
  int line_no;
  int num_recs; /* number of records in file */
  int next_rec; /* record number of next record (record number starts with 0)
		   (before reading first record: next_rec = 0) */

  /* Input/output format */
  char * valuenames;

  /* Header to write instead of format list */
  char * header;

  /* Csv order fields */
  struct csv_field_data * field_top;
  struct csv_field_data * field_last;

  /* Statistics */
  int lines_read;
  int lines_written;
  int records_read;
  int records_written;
};




/* Function definitions */

/* Public */

/* For init */
int csv_init_read (struct csv_file_data * in_file);
int csv_init_write (struct csv_file_data * in_file);
void csv_exit_read (struct csv_file_data * in_file);
void csv_exit_write (struct csv_file_data * out_file);
int csv_set_read_option (struct csv_file_data * in_file, char opt, char * opt_arg);
int csv_set_write_option (struct csv_file_data * out_file, char opt, char * opt_arg);

/* For opening & closing */
void csv_open_read (struct csv_file_data * in_file, struct header_data * header);
void csv_open_write (struct csv_file_data * out_file, struct header_data * header);
void csv_close_read (struct csv_file_data * in_file, struct header_data * header);
void csv_close_write (struct csv_file_data * out_file, struct header_data * header);
void csv_abort_read (struct csv_file_data * in_file);
void csv_abort_write (struct csv_file_data * out_file);

/* For reading */
int csv_read_eof (struct csv_file_data * in_file);
void csv_read_row (struct csv_file_data * in_file, struct row_data * row);

/* For writing */
void csv_write_row (struct csv_file_data * out_file, struct header_data * header, struct row_data * row);

/* For statistics */
void csv_show_read_statistics (struct csv_file_data * in_file);
void csv_show_write_statistics (struct csv_file_data * out_file);


/* Private */
int csv_verify_header (const char * buffer, const char * header);
void csv_read_header (struct csv_file_data * in_file, struct header_data * header);
void csv_write_header (struct csv_file_data * out_file, struct header_data * header);

/* For IO */
void csv_read_line (char * buffer, int buffer_size, struct csv_file_data * in_file);
void csv_pushback_line (char * buffer, struct csv_file_data * in_file);
void csv_write_str (struct csv_file_data * out_file, const char * out_string);
void csv_write (struct csv_file_data * out_file, const char * format, ...);

/* For field handling */
void csv_parse_valuenames (struct csv_file_data * file, char * valuenames);

/* Helper */
void csvlist_field_add(struct csv_file_data * file, struct csv_field_data * add_csv_field);
void csvlist_field_del(struct csv_file_data * file, struct csv_field_data * del_csv_field);
struct csv_field_data * csvlist_get_first_field(struct csv_file_data * file);
struct csv_field_data * csvlist_get_next_field(struct csv_file_data * file, struct csv_field_data * cur_csv_field);
struct csv_field_data * csv_new_field (void);
void csv_free_field (struct csv_field_data * csv_field);

void csv_row_write(struct csv_file_data * file, struct csv_field_data * csv_field, struct row_data * row);
void csv_row_read(struct csv_file_data * file, struct csv_field_data * csv_field, struct row_data * row, char * in_start, char * in_end);


#endif
