/*
 * Pilot Datebook processing utility
 * Matthias Hessler <pilot-datebook@mhessler.de> December 2000
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <string.h>
#include "pilot-datebook-data.h"
#include "pilot-datebook-hotsync.h"


/*
 * Please note that the fields misc_flags and index in DBInfo are Palm
 * OS 2.0 specific and can currently not be restored to the Pilot
 * with pilot-link. However, as a specialty, the hotsync functions will
 * read them, whereas pilot-xfer, and the pdb routines will set them to 0.
 *
 * Also the following fields can normally not be restored, because they
 * are under the Pilot's authority:
 * - modification number
 * - creation time
 * - modification time
 * - backup time
 *
 * Another interesting thing is that for some reason the sort order within
 * the Pilot will be mixed at every writing hotsync in some strange way.
 * At least this is the case for my Palm IIIx with PALM OS 3.3.
 * The result is that the order in which records are read from the PALM
 * will be different after every Hotsync which writes to the Pilot.
 */


/* Constants */
char HOTSYNC_DATEBOOK_DB_NAME[] = "DatebookDB";
char HOTSYNC_DEFAULT_PORT[] = "/dev/pilot";

/* Public functions */

/* For init */

char *port = NULL;

static char*
hotsync_get_port(void)
{
  if (port == NULL && (port = getenv("PILOTPORT")) == NULL) {
    fprintf(stderr, "No $PILOTPORT specified and no -p <port> given.\n"
            "Defaulting to '%s'\n\n", HOTSYNC_DEFAULT_PORT);
    port = HOTSYNC_DEFAULT_PORT;
  }
  return port;
}

/* Initialize read data structure */
int
hotsync_init_read (struct hotsync_file_data * in_file)
{

  /* Debug */
  debug_message("Entering hotsync_init_read\n");

  /* Init own data structure */
  in_file->filename = hotsync_get_port();
  in_file->file_is_open = FALSE;
  in_file->socket = 0;
  in_file->database = 0;
  memset(&(in_file->user), 0, sizeof(in_file->user));
  in_file->num_recs = 0;
  in_file->next_rec = 0;
  in_file->records_read = 0;
  in_file->records_written = 0;

  /* Debug */
  debug_message("Leaving hotsync_init_read\n");

  return TRUE;
}


/* Initialize write data structure */
int
hotsync_init_write (struct hotsync_file_data * out_file)
{

  /* Debug */
  debug_message("Entering hotsync_init_write\n");

  /* Init own data structure */
  out_file->filename = hotsync_get_port();
  out_file->file_is_open = FALSE;
  out_file->socket = 0;
  out_file->database = 0;
  memset(&(out_file->user), 0, sizeof(out_file->user));
  out_file->num_recs = 0;
  out_file->next_rec = 0;
  out_file->records_read = 0;
  out_file->records_written = 0;

  /* Debug */
  debug_message("Leaving hotsync_init_write\n");

  return TRUE;
}


/* Destroy read data structure */
void
hotsync_exit_read (struct hotsync_file_data * in_file)
{

  /* Debug */
  debug_message("Entering hotsync_exit_read\n");

  /* Free memory */
  if (in_file->filename
      && in_file->filename != port)
    free (in_file->filename);

  /* Debug */
  debug_message("Leaving hotsync_exit_read\n");
}


/* Destroy write data structure */
void
hotsync_exit_write (struct hotsync_file_data * out_file)
{

  /* Debug */
  debug_message("Entering hotsync_exit_write\n");

  /* Free memory */
  if (out_file->filename
      && out_file->filename != port)
    free (out_file->filename);

  /* Debug */
  debug_message("Leaving hotsync_exit_write\n");
}


/* Set read command line option */
int
hotsync_set_read_option (struct hotsync_file_data * in_file, char opt, char * opt_arg)
{
  int rc = FALSE;


  /* Debug */
  debug_message("Entering hotsync_set_read_option\n");

  switch (opt)
    {
    case 'f':
      /* Filename */
      if (opt_arg != NULL
	  && *opt_arg != '\0') {
	if (in_file->filename
	    && in_file->filename != port)
	  free (in_file->filename);
	in_file->filename = strdup(opt_arg);
	rc = TRUE;
      }
      else {
	/* Can not read from stdin for hotsync file format */
	error_message("Can not read from stdin for read fileformat <%s>.\n",
		      dataformat2txt(DATA_FORMAT_HOTSYNC));
      }
      break;
    default:
      fprintf(stderr, "Can not process read option <%c> for input file\n",
	      opt);
    }

  /* Debug */
  debug_message("Leaving hotsync_set_read_option\n");

  return rc;
}


