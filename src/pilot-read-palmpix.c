/* 
 * read-palmpix.c:  PalmPix image convertor
 *
 * Copyright 2001 John Marshall <jmarshall@acm.org>
 * Copyright 2002 Angus Ainslie <angusa@deltatee.com>
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pi-file.h"
#include "pi-socket.h"
#include "pi-header.h"
#include "pi-palmpix.h"

#include "getopt.h"

const char *progname;
int pilot_connect(const char *port);

void write_ppm (FILE *f, const struct PalmPixState *state, const struct PalmPixHeader *header);
void init_for_ppm (struct PalmPixState *state);
void read_db (struct PalmPixState *state, int n, int (*action) (const struct PalmPixHeader *, struct PalmPixState *, int, const char *), const char *action_arg);

struct PalmPixState_pi_file
{
   struct PalmPixState state;
   struct pi_file *f;
};

static int
  getrecord_pi_file (struct PalmPixState *vstate, int recno,
		     void **buf, int *bufsize)
{
 
   struct PalmPixState_pi_file *state =
     (struct PalmPixState_pi_file *) vstate;
   return pi_file_read_record (state->f, recno, buf, bufsize,
			       NULL, NULL, NULL);

}

struct PalmPixState_pi_socket
{
   
   struct PalmPixState state;
   int sd, db;
   
};

static int
  getrecord_pi_socket (struct PalmPixState *vstate, int recno,
		       void **buf, int *bufsize)
{
   
   static char buffer[65536];
   
   struct PalmPixState_pi_socket *state =
     (struct PalmPixState_pi_socket *) vstate;
   
   *buf = buffer;
   return !(dlp_ReadRecordByIndex (state->sd, state->db, recno, buffer,
				   NULL, bufsize, NULL, NULL) == *bufsize);
   
}

static const char *
  fmt_date (const struct PalmPixHeader *h)
{
   
   static char buf[24];
   sprintf (buf, "%d-%02d-%02d %02d:%02d:%02d",
	    h->year, h->month, h->day, h->hour, h->min, h->sec);
   return buf;
   
}

void
  init_for_ppm (struct PalmPixState *state)
{
   
   state->offset_r = 0;
   state->offset_g = 1;
   state->offset_b = 2;
   
}

void
  write_ppm (FILE *f, const struct PalmPixState *state,
	        const struct PalmPixHeader *header)
{
   
   fprintf (f, "P6\n# %s (taken at %s)\n%d %d\n255\n",
	    state->pixname, fmt_date (header), header->w, header->h);
   fwrite (state->pixmap, header->w * header->h * 3, 1, f);
   
}

static int
  write_one (const struct PalmPixHeader *header, struct PalmPixState *state,
	     int recno, const char *pixname)
{
   
   if (unpack_PalmPix (state, header, recno, pixName) != 0) 
     {
	
	init_for_ppm (state);
	if (strcmp (state->pixname, pixname) == 0
	    && unpack_PalmPix (state, header, recno,
			       pixName | pixPixmap) != 0) 
	  {
	     
	     write_ppm (stdout, state, header);
	     free_PalmPix_data (state);
	     
	  }
	
	recno = state->highest_recno;
	
     }
   
   return recno;
   
}

static int
  write_all (const struct PalmPixHeader *header, struct PalmPixState *state,
	     int recno, const char *ignored)
{
   
   init_for_ppm (state);
   if (unpack_PalmPix (state, header, recno, pixName | pixPixmap) != 0) 
     {
	
	char fname[FILENAME_MAX];
	FILE *f;
	
	/* FIXME ought to use something like protect_filename here */
	sprintf (fname, "%s.ppm", state->pixname);
	printf ("Generating %s...\n", fname);
	
	f = fopen (fname, "wb");
	if (f) 
	  {
	     
	     write_ppm (f, state, header);
	     fclose (f);
	     
	  }
	else
	  fprintf (stderr, "%s: can't write to %s\n",
		    progname, fname);
	
	free_PalmPix_data (state);
	recno = state->highest_recno;
	
     }
   
   return recno;
   
}

static int
  list (const struct PalmPixHeader *h, struct PalmPixState *state, int recno,
	const char *ignored)
{
   
   if (unpack_PalmPix (state, h, recno, pixName) != 0) 
     {
	
	printf ("%d x %d\t%d\t%s\t%s\n",
		h->w, h->h, h->num, fmt_date (h), state->pixname);
	recno = state->highest_recno;
	
     }
   
   return recno;
   
}

void
  read_db (struct PalmPixState *state, int n,
	   int (*action) (const struct PalmPixHeader *,
			  struct PalmPixState *, int, const char *),	
	   const char *action_arg)
{
   
   int i;
   for (i = 0; i < n; i++) 
     {
	void *buffer;
	int bufsize;
	struct PalmPixHeader header;
	
	if (state->getrecord (state, i, &buffer, &bufsize) == 0
	        && unpack_PalmPixHeader (&header, buffer, bufsize) != 0)
	  i = action (&header, state, i, action_arg);
	
     }
   
   
}


