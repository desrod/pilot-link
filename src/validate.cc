// This files serves no use as a program for you to run.  It is simply used
// for validation of the function in the classes provided
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream.h>
#include <unistd.h>
#include "pi-file.h"
#include "pi-todo.h"	
#include "pi-memo.h"	
#include "pi-datebook.h"
#include "pi-address.h"

void dump (void *buf, int n) {
     int i, j, c;
     
     for (i = 0; i < n; i += 16) {
	  printf ("%04x: ", i);
	  for (j = 0; j < 16; j++) {
	       if (i+j < n)
		    printf ("%02x ", ((unsigned char *)buf)[i+j]);
	       else
		    printf ("   ");
	  }
	  printf ("  ");
	  for (j = 0; j < 16 && i+j < n; j++) {
	       c = ((unsigned char *)buf)[i+j] & 0x7f;
	       if (c < ' ' || c >= 0x7f)
		    putchar ('.');
	       else
		    putchar (c);
	  }
	  printf ("\n");
     }
}

// This makes sure all the data matches.  It compares with what Ken gets
// so that, if mine is wrong, I can tell if it's my code or his code.
void compare(void *fromPilot, int pilotSize, void *pack1, int pack1size, void *pack2, int pack2size, void *kenPack, int kenPackSize) 
{
     if (pack1size != pack2size) {
	  cerr << "Pack with malloc returned a size of " << pack1size << endl;
	  cerr << "Pack with buffer returned a size of " << pack2size << endl;
	  return;
     }

     if (pack1size != pilotSize) {
	  cerr << "My pack routine gave a size of " << pack1size << endl;
	  cerr << "Pilot's packed buffer had size " << pilotSize << endl;
	  return;
     }

     if (pack1size != kenPackSize) {
	  cerr << "My pack routine gave a size of " << pack1size << endl;
	  cerr << "Ken's pack routine had size of " << kenPackSize << endl;
	  // If we get here, it just means ken got it wrong, not me.  Go on
     }

     if (memcmp(pack1, pack2, pack1size)) {
	  cerr << "My two pack routines produced different data!" << endl;
	  return;
     }

     if (memcmp(pack1, fromPilot, pack1size)) {
	  cerr << "My data is different from the pilot data" << endl;
	  dump(pack1, pack1size);
	  dump(fromPilot, pilotSize);
	  return;
     }

     if (memcmp(pack1, kenPack, pack1size)) 
	  cerr << "Ken's packed data differs from the pilot" << endl;
}

uchar_t *buf, packedBuf[0xffff], kenBuf[0xffff];
int size, packedSize1, packedSize2, kenSize, attrs, cat, nentries;
recordid_t uid;
void *packed, *app_info;
int app_info_size;
pi_file *pf;

void memos() 
{
     cout << "Examining memo database" << endl;

     pf = pi_file_open("MemoDB.pdb");
     
     if (pi_file_get_app_info(pf, &app_info, &app_info_size) < 0) {
	  cerr << "Unable to get app info" << endl;
	  return;
     }

     pi_file_get_entries(pf, &nentries);
     
     memo_t memo;
     
     for (int entnum = 0; entnum < nentries; entnum++) {
	  if (pi_file_read_record(pf, entnum, (void **) &buf, &size,
				  &attrs, &cat, &uid) < 0) {
	       cout << "Error reading record number " << entnum << endl;
	       return;
	  }

	  if ((attrs & dlpRecAttrDeleted) || (attrs & dlpRecAttrArchived))
	       continue;

	  memo.unpack(buf);

	  packed = memo.pack(&packedSize1);
	  
	  packedSize2 = sizeof(packedBuf);
	  if (memo.pack(packedBuf, &packedSize2) == NULL) {
	       cerr << "Record number " << (entnum + 1) << " too big for "
		    << "the buffer you passed in." << endl;
	       continue;
	  }

	  Memo m;
	  unpack_Memo(&m, buf, size);
	  pack_Memo(&m, kenBuf, &kenSize);
	  free_Memo(&m);

	  compare(buf, size, packed, packedSize1, packedBuf, packedSize2, kenBuf, kenSize);
	  
	  delete packed;
     }

     pi_file_close(pf);
}

