/* sync-memodir.c:  Memos stored in a directory
 *
 * Copyright (c) 1996, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */
 
/* This program is a very rough take on synchronizing the memo pad on the Pilot
   with a Unix directory,using the technique that Pace Williams mentioned.
   
   It creates a file "Memos.dir" and a directory "Memos/" in the _current
   directory_!.
   
   "Memos" in turns contains "Backup" and "Archive" directories. Archived
   items will be moved to the archive directory. Do not touch the Backup
   directory, as its contents are integral to proper synchronization.
   
   Currently this code has full sync turned off, and will just copy the Pilot's
   database into the directory. (I.e., "overwrite PC" is turned on.)
   
   Categories are not supported in any way.
   
   If you understand what is going on, feel free to hack or improve.
   */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-datebook.h"
#include "pi-todo.h"
#include "pi-dlp.h"

#include "pi-sync.h"

/* Define our own LocalRecord type. */
struct LocalRecord {
        StandardLocalRecord;
        long ID;
	time_t mtime;
	char name[128];
	LocalRecord * next;
};

LocalRecord * memos = 0;

/* Define our own SyncAbs type */
struct SyncAbs {
	StandardSyncAbs;
	/* items down here are equivalent of C++ class data.
	   Any information needed to talk to databases, etc., goes in here.
	   If we were doing multiple memo dbs, then memos would belong in here instead
	   of a global. */
};

char * filename(char * memo) {
	static char buf[256];
	sprintf(buf,"Memos/%s",memo);
	return buf;
}

char * backupname(char * memo) {
	static char buf[256];
	sprintf(buf,"Memos/Backup/%s",memo);
	return buf;
}

char * newfilename(PilotRecord * r) {
	static char name[256];
	static char buf[256];
	struct stat stbuf;
	int i = 1;
	char *rec, *end;

	rec = r->record;
	end = &r->record[r->length];

	/* use first line as file name
	 * but change whitespace chars into '.'
	*/
	while( rec < end && isspace(*rec) )		/* skip whitespace */
		++rec;

	for( i = 0; rec < end; ++i, ++rec) {
		if( *rec == '\n' )
			break;
		else if( !isalnum(*rec) )
			name[i] = '.';
		else
			name[i] = *rec;
	}
	name[i] = '\0';

	if( *name == '\0' )				/* an empty memo */
		strcpy( name, "empty" );

	if (stat(filename(name), &stbuf) != 0)
		return name;

	/* file name already exists, tack on a unique number */
	for (i = 2; ; ++i) {
		sprintf(buf, "%s,%d", name, i);
		if (stat(filename(buf), &stbuf) != 0)
			return buf;
	}
}

char * newarchivename(PilotRecord * r, LocalRecord * l) {
	static char buf[256];
	static int i = 1;
	struct stat stbuf;
	for(;;) {
		sprintf(buf,"Memos/Archive/memo%d",i);
		if(stat(buf,&stbuf)) {
			break;
		}
		i++;
	}
	return buf;
}

/*  These are the important functions below. They implement the interface that
    the abstract synchronization layer invokes */

/* Get a Pilot ID for a local record, or 0 if no Pilot ID has been set. Any
   local ID mechanism is not relevent, only IDs given by the Pilot. */
unsigned long GetPilotID(SyncAbs * thisSA,LocalRecord * Local) {
	return Local->ID;
	/* return 1; */
}

/* Set the ID on a local record to match a given Pilot ID. */
int SetPilotID(SyncAbs * thisSA,LocalRecord * Local, unsigned long ID) {
	Local->ID = ID;
	return 1;
}

/* Given a PilotRecord, try and find a local record with a matching ID */
int MatchRecord(SyncAbs * thisSA,  LocalRecord ** Local, PilotRecord * p) {
	LocalRecord * m = memos;
	while(m) {
		if(m->ID == p->ID)
			break;
		m=m->next;
	}
	if(m)
		*Local = m;
	else
		*Local = 0;
	
	return 1;
}

/* Free up the LocalRecord returned by MatchRecord */
int FreeMatch(SyncAbs * thisSA,LocalRecord ** Local) {
	*Local = 0;
	return 0;
}

/* Iterate over all LocalRecords, in arbitrary order */
int Iterate(SyncAbs * thisSA, LocalRecord ** Local) {
	LocalRecord * m = *Local;
	if( !m) {
		m = memos;
	} else {
		m=m->next;
	}
	*Local = m;
	return m!=0;
}

