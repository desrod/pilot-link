/*
 * Pilot Datebook processing utility
 * Matthias Hessler <pilot-datebook@mhessler.de> December 2000
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

          
#include "pilot-datebook-io.h"
#include "pilot-datebook-data.h"

#include "pilot-datebook-hotsync.h"
#include "pilot-datebook-pdb.h"
#include "pilot-datebook-csv.h"
#include "pilot-datebook-windat.h"
#include "pilot-datebook-longtxt.h"
#include "pilot-datebook-shorttxt.h"
#include "pilot-datebook-remind.h"
#include "pilot-datebook-ical.h"




/* Public functions */

/* IO dispatcher functions */

/* Init */

/* Initialize read data structure */
int
io_init_read (struct file_data * in_file, char * opt_arg)
{
  int rc = FALSE;


  /* Debug */
  debug_message("Entering io_init_read\n");

  /* Verify file format */
  in_file->file_format = txt2dataformat(opt_arg);
  if (in_file->file_format == DATA_FORMAT_INVALID) {
    fprintf(stderr, "File format <%s> for reading is unknown\n",
	    dataformat2txt(in_file->file_format));
    return FALSE;
  }

  /* Init own data structure */
  in_file->records_read = 0;
  in_file->records_written = 0;

  /* Init sub data structure */
  switch (in_file->file_format)
    {
    case DATA_FORMAT_HOTSYNC:
      rc = hotsync_init_read(&(in_file->file_data.hotsync));
      break;
    case DATA_FORMAT_PDB:
      rc = pdb_init_read(&(in_file->file_data.pdb));
      break;
    case DATA_FORMAT_CSV:
      rc = csv_init_read(&(in_file->file_data.csv));
      break;
    case DATA_FORMAT_WINDAT:
      rc = windat_init_read(&(in_file->file_data.windat));
      break;
    case DATA_FORMAT_LONGTXT:
      rc = longtxt_init_read(&(in_file->file_data.longtxt));
      break;
    case DATA_FORMAT_SHORTTXT:
      rc = shorttxt_init_read(&(in_file->file_data.shorttxt));
      break;
    default:
      fprintf(stderr, "Can not process format <%s> for reading\n",
	      dataformat2txt(in_file->file_format));
    }

  /* Debug */
  debug_message("Leaving io_init_read\n");

  return rc;
}


/* Initialize write data structure */
int
io_init_write (struct file_data * out_file, char * opt_arg)
{
  int rc = FALSE;


  /* Debug */
  debug_message("Entering io_init_write\n");

  /* Verify file format */
  out_file->file_format = txt2dataformat(opt_arg);
  if (out_file->file_format == DATA_FORMAT_INVALID) {
    fprintf(stderr, "File format <%s> for writing is unknown\n",
	    dataformat2txt(out_file->file_format));
    return FALSE;
  }

  /* Init own data structure */
  out_file->records_read = 0;
  out_file->records_written = 0;

  /* Init sub data structure */
  switch (out_file->file_format)
    {
    case DATA_FORMAT_HOTSYNC:
      rc = hotsync_init_write(&(out_file->file_data.hotsync));
      break;
    case DATA_FORMAT_PDB:
      rc = pdb_init_write(&(out_file->file_data.pdb));
      break;
    case DATA_FORMAT_CSV:
      rc = csv_init_write(&(out_file->file_data.csv));
      break;
    case DATA_FORMAT_WINDAT:
      rc = windat_init_write(&(out_file->file_data.windat));
      break;
    case DATA_FORMAT_LONGTXT:
      rc = longtxt_init_write(&(out_file->file_data.longtxt));
      break;
    case DATA_FORMAT_SHORTTXT:
      rc = shorttxt_init_write(&(out_file->file_data.shorttxt));
      break;
    case DATA_FORMAT_REMIND:
      rc = remind_init_write(&(out_file->file_data.remind));
      break;
    case DATA_FORMAT_ICAL:
      rc = ical_init_write(&(out_file->file_data.ical));
      break;
    default:
      fprintf(stderr, "Can not process format <%s> for writing\n",
	      dataformat2txt(out_file->file_format));
    }

  /* Debug */
  debug_message("Leaving io_init_write\n");

  return rc;
}


