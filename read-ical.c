/* read-ical.c:  Translate Pilot ToDo and Datebook databases into ical 2.0 format
 *
 * Copyright (c) 1996, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-todo.h"
#include "pi-datebook.h"
#include "pi-dlp.h"

char * tclquote(char * in)
{
  static char * buffer = 0;
  char * out;
  char * pos;
  int len;
  
  /* Skip leading bullet (and any whitespace after) */
  if (in[0] == '\x95') {
    ++in;
    while(in[0] == ' ' || in[0] == '\t') {
	++in;
    }
  }

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

static void Usage(char *progname)
{
    fprintf(stderr,"Usage: %s [-d] [-p pubtext] [%s] calfile\n"
	    "Note: calfile will be overwritten!\n"
	    "   -d         : Datebook only, no Todos\n"
	    "   -p pubtext : Replace text of items not starting with a bullet "
	    "with pubtext\n",
	progname, TTYPrompt);
    exit(2);
}

int main(int argc, char *argv[])
{
  struct pi_sockaddr addr;
  int db;
  int sd;
  int i;
  FILE *ical;
  struct PilotUser U;
  int ret;
  unsigned char buffer[0xffff];
  char cmd[128];
  struct ToDoAppInfo tai;
  int read_todos = 1;
  char *pubtext = NULL;
  char *progname = argv[0];
  char *port = getenv("PILOTPORT");

  if (!port) port = "/dev/pilot";

  while (argc > 1 && argv[1][0] == '-') {
    if (!strcmp(argv[1], "-d")) {
	/* Datebook only */
	read_todos = 0;
    } else if (!strcmp(argv[1], "-p")) {
	/* Public only, text supplied */
	if (argv[2] == NULL) {
	    Usage(progname);
	}
	pubtext = argv[2];
	--argc; ++argv;
    } else {
	Usage(progname);
    }
    --argc; ++argv;
  }

  if (argc != 2 && argc != 3) {
    Usage(progname);
  }
  if (argc == 3) {
    port = argv[1];
    --argc; ++argv;
  }
  if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP))) {
    perror("pi_socket");
    exit(1);
  }
    
  addr.pi_family = PI_AF_SLP;
  strcpy(addr.pi_device,port);
  
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
  
  unlink(argv[1]);
  sprintf(cmd, "ical -f - -calendar %s", argv[1]);
  ical = popen(cmd, "w");
  
  fprintf(ical,"calendar cal $ical(calendar)\n");
  
  if (read_todos) {
  /* Open the ToDo database, store access handle in db */
  if(dlp_OpenDB(sd, 0, 0x80|0x40, "ToDoDB", &db) < 0) {
    puts("Unable to open ToDoDB");
    dlp_AddSyncLogEntry(sd, "Unable to open ToDoDB.\n");
    exit(1);
  }
  
  dlp_ReadAppBlock(sd, db, 0, buffer, 0xffff);
  unpack_ToDoAppInfo(&tai, buffer, 0);
  
  for (i=0;1;i++) {
  	struct ToDo t;
  	int attr, category;
  	recordid_t id;
  	char id_buf[255];
  	                           
  	int len = dlp_ReadRecordByIndex(sd, db, i, buffer, &id, 0, &attr, &category);
  	if(len<0)
  		break;
  		
  	/* Skip deleted records */
  	if((attr & dlpRecAttrDeleted) || (attr & dlpRecAttrArchived))
  		continue;
  		
	unpack_ToDo(&t, buffer, len);
	
	fprintf(ical,"set n [notice]\n");
	/* '\x95' is the "bullet" character */
	fprintf(ical,"$n text %s\n", tclquote((pubtext &&
	    t.description[0] != '\x95') ? pubtext : t.description));
	fprintf(ical,"$n date [date today]\n");
	fprintf(ical,"$n todo 1\n");
	fprintf(ical,"$n option Priority %d\n", t.priority);
        sprintf(id_buf, "%lx", id);
        fprintf(ical,"$i option PilotRecordId %s\n", id_buf);
	fprintf(ical,"$n done %d\n", t.complete ? 1 : 0);
	fprintf(ical,"cal add $n\n");
	
	free_ToDo(&t);
  }

  /* Close the database */
  dlp_CloseDB(sd, db);

  dlp_AddSyncLogEntry(sd, "Read todos from Pilot.\n");
  }

  /* Open the Datebook's database, store access handle in db */
  if(dlp_OpenDB(sd, 0, 0x80|0x40, "DatebookDB", &db) < 0) {
    puts("Unable to open DatebookDB");
    dlp_AddSyncLogEntry(sd, "Unable to open DatebookDB.\n");
    pi_close(sd);
    exit(1);
  }
  
  

  for (i=0;1;i++) {
  	int j;
  	struct Appointment a;
  	int attr;
  	recordid_t id;
  	char id_buf[255];
  	                           
  	int len = dlp_ReadRecordByIndex(sd, db, i, buffer, &id, 0, &attr, 0);
  	if(len<0)
  		break;
  		
  	/* Skip deleted records */
  	if((attr & dlpRecAttrDeleted) || (attr & dlpRecAttrArchived))
  		continue;
  		
	unpack_Appointment(&a, buffer, len);
	
	if (a.event) {
	  fprintf(ical,"set i [notice]\n");

	} else {
	  int start,end;

	  fprintf(ical,"set i [appointment]\n");
	  
	  start = a.begin.tm_hour*60 + a.begin.tm_min;
	  end   = a.end.tm_hour*60 + a.end.tm_min;

	  fprintf(ical,"$i starttime %d\n", start);
	  fprintf(ical,"$i length %d\n", end-start);
	}
	
	/* '\x95' is the "bullet" character */
	fprintf(ical,"$i text %s\n", tclquote((pubtext &&
	    a.description[0] != '\x95') ? pubtext : a.description));
	
	fprintf(ical,"set begin [date make %d %d %d]\n", a.begin.tm_mday,a.begin.tm_mon+1,a.begin.tm_year+1900);
	
	if (a.repeatFreq) {
	  if (a.repeatType == repeatDaily) {
	    fprintf(ical,"$i dayrepeat %d $begin\n", a.repeatFreq);
	  } else if(a.repeatType == repeatMonthlyByDate) {
	    fprintf(ical,"$i month_day %d $begin %d\n",a.begin.tm_mon+1,a.repeatFreq);
	  } else if(a.repeatType == repeatMonthlyByDay) {
	    if (a.repeatOn>=domLastSun) {
	      fprintf(ical,"$i month_last_week_day %d 1 $begin %d\n", a.repeatOn % 7 + 1,
	                                                    a.repeatFreq);
	    } else {
	      fprintf(ical,"$i month_week_day %d %d $begin %d\n", a.repeatOn % 7 + 1,
	                                                    a.repeatOn / 7 + 1,
	                                                    a.repeatFreq);
	    }
	  } else if(a.repeatType == repeatWeekly) {
	    /*
	     * Handle the case where the user said weekly repeat, but
	     * really meant daily repeat every n*7 days.  Note: We can't
	     * do days of the week and a repeat-frequency > 1, so do the
	     * best we can and go on.
	     */
	    if (a.repeatFreq > 1) {
		int ii, found;
		for (ii = 1, found = 0; ii < 128; ii <<= 1) {
		    if (a.repeatOn & i)
			found++;
		}
		if (found > 1)
		    fprintf(stderr, "Incomplete translation of %s\n",
			    a.description);
		fprintf(ical,"$i dayrepeat %d $begin\n", a.repeatFreq * 7);
	    } else {
		fprintf(ical,"$i weekdays ");
		if(a.repeatOn & 1)
		  fprintf(ical,"1 ");
		if(a.repeatOn & 2)
		  fprintf(ical,"2 ");
		if(a.repeatOn & 4)
		  fprintf(ical,"3 ");
		if(a.repeatOn & 8)
		  fprintf(ical,"4 ");
		if(a.repeatOn & 16)
		  fprintf(ical,"5 ");
		if(a.repeatOn & 32)
		  fprintf(ical,"6 ");
		if(a.repeatOn & 64)
		  fprintf(ical,"7 ");
		fprintf(ical,"\n");
	    }
	  } else if(a.repeatType == repeatYearly) {
	    fprintf(ical,"$i monthrepeat %d $begin\n", 12 * a.repeatFreq);
	  }
	  fprintf(ical,"$i start $begin\n");
	  if (!a.repeatForever)
	    fprintf(ical,"$i finish [date make %d %d %d]\n", a.repeatEnd.tm_mday, a.repeatEnd.tm_mon+1, 
	                                               a.repeatEnd.tm_year+1900);
	  if (a.exceptions)
	    for (j=0;j<a.exceptions;j++)
	      fprintf(ical,"$i deleteon [date make %d %d %d]\n", a.exception[j].tm_mday,
	                                                   a.exception[j].tm_mon+1,
	                                                   a.exception[j].tm_year+1900);
	} else 
          fprintf(ical,"$i date $begin\n");
	
        sprintf(id_buf, "%lx", id);
        fprintf(ical,"$i option PilotRecordId %s\n", id_buf);
        
	fprintf(ical,"cal add $i\n");

	free_Appointment(&a);
		
  }
  
  fprintf(ical,"cal save [cal main]\n");
  fprintf(ical,"exit\n");
  
  pclose(ical);

  /* Close the database */
  dlp_CloseDB(sd, db);

  dlp_AddSyncLogEntry(sd, "Read datebook from Pilot.\n");

  pi_close(sd);  
  
  return 0;
}

