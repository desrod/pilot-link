/*
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
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/stat.h>
#include <errno.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-memo.h"
#include "pi-header.h"

#include "pi-userland.h"

poptContext po;

#ifndef HAVE_BASENAME
/* Yes, this isn't efficient... scanning the string twice */
#define basename(s) (strrchr((s), '/') == NULL ? (s) : strrchr((s), '/') + 1)
#endif


int main(int argc, char *argv[])
{
	int 	sd		= -1,
		po_err  	= -1,
		add_title	= 0,
		category	= -1,
		replace		= 0,
		db,
		ReadAppBlock,
		memo_size,
		preamble;

	char
		*memo_buf	= NULL,
		*category_name	= NULL,
		*filename	= NULL,
		*tmp		= NULL;

        struct 	PilotUser User;
        struct 	MemoAppInfo mai;
	struct  stat sbuf;

	pi_buffer_t *appblock;

	FILE *f;

        const struct poptOption options[] = {
		USERLAND_RESERVED_OPTIONS
                { "category", 'c', POPT_ARG_STRING, &category_name, 0, "Place the memo entry in this category (category must already exist)", "name"},
                { "replace",  'r', POPT_ARG_NONE,   &replace, 0, "Replace all memos in specified category"},
                { "title",    't', POPT_ARG_NONE,   &add_title, 0, "Use the filename as the title of the Memo entry"},
		{ "file",     'f', POPT_ARG_STRING, &filename, 0, "File containing the target memo entry"},
                POPT_TABLEEND
        };

        po = poptGetContext("install-memo", argc, (const char **) argv, options, 0);
	poptSetOtherOptionHelp(po,"\n\n"
		"    Adds a single memo from a file to the memo database.\n\n");

	if (argc < 2) {
		poptPrintUsage(po,stderr,0);
		return 1;
	}

        while ((po_err = poptGetNextOpt(po)) >= 0) {
	}
	if (po_err < -1)
	    plu_badoption(po,po_err);

	if (replace && !category_name) {
		fprintf(stderr,"   ERROR: memo category required when specifying replace\n");
		return 1;
	}
	if (!filename) {
		fprintf(stderr,"   ERROR: must specify a file with -f filename\n");
		return 1;
	}

	if (stat(filename, &sbuf) <0) {
		fprintf(stderr,"   ERROR: cannot determine size of file '%s'\n",filename);
		return 1;
	}

	f = fopen(filename, "rb");
	if (f == NULL) {
		fprintf(stderr,"   ERROR: Unable to open %s (%s)\n\n",
			filename, strerror(errno));
		return 1;
	}


	/* When adding the title, we only want the filename, not the full path. */
	tmp = strrchr (filename, '/');
	if (tmp)
		filename = tmp + 1;

	memo_size = sbuf.st_size;
	preamble = add_title ? strlen(filename) + 1 : 0;
	memo_buf = calloc(1, memo_size + preamble + 16);
	if (!memo_buf) {
		fprintf(stderr,"   ERROR: cannot allocate memory for memo (%s)\n",
			strerror(errno));
		return 1;
	}

	if (preamble)
		sprintf(memo_buf, "%s\n", filename);
	fread(memo_buf + preamble, memo_size, 1, f);
	fclose(f);

	memo_size += preamble;

	if (memo_size > 65490) {
		fprintf(stderr, "   ERROR:\n");
		fprintf(stderr, "   File is larger than the allowed size for Palm memo size. Please\n");
		fprintf(stderr, "   decrease the file size to less than 65,490 bytes and try again.\n\n");
		fprintf(stderr, "   Files larger than 4,096 bytes and less than 65,490 bytes may be\n");
		fprintf(stderr, "   syncronized, but will not be editable on the Palm device itself\n");
		fprintf(stderr, "   due to Palm limitationis.\n\n");
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


	dlp_WriteRecord(sd, db, 0, 0, category, memo_buf, -1, 0);
	free(memo_buf);


	if ((memo_size < 65490) && (memo_size > 4096)) {
		fprintf(stderr, "   This file was synconized successfully, but will remain uneditable,\n");
		fprintf(stderr, "   because it is larger than the Palm limitation of 4,096 bytes in\n");
		fprintf(stderr, "   size.\n\n");

	} else if (memo_size < 4096) {
		fprintf(stderr, "   File '%s' was synchronized to your Palm device\n\n", filename);
	}

	/* Close the database */
	dlp_CloseDB(sd, db);

	/* Tell the user who it is, with a different PC id. */
	User.lastSyncPC = 0x00010000;
	User.successfulSyncDate = time(NULL);
	User.lastSyncDate = User.successfulSyncDate;
	dlp_WriteUserInfo(sd, &User);

	dlp_AddSyncLogEntry(sd, "Successfully wrote a memo to Palm.\n"
				"Thank you for using pilot-link.\n");

	dlp_EndOfSync(sd, 0);
	pi_close(sd);
	return 0;

error_close:
	pi_close(sd);

error:
	return -1;
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
