/*
 * install-todo.c:  Palm ToDo list installer
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

#include "pi-userland.h"
#include <stdio.h>
#include <stdlib.h>

#include "pi-source.h"
#include "pi-dlp.h"
#include "pi-todo.h"
#include "pi-header.h"

poptContext po;

#ifndef HAVE_BASENAME
/* Yes, this isn't efficient... scanning the string twice */
#define basename(s) (strrchr((s), '/') == NULL ? (s) : strrchr((s), '/') + 1)
#endif

#define DATE_STR_MAX 1000

char *strptime(const char *s, const char *format, struct tm *tm);


/***********************************************************************
 *
 * Function:    read_file
 *
 * Summary:     Reads a file from disk to import into ToDoDB.pdb
 *
 * Parameters:  Pointer to a filename
 *
 * Returns:     Text inside the file, of course
 *
 ***********************************************************************/
int read_file(char *filename, char **text)
{
	FILE	*f;
	size_t	filelen;

	f = fopen(filename, "r");
	if (f == NULL) {
		fprintf(stderr,"   ERROR: Could not open '%s': %s\n",
			filename,strerror(errno));
		return -1;
	}

	fseek(f, 0, SEEK_END);
	filelen = ftell(f);
	fseek(f, 0, SEEK_SET);

	*text = (char *) malloc(filelen + 1);
	if (*text == NULL) {
		fprintf(stderr,"   ERROR: Could not allocate memory.\n");
		return -1;
	}

	fread(*text, filelen, 1, f);

	return 0;
}


/***********************************************************************
 *
 * Function:    install_ToDo
 *
 * Summary:     Adds the entry to ToDoDB.pdb
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
	char  duedate[DATE_STR_MAX];

	printf("Indefinite:  %i\n", todo.indefinite);
	/* Assume DATE_STR_MAX is at least 40 or so. */
	if (todo.indefinite == 0) {
		strftime(duedate, DATE_STR_MAX, "%a %b %d %H:%M:%S %Z %Y", &todo.due);
	} else {
		strncpy(duedate,"<none>",DATE_STR_MAX);
	}
	printf("Due Date:    %s\n", duedate);
	printf("Priority:    %i\n", todo.priority);
	printf("Complete:    %i\n", todo.complete);
	printf("Description: %s\n", todo.description);
	printf("Note:        %s\n", todo.note);

	ToDo_size = pack_ToDo(&todo, ToDo_buf, sizeof(ToDo_buf));

	dlp_WriteRecord(sd, db, 0, 0, 0, ToDo_buf, ToDo_size, 0);

	return;
}

int main(int argc, const char *argv[])
{
	int 	sd	= -1,
		db,
		po_err  = -1;

	char
		*due		= NULL,
		*description	= NULL,
		*note		= NULL,
		*filename	= NULL;
	int priority = 1;
	int completed = 0;

	struct 	PilotUser User;
	struct 	ToDo todo;

        const struct poptOption options[] = {
		USERLAND_RESERVED_OPTIONS
                { "priority",    'P', POPT_ARG_INT, &priority, 0, "The Priority (default is 1)", "[1|2|3|4|5]"},
                { "due",         'd', POPT_ARG_STRING, &due, 0, "The due Date of the item (default is no date)", "mm/dd/yyyy"},
                { "completed",   'c', POPT_ARG_NONE, &completed, 0, "Mark the item complete (default is incomplete)"},
		{ "description", 'D', POPT_ARG_STRING, &description, 0, "The title of the ToDo entry", "[title]"},
                { "note",        'n', POPT_ARG_STRING, &note, 0, "The Note(tm) text (a single string)", "<note>"},
                { "file",        'f', POPT_ARG_STRING, &filename, 0, "A local filename containing the Note text", "[filename]"},
		POPT_TABLEEND
        };


        po = poptGetContext("install-todo", argc, (const char **) argv, options, 0);
	poptSetOtherOptionHelp(po,
		" [-p port] <todo-options>\n\n"
		"   Updates the Palm ToDo list with a single new entry\n\n"
		"   Example:\n"
		"       -p /dev/pilot -n 'Buy Milk' -D 'Go shopping, see note for items'\n\n");

	if (argc < 2) {
		poptPrintUsage(po,stderr,0);
		return 1;
	}

        while ((po_err = poptGetNextOpt(po)) >= 0) {
		fprintf(stderr,"   ERROR: Unhandled option %d.\n",po_err);
		return 1;
	}

	if (po_err < -1) {
		plu_badoption(po,po_err);
	}

	if ((priority < 1) || (priority > 5)) {
		priority=1;
		fprintf(stderr,"   WARNING: Changed illegal priority to 1.\n");
	}

	/*  Setup some todo defaults */
	todo.indefinite  = 1;
	todo.priority    = priority;
	todo.complete    = completed;
	todo.description = description;
	if (filename) {
		int c = read_file(filename,&todo.note);
		if (c<0) {
			return 1;
		}
	}
	else {
		todo.note = note;
	}

	if (due != NULL) {
		strptime(due, "%m/%d/%Y", &todo.due);
		todo.indefinite = 0;
		todo.due.tm_sec  = 0;
		todo.due.tm_min  = 0;
		todo.due.tm_hour = 0;
	}

	sd = plu_connect();

        if (sd < 0)
	    return 1;

	/* Open the ToDo database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "ToDoDB", &db) < 0) {
		fprintf(stderr,"   ERROR: Unable to open ToDo Database on Palm.\n");
		dlp_AddSyncLogEntry(sd, "Unable to open ToDo Database.\n");
		pi_close(sd);
		return 1;
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

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