/* Destroy read data structure */
void
io_exit_read (struct file_data * in_file)
{

  /* Debug */
  debug_message("Entering io_exit_read\n");

  /* Destroy sub data structure */
  switch (in_file->file_format)
    {
    case DATA_FORMAT_HOTSYNC:
      hotsync_exit_read(&(in_file->file_data.hotsync));
      break;
    case DATA_FORMAT_PDB:
      pdb_exit_read(&(in_file->file_data.pdb));
      break;
    case DATA_FORMAT_CSV:
      csv_exit_read(&(in_file->file_data.csv));
      break;
    case DATA_FORMAT_WINDAT:
      windat_exit_read(&(in_file->file_data.windat));
      break;
    case DATA_FORMAT_LONGTXT:
      longtxt_exit_read(&(in_file->file_data.longtxt));
      break;
    case DATA_FORMAT_SHORTTXT:
      shorttxt_exit_read(&(in_file->file_data.shorttxt));
      break;
    default:
      fprintf(stderr, "Can not process reading options for format <%s>\n",
	      dataformat2txt(in_file->file_format));
    }

  /* Debug */
  debug_message("Leaving io_exit_read\n");
}


/* Destroy write data structure */
void
io_exit_write (struct file_data * out_file)
{

  /* Debug */
  debug_message("Entering io_exit_write\n");

  /* Destroy sub data structure */
  switch (out_file->file_format)
    {
    case DATA_FORMAT_HOTSYNC:
      hotsync_exit_write(&(out_file->file_data.hotsync));
      break;
    case DATA_FORMAT_PDB:
      pdb_exit_write(&(out_file->file_data.pdb));
      break;
    case DATA_FORMAT_CSV:
      csv_exit_write(&(out_file->file_data.csv));
      break;
    case DATA_FORMAT_WINDAT:
      windat_exit_write(&(out_file->file_data.windat));
      break;
    case DATA_FORMAT_LONGTXT:
      longtxt_exit_write(&(out_file->file_data.longtxt));
      break;
    case DATA_FORMAT_SHORTTXT:
      shorttxt_exit_write(&(out_file->file_data.shorttxt));
      break;
    case DATA_FORMAT_REMIND:
      remind_exit_write(&(out_file->file_data.remind));
      break;
    case DATA_FORMAT_ICAL:
      ical_exit_write(&(out_file->file_data.ical));
      break;
    default:
      fprintf(stderr, "Can not process writing options for format <%s>\n",
	      dataformat2txt(out_file->file_format));
    }

  /* Debug */
  debug_message("Leaving io_exit_write\n");
}


/* Set read command line option */
int
io_set_read_option (struct file_data * in_file, char opt, char * opt_arg)
{
  int rc = FALSE;


  /* Debug */
  debug_message("Entering io_set_read_option\n");

  switch (in_file->file_format)
    {
    case DATA_FORMAT_HOTSYNC:
      rc = hotsync_set_read_option(&(in_file->file_data.hotsync), opt, opt_arg);
      break;
    case DATA_FORMAT_PDB:
      rc = pdb_set_read_option(&(in_file->file_data.pdb), opt, opt_arg);
      break;
    case DATA_FORMAT_CSV:
      rc = csv_set_read_option(&(in_file->file_data.csv), opt, opt_arg);
      break;
    case DATA_FORMAT_WINDAT:
      rc = windat_set_read_option(&(in_file->file_data.windat), opt, opt_arg);
      break;
    case DATA_FORMAT_LONGTXT:
      rc = longtxt_set_read_option(&(in_file->file_data.longtxt), opt, opt_arg);
      break;
    case DATA_FORMAT_SHORTTXT:
      rc = shorttxt_set_read_option(&(in_file->file_data.shorttxt), opt, opt_arg);
      break;
    default:
      fprintf(stderr, "Can not process format <%s> for opening input file\n",
	      dataformat2txt(in_file->file_format));
    }

  /* Debug */
  debug_message("Leaving io_set_read_option\n");

  return rc;
}


