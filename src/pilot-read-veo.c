/* ex: set tabstop=4 expandtab: */
/*
 * read-veo.c
 *
 * Copyright (c) 2003, Angus Ainslie
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "getopt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pi-source.h"
#include "pi-veo.h"
#include "pi-file.h"
#include "pi-header.h"

#define pi_mktag(c1,c2,c3,c4) (((c1)<<24)|((c2)<<16)|((c3)<<8)|(c4))

/* Declare prototypes */
static void display_help(char *progname);
void print_splash(char *progname);
int pilot_connect(char *port);
int Decode(unsigned char *inP, unsigned char *outP, short w);
int Gen24bitRow(int r, struct Veo *v, unsigned char *row);
void WritePicture(int sd, int db, char *name, char *progname);
int protect_files(char *name, char *extension);

struct option options[] = {
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
	{"port",        required_argument, NULL, 'p'},
	{"list",        no_argument,       NULL, 'l'},
	{"type",        required_argument, NULL, 't'},
	{NULL,          0,                 NULL, 0}
};

static const char *optstring = "hvp:lt:";


/***********************************************************************
 *
 * Function:    display_help
 *
 * Summary:     Print out the --help options and arguments
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void display_help(char *progname)
{
	printf("   Syncronize your Veo Traveler databases with your desktop machine\n\n");
	printf("   Usage: %s -p /dev/pilot [options]\n\n", progname);
	printf("   Options:\n");
	printf("     -p, --port <port>       Use device file <port> to communicate with Palm\n");
	printf("     -h, --help              Display help information for %s\n", progname);
	printf("     -v, --version           Display %s version information\n\n", progname);
	printf("     -l                      List Photos on device\n");
	printf("   Examples: %s -p /dev/pilot -l\n\n", progname);

	exit(0);
}


/***********************************************************************
 *
 * Function:    fmt_date
 *
 * Summary:     Format the output date on the images
 *
 * Parameters:  
 *
 * Returns:     
 *
 ***********************************************************************/
static const char *fmt_date(struct Veo *v)
{
        static char buf[24];

        sprintf(buf, "%d-%02d-%02d", v->year, v->month, v->day);
        return buf;

}


/***********************************************************************
 *
 * Function:	protect_files
 *
 * Summary:     FIXME
 *
 * Parameters:  
 *
 * Returns:     
 *
 ***********************************************************************/
int protect_files(char *name, char *extension)
{
        char 	*save_name, 
		c = 1;

        save_name = strdup(name);

        if (NULL == save_name) {
                printf("Failed to generate filename %s%s\n", name,
                       extension);
                return (0);
        }

        sprintf(name, "%s%s", save_name, extension);

        while (access(name, F_OK) == 0) {
                sprintf(name, "%s_%02d%s", save_name, c, extension);

                c++;

                if (c == 'z' + 1)
                        c = 'A';

                if (c == 'Z' + 1) {
                        printf("Failed to generate filename %s\n", name);
                        return (0);
                }
        }

        free(save_name);

        return (1);
}


/***********************************************************************
 *
 * Function:	Decode
 *
 * Summary:	FIXME
 *
 * Parameters:	
 *
 * Returns:	
 *
 ***********************************************************************/
