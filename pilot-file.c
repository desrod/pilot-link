/*
 * pilot-file.c - Palm File dump utility
 *
 * Pace Willisson <pace@blitz.com> December 1996
 * Additions by Kenneth Albanowski
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
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
void dump_record(struct pi_file *pf, struct DBInfo *ip);

char *iso_time_str(time_t t);
void dump(void *buf, int size);

char *progname;

/***********************************************************************
 *
 * Function:    usage
 *
 * Summary:     Print out the program usage and arguments
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void usage(void)
{
	PalmHeader(progname);

	fprintf(stderr, "   Usage: %s [options] file\n", progname);
	fprintf(stderr, "   -h       = dump the header of the database(s)\n");
	fprintf(stderr, "   -a       = dump app_info segment of the database(s)\n");
	fprintf(stderr, "   -s       = dump sort_info block of database(s)\n");
	fprintf(stderr, "   -l       = list all records in the database(s)\n");
	fprintf(stderr, "   -r #     = dump a record (# is an index, or eg 'code0' or a uid 1234')\n");
	fprintf(stderr, "   -v       = dump all data and all records, very verbose\n\n");
	fprintf(stderr, "   -R, -V as -r, -v, but also dump resources to files\n\n");
	exit(1);
}

int hflag = 0, aflag = 0, sflag = 0, vflag = 0, lflag = 0, rflag =
    0, filedump = 0;
char *rkey;


int main(int argc, char **argv)
{
	struct pi_file *pf;
	struct DBInfo info;
	int c;
	char *name;

	progname = argv[0];

	while ((c = getopt(argc, argv, "haslr:R:vV")) != EOF) {
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
		case 'V':
			filedump = 1;
			/* FALLTHROUGH */
		case 'v':
			vflag = 1;
			break;
		case 'l':
			lflag = 1;
			break;
		case 'R':
			filedump = 1;
			/* FALLTHROUGH */
		case 'r':
			rflag = 1;
			rkey = optarg;
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
		dump_record(pf, &info);

	return (0);
}

