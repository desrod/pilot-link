/* pilot-dedupe.c:  Pilot utility to remove duplicate records
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"

char * progname;

void Help(void)
{
  fprintf(stderr,"usage:%s %s dbname [dbname ...]\n",progname,TTYPrompt);
  exit(2);
}

struct record {
	unsigned long id;
	char * data;
	int len;
	int cat;
	int index;
	struct record * next;
};

int compare_r(const void * av, const void * bv)
{
	struct record * a = *(struct record**)av;
	struct record * b = *(struct record**)bv;
	int i,o;

	if (a->cat < b->cat)
		o = -1;
	else if (a->cat > b->cat)
		o = 1;
	else if (a->len < b->len)
		o = -1;
	else if (a->len > b->len)
		o = 1;
	else if ((i = memcmp(a->data, b->data, a->len)))
		o = i;
	else if (a->index < b->index)
		o = -1;
	else if (a->index > b->index)
		o = 1;
	else
		o = 0;

	return o;
}

struct record * records = 0;


int main(int argc, char *argv[])
{
  struct pi_sockaddr addr;
  int db;
  int sd;
  int l;
  struct PilotUser U;
  int ret;
  char buf[0xffff];
  int i;
  struct record * r;
#ifdef sun
  extern char* optarg;
  extern int optind;
#endif

  progname = argv[0];

  if (argc < 3)
    Help();

  if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP))) {
    perror("pi_socket");
    exit(1);
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
  
  for (i=2;i<argc;i++) {
    struct record ** sortidx;
    int dupe=0;
    int j,k;
    int count;
    
    /* Open the database, store access handle in db */
    printf("Opening %s\n", argv[i]);
    if(dlp_OpenDB(sd, 0, dlpOpenReadWrite, argv[i], &db) < 0) {
      printf("Unable to open %s\n", argv[i]);
      /*dlp_AddSyncLogEntry(sd, "Unable to open AddressDB.\n");
      exit(1);*/
      continue;
    }

    printf("Reading records...\n");
  
    l=0;
    count=0;
    for(;;) {
      int attr;
      recordid_t id;
      int cat;
      int len = dlp_ReadRecordByIndex(sd, db, l, (unsigned char *)buf, &id, 0, &attr, &cat);

      l++;
      
      if(len<0)
        break;
                          
      /* Skip deleted records */
      if((attr & dlpRecAttrDeleted) || (attr & dlpRecAttrArchived))
        continue;

      count++;

      r = (struct record*)malloc(sizeof(struct record));
      r->data = (char*)malloc(len);
      memcpy(r->data, buf, len);
      r->len = len;
      r->cat = cat;
      r->id = id;
      r->index = l;
      
      r->next = records;
      records = r;
      
    }
    
    sortidx = malloc(sizeof(struct record *) * count);
    r = records;
    for (k=0; r && (k<count); k++, r=r->next)
    	sortidx[k] = r;
    
    qsort(sortidx, count, sizeof(struct record*), compare_r);

    printf("Scanning for duplicates...\n");
    
    for (k=0; k<count; k++) {
      struct record * r2;
      int d = 0;
      r = sortidx[k];
      
      if (r->len < 0)
        continue;
      
      for (j=k+1; j<count; j++) {
        r2 = sortidx[j];
        
        if (r2->len < 0)
          continue;
          
        if ((r->len != r2->len) || memcmp(r->data, r2->data, r->len))
          break;
        
        printf("Deleting record %d, duplicate #%d of record %d\n", r2->index, ++d, r->index);
        dupe++;
        dlp_DeleteRecord(sd, db, 0, r2->id);
        
        r2->len = -1;
        r2->id = 0;
        
      }
      k = j-1;
      
    }

    free(sortidx);
    
    while(records) {
      if (records->data)
        free(records->data);
      r = records;
      records=records->next;
      free(r);
    }
    
    /* Close the database */
    dlp_CloseDB(sd, db);
    sprintf(buf,"Removed %d duplicates from %s\n", dupe, argv[i]);
    printf("%s",buf);
    dlp_AddSyncLogEntry(sd, buf);
  }

  dlp_ResetLastSyncPC(sd);
  
  pi_close(sd);
  return 0;
}
