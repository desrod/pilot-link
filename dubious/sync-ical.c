/* sync-ical.c:  Synchronize ical calendar file
 *
 * Copyright (c) 1996, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */
 
/* This program is an experiment in synchronizing the Pilot datebook
   database with an Ical database. It will not do anything useful for you.
   Yet. If you are interested in exploring the syncronization process, read
   lib/sync.c and sync-memodir.c */
   
/* At the moment, this file is an absolute mess, but it's functional, more
   or less */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-sync.h"
#include "pi-datebook.h"
#include "pi-todo.h"
#include "pi-dlp.h"

/* Set up a quick'n'dirty emulation of the Tcl interpreter commands */
typedef struct {
	int r, w;
	char * result;
} Tcl_Interp;

int Tcl_VarEval(Tcl_Interp * interp, ...) {
	static char result[4096];
	int l;
	va_list args;
	
	va_start(args, interp);
	
	write(interp->w, "puts [catch {", 13);
	for(;;) {
		char * c = va_arg(args, char*);
		if (!c)
			break;
		
		write(interp->w, c, strlen(c));
	}
	write(interp->w, " } retval]\nputs -nonewline $retval\nflush stdout\n", 48);
	
	va_end(args);
	
	l = read(interp->r, result, 4096);
	if (l<0)
		l=0;
	result[l] = '\0';
	
	interp->result = result;

	while(*interp->result != '\n')
		interp->result++;
		
	*((interp->result)++) = '\0';
	
	return atoi(result);;
}

int Tcl_Eval(Tcl_Interp * interp, char * cmd) {
	return Tcl_VarEval(interp, cmd, NULL);
}

char * tclquote(char * in) {
  static char * buffer = 0;
  char * out;
  char * pos;
  int len;

  len = 3;
  pos = in;
  while(*pos) {
    if((*pos == '\\') || (*pos == '"') || (*pos == '[') || (*pos == '{') || (*pos == '$'))
      len++;
    len++;
    pos++;
  }

  if (buffer)
    free(buffer);
  buffer = (char*)malloc(len);
  out = buffer;

  pos = in;
  *out++ = '"';
  while(*pos) {
    if((*pos == '\\') || (*pos == '"') || (*pos == '[') || (*pos == '{') || (*pos == '$'))
      *out++ = '\\';
    *out++=*pos++;
  }
  *out++ = '"';
  *out++ = '\0';

  return buffer;
}



/* Define our own LocalRecord type. */
struct LocalRecord {
     StandardLocalRecord;
     char * item; /* non-null and a name if an Ical item */
     unsigned long deleted;  /* non-null if only a deletion ID */
};
                                        
/* Define our own SyncAbs type */
struct SyncAbs {
     StandardSyncAbs;
     /* items down here are equivalent of C++ class data.
     Any information needed to talk to databases, etc., goes in here.
      */
    Tcl_Interp * ical;
    char * cal;
    int todo; /* Are we interested in todos or memos? */
};

/* Get a Pilot ID for a local record, or 0 if no Pilot ID has been set. Any
   local ID mechanism is not relevent, only IDs given by the Pilot. */
unsigned long GetPilotID(SyncAbs * thisSA,LocalRecord * Local) {
	char * var;
	/*char id[10];*/

	if (Local->deleted)
		return Local->deleted;
		
	Tcl_VarEval(thisSA->ical, 
	  "set result 0\n",
	  "catch { set result [", Local->item, " option sync_id]}\n",
	  "set result",
	  NULL);

        if ((thisSA->ical->result[0] - '0') != thisSA->todo)
          return 0;
	
	var = thisSA->ical->result+1;
	
	return atoi(var);
}

/* Set the ID on a local record to match a given Pilot ID. */
int SetPilotID(SyncAbs * thisSA,LocalRecord * Local, unsigned long ID) {
	char buf[40];
	sprintf(buf,"%d%lu", thisSA->todo?1:0, ID);
	Tcl_VarEval(thisSA->ical, Local->item, " option sync_id ",buf,NULL);
	return 1;
}

