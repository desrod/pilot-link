/*
 * Pilot Datebook processing utility
 * Matthias Hessler <pilot-datebook@mhessler.de> December 2000
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */


#ifndef _PILOT_DATEBOOK_IO
#define _PILOT_DATEBOOK_IO


#include "pilot-datebook-data.h"

#include "pilot-datebook-hotsync.h"
#include "pilot-datebook-pdb.h"
#include "pilot-datebook-csv.h"
#include "pilot-datebook-windat.h"
#include "pilot-datebook-longtxt.h"
#include "pilot-datebook-shorttxt.h"
#include "pilot-datebook-remind.h"
#include "pilot-datebook-ical.h"



/* data structures */

/* file handle */
struct file_data {
  enum DATA_FORMAT file_format;
  union {
    struct hotsync_file_data hotsync;
    struct pdb_file_data pdb;
    struct csv_file_data csv;
    struct windat_file_data windat;
    struct longtxt_file_data longtxt;
    struct shorttxt_file_data shorttxt;
    struct remind_file_data remind;
    struct ical_file_data ical;
  } file_data;

  /* For statistics */
  int records_read;
  int records_written;
};



/* function definitions */

/* private */

/* IO dispatcher functions */

/* For init */
int io_init_read (struct file_data * in_file, char * opt_arg);
int io_init_write (struct file_data * out_file, char * opt_arg);
void io_exit_read (struct file_data * in_file);
void io_exit_write (struct file_data * out_file);
int io_set_read_option (struct file_data * in_file, char opt, char * opt_arg);
int io_set_write_option (struct file_data * out_file, char opt, char * opt_arg);

/* For reading */
void io_open_read (struct file_data * in_file, struct header_data * header);
void io_close_read (struct file_data * in_file, struct header_data * header);

void io_abort_read (struct file_data * in_file);
void io_abort_write (struct file_data * out_file);

void io_read_row (struct file_data * in_file, struct row_data * row);
int io_read_eof (struct file_data * in_file);

/* For writing */
void io_open_write (struct file_data * out_file, struct header_data * header);
void io_close_write (struct file_data * out_file, struct header_data * header);

void io_write_row (struct file_data * out_file, struct header_data * header, struct row_data * row);

/* For statistics */
void io_show_read_statistics (struct file_data * in_file);
void io_show_write_statistics (struct file_data * out_file);


#endif
