/* Simple going-through-the-motions addressDB thing */

#include <stdio.h>
#include "pi-socket.h"

static unsigned char SyncOpen[] = { 0x2e, 0 };
static unsigned char AppData[] = { 0x17, 0x01, 0x20, 0x0c, 0, 0xe0 };
static unsigned char AppDataRetrieve[] = { 0x1b, 0x01, 0x20, 0x06 };
static unsigned char DataRecordRetrieve[] = {0x1f, 0x01, 0x20, 0x01 };
static unsigned char WriteAddressDBTemplate[] = {0x1b, 0x01, 0xa0, 0, 2, 0x82, 1, 0};

do_addressdb(int sd)
{
  unsigned char buf[64];
  unsigned char *data;
  int i;
  int index;
  int size;

  data = (unsigned char *)malloc(1024);

  /* We're going to sync something now */

  pi_write(sd,SyncOpen,sizeof(SyncOpen));
  pi_read(sd,buf,64);

  /* get a handle for the AddressDB */

  memcpy(buf, AppData, sizeof(AppData));
  strcpy(&buf[sizeof(AppData)],"AddressDB");

  pi_write(sd, buf, sizeof(AppData) + strlen("AddressDB") + 1);
  pi_read(sd, buf, 64);

  if (buf[1] == 1) {
    index = buf[6];

    /* read the AddressDB template */

    memcpy(buf, AppDataRetrieve, sizeof(AppDataRetrieve));
    buf[4] = index;
    buf[5] = buf[6] = buf[7] = 0;
    buf[8] = buf[9] = 0xff;        /* presumably the template flag */
    pi_write(sd, buf, 0x0a);

    size = pi_read(sd, data, 1024);

    /* I suppose we need to look into what's in here and store it, but
       for now, we just echo it back, after fudging the header.
       Actually, I'll but we don't need this right now....*/
#if 0
    memcpy(data, WriteAddressDBTemplate, sizeof(WriteAddressDBTemplate));
    pi_write(sd, data, size);

    pi_read(sd, buf, 64);  /* this gives [9c 00 00 00] */
#endif

    do {

      memcpy(buf, DataRecordRetrieve, sizeof(DataRecordRetrieve));
      buf[sizeof(DataRecordRetrieve)] = index;

      pi_write(sd, buf, sizeof(DataRecordRetrieve) + 1);
      size = pi_read(sd, data, 1024);
    } while (data[1] == 1);

    /* next, an annoying stream of "Things that close the Database (tm)",
       all the same as the RecordRetrieve, except for the command number */

    buf[0] = 0x26;
    pi_write(sd, buf, sizeof(DataRecordRetrieve) + 1);
    pi_read(sd, data, 1024);

    buf[0] = 0x2b;       /* Oh they _like_ to do the 0x2b thing... twice! */
    pi_write(sd, buf, sizeof(DataRecordRetrieve) + 1);
    pi_read(sd, data, 1024);

    buf[0] = 0x2b;
    pi_write(sd, buf, sizeof(DataRecordRetrieve) + 1);
    pi_read(sd, data, 1024);

    buf[0] = 0x27;
    pi_write(sd, buf, sizeof(DataRecordRetrieve) + 1);
    pi_read(sd, data, 1024);

    buf[0] = 0x19;
    pi_write(sd, buf, sizeof(DataRecordRetrieve) + 1);
    pi_read(sd, data, 1024);
  }
  /* That's all folks! */
}