static const char shortopts[] = "ln:p:hv";

static struct option longopts[] = {
     {"name", required_argument, NULL, 'n' },
     {"list", no_argument, NULL, 'l' },
     {"help", no_argument, NULL, 'h' },     
     {"version", no_argument, NULL, 'v' },    
     {NULL, no_argument, NULL, 0}
};


static void print_help(char *progname) 
{
        printf("   Convert all pictures in the files given, or found via connecting to a\n"
	       "   Palm handheld if no files are given, writing each to <pixname>.ppm\n\n"
               "   Usage: %s [-p port] [-l | -n pixname] [file]...\n"
               "   Options:\n"
               "     -p <port>    Use device file <port> to communicate with Palm\n"
               "     -h           Display this information\n"
               "     --list, -l   List picture information instead of converting\n"
	       "     -n [name]    Convert only <pixname>, and output to stdout as .ppm\n\n", progname);
        return;
}



static int
  fail (const char *func)  
{
   perror (func);
   return EXIT_FAILURE;  
}


int
  main (int argc, char **argv) 
{
   int sd, c, longind, nfileargs;
   int (*action) (const struct PalmPixHeader *, struct PalmPixState *,
		  int, const char *) = write_all;
   const char *pixname = NULL;
   char *port = NULL;
   struct 	PilotUser U;
   
   char *progname = argv[0];
   
   while ((c = getopt_long (argc, argv, 
			    shortopts, longopts, &longind)) != EOF)
     {
	switch (c) 
	  {
	     
	   case 'l':
	     action = list;
	     break;
	     
	   case 'n':
	     action = write_one;
	     pixname = optarg;
	     break;
	     
	   case 'p':
	     port = optarg;
	     break;
	     
	   case 'h':
	     print_help(progname);
	     return EXIT_SUCCESS;
	     
	   case 'v':
	     print_splash(progname);
	     return EXIT_SUCCESS;
	     
	   default:
	     print_help(progname);
	     return EXIT_FAILURE;
	     
	  }
     }
   
  
   nfileargs = argc - optind - 1;

   if (nfileargs > 0) 
     {
	int i;

	for (i = optind; i < argc; i++) 
	  {
	     
	     struct pi_file *f = pi_file_open (argv[i]);
	     if (f) 
	       {
		  
		  struct DBInfo info;
		  if (nfileargs > 1 && action != write_one)
		    printf ("%s:\n", argv[i]);
		  
		  if (pi_file_get_info (f, &info) == 0
		      && ! (info.flags & dlpDBFlagResource)) 
		    {
		       
		       struct PalmPixState_pi_file s;
		       int n = 0;
		       pi_file_get_entries (f, &n);
		       s.state.getrecord = getrecord_pi_file;
		       s.f = f;
		       read_db (&s.state, n, action, pixname);
		       
		    }		  
		  else
		    fprintf (stderr,
			     "%s: %s is not a valid record database\n",
			     progname, argv[i]);
		  
		  pi_file_close (f);
		  
	       }
	     else
	       fprintf (stderr, "%s: can't open %s\n",
			progname, argv[i]);
	     
	    }
	  
	  
     }
   else 
     {
	int db;

        sd = pilot_connect(port);
	if (sd < 0)
	  return fail ("pi_socket");
	
	if (dlp_ReadUserInfo(sd, &U) < 0)
	  return fail( "Read user info" );
	       
	dlp_OpenConduit (sd);
	dlp_ReadUserInfo (sd, &U);
	
	if (dlp_OpenDB (sd, 0, dlpOpenRead, PalmPix_DB, &db) >= 0) 
	  {
	     
	     struct PalmPixState_pi_socket s;
	     int n = 0;
	     dlp_ReadOpenDBInfo (sd, db, &n);
	     s.state.getrecord = getrecord_pi_socket;
	     s.sd = sd;
	     s.db = db;
	     read_db (&s.state, n, action, pixname);
	     dlp_CloseDB (sd, db);
	     
	     dlp_AddSyncLogEntry (sd,
				  "Read PalmPix images from Palm.\n");
	     
	     U.lastSyncPC = 0x00010000;
	     U.lastSyncDate = U.successfulSyncDate = time (NULL);
	     dlp_WriteUserInfo (sd, &U);
	     
	  }
	else
	  fprintf (stderr, "%s: can't open database %s\n",
		   progname, PalmPix_DB);
	
	dlp_EndOfSync (sd, dlpEndCodeNormal);
	pi_close (sd);
	
     }
   
   
   return EXIT_SUCCESS;
   
}

