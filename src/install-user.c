/* 
 * install-user.c:  Palm Username installer
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
#include <string.h>

#include "pi-source.h"
#include "pi-dlp.h"
#include "pi-header.h"

const char *port	= NULL,
	*user		= NULL,
	*userid		= NULL;


const struct poptOption options[] = {
        { "port",    'p', POPT_ARG_STRING, &port, 0, "Use device <port> to communicate with Palm", "<port>"},
        { "help",    'h', POPT_ARG_NONE,   0, 'h', "Display this information"},
        { "version", 'v', POPT_ARG_NONE,   0, 'v', "Display version information"},
        { "user",    'u', POPT_ARG_STRING, &user, 0, "Set the username, use quotes for spaces (see example)", "<username>"},
        { "id",      'i', POPT_ARG_STRING, &userid, 0, "A 5-digit numeric UserID, required for PalmOS", "<userid>"},
        POPT_AUTOHELP
        { NULL, 0, 0, NULL, 0 }
};

poptContext po;

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
static void display_help(const char *progname) {
                printf("Assigns your Palm device a Username and unique UserID\n\n");

                poptPrintHelp(po, stderr, 0);

                printf("\n  Examples: \n");
		printf("      %s -p /dev/pilot -H \"localhost\" -a 127.0.0.1 -n 255.255.255.0\n\n",
			basename(progname));
                exit(1);
}

int main(int argc, char *argv[])
{
	int 	sd 		= -1,
		po_err		= -1;

	struct 	PilotUser 	User;

	const char *progname	= argv[0];

        po = poptGetContext("install-user", argc, (const char **) argv, options, 0);
  
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
			poptPrintHelp(po, stderr, 0);
			return 0;
		}
	}

	sd = pilot_connect(port);
	if (sd < 0)
		goto error;

	if (dlp_ReadUserInfo(sd, &User) < 0)
		goto error_close;

	if (!user && !userid) {
		printf("   Palm user: %s\n", User.username);
		printf("   UserID:    %lu\n\n", User.userID);
		pi_close(sd);
		return 0;
	}

	if (userid)
		User.userID = abs(atoi(userid));

	if (user)
		strncpy(User.username, user, sizeof(User.username) - 1);
	
	User.lastSyncDate = time(NULL);

	if (dlp_WriteUserInfo(sd, &User) < 0)
		goto error_close;

	if (user)
		printf("   Installed User Name: %s\n", User.username);
	if (userid)
		printf("   Installed User ID: %lu\n", User.userID);
	printf("\n");
	
	if (dlp_AddSyncLogEntry(sd, "install-user exited normally.\n"
				    "Thank you for using pilot-link.\n") < 0)
		goto error_close;
	
	if (dlp_EndOfSync(sd, 0) < 0)
		goto error_close;

	if (pi_close(sd) < 0)
		goto error;

	return 0;

error_close:
	pi_close(sd);
	
error:
	return -1;
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
