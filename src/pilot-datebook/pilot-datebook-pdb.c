/*
 * Pilot Datebook processing utility
 *
 * (c) 2000, Matthias Hessler <pilot-datebook@mhessler.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <string.h>
#include "pilot-datebook-data.h"
#include "pilot-datebook-pdb.h"


/*
 * Please note that the fields misc_flags and index in DBInfo are Palm
 * OS 2.0 specific and currently not part of the pdb format, therefore
 * they will NOT be stored permanently when writing to pdb.
 */

/* Constants (default header settings) */
const char PDB_DEFAULT_DB_NAME[] = "DatebookDB";
char PDB_DEFAULT_CREATOR[] = "date";
char PDB_DEFAULT_TYPE[] = "DATA";


/* Public functions */

/* For init */

/* Initialize read data structure */
int
pdb_init_read (struct pdb_file_data * in_file)
{

  /* Debug */
  debug_message("Entering pdb_init_read\n");

  /* Init own data structure */
  in_file->filename = NULL;
  in_file->file_is_open = FALSE;
  in_file->pf = NULL;
  in_file->num_recs = 0;
  in_file->next_rec = 0;
  in_file->records_read = 0;
  in_file->records_written = 0;

  /* Debug */
  debug_message("Leaving pdb_init_read\n");

  return TRUE;
}


/* Initialize write data structure */
int
pdb_init_write (struct pdb_file_data * out_file)
{

  /* Debug */
  debug_message("Entering pdb_init_write\n");

  /* Init own data structure */
  out_file->filename = NULL;
  out_file->file_is_open = FALSE;
  out_file->pf = NULL;
  out_file->num_recs = 0;
  out_file->next_rec = 0;
  out_file->records_read = 0;
  out_file->records_written = 0;

  /* Debug */
  debug_message("Leaving pdb_init_write\n");

  return TRUE;
}


/* Destroy read data structure */
void
pdb_exit_read (struct pdb_file_data * in_file)
{

  /* Debug */
  debug_message("Entering pdb_exit_read\n");

  /* Free memory */
  if (in_file->filename)
    free (in_file->filename);

  /* Debug */
  debug_message("Leaving pdb_exit_read\n");
}


/* Destroy write data structure */
void
pdb_exit_write (struct pdb_file_data * out_file)
{

  /* Debug */
  debug_message("Entering pdb_exit_write\n");

  /* Free memory */
  if (out_file->filename)
    free (out_file->filename);

  /* Debug */
  debug_message("Leaving pdb_exit_write\n");
}


/* Set read command line option */
int
pdb_set_read_option (struct pdb_file_data * in_file, char opt, char * opt_arg)
{
  int rc = FALSE;


  /* Debug */
  debug_message("Entering pdb_set_read_option\n");

  switch (opt)
    {
    case 'f':
      /* Filename */
      if (opt_arg != NULL
	  && *opt_arg != '\0') {
	if (in_file->filename)
	  free (in_file->filename);
	in_file->filename = strdup(opt_arg);
	rc = TRUE;
      }
      break;
    default:
      fprintf(stderr, "Can not process read option <%c> for input file\n",
	      opt);
    }

  /* Debug */
  debug_message("Leaving pdb_set_read_option\n");

  return rc;
}


/* Set write command line option */
int
pdb_set_write_option (struct pdb_file_data * out_file, char opt, char * opt_arg)
{
  int rc = FALSE;


  /* Debug */
  debug_message("Entering pdb_set_write_option\n");

  switch (opt)
    {
    case 'f':
      /* Filename */
      if (opt_arg != NULL
	  && *opt_arg != '\0') {
	if (out_file->filename)
	  free (out_file->filename);
	out_file->filename = strdup(opt_arg);
	rc = TRUE;
      }
      break;
    default:
      fprintf(stderr, "Can not process write option <%c> for output file\n",
	      opt);
    }

  /* Debug */
  debug_message("Leaving pdb_set_write_option\n");

  return rc;
}



/* For opening & closing */

/* Open input data file for reading */
void
pdb_open_read (struct pdb_file_data * in_file, struct header_data * header)
{

  /* Debug */
  debug_message("Entering pdb_open_read\n");

  /* Open file */
  if (!in_file->filename) {
    /* Can not read from stdin for pdb file format */
    error_message("Can not read from stdin for read fileformat <%s>\n",
		  dataformat2txt(DATA_FORMAT_PDB));
  }
  else {
    if ((in_file->pf = pi_file_open (in_file->filename)) == NULL)
      error_message("Can not open file <%s> for reading\n",
		    in_file->filename);
    in_file->file_is_open = TRUE;
  }

  /* Init */
  in_file->num_recs = 0;
  in_file->next_rec = 0;

  /* Read header data */
  pdb_read_header (in_file, header);

  /* Debug */
  debug_message("Leaving pdb_open_read\n");
}


