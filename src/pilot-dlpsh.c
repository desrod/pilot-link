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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <netinet/in.h>
#include <setjmp.h>

#include "pi-source.h"
#include "pi-dlp.h"
#include "pi-header.h"
#include "pi-error.h"
#include "pi-userland.h"

#ifdef HAVE_LIBREADLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#define TICKLE_INTERVAL 7

struct pi_socket *ticklish_pi_socket;


/* FIXME: This isn't really ideal, passing a struct in is
   probably better. Something like this:

   typedef struct { int sd; int argc; char **argv; } cmd_struct_t;

   int df_fn(cmd_struct_t *cmd_struct);
   int exit_fn(cmd_struct_t *cmd_struct);
   int ls_fn(cmd_struct_t *cmd_struct);
   int help_fn(cmd_struct_t *cmd_struct);
   int rm_fn(cmd_struct_t *cmd_struct);
   int time_fn(cmd_struct_t *cmd_struct);
   int user_fn(cmd_struct_t *cmd_struct);

   typedef int (*cmd_func_t)(cmd_struct_t *);

   struct Command {
           char *name;
           cmd_func_t func;
   };

   Knghtbrd: In the meantime, let's fix the warnings...
*/

int 	df_fn(int sd, int argc, const char *argv[]),
	reopen_fn(int sd, int argc, const char *argv[]),
	exit_fn(int sd, int argc, const char *argv[]),
	ls_fn(int sd, int argc, const char *argv[]),
	help_fn(int sd, int argc, const char *argv[]),
	rm_fn(int sd, int argc, const char *argv[]),
	time_fn(int sd, int argc, const char *argv[]),
	user_fn(int sd, int argc, const char *argv[]);

char 	*strtoke(char *str, const char *ws, const char *delim),
	*timestr(time_t t);

static jmp_buf main_jmp;

void exit_func(void);
void handle_user_commands(int sd);

typedef int (*cmd_fn_t) (int, int, const char **);

struct Command {
	char *name;
	cmd_fn_t func;
};

struct Command command_list[] = {
	{"df",   df_fn},
	{"help", help_fn},
	{"ls",   ls_fn},
	{"reopen", reopen_fn},
	{"exit", exit_fn},
	{"quit", exit_fn},
	{"rm",   rm_fn},
	{"time", time_fn},
	{"user", user_fn},
	{NULL, NULL}
};

