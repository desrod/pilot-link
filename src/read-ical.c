/* todos.c:  Translate Pilot ToDo database into generic format
 *
 * Copyright (c) 1996, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pi-socket.h"
#include "todo.h"
#include "datebook.h"
#include "dlp.h"

main(int argc, char *argv[])
{
  struct pi_sockaddr addr;
  int db;
  int sd;
  int count;
  int i;
  int l;
  time_t t;
  int memo_size;
  char *memo_buf;
  FILE *f, *ical;
  struct PilotUser U;
  int ret;
  char buffer[0xffff];
  char cmd[128];
  struct ToDoAppInfo tai;

  if (argc < 3) {
#ifdef linux  
    fprintf(stderr,"usage:%s /dev/cua?? calfile # Calfile will be overwritten!\n",argv[0]);
#else
    fprintf(stderr,"usage:%s /dev/tty?? calfile # Calfile will be overwritten!\n",argv[0]);
#endif
    exit(2);
  }
  if (!(sd = pi_socket(AF_SLP, SOCK_STREAM, PF_PADP))) {
    perror("pi_socket");
    exit(1);
  }
    
  addr.sa_family = AF_SLP;
  addr.port = 3;
  strcpy(addr.device,argv[1]);
  
  ret = pi_bind(sd, &addr, sizeof(addr));
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
  
  /* Open the ToDo database, store access handle in db */
  if(dlp_OpenDB(sd, 0, 0x80|0x40, "ToDoDB", &db) < 0) {
    puts("Unable to open ToDoDB");
    dlp_AddSyncLogEntry(sd, "Unable to open ToDoDB.\n");
    exit(1);
  }
  
  dlp_ReadAppBlock(sd, db, 0, buffer, 0xffff);
  unpack_ToDoAppInfo(&tai, buffer, 0);
  
  unlink(argv[2]);
  sprintf(cmd, "ical -f - -calendar %s", argv[2]);
  ical = popen(cmd, "w");
  
  fprintf(ical,"calendar cal $ical(calendar)\n");
  
  for (i=0;1;i++) {
  	struct ToDo t;
  	int attr, category;
  	int j;
  	                           
  	int len = dlp_ReadRecordByIndex(sd, db, i, buffer, 0, 0, &attr, &category);
  	if(len<0)
  		break;
  		
  	/* Skip deleted records */
  	if((attr & dlpRecAttrDeleted) || (attr & dlpRecAttrArchived))
  		continue;
  		
	unpack_ToDo(&t, buffer, len);
	
	fprintf(ical,"set n [notice]\n");
	fprintf(ical,"$n text \"%s\"\n", t.description);
	fprintf(ical,"$n date [date today]\n");
	fprintf(ical,"$n todo 1\n");
	fprintf(ical,"$n option Priority %d\n", t.priority);
	fprintf(ical,"$n done %d\n", t.complete ? 1 : 0);
	fprintf(ical,"cal add $n\n");
	
	free_ToDo(&t);
  }

  /* Close the database */
  dlp_CloseDB(sd, db);

  dlp_AddSyncLogEntry(sd, "Read todos from Pilot.\n");

  /* Open the Datebook's database, store access handle in db */
  if(dlp_OpenDB(sd, 0, 0x80|0x40, "DatebookDB", &db) < 0) {
    puts("Unable to open DatebookDB");
    dlp_AddSyncLogEntry(sd, "Unable to open DatebookDB.\n");
    pi_close(sd);
    exit(1);
  }
  
  

  for (i=0;1;i++) {
  	int iflags;
  	int j;
  	struct Appointment a;
  	struct tm tm;
  	unsigned int d;
  	char * p, *p2;
  	int attr;
  	char delta[80];
  	char satisfy[256];
  	                           
  	int len = dlp_ReadRecordByIndex(sd, db, i, buffer, 0, 0, &attr, 0);
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
	  fprintf(ical,"$i length %d\n", end-start+1);
	}
	
	fprintf(ical,"$i text \"%s\"\n", a.description);
	
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
}