/* Given a PilotRecord, try and find a local record with a matching ID */
int MatchRecord(SyncAbs * thisSA,  LocalRecord ** Local, PilotRecord * p) {
	static LocalRecord lr;
	char buf[40];
	sprintf(buf,"%d%lu", thisSA->todo?1:0, p->ID);
	
	/* Check active items */
	Tcl_VarEval(thisSA->ical,
	  "set result \"\"\n", 
	  thisSA->cal, " incalendar [", thisSA->cal, " main] item {\n",
	  "  catch { \n",
	  "    if {[string compare [$item option sync_id] \"", buf, "\"]==0} {\n",
	  "      set result $item\n",
	  "    }\n",
	  "  }\n",
	  "}\n",
	  "set result",
          NULL);
	
	if (strlen(thisSA->ical->result)) {
		*Local = &lr;
		lr.item = strdup(thisSA->ical->result);
		lr.deleted = 0;
		return 1;
	}
	
	/* Check deleted items */
	Tcl_VarEval(thisSA->ical,
          "set result \"\"\n",
          "catch {\n",
          "  set deleted [",thisSA->cal," option deleted]\n",
          "  foreach item $deleted {\n",
          "    if {[string compare $item  \"",buf,"\"]==0} {\n",
          "      set result $item\n",
          "      break\n",
          "    }\n",
          "  }\n",
          "}\n",
          "set result",
          NULL);
          
	if (strlen(thisSA->ical->result)) {
		*Local = &lr;
		lr.item = 0;
		lr.deleted = atoi(thisSA->ical->result+1);
		return 1;
	} else {
		*Local = 0;
		return 0;
	}
}

/* Free up the LocalRecord returned by MatchRecord */
int FreeMatch(SyncAbs * thisSA,LocalRecord ** Local) {
	*Local = 0;
	return 0;
}


/* Iterate over all LocalRecords, in arbitrary order */
int Iterate(SyncAbs * thisSA, LocalRecord ** Local) {
	static LocalRecord lr;
	
	if (!*Local) {
	  Tcl_VarEval(thisSA->ical,
            "set active {}\n",
            thisSA->cal," incalendar [", thisSA->cal, " main] item {\n",
            "  # some dohicky to make sure record's todo matches thisSA->todo\n",
            "  lappend active $item\n",
            "}\n",
            "set deleted {}\n",
            "catch {\n",
            "  set deleted [",thisSA->cal," option deleted]\n",
            "}\n",
            NULL);
	}
	
	Tcl_VarEval(thisSA->ical,
	  "set result {}\n",
	  "if {[llength $active]>0} {\n",
	  "  set result [lindex $active 0]\n",
	  "  set active [lreplace 1 end]\n",
	  "}\n",
	  "set result",
	  NULL);
	
	if (strlen(thisSA->ical->result)) {
		*Local = &lr;
		lr.item = strdup(thisSA->ical->result);
		lr.deleted = 0;
		return 1;
	}

	Tcl_VarEval(thisSA->ical,
	  "set result {}\n",
	  "if {[llength $deleted]>0} {\n",
	  "  set result [lindex $deleted 0]\n",
	  "  set deleted [lreplace $deleted 1 end]\n",
	  "}\n",
	  "set result",
	  NULL);

	
	if (strlen(thisSA->ical->result)) {
		*Local = &lr;
		lr.item = 0;
		lr.deleted = atoi(thisSA->ical->result);
		return 1;
	}

	return *Local!=0;
}

/* Iterate over local records of a specified type. */
int IterateSpecific(SyncAbs * thisSA, LocalRecord ** Local, int flag, int archived)
{
	static LocalRecord lr;
	
	if (!*Local) {
	  Tcl_VarEval(thisSA->ical,
	    "set active {}\n",
	    thisSA->cal," incalendar [", thisSA->cal, " main] item {\n",
	    "  lappend active $item\n",
	    "}\n",
	    "set deleted {}\n",
	    "catch {\n",
	    "  set deleted [",thisSA->cal," option deleted]\n",
	    "}\n",
	    NULL);
	}
	
	Tcl_VarEval(thisSA->ical,
	  "set result {}\n",
	  "if {[llength $active]>0} {\n",
	  "  set result [lindex $active 0]\n",
	  "  set active [lreplace 1 end]\n",
	  "}\n",
	  "set result",
	  NULL);
	
	if (strlen(thisSA->ical->result)) {
		*Local = &lr;
		lr.item = strdup(thisSA->ical->result);
		lr.deleted = 0;
		return 1;
	}

	Tcl_VarEval(thisSA->ical,
	  "set result {}\n",
	  "if {[llength $deleted]>0} {\n",
	  "  set result [lindex $deleted 0]\n",
	  "  set deleted [lreplace $deleted 1 end]\n",
	  "}\n",
	  "set result",
	  NULL);

	
	if (strlen(thisSA->ical->result)) {
		*Local = &lr;
		lr.item = 0;
		lr.deleted = atoi(thisSA->ical->result);
		return 1;
	}

	return *Local!=0;
}