/* Set write command line option */
int
hotsync_set_write_option (struct hotsync_file_data * out_file, char opt, char * opt_arg)
{
  int rc = FALSE;


  /* Debug */
  debug_message("Entering hotsync_set_write_option\n");

  switch (opt)
    {
    case 'f':
      /* Filename */
      if (opt_arg != NULL
	  && *opt_arg != '\0') {
	if (out_file->filename
	    && out_file->filename != port)
	  free (out_file->filename);
	out_file->filename = strdup(opt_arg);
	rc = TRUE;
      }
      else {
	/* Can not read from stdin for hotsync file format */
	error_message("Can not write to stdout for write fileformat <%s>.\n",
		      dataformat2txt(DATA_FORMAT_HOTSYNC));
      }
      break;
    default:
      fprintf(stderr, "Can not process write option <%c> for output file\n",
	      opt);
    }

  /* Debug */
  debug_message("Leaving hotsync_set_write_option\n");

  return rc;
}



/* For opening & closing */

/* Open input data file for reading */
void
hotsync_open_read (struct hotsync_file_data * in_file, struct header_data * header)
{

  /* Debug */
  debug_message("Entering hotsync_open_read\n");

  /* Need port name to open */
  if (!in_file->filename)
    error_message("Need to provide which port to use for hotsync.\n");

  /* Open data connection */
  hotsync_open (in_file, header, dlpOpenRead, in_file->filename);

  /* Read header data */
  hotsync_read_header (in_file, header);

  /* Debug */
  debug_message("Leaving hotsync_open_read\n");
}


/* Open output data file for writing */
void
hotsync_open_write (struct hotsync_file_data * out_file, struct header_data * header)
{

  /* Debug */
  debug_message("Entering hotsync_open_write\n");

  /* Need port name to open */
  if (!out_file->filename)
    error_message("Need to provide which port to use for hotsync.\n");

  /* Open data connection */
  hotsync_open (out_file, header, dlpOpenWrite, out_file->filename);

  /* Write header */
  hotsync_write_header (out_file, header);

  /* Debug */
  debug_message("Leaving hotsync_open_write\n");
}


/* Close input file at end of processing */
void
hotsync_close_read (struct hotsync_file_data * in_file, struct header_data * header)
{
  char buffer[0xffff];


  /* Debug */
  debug_message("Entering hotsync_close_read\n");

  /* Format log message */
  snprintf(buffer, sizeof(buffer)-1,
	   "pilot-datebook: Read %d appointments from Pilot.\n",
	   in_file->records_read);

  /* Close the database */
  hotsync_close (in_file, buffer, TRUE);

  /* Update statistics */

  /* clear data structures */
  in_file->num_recs = 0;
  in_file->next_rec = 0;

  /* Debug */
  debug_message("Leaving hotsync_close_read\n");
}


/* Close output file at end of processing */
void
hotsync_close_write (struct hotsync_file_data * out_file, struct header_data * header)
{
  char buffer[0xffff];


  /* Debug */
  debug_message("Entering hotsync_close_write\n");

  /* Format log message */
  snprintf(buffer, sizeof(buffer)-1,
	   "pilot-datebook: Wrote %d appointments to Pilot.\n",
	   out_file->records_written);

  /* Close the database */
  hotsync_close (out_file, buffer, TRUE);

  /* Update statistics */

  /* clear data structures */
  out_file->num_recs = 0;
  out_file->next_rec = 0;

  /* Debug */
  debug_message("Leaving hotsync_close_write\n");
}


/* Close input file in case of an error */
void
hotsync_abort_read (struct hotsync_file_data * in_file)
{
  char buffer[0xffff];


  /* Debug */
  debug_message("Entering hotsync_abort_read\n");

  /* Format log message */
  snprintf(buffer, sizeof(buffer)-1,
	   "pilot-datebook: Read from Pilot was interrupted after reading %d appointments.\n",
	   in_file->records_read);

  /* Close the database */
  hotsync_close (in_file, buffer, FALSE);

  /* No special error processing needed */

  /* Update statistics */

  /* Clear data structures */
  in_file->num_recs = 0;
  in_file->next_rec = 0;

  /* Debug */
  debug_message("Entering hotsync_abort_read\n");
}


