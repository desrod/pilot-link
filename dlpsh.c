/* DLP command Shell */

#include <signal.h>
#include <stdio.h>
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-padp.h"
#include "pi-dlp.h"
#include "pi-serial.h"

#define TICKLE_INTERVAL 7


int socket_descriptor;
struct pi_socket *ticklish_pi_socket;

/* Prototypes */
int user_fn(int sd, int argc, char **argv);
int ls_fn(int sd, int argc, char **argv);
int rm_fn(int sd, int argc, char **argv);
int help_fn(int sd, int argc, char **argv);
int exit_fn(int sd, int argc, char **argv);
char *strtoke(char *str, char *ws, char *delim);
void exit_func(void);
void sigexit(int sig);
void sigalarm(int sig);

typedef int (*cmd_fn_t)(int, int, char **);

struct Command {
  char *name;
  cmd_fn_t func;
};

struct Command command_list[] = {
  { "user", user_fn },
  { "ls", ls_fn },
  { "rm", rm_fn },
  { "help", help_fn },
  { "quit", exit_fn },
  { "exit", exit_fn },
  { NULL, NULL }
};

/* functions for builtin commands */
int exit_fn(int sd, int argc, char **argv) {
  exit_func();
  sigexit(0);
  return 0;
}

int help_fn(int sd, int argc, char **argv) {
  int i;

  printf("Built-in commands:\n");
  for (i = 0; command_list[i].name != NULL; i++) {
    printf("%s\t",command_list[i].name);
    if((i%5)==4) {
      printf("\n");
    }
  }
  printf("\n");
  return 0;
}

int user_fn(int sd, int argc, char **argv) {
  struct PilotUser U, nU;
  char fl_name = 0, fl_uid = 0, fl_vid = 0, fl_pid = 0;
  int c, ret;

#ifdef sun
  extern char* optarg;
  extern int optind;
#endif

  optind = 0;
  while ((c = getopt(argc, argv, "n:i:v:p:h")) != -1) {
    switch (c) {
    case 'n':
      fl_name = 1;
      strcpy(nU.username, optarg);
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
    case 'h': case '?':
      printf("Usage: user [ -n <username> ]\n");
      printf("            [ -i <user id> ]\n");
      printf("            [ -v <viewer id> ]\n");
      printf("            [ -p <pc id> ]\n");
      return 0;
    }
  }

  alarm(TICKLE_INTERVAL);

  ret = dlp_ReadUserInfo(sd, &U);
  if (ret < 0) {
    printf("dlp_ReadUserInfo: err %d\n", ret);
    return -1;
  }

  if (fl_name + fl_uid + fl_vid + fl_pid == 0) {
    printf("username = \"%s\"\n", U.username);
    printf("userID = %08lx   viewerID = %08lx    PCid = %08lx\n",
           U.userID, U.viewerID, U.lastSyncPC);
    return 0;
  }

  if (fl_name)
    strcpy(U.username, nU.username);
  if (fl_uid)
    U.userID = nU.userID;
  if (fl_vid)
    U.viewerID = nU.viewerID;
  if (fl_pid)
    U.lastSyncPC = nU.lastSyncPC;

  U.succSyncDate = time(NULL);
  U.lastSyncDate = U.succSyncDate;

  alarm(TICKLE_INTERVAL);

  ret = dlp_WriteUserInfo(sd, &U);
  if (ret < 0) {
    printf("dlp_WriteUserInfo: err %d\n", ret);
    return -1;
  }

  return 0;
}

char *
timestr (time_t t)
{
  struct tm tm;
  static char buf[50];

  tm = *localtime (&t);
  sprintf (buf, "%04d-%02d-%02d %02d:%02d:%02d",
	   tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
	   tm.tm_hour, tm.tm_min, tm.tm_sec);
  return (buf);
}

int 
ls_fn(int sd, int argc, char **argv) 
{
  int c, ret;
  int lflag = 0;
  int cardno, flags, start;
  int rom_flag = 0;

#ifdef sun
  extern char* optarg;
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
      printf("Usage: ls\n");
      printf ("         -l    long format\n");
      printf ("         -r    list rom instead of ram\n");
      return 0;
    }
  }

  cardno = 0;
  if (rom_flag == 0)
    flags = 0x80; /* dlpReadDBListFlagRAM */
  else
    flags = 0x40; /* dlpReadDBListFlagROM */

  start = 0;
  while (1) {
    struct DBInfo info;
    long tag;

    alarm(TICKLE_INTERVAL);

    /*
     * The databases are numbered starting at 0.  The first 12 are in
     * ROM, and the rest are in RAM.  The high two bits of the flags
     * byte control wheter you see the ROM entries, RAM entries or both.
     * start is the lowest index you want.  So, we start with 0, but
     * usually we want to see ram entries, so it will return database
     * number 12.  Then, we'll ask for 13, etc, until we get the NotFound
     * error return.
     */
    ret = dlp_ReadDBList (sd, cardno, flags, start, &info);

    if (ret == -5 /* dlpRespErrNotFound */)
      break;

    if (ret < 0) {
      printf("dlp_ReadDBList: err %d\n", ret);
      return -1;
    }

    printf ("%.32s\n", info.name);
    if (lflag) {
      printf ("  more 0x%x; ", info.more);
      printf ("flags 0x%x; ", info.flags);
      tag = htonl (info.type);
      printf ("type '%.4s'; ", (char *)&tag);
      tag = htonl (info.creator);
      printf ("creator '%.4s'; ", (char *)&tag);
      printf ("version %d; ", info.version);
      printf ("modnum %ld\n", info.modnum);
      printf ("  cr %s; ", timestr (info.crdate));
      printf ("mod %s; ", timestr (info.moddate));
      printf ("backup %s; ", timestr (info.backupdate));
      printf ("\n");
    }

    if (info.index < start) {
      /* avoid looping forever if we get confused */
      printf ("error: index backs up\n");
      break;
    }

    start = info.index + 1;
  }

  return 0;
}

