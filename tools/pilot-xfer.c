/* pilot-xfer.c:  Palm Database transfer utility
 *
 * (c) 1996, 1998, Kenneth Albanowski.
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

/* A few minor modifications, done April 19, 1998 by David H. Silber:
 *  Implemented a ``--version'' (or `-v') option to indicate which version
 *    of pilot-link a particular executable is from.
 *  Added a ``--help'' alias for the `-h' option.
 *  Added error checking to prevent ``Segmentation fault (core dumped)''
 *    when an unreadable exclude file is specified.
 */

#include <stdio.h>
#include <time.h>
#ifdef __EMX__
#include <sys/types.h>
#endif
#include "getopt.h"
#include <sys/stat.h>
#include <signal.h>
#include <utime.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-file.h"
#include "pi-dlp.h"
#include "pi-version.h"
#include "pi-header.h"

#define pi_mktag(c1,c2,c3,c4) (((c1)<<24)|((c2)<<16)|((c3)<<8)|(c4))

int sd = 0;
char *device = "/dev/pilot";
char *progname;

char *exclude[100];
int numexclude = 0;

RETSIGTYPE SigHandler(int signal);

void MakeExcludeList(char *efile)
{
   char temp[1024];
   FILE *f = fopen(efile, "r");

   /*  If the Exclude file cannot be opened, ... */
   if (!f) {
      fprintf(stderr, "Unable to open exclude list file '%s'.\n", efile);
      exit(1);
   }

   while ((fgets(temp, sizeof(temp), f)) != NULL) {
      if (temp[strlen(temp) - 1] == '\n')
	 temp[strlen(temp) - 1] = '\0';
      printf("Will exclude: %s\n", temp);
      exclude[numexclude++] = strdup(temp);
   }
}

/* Protect = and / in filenames */
static void protect_name(char *d, char *s)
{
   while (*s) {
      switch (*s) {
      case '/':
	 *(d++) = '=';
	 *(d++) = '2';
	 *(d++) = 'F';
	 break;
      case '=':
	 *(d++) = '=';
	 *(d++) = '3';
	 *(d++) = 'D';
	 break;
      case '\x0A':
	 *(d++) = '=';
	 *(d++) = '0';
	 *(d++) = 'A';
	 break;
      case '\x0D':
	 *(d++) = '=';
	 *(d++) = '0';
	 *(d++) = 'D';
	 break;
	 /* If you feel the need:
	    case ' ': *(d++) = '='; *(d++) = '2'; *(d++) = '0'; break;
	  */
      default:
	 *(d++) = *s;
      }
      ++s;
   }
   *d = '\0';
}

void Connect(void)
{
   struct pi_sockaddr addr;
   int ret;

   if (sd != 0)
      return;

   signal(SIGHUP, SigHandler);
   signal(SIGINT, SigHandler);
   signal(SIGSEGV, SigHandler);

   if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP))) {
      perror("pi_socket");
      exit(1);
   }

   addr.pi_family = PI_AF_SLP;
   strcpy(addr.pi_device, device);
 
   PalmHeader(progname);

   ret = pi_bind(sd, (struct sockaddr *) &addr, sizeof(addr));
   if (ret == -1)
   {
      fprintf (stderr, "\n\007   ***** ERROR *****\n   Unable to bind to port '%s'...\n\n   Have you used -p or set $PILOTRATE properly?\n\n", device);
      fprintf (stderr, "   Please see 'man 1 %s' or '%s --help' for information\n   on setting the port.\n\n", progname, progname);
      exit (1);
   }
   
   printf("   Port: %s\n\n   Please insert the Palm in the cradle and press the HotSync button.\n", device);

   ret = pi_listen(sd, 1);
   if (ret == -1) {
      perror("pi_listen");
      exit(1);
   }

   sd = pi_accept(sd, 0, 0);
   if (sd == -1) {
      perror("pi_accept");
      exit(1);
   }

   puts("Connected");
}

void Disconnect(void)
{
   if (sd == 0)
      return;

   dlp_EndOfSync(sd, 0);
   pi_close(sd);
   sd = 0;
}

RETSIGTYPE SigHandler(int signal)
{
   puts("   Abort on signal!");
   if(sd!=0) {
     dlp_AbortSync(sd);
     pi_close(sd);
   }
   exit(3);
}

