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
int pilot_connect(const char *port);
int usage(char *progname);

#define PILOTPORT "/dev/pilot"

static void Help(char *progname)
{
	printf("Usage: %s -p <port> [-qrt] [-c category] file [file] ...\n"
	       "       -q = do not prompt for HotSync button press\n"
	       "       -r = replace all memos in specified category\n"
	       "       -t = use filename as memo title\n", progname);

	return;
}

int main(int argc, char *argv[])
{
	int 	add_title	= 0,
		category,
		ch,
		db,
		inc,
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
	
	while ((ch = getopt(argc, argv, "c:p:qrt")) != -1)
		switch (ch) {
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
		}

	if (replace_category && !category_name) {
		printf("%s: memo category required when specifying replace\n",
			progname);
		Help(progname);
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
		for (inc = 0; inc < 16; inc++)
			if (!strcasecmp
			    (mai.category.name[inc], category_name)) {
				category = inc;
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

	for (inc = 0; inc < argc; inc++) {

		f = fopen(argv[inc], "r");
		if (f == NULL) {
			printf("%s: cannot open %s (%s), skipping...\n",
				progname, argv[inc], strerror(errno));
			continue;
		}

		fseek(f, 0, SEEK_END);
		memo_size = ftell(f);
		fseek(f, 0, SEEK_SET);

		preamble = add_title ? strlen(argv[inc]) + 1 : 0;

		memo_buf = (char *) malloc(memo_size + preamble + 1);
		if (memo_buf == NULL) {
			perror("malloc()");
			exit(1);
		}

		if (preamble)
			sprintf(memo_buf, "%s\n", argv[inc]);

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

	/* All of the following code is now unnecessary, but harmless */

	dlp_EndOfSync(sd, 0);
	pi_close(sd);

	return 0;
 
 error_close:
	pi_close(sd);
	
 error:
	perror("   ERROR");
	fprintf(stderr, "\n");

	return -1;
}
