#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include "prc.h"
#include "dlp.h"
#include "pi-socket.h"

int LoadPRC(int sd, char *fname, int cardno)
{
  FILE *fd;
  unsigned int flen;

  unsigned char prcbuf[SIZEOF_PRC];
  prc_t fh;

  unsigned char shbuf[SIZEOF_SECT];
  sect_t* sh;

  int i;
  unsigned char *buf;

  int apprec;

  fd = fopen(fname,"r");
  if (!fd) return -1;

  /* get total length */
  fseek(fd, 0, SEEK_END);
  flen = ftell(fd);
  fseek(fd, 0, SEEK_SET);

  /* read PRC header */
  fread(prcbuf,SIZEOF_PRC,1,fd);
  strncpy(fh.AppName,prcbuf,32);
  fh.Type = get_long(prcbuf+60);
  fh.Creator = get_long(prcbuf+64);
  fh.NumSections = get_short(prcbuf+76);

  printf("File %s, length %d.  Application %s, %d sections\n",
	 fname,
	 flen,
	 fh.AppName,
	 fh.NumSections);

  /* open database */
  dlp_DeleteDB(sd, cardno, fh.AppName);
  printf("%d\n", dlp_CreateDB(sd, fh.Creator, fh.Type, cardno, dlpDBFlagResource, 1, fh.AppName, &apprec));
  
  /* OK! good to go.  Send stuff section at a time */

  sh = malloc(sizeof(sect_t) * (fh.NumSections+1));
  
  /* read sections */
  for (i=0;i<fh.NumSections;i++)
  {
    fread(shbuf,SIZEOF_SECT,1,fd);

    sh[i].Type = get_long(shbuf);
    sh[i].ID = get_short(shbuf+4);
    sh[i].Offset = get_long(shbuf+6);
  }

  /* a dummy end of file section, so we know last section's length */
  sh[fh.NumSections].Type = 0;
  sh[fh.NumSections].ID = 0;
  sh[fh.NumSections].Offset = flen;
  
  buf = malloc(flen + 64);

  for (i=0;i<fh.NumSections;i++)
  {
    int slen = sh[i+1].Offset - sh[i].Offset;

    fseek(fd, sh[i].Offset, SEEK_SET);
    fread(buf, slen, 1, fd);
    
    dlp_WriteResource(sd, apprec, sh[i].Type, sh[i].ID, buf, slen);
  }

  dlp_CloseDB(sd, apprec);

  free(buf);
  free(sh);
  fclose(fd);

  return 0;
}

int RetrievePRC(int sd, char *dname, char *fname, int cardno)
{
  FILE *fd;
  unsigned int flen;

  unsigned char prcbuf[SIZEOF_PRC];
  prc_t fh;

  sect_t* sh;

  int i;
  unsigned char *buf;
  struct DBInfo info;

  int apprec;
  
  int pos;
  
  fd = fopen(fname,"w");
  if (!fd) return -1;
  
  printf("d|%s|f|%s|\n",dname,fname);

  if(dlp_OpenDB(sd, cardno, 0x80, dname, &apprec)<0) {
  	puts("failed to open db");
  	return -1;
  }
  
  if(dlp_FindDBInfo(sd, cardno, 0, dname, 0, 0, &info)<0) {
  	puts("failed to get db info");
  	return -1;
  }

  
  fh.Type = info.type;
  fh.Creator = info.creator;
  
  if(dlp_ReadOpenDBInfo(sd, apprec, &fh.NumSections)<0) {
  	puts("Failed to get record count");
  }
  
  printf("crdate %s\n",ctime(&info.crdate));
  
  fh.prcversion = 1;
  fh.version = info.version;
  
  /* Create PRC header */
  memset(prcbuf, 0, SIZEOF_PRC);
  strncpy(prcbuf,dname,32);
  set_long(prcbuf+60,fh.Type);
  set_long(prcbuf+64,fh.Creator);
  set_short(prcbuf+76,fh.NumSections);
  set_short(prcbuf+32,fh.prcversion);
  set_short(prcbuf+34,fh.version);
  set_long(prcbuf+36,info.crdate+2082844800);
  set_long(prcbuf+40,info.crdate+2082844800);
  
  fwrite(prcbuf, SIZEOF_PRC, 1, fd);
  
  printf("File %s, database %s. %d sections\n",
	 fname,
	 dname,
	 fh.NumSections);

  /* OK! good to go.  Send stuff section at a time */

  sh = malloc(sizeof(sect_t) * (fh.NumSections+1));
  
  /* read sections */
  
  pos = SIZEOF_PRC+(SIZEOF_SECT*fh.NumSections)+2;
  
  for (i=0;i<fh.NumSections;i++)
  {
    int size;
    sh[i].Offset = pos;
    dlp_ReadResourceByIndex(sd, apprec, i, 0, &sh[i].Type, &sh[i].ID, &size);
    
    pos += size;
    
    printf("Section %d, type %s, id %d, size %d, offset %ld\n",i, printlong(sh[i].Type), sh[i].ID, size, sh[i].Offset);
  }
  
  flen = pos;
  
  /* a dummy end of file section */
  sh[fh.NumSections].Type = 0;
  sh[fh.NumSections].ID = 0;
  sh[fh.NumSections].Offset = flen;
  
  buf = malloc(flen + 64);

  for (i=0;i<fh.NumSections;i++)
  {
    int slen;

    fseek(fd, SIZEOF_PRC+(SIZEOF_SECT*i), SEEK_SET);
    set_long(buf, sh[i].Type);
    set_short(buf+4, sh[i].ID);
    set_long(buf+6, sh[i].Offset);
    fwrite(buf, SIZEOF_SECT, 1, fd);
    
    dlp_ReadResourceByIndex(sd, apprec, i, buf, 0, 0, &slen);

    fseek(fd, sh[i].Offset, SEEK_SET);
    fwrite(buf, slen, 1, fd);
    
  }

  dlp_CloseDB(sd, apprec);

  free(buf);
  free(sh);
  fclose(fd);

  return 0;
}
