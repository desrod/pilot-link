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

#include "popt.h"
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

poptContext po;

#ifndef HAVE_BASENAME
/* Yes, this isn't efficient... scanning the string twice */
#define basename(s) (strrchr((s), '/') == NULL ? (s) : strrchr((s), '/') + 1)
#endif

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
static void display_help(char *progname)
{
	printf("Installs a new memo in a Palm Computing Device\n\n");

	printf("Usage: %s -p <port> [-rt] [-c category] file [file] ...\n\n", 
		basename(progname));

	poptPrintHelp(po, stderr, 0);

	exit(EXIT_SUCCESS);
}


int main(int argc, char *argv[])
{
	int 	sd		= -1,
		po_err  	= -1,
		add_title	= 0,
		category	= -1,
		replace		= 0,
		db,
		i		= 0,
		ReadAppBlock, 
		memo_size,
		preamble;
	
	char 	*port		= NULL,
		*memo_buf	= NULL,
		*progname	= argv[0],
		*category_name	= NULL,
		*filename	= NULL,
		buf[0xffff];
	
        struct 	PilotUser User;
        struct 	MemoAppInfo mai;
	struct  stat sbuf;

	FILE *f;
	
        const struct poptOption options[] = {
                { "port",     'p', POPT_ARG_STRING, &port, 0, "Use device <port> to communicate with Palm", "<port>"},
                { "help",     'h', POPT_ARG_NONE,   0, 'h', "Display this information"},
                { "version",  'v', POPT_ARG_NONE,   0, 'v', "Display version information"},
                { "category", 'c', POPT_ARG_STRING, &category_name, 0, "Place the memo entry in this category (category must already exist)", "name"},
                { "replace",  'r', POPT_ARG_NONE,   &replace, 0, "Replace all memos in specified category"},
                { "title",    't', POPT_ARG_NONE,   &add_title, 0, "Use the filename as the title of the Memo entry"},
		{ "file",     'f', POPT_ARG_STRING, &filename, 0, "File containing the target memo entry"}, 
                POPT_AUTOHELP
                { NULL, 0, 0, NULL, 0 }
        };

        po = poptGetContext("install-memo", argc, (const char **) argv, options, 0);

        if (argc < 2) {
                display_help(progname);
                exit(1);
        }

        while ((po_err = poptGetNextOpt(po)) != -1) {
                switch (po_err) {
			
		case 'v':
			print_splash(progname);
			return 0;
		default:
			display_help(progname);
			return 0;
		}
	}
		
	if (replace && !category_name) {
		printf("%s: memo category required when specifying replace\n",
			progname);
		display_help(progname);
	}

	stat(filename, &sbuf);
	if (sbuf.st_size > 65490) {
		fprintf(stderr, "\n");
		fprintf(stderr, "   File is larger than the allowed size for Palm memo size. Please\n");
		fprintf(stderr, "   decrease the file size to less than 65,490 bytes and try again.\n\n");
		fprintf(stderr, "   Files larger than 4,096 bytes and less than 65,490 bytes may be\n");
		fprintf(stderr, "   syncronized, but will not be editable on the Palm device itself\n");
		fprintf(stderr, "   due to Palm limitationis.\n\n");
		return 1;
	}
	
	sd = pilot_connect(port);
	if (sd < 0)
		goto error;

	if (dlp_OpenConduit(sd) < 0)
		goto error_close;
	
	dlp_ReadUserInfo(sd, &User);
	dlp_OpenConduit(sd);

	/* Open the Memo Pad's database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "MemoDB", &db) < 0) {
		fprintf(stderr, "   Unable to open MemoDB. Please make sure you have hit the MemoPad\n");
		fprintf(stderr, "   button at least once, to create a MemoDB.pdb file on your Palm,\n");
		fprintf(stderr, "   then sync again.\n\n");
		dlp_AddSyncLogEntry(sd, "Unable to open MemoDB.\n");
		exit(1);
	}

	ReadAppBlock = dlp_ReadAppBlock(sd, db, 0, (unsigned char *) buf, 0xffff);
	unpack_MemoAppInfo(&mai, (unsigned char *) buf, ReadAppBlock);

	if (category_name) {
		for (i = 0; i < 16; i++)
			if (!strcasecmp
			    (mai.category.name[i], category_name)) {
				category = i;
				break;
			}

                if (category < 0) {
                        printf("   Category '%s' did not exist on the Palm, "
                               "entry created in 'Unfiled'.\n\n",
                                category_name);
                        category = 0;
                }

		if (replace) {
			dlp_DeleteCategory(sd, db, category);
		}

	} else {
		category = 0;	/* Unfiled */
	}

	f = fopen(filename, "rb");

	if (f == 0) {
		printf("   Unable to open %s (%s), skipping...\n\n",
			filename, strerror(errno));
	}

	fseek(f, 0, SEEK_END);
	memo_size = ftell(f);
	fseek(f, 0, SEEK_SET);

	if ((sbuf.st_size < 65490) && (sbuf.st_size > 4096)) {
		fprintf(stderr, "   This file was synconized successfully, but will remain uneditable,\n");
		fprintf(stderr, "   because it is larger than the Palm limitation of 4,096 bytes in\n");
		fprintf(stderr, "   size.\n\n");

	} else if ((sbuf.st_size < 4096) && (sbuf.st_size > 0)) {
		fprintf(stderr, "   File '%s' was synchronized to your Palm device\n\n", filename);
	}

	preamble = add_title ? strlen(filename) + 1 : 0;

	memo_buf = malloc(memo_size + preamble + 1);

	if (memo_buf == NULL) {
		perror("malloc()");
		exit(1);
	}

	if (preamble)
		sprintf(memo_buf, "%s\n", filename);

	fread(memo_buf + preamble, memo_size, 1, f);

	memo_buf[memo_size + preamble] = '\0';

	dlp_WriteRecord(sd, db, 0, 0, category, memo_buf, -1, 0);
	free(memo_buf);
	fclose(f);

	/* Close the database */
	dlp_CloseDB(sd, db);

	/* Tell the user who it is, with a different PC id. */
	User.lastSyncPC = 0x00010000;
	User.successfulSyncDate = time(NULL);
	User.lastSyncDate = User.successfulSyncDate;
	dlp_WriteUserInfo(sd, &User);

	dlp_AddSyncLogEntry(sd, "Successfully wrote memo(s) to Palm.\n"
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