void VoidSyncFlags(void)
{
   struct PilotUser U;

   Connect();
   if (dlp_ReadUserInfo(sd, &U) >= 0) {
      U.lastSyncPC = 0x00000000;	/* Hopefully unique constant, to tell
					   any Desktop software that databases
					   have been altered, and that a slow
					   sync is necessary 
					 */
      U.lastSyncDate = U.successfulSyncDate = time(0);
      dlp_WriteUserInfo(sd, &U);
   }
}

void RemoveFromList(char *name, char **list, int max)
{
   int i;

   for (i = 0; i < max; i++) {
      if (list[i] != NULL && strcmp(name, list[i]) == 0) {
	 free(list[i]);
	 list[i] = NULL;
      }
   }
}

int creator_is_PalmOS(long creator)
{
   union {
      long L;
      char C[4];
   } buf;
   int n;
   static long special_cases[] = {
      pi_mktag('p', 'p', 'p', '_'),
      pi_mktag('u', '8', 'E', 'Z')
   };


   /* Test for special cases -- PalmOS CRIDs outside of lowercase alpha range */

   for (n = 0; n < sizeof(special_cases) / sizeof(long); n++)

      if (creator == special_cases[n])
	 return 1;

   /* Test for all lowercase alpha -- range reserved by PalmOS */

   buf.L = creator;

   for (n = 0; n < 4; n++)
      if (buf.C[n] < 'a' || buf.C[n] > 'z')
	 return 0;

   return 1;
}

void Backup(char *dirname, int only_changed, int remove_deleted, int rom, int quiet, int do_rom, int unsaved, char *archive_dir)
{
   int i, ofile_total, ofile_len;
   DIR *dir;
   struct dirent *dirent;
   char **orig_files = 0;
   int filecounter = 0;

   Connect();

   mkdir(dirname, 0700);
   if (archive_dir)
      mkdir(archive_dir, 0700);

   /* Read original list of files in the backup dir */
   ofile_total = 0;
   ofile_len = 0;

   if (only_changed) {
      dir = opendir(dirname);
      while ((dirent = readdir(dir))) {
	 char name[256];

	 if (dirent->d_name[0] == '.')
	    continue;

	 if (!orig_files) {
	    ofile_len += 256;
	    orig_files = malloc(sizeof(char *) * ofile_len);
	 } else if (ofile_total >= ofile_len) {
	    ofile_len += 256;
	    orig_files = realloc(orig_files, sizeof(char *) * ofile_len);
	 }

	 sprintf(name, "%s/%s", dirname, dirent->d_name);
	 orig_files[ofile_total++] = strdup(name);
      }
      closedir(dir);
   }

   i = 0;
   for (;;) {
      struct DBInfo info;
      struct pi_file *f;
      struct utimbuf times;
      struct stat statb;
      int x;
      int skip = 0;
      char name[256];


      if (dlp_ReadDBList(sd, 0, (rom ? 0x40 : 0x80), i, &info) < 0)
	 break;
      i = info.index + 1;

      if (quiet == 1) {
	 printf("[ %d ]\r", ++filecounter);
	 fflush(stdout);
      }

      if (dlp_OpenConduit(sd) < 0) {
	 fprintf(stderr,
		 "Exiting on cancel, all data _not_ backed up, stopped before backing up '%s'.\n",
		 info.name);
	 exit(1);
      }

      strcpy(name, dirname);
      strcat(name, "/");
      protect_name(name + strlen(name), info.name);

      if (info.flags & dlpDBFlagResource)
	 strcat(name, ".prc");
      else if (!(info.flags & dlpDBFlagClipping))
	 strcat(name, ".pdb");

      for (x = 0; x < numexclude; x++) {
	 /* printf("Skipcheck:%s:%s:\n",exclude[x],info.name); */
	 if (strcmp(exclude[x], info.name) == 0) {
	    printf("Excluding '%s'...\n", name);
	    RemoveFromList(name, orig_files, ofile_total);
	    skip = 1;
	 }
      }

      if (skip == 1)
	 continue;

      if (rom == 1 && creator_is_PalmOS(info.creator)) {
	 if (!quiet)
	    printf("OS file, skipping '%s'.\n", info.name);
	 continue;
      } else if (rom == 2 && !creator_is_PalmOS(info.creator)) {
	 if (!quiet)
	    printf("Non-OS file, skipping '%s'.\n", info.name);
	 continue;
      }

      if (!unsaved && strcmp(info.name, "Unsaved Preferences") == 0) {
	 if (!quiet)
	    printf("Skipping '%s'.\n", info.name);
	 continue;
      }

      if (only_changed) {
	 if (stat(name, &statb) == 0) {
	    if (info.modifyDate == statb.st_mtime) {
	       if (!quiet)
		  printf("No change, skipping '%s'.\n", info.name);
	       RemoveFromList(name, orig_files, ofile_total);
	       continue;
	    }
	 }
      }

      printf("Backing up '%s'... ", name);
      fflush(stdout);

      /* Ensure that DB-open flag is not kept */
      info.flags &= 0x2fd;
      
      f = pi_file_create(name, &info);
      if (f == 0) {
	 printf("Failed, unable to create file\n");
	 break;
      }

      if (pi_file_retrieve(f, sd, 0) < 0)
	 printf("Failed, unable to back up database\n");
      else
	 printf("OK\n");
      pi_file_close(f);

      /* Note: This is no guarantee that the times on the host system
         actually match the GMT times on the Palm. We only check to
         see whether they are the same or different, and do not treat
         them as real times. */

      times.actime = info.createDate;
      times.modtime = info.modifyDate;
      utime(name, &times);

      RemoveFromList(name, orig_files, ofile_total);
   }

   if (orig_files) {
      int dirname_len = strlen(dirname);
      char newname[256];

      if (remove_deleted && dlp_OpenConduit(sd) < 0) {
	 /* If the connection has gone down here, there is probably
	    a communication error. */
	 fprintf(stderr,
		 "Exiting on error, stopped before removing files.\n");
	 exit(1);
      }


      for (i = 0; i < ofile_total; i++)
	 if (orig_files[i] != NULL) {
	    if (remove_deleted) {

	       if (archive_dir) {
		  printf("Archiving '%s'.\n", orig_files[i]);
		  sprintf(newname, "%s/%s",
			  archive_dir, &orig_files[i][dirname_len + 1]);
		  if (rename(orig_files[i], newname) != 0) {
		     fprintf(stderr, "rename(%s, %s) ", orig_files[i],
			     newname);
		     perror("failed");
		  }
	       } else {
		  printf("Removing '%s'.\n", orig_files[i]);
		  unlink(orig_files[i]);
	       }
	    }
	    free(orig_files[i]);
	 }
      if (orig_files)
	 free(orig_files);
   }

   printf("%s backup done.\n",
	  (rom == 2 ? "OS" : (rom == 1 ? "Flash" : "RAM")));
}

