/* 
 * install-memo.c: Palm memo pad installer
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
#include <errno.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-memo.h"

/* Declare prototypes */
int usage(char *progname);

static const char *optstring = "c:p:rthv";

struct option options[] = {
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
	{"port"   ,     required_argument, NULL, 'p'},
	{"category",    required_argument, NULL, 'c'},
	{"replace",     no_argument,       NULL, 'r'},
	{"title",       no_argument,       NULL, 't'},
	{NULL,          0,                 NULL, 0}
};

static void display_help(char *progname)
{
	printf("Usage: %s -p <port> [-qrt] [-c category] file [file] ...\n", progname);
	printf("       -r = replace all memos in specified category\n");
	printf("       -t = use filename as memo title\n");

	exit(0);
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

	FILE *f;
	
	while ((c = getopt_long(argc, argv, optstring, options, NULL)) != -1) {
		switch (c) {
		case 'c':
			category_name = optarg;
			break;
		case 'p':
			port = optarg;
			break;
		case 'r':
			replace_category++;
			break;
		case 't':
			add_title++;
			break;
		case 'h':
			display_help(progname);
			break;
			exit(0);
		
		}
	}
		
	argc -= optind;
	argv += optind;

	if (replace_category && !category_name) {
		printf("%s: memo category required when specifying replace\n",
			progname);
		display_help(progname);
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
		printf("Unable to open MemoDB");
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

		f = fopen(argv[j], "r");

		if (f == NULL) {
			printf("%s: Cannot open %s (%s), skipping...\n",
				progname, argv[j], strerror(errno));
			continue;
		}
		
		fseek(f, 0, SEEK_END);
		memo_size = ftell(f);
		fseek(f, 0, SEEK_SET);

		preamble = add_title ? strlen(argv[i]) + 1 : 0;

		memo_buf = (char *) malloc(memo_size + preamble + 1);
		if (memo_buf == NULL) {
			perror("malloc()");
			exit(1);
		}

		if (preamble)
			sprintf(memo_buf, "%s\n", argv[i]);

		fread(memo_buf + preamble, memo_size, 1, f);

		memo_buf[memo_size + preamble] = '\0';

		dlp_WriteRecord(sd, (unsigned char) db, 0, 0, category,
				(unsigned char *) memo_buf, -1, 0);
		printf("File %s successfully installed..\n", argv[j]);
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

	/* All of the following code is now unnecessary, but harmless */

	dlp_EndOfSync(sd, 0);
	pi_close(sd);

	return 0;
 
 error_close:
	pi_close(sd);
	
 error:
	return -1;
}
