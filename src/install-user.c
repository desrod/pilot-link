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

#include "getopt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-header.h"

/* Declare prototypes */
static void display_help(char *progname);
void print_splash(char *progname);
int pilot_connect(char *port);

struct option options[] = {
	{"port",        required_argument, NULL, 'p'},
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
	{"user",        required_argument, NULL, 'u'},
	{"id",          required_argument, NULL, 'i'},
	{0, 		0, 		   0, 	 0}
};

static const char *optstring = "p:hvu:i:";


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
	printf("   Assigns your Palm device a Username and unique UserID\n\n");
	printf("   Usage: %s -p <port> -u \"User name\" -i <userid>\n", progname);
	printf("                       -o <hostname> -a <ip> -n <subnet>\n");
	printf("   Options:\n");
	printf("     -p <port>         Use device file <port> to communicate with Palm\n");
	printf("     -u <username>     Set the username, use quotes for spaces (see example)\n");
	printf("     -i <userid>       A 5-digit numeric UserID, required for PalmOS\n");
	printf("     -h --help         Display this information\n");
	printf("     -v --version      Display version information\n\n");
	printf("   Examples: \n");
	printf("      install-user -p /dev/pilot -u \"John Q. Public\" -i 12345\n\n");

	return;
}

int main(int argc, char *argv[])
{
	int 	c,		/* switch */
		sd 		= -1;
	char 	*progname 	= argv[0],
		*port		= NULL,
		*user 		= NULL,
		*userid		= NULL;

	struct 	PilotUser 	User;

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
		case 'u':
			user = optarg;
			break;
		case 'i':
			userid = optarg;
			break;
		default:
			display_help(progname);
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