/* Set write command line option */
int
io_set_write_option (struct file_data * out_file, char opt, char * opt_arg)
{
  int rc = FALSE;


  /* Debug */
  debug_message("Entering io_set_write_option\n");

  switch (out_file->file_format)
    {
    case DATA_FORMAT_HOTSYNC:
      rc = hotsync_set_write_option(&(out_file->file_data.hotsync), opt, opt_arg);
      break;
    case DATA_FORMAT_PDB:
      rc = pdb_set_write_option(&(out_file->file_data.pdb), opt, opt_arg);
      break;
    case DATA_FORMAT_CSV:
      rc = csv_set_write_option(&(out_file->file_data.csv), opt, opt_arg);
      break;
    case DATA_FORMAT_WINDAT:
      rc = windat_set_write_option(&(out_file->file_data.windat), opt, opt_arg);
      break;
    case DATA_FORMAT_LONGTXT:
      rc = longtxt_set_write_option(&(out_file->file_data.longtxt), opt, opt_arg);
      break;
    case DATA_FORMAT_SHORTTXT:
      rc = shorttxt_set_write_option(&(out_file->file_data.shorttxt), opt, opt_arg);
      break;
    case DATA_FORMAT_REMIND:
      rc = remind_set_write_option(&(out_file->file_data.remind), opt, opt_arg);
      break;
    case DATA_FORMAT_ICAL:
      rc = ical_set_write_option(&(out_file->file_data.ical), opt, opt_arg);
      break;
    default:
      fprintf(stderr, "Can not process format <%s> for opening input file\n",
	      dataformat2txt(out_file->file_format));
    }

  /* Debug */
  debug_message("Leaving io_set_write_option\n");

  return rc;
}


/* for opening & closing */

/* open input file for reading */
void
io_open_read (struct file_data * in_file, struct header_data * header)
{

  /* Debug */
  debug_message("Entering io_open_read\n");

  switch (in_file->file_format)
    {
    case DATA_FORMAT_HOTSYNC:
      hotsync_open_read(&(in_file->file_data.hotsync), header);
      break;
    case DATA_FORMAT_PDB:
      pdb_open_read(&(in_file->file_data.pdb), header);
      break;
    case DATA_FORMAT_CSV:
      csv_open_read(&(in_file->file_data.csv), header);
      break;
    case DATA_FORMAT_WINDAT:
      windat_open_read(&(in_file->file_data.windat), header);
      break;
    case DATA_FORMAT_LONGTXT:
      longtxt_open_read(&(in_file->file_data.longtxt), header);
      break;
    case DATA_FORMAT_SHORTTXT:
      shorttxt_open_read(&(in_file->file_data.shorttxt), header);
      break;
    default:
      fprintf(stderr, "Can not process format <%s> for opening input file\n",
	      dataformat2txt(in_file->file_format));
      exit (1);
    }

  /* Init data structure */
  in_file->records_read = 0;
  in_file->records_written = 0;

  /* Debug */
  debug_message("Leaving io_open_read\n");
}


/* open output file for writing */
void
io_open_write (struct file_data * out_file, struct header_data * header)
{

  /* Debug */
  debug_message("Entering io_open_write\n");

  switch (out_file->file_format)
    {
    case DATA_FORMAT_HOTSYNC:
      hotsync_open_write(&(out_file->file_data.hotsync), header);
      break;
    case DATA_FORMAT_PDB:
      pdb_open_write(&(out_file->file_data.pdb), header);
      break;
    case DATA_FORMAT_CSV:
      csv_open_write(&(out_file->file_data.csv), header);
      break;
    case DATA_FORMAT_WINDAT:
      windat_open_write(&(out_file->file_data.windat), header);
      break;
    case DATA_FORMAT_LONGTXT:
      longtxt_open_write(&(out_file->file_data.longtxt), header);
      break;
    case DATA_FORMAT_SHORTTXT:
      shorttxt_open_write(&(out_file->file_data.shorttxt), header);
      break;
    case DATA_FORMAT_REMIND:
      remind_open_write(&(out_file->file_data.remind), header);
      break;
    case DATA_FORMAT_ICAL:
      ical_open_write(&(out_file->file_data.ical), header);
      break;
    default:
      fprintf(stderr, "Can not process format <%s> for opening output file\n",
	      dataformat2txt(out_file->file_format));
      exit (1);
    }

  /* Init data structure */
  out_file->records_read = 0;
  out_file->records_written = 0;

  /* Debug */
  debug_message("Leaving io_open_write\n");
}


