/* 
 * install-todolist.c:  Palm ToDo list installer
 *
 * Copyright 1996, Robert A. Kaplan
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

#include "pi-source.h"
#include "pi-dlp.h"
#include "pi-todo.h"
#include "pi-header.h"

void install_ToDos(int sd, int db, char *filename)
{
	int 	ToDo_size,
		cLen		= 0,
		i		= 0,
		filelen;
	
        char 	*file_text 	= NULL,
		*cPtr 		= file_text,
		*begPtr 	= cPtr,
		note_text[] 	= "";
	
        unsigned char ToDo_buf[0xffff];
	
        struct 	ToDo todo;
        FILE 	*f;	
				
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

        cPtr = file_text;
        begPtr = cPtr;
        cLen = 0;
        i = 0;
	while (i < filelen) {
		i++;
		/* printf("c:%c.\n",*cPtr); */
		if (*cPtr == '\n') {
			todo.description = begPtr;
			/* replace CR with terminator */
			*cPtr = '\0';

			todo.priority 	= 4;
			todo.complete 	= 0;
			todo.indefinite = 1;
			/* now = time(0);
			   todo.due = *localtime(&now); */
			todo.note = note_text;
			ToDo_size =
			    pack_ToDo(&todo, ToDo_buf,
				      sizeof(ToDo_buf));
			printf("Description: %s\n", todo.description);

			/* printf("todobuf: %s\n",ToDo_buf);       */
			dlp_WriteRecord(sd, db, 0, 0, 0, ToDo_buf,
					ToDo_size, 0);
			cPtr++;
			begPtr = cPtr;
			cLen = 0;
		} else {
			cLen++;
			cPtr++;
		}
	}
	return;
}

static void display_help(const char *progname)
{
	printf("   Updates the Palm ToDo list with entries from a local file\n\n");
	printf("   Usage: %s -p <port> -f <filename>\n", progname);
	printf("   Options:\n");
	printf("   -p, --port <port>         Use device file <port> to communicate with Palm\n");
	printf("   -h, --help                Display help information for %s\n", progname);
	printf("   -v, --version             Display %s version information\n", progname);
	printf("   -f, --filename <filename> A local file with formatted ToDo entries\n\n");
	printf("   Examples: %s -p /dev/pilot -f $HOME/MyTodoList.txt\n\n", progname);
	printf("   The format of this file is a simple line-by-line ToDo task entry.\n");
	printf("   For each new line in the local file, a new task is created in the\n");
	printf("   ToDo database on the Palm.\n\n");

        return;
}

int main(int argc, const char *argv[])
{
	int 	c,		/* switch */
		db,
		sd 		= -1;

        const char
                *progname 	= argv[0];

        char 	*filename 	= NULL,
		*port 		= NULL;
	struct 	PilotUser User;
	
	poptContext po;
	
	struct poptOption options[] = {
        	{"port",	'p', POPT_ARG_STRING, &port, 0, "Use device <port> to communicate with Palm"},
	        {"help",	'h', POPT_ARG_NONE, NULL, 'h', "Display this information"},
                {"version",	'v', POPT_ARG_NONE, NULL, 'v', "Display version information"},
	        {"filename",	'f', POPT_ARG_STRING, &filename, 0, "A local file with formatted ToDo entries"},
        	POPT_AUTOHELP
                { NULL, 0, 0, NULL, 0 }
	};

	po = poptGetContext("install-todos", argc, argv, options, 0);

	while ((c = poptGetNextOpt(po)) >= 0) {
		switch (c) {

		case 'h':
			display_help(progname);
			return 0;
		case 'v':
			print_splash(progname);
			return 0;
		default:
			display_help(progname);
			return 0;
		}
	}
	
	if (filename == NULL) {
		printf("\n");
		printf("   ERROR: You must specify a filename to read ToDo entries from.\n");
		printf("   Please see %s --help for more information.\n\n", progname);
		exit(EXIT_FAILURE);
	}

	if (argc < 2 && !getenv("PILOTPORT")) {
		print_splash(progname);
	} else if (port == NULL && getenv("PILOTPORT")) {
		port = getenv("PILOTPORT");
	}

	if (port == NULL && argc > 1) {
		printf("\n");
		printf("   ERROR: At least one command parameter of '-p <port>' must be set, or the\n"
		     "environment variable $PILOTPORT must be used if '-p' is omitted or missing.\n\n");
		exit(EXIT_FAILURE);
	} else if (port != NULL) {
		
		sd = pilot_connect(port);

		/* Did we get a valid socket descriptor back? */
		if (dlp_OpenConduit(sd) < 0) {
			exit(EXIT_FAILURE);
		}
		
		/* Tell user (via Palm) that we are starting things up */
		dlp_OpenConduit(sd);
	
		/* Open the ToDo Pad's database, store access handle in db */
		if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "ToDoDB", &db) < 0) {
			puts("Unable to open ToDoDB");
			dlp_AddSyncLogEntry(sd, "Unable to open ToDoDB.\n");
			exit(EXIT_FAILURE);
		}

		/* Actually do the install here, passed a filename */
		install_ToDos(sd, db, filename);
		
		/* Close the database */
		dlp_CloseDB(sd, db);
	
		/* Tell the user who it is, with a different PC id. */
		User.lastSyncPC 	= 0x00010000;
		User.successfulSyncDate = time(NULL);
		User.lastSyncDate 	= User.successfulSyncDate;
	
		dlp_AddSyncLogEntry(sd, "Wrote ToDo list entries to Palm.\nThank you for using pilot-link.\n");
		dlp_EndOfSync(sd, 0);
		pi_close(sd);
	}
	return 0;
}
