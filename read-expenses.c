/* read-expenses.c: Sample code to translate Pilot Expense database into generic format
 *
 * Copyright (c) 1997, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-expense.h"
#include "pi-dlp.h"

                  
int main(int argc, char *argv[])
{
  struct pi_sockaddr addr;
  int db;
  int sd;
  int i;
  struct PilotUser U;
  int ret;
  unsigned char buffer[0xffff];
  unsigned char buffer2[0xffff];
  struct ExpenseAppInfo tai;
  struct ExpensePrefs tp;
  
  if (argc < 2) {
    fprintf(stderr,"usage:%s %s\n",argv[0],TTYPrompt);
    exit(2);
  }
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

  /* Note that under PalmOS 1.x, you can only read preferences before the DB is opened */
  ret = dlp_ReadAppPreference(sd, Expense_Creator, Expense_Pref, 1, 0xffff, buffer, 0, 0);
  
  /* Open the ToDo database, store access handle in db */
  if(dlp_OpenDB(sd, 0, 0x80|0x40, "ExpenseDB", &db) < 0) {
    puts("Unable to open ExpenseDB");
    dlp_AddSyncLogEntry(sd, "Unable to open ExpenseDB.\n");
    exit(1);
  }
  
  if (ret >= 0) {
    unpack_ExpensePrefs(&tp, buffer, 0);
    pack_ExpensePrefs(&tp, buffer2, &i);
    fprintf(stderr, "Orig prefs, %d bytes:\n", ret);
    dumpdata(buffer, ret);
    fprintf(stderr, "New prefs, %d bytes:\n", ret);
    dumpdata(buffer2, i);
    fprintf(stderr, "Expense prefs, current category %d, default category %d\n",
    	tp.currentCategory, tp.defaultCategory);
    fprintf(stderr, "  Note font %d, Show all categories %d, Show currency %d, Save backup %d\n",
    	tp.noteFont, tp.showAllCategories, tp.showCurrency, tp.saveBackup);
    fprintf(stderr, "  Allow quickfill %d, Distance unit %d, Currencies:\n",
    	tp.allowQuickFill, tp.unitOfDistance);
    for(i=0;i<7;i++) {
      fprintf(stderr, " %d", tp.currencies[i]);
    }
    fprintf(stderr, "\n");
  }
  

  ret = dlp_ReadAppBlock(sd, db, 0, buffer, 0xffff);
  unpack_ExpenseAppInfo(&tai, buffer, 0);
  pack_ExpenseAppInfo(&tai, buffer2, &i);
  fprintf (stderr, "Orig length %d, new length %d, orig data:\n", ret, i);
  dumpdata(buffer,ret);
  fprintf (stderr, "New data:\n");
  dumpdata(buffer2, i);
  
  fprintf(stderr, "Expense app info, sort order %d\n", tai.sortOrder);
  fprintf(stderr, " Currency 1, name '%s', symbol '%s', rate '%s'\n",
  	tai.currencies[0].name, tai.currencies[0].symbol, tai.currencies[0].rate);
  fprintf(stderr, " Currency 2, name '%s', symbol '%s', rate '%s'\n",
  	tai.currencies[1].name, tai.currencies[1].symbol, tai.currencies[1].rate);
  fprintf(stderr, " Currency 3, name '%s', symbol '%s', rate '%s'\n",
  	tai.currencies[2].name, tai.currencies[2].symbol, tai.currencies[2].rate);
  fprintf(stderr, " Currency 4, name '%s', symbol '%s', rate '%s'\n",
  	tai.currencies[3].name, tai.currencies[3].symbol, tai.currencies[3].rate);
  
  for (i=0;1;i++) {
  	struct Expense t;
  	int attr, category;
  	                           
  	int len = dlp_ReadRecordByIndex(sd, db, i, buffer, 0, 0, &attr, &category);
  	if(len<0)
  		break;
  		
  	/* Skip deleted records */
  	if((attr & dlpRecAttrDeleted) || (attr & dlpRecAttrArchived))
  		continue;
  		
	unpack_Expense(&t, buffer, len);
	pack_Expense(&t, buffer2, &ret);
	fprintf(stderr, "Orig length %d, data:\n", len);
	dumpdata(buffer, len);
	fprintf(stderr, "New length %d, data:\n", ret);
	dumpdata(buffer2, ret);
	
	fprintf(stderr,"Category: %s\n", tai.CategoryName[category]);
	fprintf(stderr,"Type: %d, Payment: %d, Currency: %d\n", t.type, t.payment, t.currency);
	fprintf(stderr,"Amount: '%s', Vendor: '%s', City: '%s'\n",
		t.amount ?t.amount: "<None>", t.vendor ? t.vendor: "<None>", t.city ?t.city: "<None>");
	fprintf(stderr,"Attendees: '%s', Note: '%s'\n", t.attendees ?t.attendees: "<None>", t.note ?t.note: "<None>");
	fprintf(stderr,"Date: %s", asctime(&t.date));
	fprintf(stderr,"\n");

	free_Expense(&t);
  }

  /* Close the database */
  dlp_CloseDB(sd, db);

  dlp_AddSyncLogEntry(sd, "Read expenses from Pilot.\n");

  pi_close(sd);  
  exit(0);
}