/* Close output file in case of an error */
void
hotsync_abort_write (struct hotsync_file_data * out_file)
{
  char buffer[0xffff];


  /* Debug */
  debug_message("Entering hotsync_abort_write\n");

  /* Format log message */
  snprintf(buffer, sizeof(buffer)-1,
	   "pilot-datebook: Write to Pilot was interrupted after writing %d appointments.\n",
	   out_file->records_written);

  /* Close the database */
  hotsync_close (out_file, buffer, FALSE);

  /* Provide info message to user that data may be in inconsistent state */
  info_message("Hotsync has been interrupted, data may or may not be stored on Pilot\n");

  /* Update statistics */

  /* Clear data structures */
  out_file->num_recs = 0;
  out_file->next_rec = 0;

  /* Debug */
  debug_message("Leaving hotsync_abort_write\n");
}



/* For reading */

/* Identify end of file */
int
hotsync_read_eof (struct hotsync_file_data * in_file)
{
  /* Debug */
  debug_message("Entering hotsync_read_eof\n");

  /* have to compare greater or equal, because first record number is 0 */
  return (in_file->next_rec >= in_file->num_recs);

  /* Debug */
  debug_message("Leaving hotsync_read_eof\n");
}


/* Read an appointment record */
void
hotsync_read_row (struct hotsync_file_data * in_file, struct row_data * row)
{

  /* Debug */
  debug_message("Entering hotsync_read_row\n");

  /* Read record */
  hotsync_read_specific_row (in_file, row, in_file->next_rec);

  if (getRowIsValid(row)) {
    /* Increment next record number */
    in_file->next_rec++;

    /* Update statistics */
    in_file->records_read++;
  }

  /* Debug */
  debug_message("Leaving hotsync_read_row\n");
}




/* For writing */

/* Write an appointment record */
void
hotsync_write_row (struct hotsync_file_data * out_file, struct header_data * header, struct row_data * row)
{
  struct Appointment a;
  int attributes;
  int category;
  unsigned long uid;
  unsigned long new_uid;
  int record_num;

  unsigned char buffer [0xffff];
  unsigned int buffer_len = 0;
  

  /* Debug */
  debug_message("Entering hotsync_write_row\n");

  if (!getRowIsValid(row))
    error_message("Can not write invalid row.\n");

  /* Get datebook row data */
  /* Use output record number instead of input record number
   * record_num = getRowRecordNum(row);
   */
  record_num = out_file->next_rec;
  uid = getRowUid(row);
  attributes = getRowAttributes(row);
  category = getRowCategory(row);
  a = getRowAppointment(row);


  /* Write datebook row data */
  buffer_len = pack_Appointment(&a, buffer, sizeof(buffer));
  new_uid = 0;
  if (dlp_WriteRecord (out_file->socket, out_file->database,
		       attributes, uid, category,
		       buffer, buffer_len, 
		       &new_uid) < 0)
    error_message("Write of datebook application row %d to output file failed!\n\n",
		  record_num);

  /* Update row data with new uid, if uid has changed */
  if (new_uid > 0) {
    info_message("Received new uid <%lu> for output record <%d>\n",
		 new_uid, record_num);
    setRowUid(row, new_uid);
  }


  /* Increment next record number */
  out_file->next_rec++;
  /* No update to num_recs, because we don't really know how many records
     exist on Pilot */

  /* Update statistics */
  out_file->records_written++;

  /* Debug */
  debug_message("Leaving hotsync_write_row\n");
}



/* For statistics */

/* Show input statistics */
void
hotsync_show_read_statistics (struct hotsync_file_data * in_file)
{
  /* Debug */
  debug_message("Entering hotsync_show_read_statistics\n");

  info_message("Input file <%s>, format <%s>:\n",
	       (in_file->filename) ? in_file->filename : "stdin",
	       dataformat2txt(DATA_FORMAT_HOTSYNC));
  info_message("Records read: %d\n",
	       in_file->records_read);

  /* Debug */
  debug_message("Leaving hotsync_show_read_statistics\n");
}


/* Show output statistics */
void
hotsync_show_write_statistics (struct hotsync_file_data * out_file)
{
  /* Debug */
  debug_message("Entering hotsync_show_write_statistics\n");

  info_message("Output file <%s>, format <%s>:\n",
	       (out_file->filename) ? out_file->filename : "stdout",
	       dataformat2txt(DATA_FORMAT_HOTSYNC));
  info_message("Records written: %d\n",
	       out_file->records_written);

  /* Debug */
  debug_message("Leaving hotsync_show_write_statistics\n");
}



/* Private functions */

/* This isn't declared anywhere ... copying what pilot-xfer does. */
int pilot_connect(const char *port);

