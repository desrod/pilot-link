/*
 * install-todolist.c:  Palm ToDo list installer
 *
 * Copyright 2002, Martin Fick, based on code in install-todos by Robert A.
 * 		   Kaplan
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

/* 12-27-2003:
   FIXME: Crash when using the '-d MM/DD/YYYY' specifier */

#include "getopt.h"
#include <stdio.h>
#include <stdlib.h>

#include "pi-source.h"
#include "pi-dlp.h"
#include "pi-todo.h"
#include "pi-header.h"

#ifdef __GLIBC__
#define _XOPEN_SOURCE /* glibc2 needs this */
#endif

#define DATE_STR_MAX 1000

struct option options[] = {
        {"port",        required_argument, NULL, 'p'},
        {"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
        {"priority",    required_argument, NULL, 'P'},
        {"due",         required_argument, NULL, 'd'},
        {"completed",   required_argument, NULL, 'c'},
        {"note",        required_argument, NULL, 'n'},
        {"file",        required_argument, NULL, 'f'},
        {NULL,          0,                 NULL, 0}
};

static const char *optstring = "p:hvd:P:cn:f:";

char *strptime(const char *s, const char *format, struct tm *tm);


/***********************************************************************
 *
 * Function:    read_file
 *
 * Summary:     
 *
 * Parameters:  
 *
 * Returns:     
 *
 ***********************************************************************/
char *read_file(char *filename)
{
	FILE	*f;
	int	filelen;
	char	*file_text 	= NULL;

	f = fopen(filename, "r");
	if (f == NULL) {
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	fseek(f, 0, SEEK_END);
	filelen = ftell(f);
	fseek(f, 0, SEEK_SET);

	file_text = (char *) malloc(filelen + 1);
	if (file_text == NULL) {
		perror("malloc()");
		exit(EXIT_FAILURE);
	}

	fread(file_text, filelen, 1, f);

	return file_text;
}


/***********************************************************************
 *
 * Function:    install_ToDo
 *
 * Summary:     
 *
 * Parameters:  
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void install_ToDo(int sd, int db, struct ToDo todo)
{
	int 	ToDo_size;

	unsigned char ToDo_buf[0xffff];
	char  time[DATE_STR_MAX];

	printf("Indefinite:  %i\n", todo.indefinite);
	if (!todo.indefinite)
		strftime(time, DATE_STR_MAX, "%a %b %d %H:%M:%S %Z %Y", &todo.due);
	printf("Due Date:    %s\n", time);
	printf("Priority:    %i\n", todo.priority);
	printf("Complete:    %i\n", todo.complete);
	printf("Description: %s\n\n", todo.description);
	printf("Note:        %s\n", todo.note);

	ToDo_size = pack_ToDo(&todo, ToDo_buf, sizeof(ToDo_buf));

	dlp_WriteRecord(sd, db, 0, 0, 0, ToDo_buf, ToDo_size, 0);

	return;
}

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
static void display_help(const char *progname)
{
	printf("   Updates the Palm ToDo list with a single new entry\n\n");
	printf("   Usage: %s [-pdycnft] command(s) [item]\n", progname);
	printf("   Options:\n");

	printf("     -p, --port <port>       Use device file <port> to communicate with Palm\n");
	printf("     -P, --priority <value>  The Priority (default 4)\n");
	printf("     -d, --due <duedate>     The due Date MM/DD/YYYY (default blank)\n");
	printf("     -c, --completed         Mark the item complete (default is incomplete)\n");
	printf("     -n, --note <note>       The Note text (single string)\n");
	printf("     -f, --file <filename>   A local filename containing the Note text\n");
	printf("     -h, --help              Display this information\n\n");
	printf("   Examples:\n");
	printf("     %s -p /dev/pilot -n 'Buy Milk' 'Go shopping, see note for items'\n", progname);
	printf("     %s -p /dev/pilot -f ShoppingList.txt 'Go to supermarket'\n\n", progname);

        return;
}

int main(int argc, char *argv[])
{
	int 	c,
		db,
		sd	= -1;	/* switch */

	char 	*progname 	= argv[0],
		*port		= NULL;

	struct 	PilotUser User;
	struct 	ToDo todo;

	/*  Setup some todo defaults */
	todo.indefinite  = 1;
	todo.priority    = 1;
	todo.complete    = 0;
	todo.description = "";
	todo.note        = "";
	todo.due.tm_sec  = 0;
	todo.due.tm_min  = 0;
	todo.due.tm_hour = 0;

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
		case 'P': /* Priority */
			  todo.priority = atoi(optarg);
			  break;
		case 'd': /* Due Date */
			  strptime(optarg, "%m/%d/%Y", &todo.due);
			  todo.indefinite = 0;
			  break;
		case 'c': /* Complete */
			  todo.complete = 1;
			  break;
		case 'n': /* Note */
			  todo.note = optarg;
			  break;
		case 'f': /* Note filename */
			  todo.note = read_file(optarg);
			  break;
		default:
		  	display_help(progname);
		  	return 0;
		}
	}

	if (argc < 2) {
		printf("   Insufficient or invalid options supplied.\n");
		printf("   Please use 'install-todo --help' for more info.\n\n");
		exit(EXIT_FAILURE);
	}

	if (optind < argc){
		todo.description = argv[optind];
	}

	sd = pilot_connect(port);

        if (sd < 0)
		exit(EXIT_FAILURE);

	/* Open the ToDo database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "ToDoDB", &db) < 0) {
		puts("Unable to open ToDo Database");
		dlp_AddSyncLogEntry(sd, "Unable to open ToDo Database.\n");
		exit(EXIT_FAILURE);
	}

	install_ToDo(sd, db, todo);

	dlp_CloseDB(sd, db);

	/* Tell the user who it is, with a different PC id. */
	User.lastSyncPC 	= 0x00010000;
	User.lastSyncDate = User.successfulSyncDate = time(0);
	dlp_WriteUserInfo(sd, &User);

	dlp_AddSyncLogEntry(sd, "Wrote ToDo entry to Palm.\n\n"
		"Thank you for using pilot-link.\n");

	dlp_EndOfSync(sd, 0);
	pi_close(sd);

	return 0;
}
