#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include "prc.h"
#include "pi-socket.h"

static unsigned char AppLoad[] = { 0x12, 0x00 };
static unsigned char AppLoadName[] = { 0x1a, 0x01, 0, 0, 0, 0 };
static unsigned char AppLoadHeader[] = { 0x18, 0x01 };
static unsigned char AppLoadResource[] = { 0x24, 1, 0xa0, 0 };
static unsigned char AppLoadClose[] = { 0x19, 1, 0, 1, 0};

LoadPRC(int sd, char *fname)
{
  FILE *fd;
  unsigned int flen;
  prc_t *fh;
  sect_t *sh;

  int i;
  unsigned char *buf;

  unsigned char appid;
  unsigned char apprec;

  fd = fopen(fname,"r");

  if (!fd) return -1;

  fseek(fd, 0, SEEK_END);
  flen = ftell(fd);
  fseek(fd, 0, SEEK_SET);

  fh = malloc(sizeof(prc_t));
  fread(fh,sizeof(prc_t),1,fd);

  printf("File %s, length %d.  Application %s, %d sections\n",
	 fname,
	 flen,
	 fh->AppName,
	 ntohs(fh->NumSections));

  buf = malloc(flen + 64);

  /* We're sending an app now! */
  pi_write(sd, AppLoad, sizeof(AppLoad));
  pi_read (sd, buf, 24);

  appid = buf[4];

  /* Here's the name of the app, do you have one called this? */
  memcpy(buf,AppLoadName, sizeof(AppLoadName));
  buf[2] = appid;
  buf[3] = strlen(fh->AppName) + 3;

  strcpy(&buf[6], fh->AppName);

  pi_write(sd, buf, strlen(fh->AppName) + 7);
  pi_read(sd, buf, 24);

  /* Here's it's resource name and stuff */
  memcpy(buf, AppLoadHeader, sizeof(AppLoadHeader));
  buf[2] = appid;
  buf[3] = strlen(fh->AppName) + 0xf;
  memcpy(&buf[4], &fh->ApplString[4], 4);
  memcpy(&buf[8], fh->ApplString, 4);
  buf[0x0c] = buf[0x0d] = buf[0x0e] = buf[0x10] = 0;  /* hack! */
  buf[0x0f] = buf[0x11] = 1;                          /* hack! */
  strcpy(&buf[0x12], fh->AppName);

  pi_write(sd, buf, strlen(fh->AppName) + 0x13);
  pi_read (sd, buf, 24);

  apprec = buf[6];

  /* OK! good to go.  Send stuff section at a time */

  sh = malloc(sizeof(sect_t) * ntohs(fh->NumSections));
  fread(sh,sizeof(sect_t),ntohs(fh->NumSections),fd);

  /* there's some padding in there */
  fread(buf,2,1,fd);

  for (i=0;i<ntohs(fh->NumSections);i++) {
    int slen;

#ifdef DEBUG
    printf ("Section %c%c%c%c #%d\n",
	    sh[i].SectName[0],
	    sh[i].SectName[1],
	    sh[i].SectName[2],
	    sh[i].SectName[3],
	    ntohs(sh[i].Number));
#endif

    memcpy(buf, AppLoadResource, sizeof(AppLoadResource));

    if ((i+1) != ntohs(fh->NumSections)) {
      slen = ntohs(sh[i+1].Offset) - ntohs(sh[i].Offset);
    }
    else {
      slen = flen - ntohs(sh[i].Offset);
    }

    /* section name and number */
    memcpy(&buf[0x8], sh[i].SectName, 4);
    *(unsigned short *)(&buf[0xc]) = sh[i].Number;

    /* stupid code #0 resource has 4 extra bytes, _ALWAYS_ [0 0 0 1]. Sigh */

    if ((!memcmp(sh[i].SectName,"code",4)) && (ntohs(sh[i].Number) == 1)) {
      slen -= 4;
      fread(&buf[0x10],4,1,fd);
    }

    /* length fields and some junk */
    *(unsigned short *)(&buf[0x4]) = htons(slen + 10);
    *(unsigned short *)(&buf[0xe]) = htons(slen);
    buf[6] = apprec;
    buf[7] = 0;

    /* now read in the section */
    fread(&buf[0x10],slen,1,fd);

    pi_write(sd, buf, slen + 0x10);
    pi_read (sd, buf, 24);
  }

  memcpy(buf, AppLoadClose, sizeof(AppLoadClose));
  buf[2] = appid;
  buf[4] = apprec;
  pi_write(sd, buf, 5);
  pi_read(sd, buf, 24);

  free(buf);
  fclose(fd);

  return 0;
}

