/* 
 * pilot-foto.c: Palm 'Foto' Image Fetcher/Converter
 *
 * This is a palm conduit to fetch Foto files from a Palm.  It can also
 * convert *.jpg.pdb files that have already been fetched from the Palm.
 *
 * Copyright (C) 2003 by Judd Montgomery
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License.
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
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pi-dlp.h>
#include <pi-file.h>
#include <pi-socket.h>

/* Declare prototypes */
static void display_help(char *progname);
void print_splash(char *progname);
int pilot_connect(char *port);


#ifndef TRUE
# define TRUE 1
#endif
#ifndef FALSE
# define FALSE 0
#endif


/***********************************************************************
 *
 * Function:    display_help
 *
 * Summary:     Uh, the -help, of course
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void display_help(char *progname)
{
	printf("   Decodes Palm 'Foto' Image files to jpg files you can read\n\n");
	printf("   Usage: %s -p <port> [options] file\n", progname);
	printf("   Options:\n");
	printf("     -p <port>         Use device file <port> to communicate with Palm\n");
	printf("     -h --help         Display this information\n");
	printf("     -v --version      Display version information\n");
	printf("     -f                fetch files from the Palm\n");
	printf("     -c [file]         convert [file].jpg.pdb files to jpg\n\n");
	printf("   Examples: \n");
	printf("      %s -p /dev/pilot -f MyImage.jpg.pdb\n", progname);
	printf("      %s -c MyImage.jpg.pdb\n\n", progname);
	return;
}


/***********************************************************************
 *
 * Function:    fetch_all_fotos
 *
 * Summary:     Grab all the pictures matching 'Foto' on the device
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int fetch_all_fotos(int sd)
{
	FILE *out;
	char buffer[65536];
	recordid_t id;
	int 	index,
		db,
		size,
		attr,
		category,
		ret,
		start;
	
	struct DBInfo info;
	char creator[5];

        start = 0;
	while (dlp_ReadDBList(sd, 0, dlpOpenRead, start, &info) > 0) {
                start = info.index + 1;
                creator[0] = (info.creator & 0xFF000000) >> 24;
                creator[1] = (info.creator & 0x00FF0000) >> 16;
                creator[2] = (info.creator & 0x0000FF00) >> 8;
                creator[3] = (info.creator & 0x000000FF);
                creator[4] = '\0';
                if (!strcmp(creator, "Foto")) {
                        printf("Fetching '%s' (Creator ID '%s')... ",
                               info.name, creator);
                        ret =
                            dlp_OpenDB(sd, 0, dlpOpenRead, info.name, &db);
                        if (ret < 0) {
                                fprintf(stderr, "Unable to open %s\n",
                                        info.name);
                                continue;
                        }

                        out = fopen(info.name, "w");
                        if (!out) {
                                fprintf(stderr,
                                        "Failed, unable to create file %s\n",
                                        info.name);
                                continue;
                        }

                        index = 0;
                        ret = 1;
                        while (ret > 0) {
                                ret =
                                    dlp_ReadRecordByIndex
                                    PI_ARGS((sd, db, index, buffer, &id,
                                             &size, &attr, &category));
                                index++;
                                if ((ret > 0) && (size > 8)) {
                                        fwrite(buffer + 8, size - 8, 1,
                                               out);
                                }
                        }
                        dlp_CloseDB(sd, db);
                        fclose(out);
                        printf("OK\n");
                }
        }
        return 0;
}


/***********************************************************************
 *
 * Function:    do_fetch
 *
 * Summary:     Connect and fetch the file
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int do_fetch(char *port)
{
        int     sd              = -1,
		ret;   

        sd = pilot_connect(port);

	ret = fetch_all_fotos(sd);

	dlp_EndOfSync(sd, dlpEndCodeNormal);
	pi_close(sd);

	return 0;
}


/***********************************************************************
 *
 * Function:    pdb_to_jpg
 *
 * Summary:     Do the bulk of the conversion here
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pdb_to_jpg(char *filename)
{
        struct pi_file *pi_fp;
        struct DBInfo info;
        FILE *out;
        int index;
        int ret;
        void *Pbuf;
        int size;
        int total_size;
        int attr;
        int cat;
        pi_uid_t uid;

        printf("converting %s... ", filename);
        pi_fp = pi_file_open(filename);
        if (!pi_fp) {
                printf("FAILED: could not open %s\n", filename);
                return -1;
        }
        pi_file_get_info(pi_fp, &info);

        out = fopen(info.name, "w");
        if (!out) {
                printf("FAILED: could not open %s to write\n", info.name);
                return -1;
        }

        index = 0;
        total_size = 0;
        ret = 1;
        while (ret >= 0) {
                ret =
                    pi_file_read_record(pi_fp, index, &Pbuf, &size, &attr,
                                        &cat, &uid);
                index++;
                if ((ret >= 0) && (size > 8)) {
                        fwrite(Pbuf + 8, size - 8, 1, out);
                        total_size += size - 8;
                }
        }
        fclose(out);

        printf("OK, wrote %d bytes to %s\n", total_size, info.name);
        return 0;
}


int main(int argc, char *argv[])
{
        int i;
        char *port;
        int fetch;
        int convert;
        int convert_argc;

        char    *progname       = argv[0];

        port = NULL;
        convert_argc = 0;
        fetch = convert = FALSE;

        if (argc == 1) {
                display_help(progname);
        }

        for (i = 1; i < argc; i++) {
                if ((strcmp(argv[1], "-?") == 0) ||
                    (strcmp(argv[1], "-help") == 0) ||
                    (strcmp(argv[1], "-h") == 0) ||
                    (strcmp(argv[1], "--help") == 0)) {
                        display_help(progname);
                        exit(0);
                }
                if (!strncasecmp(argv[i], "-p", 2)) {
                        if (argc < i + 2) {
                                display_help(progname);
                                exit(0);
                        }
                        port = argv[++i];
                }
                if (!strncasecmp(argv[i], "-f", 2)) {
                        fetch = TRUE;
                }
                if (!strncasecmp(argv[i], "-c", 2)) {
                        convert = TRUE;
                        convert_argc = i + 1;
                }
        }

        if (fetch) {
                do_fetch(port);
                return (0);
        }

        if (convert) {
                for (i = convert_argc; i < argc; i++) {
                        pdb_to_jpg(argv[i]);
                }
                printf("\n");
                return 0;
        }

        printf("%s: Nothing to do!\n", argv[0]);
        return 0;
}
