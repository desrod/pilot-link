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

#include "pi-header.h"
#include "pi-source.h"
#include "pi-file.h"
#include "pi-userland.h"

/***********************************************************************
 *
 * Function:    iso_time_str
 *
 * Summary:     Build an ISO-compliant date string
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static char *iso_time_str(time_t t)
{
	struct 	tm tm;
	static 	char buf[50];

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
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void dump(void *buf, int n)
{
	int 	ch,
		i,
		j;

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
			ch = ((unsigned char *) buf)[i + j] & 0x7f;
			if (ch < ' ' || ch >= 0x7f)
				putchar('.');
			else
				putchar(ch);
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
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void dump_header(struct pi_file *pf, struct DBInfo *ip)
{
	printf("Name..........: %s\n", ip->name);
	printf("flags.........: 0x%x", ip->flags);
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
	printf("version.......: %d\n", ip->version);
	printf("creation_time.: %s\n", iso_time_str(ip->createDate));
	printf("modified_time.: %s\n", iso_time_str(ip->modifyDate));
	printf("backup_time...: %s\n", iso_time_str(ip->backupDate));
	printf("mod_number....: %ld\n", ip->modnum);
	printf("type..........: %s\n", printlong(ip->type));
	printf("CreatorID.....: %s\n", printlong(ip->creator));
	printf("\n");
}


/***********************************************************************
 *
 * Function:    dump_app_info
 *
 * Summary:     Dump the AppInfo segment of the database(s)
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void dump_app_info(struct pi_file *pf, struct DBInfo *ip)
{
	size_t 	app_info_size;
	void 	*app_info;

	pi_file_get_app_info(pf, &app_info, &app_info_size);

	printf("app_info_size %zu\n", app_info_size);
	dump(app_info, app_info_size);
	printf("\n");
}


/***********************************************************************
 *
 * Function:    dump_sort_info
 *
 * Summary:     Dump the SortInfo block of the database(s)
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void dump_sort_info(struct pi_file *pf, struct DBInfo *ip)
{
	size_t 	sort_info_size;
	void 	*sort_info;


	pi_file_get_sort_info(pf, &sort_info, &sort_info_size);

	printf("sort_info_size %zu\n", sort_info_size);
	dump(sort_info, sort_info_size);
	printf("\n");
}


/***********************************************************************
 *
 * Function:    list_records
 *
 * Summary:     List all records in the database(s)
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void list_records(struct pi_file *pf, struct DBInfo *ip, int filedump, int verbose)
{
	int 	attrs,
		cat,
		entnum,		/* Number of the entry record */
		id_,
		nentries; 	/* Number of entries in the list */
	size_t	size;
	void 	*buf;
	unsigned long type, uid;

	pi_file_get_entries(pf, &nentries);

	if (ip->flags & dlpDBFlagResource) {
		printf("entries\n");
		printf("index\tsize\ttype\tid\n");
		for (entnum = 0; entnum < nentries; entnum++) {
			if (pi_file_read_resource
			    (pf, entnum, &buf, &size, &type, &id_) < 0) {
				printf("error reading %d\n\n", entnum);
				return;
			}
			printf("%d\t%zu\t%s\t%d\n", entnum, size,
			       printlong(type), id_);
			if (verbose) {
				dump(buf, size);
				printf("\n");
				if (filedump) {
					FILE *fp;
					char name[64];

					sprintf(name, "%4s%04x.bin",
						printlong(type), id_);
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
			printf("%d\t%zu\t0x%x\t%d\t0x%lx\n", entnum, size,
			       attrs, cat, uid);
			if (verbose) {
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
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void dump_record(struct pi_file *pf, struct DBInfo *ip, char *rkey, int filedump)
{
	int 	attrs,
		cat,
		id_,
		record;
	size_t	size;
	void 	*buf;
	unsigned long type;
	unsigned long uid;


	if (ip->flags & dlpDBFlagResource) {
		printf("entries\n");
		printf("index\tsize\ttype\tid\n");
		if (sscanf(rkey, "%d", &record) == 1) {
			if (pi_file_read_resource
			    (pf, record, &buf, &size, &type, &id_) < 0) {
				printf("error reading resource #%d\n\n",
				       record);
				return;
			}
		} else {
			type = makelong(rkey);
			id_ = 0;
			sscanf(&rkey[4], "%d", &id_);
			if (pi_file_read_resource_by_type_id
			    (pf, type, id_, &buf, &size, &record) < 0) {
				printf
				    ("error reading resource %s' #%d (0x%x)\n\n",
				     printlong(type), id_, id_);
				return;
			}
		}

		printf("%d\t%zu\t%s\t%d\n", record, size, printlong(type),
		       id_);
		dump(buf, size);
		if (filedump) {
			FILE *fp;
			char name[64];

			sprintf(name, "%4s%04x.bin", printlong(type), id_);
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
		printf("%d\t%zu\t0x%x\t%d\t0x%lx\n", record, size, attrs,
		       cat, uid);
		dump(buf, size);
	}

	printf("\n");
}




int main(int argc, const char **argv)
{
	int 	c,		/* switch */
		hflag 		= 0,
		aflag 		= 0,
		sflag 		= 0,
		dflag 		= 0,
		lflag 		= 0,
		rflag 		= 0,
		filedump 	= 0;

	char *rkey           = NULL;

	const char **rargv = NULL;

	struct 	pi_file *pf;
	struct 	DBInfo info;

	poptContext po;

	struct poptOption options[] = {
		USERLAND_RESERVED_OPTIONS
        	{"header",	'H', POPT_ARG_NONE, &hflag, 0, "Dump the header of the database(s)"},
	        {"appinfo",	'a', POPT_ARG_NONE, &aflag, 0, "Dump app_info segment of the database(s)"},
        	{"sortinfo",	's', POPT_ARG_NONE, &sflag, 0, "Dump sort_info block of database(s)"},
	        {"list",	'l', POPT_ARG_NONE, &lflag, 0, "List all records in the database(s)"},
        	{"record",	'r', POPT_ARG_STRING, &rkey,  0, "Dump a record by index ('code0') or uid ('1234')"},
	        {"to-file",	'f', POPT_ARG_NONE, &filedump,  0, "Same as above but also dump records to files"},
        	{"dump",	'd', POPT_ARG_NONE, &dflag, 0, "Dump all data and all records, very verbose"},
        	  POPT_TABLEEND
	};

	po = poptGetContext("pilot-file", argc, argv, options, 0);
	poptSetOtherOptionHelp(po,"<filename> ...\n\n"
	"   Dump application and header information from your local PRC/PDB files\n\n"
	"   Example arguments:\n"
	"      -l Foo.prc\n"
	"      -H -a Bar.pdb\n\n");

	plu_popt_alias(po,"dump-rec",0,"--bad-option --to-file --record");
	plu_popt_alias(po,NULL,'R',"--bad-option --to-file --record");
	plu_popt_alias(po,"dump-res",0,"--bad-option --to-file --dump");
	plu_popt_alias(po,NULL,'D',"--bad-option --to-file --dump");

	if (argc < 2) {
		poptPrintUsage(po,stderr,0);
		return 1;
	}

	while ((c = poptGetNextOpt(po)) >= 0) {
		fprintf(stderr,"   ERROR: Unhandled option %d.\n",c);
		return 1;
	}

	if (c < -1) {
		plu_badoption(po,c);
	}

	if (rkey) {
		rflag = 1;
	}

	rargv = poptGetArgs(po);
	if (!rargv || !rargv[0]) {
		fprintf(stderr,"   ERROR: Must provide one or more filenames.\n");
		return 1;
	}

	while (*rargv) {
		const char *name = *rargv++;

		if ((pf = pi_file_open(name)) == NULL) {
			fprintf(stderr, "   ERROR: Can't open '%s' Does '%s' exist?\n\n",
				name, name);
			continue;
		}

		pi_file_get_info(pf, &info);

		if (hflag || dflag)
			dump_header(pf, &info);

		if (aflag || dflag)
			dump_app_info(pf, &info);

		if (sflag || dflag)
			dump_sort_info(pf, &info);

		if (lflag || dflag)
			list_records(pf, &info, filedump, dflag);

		if (rflag)
			dump_record(pf, &info, rkey, filedump);
	}

	return 0;
}