/***********************************************************************
 *
 * Function:    df_fn
 *
 * Summary:     Simple dump of CardInfo, which includes the RAM/ROM
 * 		amounts free and used.
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int df_fn(int sd, int argc, const char *argv[])
{
	struct 	CardInfo Card;

	Card.card = -1;
	Card.more = 1;

	while (Card.more) {
		if (dlp_ReadStorageInfo(sd, Card.card + 1, &Card) < 0)
			break;

		printf("Filesystem           1k-blocks         Used   Available     Used     Total\n");
		printf("Card0: ROM           %9lu", Card.romSize);
                printf("          n/a   %9lu      n/a     %4luk\n",
			Card.romSize, Card.romSize/1024);

		printf("Card0: RAM           %9lu", Card.ramSize);
		printf("     %8lu    %8lu     %3ld%%     %4luk\n",
			(Card.ramSize - Card.ramFree), Card.ramFree,
			((Card.ramSize - Card.ramFree) * 100) / Card.ramSize,
			Card.ramSize/1024);

		printf("Total (ROM + RAM)     %8lu     %8lu         n/a      n/a    %5luk\n\n",
			(Card.romSize + Card.ramSize), (Card.romSize + Card.ramSize)-Card.ramFree,
			(Card.romSize + Card.ramSize)/1024);
	}
	return 0;
}


/***********************************************************************
 *
 * Function:    help_fn
 *
 * Summary:     Handle the parsing of -h inside 'dlpsh>'
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int help_fn(int sd, int argc, const char *argv[])
{
	printf("The current built-in commands are:\n\n");
	printf("   user           print the currently set User information\n");
	printf("   ls             used with -l and -r to provide long and ROM file lists\n");
	printf("   df             display how much RAM and ROM is free on your device\n");
	printf("   time,dtp       Desktop-to-Palm, set the time on the Palm from the desktop\n");
	printf("   rm             remove a file, delete it entirely from the Palm device\n");
	printf("   help           You Are Here(tm)\n");
	printf("   quit,exit      exit the DLP Protocol Shell\n\n");
	printf("Use '<command> -help' for more granular syntax and switches\n\n");

	return 0;
}


/***********************************************************************
 *
 * Function:    ls_fn
 *
 * Summary:     Similar to unix ls, lists files with -l and -r
 *
 * Parameters:  -l for long and -r for ram
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int ls_fn(int sd, int argc, const char *argv[])
{
	poptContext po;

	struct poptOption ls_options[] = {
		{"long", 	'l', POPT_ARG_NONE, NULL, 'l', "List all RAM databases using \"expanded\" format"},
		{"rom", 	'r', POPT_ARG_NONE, NULL, 'r', "List all ROM databases and applications"},
		{"help", 	'h', POPT_ARG_NONE, NULL, 'h', "Display this information"},
		POPT_AUTOHELP
        	{ NULL, 0, 0, NULL, 0 }
	} ;

	int 	c,		/* switch */
		cardno,
		flags,
		ret,
		start		= 0,
		lflag 		= 0,
		rom_flag 	= 0,
		i;
	pi_buffer_t *buf;

	optind = 0;

	po = poptGetContext("dlpsh", argc, argv, ls_options, 0);

	while ((c = poptGetNextOpt(po)) >= 0) {
		switch (c) {
		  case 'r':
			  rom_flag = 1;
			  break;
		  case 'l':
			  lflag = 1;
			  break;
		  case 'h':

			  printf("   List all files and databases stored on your Palm device\n\n"
			         "   Usage: ls [options]\n"
				 "   Options:\n"
				 "     -l --long      List all RAM databases using \"expanded\" format\n"
				 "     -r --rom       List all ROM databases and applications\n\n"
				 "     -h             Display this information\n\n"
				 "     Typing 'ls' on it's own will use the 'simple' listing format\n\n"
				 "     Example: ls -lr\n\n");
			  return 0;
		}
	}

	cardno = 0;
	if (rom_flag == 0)
		flags = 0x80;	/* dlpReadDBListFlagRAM */
	else
		flags = 0x40;	/* dlpReadDBListFlagROM */

	buf = pi_buffer_new (32 * sizeof (struct DBInfo));

	for (;;) {
		struct DBInfo info;
		long tag;
		char *a = (char *)&tag;

		/* The databases are numbered starting at 0.  The first 12
		   are in ROM, and the rest are in RAM.  The high two bits
		   of the flags byte control wheter you see the ROM entries,
		   RAM entries or both.  start is the lowest index you want.
		   So, we start with 0, but usually we want to see ram
		   entries, so it will return database number 12.  Then,
		   we'll ask for 13, etc, until we get the NotFound error
		   return. */
		ret = dlp_ReadDBList(sd, cardno, flags | dlpDBListMultiple, start, buf);

		if (ret == PI_ERR_DLP_PALMOS &&
				pi_palmos_error(sd) == dlpErrNotFound)
			break;

		if (ret < 0) {
			if (ret == PI_ERR_DLP_PALMOS)
				printf("dlp_ReadDBList: PalmOS err 0x%04x\n", pi_palmos_error(sd));
			else
				printf("dlp_ReadDBList: err %d\n", ret);
			pi_buffer_free (buf);
			return -1;
		}

		for (i = 0; i < (buf->used / sizeof(struct DBInfo)); i++) {
			memcpy (&info, buf->data + (i * sizeof(struct DBInfo)), sizeof (struct DBInfo));

			printf("%s\n", info.name);

			if (lflag == 1) {
				tag = htonl(info.type);
				printf("  More: 0x%x       Flags: 0x%-4x             Type: %.4s\n",
					info.more, info.flags, (char *) &tag);
				tag = htonl(info.creator);
				printf("  Creator: %c%c%c%c    Modification Number: %-4ld Version: %-2d\n",
					a[0], a[1], a[2], a[3], info.modnum, info.version);
				printf("  Created: %19s\n", timestr(info.createDate));
				printf("  Backup : %19s\n", timestr(info.backupDate));
				printf("  Modify : %19s\n\n", timestr(info.modifyDate));
			}

			if (info.index < start) {
				/* avoid looping forever if we get confused */
				printf("error: index backs up\n");
				break;
			}
		}

		if (info.index < start)
			break;

		start = info.index + 1;
	}
	pi_buffer_free (buf);
	return 0;
}