/* Open output data file for writing */
void
pdb_open_write (struct pdb_file_data * out_file, struct header_data * header)
{
  struct DBInfo ip;


  /* Debug */
  debug_message("Entering pdb_open_write\n");

  /* Get datebook header data */
  if (!header->isValid) {
    /* Prepare dummy header if header not present */
    memset(&ip, 0, sizeof(ip));
    strncpy(ip.name, PDB_DEFAULT_DB_NAME, sizeof(ip.name) -1);
    ip.creator = makelong(PDB_DEFAULT_CREATOR);
    ip.type = makelong(PDB_DEFAULT_TYPE);
  }
  else {
    /* Get header information */
    ip = header->info;
  }

  /* Open file */
  if (!out_file->filename) {
    /* Can not write to stdout for pdb file format */
    error_message("Can not write to stdout for write fileformat <%s>\n",
		  dataformat2txt(DATA_FORMAT_PDB));
  }
  else {
    /* Open file, using datebook header data */
    if ((out_file->pf = pi_file_create (out_file->filename, &ip)) == NULL)
      error_message("Can not open output file <%s> for writing\n",
		    out_file->filename);
    out_file->file_is_open = TRUE;
  }

  /* Init */
  out_file->num_recs = 0;
  out_file->next_rec = 0;

  /* Write header */
  pdb_write_header (out_file, header);

  /* Debug */
  debug_message("Leaving pdb_open_write\n");
}


/* Close input file at end of processing */
void
pdb_close_read (struct pdb_file_data * in_file, struct header_data * header)
{

  /* Debug */
  debug_message("Entering pdb_close_read\n");

  /* Close file */
  if (in_file->file_is_open == TRUE) {
    pi_file_close (in_file->pf);
    in_file->file_is_open = FALSE;
  }

  /* Update statistics */

  /* clear data structures */
  in_file->pf = NULL;
  in_file->num_recs = 0;
  in_file->next_rec = 0;

  /* Debug */
  debug_message("Leaving pdb_close_read\n");
}


/* Close output file at end of processing */
void
pdb_close_write (struct pdb_file_data * out_file, struct header_data * header)
{

  /* Debug */
  debug_message("Entering pdb_close_write\n");

  /* Close file */
  if (out_file->file_is_open == TRUE) {
    pi_file_close (out_file->pf);
    out_file->file_is_open = FALSE;
  }

  /* Update statistics */

  /* clear data structures */
  out_file->pf = NULL;
  out_file->num_recs = 0;
  out_file->next_rec = 0;

  /* Debug */
  debug_message("Leaving pdb_close_write\n");
}


/* Close input file in case of an error */
void
pdb_abort_read (struct pdb_file_data * in_file)
{

  /* Debug */
  debug_message("Entering pdb_abort_read\n");

  /* Close file */
  if (in_file->file_is_open == TRUE) {
    pi_file_close (in_file->pf);
    in_file->file_is_open = FALSE;
  }

  /* No special error processing needed */

  /* Update statistics */

  /* clear data structures */
  in_file->pf = NULL;
  in_file->num_recs = 0;
  in_file->next_rec = 0;

  /* Debug */
  debug_message("Leaving pdb_abort_read\n");
}


/* Close output file in case of an error */
void
pdb_abort_write (struct pdb_file_data * out_file)
{

  /* Debug */
  debug_message("Entering pdb_abort_write\n");

  /* Close file */
  if (out_file->file_is_open == TRUE) {
    pi_file_close (out_file->pf);
    out_file->file_is_open = FALSE;
  }

  /* Remove incompletely written file */
  if (out_file->filename) {
    info_message("Removing incompletely written output file <%s>\n",
		 out_file->filename);
    if (out_file->filename)
      unlink(out_file->filename);
  }

  /* Update statistics */

  /* Clear data structures */
  out_file->pf = NULL;
  out_file->num_recs = 0;
  out_file->next_rec = 0;

  /* Debug */
  debug_message("Leaving pdb_abort_write\n");
}



/* For reading */

