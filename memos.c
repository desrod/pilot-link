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

#include "getopt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <regex.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-memo.h"
#include "pi-dlp.h"
#include "pi-file.h"
#include "pi-header.h"

/* constants to determine how to produce memos */
#define MEMO_MBOX_STDOUT 0
#define MEMO_DIRECTORY 1
#define MAXDIRNAMELEN 1024

int verbose = 0;
char *progname;

int pilot_connect(const char *port);
static void Help(char *progname);

struct option options[] = {
        {"help",        no_argument,       NULL, 'h'},
        {"version",     no_argument,       NULL, 'v'},
        {"port",        required_argument, NULL, 'p'},
        {"verbose",     no_argument,       NULL, 'v'},
        {"delete",      required_argument, NULL, 'd'},
        {"file",        required_argument, NULL, 'f'},
        {"save",        required_argument, NULL, 's'},
        {"category",    required_argument, NULL, 'c'},
        {"regex",       required_argument, NULL, 'r'},
        {NULL,          0,                 NULL, 0}
};

static const char *optstring = "vqdp:s:f:c:r:h";

void write_memo_mbox(struct Memo m, struct MemoAppInfo mai, int category);
void write_memo_in_directory(char *dirname, struct Memo m,
			     struct MemoAppInfo mai, int category,
			     int verbose);

/***********************************************************************
 *
 * Function:    write_memo_mbox
 *
 * Summary:     Write a memo entry to MailDB database
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void write_memo_mbox(struct Memo m, struct MemoAppInfo mai, int category)
{
	int 	j;

	printf("From Your.Palm Tue Oct 1 07:56:25 1996\n"
	       "Received: Palm@p by memo Tue Oct 1 07:56:25 1996\n"
	       "To: you@y\n"
	       "Date: Thu, 31 Oct 1996 23:34:38 -0500\n"
	       "Subject: ");

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
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void
write_memo_in_directory(char *dirname, struct Memo m,
			struct MemoAppInfo mai, int category, int verbose)
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

	/* Should check if dirname exists and is a directory */
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
		exit(1);
	}
	fputs(m.text, fd);
	fclose(fd);
}