/***********************************************************************
 *
 * Function:    rm_fn
 *
 * Summary:     Similar to unix rm, deletes a database from the Palm
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int rm_fn(int sd, int argc, const char *argv[])
{
	int 	cardno,
		ret;

	const char 	*name;


	if (argc != 2) {
		printf("   Delete a application or database from your Palm device\n\n"
		       "   Usage: rm <dbname>\n"
		       "   Please note that no extension is required on 'dbname'\n\n"
		       "   Example: rm Address\n\n");
		return (0);
	}

	name 	= argv[1];
	cardno 	= 0;

	if ((ret = dlp_DeleteDB(sd, cardno, name)) < 0) {
		if (ret == PI_ERR_DLP_PALMOS && pi_palmos_error(sd) == dlpErrNotFound) {
			printf("%s: not found\n", name);
			return (0);
		}
		printf("%s: remove error %d\n", name, ret);
		return (0);
	}
	printf("File %s successfully removed\n", name);

	return 0;
}


/***********************************************************************
 *
 * Function:    time_fn
 *
 * Summary:     ntpdate-style function for setting Palm time from
 *		desktop clock
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int time_fn(int sd, int argc, const char *argv[])
{
	int 	s;
	time_t 	ltime;
	struct 	tm *tm_ptr;
	char 	timebuf[80];

	time(&ltime);
	tm_ptr = localtime(&ltime);

	strftime(timebuf, 80, "Now setting Palm time from desktop to: "
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
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int user_fn(int sd, int argc, const char *argv[])
{
	int 	c,		/* switch */
		ret,
		fl_name 	= 0,
		fl_uid 		= 0,
		fl_vid 		= 0,
		fl_pid 		= 0;

	struct 	PilotUser User, nUser;

	char *userID, *viewerID, *lastSyncPC;

	poptContext po;
	struct poptOption user_options[] = {
		{"user", 	'u', POPT_ARG_STRING, &nUser.username, 'u', "Set Username on the Palm device (use double-quotes)"},
		{"id", 		'i', POPT_ARG_STRING, &userID, 'i', "Set the numeric UserID on the Palm device"},
		{"viewid", 	'v', POPT_ARG_STRING, &viewerID, 'v', "Set the numeric ViewerID on the Palm device"},
		{"pcid", 	'p', POPT_ARG_STRING, &lastSyncPC, 'p', "Set the numeric PCID on the Palm device"},
		{"help", 	'h', POPT_ARG_NONE, NULL, 'h', "Display this information"},
		POPT_AUTOHELP
        	{ NULL, 0, 0, NULL, 0 }
		} ;

	optind = 0;
	po = poptGetContext("dlpsh", argc, argv, user_options, 0);

	while ((c = poptGetNextOpt(po)) >= 0) {
		switch (c) {
		  case 'u':
			  fl_name = 1;
			  break;
		  case 'i':
			  fl_uid = 1;
			  nUser.userID = strtoul(userID, NULL, 16);
			  break;
		  case 'v':
			  fl_vid = 1;
			  nUser.viewerID = strtoul(viewerID, NULL, 16);
			  break;
		  case 'p':
			  fl_pid = 1;
			  nUser.lastSyncPC = strtoul(lastSyncPC, NULL, 16);
			  break;
		  case 'h':
			  printf("   View or set user-specific Palm information\n\n");
			  printf("   Usage: user [options]\n");
			  printf("   Options:\n");
			  printf("     -u <Username>    Set Username on the Palm device (use double-quotes)\n");
			  printf("     -i <UserID>      Set the numeric UserID on the Palm device\n");
			  printf("     -v <ViewerID>    Set the numeric ViewerID on the Palm device\n");
			  printf("     -p <PCid>        Set the numeric PCID on the Palm device\n\n");
			  printf("     -h               Display this information\n\n");
			  printf("     Typing 'user' on it's own will display the currently stored User info\n\n");
			  printf("     Example: user -u \"John Q. Public\" -i 12345 -v 54321 -p 98765\n\n");
			  return 0;
		}
	}

	ret = dlp_ReadUserInfo(sd, &User);
	if (ret < 0) {
		printf("dlp_ReadUserInfo: err %d\n", ret);
		return -1;
	}

	if (fl_name + fl_uid + fl_vid + fl_pid == 0) {
		printf("   Username = \"%s\"\n"
		       "   UserID   = %08lx (%i)\n"
		       "   ViewerID = %08lx (%i)\n"
		       "   PCid     = %08lx (%i)\n", User.username,
		       User.userID, (int) User.userID,
		       User.viewerID, (int) User.viewerID,
		       User.lastSyncPC, (int) User.lastSyncPC);
		return 0;
	}

	if (fl_name)
		strncpy(User.username, nUser.username, sizeof(User.username));
		printf("   Username = \"%s\"\n", User.username);
	if (fl_uid)
		User.userID = nUser.userID;
		printf("   UserID   = %08lx (%i)\n", User.userID,
			(int) User.userID);
	if (fl_vid)
		User.viewerID = nUser.viewerID;
		printf("   ViewerID = %08lx (%i)\n", User.viewerID,
			(int) User.viewerID);
	if (fl_pid)
		User.lastSyncPC = nUser.lastSyncPC;
		printf("   PCid     = %08lx (%i)\n", User.lastSyncPC,
			(int) User.lastSyncPC);

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
 * Parameters:  None
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
 * Function:    parse_command
 *
 * Summary:     Parses and executes a command
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void parse_command(int sd, const char *cmd)
{
	char 	*argv[32];
	int 	argc,
		inc;
	char *cmd_dup = strdup(cmd);

	argc = 0;

	/* Changing input? BAD BAD BAD... -DD */
	argv[0] = strtoke(cmd_dup, " \t\n", "\"'");

	while (argv[argc] != NULL) {
		argc++;

		/* Tsk, tsk. Changing the input again! -DD */
		argv[argc] = strtoke(NULL, " \t\n", "\"'");
	}

	if (argc == 0) {
		free(cmd_dup);
		return;
	}

	for (inc = 0; command_list[inc].name != NULL; inc++) {
		if (strcasecmp(argv[0], command_list[inc].name) == 0) {
			command_list[inc].func(sd, argc, (const char **)argv);
		}
	}

	free(cmd_dup);
	return;
}

