/* pilot-xfer.c:  Pilot Database transfer utility
 *
 * (c) 1996, Kenneth Albanowski.
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include <sys/stat.h>
#include "dlp.h"
#include "pi-socket.h"
#include "pi-file.h"

int sd = 0;
char * device;
char * progname;

void Connect(void) {
  struct pi_sockaddr addr;
  int ret;

  if (sd!=0)
    return;
      
  if (!(sd = pi_socket(AF_SLP, SOCK_STREAM, PF_PADP))) {
    perror("pi_socket");
    exit(1);
  }

  addr.sa_family = AF_SLP;
  addr.port = 3;
  strcpy(addr.device,device);
  
  ret = pi_bind(sd, &addr, sizeof(addr));
  if(ret == -1) {
    perror("pi_bind");
    exit(1);
  }
    
  printf("Waiting for connection (press the HotSync button now)...\n");

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

void Backup(char * dirname)
{
  int i;
  
  Connect();
  
  mkdir(dirname,0700);

  i = 0;
  while(1) {
  	struct DBInfo info;
  	struct pi_file * f;
  	char name[256];

        if (dlp_OpenConduit(sd)<0) {
	  puts("Exiting on cancel, all data _not_ backed up.");
          exit(1);
        }
        
  	if( dlp_ReadDBList(sd, 0, 0x80, i, &info) < 0)
  		break;
  	i = info.index + 1;

  	sprintf(name, "%s/%s", dirname, info.name);
  	if (info.flags & dlpDBFlagResource)
  	  strcat(name,".prc");
  	else
  	  strcat(name,".pdb");
  	  
  	printf("Backing up '%s'... ", name);
  	fflush(stdout);
  	
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
  	
  dlp_FindDBInfo(sd, 0, 0, dbname, 0, 0, &info);
        
  strcpy(name, dbname);
  if (info.flags & dlpDBFlagResource)
    strcat(name,".prc");
  else
    strcat(name,".pdb");
  	  
  printf("Fetching '%s'... ", name);
  fflush(stdout);
  	
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

void Restore(char * dirname)
{
  DIR * dir;
  struct dirent * dirent;
  
  Connect();

  dir = opendir(dirname);
  
  while( (dirent = readdir(dir)) ) {
        struct pi_file * f;
        char name[256];
        
        if (dirent->d_name[0] == '.')
           continue;

	if ( dlp_OpenConduit(sd) < 0) {
	   puts("Exiting on cancel. All data not restored.");
	   exit(1);
	}
        sprintf(name,"%s/%s",dirname,dirent->d_name);
  	f = pi_file_open(name);
  	if (f==0) {
  	  printf("Unable to open '%s'!\n", name);
  	  break;
  	}
  	printf("Restoring %s... ", name);
  	fflush(stdout);
  	if(pi_file_install(f, sd, 0)<0)
  	  printf("failed.\n");
  	else
  	  printf("OK\n");
  	pi_file_close(f);
  }
  
  closedir(dir);

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

  printf("Install done\n");
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
  while(1) {
  	if( dlp_ReadDBList(sd, 0, 0x80, i, &info) < 0)
  		break;
  	i = info.index + 1;
  	
  	printf("'%s'\n", info.name);
  }
  
  printf("List done.\n");
}

void Help(void)
{
      printf("Usage: %s %s [ -b(ackup)  backupdir ]\n",progname,TTYPrompt);
      printf("                  [ -r(estore) backupdir ]\n");
      printf("                  [ -i(nstall) filename ]\n");
      printf("                  [ -f(etch)   dbname ]\n");
      printf("                  [ -l(ist) ]\n");
      exit(0);
}

int main(int argc, char *argv[])
{
  char c;
#ifdef sun
  extern char* optarg;
  extern int optind;
#endif

  
  progname = argv[0];

  if (argc < 3) {
    Help();
  }

  device = argv[1];
  argc--;
  argv++;
  
  optind = 0;
  while ((c = getopt(argc, argv, "b:r:i:f:lh")) != -1) {
    switch (c) {
    case 'b':
      Backup(optarg);
      break;
    case 'r':
      Restore(optarg);
      break;
    case 'i':
      Install(optarg);
      break;
    case 'f':
      Fetch(optarg);
      break;
    case 'l':
      List();
      break;
    default:
    case 'h': case '?':
      Help();
    }
  }
  
  Disconnect();
  
  exit(0);
}

