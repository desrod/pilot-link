/*
 * $Id$ 
 *
 * install-memo.c: Palm MemoPad Record Syncronization Conduit
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
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include <stdio.h>
#include <strings.h>
#include <sys/stat.h>

#include "pi-dlp.h"
#include "pi-memo.h"
#include "pi-header.h"
#include "pi-userland.h"

poptContext po;

#ifndef HAVE_BASENAME
/* Yes, this isn't efficient... scanning the string twice */
#define basename(s) (strrchr((s), '/') == NULL ? (s) : strrchr((s), '/') + 1)
#endif



typedef struct {
	char **filenames;
	unsigned int filenames_allocated;
	unsigned int filenames_used;
} filename_array;

filename_array memos = { NULL, 0, 0 } ;

unsigned int check_filenames(filename_array *m)
{
	int i;
	int count = 0;
	struct  stat sbuf;

	if (!m || !m->filenames) return 0;

	for (i = 0; i<m->filenames_used; i++) {
		if (!(m->filenames[i])) continue;
		if (stat(m->filenames[i], &sbuf) <0) {
			fprintf(stderr,"   ERROR: cannot determine size of file '%s'\n",m->filenames[i]);
			m->filenames[i] = NULL;
		} else {
			count++;
		}
	}
	return count;
}

void append_filename(filename_array *m, const char *filename)
{
	if (!m || !m->filenames)
		return;
	if (!filename)
		return;
	if (m->filenames_used >= (m->filenames_allocated-1)) {
		fprintf(stderr,"  ERROR: More filenames listed than were expected.\n");
		fprintf(stderr,"         File `%s' is ignored.\n", filename);
		return;
	}

	m->filenames[m->filenames_used] = strdup(filename);
	m->filenames_used++;
}

void append_filenames(filename_array *m, const char **names)
{
	if (!m || !m->filenames) return;
	if (!names) return;

	while (*names) {
		append_filename(m,*names);
		names++;
	}
}

int install_memo(int sd, int db, int category, int add_title, char *filename)
{
	struct  stat sbuf;
	FILE *f = NULL;
	char *tmp = NULL;
	char *memo_buf = NULL;
	int memo_size, preamble;

	if (!filename) return 1;

	/* Check the file still exists and its size. */
	if (stat(filename, &sbuf) <0) {
		fprintf(stderr,"   ERROR: Unable to open %s (%s)\n\n",
			filename, strerror(errno));
		return 1;
	}

	/* When adding the title, we only want the filename, not the full path. */
	tmp = strrchr (filename, '/');
	if (tmp) {
		filename = tmp + 1;
	}

	memo_size = sbuf.st_size;
	preamble = add_title ? strlen(filename) + 1 : 0;
	memo_buf = calloc(1, memo_size + preamble + 16);
	if (!memo_buf) {
		fprintf(stderr,"   ERROR: cannot allocate memory for memo (%s)\n",
			strerror(errno));
		return 1;
	}

	if (memo_size + preamble > 65490) {
		fprintf(stderr, "   WARNING: `%s'\n",filename);
		fprintf(stderr, "   File is larger than the allowed size for Palm memo size. Please\n");
		fprintf(stderr, "   decrease the file size to less than 65,490 bytes and try again.\n\n");
		return 1;
	}

	if (preamble)
		sprintf(memo_buf, "%s\n", filename);

	f = fopen(filename, "rb");
	if (f == NULL) {
		fprintf(stderr,"   ERROR: Unable to open %s (%s)\n\n",
			filename, strerror(errno));
		return 1;
	}
	fread(memo_buf + preamble, memo_size, 1, f);
	fclose(f);

	dlp_WriteRecord(sd, db, 0, 0, category, memo_buf, -1, 0);
	free(memo_buf);


	if ((memo_size < 65490) && (memo_size > 4096)) {
		fprintf(stderr, "   WARNING: `%s'\n", filename);
		fprintf(stderr, "   This file was synchonized successfully, but will remain uneditable,\n");
		fprintf(stderr, "   because it is larger than the Palm limitation of 4,096 bytes in\n");
		fprintf(stderr, "   size.\n\n");

	} else if (memo_size < 4096) {
		fprintf(stderr, "   File '%s' was synchronized to your Palm device\n\n", filename);
	}

	return 0;
}


