/*
 * dlpsh.c: DLP Command Shell
 *
 * (c) 1996, 2000, The pilot-link Team
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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>		/* free() */
#include <ctype.h>

#include "pi-source.h"
#include "pi-dlp.h"
#include "pi-serial.h"
#include "pi-header.h"

#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#define TICKLE_INTERVAL 7

struct pi_socket *ticklish_pi_socket;

/* Declare prototypes */
int 	df_fn(int sd, int argc, char *argv[]),
	exit_fn(int sd, int argc, char *argv[]),
	help_fn(int sd, int argc, char *argv[]),
	ls_fn(int sd, int argc, char *argv[]),
	rm_fn(int sd, int argc, char *argv[]),
	time_fn(int sd, int argc, char *argv[]),
	user_fn(int sd, int argc, char *argv[]);

char 	*strtoke(char *str, char *ws, char *delim),
	*timestr(time_t t);

void exit_func(void);
void handle_user_commands(int sd);

typedef int (*cmd_fn_t) (int, int, char **);

int pilot_connect(const char *port);
static void Help(char *progname);

static const char *optstring = "hp:";

struct option options[] = {
	{"help", no_argument,       NULL, 'h'},
	{"port", required_argument, NULL, 'p'},
	{NULL,   0,                 NULL, 0}
};

struct Command {
	char *name;
	cmd_fn_t func;
};

struct Command command_list[] = {
	{"df",   df_fn},  
	{"help", help_fn},
	{"ls",   ls_fn},  
	{"exit", exit_fn},
	{"quit", exit_fn},
	{"rm",   rm_fn},
	{"time", time_fn},
	{"user", user_fn},
	{NULL,   NULL}
};

int main(int argc, char *argv[])
{
	int 	c,
		sd = -1;

	char 	*progname = argv[0],
		*port = NULL;

	while ((c = getopt(argc, argv, optstring)) != -1) {
		switch (c) {

		  case 'h':
			  Help(progname);
			  exit(0);
		  case 'p':
			  port = optarg;
			  break;
		  default:
		}
	}

	if (argc < 2 && !getenv("PILOTPORT")) {
		PalmHeader(progname);
	} else if (port == NULL && getenv("PILOTPORT")) {
		port = getenv("PILOTPORT");
	}

	if (port == NULL && argc > 1) {
		printf("\nERROR: At least one command parameter of '-p <port>' must be set, or the\n"
		       "environment variable $PILOTPORT must be used if '-p' is omitted or missing.\n");
		exit(1);
	} else if (port != NULL) {

		sd = pilot_connect(port);

		/* Did we get a valid socket descriptor back? */
		if (dlp_OpenConduit(sd) < 0) {
			exit(1);
		} else {
			printf("\nWelcome to the DLP Shell\n"
			       "Type 'help' for additional information\n\n");

			/* Stayin' alive, stayin' alive...
			   Normally we tickle to keep the port open and listening, 
			   but since we're removing PADP support, we have to find
			   another way to do this cleanly. Maybe pilot_connect() 
			   has to go... 
			*/			
			pi_watchdog(sd, TICKLE_INTERVAL);

			handle_user_commands(sd);

			return 0;
		}
	}
	return 0;
}

