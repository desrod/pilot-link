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

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-header.h"

int pilot_connect(const char *port);

struct option options[] = {
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
	{"port",        required_argument, NULL, 'p'},
	{"user",        required_argument, NULL, 'u'},
	{"userid",      required_argument, NULL, 'i'},
	{NULL,          0,                 NULL, 0}
};

static const char *optstring = "hvp:u:i:";

static void Help(char *progname)
{
	printf("   Assigns your Palm device a Username and unique UserID\n\n"
	       "   Usage: %s -p <port> -u \"User name\" -i <userid>\n"
	       "                       -o <hostname> -a <ip> -n <subnet>\n"
	       "   Options:\n"
	       "     -p <port>         Use device file <port> to communicate with Palm\n"
	       "     -u <username>     Your username, use quotes for spaces (see example)\n"
	       "     -i <userid>       A 5-digit numeric UserID, required for PalmOS\n"
	       "     -h --help         Display this information\n"
	       "     -v --version      Display version information\n\n"
	       "   Examples: %s -p /dev/pilot -u \"John Q. Public\" -i 12345\n"
	       "             %s -p /dev/pilot -o Host -a 192.168.1.1 -n 255.255.255.0\n\n",
	       progname, progname, progname);
	return;
}

int main(int argc, char *argv[])
{
	int 	c,		/* switch */
		sd 		= -1;
	char 	*progname 	= argv[0],
		*port 		= NULL,
		*user 		= NULL,
		*userid 	= NULL;
	
	struct 	PilotUser 	User;

	opterr = 0;

	while ((c = getopt_long(argc, argv, optstring, options, NULL)) != -1) {
		switch (c) {

		case 'h':
			Help(progname);
			return 0;
		case 'v':
			PalmHeader(progname);
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
		}
	}
	
	if (optind < 2)
		PalmHeader(progname);
		exit(0);
	
	sd = pilot_connect(port);
	if (sd < 0)
		goto error;

	if (dlp_ReadUserInfo(sd, &User) < 0)
		goto error_close;

	if (!user && !userid) {
		printf("   Palm user: %s\n", User.username);
		printf("   UserID:    %ld \n", User.userID);   
	}
	
	/* Let's make sure we have valid arguments for these
	   before we write the data to the Palm */
	if (user != NULL)
		strncpy(User.username, user, sizeof(User.username) - 1);

	if (userid != NULL)
		User.userID = atoi(userid);
	
	User.lastSyncDate = time(NULL);

	if (dlp_WriteUserInfo(sd, &User) < 0)
		goto error_close;

	if (user != NULL)
		printf("   Installed User Name: %s\n", User.username);
	if (userid != NULL)
		printf("   Installed User ID: %ld \n", User.userID);
	printf("\n");
	
	if (dlp_AddSyncLogEntry(sd, "install-user, exited normally.\n"
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
	perror("   ERROR:");
	fprintf(stderr, "\n");

	return -1;
}