/***********************************************************************
 *
 * Function:    Help
 *
 * Summary:     Outputs the program arguments and params
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void Help(char *progname)
{
	printf("   Manipulate your MemoDB.pdb file or your Memos database on your Palm device\n\n"
	       "   Usage: memos [-p <port> | -f MemoDB] [options]\n\n"
	       "   Options:\n"
               "     -p <port>      Use device file <port> to communicate with Palm\n"
               "     -h             Display this information\n"
	       "     -v             Verbose, with -d, print each filename as it's written\n" 
	       "     -d             Delete the memo named by <number>\n"
	       "     -f file        Use <file> as memo database file (rather than HotSync)\n"
	       "     -s <dir>       Save memos in <dir> instead of writing to STDOUT\n"
	       "     -c category    Only upload memos in this category\n"
	       "     -r regexp      Select memos to be saved by title\n\n" 
	       "   By default, the contents of your Palm's memo database will be written to\n"
	       "   standard output as a standard Unix mailbox (mbox-format) file, with each\n"
	       "   memo as a separate message.  The subject of each message will be the\n"
	       "   category.\n\n"
	       "   If '-s' is specified, than instead of being written to standard output,\n"
	       "   will be saved in subdirectories of <dir>.  Each subdirectory will be the\n"
	       "   name of a category on the Palm, and will contain the memos in that\n"
	       "   category.  Each memo's filename will be the first line (up to the first\n"
	       "   40 chcters) of the memo.  Control chcters, slashes, and equal signs\n"
	       "   that would otherwise appear in filenames are converted after the fashion\n" 
	       "   of MIME's quoted-printable encoding.  Note that if you have two memos in\n" 
	       "   the same category whose first lines are identical, one of them will be\n"   
	       "   overwritten.\n\n"
	       "   If '-f' is specified, the specified file will be treated as a memo\n"
	       "   database from which to read memos, rather than HotSyncing from the Palm.\n\n");
	return;
}

int main(int argc, char *argv[])
{
	int 	attr, 
		c,		/* switch */
		category,
		db,
		index,
		len,
		ret,
		sd 		= -1,
		verbose 	= 0,
		delete 		= 0,
		mode 		= MEMO_MBOX_STDOUT,
		bufsize 	= 1024,
		match_category 	= -1,
		title_matching 	= 0;

	unsigned char 	buffer[0xffff];
	
	char 	appblock[0xffff],
		dirname[MAXDIRNAMELEN] = "",
		*buf 		= NULL,
		*progname 	= argv[0],
		*port 		= NULL,
		category_name[MAXDIRNAMELEN + 1] = "",
		filename[MAXDIRNAMELEN + 1], *ptr;
	
	struct 	MemoAppInfo mai;
	struct 	pi_file *pif = NULL;
	struct 	Memo m;

	regex_t title_pattern;
	recordid_t id;

	while ((c = getopt_long(argc, argv, optstring, options, NULL)) != -1) {
		switch (c) {
		  case 'v':
			  PalmHeader(progname);
			  exit(0);
		  case 'd':
			  delete = 1;
			  break;
		  case 'p':
			  port = optarg;
			  break;
		  case 'f':
			  /* optarg is name of file to use instead of hotsyncing */
			  strncpy(filename, optarg, MAXDIRNAMELEN);
			  filename[MAXDIRNAMELEN] = '\0';
			  break;
		  case 's':
			  /* optarg is name of directory to create and store memos in */
			  strncpy(dirname, optarg, sizeof(dirname));
			  mode = MEMO_DIRECTORY;
			  break;
		  case 'c':
			  /* optarg is name of category to fetch memos of */
			  strncpy(category_name, optarg, MAXDIRNAMELEN);
			  filename[MAXDIRNAMELEN] = '\0';
			  break;
		  case 'r':
			  /* optarg is a query to select memos by title */
			  ret = regcomp(&title_pattern, optarg, REG_NOSUB);
			  buf = (char *) malloc(bufsize);
			  if (ret) {
				  regerror(ret, &title_pattern, buf,
					   bufsize);
				  printf("%s\n", buf);
				  exit(1);
			  }
			  title_matching = 1;
			  break;
		  case 'h':
			  Help(progname);
			  exit(0);
		}
	}

	/* Need to add tests here for port/filename, clean this. -DD */
	if (filename[0] == '\0') {
	
	        sd = pilot_connect(port);

        	if (sd < 0)
                	goto error;

	        if (dlp_OpenConduit(sd) < 0)
        	        goto error_close;

		/* Open the Memo Pad's database, store access handle in db */
		if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "MemoDB", &db) < 0) {
			printf("Unable to open MemoDB.\n");
			dlp_AddSyncLogEntry(sd,
					    "Unable to open MemoDB.\n");
			exit(1);
		}
	
		dlp_ReadAppBlock(sd, db, 0, (unsigned char *) appblock,
				 0xffff);
	} else {
		int len;

		pif = pi_file_open(filename);
		if (!pif) {
			perror("pi_file_open");
			exit(1);
		}

		ret = pi_file_get_app_info(pif, (void *) &ptr, &len);
		if (ret == -1) {
			perror("pi_file_get_app_info");
			exit(1);
		}

		memcpy(appblock, ptr, len);
	}

	unpack_MemoAppInfo(&mai, (unsigned char *) appblock, 0xffff);

	if (category_name[0] != '\0') {
		for (index = 0, match_category = -1; index < 16; index += 1) {
			if ((strlen(mai.category.name[index]) > 0)
			    && (strcmp(mai.category.name[index], category_name)
				== 0))
				match_category = index;
		}
		if (match_category < 0) {
			printf("Can't find specified Memo category \"%s\".\n",
				category_name);
			dlp_AddSyncLogEntry(sd,
					    "Can't find specified memo category.\n");
			exit(1);
		};
	}

	for (index = 0;; index++) {

		if (filename[0] == '\0') {
			if (match_category >= 0) {
				len = dlp_ReadNextRecInCategory(sd, db,
							        match_category,
							        buffer, &id,
							        0, 0,
							        &attr);
				category = match_category;
			} else {
				len = dlp_ReadRecordByIndex(sd, db, index,
							    buffer, &id, 0,
							    &attr,
							    &category);
			}
			if (len < 0)
				break;
		} else {
			if (pi_file_read_record
			    (pif, index, (void *) &ptr, &len, &attr, &category,
			     0))
				break;
			memcpy(buffer, ptr, len);
		}

		/* Skip deleted records */
		if ((attr & dlpRecAttrDeleted)
		    || (attr & dlpRecAttrArchived))
			continue;

		unpack_Memo(&m, buffer, len);

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
						  category, verbose);
			  break;
		}
	}

	if (delete && (filename[0] == '\0')) {
		if (verbose)
			printf("Deleting record %d.\n", (int) id);
		dlp_DeleteRecord(sd, db, 0, id);
	}

	if (title_matching) {
		regfree(&title_pattern);
		free(buf);
	}

	if (filename[0] == '\0') {
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
	pi_close(sd);

error:
	perror("   ERROR:");
	fprintf(stderr, "\n");
	fprintf(stderr, "Please use -h for more detailed options.\n");

	return -1;
}