/* Close input file after end of processing */
void
io_close_read (struct file_data * in_file, struct header_data * header)
{

  /* Debug */
  debug_message("Entering io_close_read\n");

  switch (in_file->file_format)
    {
    case DATA_FORMAT_HOTSYNC:
      hotsync_close_read(&(in_file->file_data.hotsync), header);
      break;
    case DATA_FORMAT_PDB:
      pdb_close_read(&(in_file->file_data.pdb), header);
      break;
    case DATA_FORMAT_CSV:
      csv_close_read(&(in_file->file_data.csv), header);
      break;
    case DATA_FORMAT_WINDAT:
      windat_close_read(&(in_file->file_data.windat), header);
      break;
    case DATA_FORMAT_LONGTXT:
      longtxt_close_read(&(in_file->file_data.longtxt), header);
      break;
    case DATA_FORMAT_SHORTTXT:
      shorttxt_close_read(&(in_file->file_data.shorttxt), header);
      break;
    default:
      fprintf(stderr, "Can not process format <%s> for closing input file\n",
	      dataformat2txt(in_file->file_format));
      exit (1);
    }

  /* Debug */
  debug_message("Leaving io_close_read\n");
}


/* Close output file after end of processing */
void
io_close_write (struct file_data * out_file, struct header_data * header)
{

  /* Debug */
  debug_message("Entering io_close_write\n");

  switch (out_file->file_format)
    {
    case DATA_FORMAT_HOTSYNC:
      hotsync_close_write(&(out_file->file_data.hotsync), header);
      break;
    case DATA_FORMAT_PDB:
      pdb_close_write(&(out_file->file_data.pdb), header);
      break;
    case DATA_FORMAT_CSV:
      csv_close_write(&(out_file->file_data.csv), header);
      break;
    case DATA_FORMAT_WINDAT:
      windat_close_write(&(out_file->file_data.windat), header);
      break;
    case DATA_FORMAT_LONGTXT:
      longtxt_close_write(&(out_file->file_data.longtxt), header);
      break;
    case DATA_FORMAT_SHORTTXT:
      shorttxt_close_write(&(out_file->file_data.shorttxt), header);
      break;
    case DATA_FORMAT_REMIND:
      remind_close_write(&(out_file->file_data.remind), header);
      break;
    case DATA_FORMAT_ICAL:
      ical_close_write(&(out_file->file_data.ical), header);
      break;
    default:
      fprintf(stderr, "Can not process format <%s> for closing output file\n",
	      dataformat2txt(out_file->file_format));
      exit (1);
    }

  /* Debug */
  debug_message("Leaving io_close_write\n");
}


/* Close input file in case of an error */
void
io_abort_read (struct file_data * in_file)
{

  /* Debug */
  debug_message("Entering io_abort_read\n");

  switch (in_file->file_format)
    {
    case DATA_FORMAT_HOTSYNC:
      hotsync_abort_read(&(in_file->file_data.hotsync));
      break;
    case DATA_FORMAT_PDB:
      pdb_abort_read(&(in_file->file_data.pdb));
      break;
    case DATA_FORMAT_CSV:
      csv_abort_read(&(in_file->file_data.csv));
      break;
    case DATA_FORMAT_WINDAT:
      windat_abort_read(&(in_file->file_data.windat));
      break;
    case DATA_FORMAT_LONGTXT:
      longtxt_abort_read(&(in_file->file_data.longtxt));
      break;
    case DATA_FORMAT_SHORTTXT:
      shorttxt_abort_read(&(in_file->file_data.shorttxt));
      break;
    default:
      fprintf(stderr, "Can not process format <%s> for closing input file on error\n",
	      dataformat2txt(in_file->file_format));
      exit (1);
    }

  /* Debug */
  debug_message("Leaving io_abort_read\n");
}