/* Open hotsync data connection for reading or writing */
void
hotsync_open (struct hotsync_file_data * file, struct header_data * header, enum dlpOpenFlags open_flags, char * device)
{
  /* pilot_connect() does everything up through here, returns file->socket */
  file->socket = pilot_connect(file->filename);
  if (file->socket < 0) {
    dlp_AddSyncLogEntry(file->socket, "Unable to open conduit.\n");
    pi_close(file->socket);
    error_message("Unable to open conduit %s\n", file->filename);
  }

#if 0
  /* Read info header */
  /* Want to read it before opening database for accuracy
     (database will have open flag otherwise) and safety */
  if (dlp_FindDBInfo(file->socket, 0, 0, HOTSYNC_DATEBOOK_DB_NAME,
		     0, 0, &(header->info)) < 0)
    error_message("Can not get database header info for %s\n",
                  HOTSYNC_DATEBOOK_DB_NAME);

  if ((header->info).flags & dlpDBFlagResource)
    error_message("Input file is not a Datebook file, resource flag is set!\n\n");

#endif

  /* Open the Datebook's database, store access handle in db */
  file->database = 0;
  if(dlp_OpenDB(file->socket, 0, open_flags, HOTSYNC_DATEBOOK_DB_NAME,
		&(file->database)) < 0) {
    dlp_AddSyncLogEntry(file->socket, "Unable to open datebook database.\n");
    pi_close(file->socket);
    error_message("Unable to open datebook database%s\n", file->filename);
  }

  /* Now connection is open */
  file->file_is_open = TRUE;


  /* Initialize record numbers like an empty file,
   * to detect reading without calling read_header first..
   */
  file->num_recs = 0;
  file->next_rec = 0;

  /* Initialize statistics */
  file->records_read = 0;
  file->records_written = 0;

  /* Debug */
  debug_message("Leaving hotsync_open\n");
}


/* Close hotsync data connection for reading or writing */
void
hotsync_close (struct hotsync_file_data * file, char * log_message, int successSync)
{

  /* Debug */
  debug_message("Entering hotsync_close\n");

  if (file->file_is_open == TRUE) {
    /* Close database */
    dlp_CloseDB(file->socket, file->database);
    file->database = 0;

    /* If successful sync, then update last sync time */
    if (successSync) {
      /* Tell the user who it is, with a different PC id. */

      /* for the moment treat interaction with this program as
	 non-backup specific => commented out:

	 U.lastSyncPC = 0x00010000;
	 U.successfulSyncDate = time(NULL);
	 U.lastSyncDate = U.successfulSyncDate;
	 dlp_WriteUserInfo(sd,&U);
      */
    }

    /* Log entry */
    dlp_AddSyncLogEntry(file->socket, log_message);

    /* Close connection to Pilot */
    dlp_EndOfSync(file->socket,0);
    pi_close(file->socket);
    file->socket = 0;

    file->file_is_open = FALSE;
  }

  /* Debug */
  debug_message("Leaving hotsync_close\n");
}



/* Read datebook file header */
void
hotsync_read_header (struct hotsync_file_data * in_file, struct header_data * header)
{
  unsigned char buffer[0xffff];


  /* Debug */
  debug_message("Entering hotsync_read_header\n");

  /* Skip reading header, if header has already been read */
  if (header->isValid) {
    info_message("Skip reading header, since header has already been read.\n");
  }
  else {
    /* Read number of records */
    if (dlp_ReadOpenDBInfo (in_file->socket, in_file->database, &in_file->num_recs) < 0)
      error_message("Can not read number of records via hotsync\n\n");


    /* Initialize current record */
    in_file->next_rec = 0;


    /* Read datebook application header data */
    header->app_info_size = dlp_ReadAppBlock (in_file->socket,
					      in_file->database,
					      0,
					      buffer,
					      sizeof(buffer));
    if (header->app_info_size < 0)
      error_message("Can not get application information header data from input file\n\n");


    /* Convert datebook application header data */
    if (header->app_info_size > 0)
      unpack_AppointmentAppInfo(&(header->aai),
				buffer,
				header->app_info_size);


    /* Read datebook sort header data */
    /* Workaround: ReadSortBlock does not properly handle the case where a
     * sort info block is empty, and will crash with segmentation fault
     * before pilot-link version 0.9.5.
     */
    header->sort_info_size = dlp_ReadSortBlock (in_file->socket,
						in_file->database,
						0,
						NULL,
						sizeof(buffer));
    if (header->sort_info_size > 0) {
      header->sort_info_size = dlp_ReadSortBlock (in_file->socket,
						  in_file->database,
						  0,
						  buffer,
						  sizeof(buffer));
    }

    /* No error if sort info can not be read, assuming empty sort info */ 
    if (header->sort_info_size <= 0) {
      header->sort_info = NULL;
      header->sort_info_size = 0;
    }
    else {
      /* Will create a memory leak if it happens, since currently header is not
       * freed after usage
       */
      header->sort_info = malloc(header->sort_info_size);
      if (header->sort_info == NULL)
	error_message("Malloc for sort information failed!\n");
      memcpy(header->sort_info, buffer, header->sort_info_size);
    }

    /* If we reach here, we have successfully read a header.
     * Any failure would have caused a fatal error, since if any reading with
     * hotsync fails, then very likely the rest of hotsync will also fail.
     */
    header->isValid = TRUE;
  } /* if header has already been read */

  /* Debug */
  debug_message("Leaving hotsync_read_header\n");
}