int main(int argc, char *argv[])
{
	int 	sd		= -1,
		po_err  	= -1,
		add_title	= 0,
		category	= -1,
		replace		= 0,
		db,
		ReadAppBlock;
	unsigned int i;
	unsigned int count = 0;

	char
		*category_name	= NULL,
		*filename	= NULL;
	char messagebuffer[256];

        struct 	PilotUser User;
        struct 	MemoAppInfo mai;

	pi_buffer_t *appblock;

        const struct poptOption options[] = {
		USERLAND_RESERVED_OPTIONS
                { "category", 'c', POPT_ARG_STRING, &category_name, 0, "Place the memo entry in this category (category must already exist)", "name"},
                { "replace",  'r', POPT_ARG_NONE,   &replace, 0, "Replace all memos in specified category"},
                { "title",    't', POPT_ARG_NONE,   &add_title, 0, "Use the filename as the title of the Memo entry"},
		{ "file",     'f', POPT_ARG_STRING, &filename, 'f', "File containing the target memo entry"},
                POPT_TABLEEND
        };

        po = poptGetContext("install-memo", argc, (const char **) argv, options, 0);
	poptSetOtherOptionHelp(po,"[file ...]\n\n"
		"    Adds a single memo from a file to the memo database.\n\n");

	if (argc < 2) {
		poptPrintUsage(po,stderr,0);
		return 1;
	}

	/* How many files _might_ we have as arguments? Clearly, argc
	** is the maximum that is on the command-line, so use that as
	** a good guess.
	*/
	memos.filenames = (char **)calloc(argc, sizeof(char *));
	memos.filenames_allocated = argc;
	memos.filenames_used = 0;

        while ((po_err = poptGetNextOpt(po)) >= 0) {
		switch (po_err) {
		case 'f' :
			append_filename(&memos,filename);
			break;
		default :
			fprintf(stderr,"   ERROR: Unhandled option %d.\n",po_err);
			return 1;
		}
	}
	if (po_err < -1)
		plu_badoption(po,po_err);

	append_filenames(&memos, poptGetArgs(po));

	if (replace && !category_name) {
		fprintf(stderr,"   ERROR: memo category required when specifying replace\n");
		return 1;
	}
	if (!check_filenames(&memos)) {
		fprintf(stderr,"   ERROR: must specify a file with -f filename\n");
		return 1;
	}


	sd = plu_connect();
	if (sd < 0)
		goto error;

	if (dlp_OpenConduit(sd) < 0)
		goto error_close;

	dlp_ReadUserInfo(sd, &User);
	dlp_OpenConduit(sd);

	/* Open the Memo Pad's database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "MemoDB", &db) < 0) {
		fprintf(stderr, "   ERROR:\n");
		fprintf(stderr, "   Unable to open MemoDB. Please make sure you have hit the MemoPad\n");
		fprintf(stderr, "   button at least once, to create a MemoDB.pdb file on your Palm,\n");
		fprintf(stderr, "   then sync again.\n\n");
		dlp_AddSyncLogEntry(sd, "Unable to open MemoDB.\n");
		goto error_close;
	}

	appblock = pi_buffer_new(0xffff);
	ReadAppBlock = dlp_ReadAppBlock(sd, db, 0, 0xffff, appblock);
	unpack_MemoAppInfo(&mai, appblock->data, ReadAppBlock);
	pi_buffer_free(appblock);

	if (category_name) {
		category = plu_findcategory(&mai.category,category_name,
			PLU_CAT_CASE_INSENSITIVE |
			PLU_CAT_DEFAULT_UNFILED |
			PLU_CAT_WARN_UNKNOWN);

		if (replace) {
			dlp_DeleteCategory(sd, db, category);
		}

	} else {
		category = 0;	/* Unfiled */
	}

	for (i=0; i<memos.filenames_used; i++) {
		if (!install_memo(sd, db, category, add_title, memos.filenames[i])) count++;
	}


	/* Close the database */
	dlp_CloseDB(sd, db);

	/* Tell the user who it is, with a different PC id. */
	User.lastSyncPC = 0x00010000;
	User.successfulSyncDate = time(NULL);
	User.lastSyncDate = User.successfulSyncDate;
	dlp_WriteUserInfo(sd, &User);

	if (count == 0) {
		snprintf(messagebuffer, sizeof(messagebuffer),
			"No memos were installed to Palm.\n"
			"Thank you for using pilot-link.\n");
	} else if (count == 1) {
		snprintf(messagebuffer, sizeof(messagebuffer),
			"Successfully wrote a memo to Palm.\n"
			"Thank you for using pilot-link.\n");
	} else {
		snprintf(messagebuffer, sizeof(messagebuffer),
			"Successfully wrote %d memos to Palm.\n"
			"Thank you for using pilot-link.\n",count);
	}
	messagebuffer[sizeof(messagebuffer)-1] = 0;
	dlp_AddSyncLogEntry(sd,messagebuffer);

	dlp_EndOfSync(sd, 0);
	pi_close(sd);
	return 0;

error_close:
	pi_close(sd);

error:
	return -1;
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
/* ex: set tabstop=4 expandtab: */
/* Local Variables: */
/* indent-tabs-mode: t */
/* c-basic-offset: 8 */
/* End: */