void Fetch(char *dbname)
{
   struct DBInfo info;
   char name[256];
   struct pi_file *f;

   Connect();

   if (dlp_OpenConduit(sd) < 0) {
      fprintf(stderr, "Exiting on cancel, stopped before fetching '%s'.\n",
	      dbname);
      exit(1);
   }

   if (dlp_FindDBInfo(sd, 0, 0, dbname, 0, 0, &info) < 0) {
      printf("Unable to locate database '%s', fetch skipped.\n", dbname);
      return;
   }

   protect_name(name, dbname);

   /* Judd - Graffiti hack
    * Graffiti ShortCuts with a space on the end or not is really supposed
    * to be the same file, so we will treat it as such to avoid confusion.
    * remove the space.
    */
   if (strcmp(name, "Graffiti ShortCuts ") == 0) {
      strcpy(name, "Graffiti ShortCuts");
   }

   if (info.flags & dlpDBFlagResource)
      strcat(name, ".prc");
   else if (!(info.flags & dlpDBFlagClipping))
      strcat(name, ".pdb");

   printf("Fetching '%s'... ", name);
   fflush(stdout);

   info.flags &= 0x2fd;

   f = pi_file_create(name, &info);
   if (f == 0) {
      printf("Failed, unable to create file\n");
      return;
   }

   if (pi_file_retrieve(f, sd, 0) < 0)
      printf("Failed, unable to back up database\n");
   else
      printf("OK\n");
   pi_file_close(f);

   printf("Fetch done.\n");
}