/* Directly read a specific record, providing the record number */
void
hotsync_read_specific_row (struct hotsync_file_data * in_file, struct row_data * row, int record_num)
{
  unsigned char buffer[0xffff];
  int buffer_len = 0;

  struct Appointment a;
  int attributes;
  int category;
  unsigned long uid;


  /* Debug */
  debug_message("Entering hotsync_read_specific_row\n");

  /* Check whether record_num is out of bound */
  /* Have to compare greater or equal, because first record number is 0 */
  if (record_num >= in_file->num_recs)
    error_message("Record number %d exceeds maximum number of records %d\n\n",
		  record_num, in_file->num_recs);

  /* Read from file */
  if (dlp_ReadRecordByIndex(in_file->socket, in_file->database,
			    record_num, buffer, &uid, &buffer_len,
			    &attributes, &category) < 0) {
    warn_message("Can not read record <%d> from input file\n\n", record_num);
    setRowIsValid(row, FALSE);
  }
  else {
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
  }





  /* Check whether pack will bring same result */
  /*
  {
    unsigned char buffer2[0xffff];
    int buffer_len2 = 0;
    int i;

    buffer_len2 = pack_Appointment(&a, buffer2, sizeof(buffer2));
    if (buffer_len != buffer_len2) {
    info_message("Warning: input record <%d> (uid=%lu): pack buffer length differs!\n",
    record_num, uid);
    }
    if (memcmp(buffer, buffer2, buffer_len) != 0) {
    info_message("Warning: input record <%d> (uid=%lu): pack buffer content differs!\n",
    record_num, uid);
    fprintf(stderr, "Bytes which differ:");
    for (i=0; i<buffer_len; i++) {
    if (((unsigned char *)buffer)[i] != buffer2[i])
    fprintf(stderr, " %d", i);
    }
    info_message("\nOriginal buffer:\n");
    write_dump (stderr, buffer, buffer_len);
    info_message("Re-packed buffer:\n");
    write_dump (stderr, buffer2, buffer_len2);
    info_message("\n");
    }
  }
  */

  /* Debug */
  debug_message("Leaving hotsync_read_specific_row\n");
}


/* Write file header */
void
hotsync_write_header (struct hotsync_file_data * out_file, struct header_data * header)
{
  struct DBInfo * ip;
  struct AppointmentAppInfo * aai;
  void * sort_info;
  int sort_info_size;

  unsigned char buffer[0xffff];
  unsigned int buffer_len = 0;



  /* Debug */
  debug_message("Entering hotsync_write_header\n");

  if (header->isValid) {
    /* Get datebook header data */
    ip = &(header->info);
    aai = &(header->aai);
    sort_info = header->sort_info;
    sort_info_size = header->sort_info_size;


    /* Set datebook application information */
    if (header->app_info_size > 0)
      buffer_len = pack_AppointmentAppInfo (aai,
					    buffer,
					    sizeof(buffer));
    else
      buffer_len = 0;

    if(buffer_len < 0)
      error_message("Datebook application info for output file could not be packed!\n\n");

    if (dlp_WriteAppBlock (out_file->socket,
			   out_file->database,
			   buffer,
			   buffer_len))
      error_message("Can not set datebook application info for output file!\n\n");


    /* Set database sort information */
    if (dlp_WriteSortBlock (out_file->socket,
			    out_file->database,
			    sort_info,
			    sort_info_size))
      error_message("Can not set datebook sort info for output file!\n\n");
  }
  else {
    debug_message("No valid header present => skip writing header.\n");
  }

  /* Debug */
  debug_message("Leaving hotsync_write_header\n");
}
