/* ex: set tabstop=4 expandtab: */
/*
 * read-palmpix.c:  PalmPix image convertor
 *
 * Copyright 2001 John Marshall <jmarshall@acm.org>
 * Copyright 2002-2004 Angus Ainslie <angusa@deltatee.com>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "pi-file.h"
#include "pi-socket.h"
#include "pi-header.h"
#include "pi-palmpix.h"

#include "userland.h"

#ifdef HAVE_PNG
#include "png.h"
#if (PNG_LIBPNG_VER < 10201)
 #define png_voidp_NULL (png_voidp)NULL
 #define png_error_ptr_NULL (png_error_ptr)NULL
#endif
#endif

const char *progname;

/***********************************************************************
 *
 * Function:    PalmPixState_pi_file
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
struct PalmPixState_pi_file
{
	struct PalmPixState state;
	struct pi_file *f;
};


/***********************************************************************
 *
 * Function:    getrecord_pi_file
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int getrecord_pi_file (struct PalmPixState *vstate, int recno,
	void **buf, size_t *bufsize)
{

	struct PalmPixState_pi_file *state =
		(struct PalmPixState_pi_file *) vstate;

	return pi_file_read_record (state->f, recno, buf, bufsize, NULL,
		NULL, NULL) < 0 ? -1 : 0;
}


/***********************************************************************
 *
 * Function:    PalmPixState_pi_socket
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
struct PalmPixState_pi_socket
{
	struct PalmPixState state;
	int 	sd,
		db;
};


/***********************************************************************
 *
 * Function:    getrecord_pi_socket
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int getrecord_pi_socket (struct PalmPixState *vstate, int recno,
	void **buf, size_t *bufsize)
{

	static char buffer[65536];
    static pi_buffer_t fakebuf;

	struct PalmPixState_pi_socket *state =
		(struct PalmPixState_pi_socket *) vstate;

	*buf = buffer;
    fakebuf.data = buffer;
    fakebuf.allocated = 0;
    fakebuf.used = 0;

	return dlp_ReadRecordByIndex (state->sd, state->db, recno, &fakebuf,
		NULL, NULL, NULL) < 0 ? 1 : 0;
}


/***********************************************************************
 *
 * Function:    fmt_date
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static const char *fmt_date (const struct PalmPixHeader *h)
{
	static char buf[24];

	sprintf (buf, "%d-%02d-%02d %02d:%02d:%02d", h->year, h->month,
		h->day, h->hour, h->min, h->sec);

	return buf;
}


/***********************************************************************
 *
 * Function:    init_for_ppm
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void init_for_ppm (struct PalmPixState *state)
{
	state->offset_r = 0;
	state->offset_g = 1;
	state->offset_b = 2;
}


/***********************************************************************
 *
 * Function:    write_ppm
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void write_ppm (FILE *f, const struct PalmPixState *state,
	const struct PalmPixHeader *header)
{
	fprintf (f, "P6\n# %s (taken at %s)\n%d %d\n255\n",
		state->pixname, fmt_date (header), header->w, header->h);

	fwrite (state->pixmap, header->w * header->h * 3, 1, f);
}


/***********************************************************************
 *
 * Function:    write_png
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
#ifdef HAVE_PNG
void write_png( FILE *f, const struct PalmPixState *state,
	        const struct PalmPixHeader *header)
{
	int i;
	png_structp png_ptr;
	png_infop info_ptr;

	png_ptr = png_create_write_struct
		( PNG_LIBPNG_VER_STRING, png_voidp_NULL,
		png_error_ptr_NULL, png_error_ptr_NULL);

	if(!png_ptr)
		return;

	info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		return;
	}

	if( setjmp( png_jmpbuf(png_ptr))) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(f);
		return;
	}

	png_init_io(png_ptr, f);

	png_set_IHDR(png_ptr, info_ptr, header->w, header->h,
		8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);

	png_write_info( png_ptr, info_ptr );

	for(i=0; i<header->h; i++) {
		png_write_row(png_ptr, &state->pixmap[i*header->w*3]);
		png_write_flush(png_ptr);
	}

	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);

}
#endif




/***********************************************************************
 *
 * Function:    write_one
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int write_one (const struct PalmPixHeader *header,
	struct PalmPixState *state, int recno, const char *pixname)
{

	if (unpack_PalmPix (state, header, recno, pixName) != 0) {

	init_for_ppm (state);
	if (strcmp (state->pixname, pixname) == 0
		&& unpack_PalmPix (state, header, recno,
		pixName | pixPixmap) != 0) {
#ifdef HAVE_PNG
		if( state->output_type == PALMPIX_OUT_PPM )
			write_ppm( stdout, state, header);

		else if( state->output_type == PALMPIX_OUT_PNG )
			write_png( stdout, state, header);
#else
		write_ppm (stdout, state, header);
#endif
		free_PalmPix_data (state);
		}

		recno = state->highest_recno;
	}
	return recno;
}


/***********************************************************************
 *
 * Function:    write_all
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int write_all (const struct PalmPixHeader *header,
	struct PalmPixState *state, int recno, const char *ignored)
{
	char fname[FILENAME_MAX], ext[10];
	FILE *f;

	init_for_ppm (state);
	if (!unpack_PalmPix (state, header, recno, pixName | pixPixmap)) {
		/* bail */
		return recno;
	}


	sprintf( fname, "%s", state->pixname );

	if( state->output_type == PALMPIX_OUT_PPM )
		sprintf( ext, "_pp.ppm" );

	else if( state->output_type == PALMPIX_OUT_PNG )
		sprintf( ext, "_pp.png" );

	if (plu_protect_files( fname, ext, sizeof(fname) ) < 1) {
		return recno;
	}

	printf ("Generating %s...\n", fname);

	f = fopen (fname, "wb");
	if (f) {
#ifdef HAVE_PNG
		if( state->output_type == PALMPIX_OUT_PPM )
			write_ppm(f, state, header);

		else if( state->output_type == PALMPIX_OUT_PNG )
			write_png(f, state, header);
#else
			write_ppm (f, state, header);
#endif
			fclose (f);
	} else {
		fprintf (stderr, "%s: can't write to %s\n",
			progname, fname);
	}

		free_PalmPix_data (state);
		recno = state->highest_recno;

	return recno;
}


