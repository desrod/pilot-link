#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pi-file.h"
#include "pi-versamail.h"

int print_mail_record(void *record, int size, int attr, int idx) {
  struct VersaMail mail;
  char datestr[255];

  unpack_VersaMail(&mail, record, size);

  printf("----\n");
  printf("version: %d\n", mail.imapuid);
  strftime(datestr, 254, "%c", &(mail.date));
  //printf("date: %s\n", datestr);
  //printf("category: %d\n", mail.category);
  //printf("account: %d\n", mail.accountNo);
  printf("rfu1: %d\n", mail.rfu1);
  printf("rfu2: %d\n", mail.rfu2);
  printf("reserved1: %d\n", mail.reserved1);
  printf("reserved2: %d\n", mail.reserved2);
  printf("download: %d\n", mail.download);
  printf("mark: %d\n", mail.mark);
  printf("read: %d\n", mail.read);
  printf("body: %s\n", mail.body);
  //printf("to: %s\n", mail.to);
  //printf("from: %s\n", mail.from);
  //printf("cc: %s\n", mail.cc);
  //printf("bcc: %s\n", mail.bcc);
  printf("subject: %s\n", mail.subject);
  //printf("dateString: %s\n", mail.dateString);
  //printf("replyTo: %s\n", mail.replyTo);
  
  free_VersaMail(&mail);
}

int validate_packer(void *record, int size, int attr, int idx) {
  struct VersaMail mail;
  char datestr[255];
  char *buffer;
  int len;
  int i;

  unpack_VersaMail(&mail, record, size);
  len = pack_VersaMail(&mail, NULL, 0);


  if (size-len == 0) {
    buffer = malloc(len);
    pack_VersaMail(&mail, buffer, len);
    //printf("-------------\n");
    printf("subject: %s\n", mail.subject);
    //printf("body: %s\n", mail.body);
    //printf("dateString: %s\n", mail.dateString);
    //printf("replyTo: %s\n", mail.replyTo);
    for (i=0;i<len;i++) {
      if (!((char *)record)[i] == buffer[i]) {
	printf("WRONG Byte %3d: 0x%10x (%c) vs. 0x%10x (%c)\n", 
	       i+1,
	       ((char *)record)[i], 
	       ((char *)record)[i], 
	       buffer[i],
	       buffer[i]);
      }
    }
    free(buffer);
    free_VersaMail(&mail);
  } else {
    printf("on-disk size is %d, pack asked for %d to repack, wrong by %d.\n", size, len, size-len);
  }

}

int main(int argc, char *argv[]) {
   struct pi_file *pi_fp;
   int db;
   char *DBname;
   int r;
   int idx;
   int size;
   int attr;
   int cat;
   pi_uid_t uid;
   void *record;
  
   DBname = "MultiMail Messages.pdb";

   pi_fp = pi_file_open(DBname);
   if (pi_fp==0) {
      printf("Unable to open '%s'!\n", DBname);
      return -1;
   }


   for(idx=0;;idx++) {
      r = pi_file_read_record(pi_fp, idx, &record, &size, &attr, &cat, &uid);
      if (r<0) break;
      //print_mail_record(record, size, attr, idx);
      validate_packer(record, size, attr, idx);
   }

   pi_file_close(pi_fp);

   return 0;
}
