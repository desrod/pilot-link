/* sync up all the DB's */

#include "pi-socket.h"

static unsigned char SyncConfirm[] = { 0x2a, 0x01, 0xa0, 0x00, 0x00, 0x00 };

static unsigned char DatebookData[] = { 0x18, 0x01, 0x20, 0x19, 0x64, 0x61, 
					0x74, 0x65, 0x44, 0x41, 0x54, 0x41,
					0,    0,    0,    0,    0,    0,
					0x44, 0x61, 0x74, 0x65, 0x62, 0x6f,
					0x6f, 0x6b, 0x44, 0x42, 0};

static unsigned char confirm[][32] = {
  "Address Book",
  "Date Book",
  "Memo Pad",
  "To Do List",
  "Restore",
  "Install",
  ""
};

SyncAll(int sd)
{
  /* add new things here.  This should be dynamic */

  SyncDB(sd, "AddressDB", 0, 0);
  SyncDB(sd, "DatebookDB", DatebookData, sizeof(DatebookData));
  SyncDB(sd, "MemoDB", 0, 0);
  SyncDB(sd, "ToDoDB", 0, 0);
}

ConfirmAll(int sd)
{
  unsigned char buf[256];
  unsigned char *bp;
  int i;

  memcpy(buf, SyncConfirm, sizeof(SyncConfirm));

  bp = &buf[6];
  for (i=0; strlen(confirm[i]); i++) {

    strcpy(bp, "OK ");
    bp += 3;
    buf[5] += 3;

    strcpy(bp, confirm[i]);
    bp += strlen(confirm[i]);
    buf[5] += strlen(confirm[i]);

    *(bp++) = 0x0a;
    buf[5]++;
  }
  *(--bp) = 0;

  pi_write(sd, buf, buf[5] + 6);
  pi_read(sd, buf, 64);
}