/* Given a PilotRecord, store it in the local database */
int StoreRemote(SyncAbs * thisSA,PilotRecord* p) {
	struct Appointment a;
	int j;
	char buf[256];
	
	unpack_Appointment(&a, p->record, p->length);
	
	if (a.event) {
	  Tcl_Eval(thisSA->ical,"set i [notice]");

	} else {
	  int start,end;

	  Tcl_Eval(thisSA->ical,"set i [appointment]");
	  
	  start = a.begin.tm_hour*60 + a.begin.tm_min;
	  end   = a.end.tm_hour*60 + a.end.tm_min;

	  sprintf(buf, "%d", start);
	  Tcl_VarEval(thisSA->ical,"$i starttime ",buf, NULL);
	  sprintf(buf, "%d", end-start+1);
	  Tcl_VarEval(thisSA->ical,"$i length ",buf, NULL);
	}
	sprintf(buf, "%lu", p->ID);
        Tcl_VarEval(thisSA->ical,"$i option sync_id ",buf, NULL);
	
        Tcl_VarEval(thisSA->ical,"$i text ",tclquote(a.description), NULL);
	
	sprintf(buf,"set begin [date make %d %d %d]\n", a.begin.tm_mday,a.begin.tm_mon+1,a.begin.tm_year+1900);
	Tcl_Eval(thisSA->ical,buf);
	
	if (a.repeatFrequency) {
	  if (a.repeatType == repeatDaily) {
	    sprintf(buf,"$i dayrepeat %d $begin", a.repeatFrequency);
	    Tcl_Eval(thisSA->ical,buf);
	  } else if(a.repeatType == repeatMonthlyByDate) {
	    sprintf(buf,"$i month_day %d $begin %d", a.begin.tm_mon+1,a.repeatFrequency);
	    Tcl_Eval(thisSA->ical,buf);
	  } else if(a.repeatType == repeatMonthlyByDay) {
	    if (a.repeatDay>=domLastSun) {
	      sprintf(buf,"$i month_last_week_day %d 1 $begin %d\n", a.repeatDay % 7 + 1,
	                                                    a.repeatFrequency);
	      Tcl_Eval(thisSA->ical,buf);
	    } else {
	      sprintf(buf,"$i month_week_day %d %d $begin %d\n", a.repeatDay % 7 + 1,
	                                                    a.repeatDay / 7 + 1,
	                                                    a.repeatFrequency);
	      Tcl_Eval(thisSA->ical,buf);
	    }
	  } else if(a.repeatType == repeatWeekly) {
	    int i;
	    strcpy(buf,"$i weekdays ");
	    for (i=0;i<7;i++)
	      if (a.repeatDays[i])
	        sprintf(buf+strlen(buf), "%d ", i+1);
            strcat(buf,"\n");
            Tcl_Eval(thisSA->ical,buf);
	  } else if(a.repeatType == repeatYearly) {
	    sprintf(buf,"$i monthrepeat %d $begin\n", 12 * a.repeatFrequency);
	    Tcl_Eval(thisSA->ical,buf);
	  }
	  Tcl_Eval(thisSA->ical,"$i start $begin\n");
	  if (!a.repeatForever) {
	    sprintf(buf,"$i finish [date make %d %d %d]\n", a.repeatEnd.tm_mday, a.repeatEnd.tm_mon+1, 
	                                               a.repeatEnd.tm_year+1900);
	    Tcl_Eval(thisSA->ical,buf);
	  }
	  if (a.exceptions)
	    for (j=0;j<a.exceptions;j++) {
	      sprintf(buf,"$i deleteon [date make %d %d %d]\n", a.exception[j].tm_mday,
	                                                   a.exception[j].tm_mon+1,
	                                                   a.exception[j].tm_year+1900);
	      Tcl_Eval(thisSA->ical,buf);
	    }
	} else 
          Tcl_Eval(thisSA->ical,"$i date $begin\n");
	
	Tcl_VarEval(thisSA->ical,thisSA->cal," add $i\n",NULL);

	free_Appointment(&a);

	return 0;	
}


/* Delete all local records */
int DeleteAll(SyncAbs * thisSA) {

	Tcl_VarEval(thisSA->ical,
	  thisSA->cal," incalendar [", thisSA->cal, " main] item {\n",
	  "  $item delete\n",
	  "}\n",
	  thisSA->cal," option deleted {}",
	  NULL);
	return 0;
}