/* Identify end of file */
int
pdb_read_eof (struct pdb_file_data * in_file)
{

  /* Debug */
  debug_message("Entering pdb_read_eof\n");

  /* have to compare greater or equal, because first record number is 0 */
  return (in_file->next_rec >= in_file->num_recs);

  /* Debug */
  debug_message("Leaving pdb_read_eof\n");
}


/* Read an appointment record */
void
pdb_read_row (struct pdb_file_data * in_file, struct row_data * row)
{

  /* Debug */
  debug_message("Entering pdb_read_row\n");

  /* read record */
  pdb_read_specific_row (in_file, row, in_file->next_rec);

  /* increment next record number */
  in_file->next_rec++;

  /* Update statistics */
  in_file->records_read++;

  /* Debug */
  debug_message("Leaving pdb_read_row\n");
}




/* For writing */

/* Write an appointment record */
void
pdb_write_row (struct pdb_file_data * out_file, struct header_data * header, struct row_data * row)
{
  struct Appointment a;
  int attributes;
  int category;
  unsigned long uid;
  int record_num;

  unsigned char buffer [0xffff];
  unsigned int buffer_len = 0;
  

  /* Debug */
  debug_message("Entering pdb_write_row\n");

  if (!getRowIsValid(row))
    error_message("Can not write invalid row.\n");

  /* Get datebook row data */
  record_num = getRowRecordNum(row);
  uid = getRowUid(row);
  attributes = getRowAttributes(row);
  category = getRowCategory(row);
  a = getRowAppointment(row);


  /* Write datebook row data */
  buffer_len = pack_Appointment(&a, buffer, sizeof(buffer));
  if (pi_file_append_record (out_file->pf, &buffer, buffer_len, attributes, category, uid))
    error_message("Write of datebook application row %d to output file failed!\n\n",
		  record_num);

  /* Update statistics */
  out_file->records_written++;

  /* Debug */
  debug_message("Leaving pdb_write_row\n");
}



/* For statistics */

/* Show input statistics */
void
pdb_show_read_statistics (struct pdb_file_data * in_file)
{

  /* Debug */
  debug_message("Entering pdb_show_read_statistics\n");

  info_message("Input file <%s>, format <%s>:\n",
	       (in_file->filename) ? in_file->filename : "stdin",
	       dataformat2txt(DATA_FORMAT_PDB));
  info_message("Records read: %d\n",
	       in_file->records_read);

  /* Debug */
  debug_message("Leaving pdb_show_read_statistics\n");
}


/* Show output statistics */
void
pdb_show_write_statistics (struct pdb_file_data * out_file)
{

  /* Debug */
  debug_message("Entering pdb_show_write_statistics\n");

  info_message("Output file <%s>, format <%s>:\n",
	       (out_file->filename) ? out_file->filename : "stdout",
	       dataformat2txt(DATA_FORMAT_PDB));
  info_message("Records written: %d\n",
	       out_file->records_written);

  /* Debug */
  debug_message("Leaving pdb_show_write_statistics\n");
}



/* Private functions */

/* Read datebook file header */
void
pdb_read_header (struct pdb_file_data * in_file, struct header_data * header)
{
  /* The pi_file functions return pointers to internally
     stored application information structure */
  void * app_info = NULL;


  /* Debug */
  debug_message("Entering pdb_read_header\n");

  /* Skip reading header if header has already been read earlier.
   * This is not a problem, since the pi_file functions do not need
   * header data reading for positioning properly for row reading.
   */
  if (header->isValid) {
    info_message("Skip reading header, since header has already been read.\n");
  }
  else {
    /* Read info header */
    if (pi_file_get_info (in_file->pf, &header->info) < 0)
      error_message("Can not get database header info from input file\n\n");

    if ((header->info).flags & dlpDBFlagResource)
      error_message("Input file is not a Datebook file, resource flag is set!\n\n");

    /* Read datebook application header data */
    if (pi_file_get_app_info (in_file->pf,
			      &app_info,
			      &(header->app_info_size)) < 0)
      error_message("Can not get application information header data from input file\n\n");

    /* Convert datebook application header data */
    if (header->app_info_size > 0)
      unpack_AppointmentAppInfo(&(header->aai),
				app_info,
				header->app_info_size);

    /* Read datebook sort header data */
    if (pi_file_get_sort_info (in_file->pf,
			       &(header->sort_info),
			       &(header->sort_info_size)) < 0)
      error_message("Can not get sort information header data from input file\n\n");

  /* If we reach this, then header is valid
   * (otherwise an error_message would have terminated the program).
   */
  header->isValid = TRUE;
  }

  /* Read number of records */
  pi_file_get_entries (in_file->pf, &in_file->num_recs);

  /* Initialize current record number */
  in_file->next_rec = 0;

  /* Debug */
  debug_message("Leaving pdb_read_header\n");
}