void Delete(char *dbname)
{
   struct DBInfo info;

   Connect();

   if (dlp_OpenConduit(sd) < 0) {
      fprintf(stderr, "Exiting on cancel, stopped before deleting '%s'.\n",
	      dbname);
      exit(1);
   }

   dlp_FindDBInfo(sd, 0, 0, dbname, 0, 0, &info);

   printf("Deleting '%s'... ", dbname);
   if (dlp_DeleteDB(sd, 0, dbname) >= 0) {
      if (info.type == pi_mktag('b', 'o', 'o', 't')) {
	 printf(" (rebooting afterwards) ");
      }
      printf("OK\n");
   } else {
      printf("Failed, unable to delete database\n");
   }
   fflush(stdout);

   printf("Delete done.\n");
}

struct db {
   char name[256];
   int flags;
   unsigned long creator;
   unsigned long type;
   int maxblock;
};

int compare(struct db *d1, struct db *d2)
{
   /* types of 'appl' sort later then other types */
   if (d1->creator == d2->creator)
      if (d1->type != d2->type) {
	 if (d1->type == pi_mktag('a', 'p', 'p', 'l'))
	    return 1;
	 if (d2->type == pi_mktag('a', 'p', 'p', 'l'))
	    return -1;
      }
   return d1->maxblock < d2->maxblock;
}

void Restore(char *dirname)
{
   DIR *dir;
   struct dirent *dirent;
   struct DBInfo info;
   struct db **db = NULL;
   int dbcount = 0;
   int i, j, max, size;
   struct pi_file *f;

   dir = opendir(dirname);


   /* Find out how many directory entries exist, so that we can allocate
      the buffer.  We avoid scandir() for maximum portability.

      The count is a bit conservative, as it includes . and .. entries.
    */
   while (readdir(dir))
      dbcount++;

   db = (struct db **) calloc(dbcount, sizeof(struct db *));

   if (!db) {
      fprintf(stderr,
	      "Unable to allocate memory for directory entry table\n");
      exit(1);
   }

   dbcount = 0;
   rewinddir(dir);

   while ((dirent = readdir(dir))) {
      char name[256];

      if (dirent->d_name[0] == '.')
	 continue;


      db[dbcount] = (struct db *) malloc(sizeof(struct db));

      sprintf(db[dbcount]->name, "%s/%s", dirname, dirent->d_name);

      f = pi_file_open(db[dbcount]->name);
      if (f == 0) {
	 printf("Unable to open '%s'!\n", name);
	 break;
      }

      pi_file_get_info(f, &info);

      db[dbcount]->creator = info.creator;
      db[dbcount]->type = info.type;
      db[dbcount]->flags = info.flags;
      db[dbcount]->maxblock = 0;

      pi_file_get_entries(f, &max);

      for (i = 0; i < max; i++) {
	 if (info.flags & dlpDBFlagResource)
	    pi_file_read_resource(f, i, 0, &size, 0, 0);
	 else
	    pi_file_read_record(f, i, 0, &size, 0, 0, 0);

	 if (size > db[dbcount]->maxblock)
	    db[dbcount]->maxblock = size;
      }

      pi_file_close(f);
      dbcount++;
   }

   closedir(dir);

#ifdef DEBUG
   printf("Unsorted:\n");
   for (i = 0; i < dbcount; i++) {
      printf("%d: %s\n", i, db[i]->name);
      printf("  maxblock: %d\n", db[i]->maxblock);
      printf("  creator: '%s'\n", printlong(db[i]->creator));
      printf("  type: '%s'\n", printlong(db[i]->type));
   }
#endif

   for (i = 0; i < dbcount; i++)
      for (j = i + 1; j < dbcount; j++)
	 if (compare(db[i], db[j]) > 0) {
	    struct db *temp = db[i];

	    db[i] = db[j];
	    db[j] = temp;
	 }
#ifdef DEBUG
   printf("Sorted:\n");
   for (i = 0; i < dbcount; i++) {
      printf("%d: %s\n", i, db[i]->name);
      printf("  maxblock: %d\n", db[i]->maxblock);
      printf("  creator: '%s'\n", printlong(db[i]->creator));
      printf("  type: '%s'\n", printlong(db[i]->type));
   }
#endif

   Connect();

   for (i = 0; i < dbcount; i++) {

      if (dlp_OpenConduit(sd) < 0) {
	 fprintf(stderr,
		 "Exiting on cancel, all data not restored, stopped before restoing '%s'.\n",
		 db[i]->name);
	 exit(1);
      }

      f = pi_file_open(db[i]->name);
      if (f == 0) {
	 printf("Unable to open '%s'!\n", db[i]->name);
	 break;
      }
      printf("Restoring %s... ", db[i]->name);
      fflush(stdout);
      if (pi_file_install(f, sd, 0) < 0)
	 printf("failed.\n");
      else
	 printf("OK\n");
      pi_file_close(f);
   }

   VoidSyncFlags();

   for (i = 0; i < dbcount; i++) {
      free(db[i]);
   }
   free(db);

   printf("Restore done\n");
}

