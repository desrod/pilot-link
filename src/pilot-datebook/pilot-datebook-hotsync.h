/*
 * Pilot Datebook processing utility
 * Matthias Hessler <pilot-datebook@mhessler.de> December 2000
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */
          

#ifndef _PILOT_DATEBOOK_HOTSYNC
#define _PILOT_DATEBOOK_HOTSYNC

#include "pilot-datebook-data.h"



/* data structures */

/* file data */
struct hotsync_file_data {
  char * filename;

  /* file handle stuff for reading pilotlink data file */
  BOOLEAN file_is_open;
  int socket;
  int database;
  struct PilotUser user;

  int num_recs; /* number of records in file */
  int next_rec; /* record number of next record (record number starts with 0)
		   (before reading first record: next_rec = 0) */

  /* Statistics */
  int records_read;
  int records_written;
};




/* function definitions */

/* Public */

/* For init */
int hotsync_init_read (struct hotsync_file_data * in_file);
int hotsync_init_write (struct hotsync_file_data * in_file);
void hotsync_exit_read (struct hotsync_file_data * in_file);
void hotsync_exit_write (struct hotsync_file_data * out_file);
int hotsync_set_read_option (struct hotsync_file_data * in_file, char opt, char * opt_arg);
int hotsync_set_write_option (struct hotsync_file_data * out_file, char opt, char * opt_arg);

/* For opening & closing */
void hotsync_open_read (struct hotsync_file_data * in_file, struct header_data * header);
void hotsync_open_write (struct hotsync_file_data * out_file, struct header_data * header);
void hotsync_close_read (struct hotsync_file_data * in_file, struct header_data * header);
void hotsync_close_write (struct hotsync_file_data * out_file, struct header_data * header);
void hotsync_abort_read (struct hotsync_file_data * in_file);
void hotsync_abort_write (struct hotsync_file_data * out_file);

/* For reading */
int hotsync_read_eof (struct hotsync_file_data * in_file);
void hotsync_read_row (struct hotsync_file_data * in_file, struct row_data * row);

/* For writing */
void hotsync_write_row (struct hotsync_file_data * out_file, struct header_data * header, struct row_data * row);

/* For statistics */
void hotsync_show_read_statistics (struct hotsync_file_data * in_file);
void hotsync_show_write_statistics (struct hotsync_file_data * out_file);


/* Private */
void hotsync_open (struct hotsync_file_data * file, struct header_data * header, enum dlpOpenFlags open_flags, char * device);
void hotsync_close (struct hotsync_file_data * file, char * log_message, int successSync);

void hotsync_read_header (struct hotsync_file_data * in_file, struct header_data * header);
void hotsync_read_specific_row (struct hotsync_file_data * in_file, struct row_data * row, int record_num);

void hotsync_write_header (struct hotsync_file_data * out_file, struct header_data * header);



#endif
