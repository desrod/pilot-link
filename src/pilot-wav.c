/* 
 * pilot-wav.c: Palm Voice Memo Wav Fetcher/Converter
 *
 * This is a palm conduit to fetch Memo Wav files from a Palm.  It can also
 * convert *.wav.pdb files that have already been fetched from the Palm.
 *
 * Copyright (C) 2003 by David Turner
 * Based on pilot-foto Copyright (C) 2003 by Judd Montgomery 
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
 
#include "getopt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pi-file.h"
#include "pi-source.h"

/* Declare prototypes */
static void display_help(char *progname);
void print_splash(char *progname);
int pilot_connect(char *port);
long write_header(FILE *out);
long write_data(char *buffer, int index, int size, long dataChunkSize, FILE *out);
int pdb_to_wav(char *filename);
int fetch_wavs(int sd, char *dbname);
int do_fetch(char *port, char *dbname);

struct option options[] = {
        {"port",        required_argument, NULL, 'p'},
        {"help",        no_argument,       NULL, 'h'},
        {"version",     no_argument,       NULL, 'v'},
        {"fetch",       required_argument, NULL, 'f'},
        {"convert",     required_argument, NULL, 'c'},
        {0,             0,                 0,    0}
};

static const char *optstring = "p:hvf:c:";

#ifndef TRUE
# define TRUE 1
#endif
#ifndef FALSE
# define FALSE 0
#endif


/***********************************************************************
 *
 * Function:    write_header
 *
 * Summary:     Writes out header of wave file with correct parameters
 *
 * Parameters:  File out - output file pointer
 *
 * Returns:     long - size of format chunk
 *
 ***********************************************************************/
long write_header(FILE * out)
{
        const char formatChunkID[4] = "fmt ";
        long formatChunkSize;

        short wFormatTag;
        unsigned short wChannels;
        unsigned long dwSamplesPerSec;
        unsigned long dwAvgBytesPerSec;
        unsigned short wBlockAlign;
        unsigned short wBitsPerSample;
        unsigned short cbSize;
        unsigned short wSamplesPerBlock;

        const char dataChunkID[4] = "data";
        long dataChunkSize;

        long wWaveLength;

	/* In bytes, not including formatChunkID and formatChunkSize itself */
        formatChunkSize = 20;

	/* 17 = Intel IMA (DVI) ADPCM */
        wFormatTag = 17;

	/* Mono */
        wChannels = 1;
        dwSamplesPerSec = 8000;

        /* This value varies with file - need formula */
        dwAvgBytesPerSec = 4425;
        wBlockAlign = 256;
        wBitsPerSample = 4;

	/* Extended format block size including(it appears) cbSize itself */
        cbSize = 2;
        wSamplesPerBlock = 505;

	/* fill in at end of file with seek */
        dataChunkSize = 0;

        /* fill in at end of file with seek */
        wWaveLength = 0;

        /* RIFF Header */
        fwrite("RIFF", 4, 1, out);
        fwrite(&wWaveLength, 4, 1, out);
        fwrite("WAVE", 4, 1, out);

        /* Format Chunk */
        fwrite(formatChunkID, 4, 1, out);

	/* Length of Format Chunk - 4 (fmt ) - 4 (length value itself) */
        fwrite(&formatChunkSize, 4, 1, out);
        fwrite(&wFormatTag, 2, 1, out);
        fwrite(&wChannels, 2, 1, out);
        fwrite(&dwSamplesPerSec, 4, 1, out);
        fwrite(&dwAvgBytesPerSec, 4, 1, out);
        fwrite(&wBlockAlign, 2, 1, out);
        fwrite(&wBitsPerSample, 2, 1, out);

        /* Extended Format Chunk Fields */

	/* Extended format block size including(it appears) cbSize itself */
        fwrite(&cbSize, 2, 1, out);
        fwrite(&wSamplesPerBlock, 2, 1, out);

        /* Data Chunk */
        fwrite(dataChunkID, 4, 1, out);
        fwrite(&dataChunkSize, 4, 1, out);

        return formatChunkSize;
}


/***********************************************************************
 *
 * Function:    write_data
 *
 * Summary:     Writes out data from record buffer updating dataChunkSize
 *
 * Parameters:  char pointer buffer - self explanatory
 *              int size - size of data to be written 
 *              FILE *out - pointer to output file    
 *              long dataChunkSize - present data chunk size
 *
 * Returns:     long - updated dataChunkSize
 *
 ***********************************************************************/
long write_data(char *buffer, int index, int size, long dataChunkSize, FILE *out)
{
	if (index == 0) {
		fwrite(buffer + 122, size - 122, 1, out);
		dataChunkSize += size - 122;
	} else {
		fwrite(buffer + 8, size - 8, 1, out);
		dataChunkSize += size - 8;
	}
	return dataChunkSize;
}

/***********************************************************************
 *
 * Function:    fetch_wavs
 *
 * Summary:     Grab the voice memos matching 'Vpad' on the device
 *
 * Parameters:  None
 *
 * Returns:     0
 *
 ***********************************************************************/