void Install(char *filename)
{
   struct pi_file *f;

   Connect();

   if (dlp_OpenConduit(sd) < 0) {
      fprintf(stderr,
	      "Exiting on cancel, stopped before installing '%s'.\n",
	      filename);
      exit(1);
   }

   f = pi_file_open(filename);
   if (f == 0) {
      printf("Unable to open '%s'!\n", filename);
      return;
   }
   printf("Installing %s... ", filename);
   fflush(stdout);
   if (pi_file_install(f, sd, 0) < 0)
      printf("failed.\n");
   else
      printf("OK\n");
   pi_file_close(f);

   VoidSyncFlags();

   printf("Install done\n");
}

void Merge(char *filename)
{
   struct pi_file *f;

   Connect();

   if (dlp_OpenConduit(sd) < 0) {
      fprintf(stderr, "Exiting on cancel, stopped before merging '%s'.\n",
	      filename);
      exit(1);
   }

   f = pi_file_open(filename);
   if (f == 0) {
      printf("Unable to open '%s'!\n", filename);
      return;
   }


   printf("Merging %s... ", filename);
   fflush(stdout);
   if (pi_file_merge(f, sd, 0) < 0)
      printf("failed.\n");
   else
      printf("OK\n");
   pi_file_close(f);

   VoidSyncFlags();

   printf("Merge done\n");
}

void List(int rom)
{
   struct DBInfo info;
   int i;

   Connect();

   if (dlp_OpenConduit(sd) < 0) {
      fprintf(stderr,
	      "Exiting on cancel, stopped before listing databases.\n");
      exit(1);
   }

   if (rom)
      printf("Reading list of databases in RAM and ROM...\n");
   else
      printf("Reading list of databases in RAM...\n");

   i = 0;
   for (;;) {
      if (dlp_ReadDBList(sd, 0, (rom ? 0x80 | 0x40 : 0x80), i, &info) < 0)
	 break;
      i = info.index + 1;

      printf("'%s'\n", info.name);
   }

   printf("List done.\n");
}

void Purge(void)
{
   struct DBInfo info;
   int i;
   int h;

   Connect();

   if (dlp_OpenConduit(sd) < 0) {
      fprintf(stderr,
	      "Exiting on cancel, stopped before purging databases.\n");
      exit(1);
   }

   printf("Reading list of databases to purge...\n");

   i = 0;
   for (;;) {
      if (dlp_ReadDBList(sd, 0, 0x80, i, &info) < 0)
	 break;
      i = info.index + 1;

      if (info.flags & 1)
	 continue;		/* skip resource databases */

      printf("Purging deleted records from '%s'... ", info.name);

      h = 0;
      if ((dlp_OpenDB(sd, 0, 0x40 | 0x80, info.name, &h) >= 0) &&
	  (dlp_CleanUpDatabase(sd, h) >= 0) &&
	  (dlp_ResetSyncFlags(sd, h) >= 0)) {
	 printf("OK\n");
      } else {
	 printf("Failed\n");
      }

      if (h != 0)
	 dlp_CloseDB(sd, h);

   }

   VoidSyncFlags();

   printf("Purge done.\n");
}

