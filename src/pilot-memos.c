/*
 * memos.c:  Translate Palm Memos into e-mail format
 *
 * Copyright (c) 1996, Kenneth Albanowski
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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <regex.h>

#include "pi-source.h"
#include "pi-memo.h"
#include "pi-file.h"
#include "pi-header.h"
#include "pi-userland.h"

/* constants to determine how to produce memos */
#define MEMO_MBOX_STDOUT 0
#define MEMO_DIRECTORY 1
#define MAXDIRNAMELEN 1024

void write_memo_mbox(struct Memo m, struct MemoAppInfo mai, int category);
void write_memo_in_directory(char *dirname, struct Memo m,
			     struct MemoAppInfo mai, int category,
			     int verbose, const char *progname);

/***********************************************************************
 *
 * Function:    write_memo_mbox
 *
 * Summary:     Write a memo entry to MailDB database
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void write_memo_mbox(struct Memo m, struct MemoAppInfo mai, int category)
{
	int 	j;
	time_t	now;
	char	*time_str;

	now = time( NULL );

	/* FIXME: might be good to get the time stamp of the memo file for
	   the "Received" line */

	time_str = ctime(&now);

	printf("From Palm.Handheld %s"
	       "From: pilot-link memos (MemoDB) <Palm.Handheld@your.machine>\n"
	       "Received: %s"
	       "To: You\n"
	       "Date: %s"
	       "Subject: ", time_str, time_str, time_str);

	/* print category name in brackets in subject field */
	printf("[%s] ", mai.category.name[category]);

	/* print (at least part of) first line as part of subject: */
	for (j = 0; j < 40; j++) {
		if ((!m.text[j]) || (m.text[j] == '\n'))
			break;
		printf("%c", m.text[j]);
	}
	if (j == 40)
		printf("...\n");
	else
		printf("\n");
	printf("\n");
	puts(m.text);
}

/***********************************************************************
 *
 * Function:    write_memo_in_directory
 *
 * Summary:     Writes each memo into /$DIR/$CATEGORY/$FILENAME form
 *              after the user specifies the -d /dir/name argument
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void
write_memo_in_directory(char *dirname, struct Memo m,
			struct MemoAppInfo mai, int category, int verbose, const char *progname)
{
	int 	j;
	char 	pathbuffer[MAXDIRNAMELEN + (128 * 3)] = "",
		tmp[5] = "";
	FILE *fd;

	/* Should check if dirname exists and is a directory */
	mkdir(dirname, 0700);

	/* Create a directory for the category */
	strncat(pathbuffer, dirname, MAXDIRNAMELEN);
	strncat(pathbuffer, "/", 1);

	/* Should make sure category doesn't have slashes in it */
	strncat(pathbuffer, mai.category.name[category], 60);

	/* Should check if pathbuffer exists and is a directory */
	mkdir(pathbuffer, 0700);

	/* Should check if there were problems creating directory */
	/* open the actual file to write */
	strncat(pathbuffer, "/", 1);
	for (j = 0; j < 40; j++) {
		if ((!m.text[j]) || (m.text[j] == '\n'))
			break;
		if (m.text[j] == '/') {
			strncat(pathbuffer, "=2F", 3);
			continue;
		}
		if (m.text[j] == '=') {
			strncat(pathbuffer, "=3D", 3);
			continue;
		}
#ifdef OS2
		if (m.text[j] == ':') {
			strncat(pathbuffer, "=3A", 3);
			continue;
		}
#endif
		/* escape if it's an ISO8859 control chcter (note: some
		   are printable on the Palm) */
		if ((m.text[j] | 0x7f) < ' ') {
			tmp[0] = '\0';
			sprintf(tmp, "=%2X", (unsigned char) m.text[j]);
		} else {
			tmp[0] = m.text[j];
			tmp[1] = '\0';
		}
		strcat(pathbuffer, tmp);
	}

	if (verbose) {
		printf("Writing %s\n", pathbuffer);
	}

	if (!(fd = fopen(pathbuffer, "w"))) {
		printf("%s: can't open file \"%s\" for writing\n",
			progname, pathbuffer);
		exit(EXIT_FAILURE);
	}
	fputs(m.text, fd);
	fclose(fd);
}