/* Iterate over local records of a specified type. */
int IterateSpecific(SyncAbs * thisSA, LocalRecord ** Local, int flag, int archived) {
	LocalRecord * m = *Local;
	if( !m) {
		m = memos;
	} else {
		m=m->next;
	}
	while(m) {
	        if(archived) {
	          if(m->archived)
	        	break;
	        } else {
		  if(m->attr == flag)
			break;
		}
		m=m->next;
	}
	*Local = m;
	return m!=0;
}

/* Set status of local record */
int SetStatus(SyncAbs * thisSA,LocalRecord * Local, int status) {
	Local->attr = status;
	return 0;
}

/* There is no GetStatus, the abstract layer used Local->attr */

/* Set archival status of local record */
int SetArchived(SyncAbs * thisSA,LocalRecord * Local,int archived) {
        Local->archived = archived;
	return 0;
}

/* There is no GetStatus, the abstract layer used Local->archived */

/* Given a PilotRecord, store it in the local database */
int StoreRemote(SyncAbs * thisSA,PilotRecord* p) {
	LocalRecord * m;
	int h;
	struct stat stbuf;
	
	if(p->ID != 0) {
		/* replace record */
		m = memos;
		while(m) {
			if(m->ID == p->ID)
				break;
			m=m->next;
		}
		if (m) {
			h = open(filename(m->name),O_WRONLY|O_CREAT|O_TRUNC,0666);
			write(h, p->record, p->length-1);
			write(h, "\n", 1);
			close(h);
			stat(filename(m->name),&stbuf);
			m->mtime = stbuf.st_mtime;
			return 0;
		}
	}
	/* new record */
	m = (LocalRecord*)malloc(sizeof(LocalRecord));
	m->ID = p->ID;
	m->attr = p->attr;
	m->secret = p->secret;
	
	strcpy(m->name, newfilename(p));
	
	m->next = memos;
	memos = m;

	h = open(filename(m->name),O_WRONLY|O_CREAT|O_TRUNC,0666);
	write(h, p->record, p->length-1);
	write(h, "\n", 1);
	close(h);

	stat(filename(m->name),&stbuf);
	m->mtime = stbuf.st_mtime;
	
	return 0;	
}

/* Given a local record, construct a PilotRecord suitable for transmission
to a Pilot */
PilotRecord * Transmit(SyncAbs* thisSA ,LocalRecord* Local) {
        static PilotRecord p;
	int h = open(filename(Local->name),O_RDONLY);
	
	struct stat statbuf;
	stat(filename(Local->name),&statbuf);
	p.length = statbuf.st_size+1;
	p.record = (unsigned char*)malloc(p.length);
	read(h, p.record, p.length-1);
	p.record[p.length-1] = '\0';
	close(h);
	
	p.category = 0;
	p.attr = Local->attr;
	p.archived = Local->archived;
	p.secret = Local->secret;
	
	return &p;
}

/* Free PilotRecord created by Transmit */
int FreeTransmit(SyncAbs* thisSA,LocalRecord* Local,PilotRecord* Remote) {
	free(Remote->record);
	return 0;
}

/* Find a local backup record and compare it to the pilot record for inequality */
int CompareBackup(SyncAbs * thisSA, LocalRecord* m, PilotRecord* p) {
	char buffer[0xffff];
	int r,len;

  	r = open(backupname(m->name),O_RDONLY);
  	
  	if (r<0)
  	  return -1; /* "less", arbitrary */
  	
  	len = read(r,buffer,0xffff);
	close(r);
	
	if(len != p->length)
		return -1; /* "less", arbitrary */

	if(buffer[len-1] == '\n')
		buffer[len-1] = '\0';
	
	return memcmp(buffer, p->record, len);
}

/* Compare a local record and pilot record for inequality */
int Compare(SyncAbs * thisSA, LocalRecord * m, PilotRecord* p) {
	char buffer[0xffff];
  	int r = open(filename(m->name),O_RDONLY);
  	int len = read(r,buffer,0xffff);
	close(r);

	if(len != p->length)
		return -1; /* "less", arbitrary */

	if(buffer[len-1] == '\n')
		buffer[len-1] = '\0';
	
	return memcmp(buffer, p->record, len);
}

/* Delete all local records */
int DeleteAll(SyncAbs * thisSA) {
	while(memos) {
		LocalRecord * m = memos;
		memos = memos->next;
		if(m->attr != RecordDeleted) {
			unlink(filename(m->name));
		}
		free(m);
	}
	return 0;
}