void todos() 
{
     cout << "Examining todo database" << endl;

     pf = pi_file_open("ToDoDB.pdb");
     
     if (pi_file_get_app_info(pf, &app_info, &app_info_size) < 0) {
	  cerr << "Unable to get app info" << endl;
	  return;
     }

     pi_file_get_entries(pf, &nentries);
     
     todo_t todo;
     
     for (int entnum = 0; entnum < nentries; entnum++) {
	  if (pi_file_read_record(pf, entnum, (void **) &buf, &size,
				  &attrs, &cat, &uid) < 0) {
	       cout << "Error reading record number " << entnum << endl;
	       return;
	  }

	  if ((attrs & dlpRecAttrDeleted) || (attrs & dlpRecAttrArchived))
	       continue;

	  todo.unpack(buf);

	  packed = todo.pack(&packedSize1);
	  
	  packedSize2 = sizeof(packedBuf);
	  todo.pack(packedBuf, &packedSize2);
	  
	  ToDo m;
	  unpack_ToDo(&m, buf, size);
	  pack_ToDo(&m, kenBuf, &kenSize);
	  free_ToDo(&m);
	  
	  compare(buf, size, packed, packedSize1, packedBuf, packedSize2, kenBuf, kenSize);
	  
	  delete packed;
     }

     pi_file_close(pf);
}


void appointments() 
{
     cout << "Examining datebook database" << endl;

     pf = pi_file_open("DatebookDB.pdb");
     
     if (pi_file_get_app_info(pf, &app_info, &app_info_size) < 0) {
	  cerr << "Unable to get app info" << endl;
	  return;
     }

     pi_file_get_entries(pf, &nentries);
     
     appointment_t appointment;
     
     for (int entnum = 0; entnum < nentries; entnum++) {
	  if (pi_file_read_record(pf, entnum, (void **) &buf, &size,
				  &attrs, &cat, &uid) < 0) {
	       cout << "Error reading record number " << entnum << endl;
	       return;
	  }

	  if ((attrs & dlpRecAttrDeleted) || (attrs & dlpRecAttrArchived))
	       continue;

	  appointment.unpack(buf);

	  packed = appointment.pack(&packedSize1);

	  // Fake packed + 7, as it's not used
	  *(((uchar_t *)packed) + 7) = buf[7];
	  
	  packedSize2 = sizeof(packedBuf);
	  if (appointment.pack(packedBuf, &packedSize2) == NULL) {
	       cerr << "Record number " << (entnum + 1) << " too big for "
		    << "the buffer you passed in." << endl;
	       continue;
	  }
	  packedBuf[7] = buf[7];
	  
	  Appointment m;
	  unpack_Appointment(&m, buf, size);
	  pack_Appointment(&m, kenBuf, &kenSize);
	  free_Appointment(&m);
	  packedBuf[7] = buf[7];
	  
	  compare(buf, size, packed, packedSize1, packedBuf, packedSize2, kenBuf, kenSize);

	  delete packed;
     }

     pi_file_close(pf);
}
     

void addresses() 
{
     cout << "Examining address database" << endl;

     pf = pi_file_open("AddressDB.pdb");
     
     if (pi_file_get_app_info(pf, &app_info, &app_info_size) < 0) {
	  cerr << "Unable to get app info" << endl;
	  return;
     }

     pi_file_get_entries(pf, &nentries);
     
     address_t address;
     
     for (int entnum = 0; entnum < nentries; entnum++) {
	  if (pi_file_read_record(pf, entnum, (void **) &buf, &size,
				  &attrs, &cat, &uid) < 0) {
	       cout << "Error reading record number " << entnum << endl;
	       return;
	  }

	  if ((attrs & dlpRecAttrDeleted) || (attrs & dlpRecAttrArchived))
	       continue;

	  address.unpack(buf);

	  packed = address.pack(&packedSize1);

	  // buf[0] is gapfill, so make them match
	  *(((uchar_t *) packed)) = buf[0];
	  
	  packedSize2 = sizeof(packedBuf);
	  if (address.pack(packedBuf, &packedSize2) == NULL) {
	       cerr << "Record number " << (entnum + 1) << " too big for "
		    << "the buffer you passed in." << endl;
	       continue;
	  }
	  packedBuf[0] = buf[0];
	  
	  Address m;
	  unpack_Address(&m, buf, size);
	  pack_Address(&m, kenBuf, &kenSize);
	  free_Address(&m);
	  packedBuf[0] = buf[0];
	  
	  compare(buf, size, packed, packedSize1, packedBuf, packedSize2, kenBuf, kenSize);

	  delete packed;
     }

     pi_file_close(pf);
}

/* ARGSUSED */
int main(int argc, char **argv)
{
     chdir("/afs/pdx/u/g/r/grosch/.xusrpilot/backup");
     
     memos();
     todos();
     appointments();
     addresses();
     
     return 0;
}

