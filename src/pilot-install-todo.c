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

#include "getopt.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define DATE_STR_MAX 1000

#ifdef __GLIBC__
#define _XOPEN_SOURCE /* glibc2 needs this */
#endif

#include <time.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-todo.h"
#include "pi-header.h"

/* Declare prototypes */
static void display_help(char *progname);
void print_splash(char *progname);
int pilot_connect(char *port);
char *read_file(char *filename);
void install_ToDo(int sd, int db, struct ToDo todo);
void install(char *port, struct ToDo todo);

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

char *read_file(char *filename)
{
	FILE	*f;
	int	filelen;
	char	*file_text 	= NULL;

	f = fopen(filename, "r");
	if (f == NULL) {
		perror("fopen");
		exit(1);
	}

	fseek(f, 0, SEEK_END);
	filelen = ftell(f);
	fseek(f, 0, SEEK_SET);

	file_text = (char *) malloc(filelen + 1);
	if (file_text == NULL) {
		perror("malloc()");
		exit(1);
	}

	fread(file_text, filelen, 1, f);

	return file_text;
}

void install_ToDo(int sd, int db, struct ToDo todo)
{
	int 	ToDo_size;

	unsigned char ToDo_buf[0xffff];
	char  time[DATE_STR_MAX];

	printf("Indefinite:  %i\n", todo.indefinite);
	strftime(time, DATE_STR_MAX, "%a %b %d %H:%M:%S %Z %Y", &todo.due);
	printf("Due Date:    %s\n", time);
	printf("Priority:    %i\n", todo.priority);
	printf("Complete:    %i\n", todo.complete);
	printf("Description: %s\n\n", todo.description);
	printf("Note:        %s\n", todo.note);

	ToDo_size = pack_ToDo(&todo, ToDo_buf, sizeof(ToDo_buf));

	/* 
	printf("todobuf: %s\n",ToDo_buf);
	dlp_WriteRecord(sd, db, 0, 0, 0, ToDo_buf, ToDo_size, 0);
	*/

	return;
}

static void display_help(char *progname)
{
	printf("   Updates the Palm ToDo list with one new entry\n\n");

	printf("   Usage: %s [-pdycnNt] command(s) [item]\n", progname);
	printf("   Options:\n");
	printf("     -p <port>      Use device file <port> to communicate with Palm\n");
	printf("     -P <priority>  The Priority (default 4)\n");
	printf("     -d <duedate>   The due Date MM/DD/YYYY (default blank)\n");
	printf("     -c             Mark the item complete (default is incomplete)\n");
	printf("     -n <note>      The Note\n");
	printf("     -f <filename>  A local filename containing the Note text\n");
	printf("     -h             Display this information\n\n");
	printf("   Examples:\n");
	printf("     %s -y 1 'Buy Milk'\n", progname);
	printf("     %s -p /dev/pilot -N ShoppingList.txt 'Go to supermarket'\n\n", progname);

        exit(0);
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
	todo.indefinite = 1;
	todo.priority = 4;
	todo.complete = 0;
	todo.description = "";
	todo.note = "";

	while ((c = getopt_long(argc, argv, optstring, options, NULL)) != -1) {
		switch (c) {

		  case 'h': /* Help */
			  display_help(progname);
			  exit(0);
		  case 'p': /* Port */
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
                  case 'v':
                          print_splash(progname);
                          exit(0);
		}
	}

	/* Get Description */
	if (optind < argc){
		todo.description = argv[optind];
	}

	/* Look for other port settings */
	if (port == NULL && getenv("PILOTPORT")) {
		port = getenv("PILOTPORT");
	}
	if (port == NULL) {
		port = "/dev/pilot";
	}

	sd = pilot_connect(port);

	/* Did we get a valid socket descriptor back? */
	if (dlp_OpenConduit(sd) < 0) {
		exit(1);
	}

	/* Tell user (via Palm) that we are starting things up */
	dlp_OpenConduit(sd);

	/* Open the ToDo Pad's database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "ToDoDB", &db) < 0) {
		puts("Unable to open ToDoDB");
		dlp_AddSyncLogEntry(sd, "Unable to open ToDoDB.\n");
		exit(1);
	}

	/* Actually do the install here */
	install_ToDo(sd, db, todo);

	/* Close the database */
	dlp_CloseDB(sd, db);

	/* Tell the user who it is, with a different PC id. */
	User.lastSyncPC 	= 0x00010000;
	User.successfulSyncDate = time(NULL);
	User.lastSyncDate 	= User.successfulSyncDate;

	dlp_AddSyncLogEntry(sd, "Wrote ToDo entry to Palm.\nThank you for using pilot-link.\n");

	/* All of the following code is now unnecessary, but harmless */
	dlp_EndOfSync(sd, 0);
	pi_close(sd);

	return 0;
}