/* Do a local purge, deleting all records marked Deleted, and
   archiving all records marked for archiving */
int Purge(SyncAbs * thisSA) {
	LocalRecord * prev = 0;
	LocalRecord * m = memos, *next;
	while(m) {
		next = m->next;
		if((m->attr == RecordDeleted) || (m->archived)) {
			unlink(filename(m->name));
			if(prev)
				prev->next = next;
			else
				memos=next;
			free(m);
		} else
			prev = m;
		m = next;
	}
	return 0;
}

/* Add remote record to archive. l is non-NULL if there is a matching local record */
int ArchiveRemote(SyncAbs * s, LocalRecord * l, PilotRecord * p) {
	char name[256];
	int h;
	strcpy(name,"Memos/Archive/memoXXXXXX");

	h = open(newarchivename(p,l),O_WRONLY|O_CREAT|O_TRUNC,0666);
	write(h, p->record, p->length-1);
	write(h, "\n", 1);
	close(h);
	
	return 0;
}

struct file {
	char name[128];
	struct file * next;
};

struct file * files = 0;

int main(int argc, char *argv[])
{
  struct pi_sockaddr addr;
  int db;
  int sd;
  struct PilotUser U;
  struct dirent * dirent;
  DIR * d;
  int ret;
  FILE * f;
  struct SyncAbs abs;

  /* Set up abstraction structure */  
  abs.MatchRecord = MatchRecord;
  abs.Iterate = Iterate;
  abs.IterateSpecific = IterateSpecific;
  abs.SetStatus = SetStatus;
  abs.SetArchived = SetArchived;
  abs.SetPilotID = SetPilotID;
  abs.GetPilotID = GetPilotID;
  abs.StoreRemote = StoreRemote;
  abs.ArchiveLocal = 0;             /* missing */
  abs.ClearStatusArchiveLocal = 0;  /* likewise */
  abs.ArchiveRemote = ArchiveRemote;
  abs.DeleteAll = DeleteAll;
  abs.Purge = Purge;
  abs.CompareBackup = CompareBackup;
  abs.Compare = Compare;
  abs.Transmit = Transmit;
  abs.FreeTransmit = FreeTransmit;

  fprintf(stderr, "A Warning be upon Ye: Here Be Dragons!\nThis program is incomplete, ill-considered, and unreliable!\nDo not Rely on without Consideration of Completing the program...\n\n");
  
  if (argc < 2) {
    fprintf(stderr,"usage:%s %s # A directory called Memos will be created!\n",argv[0],TTYPrompt);
    exit(2);
  }
  if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP))) {
    perror("pi_socket");
    exit(1);
  }
  
  mkdir("Memos",0700);
  mkdir("Memos/Archive",0700);
  mkdir("Memos/Backup",0700);
  
  d = opendir("Memos");
  while( (dirent = readdir(d)) ) {
  	struct file * f = (struct file*)malloc(sizeof(struct file));
  	if(dirent->d_name[0] == '.')
  		continue;
  	if(strcmp(dirent->d_name,"Archive")==0)
  		continue;
  	if(strcmp(dirent->d_name,"Backup")==0)
  		continue;
  	if(strcmp(dirent->d_name,"Memos.dir")==0)
  		continue;
  	if(dirent->d_name[strlen(dirent->d_name)-1] == '~')
  		continue;
  	f->next = files;
  	files = f;
  	strcpy(f->name, dirent->d_name);
  	printf("|%s|\n", f->name);
  }
  closedir(d);
  
  f = fopen(filename("Memos.dir"),"r");
  while (f && !feof(f)) {
  	long l;
  	LocalRecord * m = (LocalRecord*)malloc(sizeof(LocalRecord));
  	if(fscanf(f, "%lu %lu %s", &m->ID, &l, &m->name[0])<3)
  		break;
  	m->mtime = (time_t)l;
  	m->attr = RecordNothing;
  	m->secret = 0;
  	m->archived = 0;
  	m->next = memos;
  	printf("Read in memo '%s'\n",m->name);
  	memos = m;
  }
  fclose(f);
  
  /* Iterate over files in Memos directory, recording new and modified memos */
  {
  	struct file * f = files;
  	while(f) {
  		LocalRecord * m = memos;
		struct stat stbuf;
		char buf[256];
		sprintf(buf,"Memos/%s",f->name);
		stat(buf, &stbuf);
		
  		while(m) {
  			if(strcmp(m->name,f->name)==0)
  				break;
  			m=m->next;
  		}
  		if(!m) {
  			m = (LocalRecord*)malloc(sizeof(LocalRecord));
  			strcpy(m->name, f->name);
  			m->ID = 0;
  			m->attr = RecordNew;
  			m->secret = 0;
  			m->archived = 0;
  			m->next = memos;
			printf("Memo %s is new\n", m->name);
  			memos = m;
  		} else {
			if(stbuf.st_mtime > m->mtime) {
				m->attr = RecordModified;
				printf("Memo %s is modified\n", m->name);
			}
  		}
		m->mtime = stbuf.st_mtime;
  		f=f->next;
  	}
  }

  /* Iterate over memo list, marking deleted memos */
  {
  	LocalRecord * m = memos;
  	while(m) {
  		struct file * f = files;
  		while(f) {
  			if(strcmp(m->name,f->name)==0)
  				break;
  			f=f->next;
  		}
  		if(!f) {
  			m->attr = RecordDeleted;
			printf("Memo %s is deleted\n", m->name);
  		}
  		m=m->next;
  	}
  }
  

  addr.pi_family = PI_AF_SLP;
  strcpy(addr.pi_device,argv[1]);
  
  ret = pi_bind(sd, (struct sockaddr*)&addr, sizeof(addr));
  if(ret == -1) {
    perror("pi_bind");
    exit(1);
  }

  ret = pi_listen(sd,1);
  if(ret == -1) {
    perror("pi_listen");
    exit(1);
  }

  sd = pi_accept(sd, 0, 0);
  if(sd == -1) {
    perror("pi_accept");
    exit(1);
  }

  /* Ask the pilot who it is. */
  dlp_ReadUserInfo(sd,&U);
  
  /* Tell user (via Pilot) that we are starting things up */
  dlp_OpenConduit(sd);