/***********************************************************************
 *
 * Function:    df_fn
 *
 * Summary:     Simple dump of CardInfo, which includes the RAM/ROM 
 * 		amounts free and used. 
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int df_fn(int sd, int argc, char *argv[])
{
	struct CardInfo Card;

	Card.card = -1;
	Card.more = 1;

	while (Card.more) {
		if (dlp_ReadStorageInfo(sd, Card.card + 1, &Card) < 0)
			break;

		printf("Filesystem           1k-blocks      Used   Available     Used     Total\n");
		printf("/dev/ROM               %7lu", Card.romSize);
                printf("       n/a     %7lu      n/a     %4luk\n", Card.romSize, Card.romSize/1024);
		printf("/dev/RAM               %7lu   %7lu     %7lu     %3ld%%     %4luk\n", 
			Card.ramSize, (Card.ramSize - Card.ramFree), Card.ramFree, 
			((Card.ramSize - Card.ramFree) * 100) / Card.ramSize, Card.ramSize/1024);
		printf("Total (ROM + RAM)     %8lu   %7lu         n/a      n/a    %5luk\n\n", 
			(Card.romSize + Card.ramSize), (Card.romSize + Card.ramSize)-Card.ramFree, 
			(Card.romSize + Card.ramSize)/1024);
	}
	return 0;
}

/***********************************************************************
 *
 * Function:    help_fn
 *
 * Summary:     Handle the parsing of -h inside dlpsh>
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int help_fn(int sd, int argc, char *argv[])
{

/* Let's figure out a better way to do this to automagically 
   "self-document" arguments

	int inc;
	for (inc = 0; command_list[inc].name != NULL; inc++) {
		printf("%s\t\n", command_list[inc].name);
	}
*/

	printf("Built-in commands:\n\n"
	       "user     = print the currently set User information\n"
	       "ls       = used with -l and -r to provide long and ROM file lists\n"
	       "df       = display how much RAM and ROM is free on your device\n"
	       "time     = set the time on the Palm using the desktop time\n"
	       "rm       = remove a file, delete it entirely from the Palm device\n"
	       "help     = You Are Here(tm)\n"
	       "quit     = exit the DLP Protocol Shell\n\n"
	       "Use '<command> -help' for more granular syntax and switches\n\n");
	return 0;
}

/***********************************************************************
 *
 * Function:    ls_fn
 *
 * Summary:     Similar to unix ls, lists files with -l and -r
 *
 * Parmeters:   -l for long and -r for ram
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int ls_fn(int sd, int argc, char *argv[])
{
	int 	c,
		cardno,
		flags,
		ret,
		start,
		lflag = 0,
		rom_flag = 0;

#ifdef sun
	extern char *optarg;
	extern int optind;
#endif

	optind = 0;
	while ((c = getopt(argc, argv, "lr")) != -1) {
		switch (c) {
		  case 'r':
			  rom_flag = 1;
			  break;
		  case 'l':
			  lflag = 1;
			  break;
		  default:
			  printf(" Usage: ls -l -r\n"
				 "   -l        = list all RAM databases using \"expanded\" format\n"
				 "   -r        = list all ROM databases and applications\n\n");
			  return 0;
		}
	}

	cardno = 0;
	if (rom_flag == 0)
		flags = 0x80;	/* dlpReadDBListFlagRAM */
	else
		flags = 0x40;	/* dlpReadDBListFlagROM */

	start = 0;
	for (;;) {
		struct DBInfo info;
		long tag;

		/* The databases are numbered starting at 0.  The first 12
		   are in ROM, and the rest are in RAM.  The high two bits
		   of the flags byte control wheter you see the ROM entries,
		   RAM entries or both.  start is the lowest index you want. 
		   So, we start with 0, but usually we want to see ram
		   entries, so it will return database number 12.  Then,
		   we'll ask for 13, etc, until we get the NotFound error
		   return. */
		ret = dlp_ReadDBList(sd, cardno, flags, start, &info);

		if (ret == -5 /* dlpRespErrNotFound */ )
			break;

		if (ret < 0) {
			printf("dlp_ReadDBList: err %d\n", ret);
			return -1;
		}


		printf("  Filename: \"%s\"\n", info.name);

		if (lflag) {
			printf("       More: 0x%x     Flags: 0x%-4x             Type: %.4s\n", info.more, info.flags, (char *) &tag);
			tag = htonl(info.creator);
			printf("  CreatorID: %s    Modification Number: %-4ld Version: %-2d\n", (char *) &tag, info.modnum, info.version);
			printf("Created: %19s\n", timestr(info.createDate));
			printf("Backup : %19s\n", timestr(info.backupDate));
			printf("Modify : %19s\n\n", timestr(info.modifyDate));
		}

		if (info.index < start) {
			/* avoid looping forever if we get confused */
			printf("error: index backs up\n");
			break;
		}

		start = info.index + 1;
	}

	return 0;
}