/***********************************************************************
 *
 * Function:    handle_single_command
 *
 * Summary:     Dispatch a command from the cmdline and exit
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void handle_single_command(int sd, char *cmd)
{
	parse_command(sd, cmd);
	/* FIXME: knghtbrd: log something or not? */
	dlp_EndOfSync(sd, 0);
	pi_close(sd);
	exit(0);
}


/***********************************************************************
 *
 * Function:    handle_user_commands
 *
 * Summary:     Read user commands interactively
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void handle_user_commands(int sd)
{
#ifdef HAVE_LIBREADLINE

/* A safer alternative
	char 	*line = (char *)malloc(256*sizeof(char)),
 */
	char 	*line,
		*prompt = "dlpsh> ";
#else
	char 	buf[256];
#endif

	printf("\nWelcome to the DLP Shell\n"
	       "Type 'help' for additional information\n\n");

	pi_watchdog(sd, TICKLE_INTERVAL);

	for (;;) {
		fflush(stdout);

#ifdef HAVE_LIBREADLINE
		line = readline(prompt);
		if (line == NULL)	/* user pressed ^d or so */
			break;
		if (*line)	/* skip blanks */
			add_history(line);

		parse_command(sd, line);

		free(line);
#else
		printf("dlpsh> ");

		if (fgets(buf, 256, stdin) == NULL)
			break;

		parse_command(sd, buf);
#endif
	}
	printf("\n");

	/* User must have pressed ^d or something */
	parse_command(sd, "exit");
}


