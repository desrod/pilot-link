// This files serves no use as a program for you to run.  It is simply used
// for validation of the function in the classes provided
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream.h>
#include <unistd.h>
#include <errno.h>

#include "pi-source.h"
#include "pi-file.h"
#include "pi-todo.h"	
#include "pi-memo.h"	
#include "pi-datebook.h"
#include "pi-address.h"

typedef void (*funcPtr_t)(void *);

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
	  cerr << "Palm's packed buffer had size " << pilotSize << endl;
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

unsigned char packedBuf[0xffff], kenBuf[0xffff];
int size, packedSize1, packedSize2, kenSize, nentries, entnum;
void *packed;

void memos(void *buf) 
{
     memo_t memo(buf);
     
     packed = memo.pack(&packedSize1);
     
     packedSize2 = sizeof(packedBuf);
     if (memo.pack(packedBuf, &packedSize2) == NULL) {
	  cerr << "Record number " << (entnum + 1) << " too big for "
	       << "the buffer you passed in." << endl;
	  return;
     }
     
     Memo m;
     unpack_Memo(&m, (unsigned char *) buf, size);
     kenSize = pack_Memo(&m, kenBuf, 0xffff);
     free_Memo(&m);
     
     compare(buf, size, packed, packedSize1,
	     packedBuf, packedSize2, kenBuf, kenSize);
     
     delete packed;
}

void todos(void *buf) 
{
     todo_t todo(buf);
     
     packed = todo.pack(&packedSize1);
     
     packedSize2 = sizeof(packedBuf);
     todo.pack(packedBuf, &packedSize2);
     
     ToDo m;
     unpack_ToDo(&m, (unsigned char *) buf, size);
     kenSize = pack_ToDo(&m, kenBuf, 0xffff);
     free_ToDo(&m);
     
     compare(buf, size, packed, packedSize1,
	     packedBuf, packedSize2, kenBuf, kenSize);
     
     delete packed;
}

void appointments(void *buf) 
{
     appointment_t appointment(buf);

     packed = appointment.pack(&packedSize1);
     
     // Fake packed + 7, as it's not used
     unsigned char *ptr = (unsigned char *) buf;
     *(((unsigned char *)packed) + 7) = *(ptr + 7);
     
     packedSize2 = sizeof(packedBuf);
     if (appointment.pack(packedBuf, &packedSize2) == NULL) {
	  cerr << "Record number " << (entnum + 1) << " too big for "
	       << "the buffer you passed in." << endl;
	  return;
     }
     packedBuf[7] = *(ptr + 7);
     
     Appointment m;
     unpack_Appointment(&m, ptr, size);
     kenSize = pack_Appointment(&m, kenBuf, 0xffff);
     free_Appointment(&m);
     kenBuf[7] = *(ptr + 7);
     
     // Fake the byte just before the description.  It's not used
     if (appointment.repeatType() != appointment_t::none) {
	  int fakePos = 15;
	  if (appointment.hasAlarm()) 
	       fakePos += 2;
	  
	  *(((unsigned char *)packed) + fakePos) = *(ptr + fakePos);
	  packedBuf[fakePos] = *(ptr + fakePos);
	  kenBuf[fakePos] = *(ptr + fakePos);
     }

     compare(buf, size, packed, packedSize1,
	     packedBuf, packedSize2, kenBuf, kenSize);
     
     delete packed;
}
     
void addresses(void *buf) 
{
     address_t address(buf);
     
     packed = address.pack(&packedSize1);
     
     // buf[0] is gapfill, so make them match
     unsigned char *ptr = (unsigned char *) buf;
     
     *(((unsigned char *) packed)) = *ptr;
     
     packedSize2 = sizeof(packedBuf);
     if (address.pack(packedBuf, &packedSize2) == NULL) {
	  cerr << "Record number " << (entnum + 1) << " too big for "
	       << "the buffer you passed in." << endl;
	  return;
     }
     packedBuf[0] = *ptr;
     
     Address m;
     pi_buffer_t *record;

     record = pi_buffer_new();
     unpack_Address(&m, record, address_v1);
     pi_buffer_free(record);
     record = pi_buffer_new();
     pack_Address(&m, record, address_v1);
     free_Address(&m);
     
     kenBuf[0] = *ptr; /* From before pi_buffer-ification. What did it do? */
     
     compare(buf, size, packed, packedSize1,
	     packedBuf, packedSize2, record->buf, record->used);
     pi_buffer_free(record);     
     delete packed;
}

/* ARGSUSED */
int main(int argc, char **argv)
{
     chdir("/afs/pdx/u/g/r/grosch/.xusrpilot/backup");

     struct {
	  char *db;
	  funcPtr_t func;
     } *aptr, apps[] = {
	  { "MemoDB.pdb", memos },
	  { "ToDoDB.pdb", todos },
	  { "DatebookDB.pdb", appointments },
	  { "AddressDB.pdb", addresses },
	  { NULL, NULL }
     };

     void *app_info, *buf;
     int app_info_size, attrs;
     pi_file *pf;
     
     for (aptr = &apps[0]; aptr->db; aptr++) {
	  cout << "Examining " << aptr->db << endl;
	  
	  if ((pf = pi_file_open(aptr->db)) == NULL) {
	       cerr << "\tUnable to open " << aptr->db << ": " 
		    << strerror(errno) << endl;
	       continue;
	  }

	  if (pi_file_get_app_info(pf, &app_info, &app_info_size) < 0) {
	       cerr << "Unable to get app info" << endl;
	       continue;
	  }
	  
	  pi_file_get_entries(pf, &nentries);
	  
	  for (int entnum = 0; entnum < nentries; entnum++) {
	       if (pi_file_read_record(pf, entnum, (void **) &buf, &size,
				       &attrs, NULL, NULL) < 0) {
		    cout << "Error reading record number " << entnum << endl;
		    continue;
	       }
	       
	       if ((attrs & dlpRecAttrDeleted) || (attrs & dlpRecAttrArchived))
		    continue;

	       (*(aptr->func))(buf);
	  }
	  
	  pi_file_close(pf);
     }
     
     return 0;
}

