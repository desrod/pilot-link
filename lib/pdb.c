#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include "pdb.h"
#include "dlp.h"
#include "pi-socket.h"

LoadPDB(int sd, char *fname, int cardno)
{
  FILE *fd;
  unsigned int flen;

  unsigned char prcbuf[SIZEOF_PDB];
  pdb_t fh;

  unsigned char shbuf[SIZEOF_PDB_SECT];
  pdb_sect_t* sh;

  int i;
  unsigned char *buf;
  
  int AppInfoLen;
  char * AppInfo;

  int apprec;

  fd = fopen(fname,"r");
  if (!fd) return -1;

  // get total length
  fseek(fd, 0, SEEK_END);
  flen = ftell(fd);
  fseek(fd, 0, SEEK_SET);

  // read PDB header
  fread(prcbuf,SIZEOF_PDB,1,fd);
  strncpy(fh.AppName,prcbuf,32);
  fh.Type = get_long(prcbuf+60);
  fh.Creator = get_long(prcbuf+64);
  fh.NumSections = get_short(prcbuf+76);

  printf("File %s, length %d.  Application %s, %d sections\n",
	 fname,
	 flen,
	 fh.AppName,
	 fh.NumSections);

  // open database
  dlp_DeleteDB(sd, cardno, fh.AppName);
  printf("%d\n", dlp_CreateDB(sd, fh.Creator, fh.Type, cardno, 0, 1, fh.AppName, &apprec));
  
  /* OK! good to go.  Send stuff section at a time */

  sh = malloc(sizeof(pdb_sect_t) * (fh.NumSections+1));
  
  // read sections
  for (i=0;i<fh.NumSections;i++)
  {
    fread(shbuf,SIZEOF_PDB_SECT,1,fd);

    sh[i].Offset = get_long(shbuf);
    sh[i].ID = get_long(shbuf+4);
    sh[i].Attr = sh[i].ID >> 24;
    sh[i].ID &= 0x00ffffff;
    sh[i].Category = sh[i].Attr & 0x0F;
    sh[i].Attr &= 0xF0;
  }

  /* a dummy end of file section, so we know last section's length */
  sh[fh.NumSections].ID = 0;
  sh[fh.NumSections].Offset = flen;
  
  buf = malloc(flen + 64);
  
  /* Calculate size of AppInfo block */
  /* FIXME: verify that this is the proper way to find the AI block */
  
  AppInfoLen = sh[0].Offset-(SIZEOF_PDB+(SIZEOF_PDB_SECT*fh.NumSections)+2);
  if(AppInfoLen>0) {
    AppInfo = malloc(AppInfoLen);
    fseek(fd, (SIZEOF_PDB+(SIZEOF_PDB_SECT*fh.NumSections)+2), SEEK_SET);
    fread(AppInfo, AppInfoLen, 1, fd);

    dlp_WriteAppBlock(sd, apprec, AppInfo, AppInfoLen);
  }

  /* Transmit records */
  for (i=0;i<fh.NumSections;i++)
  {
    int slen = sh[i+1].Offset - sh[i].Offset;

    fseek(fd, sh[i].Offset, SEEK_SET);
    fread(buf, slen, 1, fd);
    
    dlp_WriteRecord(sd, apprec, sh[i].Attr, sh[i].ID, sh[i].Category, buf, slen, 0);
  }

  dlp_CloseDB(sd, apprec);

  free(buf);
  free(sh);
  fclose(fd);

  return 0;
}

