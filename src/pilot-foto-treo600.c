/*
 * pilot-treofoto.c: Grab jpeg photos off a Treo 600 camera phone.
 *
 * (c) 2004, Matthew Allum <mallum@o-hand.com>
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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include "pi-file.h"
#include "pi-header.h"
#include "pi-dlp.h"
#include "pi-socket.h"
#include "pi-userland.h"

#define MAIN_PDB_NAME "ImageLib_mainDB.pdb"
#define IMG_PDB_NAME  "ImageLib_imageDB.pdb"

/* Record formats via script from
 * http://www.xent.com/~bsittler/treo600/dumphotos
 */

typedef struct MainDBImgRecord {
	unsigned char name[32];	/* padded NULL */
	unsigned char unknown_1[8];
	pi_uid_t first_image_uid;
	unsigned int unknown_2;
	pi_uid_t first_thumb_uid;
	unsigned short image_n_blocks;
	unsigned short thumb_n_blocks;
	unsigned int timestamp;
	unsigned short image_n_bytes;
	unsigned short thumb_n_bytes;
	unsigned char padding[56];
} MainDBImgRecord;


int extract_image(struct pi_file *pi_fp, MainDBImgRecord * img_rec)
{

	int i, attr, cat, nentries, fd;
	size_t size;
	void *Pbuf;
	pi_uid_t uid, req_uid;
	char imgfilename[1024];

	snprintf(imgfilename, 1024, "%s.jpg", img_rec->name);

	req_uid = img_rec->first_image_uid;

	if ((fd = open(imgfilename, O_RDWR | O_CREAT | O_TRUNC,
		       S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1) {
		fprintf(stderr, "   WARNING: failed to open '%s' for writing\n",
			imgfilename);
		return 0;
	}

	pi_file_get_entries(pi_fp, &nentries);

	for (i = 0; i < nentries; i++) {
		if (pi_file_read_record
		    (pi_fp, i, &Pbuf, &size, &attr, &cat, &uid) < 0) {
			fprintf(stderr,"   WARNING: Error reading image record %d\n\n", i);
			return -1;
		}

		if (req_uid && uid == req_uid) {
			memcpy(&req_uid, Pbuf, 4);	/* get next req_uid for image 'block' */
			write(fd, Pbuf + 4, size - 4);	/* The rest is just jpeg data */
		}
	}

	if (!plu_quiet) {
		printf("   Wrote '%s' \n", imgfilename);
	}

	close(fd);

	return 1;
}


int fetch_remote_file(int sd, char *fullname)
{
	struct DBInfo info;
	char *basename = NULL;
	struct pi_file *pi_fp;

	basename = strdup(fullname);

	basename[strlen(fullname) - 4] = '\0';

	if (dlp_FindDBInfo(sd, 0, 0, basename, 0, 0, &info) < 0) {
		free(basename);
		return 0;
	}

	/* info.flags &= 0x2fd; needed ? */

	pi_fp = pi_file_create(fullname, &info);

	if (pi_file_retrieve(pi_fp, sd, 0, NULL) < 0) {
		free(basename);
		return 0;
	}

	pi_file_close(pi_fp);

	free(basename);
	return 1;
}


int main(int argc, const char *argv[])
{
	struct pi_file *pi_fp = NULL, *img_fp = NULL;
	int i, attr, cat, nentries;
	size_t size;
	pi_uid_t uid;
	MainDBImgRecord *img_rec;
	int sd = -1;
	int c;
	int ret = 1;
	enum { mode_none, mode_write = 257 } run_mode = mode_none;

	struct poptOption options[] = {
		USERLAND_RESERVED_OPTIONS
		{"write",'w', POPT_ARG_NONE,NULL,mode_write,"Write data"},
		POPT_TABLEEND
		} ;

	poptContext po = poptGetContext("pilot-treofoto",argc,argv,options,0);
	poptSetOtherOptionHelp(po,"\n\n"
		"   Copies Treo foto databases to current directory and\n"
		"   extracts .jpg files from them.");

	if ((argc < 2)) {
		poptPrintUsage(po,stderr,0);
		return 1;
	}

	while ((c = poptGetNextOpt(po)) >= 0) {
		switch(c) {
		case mode_write :
			if (run_mode == mode_none) {
				run_mode = c;
			} else {
				if (c != run_mode) {
					fprintf(stderr,"   ERROR: Specify exactly one of -w.\n");
					return 1;
				}
			}
			break;
		default:
			fprintf(stderr,"   ERROR: Unhandled option %d.\n",c);
			return 1;
		}
	}

	if (c < -1) {
		plu_badoption(po,c);
	}

	if (mode_none == run_mode) {
		fprintf(stderr,"   ERROR: Specify --write (-w) to output data.\n");
		return 1;
	}


	if ((sd = plu_connect()) < 0) {
		return 1;
	}

	if (!fetch_remote_file(sd, MAIN_PDB_NAME)) {
		fprintf(stderr, "   ERROR: unable to fetch '%s'\n",
			MAIN_PDB_NAME);
		goto cleanup;
	}


	if (!fetch_remote_file(sd, IMG_PDB_NAME)) {
		fprintf(stderr, "   ERROR: unable to fetch '%s'\n",
			IMG_PDB_NAME);
		goto cleanup;
	}


	if ((pi_fp = pi_file_open(MAIN_PDB_NAME)) == NULL) {
		fprintf(stderr,"   ERROR: could not open local '%s'\n",
		       MAIN_PDB_NAME);
		goto cleanup;
	}

	if ((img_fp = pi_file_open(IMG_PDB_NAME)) == NULL) {
		fprintf(stderr,"   ERROR: could not open '%s'\n", IMG_PDB_NAME);
		goto cleanup;
	}

	pi_file_get_entries(pi_fp, &nentries);

	for (i = 0; i < nentries; i++) {
		if (pi_file_read_record
		    (pi_fp, i, (void **) &img_rec, &size, &attr, &cat,
		     &uid) < 0) {
			fprintf(stderr,"   WARNING: Could not read record: %d\n\n", i);
			continue;
		}

		extract_image(img_fp, img_rec);
	}

	ret=0;

cleanup:
	if (img_fp) {
		pi_file_close(img_fp);
		unlink(IMG_PDB_NAME);
	}
	if (pi_fp) {
		pi_file_close(pi_fp);
		unlink(MAIN_PDB_NAME);
	}
	if (sd >= 0) {
		dlp_EndOfSync(sd, 0);
		pi_close(sd);
	}

	return ret;

}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