/***********************************************************************
 *
 * Function:    iso_time_str
 *
 * Summary:     Build an ISO-compliant date string
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
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

/***********************************************************************
 *
 * Function:    dump
 *
 * Summary:     Dump data as requested by other functions
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void dump(void *buf, int n)
{
	int c;
	int i;
	int j;

	for (i = 0; i < n; i += 16) {
		printf("%04x: ", i);
		for (j = 0; j < 16; j++) {
			if (i + j < n)
				printf("%02x ",
				       ((unsigned char *) buf)[i + j]);
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

/***********************************************************************
 *
 * Function:    dump_header
 *
 * Summary:     Dump the header section of the database
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void dump_header(struct pi_file *pf, struct DBInfo *ip)
{
	printf("name: \"%s\"\n", ip->name);
	printf("flags: 0x%x", ip->flags);
	if (ip->flags & dlpDBFlagNewer)
		printf(" NEWER");
	if (ip->flags & dlpDBFlagReset)
		printf(" RESET");
	if (ip->flags & dlpDBFlagResource)
		printf(" RESOURCE");
	if (ip->flags & dlpDBFlagReadOnly)
		printf(" READ_ONLY");
	if (ip->flags & dlpDBFlagAppInfoDirty)
		printf(" APP-INFO-DIRTY");
	if (ip->flags & dlpDBFlagBackup)
		printf(" BACKUP");
	if (ip->flags & dlpDBFlagCopyPrevention)
		printf(" COPY-PREVENTION");
	if (ip->flags & dlpDBFlagStream)
		printf(" STREAM");
	if (ip->flags & dlpDBFlagOpen)
		printf(" OPEN");
	printf("\n");
	printf("version: %d\n", ip->version);
	printf("creation_time: %s\n", iso_time_str(ip->createDate));
	printf("modified_time: %s\n", iso_time_str(ip->modifyDate));
	printf("backup_time: %s\n", iso_time_str(ip->backupDate));
	printf("modification_number: %ld\n", ip->modnum);
	printf("type: '%s', ", printlong(ip->type));
	printf("creator: '%s'\n", printlong(ip->creator));
	printf("\n");
}

/***********************************************************************
 *
 * Function:    dump_app_info
 *
 * Summary:     Dump the AppInfo segment of the database(s)
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
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

/***********************************************************************
 *
 * Function:    dump_sort_info
 *
 * Summary:     Dump the SortInfo block of the database(s)
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
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

/***********************************************************************
 *
 * Function:    list_records
 *
 * Summary:     List all records in the database(s)
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void list_records(struct pi_file *pf, struct DBInfo *ip)
{
	int attrs;
	int cat;
	int entnum;
	int id;
	int nentries;
	int size;
	unsigned long type, uid;
	void *buf;

	pi_file_get_entries(pf, &nentries);

	if (ip->flags & dlpDBFlagResource) {
		printf("entries\n");
		printf("index\tsize\ttype\tid\n");
		for (entnum = 0; entnum < nentries; entnum++) {
			if (pi_file_read_resource
			    (pf, entnum, &buf, &size, &type, &id) < 0) {
				printf("error reading %d\n\n", entnum);
				return;
			}
			printf("%d\t%d\t%s\t%d\n", entnum, size,
			       printlong(type), id);
			if (vflag) {
				dump(buf, size);
				printf("\n");
				if (filedump) {
					FILE *fp;
					char name[64];

					sprintf(name, "%4s%04x.bin",
						printlong(type), id);
					fp = fopen(name, "w");
					fwrite(buf, size, 1, fp);
					fclose(fp);
					printf("(written to %s)\n", name);
				}
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
			printf("%d\t%d\t0x%x\t%d\t0x%lx\n", entnum, size,
			       attrs, cat, uid);
			if (vflag) {
				dump(buf, size);
				printf("\n");
			}
		}
	}

	printf("\n");
}

/***********************************************************************
 *
 * Function:    dump_record
 *
 * Summary:     Dump a record
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void dump_record(struct pi_file *pf, struct DBInfo *ip)
{
	int attrs;
	int cat;
	int id;
	int record;
	int size;
	unsigned long type;
	unsigned long uid;
	void *buf;

	if (ip->flags & dlpDBFlagResource) {
		printf("entries\n");
		printf("index\tsize\ttype\tid\n");
		if (sscanf(rkey, "%d", &record) == 1) {
			if (pi_file_read_resource
			    (pf, record, &buf, &size, &type, &id) < 0) {
				printf("error reading resource #%d\n\n",
				       record);
				return;
			}
		} else {
			type = makelong(rkey);
			id = 0;
			sscanf(&rkey[4], "%x", &id);
			if (pi_file_read_resource_by_type_id
			    (pf, type, id, &buf, &size, &record) < 0) {
				printf
				    ("error reading resource %s' #%d (0x%x)\n\n",
				     printlong(type), id, id);
				return;
			}
		}
		printf("%d\t%d\t%s\t%d\n", record, size, printlong(type),
		       id);
		dump(buf, size);
		if (filedump) {
			FILE *fp;
			char name[64];

			sprintf(name, "%4s%04x.bin", printlong(type), id);
			fp = fopen(name, "w");
			fwrite(buf, size, 1, fp);
			fclose(fp);
			printf("(written to %s)\n", name);
		}
	} else {
		printf("entries\n");
		printf("index\tsize\tattrs\tcat\tuid\n");
		if (sscanf(rkey, "0x%lx", &uid) == 1) {
			if (pi_file_read_record_by_id(pf, uid, &buf, &size,
						      &record, &attrs,
						      &cat) < 0) {
				printf
				    ("error reading record uid 0x%lx\n\n",
				     uid);
				return;
			}
		} else {
			record = 0;
			sscanf(rkey, "%d", &record);
			if (pi_file_read_record(pf, record, &buf, &size,
						&attrs, &cat, &uid) < 0) {
				printf("error reading record #%d\n\n",
				       record);
				return;
			}
		}
		printf("%d\t%d\t0x%x\t%d\t0x%lx\n", record, size, attrs,
		       cat, uid);
		dump(buf, size);
	}

	printf("\n");
}
