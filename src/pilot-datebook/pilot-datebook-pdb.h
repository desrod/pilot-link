/*
 * Pilot Datebook processing utility
 * Matthias Hessler <pilot-datebook@mhessler.de> December 2000
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */
          

#ifndef _PILOT_DATEBOOK_PDB
#define _PILOT_DATEBOOK_PDB

#include "pilot-datebook-data.h"



/* data structures */

/* file data */
struct pdb_file_data {
  char * filename;

  /* file handle stuff for reading pilotlink data file */
  BOOLEAN file_is_open;
  struct pi_file *pf;
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
int pdb_init_read (struct pdb_file_data * in_file);
int pdb_init_write (struct pdb_file_data * in_file);
void pdb_exit_read (struct pdb_file_data * in_file);
void pdb_exit_write (struct pdb_file_data * out_file);
int pdb_set_read_option (struct pdb_file_data * in_file, char opt, char * opt_arg);
int pdb_set_write_option (struct pdb_file_data * out_file, char opt, char * opt_arg);

/* For opening & closing */
void pdb_open_read (struct pdb_file_data * in_file, struct header_data * header);
void pdb_open_write (struct pdb_file_data * out_file, struct header_data * header);
void pdb_close_read (struct pdb_file_data * in_file, struct header_data * header);
void pdb_close_write (struct pdb_file_data * out_file, struct header_data * header);
void pdb_abort_read (struct pdb_file_data * in_file);
void pdb_abort_write (struct pdb_file_data * out_file);

/* For reading */
int pdb_read_eof (struct pdb_file_data * in_file);
void pdb_read_row (struct pdb_file_data * in_file, struct row_data * row);

/* For writing */
void pdb_write_row (struct pdb_file_data * out_file, struct header_data * header, struct row_data * row);

/* For statistics */
void pdb_show_read_statistics (struct pdb_file_data * in_file);
void pdb_show_write_statistics (struct pdb_file_data * out_file);


/* Private */
void pdb_read_header (struct pdb_file_data * in_file, struct header_data * header);
void pdb_read_specific_row (struct pdb_file_data * in_file, struct row_data * row, int record_num);

void pdb_write_header (struct pdb_file_data * out_file, struct header_data * header);



#endif
