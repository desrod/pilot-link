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

#include <stdio.h>
#include <stdlib.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-memo.h"

extern char *optarg;
extern int optind;

/* Declare prototypes */
int usage(char *progname);

#define PILOTPORT "/dev/pilot"

int usage(char *progname)
{
	fprintf(stderr, "usage: %s [-qrt] [-c category] [-p %s] file [file] ...\n",
		progname, TTYPrompt);
	fprintf(stderr, "       -q = do not prompt for HotSync button press\n");
	fprintf(stderr, "       -r = replace all memos in specified category\n");
	fprintf(stderr, "       -t = use filename as memo title\n");
	exit(2);
}

int main(int argc, char *argv[])
{
	int 	add_title,
		category,
		ch,
		db,
		inc,
		ReadAppBlock, 
		memo_size,
		preamble,
		quiet,
		replace_category,
		ret,
		sd;
	
	char 	*memo_buf,
		*progname,
		*category_name,
		buf[0xffff];
	
        struct 	pi_sockaddr addr;
        struct 	PilotUser U;
        struct 	MemoAppInfo mai;

	FILE *f;

	progname = argv[0];
	category_name = NULL;
	quiet = replace_category = add_title = 0;

	if (getenv("PILOTPORT")) {
		strcpy(addr.pi_device, getenv("PILOTPORT"));
	} else {
		strcpy(addr.pi_device, PILOTPORT);
	}

	while ((ch = getopt(argc, argv, "c:p:qrt")) != -1)
		switch (ch) {
		case 'c':
			category_name = optarg;
			break;
		case 'p':
			/* optarg is name of port to use instead of
			   $PILOTPORT or /dev/pilot */
			strcpy(addr.pi_device, optarg);
			break;
		case 'q':
			quiet++;
			break;
		case 'r':
			replace_category++;
			break;
		case 't':
			add_title++;
			break;
		case '?':
		default:
			usage(progname);
		}

	argc -= optind;
	argv += optind;

	if (argc < 1) {
		fprintf(stderr, "%s: insufficient number of arguments\n",
			progname);
		usage(progname);
	}

	if (replace_category && !category_name) {
		fprintf(stderr,
			"%s: memo category required when specifying replace\n",
			progname);
		usage(progname);
	}

	if (!quiet)
		printf
		    ("Please insert Palm in cradle on %s and press HotSync button.\n",
		     addr.pi_device);

	if (!(sd = pi_socket(PI_AF_PILOT, PI_SOCK_STREAM, PI_PF_DLP))) {
		perror("pi_socket");
		exit(1);
	}

	addr.pi_family = PI_AF_PILOT;

	ret = pi_bind(sd, (struct sockaddr *) &addr, sizeof(addr));
	if (ret == -1) {
		perror("pi_bind");
		exit(1);
	}

	ret = pi_listen(sd, 1);
	if (ret == -1) {
		perror("pi_listen");
		exit(1);
	}

	sd = pi_accept(sd, 0, 0);
	if (sd == -1) {
		perror("pi_accept");
		exit(1);
	}

	/* Ask the pilot who it is. */
	dlp_ReadUserInfo(sd, &U);

	/* Tell user (via Palm) that we are starting things up */
	dlp_OpenConduit(sd);

	/* Open the Memo Pad's database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "MemoDB", &db) < 0) {
		puts("Unable to open MemoDB");
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
			fprintf(stderr,
				"%s: category %s not found on Palm\n",
				progname, category_name);
			exit(2);
		}

		if (replace_category)
			dlp_DeleteCategory(sd, db, category);

	} else
		category = 0;	/* unfiled */

	for (inc = 0; inc < argc; inc++) {

		f = fopen(argv[inc], "r");
		if (f == NULL) {
			fprintf(stderr,
				"%s: cannot open %s (%s), skipping...\n",
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
	U.lastSyncPC = 0x00010000;
	U.successfulSyncDate = time(NULL);
	U.lastSyncDate = U.successfulSyncDate;
	dlp_WriteUserInfo(sd, &U);

	dlp_AddSyncLogEntry(sd, "Successfully wrote memo(s) to Palm.\n"
				"Thank you for using pilot-link.\n");

	/* All of the following code is now unnecessary, but harmless */

	dlp_EndOfSync(sd, 0);
	pi_close(sd);

	return 0;
}