int RetrievePDB(int sd, char *dname, char *fname, int cardno)
{
  FILE *fd;
  unsigned int flen;

  unsigned char prcbuf[SIZEOF_PDB];
  pdb_t fh;

  unsigned char shbuf[SIZEOF_PDB_SECT];
  pdb_sect_t* sh;

  int i;
  unsigned char *buf;
  struct DBInfo info;

  int apprec;
  char * AppBlock;
  int AppBlockLen;
  
  int pos;
  
  fd = fopen(fname,"w");
  if (!fd) return -1;
  
  printf("d|%s|f|%s|\n",dname,fname);

  if(dlp_OpenDB(sd, cardno, 0x80, dname, &apprec)<0) {
  	puts("failed to open db");
  	return;
  }
  
  if(dlp_FindDBInfo(sd, cardno, 0, dname, 0, 0, &info)<0) {
  	puts("failed to get db info");
  	return;
  }

  
  fh.Type = info.type;
  fh.Creator = info.creator;
  
  if(dlp_ReadOpenDBInfo(sd, apprec, &fh.NumSections)<0) {
  	puts("Failed to get record count");
  }
  
  printf("crdate %s\n",ctime(&info.crdate));
  
  fh.prcversion = 1;
  fh.version = info.version;
  
  // Create PDB header
  memset(prcbuf, 0, SIZEOF_PDB);
  strncpy(prcbuf,dname,32);
  set_long(prcbuf+60,fh.Type);
  set_long(prcbuf+64,fh.Creator);
  set_short(prcbuf+76,fh.NumSections);
  set_short(prcbuf+32,fh.prcversion);
  set_short(prcbuf+34,fh.version);
  set_long(prcbuf+36,info.crdate+2082844800);
  set_long(prcbuf+40,info.crdate+2082844800);
  
  fwrite(prcbuf, SIZEOF_PDB, 1, fd);
  
  printf("File %s, database %s. %d sections\n",
	 fname,
	 dname,
	 fh.NumSections);

  /* OK! good to go.  Send stuff section at a time */

  sh = malloc(sizeof(pdb_sect_t) * (fh.NumSections+1));
  
  AppBlockLen = dlp_ReadAppBlock(sd, apprec, 0, 0, 0xFFFF);
  if (AppBlockLen<=0)
    AppBlockLen=0;
  else {
    AppBlock = malloc(AppBlockLen);
    dlp_ReadAppBlock(sd, apprec, 0, AppBlock, AppBlockLen);
  }
  
  fseek(fd, SIZEOF_PDB+(SIZEOF_PDB_SECT*fh.NumSections)+2, SEEK_SET);
  if (AppBlockLen)
    fwrite(AppBlock, AppBlockLen, 1, fd);
  
  // read sections
  
  pos = SIZEOF_PDB+(SIZEOF_PDB_SECT*fh.NumSections)+2+AppBlockLen;
  
  for (i=0;i<fh.NumSections;i++)
  {
    int size, Attr, Category;
    
    sh[i].Offset = pos;
    dlp_ReadRecordByIndex(sd, apprec, i, 0, &sh[i].ID, &size, &Attr, &Category);
    sh[i].Attr = Attr;
    sh[i].Category = Category;
    
    pos += size;
    
    printf("Record %d, id %8.8X, category %d, attr %d, size %d, offset %d\n",i, sh[i].ID, sh[i].Category,sh[i].Attr,size,sh[i].Offset);
  }
  
  flen = pos;
  
  /* a dummy end of file section */
  sh[fh.NumSections].ID = 0;
  sh[fh.NumSections].Offset = flen;
  
  buf = malloc(flen + 64);

  for (i=0;i<fh.NumSections;i++)
  {
    int slen;

    fseek(fd, SIZEOF_PDB+(SIZEOF_PDB_SECT*i), SEEK_SET);
    set_long(buf, sh[i].Offset);
    set_long(buf+4, sh[i].ID);
    set_byte(buf+4, sh[i].Category | sh[i].Attr);
    fwrite(buf, SIZEOF_PDB_SECT, 1, fd);
    
    dlp_ReadRecordByIndex(sd, apprec, i, buf, 0, &slen, 0, 0);

    fseek(fd, sh[i].Offset, SEEK_SET);
    fwrite(buf, slen, 1, fd);
    
  }

  dlp_CloseDB(sd, apprec);

  free(buf);
  free(sh);
  fclose(fd);

  return 0;
}