/* Directly read a specific record, providing the record number */
void
pdb_read_specific_row (struct pdb_file_data * in_file, struct row_data * row, int record_num)
{
  void *buffer = NULL;
  int buffer_len = 0;

  struct Appointment a;
  int attributes;
  int category;
  unsigned long uid;


  /* Debug */
  debug_message("Entering pdb_read_specific_row\n");

  /* Check whether record_num is out of bound */
  /* Have to compare greater or equal, because first record number is 0 */
  if (record_num >= in_file->num_recs)
    error_message("Record number %d exceeds maximum number of records %d in input file\n\n",
		  record_num, in_file->num_recs);

  /* Read from file */
  if (pi_file_read_record (in_file->pf, record_num, &buffer, &buffer_len,
			   &attributes, &category, &uid) < 0)
    error_message("Can not read record <%d> from input file\n\n", record_num);

  /* Convert data */
  /* (ensure to later free appointment data after usage) */
  unpack_Appointment(&a, buffer, buffer_len);

  /* Set datebook data */
  setRowRecordNum(row, record_num);
  setRowUid(row, uid);
  setRowAttributes(row, attributes);
  setRowCategory(row, category);
  setRowAppointment(row, a);
  setRowIsValid(row, TRUE);


  /* Check whether pack will bring same result */
  /*
  {
    unsigned char buffer2[0xffff];
    int buffer_len2 = 0;
    int i;

    buffer_len2 = pack_Appointment(&a, buffer2, sizeof(buffer2));
    if (buffer_len != buffer_len2) {
    fprintf(stderr, "Warning: input record <%d> (uid=%lu): pack buffer length differs!\n",
    record_num, uid);
    }
    if (memcmp(buffer, buffer2, buffer_len) != 0) {
    fprintf(stderr, "Warning: input record <%d> (uid=%lu): pack buffer content differs!\n",
    record_num, uid);
    fprintf(stderr, "Bytes which differ:");
    for (i=0; i<buffer_len; i++) {
    if (((unsigned char *)buffer)[i] != buffer2[i])
    fprintf(stderr, " %d", i);
    }
    fprintf(stderr, "\nOriginal buffer:\n");
    write_dump (stderr, buffer, buffer_len);
    fprintf(stderr, "Re-packed buffer:\n");
    write_dump (stderr, buffer2, buffer_len2);
    fprintf(stderr, "\n");
    }
  }
  */

  /* Debug */
  debug_message("Leaving pdb_read_specific_row\n");
}


/* Write file header */
void
pdb_write_header (struct pdb_file_data * out_file, struct header_data * header)
{
  struct DBInfo ip;
  struct AppointmentAppInfo aai;
  void * sort_info;
  int sort_info_size;

  unsigned char buffer[0xffff];
  unsigned int buffer_len = 0;



  /* Debug */
  debug_message("Entering pdb_write_header\n");

  /* Prepare dummy header if header not present */
  if (!header->isValid) {
    int j;

    memset(&ip, 0, sizeof(ip));
    memset(&aai, 0, sizeof(aai));
    for(j=0;j<DATEBOOK_MAX_CATEGORIES;j++) {
      aai.category.ID[j] = 0;
      aai.category.renamed[j] = 0;
      memset(aai.category.name[j], 0, sizeof(aai.category.name[j]));
    } /* for all categories */
    aai.category.lastUniqueID = 0;
    aai.startOfWeek = 0;

    sort_info = NULL;
    sort_info_size = 0;
  }
  else {
    /* Get datebook header data */
    ip = header->info;
    aai = header->aai;
    sort_info = header->sort_info;
    sort_info_size = header->sort_info_size;
  }


  /* Set datebook application information */
  if (header->app_info_size > 0)
    buffer_len = pack_AppointmentAppInfo (&aai,
					  buffer,
					  sizeof(buffer));
  else
    buffer_len = 0;

  if(buffer_len < 0)
    error_message("Datebook application info for output file could not be packed!\n\n");

  if (pi_file_set_app_info(out_file->pf, buffer, buffer_len))
    error_message("Can not set datebook application info for output file!\n\n");


  /* Set database sort information */
  if (pi_file_set_sort_info(out_file->pf, sort_info, sort_info_size))
    error_message("Can not set datebook sort info for output file!\n\n");

  /* Debug */
  debug_message("Leaving pdb_write_header\n");
}
