/*
 * Palm .prc pack/unpack utility
 * Based on pilot-file
 * Additions by Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-file.h"
#include "pi-header.h"

#ifdef sun
extern char *optarg;
extern int optind;
#endif

void dump_header(struct pi_file *pf, struct DBInfo *ip);
void dump_app_info(struct pi_file *pf, struct DBInfo *ip);
void dump_sort_info(struct pi_file *pf, struct DBInfo *ip);
void list_records(struct pi_file *pf, struct DBInfo *ip);
void dump_record(struct pi_file *pf, struct DBInfo *ip, int record);

char *iso_time_str(time_t t);
void dump(void *buf, int size);

char *progname;

void usage(void)
{
   PalmHeader();

   fprintf(stderr, "   Usage: %s [-s] -p|-u dir file\n", progname);
   fprintf(stderr, "  
   -p      pack the contents of a directory into the named file
   -u      unpack the contents of the named file into a directory
   -s      don't obey/generate sort list

   Contents of directory may include files like these:
     tSTRx12CD[.bin] (a resource with hex index)
     tSTR10000[.bin] (a reousrce with decimal index)
     sort            (a file containing the names of other files)\n");
   exit(1);
}

int hflag, aflag, sflag, vflag, lflag, rflag, rnum;

int main(int argc, char **argv)
{
   struct pi_file *pf;
   int c;
   char *name;
   struct DBInfo info;

   progname = argv[0];

   while ((c = getopt(argc, argv, "haslr:v")) != EOF) {
      switch (c) {
      case 'h':
	 hflag = 1;
	 break;
      case 'a':
	 aflag = 1;
	 break;
      case 's':
	 sflag = 1;
	 break;
      case 'v':
	 vflag = 1;
	 break;
      case 'l':
	 lflag = 1;
	 break;
      case 'r':
	 rflag = 1;
	 rnum = atoi(optarg);
	 break;
      default:
	 usage();
      }
   }

   if (optind >= argc)
      usage();

   name = argv[optind++];

   if (optind != argc)
      usage();

   if ((pf = pi_file_open(name)) == NULL) {
      fprintf(stderr, "can't open %s\n", name);
      exit(1);
   }

   if (pi_file_get_info(pf, &info) < 0) {
      fprintf(stderr, "can't get info\n\n");
      exit(1);
   }

   if (hflag || vflag)
      dump_header(pf, &info);

   if (aflag || vflag)
      dump_app_info(pf, &info);

   if (sflag || vflag)
      dump_sort_info(pf, &info);

   if (lflag || vflag)
      list_records(pf, &info);

   if (rflag)
      dump_record(pf, &info, rnum);

   return (0);
}

char *iso_time_str(time_t t)
{
   struct tm tm;
   static char buf[50];

   tm = *localtime(&t);
   sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d",
	   tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
	   tm.tm_hour, tm.tm_min, tm.tm_sec);
   return (buf);
}

void dump(void *buf, int n)
{
   int i, j, c;

   for (i = 0; i < n; i += 16) {
      printf("%04x: ", i);
      for (j = 0; j < 16; j++) {
	 if (i + j < n)
	    printf("%02x ", ((unsigned char *) buf)[i + j]);
	 else
	    printf("   ");
      }
      printf("  ");
      for (j = 0; j < 16 && i + j < n; j++) {
	 c = ((unsigned char *) buf)[i + j] & 0x7f;
	 if (c < ' ' || c >= 0x7f)
	    putchar('.');
	 else
	    putchar(c);
      }
      printf("\n");
   }
}

void dump_header(struct pi_file *pf, struct DBInfo *ip)
{
   printf("name: \"%s\"\n", ip->name);
   printf("flags: 0x%x", ip->flags);
   if (ip->flags & dlpDBFlagResource)
      printf(" RESOURCE");
   if (ip->flags & dlpDBFlagReadOnly)
      printf(" READ_ONLY");
   if (ip->flags & dlpDBFlagAppInfoDirty)
      printf(" APP-INFO-DIRTY");
   if (ip->flags & dlpDBFlagBackup)
      printf(" BACKUP");
   if (ip->flags & dlpDBFlagOpen)
      printf(" OPEN");
   printf("\n");
   printf("version: %d\n", ip->version);

   printf("creation_time: %s\n", iso_time_str(ip->crdate));
   printf("modified_time: %s\n", iso_time_str(ip->moddate));
   printf("backup_time  : %s\n", iso_time_str(ip->backupdate));

   printf("modification_number: %ld\n", ip->modnum);
   printf("type: '%s', ", printlong(ip->type));
   printf("creator: '%s'\n", printlong(ip->creator));
   printf("\n");
}

void dump_app_info(struct pi_file *pf, struct DBInfo *ip)
{
   void *app_info;
   int app_info_size;

   if (pi_file_get_app_info(pf, &app_info, &app_info_size) < 0) {
      printf("can't get app_info\n\n");
      return;
   }

   printf("app_info_size %d\n", app_info_size);
   dump(app_info, app_info_size);
   printf("\n");
}

void dump_sort_info(struct pi_file *pf, struct DBInfo *ip)
{
   void *sort_info;
   int sort_info_size;

   if (pi_file_get_sort_info(pf, &sort_info, &sort_info_size) < 0) {
      printf("can't get sort_info\n\n");
      return;
   }

   printf("sort_info_size %d\n", sort_info_size);
   dump(sort_info, sort_info_size);
   printf("\n");
}

void list_records(struct pi_file *pf, struct DBInfo *ip)
{
   int entnum;
   int size;
   unsigned long type, uid;
   int id;
   void *buf;
   int nentries;
   int attrs, cat;


   pi_file_get_entries(pf, &nentries);

   if (ip->flags & dlpDBFlagResource) {
      printf("entries\n");
      printf("index\tsize\ttype\tid\n");
      for (entnum = 0; entnum < nentries; entnum++) {
	 if (pi_file_read_resource(pf, entnum, &buf, &size, &type, &id) <
	     0) {
	    printf("error reading %d\n\n", entnum);
	    return;
	 }
	 printf("%d\t%d\t%s\t%d\n", entnum, size, printlong(type), id);
	 if (vflag) {
	    dump(buf, size);
	    printf("\n");
	 }
      }
   } else {
      printf("entries\n");
      printf("index\tsize\tattrs\tcat\tuid\n");
      for (entnum = 0; entnum < nentries; entnum++) {
	 if (pi_file_read_record(pf, entnum, &buf, &size,
				 &attrs, &cat, &uid) < 0) {
	    printf("error reading %d\n\n", entnum);
	    return;
	 }
	 printf("%d\t%d\t0x%x\t%d\t0x%lx\n", entnum, size, attrs, cat,
		uid);
	 if (vflag) {
	    dump(buf, size);
	    printf("\n");
	 }
      }
   }

   printf("\n");
}

void dump_record(struct pi_file *pf, struct DBInfo *ip, int record)
{
   int size;
   unsigned long type, uid;
   int id;
   void *buf;
   int attrs, cat;

   if (ip->flags & dlpDBFlagResource) {
      printf("entries\n");
      printf("index\tsize\ttype\tid\n");
      if (pi_file_read_resource(pf, record, &buf, &size, &type, &id) < 0) {
	 printf("error reading resource #%d\n\n", record);
	 return;
      }
      printf("%d\t%d\t%s\t%d\n", record, size, printlong(type), id);
      dump(buf, size);
   } else {
      printf("entries\n");
      printf("index\tsize\tattrs\tcat\tuid\n");
      if (pi_file_read_record(pf, record, &buf, &size,
			      &attrs, &cat, &uid) < 0) {
	 printf("error reading record #%d\n\n", record);
	 return;
      }
      printf("%d\t%d\t0x%x\t%d\t0x%lx\n", record, size, attrs, cat, uid);
      dump(buf, size);
   }

   printf("\n");
}