/* Close output file in case of an error */
void
io_abort_write (struct file_data * out_file)
{

  /* Debug */
  debug_message("Entering io_abort_write\n");

  switch (out_file->file_format)
    {
    case DATA_FORMAT_HOTSYNC:
      hotsync_abort_write(&(out_file->file_data.hotsync));
      break;
    case DATA_FORMAT_PDB:
      pdb_abort_write(&(out_file->file_data.pdb));
      break;
    case DATA_FORMAT_CSV:
      csv_abort_write(&(out_file->file_data.csv));
      break;
    case DATA_FORMAT_WINDAT:
      windat_abort_write(&(out_file->file_data.windat));
      break;
    case DATA_FORMAT_LONGTXT:
      longtxt_abort_write(&(out_file->file_data.longtxt));
      break;
    case DATA_FORMAT_SHORTTXT:
      shorttxt_abort_write(&(out_file->file_data.shorttxt));
      break;
    case DATA_FORMAT_REMIND:
      remind_abort_write(&(out_file->file_data.remind));
      break;
    case DATA_FORMAT_ICAL:
      ical_abort_write(&(out_file->file_data.ical));
      break;
    default:
      fprintf(stderr, "Can not process format <%s> for closing output file on error\n",
	      dataformat2txt(out_file->file_format));
      exit (1);
    }

  /* Debug */
  debug_message("Leaving io_abort_write\n");
}



/* for reading */

/* read next row from input file */
void
io_read_row (struct file_data * in_file, struct row_data * row)
{
  /* Debug */
  debug_message("Entering io_read_row\n");

  switch (in_file->file_format)
    {
    case DATA_FORMAT_HOTSYNC:
      hotsync_read_row (&(in_file->file_data.hotsync), row);
      break;
    case DATA_FORMAT_PDB:
      pdb_read_row (&(in_file->file_data.pdb), row);
      break;
    case DATA_FORMAT_CSV:
      csv_read_row (&(in_file->file_data.csv), row);
      break;
    case DATA_FORMAT_WINDAT:
      windat_read_row (&(in_file->file_data.windat), row);
      break;
    case DATA_FORMAT_LONGTXT:
      longtxt_read_row (&(in_file->file_data.longtxt), row);
      break;
    case DATA_FORMAT_SHORTTXT:
      shorttxt_read_row (&(in_file->file_data.shorttxt), row);
      break;
    default:
      fprintf(stderr, "Can not process format <%s> for reading next row from input file\n",
	      dataformat2txt(in_file->file_format));
      exit (1);
    }

  /* Increase record counter */
  in_file->records_read++;

  /* Debug */
  debug_message("Leaving io_read_row\n");
}


/* check end of input file */
int
io_read_eof (struct file_data * in_file)
{
  int rc = FALSE;


  /* Debug */
  debug_message("Entering io_read_eof\n");

  switch (in_file->file_format)
    {
    case DATA_FORMAT_HOTSYNC:
      rc = hotsync_read_eof(&(in_file->file_data.hotsync));
      break;
    case DATA_FORMAT_PDB:
      rc = pdb_read_eof(&(in_file->file_data.pdb));
      break;
    case DATA_FORMAT_CSV:
      rc = csv_read_eof(&(in_file->file_data.csv));
      break;
    case DATA_FORMAT_WINDAT:
      rc = windat_read_eof(&(in_file->file_data.windat));
      break;
    case DATA_FORMAT_LONGTXT:
      rc = longtxt_read_eof(&(in_file->file_data.longtxt));
      break;
    case DATA_FORMAT_SHORTTXT:
      rc = shorttxt_read_eof(&(in_file->file_data.shorttxt));
      break;
    default:
      fprintf(stderr, "Can not process format <%s> for checking end of input file\n",
	      dataformat2txt(in_file->file_format));
      exit (1);
    }

  /* Debug */
  debug_message("Leaving io_read_eof\n");

  return rc;
}




/* for writing */

