/* pilot-xfer.c:  Pilot Database transfer utility
 *
 * (c) 1996, Kenneth Albanowski.
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
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

#define pi_mktag(c1,c2,c3,c4) (((c1)<<24)|((c2)<<16)|((c3)<<8)|(c4))

int sd = 0;
char * device = "/dev/pilot";
char * progname;

char * exclude[100];
int numexclude = 0;

RETSIGTYPE SigHandler(int signal);

void MakeExcludeList(char *efile) {
    char temp[1024];
    FILE *f = fopen(efile,"r");

    while((fgets(temp,sizeof(temp),f)) != NULL) {
        if (temp[strlen(temp)-1] == '\n')
            temp[strlen(temp)-1] = '\0';
        printf("Will exclude: %s\n",temp);
        exclude[numexclude++] = strdup(temp);
    }
}

/* Protect = and / in filenames */
static void protect_name(char *d, char *s)
{
    while(*s) {
      switch(*s) {
          case '/': *(d++) = '='; *(d++) = '2'; *(d++) = 'F'; break;
          case '=': *(d++) = '='; *(d++) = '3'; *(d++) = 'D'; break;
          case '\x0A': *(d++) = '='; *(d++) = '0'; *(d++) = 'A'; break;
          case '\x0D': *(d++) = '='; *(d++) = '0'; *(d++) = 'D'; break;
       /* If you feel the need:
          case ' ': *(d++) = '='; *(d++) = '2'; *(d++) = '0'; break;
        */
          default: *(d++) = *s;
      }
      ++s;
    }
    *d = '\0';
}

void Connect(void) {
  struct pi_sockaddr addr;
  int ret;

  if (sd!=0)
    return;
    
  signal(SIGHUP, SigHandler);
  signal(SIGINT, SigHandler);
  signal(SIGSEGV, SigHandler);
      
  if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP))) {
    perror("pi_socket");
    exit(1);
  }

  addr.pi_family = PI_AF_SLP;
  strcpy(addr.pi_device,device);
  
  ret = pi_bind(sd, (struct sockaddr*)&addr, sizeof(addr));
  if(ret == -1) {
    fprintf(stderr, "Unable to bind to port '%s'.\n(Please see '%s -h' for information setting the port).\n", device, progname);
    exit(1);
  }
    
  printf("Waiting for connection on %s (press the HotSync button now)...\n", device);

  ret = pi_listen(sd,1);
  if(ret == -1) {
    perror("pi_listen");
    exit(1);
  }

  sd = pi_accept(sd,0,0);
  if(sd == -1) {
    perror("pi_accept");
    exit(1);
  }

  puts("Connected");
}

void Disconnect(void)
{
  if(sd==0)
    return;
    
  dlp_EndOfSync(sd, 0);
  pi_close(sd);
  sd = 0;
}

RETSIGTYPE SigHandler(int signal)
{
  puts("Abort on signal!");
  Disconnect();
  exit(3);
}