int Decode(unsigned char *inP, unsigned char *outP, short w)
{
	short shifter = 7, index = 0;
	unsigned short tmp0, tmp3, tmp4, i, j; /* d3 */
	unsigned char *origOutP = outP;
	
	shifter = 7;

	for (j = 0; j < 2; j++) {
		if (j == 1)
			origOutP += w * 2;
	
		for (i = 0; i < 2; i++) {
			switch (i) {
			  case 0:
				  outP = (unsigned char *) (origOutP + 1);
				  break;
			  case 1:
				  outP = (unsigned char *) (origOutP + w);
				  break;
			}
	
			if (shifter != 7) {
				shifter = 7;
				inP++;
			}
	
			*outP = *inP++;
	
			outP += 2;      /* a3 */
			index = 2;
	
			while (index < w) {
				/* Top of decompress loop */
				tmp3 = 1 << shifter;
	
				tmp3 &= *inP;
	
				if ((shifter & 0xFF) <= 0) {
					shifter = 7;
					inP++;
				} else {
					shifter--;
				}
	
				if ((tmp3 & 0xFF) != 0) {
					*outP = *(outP - 2);
				} else {
					tmp0 = inP[1];
					tmp4 = inP[0] << 8;
	
					tmp4 |= tmp0;
	
					tmp4 = tmp4 << (7 - shifter);
	
					if (shifter >= 5) {
						shifter -= 5;
					} else {
						inP++;
						shifter += 3;
					}
	
					tmp4 = tmp4 >> 11;
	
					/* if((char)tmp4 <= 0) */
					if ((char) tmp4 == 0) {
						tmp3 = inP[0] << 8 | inP[1];
	
						tmp3 = tmp3 << (7 - shifter);
	
						if (shifter >= 8) {
							shifter -= 8;
						} else {
							inP++;
							shifter &= 0xf;
						}
	
						*outP = tmp3 >> 8;
					} else {
						if (tmp4 & 0x10) {
							tmp4 &= 0xf;
	
							*outP = *(outP - 2) - tmp4;
						} else {
							tmp4 &= 0xf;
	
							*outP = *(outP - 2) + tmp4;
						}
					}
				}
				outP += 2;
				index += 2;
			} /* End 'while (index < w)' */
		}
	
		outP = origOutP;
	
		if (shifter != 7) {
			shifter = 7;
			inP++;
		}
	
		*outP = *inP++;
	
		if (shifter != 7) {     /* this second set probably isn't 
					   needed but we're going to use 
					   it anyhow */
			shifter = 7;
			inP++;
		}
	
		outP[w + 1] = *inP++;
	
		outP += 2;              /* a3 */
		index = 2;
	
		while (index < w) {
			/* Top of decompress loop */
			tmp3 = 1 << shifter;
	
			tmp3 &= *inP;
	
			if ((shifter & 0xFF) <= 0) {
				shifter = 7;
				inP++;
			} else {
				shifter--;
			}
	
			if ((tmp3 & 0xFF) != 0) {
				*outP = outP[w - 1];
			} else {
				tmp0 = inP[1];
				tmp4 = inP[0] << 8;
	
				tmp4 |= tmp0;
	
				tmp4 = tmp4 << (7 - shifter);
	
				if (shifter >= 5) {
					shifter -= 5;
				} else {
					inP++;
					shifter += 3;
				}
	
				tmp4 = tmp4 >> 11;
	
				/* if((char)tmp4 <= 0) */
				if ((char) tmp4 == 0) {
					tmp3 = inP[0] << 8 | inP[1];
	
					tmp3 = tmp3 << (7 - shifter);
	
					if (shifter >= 8) {
						shifter -= 8;
					} else {
						inP++;
						shifter &= 0xf;
					}
	
					*outP = tmp3 >> 8;
				} else {
					if (tmp4 & 0x10) {
						tmp4 &= 0xf;
	
						*outP = outP[w - 1] - tmp4;
					} else {
						tmp4 &= 0xf;
						*outP = outP[w - 1] + tmp4;
					}
				}
			}
	
			tmp3 = 1 << shifter;
			tmp3 &= *inP;
	
			if ((shifter & 0xFF) <= 0) {
				shifter = 7;
				inP++;
			} else {
				shifter--;
			}
	
			if ((tmp3 & 0xFF) != 0) {
				outP[w + 1] = *outP;
			} else {
				tmp0 = inP[1];
				tmp4 = inP[0] << 8;
	
				tmp4 |= tmp0;
	
				tmp4 = tmp4 << (7 - shifter);
	
				if (shifter >= 5) {
					shifter -= 5;
				} else {
					inP++;
					shifter += 3;
				}
	
				tmp4 = tmp4 >> 11;
	
				/* if((char)tmp4 <= 0) */
				if ((char) tmp4 == 0) {
					tmp3 = inP[0] << 8 | inP[1];
	
					tmp3 = tmp3 << (7 - shifter);
	
					if (shifter >= 8) {
						shifter -= 8;
					} else {
						inP++;
						shifter &= 0xf;
					}
	
					outP[w + 1] = tmp3 >> 8;
				} else {
					if (tmp4 & 0x10) {
						tmp4 &= 0xf;
	
						outP[w + 1] = *outP - tmp4;
					} else {
						tmp4 &= 0xf;
	
						outP[w + 1] = *outP + tmp4;
					}
				}
			}
			outP += 2;
			index += 2;
		} /* End 'while (index < w)' */

	} /* End 'for (j = 0; j < 2; j++)' */
	return(1);
}