/*===========================================================*/
#if 0
/*  These are the important functions below. They implement the interface that
    the abstract synchronization layer invokes */

/* Get a Pilot ID for a local record, or 0 if no Pilot ID has been set. Any
   local ID mechanism is not relevent, only IDs given by the Pilot. */
unsigned long GetPilotID(SyncAbs * thisSA,LocalRecord * Local) {
	return Local->ID;
	return 1;
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

/* There is no GetStatus, the abstract layer uses Local->attr */

/* Set archival status of local record */
int SetArchived(SyncAbs * thisSA,LocalRecord * Local,int archived) {
        Local->archived = archived;
	return 0;
}

/* There is no GetStatus, the abstract layer uses Local->archived */

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

#endif
/*===========================================================*/
                                                                                         
                                                                                         
/*typedef struct{
	char calname[40];
	int r,w;
} LocalData;

typedef struct {
	char recname[40];
	long PilotID;
} LocalRecord;*/

void dosend(char*text, int w) {
	write(w,text,strlen(text));
	write(w,"flush stdout\n",13);
}

void doread(char*buffer, int r) {
	int l = read(r,buffer,4096);
	buffer[l] = 0;
}

int main(int argc, char *argv[])
{
  struct pi_sockaddr addr;
  int db;
  int sd;
  struct PilotUser U;
  int in[2], out[2];
  int ret;
  struct SyncAbs abs;
  /*struct SyncAbs abs;*/
/*   = {MatchRecord, IsNew, IsModified, IsArchived, IsNothing,
                        AppendLocal, AppendArchive, StartIterator, NextIterator};*/
  /*LocalData ld;
  LocalRecord *lrp;*/
  Tcl_Interp ical;

#if 0
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
#endif
  abs.DeleteAll = DeleteAll;
  abs.StoreRemote = StoreRemote;

#if 0  
  abs.MatchRecord = MatchRecord;
  abs.IsNew = IsNew;
  abs.GetPilotID = GetPilotID;
  abs.IsModified = IsModified;
  abs.IsArchived = IsArchived;
  abs.IsNothing = IsNothing;
  abs.AppendLocal = AppendLocal;
  abs.AppendArchive = AppendArchive;
#endif  

  fprintf(stderr, "A Warning be upon Ye: Here Be Dragons!\nThis program is incomplete, ill-considered, and unreliable!\nDo not Rely on without Consideration of Completing the program...\n\n");

  if (argc < 3) {
    fprintf(stderr,"usage:%s %s calfile # Calfile will be overwritten!\n",argv[0],TTYPrompt);
    exit(2);
  }
  if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP))) {
    perror("pi_socket");
    exit(1);
  }

  pipe(&in[0]);
  pipe(&out[0]);
  
  if(!fork()) {
  	close(in[1]);
  	close(out[0]);
  	dup2(in[0],fileno(stdin));
  	dup2(out[1],fileno(stdout));
  	system("ical -f -");
  	exit(0);
  }
  
  ical.w = in[1];
  ical.r = out[0];
  
  /*ical = Tcl_CreateInterp();
  
  app_init(ical);*/
  
  Tcl_VarEval(&ical, "calendar calmain ",argv[2]);

  abs.ical = &ical;
  abs.cal = "calmain";
  
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
  
  /* Open the Datebook's database, store access handle in db */
  if(dlp_OpenDB(sd, 0, dlpOpenReadWrite, "DatebookDB", &db) < 0) {
    puts("Unable to open DatebookDB");
    dlp_AddSyncLogEntry(sd, "Unable to open DatebookDB.\n");
    pi_close(sd);
    exit(1);
  }
  
  CopyFromRemote(sd, db, &abs);
  
/*  SyncDB(sd, &db, &abs, &ld);*/

  /* Close the database */
  dlp_CloseDB(sd, db);
  
  dlp_ReadUserInfo(sd, &U);
  
  U.lastSyncPC = 0xDEADBEEF;
    
  dlp_WriteUserInfo(sd, &U);
        

  dlp_AddSyncLogEntry(sd, "Read datebook from Pilot.\n");
  
  Tcl_VarEval(&ical,abs.cal," save [",abs.cal," main]",NULL);


  close(ical.w);
  close(ical.r);

  pi_close(sd);  
  
  return 0;
}