#if 0
  if (dlp_CreateDB(sd, 'KeAl', 'data', 0, 0, 1, "XMemoDB", &db)<0) {
    dlp_OpenDB(sd, 0, dlpOpenRead|dlpOpenWrite, "XMemoDB", &db);
  }
#endif
                 
  /* Open the Datebook's database, store access handle in db */
  if(dlp_OpenDB(sd, 0, dlpOpenRead|dlpOpenWrite, "MemoDB", &db) < 0) {
    puts("Unable to open MemoDB");
    dlp_AddSyncLogEntry(sd, "Unable to open MemoDB.\n");
    pi_close(sd);
    exit(1);
  }

#if 1  
  /* Trigger overwrite of local with remote data. */
  CopyFromRemote(sd, db, &abs);
#endif

#if 0
  /* Trigger overwrite of remote with local data. */
  CopyToRemote(sd, db, &abs);
#endif

#if 0
  /* For testing slow syncs, get rid of the flags, since we don't need them */
  dlp_ResetSyncFlags(sd, db);

  SlowSync(sd, db, &abs);

  /* Reset the flags now that we are done with them. */
  dlp_ResetSyncFlags(sd, db);
#endif

  /* Close the database */
  dlp_CloseDB(sd, db);
  
  /* Delete previous backup */
  d = opendir("Memos/Backup");
  while( (dirent = readdir(d)) ) {
        char name[256];
  	if(dirent->d_name[0] == '.')
  		continue;
  	sprintf(name,"Memos/Backup/%s", dirent->d_name);
  	unlink(name);
  }
  closedir(d);

  /* Backup current memos */
  {
  	LocalRecord * m = memos;
  	while(m) {
  		char buffer[0xffff];
  		int r = open(filename(m->name),O_RDONLY);
  		int w = open(backupname(m->name),O_WRONLY|O_CREAT|O_TRUNC,0600);
  		write(w,buffer, read(r,buffer,0xffff));
  		close(r);
  		close(w);
  		m=m->next;
  	}
  }

  dlp_AddSyncLogEntry(sd, "Read memos from Pilot.\n");

  dlp_ReadUserInfo(sd, &U);
  
  U.lastSyncPC = 0xDEADBEEF;
  
  dlp_WriteUserInfo(sd, &U);
    
  pi_close(sd);  

  /* Rewrite memo index */
  f = fopen(filename("Memos.dir"),"w");
  while(memos) {
  	fprintf(f,"%lu %lu %s\n", memos->ID, (unsigned long)memos->mtime, memos->name);
  	memos = memos->next;
  }
  fclose(f);

  return 0;
}