/***********************************************************************
 *
 * Function:	GetPicData
 *
 * Summary:     FIXME
 *
 * Parameters:  
 *
 * Returns:     
 *
 ***********************************************************************/
static int GetPicData(int r, struct Veo *v, unsigned char *row)
{
        int 	attr, 
		category, 
		len;

        unsigned char tmpRow[2560];

        if (!v->sd)
                return (-1);

        len = dlp_ReadRecordByIndex(v->sd, v->db, 1 + r / 4, tmpRow, 0, 0,
                                    &attr, &category);

        if (len < 0)
                return 0;

        Decode(tmpRow, row, v->width);

        return (len);
}


/***********************************************************************
 *
 * Function:	Gen24bitRow
 *
 * Summary:     FIXME
 *
 * Parameters:  
 *
 * Returns:     
 *
 ***********************************************************************/
int Gen24bitRow(int r, struct Veo *v, unsigned char *row)
{
        int 	i, 
		rawW, 
		rawH, 
		modR = r % 4;

        unsigned char rowA[2560], rowB[2560];
        unsigned char *rAP, *rBP, *rCP;

        rawW = v->width / 2;
        rawH = v->height / 2;

        if (r == 0) {
                if (-1 == GetPicData(r, v, rowB))
                        return (-1);
                rAP = rBP = rowB;
                rCP = rowB + v->width;

        } else if (r == (v->height - 1)) {
                if (-1 == GetPicData(r, v, rowA))
                        return (-1);
                rAP = rowA + v->width * 2;
                rCP = rBP = rowA + v->width * 3;

        } else if (modR == 0) {
                if (-1 == GetPicData(r - 1, v, rowA))
                        return (-1);
                rAP = rowA + v->width * 3;
                if (-1 == GetPicData(r, v, rowB))
                        return (-1);
                rBP = rowB;
                rCP = rowB + v->width;

        } else if (modR == 3) {
                if (-1 == GetPicData(r, v, rowA))
                        return (-1);
                rAP = rowA + v->width * 2;
                rBP = rowA + v->width * 3;
                if (-1 == GetPicData(r + 1, v, rowB))
                        return (-1);
                rCP = rowB;

        } else {
                if (-1 == GetPicData(r, v, rowA))
                        return (-1);
                rAP = rowA + v->width * (modR - 1);
                rBP = rowA + v->width * modR;
                rCP = rowA + v->width * (modR + 1);
        }

        if (r % 2 == 0) {
                /* green blue center */
                row[0] = (rAP[0] + rCP[0]) >> 1;
                row[1] = rBP[0];
                row[2] = rBP[1];

                /* blue center */
                row[3] = (rAP[0] + rAP[2] + rCP[0] + rCP[2]) >> 2;
                row[4] = (rAP[1] + rBP[0] + rBP[2] + rCP[1]) >> 2;
                row[5] = rBP[1];

                for (i = 1; i < rawW - 1; i++) {
                        /* green blue center */
                        row[i * 6] = (rAP[i * 2] + rCP[i * 2]) >> 1;
                        row[i * 6 + 1] = rBP[i * 2];
                        row[i * 6 + 2] =
                            (rBP[i * 2 - 1] + rBP[i * 2 + 1]) >> 1;

                        /* blue center */
                        row[i * 6 + 3] =
                            (rAP[i * 2] + rAP[i * 2 + 2] + rCP[i * 2] +
                             rCP[i * 2 + 2]) >> 2;
                        row[i * 6 + 4] =
                            (rAP[i * 2 + 1] + rBP[i * 2] + rBP[i * 2 + 2] +
                             rCP[i * 2 + 1]) >> 2;
                        row[i * 6 + 5] = rBP[i * 2 + 1];
                }

                i = rawW - 1;

                /* green blue center */
                row[i * 6] = (rAP[i * 2] + rCP[i * 2]) >> 1;
                row[i * 6 + 1] = rBP[i * 2];
                row[i * 6 + 2] = (rBP[i * 2 - 1] + rBP[i * 2 + 1]) >> 1;

                /* blue center */
                row[i * 6 + 3] = (rAP[i * 2] + rCP[i * 2]) >> 1;
                row[i * 6 + 4] =
                    (rAP[i * 2 + 1] + rBP[i * 2] + rCP[i * 2 + 1]) / 3;
                row[i * 6 + 5] = rBP[i * 2 + 1];
        } else {
                /* red center */
                row[0] = rBP[0];
                row[1] = (rAP[0] + rBP[1] + rCP[0]) / 3;
                row[2] = (rAP[1] + rCP[1]) >> 1;

                /* green red center */
                row[3] = (rBP[0] + rBP[2]) >> 1;
                row[4] = rBP[1];
                row[5] = (rAP[1] + rCP[1]) >> 1;

                for (i = 1; i < rawW - 1; i++) {
                        /* red center */
                        row[i * 6] = rBP[i * 2];
                        row[i * 6 + 1] =
                            (rAP[i * 2] + rBP[i * 2 - 1] + rBP[i * 2 + 1] +
                             rCP[i * 2]) >> 2;

                        /* row[i*6+1] = (rBP[i*2-1] + rBP[i*2+1]) >> 1; */
                        row[i * 6 + 2] =
                            (rAP[i * 2 - 1] + rAP[i * 2 + 1] +
                             rCP[i * 2 + 1] + rCP[i * 2 - 1]) >> 2;

                        /* green red center */
                        row[i * 6 + 3] =
                            (rBP[i * 2] + rBP[i * 2 + 2]) >> 1;
                        row[i * 6 + 4] = rBP[i * 2 + 1];
                        row[i * 6 + 5] =
                            (rAP[i * 2 + 1] + rCP[i * 2 + 1]) >> 1;
                }

                i = rawW - 1;

                /* red center */
                row[i * 6] = rBP[i * 2];
                row[i * 6 + 1] =
                    (rAP[i * 2] + rBP[i * 2 - 1] + rBP[i * 2 + 1] +
                     rCP[i * 2]) >> 2;
                row[i * 6 + 2] =
                    (rAP[i * 2 - 1] + rAP[i * 2 + 1] + rCP[i * 2 + 1] +
                     rCP[i * 2 - 1]) >> 2;

                /* green red center */
                row[i * 6 + 3] = rBP[i * 2];
                row[i * 6 + 4] = rBP[i * 2 + 1];
                row[i * 6 + 5] = (rAP[i * 2 + 1] + rCP[i * 2 + 1]) >> 1;

        }

        return (1);
}