void Help(void) 
{

   PalmHeader(progname);
   fprintf(stderr,
   "   Usage: %s [-p port] [ -F|-O -U -q|-c ] command(s)\n", progname);
   fprintf(stderr, " 

   -b[ackup] backupdir   = copy the contents of your palm to < backupdir >
   -u[pdate] backupdir   = update backupdir with newer Palm data 
   -s[ync] backupdir     = same as - u above, but removes local files if data 
                           is removed from your Palm 
   -r[estore] backupdir  = restore backupdir to your Palm 
   -i[nstall] dbname(s)  = install local[prc | pdb] files to your Palm 
   -m[erge] filename(s)  = adds the records in <file> into the corresponding
                           Palm database 
   -f[etch] dbname(s)    = retrieve dbname(s) from your Palm 
   -d[elete] dbname(s)   = delete(permanently) dbname(s) from your Palm 
   -e[xclude] filename   = exclude dbname(s) listed in <filename> from being 
                           included by -backup, -sync, or -update. 
   -P[urge]              = purge any deleted data that hasn't been cleaned up 
                           by a sync 

   -B[ackup]             = backup your Palm, clearing the read-only bits on all 
                           backed up databases
   -U                    = update but clear the read-only bit upon sync
   -S[ync]               = sync your Palm with a directory, clearing the 
                           read-only bit in the process for all sync'd 
                           databases

   -l[ist]               = list all application and 3 rd party data on the Palm 
   -L[istall]            = list all data, internal and external on the Palm 
   -v[ersion]            = report the version of %s(currently %d.%d.%d) 
   -h[elp]               = reprint this usage screen 

   -a modifies -s to archive deleted files in specified directory. 
   -F modifies -b, -u, and -s, to back up non-OS db's from Flash ROM. 
   -O modifies -b, -u, and -s, to back up OS db 's from Flash ROM. 
   -I modifies -b|-B, -u|-U, and -s|-S, to back up \"illegal\" 
      Unsaved Preferences.PDB (normally skipped, per Palm's recommendation). 
   -q makes all the backup options shut up about skipped files. 
   -c does same as '-q', but counts files(\"[nnn]...\") as they go by. 

   The serial port used to connect to may be specified by the $PILOTPORT
   environment variable in your shell instead of the command line.  If it is
   not specified anywhere, it will default to /dev/pilot.
   
   Additionally, the baud rate to connect with may be specified by the 
   $PILOTRATE environment variable.If not specified, it will default to 9600. 
   Please use caution setting $PILOTRATE to higher values, as several types of 
   workstations have problems with higher baud rates.
   
   Always consult the man page(s) for additional usage of these options as well 
   as details on the results of combining other parameters together.\n\n"
   , progname, PILOT_LINK_VERSION, PILOT_LINK_MAJOR, PILOT_LINK_MINOR);

exit(0);}

void Version(void) 
{
   PalmHeader(progname);
   fprintf(stderr,
   "   Type 'man 7 pilot-link' at your shell for more information.\n\n");
   exit(0);
}

                                                                                                                                                                                                			 
struct {
   char *name;
   int value;
} longopt[] = {
   {"backup",  'b'}
 , {"update",  'u'}
 , {"sync",    's'}
 , {"restore", 'r'}
 , {"install", 'i'}
 , {"merge",   'm'}
 , {"fetch",   'f'}
 , {"delete",  'd'}
 , {"exclude", 'e'}
 , {"purge",   'P'}
 , {"list",    'l'}
 , {"Listall", 'L'}
 , {"version", 'v'}
 , {"help",    'h'}
 , {0, 0}
};
                                                                                                			 
			 
int main(int argc, char *argv[])
{
   int mode;
   char *tmp;
   int i;
   int action;
   int quiet = 0;
   int do_rom = 0;
   int do_unsaved = 0;
   char *archive_dir = NULL;


   progname = argv[0];

   if (argc < 2)
   {
      Help();
   }

   tmp = getenv ("PILOTPORT");
   if (tmp != NULL)
      device = tmp;


   /* Analyze arguments for errors */

   if ((argc > 1) && (!strcmp (argv[1], "-p") || !strcmp (argv[1], "--port")))
   {
      argv++;
      argc--;
   }

   mode = 'p';
   action = 0;

   for (i = 1; i < argc; i++)
   {
      if (argv[i][0] == '-')
      {
	 /* If it is a long argument, convert it to the canonical short version */
	 if (argv[i][1] == '-')
	 {
	    int j;

	    for (j = 0; longopt[j].name; j++)
	       if (!strcmp (longopt[j].name, argv[i] + 2))
	       {
		  argv[i][1] = longopt[j].value;
		  argv[i][2] = '\0';
		  break;
	       }
	 }
	 if (strlen (argv[i]) != 2)
	 {
	    fprintf (stderr, "%s: Unknown option '%s'\n", argv[0], argv[i]);
	    Help();
	 }

	 /* Check for commands that take a single argument */
	 else if (strchr ("busBRS", argv[i][1]))
	 {
	    mode = 0;
	    if (++i >= argc)
	    {
	       fprintf (stderr, "%s: Options '%s' requires argument\n",
			argv[0], argv[i - 1]);
	       Help();
	    }
	    action = 1;

	    /* Check for commands with unlimited arguments */
	 }
	 else if (strchr ("rimfde", argv[i][1]))
	 {
	    mode = argv[i][1];
	    action = 1;
	    if (++i >= argc)
	    {
	       fprintf (stderr, "%s: Options '%s' requires argument\n",
			argv[0], argv[i - 1]);
	       Help();
	    }
	    /* Check for commands that take no arguments */
	 }
	 else if (strchr ("lLP", argv[i][1]))
	 {
	    action = 1;
	    mode = 0;

	    /* Check for modifiers to -b -u -s */
	 }
	 else if (strchr ("FOUqc", argv[i][1]))
	 {
	    /* nothing to do, yet... */

	    /* Check for options that take a single argument */
	 }
	 else if (strchr ("a", argv[i][1]))
	 {
	    if (++i >= argc)
	    {
	       fprintf (stderr, "%s: Options '%s' requires argument\n",
			argv[0], argv[i - 1]);
	       Help();
	    }

	    /* Check for forcing commands */
	 }
	 else if (argv[i][1] == 'v')
	 {
	    Version ();
	 }
	 else if (argv[i][1] == 'h')
	 {
	    Help();

	 }
	 else
	 {
	    fprintf (stderr, "%s: Unknown option '%s'\n", argv[0], argv[i]);
	    Help();
	 }
      }
      else
      {
	 if (!mode)
	 {
	    fprintf (stderr,
		     "%s: Must specify command before argument '%s'\n",
		     argv[0], argv[i]);
	    Help();
	 }
	 if (mode != 'p')
	    action = 1;
	 if (strchr ("busrep", mode))
	    mode = 0;
      }
   }
   if (!action)
   {
      fprintf (stderr, "%s: Please give a command\n", argv[0]);
      Help();
   }

   /* Process arguments */

   mode = 'p';

   for (i = 1; i < argc; i++)
   {
      if (argv[i][0] == '-')
      {
	 mode = 0;
	 if (strchr ("aBRSbusrimfde", argv[i][1]))
	 {
	    mode = argv[i][1];
	    i++;
	 }
	 else
	    switch (argv[i][1])
	    {
	    case 'l':
	       List (0);
	       continue;
	    case 'L':
	       List (1);
	       continue;
	    case 'P':
	       Purge ();
	       continue;
	    case 'F':
	       do_rom = !do_rom;	/* can be toggled */
	       continue;
	    case 'O':
	       do_rom = 2;
	       continue;
	    case 'I':
	       do_unsaved = 1;
	       continue;
	    case 'q':
	       quiet = 2;
	       continue;
	    case 'c':
	       quiet = 1;
	       continue;
	    default:
	       /* Shouldn't reach here */
	       Help();
	    }
      }
      switch (mode)
      {
      case 'p':
	 device = argv[i];
	 mode = 0;
	 break;
      case 'a':
	 archive_dir = argv[i];
	 mode = 0;
	 break;
      case 'b':
	 Backup (argv[i], 0, 0, 0, quiet, do_rom, do_unsaved, archive_dir);
	 mode = 0;
	 break;
      case 'u':
	 Backup (argv[i], 1, 0, 0, quiet, do_rom, do_unsaved, archive_dir);
	 mode = 0;
	 break;
      case 's':
	 Backup (argv[i], 1, 1, 0, quiet, do_rom, do_unsaved, archive_dir);
	 mode = 0;
	 break;
      case 'B':
         Backup (argv[i], 0, 0, 1, quiet, do_rom, do_unsaved, archive_dir);
         mode = 0;
         break;
      case 'R':
         Backup (argv[i], 1, 0, 1, quiet, do_rom, do_unsaved, archive_dir);
         mode = 0;
         break;
      case 'S':
         Backup (argv[i], 1, 1, 1, quiet, do_rom, do_unsaved, archive_dir);
         mode = 0;
         break;
      case 'r':
	 Restore (argv[i]);
	 break;
      case 'i':
	 Install (argv[i]);
	 break;
      case 'm':
	 Merge (argv[i]);
	 break;
      case 'f':
	 Fetch (argv[i]);
	 break;
      case 'd':
	 Delete (argv[i]);
	 break;
      case 'e':
	 MakeExcludeList (argv[i]);
	 break;
      default:
	 /* Shouldn't reach here */
	 Help();
      }
   }

   Disconnect ();

   return 0;
}

