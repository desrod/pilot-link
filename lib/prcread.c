#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include "prc.h"
#include "dlp.h"
#include "pi-socket.h"

LoadPRC(int sd, char *fname)
{
  FILE *fd;
  unsigned int flen;
  prc_t *fh;
  sect_t *sh;

  int i;
  unsigned char *buf;

  int apprec;

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

  dlp_DeleteDB(sd, 0, fh->AppName);

  dlp_CreateDB(sd, ntohl(fh->Creator), ntohl(fh->Type), 0, dlpDBFlagResource, 1, fh->AppName, &apprec);
  
  /* OK! good to go.  Send stuff section at a time */

  fh->NumSections = ntohs(fh->NumSections);
  
  sh = malloc(sizeof(sect_t) * (fh->NumSections+1));
  fread(sh,sizeof(sect_t),fh->NumSections,fd);
  
  for(i=0;i<fh->NumSections;i++) {
    sh[i].Offset = ntohl(sh[i].Offset);
    sh[i].Type = ntohl(sh[i].Type);
    sh[i].ID = ntohs(sh[i].ID);
  }

  /* a dummy end of file section, so we know last section's length */
  sh[fh->NumSections].Offset = flen;
  sh[fh->NumSections].ID = 0;
  sh[fh->NumSections].Type = 0;
  
  for (i=0;i<fh->NumSections;i++) {
    int slen;

    slen = sh[i+1].Offset - sh[i].Offset;
    fseek(fd, sh[i].Offset, SEEK_SET);
    fread(buf, slen, 1, fd);
    
    dlp_WriteResource(sd, apprec, sh[i].Type, sh[i].ID, buf, slen);

  }

  dlp_CloseDB(sd, apprec);

  free(buf);
  fclose(fd);

  return 0;
}