/***********************************************************************
 *
 * Function:    WritePicture
 *
 * Summary:	FIXME
 *
 * Parameters:  
 *
 * Returns:     
 *
 ***********************************************************************/
void WritePicture(int sd, int db, char *name, char *progname)
{
        char fname[FILENAME_MAX];
        FILE *f;
        char extension[8];
        static int i = 1, len;
        struct Veo v;
        unsigned char inBuf[2560];
        unsigned char outBuf[2560];
        int attr, category;

        /*
           if(type == VEO_OUT_PNG)
           sprintf(extension, ".png");
           else if(type == VEO_OUT_PPM)
         */
        sprintf(extension, ".ppm");

        sprintf(fname, "%s", name);

        protect_files(fname, extension);

        printf("Generating %s...\n", fname);

        f = fopen(fname, "wb");
        if (f) {
                if (sd) {
                        len =
                            dlp_ReadRecordByIndex(sd, db, 0, inBuf, 0, 0,
                                                  &attr, &category);
                        unpack_Veo(&v, inBuf, len);
                        v.sd = sd;
                        v.db = db;
                } else
                        return;

                fprintf(f, "P6\n# ");

                if (name != NULL)
                        fprintf(f, "%s (created on %s)\n", name,
                                fmt_date(&v));

                fprintf(f, "%d %d\n255\n", v.width, v.height);

                for (i = 0; i < v.height; i++) {
                        Gen24bitRow(i, &v, outBuf);

                        fwrite(outBuf, v.width * 3, 1, f);
                }


                fclose(f);
        } else {
                fprintf(stderr, "%s: can't write to %s\n", progname,
                        fname);
	}

}