/* write a row to output file */
void
io_write_row (struct file_data * out_file, struct header_data * header, struct row_data * row)
{

  /* Debug */
  debug_message("Entering io_write_row\n");

  switch (out_file->file_format)
    {
    case DATA_FORMAT_HOTSYNC:
      hotsync_write_row (&(out_file->file_data.hotsync), header, row);
      break;
    case DATA_FORMAT_PDB:
      pdb_write_row (&(out_file->file_data.pdb), header, row);
      break;
    case DATA_FORMAT_CSV:
      csv_write_row (&(out_file->file_data.csv), header, row);
      break;
    case DATA_FORMAT_WINDAT:
      windat_write_row (&(out_file->file_data.windat), header, row);
      break;
    case DATA_FORMAT_LONGTXT:
      longtxt_write_row (&(out_file->file_data.longtxt), header, row);
      break;
    case DATA_FORMAT_SHORTTXT:
      shorttxt_write_row (&(out_file->file_data.shorttxt), header, row);
      break;
    case DATA_FORMAT_REMIND:
      remind_write_row (&(out_file->file_data.remind), header, row);
      break;
    case DATA_FORMAT_ICAL:
      ical_write_row (&(out_file->file_data.ical), header, row);
      break;
    default:
      fprintf(stderr, "Can not process format <%s> for writing row to output file\n",
	      dataformat2txt(out_file->file_format));
      exit (1);
    }

  /* Increase record counter */
  out_file->records_written++;

  /* Debug */
  debug_message("Leaving io_write_row\n");
}



/* For statistics */

/* Write special input statistics */
void
io_show_read_statistics (struct file_data * in_file)
{

  /* Debug */
  debug_message("Entering io_show_read_statistics\n");

  switch (in_file->file_format)
    {
    case DATA_FORMAT_HOTSYNC:
      hotsync_show_read_statistics (&(in_file->file_data.hotsync));
      break;
    case DATA_FORMAT_PDB:
      pdb_show_read_statistics (&(in_file->file_data.pdb));
      break;
    case DATA_FORMAT_CSV:
      csv_show_read_statistics (&(in_file->file_data.csv));
      break;
    case DATA_FORMAT_WINDAT:
      windat_show_read_statistics (&(in_file->file_data.windat));
      break;
    case DATA_FORMAT_LONGTXT:
      longtxt_show_read_statistics (&(in_file->file_data.longtxt));
      break;
    case DATA_FORMAT_SHORTTXT:
      shorttxt_show_read_statistics (&(in_file->file_data.shorttxt));
      break;
    default:
      fprintf(stderr, "Can not process format <%s> for writing input statistics\n",
	      dataformat2txt(in_file->file_format));
      exit (1);
    }

  /* Debug */
  debug_message("Leaving io_show_read_statistics\n");
}


/* Write special output statistics */
void
io_show_write_statistics (struct file_data * out_file)
{

  /* Debug */
  debug_message("Entering io_show_write_statistics\n");

  switch (out_file->file_format)
    {
    case DATA_FORMAT_HOTSYNC:
      hotsync_show_write_statistics (&(out_file->file_data.hotsync));
      break;
    case DATA_FORMAT_PDB:
      pdb_show_write_statistics (&(out_file->file_data.pdb));
      break;
    case DATA_FORMAT_CSV:
      csv_show_write_statistics (&(out_file->file_data.csv));
      break;
    case DATA_FORMAT_WINDAT:
      windat_show_write_statistics (&(out_file->file_data.windat));
      break;
    case DATA_FORMAT_LONGTXT:
      longtxt_show_write_statistics (&(out_file->file_data.longtxt));
      break;
    case DATA_FORMAT_SHORTTXT:
      shorttxt_show_write_statistics (&(out_file->file_data.shorttxt));
      break;
    case DATA_FORMAT_REMIND:
      remind_show_write_statistics (&(out_file->file_data.remind));
      break;
    case DATA_FORMAT_ICAL:
      ical_show_write_statistics (&(out_file->file_data.ical));
      break;
    default:
      fprintf(stderr, "Can not process format <%s> for writing output statistics\n",
	      dataformat2txt(out_file->file_format));
      exit (1);
    }

  /* Debug */
  debug_message("Leaving io_show_write_statistics\n");
}
