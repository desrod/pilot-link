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


#include <signal.h>
#include <stdio.h>
#include <stdlib.h>             /* free() */
#include <ctype.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-padp.h"
#include "pi-dlp.h"
#include "pi-serial.h"
#include "pi-header.h"

#ifdef READLINE_2_1
#include <readline/readline.h>
#include <readline/history.h>
#endif

#define TICKLE_INTERVAL 7

int sd;
struct pi_socket *ticklish_pi_socket;

/* Declare prototypes */
int user_fn(int sd, int argc, char **argv);
int ls_fn(int sd, int argc, char **argv);
int df_fn(int sd, int argc, char **argv);
int time_fn(int sd, int argc, char **argv);
int rm_fn(int sd, int argc, char **argv);
int help_fn(int sd, int argc, char **argv);
int exit_fn(int sd, int argc, char **argv);
char *strtoke(char *str, char *ws, char *delim);
void exit_func(void);
void sigexit(int sig);
char *timestr(time_t t);

void handle_user_commands(int sd);

typedef int (*cmd_fn_t) (int, int, char **);

struct Command {
	char *name;
	cmd_fn_t func;
};

struct Command command_list[] = {
	{"user", user_fn},
	{"ls", ls_fn},
	{"df", df_fn},
	{"time", time_fn},
	{"rm", rm_fn},
	{"help", help_fn},
	{"quit", exit_fn},
	{NULL, NULL}
};