/***********************************************************************
 *
 * Function:    list
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int list (const struct PalmPixHeader *h, struct PalmPixState *state,
	int recno, const char *ignored)
{
	if (unpack_PalmPix (state, h, recno, pixName) != 0) {

		printf ("%d x %d\t%d\t%s\t%s\n",
			h->w, h->h, h->num, fmt_date (h), state->pixname);
		recno = state->highest_recno;
	}
	return recno;
}


/***********************************************************************
 *
 * Function:    read_db
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void read_db (struct PalmPixState *state, int n, int (*action)
	(const struct PalmPixHeader *, struct PalmPixState *,
	int, const char *), const char *action_arg)
{
	int i;

	for (i = 0; i < n; i++) {
		void *buffer;
		size_t bufsize;
		struct PalmPixHeader header;

		if (state->getrecord (state, i, &buffer, &bufsize) == 0
			&& unpack_PalmPixHeader (&header, buffer, bufsize) != 0)

		i = action (&header, state, i, action_arg);
	}
}

static int fail (const char *func) {
	perror (func);
	return -1;
}


/***********************************************************************
 *
 * Function:    display_help
 *
 * Summary:     Print out the --help options and arguments
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void display_help(const char *progname)
{
	printf("   Convert all pictures in the files given, or found via connecting to a\n");
	printf("   Palm handheld if no files are given, writing each to <pixname>.ppm\n\n");
	printf("   Usage: %s [-p port] [-l | -n pixname] [file]...\n\n", progname);
	printf("   Options:\n");
	printf("     -p, --port <port>       Use device file <port> to communicate with Palm\n");
    printf("     -b, --bias [num],       lighten or darken the image\n");
    printf("                             0..50   darken image\n");
    printf("                             50..100 lighten image\n");
	printf("     -c, --colour            Do a simple colour correction\n");
	printf("     -h, --help              Display help information for %s\n", progname);
    printf("     -l, --list,             List picture information instead of converting\n");
	printf("     -n, --name [name]       Convert only <name>, and output to STDOUT as type\n\n");
	printf("     -s, --stretch,          Do a histogram stretch on the colour planes\n");
	printf("     -t, --type,             Specify picture output type (ppm or png)\n");
	printf("     -v, --version           Display %s version information\n", progname);

	return;
}


int main (int argc, const char **argv) {
	int 	c, 	/* switch */
	sd	= -1,
	output_type = PALMPIX_OUT_PPM,
	bias = 50,
	flags = 0;

	int (*action) (const struct PalmPixHeader *, struct PalmPixState *,
		int, const char *) = write_all;

	const 	char *pixname 	= NULL;
	char 	*port 		= NULL,
	    	*file_arg	= NULL,
	    	*type_str	= NULL;
	struct 	PilotUser User;

	char *progname = argv[0];

	poptContext pc;

	struct poptOption options[] = {
		{"port", 'p', POPT_ARG_STRING, &port, 0,
		 "Use device file <port> to communicate with Palm", "port"},
		{"help", 'h', POPT_ARG_NONE, NULL, 'h',
		 "Display help information", NULL},
		{"version", 'v', POPT_ARG_NONE, NULL, 'v',
		 "Show program version information", NULL},
		{"stretch", 's', POPT_ARG_VAL | POPT_ARGFLAG_OR, &flags,
		 PALMPIX_HISTOGRAM_STRETCH,
		 "Do a histogram stretch on the colour planes", NULL},
		{"colour", 'c', POPT_ARG_VAL | POPT_ARGFLAG_OR, &flags,
		 PALMPIX_COLOUR_CORRECTION,
		 "Do a simple colour correction", NULL},
		{"type", 't', POPT_ARG_STRING, &type_str, 't',
		 "Specify picture output type (ppm or png)", "[ppm|png]"},
		{"bias", 'b', POPT_ARG_INT, &bias, 'b',
		 "lighten or darken the image (0..50 darken, 50..100 lighten)", "bias"},
		{"list", 'l', POPT_ARG_NONE, NULL, 'l',
		 "List picture information instead of converting", NULL},
		{"name", 'n', POPT_ARG_STRING, &pixname, 'n',
		 "Convert only <name>, and output to STDOUT as type", "name"},
		POPT_TABLEEND
	};

	pc = poptGetContext("read-palmpix", argc, argv, options, 0);

	while ((c = poptGetNextOpt(pc)) >= 0) {
	        switch (c) {

        	case 'h':
        		display_help(progname);
	        	return 0;
        	case 'v':
	        	print_splash(progname);
		        return 0;
        	case 'b':
			if( bias < 0 || bias > 100 ) {
					fprintf( stderr, "Bad bias\n" );
					exit( EXIT_FAILURE );
				 }
		        break;
        	case 'n':
	        	action = write_one;
        		break;
        	case 'l':
	        	action = list;
        		break;
	        case 't':
		        if( !strncmp( "png", type_str, 3 )) {
#ifdef HAVE_PNG
        			output_type = PALMPIX_OUT_PNG;
#else
	        		fprintf( stderr, "read-palmpix was built without png support\n" );
#endif
		        } else if( !strncmp( "ppm", type_str, 3 )) {
			        output_type = PALMPIX_OUT_PPM;
        		} else {
        			fprintf( stderr, "Unknown output type defaulting to ppm\n" );
        			output_type = PALMPIX_OUT_PPM;
        		}
        		break;
	        default:
        		display_help(progname);
	        	return 0;
                }
        }

	if (c < -1) {
		/* an error occurred during option processing */
		fprintf(stderr, "%s: %s\n",
		    poptBadOption(pc, POPT_BADOPTION_NOALIAS),
		    poptStrerror(c));
		return 1;
	}

	if(poptPeekArg(pc) != NULL) {
		int i = 0;

		while((file_arg = poptGetArg(pc)) != NULL) {
			i++;
			struct pi_file *f = pi_file_open (file_arg);
			if (f) {

				struct DBInfo info;
				if ((poptPeekArg(pc) != NULL || i > 1)
				    && action != write_one)
					printf ("%s:\n", file_arg);

				pi_file_get_info (f, &info);
                if (info.flags & dlpDBFlagResource) {

				struct PalmPixState_pi_file s;
				int n = 0;

				s.state.output_type = output_type;

				pi_file_get_entries (f, &n);
				s.state.getrecord = getrecord_pi_file;
				s.f = f;
				read_db (&s.state, n, action, pixname);
				} else {
				fprintf (stderr,
					"%s: %s is not a valid record database\n",
					    progname, file_arg);
				}
				pi_file_close (f);
			} else {
				fprintf (stderr, "%s: can't open %s\n",
				    progname, file_arg);
			}
			}
	} else  {
		int db;

		sd = pilot_connect(port);
		if (sd < 0)
			return fail ("pi_socket");

		if (dlp_ReadUserInfo(sd, &User) < 0)
			return fail( "Read user info" );

		dlp_OpenConduit (sd);
		dlp_ReadUserInfo (sd, &User);

		if (dlp_OpenDB (sd, 0, dlpOpenRead, PalmPix_DB, &db) >= 0) {

			struct PalmPixState_pi_socket s;
			int n = 0;

			s.state.output_type = output_type;
		    s.state.bias = bias;
		    s.state.flags = flags;

			dlp_ReadOpenDBInfo (sd, db, &n);
			s.state.getrecord = getrecord_pi_socket;
			s.sd = sd;
			s.db = db;
			read_db (&s.state, n, action, pixname);
			dlp_CloseDB (sd, db);

			dlp_AddSyncLogEntry (sd,
				"Read PalmPix images from Palm.\n");

			User.lastSyncPC = 0x00010000;
			User.lastSyncDate = User.successfulSyncDate = time (NULL);
			dlp_WriteUserInfo (sd, &User);
		} else
			fprintf (stderr, "%s: can't open database %s\n",
				progname, PalmPix_DB);
			dlp_EndOfSync (sd, dlpEndCodeNormal);
			pi_close (sd);
	}
	return 0;
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