int 
rm_fn(int sd, int argc, char **argv) 
{
  int ret;
  int cardno;
  char *name;

  if (argc != 2) {
    printf ("Usage: rm database\n");
    return (0);
  }

  name = argv[1];

  cardno = 0;
  if ((ret = dlp_DeleteDB (sd, cardno, name)) < 0) {
    if (ret == dlpErrNotFound) {
      printf ("%s: not found\n", name);
      return (0);
    }
    printf ("%s: remove error %d\n", name, ret);
    return (0);
  }

  return (0);
}

/* parse user commands and do the right thing.. */
void handle_user_commands(int sd) {
  char buf[256];
  char *argv[32];
  int argc;
  int i;

  while (1) {
    printf("dlpsh> "); fflush(stdout);
    if(fgets(buf, 256, stdin) == NULL) break;

    argc = 0;
    argv[0] = strtoke(buf, " \t\n", "\"'");

    while (argv[argc] != NULL) {
      argc++;
      argv[argc] = strtoke(NULL, " \t\n", "\"'");
    }

    if(argc == 0) continue;

    for (i = 0; command_list[i].name != NULL; i++) {
      if (strcasecmp(argv[0], command_list[i].name) == 0) {
        command_list[i].func(sd, argc, argv);
      }
    }
  }
  printf("\n");
}

int main(int argc, char **argv) {
  struct pi_sockaddr addr;
#ifdef HAVE_SIGACTION
  struct sigaction sigact;
#else
  struct sigvec vec;
#endif
  
  int sd;
  int ret;

  if (argc != 2) {
    fprintf(stderr, "Usage: %s %s\n", argv[0],TTYPrompt);
    exit(1);
  }

  /* Connect to the Pilot. */

  sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP);
  if (sd == -1) {
    perror("pi_socket");
    exit(1);
  }

  addr.sa_family = PI_AF_SLP;
  addr.port = 3;
  strcpy(addr.device, argv[1]);

  ret = pi_bind(sd, &addr, sizeof(addr));
  if (ret == -1) {
    perror("pi_bind");
    exit(1);
  }

  ret = pi_listen(sd, 5);
  if (ret == -1) {
    perror("pi_listen");
    exit(1);
  }

  printf("Connecting to Pilot at %s...", argv[1]); fflush(stdout);

  ret = pi_accept(sd, &addr, NULL);
  if (ret == -1) {
    perror("pi_accept");
    exit(1);
  }

  printf("connected.\n");

  /* Stayin' alive, stayin' alive... */
  ticklish_pi_socket = find_pi_socket(sd);

  /* Set up signal handlers. */

#ifdef HAVE_SIGACTION
  sigact.sa_handler = sigalarm;
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = 0;
  sigaction(SIGALRM, &sigact, NULL);

  sigact.sa_handler = sigexit;
  sigaction(SIGINT, &sigact, NULL);
  sigaction(SIGTERM, &sigact, NULL);
#else
  vec.sv_handler = sigalarm;
  vec.sv_mask = 0;
  vec.sv_onstack = 0;
  sigvec(SIGALRM, &vec, NULL);
  
  vec.sv_handler = sigexit;
  sigvec(SIGINT, &vec, NULL);
  sigvec(SIGTERM, &vec, NULL);
#endif

  alarm(TICKLE_INTERVAL);

  socket_descriptor = sd;
  atexit(exit_func);

  handle_user_commands(sd);
  exit(0);
}

void sigalarm(int sig) {
  struct padp pd;
  pd.type = padTickle;
  pd.flags = 0x00;
  pd.size = 0x00;
  padp_tx(ticklish_pi_socket, (void *)&pd, 0, padTickle);
  pi_socket_flush(ticklish_pi_socket);
  alarm(TICKLE_INTERVAL);
}

void sigexit(int sig) {
  exit(0);
}

void exit_func(void) {
#ifdef DEBUG
  fprintf(stderr, "\n\n================== EXITING ===================\n\n\n");
#endif
  alarm(0);
  dlp_EndOfSync(socket_descriptor, 0);
  pi_close(socket_descriptor);
}

char *strtoke(char *str, char *ws, char *delim) {
  static char *s, *start;
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
    s = strchr(s+1, *s);
    s[i] = '\0';
    s++;
  } else {
    i = strcspn(s, ws);
    s[i] = '\0';
    s += i+1;
  }

  return start;
}