/* functions for builtin commands */
int exit_fn(int sd, int argc, char **argv)
{
	exit_func();
	sigexit(0);
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
int help_fn(int sd, int argc, char **argv)
{
	int i;

	printf("Built-in commands:\n\n");
	for (i = 0; command_list[i].name != NULL; i++) {
		printf("%s\t\n", command_list[i].name);
	}
	printf("\n");
	printf("Use '<command> -help' for additional syntax\n\n");
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
int user_fn(int sd, int argc, char **argv)
{
	struct PilotUser U, nU;
	char fl_name = 0, fl_uid = 0, fl_vid = 0, fl_pid = 0;
	int c;
	int ret;

	extern char *optarg;
	extern int optind;

	optind = 0;
	while ((c = getopt(argc, argv, "n:i:v:p:h")) != -1) {
		switch (c) {
		case 'n':
			fl_name = 1;
			strncpy(nU.username, optarg, sizeof(nU.username)-1);
			break;
		case 'i':
			fl_uid = 1;
			nU.userID = strtoul(optarg, NULL, 16);
			break;
		case 'v':
			fl_vid = 1;
			nU.viewerID = strtoul(optarg, NULL, 16);
			break;
		case 'p':
			fl_pid = 1;
			nU.lastSyncPC = strtoul(optarg, NULL, 16);
			break;
		case 'h':
		case '?':
			printf("   Usage: user [ -n <username> ]\n");
			printf("            [ -i <user id> ]\n");
			printf("            [ -v <viewer id> ]\n");
			printf("            [ -p <pc id> ]\n");
			return 0;
		}
	}

	ret = dlp_ReadUserInfo(sd, &U);
	if (ret < 0) {
		printf("dlp_ReadUserInfo: err %d\n", ret);
		return -1;
	}

	if (fl_name + fl_uid + fl_vid + fl_pid == 0) {
		printf("username = \"%s\"\n", U.username);
		printf("userID = %08lx (%i)   viewerID = %08lx (%i)   PCid = %08lx (%i)\n",
			U.userID, (int)U.userID, U.viewerID, (int)U.viewerID, U.lastSyncPC,
			(int)U.lastSyncPC);
		return 0;
	}

	if (fl_name)
		strncpy(U.username, nU.username, sizeof(U.username)-1);
	if (fl_uid)
		U.userID = nU.userID;
	if (fl_vid)
		U.viewerID = nU.viewerID;
	if (fl_pid)
		U.lastSyncPC = nU.lastSyncPC;

	U.successfulSyncDate = time(NULL);
	U.lastSyncDate = U.successfulSyncDate;

	ret = dlp_WriteUserInfo(sd, &U);
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
 * Function:    df_fn
 *
 * Summary:     Simple dump of CardInfo, which includes the RAM/ROM 
 * 		and manufacturer
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int df_fn(int sd, int argc, char **argv)
{
	struct CardInfo C;
	int i;

	C.card = -1;
	C.more = 1;

	while (C.more) {
		if (dlp_ReadStorageInfo(sd, C.card + 1, &C) < 0)
			break;

		printf("ROM      : %lu bytes  (%luk) \n", C.romSize,
		       (C.romSize / 1024));
		printf("RAM      : %lu bytes  (%luk) \n", C.ramSize,
		       (C.ramSize / 1024));
		printf("Free RAM : %lu bytes  (%luk) \n", C.ramFree,
		       (C.ramFree / 1024));
		for(i=0;i<33;i++) printf("-");
		printf("\n");
		printf("Total    : %lu bytes (%luk) \n\n",
		       (C.romSize + C.ramSize),
		       ((C.romSize + C.ramSize) / 1024));
	}
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
int time_fn(int sd, int argc, char **argv)
{
	int s;
	time_t ltime;
	struct tm *tm_ptr;
	char c, timebuf[80];

	time(&ltime);

	tm_ptr = localtime(&ltime);

	c = *asctime(tm_ptr);
	s = dlp_SetSysDateTime(sd, ltime);

	strftime(timebuf, 80,
		 "Now setting Palm time from desktop to: %a %b %d %H:%M:%S %Z %Y\n",
		 tm_ptr);
	printf(timebuf);
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
int ls_fn(int sd, int argc, char **argv)
{
	int c;
	int cardno;
	int flags;
	int ret;
	int start;
	int lflag = 0;
	int rom_flag = 0;

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
			printf("   Usage: ls\n");
			printf("         -l    long format\n");
			printf("         -r    list rom instead of ram\n");
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

		printf("%.32s\n", info.name);
		if (lflag) {
			printf("  more 0x%x; ", info.more);
			printf("Flags 0x%x; ", info.flags);
			tag = htonl(info.type);
			printf("Type '%.4s'; ", (char *) &tag);
			tag = htonl(info.creator);
			printf("  CreatorID '%.4s';", (char *) &tag);
			printf(" Version %d;\n ", info.version);
			printf(" modnum %ld; ", info.modnum);
			printf("Created %s; ", timestr(info.createDate));
			printf("Modified %s;\n  ",
			       timestr(info.modifyDate));
			printf("Backed up %s; ", timestr(info.backupDate));
			printf("\n\n");
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
int rm_fn(int sd, int argc, char **argv)
{
	int cardno;
	int ret;
	char *name;

	if (argc != 2) {
		printf("Usage: rm database\n");
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

	return (0);
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

#ifdef READLINE_2_1
	char *line;
	static const char *prompt = "dlpsh> ";
#else
        char buf[256];
#endif

        char *argv[32];
        int argc;
	int i;

        for (;;) {
                fflush(stdout);

#ifdef READLINE_2_1
                line = readline(prompt);
                if (line && *line)      	/* skip blanks */
                        add_history(line);
                if (!line)
                        break;

                argc = 0;
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
                        argv[argc] = strtoke(NULL, " \t\n", "\"'");
                }

                if (argc == 0)
                        continue;

                for (i = 0; command_list[i].name != NULL; i++) {
                        if (strcasecmp(argv[0], command_list[i].name) == 0) {
                                command_list[i].func(sd, argc, argv);
                        }
                }

#ifdef READLINE_2_1
		free(line);
#endif
        }
        printf("\n");
}

int main(int argc, char **argv)
{
	struct pi_sockaddr addr;

	int ret;
	int sd;
	char *progname = argv[0];
	char *device = argv[1];

	PalmHeader(progname);

	if (argc != 2) {
		fprintf(stderr, "   Usage: %s %s\n\n", argv[0], TTYPrompt);
		exit(1);
	}

	/* Connect to the Palm device. */

	sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP);
	if (sd == -1) {
		perror("pi_socket");
		exit(1);
	}

	addr.pi_family = PI_AF_SLP;
	strncpy(addr.pi_device, device, sizeof(addr.pi_device));

	ret = pi_bind(sd, (struct sockaddr *) &addr, sizeof(addr));
	if (ret == -1) {
		fprintf(stderr, "\n   Unable to bind to port %s\n",
			device);
		perror("   pi_bind");
		fprintf(stderr, "\n");
		exit(1);
	}

	printf
	    ("   Port: %s\n\n   Please press the HotSync button now...\n\n",
	     device);

	ret = pi_listen(sd, 1);
	if (ret == -1) {
		fprintf(stderr, "\n   Error listening on %s\n", device);
		perror("   pi_listen");
		fprintf(stderr, "\n");
		exit(1);
	}

	sd = pi_accept(sd, 0, 0);
	if (sd == -1) {
		fprintf(stderr, "\n   Error accepting data on %s\n",
			device);
		perror("   pi_accept");
		fprintf(stderr, "\n");
		exit(1);
	}

	fprintf(stderr,
		"Connected... Welcome to the DLP Shell\nType 'help' for additional information\n\n");

	/* Stayin' alive, stayin' alive... */
	pi_watchdog(sd, TICKLE_INTERVAL);

//   atexit(exit_func);

	handle_user_commands(sd);

	return 0;
}

void sigexit(int sig)
{
	exit(0);
}

/***********************************************************************
 *
 * Function:    exit_func
 *
 * Summary:     That's all folks, clean up and exit
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void exit_func(void)
{
#ifdef DEBUG
	fprintf(stderr,
		"\n\n================== EXITING ===================\n\n");
#endif
	fprintf(stderr, "Exiting.\n");
	dlp_EndOfSync(sd, 0);
	pi_close(sd);
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
	static char *s;
	static char *start;
	int i;

	if (str != NULL) {
		s = str;
	}

	i = strspn(s, ws);
	s += i;
	start = s;

	if (*s == '\0') {
		return NULL;
	} else if (strchr(delim, *s) != NULL) {
		start++;
		s = strchr(s + 1, *s);
		s[i] = '\0';
		s++;
	} else {
		i = strcspn(s, ws);
		s[i] = '\0';
		s += i + 1;
	}

	return start;
}