/***********************************************************************
 *
 * Function:    rm_fn
 *
 * Summary:     Similar to unix rm, deletes a database from the Palm
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int rm_fn(int sd, int argc, char *argv[])
{
	int 	cardno,
		ret;

	char 	*name;

	if (argc != 2) {
		printf("Delete the database or application on your Palm device by name\n\n"
		       "Usage: rm <dbname>\n");
		return (0);
	}

	name = argv[1];

	cardno = 0;
	if ((ret = dlp_DeleteDB(sd, cardno, name)) < 0) {
		if (ret == dlpErrNotFound) {
			printf("%s: not found\n", name);
			return (0);
		}
		printf("%s: remove error %d\n", name, ret);
		return (0);
	}
	printf("Application %s successfully removed\n", name);

	return 0;
}

/***********************************************************************
 *
 * Function:    time_fn
 *
 * Summary:     ntpdate-style function for setting Palm time from
 *		desktop clock
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int time_fn(int sd, int argc, char *argv[])
{
	int 	s;
	time_t 	ltime;
	struct 	tm *tm_ptr;
	char 	timebuf[80];

	time(&ltime);
	tm_ptr = localtime(&ltime);

	strftime(timebuf, 80, "Now setting Palm time from desktop to:"
			      "%a %b %d %H:%M:%S %Z %Y\n", tm_ptr);
	printf(timebuf);
	s = dlp_SetSysDateTime(sd, ltime);

	return 0;
}

/***********************************************************************
 *
 * Function:    user_fn
 *
 * Summary:     Set the username, UserID and PCID on the device, 
 *		similar to install-user, but interactive
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int user_fn(int sd, int argc, char *argv[])
{
	struct 	PilotUser User, nUser;

	char 	fl_name = 0,
		fl_uid = 0,
		fl_vid = 0, 
		fl_pid = 0;
	
	int 	chara,
		ret;

	optind = 0;

	while ((chara = getopt(argc, argv, "n:i:v:p:h")) != -1) {
		switch (chara) {
		  case 'n':
			  fl_name = 1;
			  strncpy(nUser.username, optarg, sizeof(nUser.username));
			  break;
		  case 'i':
			  fl_uid = 1;
			  nUser.userID = strtoul(optarg, NULL, 16);
			  break;
		  case 'v':
			  fl_vid = 1;
			  nUser.viewerID = strtoul(optarg, NULL, 16);
			  break;
		  case 'p':
			  fl_pid = 1;
			  nUser.lastSyncPC = strtoul(optarg, NULL, 16);
			  break;
		  case 'h':
			  printf(" Usage: user -n \"John Q. Public\" (full username in quotes)\n"
				 "             -i <UserID>\n"
				 "             -v <ViewerID>\n"
				 "             -p <PCid>\n");
			  return 0;
		}
	}

	ret = dlp_ReadUserInfo(sd, &User);
	if (ret < 0) {
		printf("dlp_ReadUserInfo: err %d\n", ret);
		return -1;
	}

	if (fl_name + fl_uid + fl_vid + fl_pid == 0) {
		printf(" Username = \"%s\"\n"
		       " UserID   = %08lx (%i)\n"
		       " ViewerID = %08lx (%i)\n"
		       " PCid     = %08lx (%i)\n", User.username,
		       User.userID, (int) User.userID,
		       User.viewerID, (int) User.viewerID,
		       User.lastSyncPC, (int) User.lastSyncPC);
		return 0;
	}

	if (fl_name)
		strncpy(User.username, nUser.username, sizeof(User.username));
	if (fl_uid)
		User.userID = nUser.userID;
	if (fl_vid)
		User.viewerID = nUser.viewerID;
	if (fl_pid)
		User.lastSyncPC = nUser.lastSyncPC;

	User.successfulSyncDate = time(NULL);
	User.lastSyncDate = User.successfulSyncDate;

	ret = dlp_WriteUserInfo(sd, &User);
	if (ret < 0) {
		printf("dlp_WriteUserInfo: err %d\n", ret);
		return -1;
	}

	return 0;
}

/***********************************************************************
 *
 * Function:    timestr
 *
 * Summary:     Build an ISO-compliant time string for use later
 *
 * Parmeters:   None
 *
 * Returns:     String containing the proper time
 *
 ***********************************************************************/