int main(int argc, const char *argv[])
{
	int 	attr,
		c,		/* switch */
		category,
		db,
		index,
		ret,
		sd 		= -1,
		verbose 	= 0,
		delete 		= 0,
		mode 		= MEMO_MBOX_STDOUT,
		bufsize 	= 1024,
		match_category 	= -1,
		title_matching 	= 0;

	ssize_t	len;

	pi_buffer_t	*buffer,
		*appblock;

	const char
                *progname 	= argv[0];

	char    *buf = NULL,
		*dirname = NULL,
		*category_name = NULL,
		*filename = NULL,
		*regex = NULL,
	        *ptr;

	struct 	MemoAppInfo mai;
	struct 	pi_file *pif = NULL;
	struct 	Memo m;

	regex_t title_pattern;
	recordid_t id_;

	poptContext po;

	struct poptOption options[] = {
		USERLAND_RESERVED_OPTIONS
		{"verbose",	'v', POPT_ARG_VAL, &verbose, 1, "Verbose, with -s, print each filename when written"},
		{"delete",	'd', POPT_ARG_VAL, &delete,  1, "Delete memo named by number <num>"},
		{"file",	'f', POPT_ARG_STRING, &filename, 0, "Use <file> as input file (instead of MemoDB.pdb)"},
		{"save",	's', POPT_ARG_STRING, &dirname, 0, "Save memos in <dir> instead of writing to STDOUT"},
		{"category",	'c', POPT_ARG_STRING, &category_name, 0, "Only upload memos in this category"},
		{"regex",	'r', POPT_ARG_STRING, &regex, 0, "Select memos saved by regular expression on title"},
		POPT_TABLEEND
	};

	po = poptGetContext("memos", argc, argv, options, 0);
	poptSetOtherOptionHelp(po,"\n\n"
		"  Manipulate Memo entries from a file or your Palm device\n\n"
		"  By default, the contents of your Palm's memo database will be written to\n"
		"  standard output as a standard UNIX mailbox (mbox-format) file, with each\n"
		"  memo as a separate message.  The subject of each message will be the\n"
		"  category.\n\n"

		"  If '-s' is specified, than instead of being written to standard output,\n"
		"  will be saved in subdirectories of <dir>. Each subdirectory will be the\n"
		"  name of a category on the Palm, and will contain the memos in that\n"
		"  category. Each memo's filename will be the first line (up to the first 40\n"
		"  characters) of the memo. Control chcters, slashes, and equal signs that\n"
		"  would otherwise appear in filenames are converted after the fashion of\n"
		"  MIME's quoted-printable encoding. Note that if you have two memos in the\n"
		"  same category whose first lines are identical, one of them will be\n"
		"  overwritten.\n\n"

		"  If '-f' is specified, the specified file will be treated as a memo\n"
		"  database from which to read memos, rather than HotSyncing from the Palm.\n");

	if (argc < 2) {
		poptPrintUsage(po,stderr,0);
		return 1;
	}

	while ((c = poptGetNextOpt(po)) >= 0) {
		fprintf(stderr,"   ERROR: Unhandled option %d.\n",c);
		return -1;
	}

	if (dirname)
		mode = MEMO_DIRECTORY;

	if (c < -1)
		plu_badoption(po, c);

	if (regex) {
		ret = regcomp(&title_pattern, regex, REG_NOSUB);
		buf = (char *) malloc(bufsize);
		if (ret) {
			regerror(ret, &title_pattern, buf, bufsize);
			fprintf(stderr,"   ERROR: Regexp - %s\n", buf);
			return 1;
		}
		title_matching = 1;
	}

	/* FIXME - Need to add tests here for port/filename, clean this. -DD */
	appblock = pi_buffer_new(0xffff);
	if (!filename) {

	        sd = plu_connect();

        	if (sd < 0)
                	goto error;

	        if (dlp_OpenConduit(sd) < 0)
        	        goto error_close;

		/* Open the Memo Pad's database, store access handle in db */
		if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "MemoDB", &db) < 0) {
			fprintf(stderr,"   ERROR: Unable to open MemoDB on Palm.\n");
			dlp_AddSyncLogEntry(sd,
					    "Unable to open MemoDB.\n");
			goto error_close;
		}

		dlp_ReadAppBlock(sd, db, 0, 0xffff, appblock);
	} else {
		pif = pi_file_open(filename);
		if (!pif) {
			fprintf(stderr,"   ERROR: pi_file_open: %s\n",strerror(errno));
			return 1;
		}

		pi_file_get_app_info(pif, (void **) &ptr, &len);
		pi_buffer_append(appblock, ptr, len);
	}

	unpack_MemoAppInfo(&mai, appblock->data, appblock->used);
	pi_buffer_free(appblock);

	if (category_name && category_name[0] != '\0') {
		match_category = plu_findcategory(&mai.category,category_name, PLU_CAT_WARN_UNKNOWN);
		if (match_category < 0) {
			goto error_close;
		};
	}

	buffer = pi_buffer_new (0xffff);

	for (index = 0;; index++) {

		if (!filename) {
			if (match_category >= 0) {
				len = dlp_ReadNextRecInCategory(sd, db,
							        match_category,
							        buffer, &id_,
							        0, &attr);
				category = match_category;
			} else {
				len = dlp_ReadRecordByIndex(sd, db, index,
							    buffer, &id_,
							    &attr,
							    &category);
			}
			if (len < 0)
				break;
		} else {
			if (pi_file_read_record
			    (pif, index, (void **) &ptr, &len, &attr, &category,
			     0) < 0)
				break;
			memcpy(buffer->data, ptr, len);
			buffer->used = len;
		}

		/* Skip deleted records */
		if ((attr & dlpRecAttrDeleted)
		    || (attr & dlpRecAttrArchived))
			continue;

		/* Skip memos whose category doesn't match */
		if( match_category >= 0 ) {
			if( match_category != category )
				continue;
		}

		unpack_Memo(&m, buffer->data, buffer->used);

		/* Skip memos whose title does not match with the query */
		if (title_matching) {
			for (len = 0; m.text[len] && m.text[len] != '\n';
			     len++);
			if (bufsize < len + 1)
				buf = (char *) realloc(buf, len + 1);
			strncpy(buf, m.text, len);
			buf[len] = '\0';
			if (regexec(&title_pattern, buf, 0, NULL, 0) ==
			    REG_NOMATCH)
				continue;
		}

		switch (mode) {
		  case MEMO_MBOX_STDOUT:
			  write_memo_mbox(m, mai, category);
			  break;
		  case MEMO_DIRECTORY:
			  write_memo_in_directory(dirname, m, mai,
						  category, verbose, progname);
			  break;
		}
	}

	pi_buffer_free (buffer);

	if (delete && !filename) {
		if (verbose)
			printf("Deleting record %d.\n", (int) id_);
		dlp_DeleteRecord(sd, db, 0, id_);
	}

	if (title_matching) {
		regfree(&title_pattern);
		free(buf);
	}

	if (!filename) {
		/* Close the database */
		dlp_CloseDB(sd, db);
		dlp_AddSyncLogEntry(sd, "Successfully read memos from Palm.\n"
					"Thank you for using pilot-link.\n");
		pi_close(sd);
	} else {
		pi_file_close(pif);
	}
	return 0;

error_close:
	if (pif) {
		pi_file_close(pif);
	}
	if (sd >= 0) {
		pi_close(sd);
	}

error:
	return -1;
}