/***********************************************************************
 *
 * Function:    exit_fn
 *
 * Summary:     Exit the DLP shell and write a log entry on the Palm
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int exit_fn(int sd, int argc, const char *argv[])
{
	printf("Exiting.\n");
	dlp_AddSyncLogEntry(sd, "dlpsh, the DLP Protocol Shell ended.\n\n"
				"Thank you for using pilot-link.\n");
	dlp_EndOfSync(sd, 0);
	pi_close(sd);
	exit(0);
}


/***********************************************************************
 *
 * Function:    reopen_fn
 *
 * Summary:     Reopen the connection.
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int reopen_fn(int sd, int argc, const char *argv[])
{
	printf("Reopening.\n");
	dlp_AddSyncLogEntry(sd, "dlpsh, the DLP Protocol Shell ended.\n\n"
				"Thank you for using pilot-link.\n");
	dlp_EndOfSync(sd, 0);
	pi_close(sd);

	sleep (2);

	longjmp (main_jmp, 1);
}


/***********************************************************************
 *
 * Function:    strtoke
 *
 * Summary:     Strip out path delimiters in arguments and filenames
 *
 * Parameters:  None
 *
 * Returns:     Input string minus path delimiters (ala basepath)
 *
 ***********************************************************************/
char *strtoke(char *str, const char *ws, const char *delim)
{
	int 		inc;
	static char 	*s = NULL,
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
		*s = '\0';
		s++;
	} else {
		inc = strcspn(s, ws);
		if (s[inc] == '\0') {
			s += inc;
		} else {
			s[inc] = '\0';
			s += inc + 1;
		}
	}

	return start;
}




int main(int argc, const char *argv[])
{
	int 	c		= -1,
		sd 		= -1;

	char
		*cmd		= NULL;

	poptContext po;

	enum { mode_none=0, mode_interactive=257, mode_command }
		run_mode = mode_none;

	struct poptOption options[] = {
		USERLAND_RESERVED_OPTIONS
		{"command", 	'c', POPT_ARG_STRING, &cmd, mode_command, "Execute <cmd> and exit immediately"},
		{"interactive",	'i', POPT_ARG_NONE, NULL, mode_interactive, "Enter interactive mode."},
		POPT_TABLEEND
	} ;

	po = poptGetContext("dlpsh", argc, argv, options, 0);
	poptSetOtherOptionHelp(po,"\n\n"
	"   An interactive Desktop Link Protocol (DLP) Shell for your Palm device.\n"
	"   dlpsh can query many different types of information from your Palm\n"
	"   device, such as username, memory capacity, set the time, as well as\n"
	"   other useful functions.\n\n"
	"   While inside the dlpsh shell, type 'help' for more options.\n\n");

	if (argc < 2) {
		poptPrintUsage(po,stderr,0);
		return 1;
	}

	while ((c = poptGetNextOpt(po)) >= 0) {
		switch(c)
		{
		case mode_command:
		case mode_interactive:
			if (run_mode == mode_none) {
				run_mode = c;
			} else {
				if (c != run_mode) {
					fprintf(stderr,"   ERROR: Specify exactly one of -c or -i\n");
					return 1;
				}
			}
			break;
		default:
			fprintf(stderr,"   ERROR: Unhandled option %d.\n",c);
			return 1;
		}
	}

	if ( c < -1) {
		plu_badoption(po,c);
	}

	if (mode_none == run_mode) {
		fprintf(stderr,"   ERROR: Specify exactly one of -c or -i\n");
		return 1;
	}

	setjmp (main_jmp);

	sd = plu_connect();
	if (sd < 0)
		goto error;

	if (dlp_OpenConduit(sd) < 0)
		goto error_close;

	if (cmd != NULL)
		handle_single_command(sd, cmd);
	else
		handle_user_commands(sd);

	return 0;

error_close:
	pi_close(sd);

error:
	return -1;
}