char *timestr(time_t t)
{
	struct tm tm;
	static char buf[50];

	tm = *localtime(&t);
	sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
	return (buf);
}




/***********************************************************************
 *
 * Function:    handle_user_commands
 *
 * Summary:     Parse user commands and arguments
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void handle_user_commands(int sd)
{
#ifdef HAVE_READLINE

/* A safer alternative
	char 	*line = (char *)malloc(256*sizeof(char)),
 */
	char 	*line,
		*prompt = "dlpsh> ";
#else
	char 	buf[256];
#endif

	char 	*argv[32];
	int 	argc,
		inc;

	for (;;) {
		fflush(stdout);

#ifdef HAVE_READLINE
	line = readline(prompt);
	if (line && *line)	/* skip blanks */
		add_history(line);
	if (!line)
		break;

	argc = 0;
	
	/* Changing input? BAD BAD BAD... -DD */
	argv[0] = strtoke(line, " \t\n", "\"'");
#else
	printf("dlpsh> ");
	
	if (fgets(buf, 256, stdin) == NULL)
		break;

	argc = 0;
	argv[0] = strtoke(buf, " \t\n", "\"'");
#endif

	while (argv[argc] != NULL) {
		argc++;

		/* Tsk, tsk. Changing the input again! -DD */
		argv[argc] = strtoke(NULL, " \t\n", "\"'");
	}

	if (argc == 0)
		continue;

	for (inc = 0; command_list[inc].name != NULL; inc++) {
		if (strcasecmp(argv[0], command_list[inc].name) == 0) {
			command_list[inc].func(sd, argc, argv);
		}
	}

#ifdef HAVE_READLINE
	free(line);
#endif
	}
	printf("\n");
}

/* functions for builtin commands */
int exit_fn(int sd, int argc, char *argv[])
{
#ifdef DEBUG
	fprintf(stderr,
		"\n\n================== EXITING ===================\n\n");
#endif
	printf("Exiting.\n");
	dlp_AddSyncLogEntry(sd, "dlpsh, DLP Protocol Shell ended.\n"
				"Thank you for using pilot-link.\n");
	dlp_EndOfSync(sd, 0);
	pi_close(sd);
	exit(0);
}

/***********************************************************************
 *
 * Function:    strtoke
 *
 * Summary:     Strip out path delimiters in arguments and filenames
 *
 * Parmeters:   None
 *
 * Returns:     Input string minus path delimiters (ala basepath)
 *
 ***********************************************************************/
char *strtoke(char *str, char *ws, char *delim)
{
	int 		inc;
	static char 	*s,
			*start;


	if (str != NULL) {
		s = str;
	}

	inc = strspn(s, ws);
	s += inc;
	start = s;

	if (*s == '\0') {
		return NULL;
	} else if (strchr(delim, *s) != NULL) {
		start++;
		s = strchr(s + 1, *s);
		s[inc] = '\0';
		s++;
	} else {
		inc = strcspn(s, ws);
		s[inc] = '\0';
		s += inc + 1;
	}

	return start;
}

static void Help(char *progname)
{
	printf("   An interactive DLP Protocol Shell for your Palm device\n\n"
	       "   Usage: %s -p <port>\n\n"
	       "   Options:\n"
	       "     -p <port>    Use device file <port> to communicate with Palm\n"
	       "     -h           Display this information\n\n"
	       "   dlpsh can query many different types of information from your Palm\n"
	       "   device, such as user memory capacity, and setting the time, as well as\n"
	       "   other useful functions. While in dlpsh, type 'help' for more options.\n\n"
	       "   Example: %s -p /dev/pilot\n\n", progname, progname);
	return;
}
