/*
 * Pilot Datebook processing utility
 * Matthias Hessler <pilot-datebook@mhessler.de> December 2000
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */
          

#ifndef _PILOT_DATEBOOK_WINDAT
#define _PILOT_DATEBOOK_WINDAT


#include "pilot-datebook-data.h"



/* Data structures */

/* File data */
struct windat_file_data {
  char * filename;

  FILE * file;

  int file_is_open;
  char * line_buffer;
  int line_no;
  int num_recs; /* number of records in file */
  int next_rec; /* record number of next record (record number starts with 0)
		   (before reading first record: next_rec = 0) */

  /* Statistics */
  int fields_read;
  int fields_written;
  int records_read;
  int records_written;
  int records_stored;

  /* File location on desktop PC (is part of header data) */
  char * desktopFileLocation;

  /* Repeat event flag index (needed for writing repeat events) */
  int repeatEventFlagCount;

  int dailyRepeatEventFlag;
  int weeklyRepeatEventFlag;
  int monthlyByDayRepeatEventFlag;
  int monthlyByDateRepeatEventFlag;
  int yearlyRepeatEventFlag;

  /* Windat row data (need to know number of records before writing) */
  struct windat_row_data * top;
  struct windat_row_data * last;
};


/* Windat row structure */
struct windat_row_data {
  struct windat_row_data * next;
  struct row_data row;
};




/* Function definitions */

/* Public */

/* For init */
int windat_init_read (struct windat_file_data * in_file);
int windat_init_write (struct windat_file_data * in_file);
void windat_exit_read (struct windat_file_data * in_file);
void windat_exit_write (struct windat_file_data * out_file);
int windat_set_read_option (struct windat_file_data * in_file, char opt, char * opt_arg);
int windat_set_write_option (struct windat_file_data * out_file, char opt, char * opt_arg);

/* For opening & closing */
void windat_open_read (struct windat_file_data * in_file, struct header_data * header);
void windat_open_write (struct windat_file_data * out_file, struct header_data * header);
void windat_close_read (struct windat_file_data * in_file, struct header_data * header);
void windat_close_write (struct windat_file_data * out_file, struct header_data * header);
void windat_abort_read (struct windat_file_data * in_file);
void windat_abort_write (struct windat_file_data * out_file);

/* For reading */
int windat_read_eof (struct windat_file_data * in_file);
void windat_read_row (struct windat_file_data * in_file, struct row_data * row);

/* For writing */
void windat_write_row (struct windat_file_data * out_file, struct header_data * header, struct row_data * row);
void windat_store_row (struct windat_file_data * out_file, struct header_data * header, struct row_data * row);

/* For statistics */
void windat_show_read_statistics (struct windat_file_data * in_file);
void windat_show_write_statistics (struct windat_file_data * out_file);


/* Private */
void windat_read_header (struct windat_file_data * in_file, struct header_data * header);
void windat_write_header (struct windat_file_data * out_file, struct header_data * header);

/* For IO */
void windat_write_str (struct windat_file_data * out_file, const char * out_string);
void windat_write (struct windat_file_data * out_file, const char * format, ...);

/* For windat file format */

/* High level read/write functions */
unsigned long windat_read_field_integer(struct windat_file_data * in_file);
unsigned long windat_read_field_date(struct windat_file_data * in_file);
void windat_read_field_string(struct windat_file_data * in_file, char * buffer, int buffer_size);
unsigned long windat_read_field_boolean(struct windat_file_data * in_file);
void windat_read_repeatEventData(struct windat_file_data * in_file, struct row_data * row);

void windat_write_field_integer(struct windat_file_data * in_file, unsigned long l);
void windat_write_field_date(struct windat_file_data * in_file, unsigned long l);
void windat_write_field_string(struct windat_file_data * in_file, char * buffer);
void windat_write_field_boolean(struct windat_file_data * in_file, unsigned long l);
void windat_write_repeatEventData(struct windat_file_data * out_file, struct row_data * row);
void windat_write_classname(struct windat_file_data * out_file, const char * classname);


/* Low level read/write functions */
void windat_read_cstring(struct windat_file_data * in_file, char * buffer, unsigned int buffer_size);
unsigned long windat_read_long(struct windat_file_data * in_file);
unsigned int windat_read_short(struct windat_file_data * in_file);
unsigned int windat_read_byte(struct windat_file_data * in_file);

void windat_write_cstring(struct windat_file_data * out_file, char * buffer);
void windat_write_long(struct windat_file_data * out_file, unsigned long u);
void windat_write_short(struct windat_file_data * out_file, unsigned int i);
void windat_write_byte(struct windat_file_data * out_file, unsigned int i);


/* Helper for locally storing data records */
void windatlist_add(struct windat_file_data * windatlist, struct windat_row_data * add_windat_row);
void windatlist_del(struct windat_file_data * windatlist, struct windat_row_data * del_windat_row);
struct windat_row_data * windatlist_get_first(struct windat_file_data * windatlist);
struct windat_row_data * windatlist_get_next(struct windat_file_data * windatlist, struct windat_row_data * cur_windat_row);
struct windat_row_data * windat_new_row (void);
void windat_free_row (struct windat_row_data * windat_row);


#endif