void VoidSyncFlags(void)
{
  struct PilotUser U;
  Connect();
  if (dlp_ReadUserInfo(sd, &U)>=0) {
    U.lastSyncPC = 0x00000000; /* Hopefully unique constant, to tell
                                  any Desktop software that databases
                                  have been altered, and that a slow
                                  sync is necessary */
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

void Backup(char * dirname, int only_changed, int remove_deleted)
{
  int i, ofile_total, ofile_len;
  DIR * dir;
  struct dirent * dirent;
  char ** orig_files = 0;
  
  Connect();
  
  mkdir(dirname,0700);

  /* Read original list of files in the backup dir */
  ofile_total = 0;
  ofile_len = 0;
  
  if (only_changed) {
    dir = opendir(dirname);
    while( (dirent = readdir(dir)) ) {
      char name[256];
      if (dirent->d_name[0] == '.')
         continue;
         
      if (!orig_files) {
        ofile_len += 256;
      	orig_files = malloc(sizeof(char*) * ofile_len);
      } else if (ofile_total >= ofile_len) {
        ofile_len += 256;
      	orig_files = realloc(orig_files, sizeof(char*) * ofile_len);
      }

      sprintf(name, "%s/%s", dirname, dirent->d_name);
      orig_files[ofile_total++] = strdup(name);
    }
    closedir(dir);
  }

  i = 0;
  for(;;) {
  	struct DBInfo info;
  	struct pi_file * f;
	struct utimbuf times;
	struct stat statb;
        int x;
        int skip = 0;
  	char name[256];

        if (dlp_OpenConduit(sd)<0) {
	  puts("Exiting on cancel, all data _not_ backed up.");
          exit(1);
        }
        
  	if( dlp_ReadDBList(sd, 0, 0x80, i, &info) < 0)
  		break;
  	i = info.index + 1;

	strcpy(name, dirname);
	strcat(name, "/");
	protect_name(name + strlen(name), info.name);

  	if (info.flags & dlpDBFlagResource)
  	  strcat(name,".prc");
  	else
  	  strcat(name,".pdb");

        for(x = 0; x < numexclude; x++) {
          /* printf("Skipcheck:%s:%s:\n",exclude[x],info.name); */
          if(strcmp(exclude[x],info.name) == 0) {
            printf("Excluding '%s'...\n",name);
	    RemoveFromList(name, orig_files, ofile_total);
            skip = 1;
          }
        }

        if(skip == 1)
          continue;

	if (stat(name, &statb) == 0) {
	  if (info.modifyDate == statb.st_mtime) {
	    printf("No change, skipping '%s'.\n", info.name);
	    RemoveFromList(name, orig_files, ofile_total);
	    continue;
	  }
	}

  	printf("Backing up '%s'... ", name);
  	fflush(stdout);

  	/* Ensure that DB-open flag is not kept */
  	info.flags &= 0xff;
  	
  	f = pi_file_create(name, &info);
  	if (f==0) {
  	  printf("Failed, unable to create file\n");
  	  break;
  	}
  	
  	if(pi_file_retrieve(f, sd, 0)<0)
  	  printf("Failed, unable to back up database\n");
  	else
  	  printf("OK\n");
  	pi_file_close(f);
  	
  	/* Note: This is no guarantee that the times on the host system
  	   actually match the GMT times on the Pilot. We only check to
  	   see whether they are the same or different, and do not treat
  	   them as real times. */
  	   
	times.actime = info.createDate;
	times.modtime = info.modifyDate;
	utime(name, &times);
	
	RemoveFromList(name, orig_files, ofile_total);
  }
  
  if (orig_files) {
    for (i = 0; i < ofile_total; i++)
      if (orig_files[i] != NULL) {
        if (remove_deleted) {
          printf("Removing '%s'.\n", orig_files[i]);
          unlink(orig_files[i]);
        }
        free(orig_files[i]);
      }
    if (orig_files)
      free(orig_files);
  }

  printf("Backup done.\n");
}

void Fetch(char * dbname)
{
  struct DBInfo info;
  char name[256];
    	struct pi_file * f;
    	
  Connect();

  if (dlp_OpenConduit(sd)<0) {
    puts("Exiting on cancel, all data _not_ backed up.");
    exit(1);
  }
  	
  if (dlp_FindDBInfo(sd, 0, 0, dbname, 0, 0, &info)<0) {
    printf("Unable to locate database '%s', fetch skipped.\n", dbname);
    return;
  }
        
  protect_name(name, dbname);
  if (info.flags & dlpDBFlagResource)
    strcat(name,".prc");
  else
    strcat(name,".pdb");
  	  
  printf("Fetching '%s'... ", name);
  fflush(stdout);

  info.flags &= 0xff;
  	
  f = pi_file_create(name, &info);
  if (f==0) {
    printf("Failed, unable to create file\n");
    return;
  }
  	
  if(pi_file_retrieve(f, sd, 0)<0)
    printf("Failed, unable to back up database\n");
  else
    printf("OK\n");
  pi_file_close(f);

  printf("Fetch done.\n");
}

void Delete(char * dbname)
{
  struct DBInfo info;
  Connect();

  if (dlp_OpenConduit(sd)<0) {
    puts("Exiting on cancel, all data _not_ backed up.");
    exit(1);
  }

  dlp_FindDBInfo(sd, 0, 0, dbname, 0, 0, &info);
  
  printf("Deleting '%s'... ", dbname);
  if (dlp_DeleteDB(sd, 0, dbname)>=0) {
        if (info.type == pi_mktag('b','o','o','t')) {
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

int compare(struct db * d1, struct db * d2)
{
  /* types of 'appl' sort later then other types */
  if(d1->creator == d2->creator)
    if(d1->type != d2->type) {
      if(d1->type == pi_mktag('a','p','p','l'))
        return 1;
      if(d2->type == pi_mktag('a','p','p','l'))
        return -1;
    }
  return d1->maxblock < d2->maxblock;
}

void Restore(char * dirname)
{
  DIR * dir;
  struct dirent * dirent;
  struct DBInfo info;
  struct db * db[256];
  int dbcount = 0;
  int i,j,max,size;
  struct pi_file * f;

  dir = opendir(dirname);
  
  while( (dirent = readdir(dir)) ) {
        char name[256];
        
        if (dirent->d_name[0] == '.')
           continue;

	
	db[dbcount] = (struct db*)malloc(sizeof(struct db));
	
	sprintf(db[dbcount]->name, "%s/%s", dirname, dirent->d_name);
	
	f = pi_file_open(db[dbcount]->name);
  	if (f==0) {
  	  printf("Unable to open '%s'!\n", name);
  	  break;
  	}
  	
  	pi_file_get_info(f, &info);
  	
  	db[dbcount]->creator = info.creator;
  	db[dbcount]->type = info.type;
  	db[dbcount]->flags = info.flags;
  	db[dbcount]->maxblock = 0;
  	
  	pi_file_get_entries(f, &max);
  	
  	for (i=0;i<max;i++) {
  	  if (info.flags & dlpDBFlagResource)
  	    pi_file_read_resource(f, i, 0, &size, 0, 0);
  	  else
  	    pi_file_read_record(f, i, 0, &size, 0, 0,0 );
  	    
  	  if (size > db[dbcount]->maxblock)
  	    db[dbcount]->maxblock = size;
  	}
  	
  	pi_file_close(f);
  	dbcount++;
  }

  closedir(dir);

#ifdef DEBUG      
  printf("Unsorted:\n");
  for (i=0;i<dbcount;i++) {
     printf("%d: %s\n", i, db[i]->name);
     printf("  maxblock: %d\n", db[i]->maxblock);
     printf("  creator: '%s'\n", printlong(db[i]->creator));
     printf("  type: '%s'\n", printlong(db[i]->type));
  }
#endif
  
  for (i=0;i<dbcount;i++)
    for (j=i+1;j<dbcount;j++) 
      if (compare(db[i],db[j])>0) {
        struct db * temp = db[i];
        db[i] = db[j];
        db[j] = temp;
      }

#ifdef DEBUG      
  printf("Sorted:\n");
  for (i=0;i<dbcount;i++) {
     printf("%d: %s\n", i, db[i]->name);
     printf("  maxblock: %d\n", db[i]->maxblock);
     printf("  creator: '%s'\n", printlong(db[i]->creator));
     printf("  type: '%s'\n", printlong(db[i]->type));
  }
#endif

  Connect();
  
  for (i=0;i<dbcount;i++) {

	if ( dlp_OpenConduit(sd) < 0) {
	   puts("Exiting on cancel. All data not restored.");
	   exit(1);
	}

  	f = pi_file_open(db[i]->name);
  	if (f==0) {
  	  printf("Unable to open '%s'!\n", db[i]->name);
  	  break;
  	}
  	printf("Restoring %s... ", db[i]->name);
  	fflush(stdout);
  	if(pi_file_install(f, sd, 0)<0)
  	  printf("failed.\n");
  	else
  	  printf("OK\n");
  	pi_file_close(f);
  }
  
  VoidSyncFlags();

  printf("Restore done\n");
}

void Install(char * filename)
{
  struct pi_file * f;
  
  Connect();

  if ( dlp_OpenConduit(sd) < 0) {
    puts("Exiting on cancel. All data not restored.");
    exit(1);
  }

  f = pi_file_open(filename);
  if (f==0) {
    printf("Unable to open '%s'!\n", filename);
    return;
  }
  printf("Installing %s... ", filename);
  fflush(stdout);
  if(pi_file_install(f, sd, 0)<0)
    printf("failed.\n");
  else
    printf("OK\n");
  pi_file_close(f);
  
  VoidSyncFlags();

  printf("Install done\n");
}

void Merge(char * filename)
{
  struct pi_file * f;
  
  Connect();

  if ( dlp_OpenConduit(sd) < 0) {
    puts("Exiting on cancel. All data not restored.");
    exit(1);
  }

  f = pi_file_open(filename);
  if (f==0) {
    printf("Unable to open '%s'!\n", filename);
    return;
  }
  
  
  printf("Merging %s... ", filename);
  fflush(stdout);
  if(pi_file_merge(f, sd, 0)<0)
    printf("failed.\n");
  else
    printf("OK\n");
  pi_file_close(f);
  
  VoidSyncFlags();

  printf("Merge done\n");
}

void List(void)
{
  struct DBInfo info;
  int i;
  
  Connect();

  if ( dlp_OpenConduit(sd) < 0) {
    puts("Exiting on cancel. All data not restored.");
    exit(1);
  }
  
  printf("Reading list of databases...\n");
  
  i = 0;
  for(;;) {
  	if( dlp_ReadDBList(sd, 0, 0x80, i, &info) < 0)
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

  if ( dlp_OpenConduit(sd) < 0) {
    puts("Exiting on cancel. All data not restored.");
    exit(1);
  }
  
  printf("Reading list of databases to purge...\n");
  
  i = 0;
  for(;;) {
  	if( dlp_ReadDBList(sd, 0, 0x80, i, &info) < 0)
  		break;
  	i = info.index + 1;
  	
  	if (info.flags & 1)
  		continue; /* skip resource databases */
  	
  	printf("Purging deleted records from '%s'... ", info.name);
  	
  	h = 0;
  	if ((dlp_OpenDB(sd, 0, 0x40|0x80, info.name, &h)>=0) &&
	    (dlp_CleanUpDatabase(sd, h)>=0) &&
  	    (dlp_ResetSyncFlags(sd,h)>=0) ) {
  	    printf("OK\n");
  	} else {
  	    printf("Failed\n");
  	}
  	
  	if (h!=0)
  	    dlp_CloseDB(sd, h);
  	
  }
  
  VoidSyncFlags();
  
  printf("Purge done.\n");
}

void Help(void)
{
      printf("Usage: %s [%s] command(s)\n\n",progname,TTYPrompt);
      printf("Where a command is one or more of: -b(ackup)  backupdir\n");
      printf("                                   -u(pdate)  backupdir\n");
      printf("                                   -s(ync)    backupdir\n");
      printf("                                   -r(estore) backupdir\n");
      printf("                                   -i(nstall) filename(s)\n");
      printf("                                   -m(erge)   filename(s)\n");
      printf("                                   -f(etch)   dbname(s)\n");
      printf("                                   -d(elete)  dbname(s)\n");
      printf("                                   -e(xclude) filename(s)\n");
      printf("                                   -p(urge)\n");
      printf("                                   -l(ist)\n");
      printf("\n");
      printf("The serial port to connect to may be specified by the PILOTPORT\n");
      printf("environment variable instead of the command line. If not specified\n");
      printf("anywhere, it will default to /dev/pilot.\n");
      printf("\n");
      printf(" -b backs up all databases to the directorym\n");
      printf(" -u is the same as -b, except it only backs up changed or new db's\n");
      printf(" -s is the same as -u, except it removes files if the database is\n");
      printf("    deleted on the Pilot.\n");
      exit(0);
}

struct option options[] = {
	{"backup", required_argument, 0, 'b'},
	{"update", required_argument, 0, 'u'},
	{"sync", required_argument, 0, 's'},
	{"restore", required_argument, 0, 'r'},
	{"install", required_argument, 0, 'i'},
	{"merge", required_argument, 0, 'm'},
	{"fetch", required_argument, 0, 'f'},
	{"delete", required_argument, 0, 'd'},
	{"exclude", required_argument, 0, 'e'},
	{"purge", no_argument, 0, 'p'},
	{"list", no_argument, 0, 'l'},
	{0, 0, 0, 0}};

int main(int argc, char *argv[])
{
  int c;
  int lastmode = 0;
  char *tmp;
#if defined(sun) || defined(ultrix)
  extern char* optarg;
  extern int optind;
#endif


  progname = argv[0];

  if (argc < 2) {
    Help();
  }

  tmp = getenv("PILOTPORT");
  if (tmp != NULL)
    device = tmp;
  
#ifdef __EMX__
  optmode = GETOPT_KEEP;
#else
  optind=1;
#endif  
  while (argv[optind] != NULL) {
    c = getopt_long(argc, argv, "b:u:s:e:r:i:m:f:d:plh", options, 0);
    if (c == EOF) {
	optarg=argv[optind++];
	c = lastmode;
	if (lastmode=='b' || lastmode=='u' || lastmode=='s' || lastmode=='r' || lastmode=='l' || lastmode=='p') {
	    fprintf(stderr, "'%c', only accepts one argument!\n", lastmode);
	    continue;
	}
	if (c == 0) {
	  device = optarg;
	  continue;
	}
    }
    switch (c) {
    case 'b':
      Backup(optarg, 0, 0);
      break;
    case 'u':
      Backup(optarg, 1, 0);
      break;
    case 's':
      Backup(optarg, 1, 1);
      break;
    case 'r':
      Restore(optarg);
      break;
    case 'i':
      Install(optarg);
      break;
    case 'm':
      Merge(optarg);
      break;
    case 'f':
      Fetch(optarg);
      break;
    case 'd':
      Delete(optarg);
      break;
    case 'e':
      MakeExcludeList(optarg);
      break;
    case 'p':
      Purge();
      break;
    case 'l':
      List();
      break;
    default:
      break;
    case 'h': case '?':
      Help();
    }
    lastmode=c;
  }
  if (lastmode==0)
    Help();
  
  Disconnect();
  
  return 0;
}