int fetch_wavs(int sd, char *dbname)
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
	char type[5];

        int booldb = TRUE;
	int reinit;
        
        long wWaveLength;
        long formatChunkSize;
        long dataChunkSize;

        start = 0;
	while (dlp_ReadDBList(sd, 0, dlpOpenRead, start, &info) > 0) {
                start = info.index + 1;
                creator[0] = (info.creator & 0xFF000000) >> 24;
                creator[1] = (info.creator & 0x00FF0000) >> 16;
                creator[2] = (info.creator & 0x0000FF00) >> 8;
                creator[3] = (info.creator & 0x000000FF);
                creator[4] = '\0';
		type[0] = (info.type & 0xFF000000) >> 24;
                type[1] = (info.type & 0x00FF0000) >> 16;
                type[2] = (info.type & 0x0000FF00) >> 8;
                type[3] = (info.type & 0x000000FF);
                type[4] = '\0';

                if (!strcmp(dbname, "all")) {
			booldb = FALSE;
		} else {
			booldb = strcmp(info.name, dbname);
		}

                if (!(strcmp(creator, "Vpad") || strcmp(type, "strm") || booldb)) {
		        for(reinit = 0; reinit <= 65536; reinit++) buffer[reinit] = 0;
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
                        
                        formatChunkSize = write_header(out);
                        
                        index = 0;
                        dataChunkSize = 0;
                        ret = 1;
                        while (ret > 0) {
                                ret =
                                    dlp_ReadRecordByIndex
                                    PI_ARGS((sd, db, index, buffer, &id,
                                             &size, &attr, &category));
				if (ret > 0) {
                                  dataChunkSize = write_data(buffer, index, size, dataChunkSize, out);
				}
				index++;
                        }
			wWaveLength = 4 + 4 + 4 + formatChunkSize + 4 + 4 + dataChunkSize;
                        fseek(out, 44, SEEK_SET);
                        fwrite(&dataChunkSize, 4, 1, out);
                        fseek(out, 4, SEEK_SET);
			fwrite(&wWaveLength, 4, 1, out);
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
 * Returns:     0
 *
 ***********************************************************************/
int do_fetch(char *port, char *dbname)
{
        int     sd              = -1,
		ret;   

        sd = pilot_connect(port);

	ret = fetch_wavs(sd, dbname);

	dlp_EndOfSync(sd, dlpEndCodeNormal);
	pi_close(sd);

	return 0;
}


/***********************************************************************
 *
 * Function:    pdb_to_wav
 *
 * Summary:     Do the bulk of the conversion here
 *
 * Parameters:  None
 *
 * Returns:     0 - Success, -1 - Failure
 *
 ***********************************************************************/
int pdb_to_wav(char *filename)
{
        struct pi_file *pi_fp;
        struct DBInfo info;
        FILE *out;
        int 	index,
		ret,
		size,
		attr,
		cat;
        void *buffer;
        pi_uid_t uid;

	long wWaveLength;
        long formatChunkSize;
	long dataChunkSize;

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

        formatChunkSize = write_header(out);

        index = 0;
        dataChunkSize = 0;
        ret = 1;
        while (ret >= 0) {
                ret =
                    pi_file_read_record(pi_fp, index, &buffer, &size, &attr,
                                        &cat, &uid);
		if (ret >= 0) {
                  dataChunkSize = write_data(buffer, index, size, dataChunkSize, out);
		}
		index++;
        }
	wWaveLength = 4 + 4 + 4 + formatChunkSize + 4 + 4 + dataChunkSize;
        fseek(out, 44, SEEK_SET);
        fwrite(&dataChunkSize, 4, 1, out);
        fseek(out, 4, SEEK_SET);
	fwrite(&wWaveLength, 4, 1, out);
        fclose(out);
        pi_file_close(pi_fp);
        printf("OK, wrote %ld bytes to %s\n", dataChunkSize, info.name);
        return 0;
}


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
	printf("   Decodes Palm Voice Memo files to wav files you can read\n\n");
	printf("   Usage: %s -p <port> [options] file\n", progname);
	printf("   Options:\n");
	printf("     -p --port <port>    Use device file <port> to communicate with Palm\n");
	printf("     -h --help           Display this information\n");
	printf("     -v --version        Display version information\n");
	printf("     -f --fetch | <file> Fetch all wav files or specified wav file from the Palm\n");
	printf("     -c --convert <file> Convert <file>.wav.pdb file to wav\n\n");
	printf("   Examples: \n");
	printf("      pilot-wav -p /dev/pilot -f MyVoiceMemo.wav.pdb\n");
	printf("      pilot-wav -c MyVoiceMemo.wav.pdb\n\n");
	return;
}


int main(int argc, char *argv[])
{
        int 	c,
		fetch,
		convert;

        char 	*port		= NULL,
		*progname       = argv[0];

        fetch = convert = FALSE;

        if (argc < 2 ) {
                display_help(progname);
        }

        while ((c = getopt_long(argc, argv, optstring, options, NULL)) != -1) {
                switch (c) {

                case 'h':
                        display_help(progname);
                        return 0;
                case 'v':
                        print_splash(progname);
                        return 0;
                case 'p':
                        port = optarg;
                        break;
                case 'c':
                        pdb_to_wav(optarg);
                        break;
                case 'f':
                        do_fetch(port, optarg);
                        break;
                default:
                        display_help(progname);
                        return 0;
                }
        }
        
        return 0;
}
