/* simple server test */

#include <stdio.h>
#include "pi-socket.h"

static unsigned char Ident[] = { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static unsigned char User[] = { 0x10, 0};
static unsigned char QueryRS[] = { 0x16, 0x01, 0x20, 0x04, 0x40, 0, 0, 0};
static unsigned char QueryDB[] = { 0x16, 0x01, 0x20, 0x04, 0x80, 0, 0, 0};

static unsigned char SyncOpen[] = { 0x2e, 0 };
static unsigned char EOS[] = { 0x2f, 0x01, 0x02, 0, 0};

static unsigned char AppData[] = { 0x17, 0x01, 0x20, 0x0c, 0, 0xe0 };
static unsigned char AppDataRetrieve[] = { 0x1b, 0x01, 0x20, 0x06 };
static unsigned char DataRecordRetrieve[] = {0x1f, 0x01, 0x20, 0x01 };


main(int argc, char *argv[])
{
  struct pi_sockaddr addr;
  int sd;
  int i;
  char *buf;

  if (argc != 2) {
    fprintf(stderr,"usage:%s /dev/tty??\n",argv[0]);
    exit(2);
  }

  if (!(sd = pi_socket(AF_SLP, SOCK_STREAM, PF_PADP))) {
    perror("pi_socket");
    exit(1);
  }

  addr.sa_family = AF_SLP;
  addr.port = 3;
  strcpy(addr.device,argv[1]);

  pi_bind(sd, &addr, sizeof(addr));

  pi_listen(sd,0);

  buf = (char *)malloc(4096);  /* some huge (in Pilot terms) working space */

  pi_write(sd,Ident,sizeof(Ident));
  pi_write(sd,User,sizeof(User));
  pi_read(sd,buf,64);

  i = 0;
  do {
    *(unsigned short *)(&QueryRS[6]) = htons(i);

    pi_write(sd,QueryRS,sizeof(QueryRS));
    pi_read(sd,buf,64);
    i = ntohs(*(unsigned short *)(&buf[6])) + 1;
  } while (buf[1] == 1);

  i = 0;
  do {
    *(unsigned short *)(&QueryDB[6]) = htons(i);

    pi_write(sd,QueryDB,sizeof(QueryDB));
    pi_read(sd,buf,64);
    i = ntohs(*(unsigned short *)(&buf[6])) + 1;
  } while (buf[1] == 1);

  do_addressdb(sd);
  do_datebookdb(sd);
  do_memodb(sd);

  /* I think this is End Of Session */

  pi_write(sd,EOS,sizeof(EOS));
  pi_read(sd,buf,64);

  /* wait a second, for things to close */
  sleep(1);
}
