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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "getopt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-memo.h"
#include "pi-header.h"

/* Declare prototypes */
static void display_help(char *progname);
void print_splash(char *progname);
int pilot_connect(char *port);

struct option options[] = {
	{"port"   ,     required_argument, NULL, 'p'},
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
	{"category",    required_argument, NULL, 'c'},
	{"replace",     no_argument,       NULL, 'r'},
	{"title",       required_argument, NULL, 't'},
	{NULL,          0,                 NULL, 0}
};

static const char *optstring = "p:hvc:rt:";

static void display_help(char *progname)
{
	printf("   Installs a new memo in a Palm Computing Device\n\n");
	printf("   Usage: %s -p <port> [-rt] [-c category] file [file] ...\n\n", 
		progname);
	printf("   Options:\n");
	printf("     -r = replace all memos in specified category\n");
	printf("     -t = use filename as memo title\n\n");

	return;
}

int main(int argc, char *argv[])
{
	int 	add_title	= 0,
		category,
		c,		/* switch */
		db,
		i		= 0,
		j		= 0,
		ReadAppBlock, 
		memo_size,
		preamble,
		replace_category= 0,
		sd		= -1;
	
	char 	*port		= NULL,
		*memo_buf,
		*progname	= argv[0],
		*category_name	= NULL,
		buf[0xffff];
	
        struct 	PilotUser User;
        struct 	MemoAppInfo mai;
	struct  stat sbuf;

	FILE *f;
	
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
			category_name = optarg;
			break;
		case 'r':
			replace_category++;
			break;
		case 't':
			add_title++;
			break;
		default:
			display_help(progname);
			return 0;
		}
	}
		
	argc -= optind;
	argv += optind;

	if (replace_category && !category_name) {
		printf("%s: memo category required when specifying replace\n",
			progname);
		display_help(progname);
	}

	stat(argv[j], &sbuf);
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
		category = -1;	/* invalid category */
		for (i = 0; i < 16; i++)
			if (!strcasecmp
			    (mai.category.name[i], category_name)) {
				category = i;
				break;
			}
		if (category < 0) {
			printf("Category %s not found on Palm\n", category_name);
			exit(2);
		}

		if (replace_category)
			dlp_DeleteCategory(sd, db, category);

	} else
		category = 0;	/* unfiled */

	for (j = 0; j < argc; j++) {

		f = fopen(argv[j], "rb");

		if (f == 0) {
			printf("   Unable to open %s (%s), skipping...\n\n",
				argv[j], strerror(errno));
			continue;
		}

		fseek(f, 0, SEEK_END);
		memo_size = ftell(f);
		fseek(f, 0, SEEK_SET);

		if ((sbuf.st_size < 65490) && (sbuf.st_size > 4096)) {
			fprintf(stderr, "   This file was synconized successfully, but will remain uneditable,\n");
			fprintf(stderr, "   because it is larger than the Palm limitation of 4,096 bytes in\n");
			fprintf(stderr, "   size.\n\n");

		} else if ((sbuf.st_size < 4096) && (sbuf.st_size > 0)) {
			fprintf(stderr, "   %s was syncronized successfully to your Palm device\n\n", argv[j]);
		}

		preamble = add_title ? strlen(argv[j]) + 1 : 0;

		memo_buf = malloc(memo_size + preamble + 1);
		if (memo_buf == NULL) {
			perror("malloc()");
			exit(1);
		}

		if (preamble)
			sprintf(memo_buf, "%s\n", argv[j]);

		fread(memo_buf + preamble, memo_size, 1, f);

		memo_buf[memo_size + preamble] = '\0';

		dlp_WriteRecord(sd, (unsigned char) db, 0, 0, category,
				(unsigned char *) memo_buf, -1, 0);
		free(memo_buf);
		fclose(f);
	}

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