int main(int argc, char *argv[])
{
        int	c,		/* switch */
         	db, 
		i 		= 0, 
		sd 		= -1, 
		action 		= VEO_ACTION_OUTPUT, 
		dbcount 	= 0;

        struct DBInfo info;

        char 	*progname 	= argv[0],
		*port 		= NULL;

        struct PilotUser User;
        unsigned char buffer[0xffff];

        while ((c = getopt_long(argc, argv, optstring, options, NULL)) != -1) {
                switch (c) {
		case 'h':
			display_help(progname);
			return 0;
		case 'v':
			print_splash(progname);
			return 0;
		case 'p':
			free(port);
			port = strdup(optarg);
			break;
		case 'l':
			action = VEO_ACTION_LIST;
			break;
                }
        }

        sd = pilot_connect(port);

        if (sd < 0)
                goto error;

        if (dlp_ReadUserInfo(sd, &User) < 0)
                goto error_close;

        for (;;) {
                if (dlp_ReadDBList(sd, 0, 0x80, i, &info) < 0)
                        break;
                dbcount++;
                i = info.index + 1;
/*      if(info.type == 'EZVI' && info.creator == 'ODI2') */
                if (info.type == pi_mktag('E', 'Z', 'V', 'I')
                    && info.creator == pi_mktag('O', 'D', 'I', '2')) {
                        switch (action) {
                          case VEO_ACTION_LIST:
                                  printf("%s\n", info.name);
                                  break;

                          case VEO_ACTION_OUTPUT:
                                  printf("%s\n", info.name);
                                  if (dlp_OpenDB
                                      (sd, 0, 0x80 | 0x40, info.name,
                                       &db) < 0) {
                                          puts("Unable to open Veo database");
                                          dlp_AddSyncLogEntry(sd,
                                                              "Unable to open Veo database.\n");
                                          exit(EXIT_FAILURE);
                                  }

                                  dlp_ReadAppBlock(sd, db, 0, buffer,
                                                   0xffff);

                                  WritePicture(sd, db, info.name, progname);

/*              unpack_VeoAppInfo(&vai, buffer, 0xffff); */

                                  if (sd) {
                                          /* Close the database */
                                          dlp_CloseDB(sd, db);
                                  }

                                  break;
                        }
                }
        }

        if (sd) {
                dlp_AddSyncLogEntry(sd,
                                    "Successfully read Veo photos from Palm.\n"
                                    "Thank you for using pilot-link.");
                dlp_EndOfSync(sd, 0);
                pi_close(sd);
        }

        printf("\nList complete. %d files found.\n", dbcount);

        return 0;

      error_close:
        pi_close(sd);

      error:
        return -1;

}
