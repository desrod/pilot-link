/* 
 * pilot-xfer.c:  Palm Database transfer utility
 *
 * (c) 1996, 1998, Kenneth Albanowski.
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

#include "getopt.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h> 	 
#include <time.h>
#include <locale.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <utime.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "pi-socket.h"
#include "pi-file.h"
#include "pi-header.h"

#define pi_mktag(c1,c2,c3,c4) (((c1)<<24)|((c2)<<16)|((c3)<<8)|(c4))

/* unsigned char typedef byte; */
typedef unsigned char byte;

typedef struct {
  byte data[4];
  char attr;
  byte id[3];
} recInfo_t;

typedef struct {
  char name[32];
  byte attr[2];
  byte version[2];
  byte cdate[4];
  byte mdate[4];
  byte backupdate[4];
  byte modno[4];
  byte appinfo[4];
  byte sortinfo[4];
  char dbType[4];
  char dbCreator[4];
  byte seed[4];
  byte nextRecList[4];
  char nRec[2];
} pdb_t;

typedef enum { 
  palm_media_ram, 	/* media_type 0 */
  palm_media_rom, 	/* media_type 1 */
  palm_media_flash 	/* media_type 2 */
} palm_media_t;

typedef enum { 
  palm_op_restore,
  palm_op_backup, 
  palm_op_install, 
  palm_op_fetch,
  palm_op_list,
  palm_op_settime,
  palm_op_noop
} palm_op_t;

struct option options[] = {
	{"port",        required_argument, NULL, 'p'},
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
	{"backup",      required_argument, NULL, 'b'},
	{"update",      required_argument, NULL, 'u'},
	{"sync",        required_argument, NULL, 's'},
	{"time",        no_argument,       NULL, 't'},
	{"novsf",       no_argument,       NULL, 'S'},
	{"restore",     required_argument, NULL, 'r'},
	{"install",     required_argument, NULL, 'i'},
	{"merge",       required_argument, NULL, 'm'},
	{"fetch",       required_argument, NULL, 'f'},
	{"delete",      required_argument, NULL, 'd'},
	{"exclude",     required_argument, NULL, 'e'},
	{"Purge",       no_argument,       NULL, 'P'},
	{"list",        no_argument,       NULL, 'l'},
	{"Listall",     no_argument,       NULL, 'L'},
	{"archive",     required_argument, NULL, 'a'},
	{"exec",	required_argument, NULL, 'x'},
	{"Flash",       no_argument,       NULL, 'F'},
	{"Osflash",     no_argument,       NULL, 'O'},
	{"Illegal",     no_argument,       NULL, 'I'},
	{"verbose",     no_argument,       NULL, 'V'},
	{NULL,          0,                 NULL, 0},

};

static const char *optstring = "-p:hvb:u:s:tSr:i:m:f:d:e:PlLa:x:FOIV";

int	novsf	= 0;

int 	sd 	= 0;
char    *port 	= NULL;

#define MAXEXCLUDE 100
char 	*exclude[MAXEXCLUDE];
int 	numexclude = 0;

#define BACKUP (1<<0)
#define UPDATE (1<<1)
#define SYNC (1<<2)

/***********************************************************************
 *
 * Function:    MakeExcludeList
 *
 * Summar:      Excludes a list of dbnames from the operation called
 *
 * Parameters:  None
 *
 * Return:      Nothing
 *
 ***********************************************************************/
static void MakeExcludeList(char *efile)
{
	char 	temp[1024];
	FILE 	*f = fopen(efile, "r");

	if (!f) {
		printf("   Unable to open exclude list file '%s'.\n", efile);
		exit(EXIT_FAILURE); 
	}

	while ((fgets(temp, sizeof(temp), f)) != NULL) {
		temp[strlen(temp) - 1] = '\0';
		printf("Now excluding: %s\n", temp);
		exclude[numexclude++] = strdup(temp);
		if (numexclude == MAXEXCLUDE) {
			printf("Maximum number of exclusions reached [%d]\n", MAXEXCLUDE);
			break;
		}
	}
}


/***********************************************************************
 *
 * Function:    protect_name
 *
 * Summary:     Protects filenames and paths which include 'illegal' 
 *              characters, such as '/' and '=' in them. 
 *
 * Parameters:  None
 *
 * Return:      Nothing
 *
 ***********************************************************************/
static void protect_name(char *d, char *s)
{

	/* Maybe even..
	char *c = strchr("/=\r\n", foo); 

	if (c) { 
		d += sprintf(d, "=%02X", c); 
	}
	*/

	while (*s) {
		switch (*s) {
		case '/':
			*(d++) = '=';
			*(d++) = '2';
			*(d++) = 'F';
			break;

		case '=':
			*(d++) = '=';
			*(d++) = '3';
			*(d++) = 'D';
			break;

		case '\x0A':
			*(d++) = '=';
			*(d++) = '0';
			*(d++) = 'A';
			break;

		case '\x0D':
			*(d++) = '=';
			*(d++) = '0';
			*(d++) = 'D';
			break;

		/* Replace spaces in names with =20
		case ' ': 
			*(d++) = '='; 
			*(d++) = '2'; 
			*(d++) = '0'; 
			break; 
		*/
		default:
			*(d++) = *s;
		}
		++s;
	}
	*d = '\0';
}


/***********************************************************************
 *
 * Function:    Connect
 *
 * Summary:     Establish the connection with the device
 *
 * Parameters:  None
 *
 * Return:      Nothing
 *
 ***********************************************************************/
static void Connect(void)
 {
	if (sd != 0)
		return;

	sd = pilot_connect(port);
	if (sd < 0)
		exit(EXIT_FAILURE);
}


/***********************************************************************
 *
 * Function:    VoidSyncFlags
 *
 * Summary:     Get the last PCID and lastSyncTime from the Palm
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/ 
static void VoidSyncFlags(void)
{
	struct 	PilotUser User;

	if (dlp_ReadUserInfo(sd, &User) == 0) {
		/* Hopefully unique constant, to tell any Desktop software
		   that databases have been altered, and that a slow sync is
		   necessary 
		 */
		User.lastSyncPC = 0x00000000;	
		User.lastSyncDate = User.successfulSyncDate = time(0);
		dlp_WriteUserInfo(sd, &User);
	}
}


/***********************************************************************
 *
 * Function:    RemoveFromList
 *
 * Summary:     Remove the excluded files from the op list
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void RemoveFromList(char *name, char **list, int max)
{
	int i;

	for (i = 0; i < max; i++) {
		if (list[i] != NULL && strcmp(name, list[i]) == 0) {
			list[i] = NULL;
		}
	}
}


/***********************************************************************
 *
 * Function:    creator_is_PalmOS
 *
 * Summary:     Skip Palm files which match the internal Palm CreatorID
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int creator_is_PalmOS(unsigned long creator)
{
        union {
                long    L;
                char    C[4];
        } buf;

        union buf;

        int     n;

        static long special_cases[] = {
                pi_mktag('p', 'p', 'p', '_'),
                pi_mktag('u', '8', 'E', 'Z'),

                /* These cause a reset/crash on OS5 when accessed       */
                pi_mktag('P', 'M', 'H', 'a'),   /* Phone Link Update    */
                pi_mktag('P', 'M', 'N', 'e'),   /* Ditto                */
                pi_mktag('F', 'n', 't', '1'),   /* Hires font resource  */
                pi_mktag('m', 'o', 'd', 'm'),
        };

        for (n = 0; n < sizeof(special_cases) / sizeof(long); n++)
                if (creator == special_cases[n])
                        return 1;

        for (n = 0; n < 4; n++)
                if (buf.C[n] < 'a' || buf.C[n] > 'z')
                        return 0;

        return 1;
}


/***********************************************************************
 *
 * Function:    Backup
 *
 * Summary:     Build a file list and back up the Palm to destination
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void Backup(char *dirname, unsigned long int flags, palm_media_t 
	media_type, int unsaved, char *archive_dir)
{

	int	i		= 0,
		ofile_len	= 0,
		ofile_total	= 0,
		filecount 	= 0,
		failed		= 0,
		skipped		= 0;	

	static int totalsize;

	char 	**orig_files = NULL,
		synclog[70];

	const char *synctext = (flags & UPDATE) ? "Syncronizing" : "Backing up";

	DIR 	*dir;
	pi_buffer_t *buffer;

	/* Check if the directory exists before writing to it. If it doesn't
	   exist as a directory, and it isn't a file, create it. */
	if (access(dirname, F_OK) == -1) {
		fprintf(stderr, "   Creating directory '%s'...\n", dirname);
		mkdir(dirname, 0700);

	} else if (access(dirname, R_OK|W_OK|X_OK) != 0) {
		fprintf(stderr, "\n");
		fprintf(stderr, "   ERROR: %s\n", strerror(errno));
		fprintf(stderr, "   Please check ownership and permissions"
			" on %s.\n\n", dirname);
		return;

	} else if (flags & UPDATE) {
		if ((dir = opendir(dirname)) == NULL) {
			fprintf(stderr, "\n");
			fprintf(stderr, "   ERROR: %s\n", strerror(errno));   
			fprintf(stderr, "   Does the directory %s exist?\n\n",
				dirname);  
			return;
		} else {
			struct 	dirent *dirent;

			while ((dirent = readdir(dir))) {
				char *name;
				int dirnamelen;
				dirnamelen = strlen(dirname);

				if (dirent->d_name[0] == '.')
					continue;

				if (ofile_total >= ofile_len) {
					ofile_len += 256;
					orig_files = realloc(orig_files, 
							(sizeof (char *)) * ofile_len);
				}
				name = malloc(dirnamelen + strlen (dirent->d_name) + 2);
				if (name == NULL) {
					continue;
				} else {
					sprintf(name, "%s/%s", dirname, dirent->d_name);
					orig_files[ofile_total++] = name;
				}
			}
			closedir(dir);
		}
	}

	buffer = pi_buffer_new (sizeof(struct DBInfo));

	for(;;) {
		struct 	DBInfo info;
		struct 	pi_file *f;
		struct 	utimbuf times;

		int 	skip 	= 0;
		int 	excl	= 0;

		char 	name[256];
		struct 	stat sbuf;
		char crid[5];
	
		if (dlp_ReadDBList(sd, 0, (media_type ? 0x40 : 0x80), i, buffer) < 0)
			break;

		memcpy(&info, buffer->data, sizeof(struct DBInfo));
		i = info.index + 1;

		crid[0] = (info.creator & 0xFF000000) >> 24;
		crid[1] = (info.creator & 0x00FF0000) >> 16;
		crid[2] = (info.creator & 0x0000FF00) >> 8;
		crid[3] = (info.creator & 0x000000FF);
		crid[4] = '\0';

		if (dlp_OpenConduit(sd) < 0) {
			printf("\n   Exiting on cancel, all data was not backed up" 
			       "\n   Stopped before backing up: '%s'\n\n", info.name);
			sprintf(synclog, "\npilot-xfer was cancelled by the user "
				"before backing up '%s'.", info.name);
			dlp_AddSyncLogEntry(sd, synclog);
			pi_buffer_free(buffer);
			exit(EXIT_FAILURE);
		}
		
		strncpy(name, dirname, sizeof(name));
		name[sizeof(name) - 1] = 0;
		strcat(name, "/");
		protect_name(name + strlen(name), info.name);

		if (creator_is_PalmOS(info.creator)) {
			printf("   [-][skip][%s] Skipping OS file '%s'.\n", 
				crid, info.name);
			continue;
		}
		
		if (info.flags & dlpDBFlagResource) { 
			strcat(name, ".prc");
		} else if (info.flags & dlpDBFlagClipping) { 
			strcat(name, ".pqa");
		} else {
			strcat(name, ".pdb");
		}
		
		for (excl = 0; excl < numexclude; excl++) {
			if (strcmp(exclude[excl], info.name) == 0) {
				printf("   [-][excl] Excluding '%s'...\n", name);
				RemoveFromList(name, orig_files, ofile_total);
				skip = 1;
			}
		}

		if (info.creator == pi_mktag('a', '6', '8', 'k')) {
			printf("   [-][a68k][PACE] Skipping '%s'\n", info.name);
			skipped++;
			continue;
		}

		if (skip == 1)
			continue;

		if (!unsaved
		    && strcmp(info.name, "Unsaved Preferences") == 0) {
			printf("   [-][unsv] Skipping '%s'\n", info.name);
			continue;
		}

		RemoveFromList(name, orig_files, ofile_total); 
		if ((stat(name, &sbuf) && (flags & UPDATE) == 0)) {
			if (info.modifyDate == sbuf.st_mtime) {
				printf("   [-][unch] Unchanged, skipping %s\n",
					name);
				continue;
			}
		}

		/* Ensure that DB-open and DB-ReadOnly flags are not kept */
		info.flags &= ~(dlpDBFlagOpen | dlpDBFlagReadOnly);

		totalsize += sbuf.st_size;
		printf("   [+][%-4d]", filecount);
		printf("[%s] %s '%s'", crid, synctext, info.name);
		
		setlocale(LC_ALL, "");
		printf(", %'ld bytes, %'ld KiB... ", 
			(long)sbuf.st_size, (long)totalsize/1024);

		filecount++;

		f = pi_file_create(name, &info);

		if (f == 0) {
			printf("\nFailed, unable to create file.\n");
			break;
		} else if (pi_file_retrieve(f, sd, 0) < 0) {
			printf("\n   [-][fail][%s] Failed, unable to retrieve %s from the Palm.\n", 
				crid, info.name);
			failed++;
		} 
		pi_file_close(f);
		printf("\n");

		times.actime 	= info.createDate;
		times.modtime 	= info.modifyDate;
		utime(name, &times);
	}
	pi_buffer_free(buffer);
		
	if (orig_files) {
		int     i = 0;
		int 	dirname_len = strlen(dirname);
		char 	newname[256];

		for (i = 0; i < ofile_total; i++) {
			if (orig_files[i] != NULL) {
				if (flags & SYNC) {
					if (archive_dir) {
						printf("Archiving '%s'", orig_files[i]);

						sprintf(newname, "%s/%s", archive_dir, 
							&orig_files[i] [dirname_len + 1]);
						
						if (rename (orig_files[i], newname) != 0) {
							printf("rename(%s, %s) ", orig_files [i], 
								newname);
							perror("failed");
						}
					} else {
						printf("Removing '%s'.\n", orig_files[i]); 
						unlink(orig_files[i]);
					}
				}
				free(orig_files[i]);
			}
		}
	}

	printf("%s backup complete.",
	       (media_type == 2 ? "\n   OS" 
		: (media_type == 1 ? "\n   Flash" : "\n   RAM")));

	printf(" %d files backed up, %d skipped, %d file%s failed.\n", 
		(filecount ? filecount - 1 : 0), skipped, failed, (failed == 1) ? "" : "s");

	sprintf(synclog, "%d files successfully backed up.\n\n"
		"Thank you for using pilot-link.", filecount - 1);
	dlp_AddSyncLogEntry(sd, synclog);
}


/***********************************************************************
 *
 * Function:    Fetch
 *
 * Summary:     Grab a file from the Palm, write to disk
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void Fetch(char *dbname)
{
	struct 	DBInfo info;
	char 	name[256],
		synclog[70];

	struct 	pi_file *f;

	if (access(dbname, F_OK) == 0 && access(dbname, R_OK|W_OK) != 0) {
		fprintf(stderr, "\n   Unable to write to %s, check "
			"ownership and permissions.\n\n", dbname);
		exit(EXIT_FAILURE);
	}

	printf("   Parsing list of files from handheld... ");
	fflush(stdout);
	if (dlp_FindDBInfo(sd, 0, 0, dbname, 0, 0, &info) < 0) {
		printf("\n   Unable to locate app/database '%s', ", 
			dbname);
		printf("fetch skipped.\n   Did you spell it correctly?\n\n");
		return;
	} else {
		printf("done.\n");
	}

	protect_name(name, dbname);

	/* Judd - Graffiti hack 
	   Graffiti ShortCuts with a space on the end or not is really
	   supposed to be the same file, so we will treat it as such to
	   avoid confusion, remove the space.
	 */
	if (strcmp(name, "Graffiti ShortCuts ") == 0) {
		strncpy(name, "Graffiti ShortCuts", sizeof(name));
	}

	if (info.flags & dlpDBFlagResource) { 
		strcat(name, ".prc");   
        } else if (info.flags & dlpDBFlagClipping) {
		strcat(name, ".pqa");
        } else {
		strcat(name, ".pdb");   
        }

	printf("   Fetching %s... ", name);
	fflush(stdout);

	info.flags &= 0x2fd;

	/* Write the file records to disk as 'dbname' 	*/
	f = pi_file_create(name, &info);
	if (f == 0) {
		printf("Failed, unable to create file.\n");
		return;
	} else if (pi_file_retrieve(f, sd, 0) < 0) {
		printf("Failed, unable to fetch database from the Palm.\n");
	}

	printf("complete.\n\n");	

        sprintf(synclog, "File '%s' successfully fetched.\n\n"
		"Thank you for using pilot-link.", name);
        dlp_AddSyncLogEntry(sd, synclog);

	pi_file_close(f);
}


/***********************************************************************
 *
 * Function:    Delete
 *
 * Summary:     Delete a database from the Palm
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void Delete(char *dbname)
{
	struct 	DBInfo info;

	Connect();

	dlp_FindDBInfo(sd, 0, 0, dbname, 0, 0, &info);

	printf("Deleting '%s'... ", dbname);
	if (dlp_DeleteDB(sd, 0, dbname) >= 0) {
		if (info.type == pi_mktag('b', 'o', 'o', 't')) {
			printf(" (rebooting afterwards) ");
		}
		printf("OK\n");
	} else {
		printf("Failed, unable to delete database\n");
	}
	fflush(stdout);

	printf("Delete complete.\n");
}

struct db {
	int 	flags,
		maxblock;

	char 	name[256];

	unsigned long creator, type;
};

static int compare(struct db *d1, struct db *d2)
{
	/* types of 'appl' sort later then other types */
	if (d1->creator == d2->creator)
		if (d1->type != d2->type) {
			if (d1->type == pi_mktag('a', 'p', 'p', 'l'))
				return 1;
			if (d2->type == pi_mktag('a', 'p', 'p', 'l'))
				return -1;
		}
	return d1->maxblock < d2->maxblock;
}


/***********************************************************************
 *
 * Function:    Restore
 *
 * Summary:     Send files to the Palm from disk, restoring Palm
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void Restore(char *dirname)
{
	int 	dbcount 	= 0,
		i,
		j,
		max,
		save_errno 	= errno;
	size_t	size;
	DIR 	*dir;
	struct 	dirent *dirent;
	struct 	DBInfo info;
	struct 	db **db 	= NULL;
	struct 	pi_file *f;
	struct  stat sbuf;

        struct  CardInfo Card;

        Card.card = -1;
        Card.more = 1;
		
	if ((dir = opendir(dirname)) == NULL) {
		fprintf(stderr, "\n");
		perror("   ERROR");
		fprintf(stderr, "   opendir() failed. Cannot open directory %s.\n", dirname);
		fprintf(stderr, "   Does the directory exist?\n\n");
		errno = save_errno;
		exit(EXIT_FAILURE);
	}

	/* Find out how many directory entries exist, so that we can
	   allocate the buffer.  We avoid scandir() for maximum portability.

	   The count is a bit conservative, as it includes . and .. entries.
	 */
	while (readdir(dir))
		dbcount++;

	db = (struct db **) calloc(dbcount, sizeof(struct db *));

	if (!db) {
		printf("Unable to allocate memory for directory entry table\n");
		exit(EXIT_FAILURE);
	}

	dbcount = 0;
	rewinddir(dir);

	while ((dirent = readdir(dir)) != NULL) {

		if (dirent->d_name[0] == '.')
			continue;

		db[dbcount] = (struct db *) malloc(sizeof(struct db));

		sprintf(db[dbcount]->name, "%s/%s", dirname,
			dirent->d_name);

		f = pi_file_open(db[dbcount]->name);
		if (f == 0) {
			printf("Unable to open '%s'!\n",
			       db[dbcount]->name);
			break;
		}

		pi_file_get_info(f, &info);

		db[dbcount]->creator 	= info.creator;
		db[dbcount]->type 	= info.type;
		db[dbcount]->flags 	= info.flags;
		db[dbcount]->maxblock 	= 0;

		pi_file_get_entries(f, &max);

		for (i = 0; i < max; i++) {
			if (info.flags & dlpDBFlagResource) {
				pi_file_read_resource(f, i, 0, &size, 0, 0);
			} else {
				pi_file_read_record(f, i, 0, &size, 0, 0, 0);
			}

			if (size > db[dbcount]->maxblock)
				db[dbcount]->maxblock = size;
		}

		pi_file_close(f);
		dbcount++;
	}

	closedir(dir);

	for (i = 0; i < dbcount; i++) {
		for (j = i + 1; j < dbcount; j++) {
			if (compare(db[i], db[j]) > 0) {
				struct db *temp = db[i];

				db[i] = db[j];
				db[j] = temp;
			}
		}
	}

	for (i = 0; i < dbcount; i++) {

		f = pi_file_open(db[i]->name);
		if (f == 0) {
			printf("Unable to open '%s'!\n", db[i]->name);
			break;
		}
		printf("Restoring %s... ", db[i]->name);
		fflush(stdout);

	        stat(db[i]->name, &sbuf);
 
	        while (Card.more) {  
        	        if (dlp_ReadStorageInfo(sd, Card.card + 1, &Card) < 0)
	                        break;
	        }

	        if (sbuf.st_size > Card.ramFree) {
        	        fprintf(stderr, "\n\n");
                	fprintf(stderr, "   Insufficient space to install this file on your Palm.\n");
	                fprintf(stderr, "   We needed %lu and only had %lu available..\n\n",
        	                (unsigned long)sbuf.st_size, Card.ramFree);
			exit(EXIT_FAILURE);
        	}

		if (pi_file_install(f, sd, 0) < 0) {
			printf("failed.\n");
		} else {
			printf("OK\n");
		}

		pi_file_close(f);
	}

	if (!novsf)
		VoidSyncFlags();

	for (i = 0; i < dbcount; i++) {
		free(db[i]);
	}
	free(db);

	printf("Restore done\n");
}


/***********************************************************************
 *
 * Function:    Install
 *
 * Summary:     Push file(s) to the Palm
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void Install(char *filename)
{
	static int totalsize;

	struct 	pi_file *f;
	struct 	stat sbuf;

        struct  CardInfo Card;

        Card.card = -1;
        Card.more = 1;

	f = pi_file_open(filename);

	if (f == 0) {
		printf("Unable to open '%s'!\n", filename);
		return;
	}

	if (dlp_OpenConduit(sd) < 0) {
                fprintf(stderr, "\nExiting on cancel, some files were not"
			       "installed\n\n");
		exit(EXIT_FAILURE);
	}

	fprintf(stderr, "   Installing '%s'... ", filename);

	stat(filename, &sbuf);

        while (Card.more) {
                if (dlp_ReadStorageInfo(sd, Card.card + 1, &Card) < 0)
                        break;
	}

	if (sbuf.st_size > Card.ramFree) {
		fprintf(stderr, "\n\n");
		fprintf(stderr, "   Insufficient space to install this file on your Palm.\n");
		fprintf(stderr, "   We needed %lu and only had %lu available..\n\n", 
			(unsigned long)sbuf.st_size, Card.ramFree);
		return;
	}

	if (pi_file_install(f, sd, 0) < 0) {
		fprintf(stderr, "failed.\n");

	} else if (stat(filename, &sbuf) == 0) {
		printf("(%lu bytes, %ld KiB total)\n\n",
			(unsigned long)sbuf.st_size, (totalsize == 0) 
			? (long)sbuf.st_size/1024 
			: totalsize/1024);
		totalsize += sbuf.st_size;
	}
	
	pi_file_close(f);

	if (!novsf)
		VoidSyncFlags();
}


/***********************************************************************
 *
 * Function:    Merge
 *
 * Summary:     Adds the records in <file> into the corresponding
 *		Palm database
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void Merge(char *filename)
{
	struct pi_file *f;

	Connect();

	f = pi_file_open(filename);
	if (f == 0) {
		printf("Unable to open '%s'!\n", filename);
		return;
	}

	printf("Merging %s... ", filename);
	fflush(stdout);
	if (pi_file_merge(f, sd, 0) < 0)
		printf("failed.\n");
	else
		printf("OK\n");
	pi_file_close(f);
	free(f);

	if (!novsf)
		VoidSyncFlags();

	printf("Merge done\n");
}


/***********************************************************************
 *
 * Function:    List
 *
 * Summary:     List the databases found on the Palm device
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void List(palm_media_t media_type)
{
	int 	i		= 0,
		dbcount 	= 0; 
	struct 	DBInfo info;
	char	text[10],
		synclog[68];
	pi_buffer_t *buffer;
	
	printf("DEBUG: media_type: %d\n", media_type);

	media_type == 1 ? sprintf(text, " and ROM") : sprintf(text, "%s", "");
	printf("   Reading list of databases in RAM%s...\n", text);
	
	buffer = pi_buffer_new(sizeof(struct DBInfo));
	
	for (;;) {
		if (dlp_ReadDBList
		    (sd, 0, (media_type ? 0x80 | 0x40 : 0x80), i, buffer) < 0)
			break;
		memcpy(&info, buffer->data, sizeof(struct DBInfo));
		dbcount++;
		i = info.index + 1;
		printf("   %s\n", info.name);
		fflush(stdout);
	}
	pi_buffer_free(buffer);
	
	printf("\n   List complete. %d files found.\n\n", dbcount);
	sprintf(synclog, "List complete. %d files found..\n\nThank you for using pilot-link.",
		dbcount);
	dlp_AddSyncLogEntry(sd, synclog);
}


/***********************************************************************
 *
 * Function:    Purge
 *
 * Summary:     Purge any deleted data that hasn't been cleaned up
 *              by a sync
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void Purge(void)
{
	int 	i	= 0,
		h;
	struct 	DBInfo info;
	pi_buffer_t *buffer;

	Connect();

	printf("Reading list of databases to purge...\n");

	buffer = pi_buffer_new(sizeof(struct DBInfo));

	for (;;) {
		
		if (dlp_ReadDBList(sd, 0, 0x80, i, buffer) < 0)
			break;

		memcpy (&info, buffer->data, sizeof(struct DBInfo));
		i = info.index + 1;

		if (info.flags & 1)
			continue;	/* skip resource databases */

		printf("Purging deleted records from '%s'... ", info.name);

		h = 0;
		if ((dlp_OpenDB(sd, 0, 0x40 | 0x80, info.name, &h) >= 0) &&
		    (dlp_CleanUpDatabase(sd, h) >= 0) &&
		    (dlp_ResetSyncFlags(sd, h) >= 0)) {
			printf("OK\n");
		} else {
			printf("Failed\n");
		}

		if (h != 0)
			dlp_CloseDB(sd, h);

	}

	pi_buffer_free (buffer);

	if (!novsf)
		VoidSyncFlags();
	
	printf("Purge complete.\n");
}


/***********************************************************************
 *
 * Function:    palm_time
 *
 * Summary:     Sync time
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void palm_time(void)
{
	time_t  t1, 
		t2;
	char    synclog[60],
		timebuf[63];
	struct	tm *tm_ptr;
	struct  SysInfo sys;

	dlp_ReadSysInfo(sd, &sys);

	if ((sys.romVersion) == 0x03303000) {
		dlp_AddSyncLogEntry(sd, "Syncing time will cause the device \
			to\nhard-reset on PalmOS version 3.3!\n");
	} else {
		dlp_GetSysDateTime(sd, &t2);
		t1 = time(NULL);
		tm_ptr = localtime(&t2);
		dlp_SetSysDateTime(sd, t1);

		strftime(timebuf, 63, "   Palm time was successfully set to: "
			"%m/%d/%Y %X %p\n\n", tm_ptr);

		printf(timebuf);
		sprintf(synclog, "Time sync successful.\n\nThank you for using pilot-link.");
		dlp_AddSyncLogEntry(sd, synclog);
	}
}


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
static void display_help(const char *progname)
{
	printf("   Sync, backup, install, delete and more from your Palm device.\n");
	printf("   This is the swiss-army-knife of the entire pilot-link suite.\n\n");
	printf("   Usage: %s [-p port] [ -F|-O -I -q|-c ] command(s)\n", progname);
	printf("   Options:\n");
	printf("     -p, --port <port>       Use device file <port> to communicate with Palm\n");
	printf("     -h, --help              Display help information for pilot-xfer\n");
	printf("     -v, --version           Display pilot-xfer version information\n");
	printf("     -b, --backup <dir>      Back up your Palm to <dir>\n");
	printf("     -u, --update <dir>      Update <dir> with newer Palm data\n");
	printf("     -s, --sync <dir>        Same as -u above, but removes local files if\n");
	printf("                             data is removed from your Palm\n");
	printf("     -S, --novsf             Do NOT reset the SyncFlags when sync completes\n");
	printf("     -r, --restore <dir>     Restore backupdir to your Palm\n");
	printf("     -i, --install [db] ..   Install local prc, pdb, pqa files to your Palm\n");
	printf("     -m, --merge [file] ..   Adds the records in <file> into the corresponding\n");
	printf("                             Palm database\n");
	printf("     -f, --fetch [db]        Retrieve [db] from your Palm\n");
	printf("     -d, --delete [db]       Delete (permanently) [db] from your Palm\n");
	printf("     -e, --exclude <file>    Exclude databases listed in <file> from being included\n");
	printf("                             by -b, -s, or -u (See pilot-xfer(1) for more detail)\n");
	printf("     -P, --Purge             Purge any deleted data that hasn't been cleaned up\n");
	printf("                             by a sync\n");
	printf("     -l, --list              List all application and 3rd party Palm data/apps\n");
	printf("     -L, --List              List all data, internal and external on the Palm\n");
	printf("     -a, --archive           Modifies -s to archive deleted files in specified\n");
	printf("                             directory.\n");
	printf("     -x, --exec              Execute a shell command for intermediate processing\n");
	printf("     -t, --time              Sync the time on the Palm with the desktop time\n");
	printf("     -F, --Flash             Modifies -b, -u, and -s, to back up non-OS db's\n");
	printf("                             from Flash ROM\n");
	printf("     -O, --Osflash           Modifies -b, -u, and -s, to back up OS db 's from\n");
	printf("                             Flash ROM\n");
	printf("     -I, --Illegal           Modifies -b, -u, and -s, to back up the 'illegal'\n");
	printf("                             database Unsaved Preferences.prc (normally skipped,\n");
	printf("                             per Palm's recommendation)\n\n");
	printf("   The serial port used to connect to may be specified by the $PILOTPORT\n");
	printf("   environment variable in your shell instead of the command line.  If it is\n");
	printf("   not specified anywhere, it will default to /dev/pilot.\n\n");
	printf("   Additionally, the baud rate to connect with may be specified by the\n");
	printf("   $PILOTRATE environment variable.If not specified, it will default to\n");
	printf("   a safe rate of 9600.\n\n");
	printf("   Please use caution setting $PILOTRATE to higher values, as several types\n");
	printf("   of workstations have problems with higher baud rates.  Always consult the\n");
	printf("   man page(s) for additional usage of these options as well as details on\n");
	printf("   the results of combining other parameters together.\n\n");

	return;
}


int main(int argc, char *argv[])
{
	int 	optc,		/* switch */
		unsaved 	= 0,
                verbose         = 0;

	char 	*archive_dir 	= NULL,
		*dirname	= NULL,
		*dbname		= NULL,
		*progname 	= argv[0];

	unsigned long int sync_flags = palm_op_backup;
	palm_media_t media_type      = palm_media_ram;
	palm_op_t palm_operation     = palm_op_noop;

	while ((optc = getopt_long(argc, argv, optstring, options, NULL)) != -1) {
		switch (optc) {

                case 'V':
                        verbose = 1;
                        break;
		case 'h':
			display_help(progname);
			return 0;
		case 'v':
			print_splash(progname);
			return 0;
		case 'p':
			port = optarg;
                        if (verbose)
                                printf("Option -p with value: %s\n", optarg);
			break;
		case 'b':
			dirname = optarg;
			palm_operation = palm_op_backup;
			sync_flags = BACKUP;
                        if (verbose)
                                printf("Option -b with value: %s\n", optarg);
			break;
		case 'u':
			dirname = optarg;
			palm_operation = palm_op_backup;
			sync_flags = UPDATE;
                        if (verbose) 
                                printf("Option -u with value: %s\n", optarg);
			break;
		case 's':
			dirname = optarg;
			palm_operation = palm_op_backup;
			sync_flags = UPDATE|SYNC;
			if (verbose)
				printf("Option -s with value: %s\n", optarg);
			break;
		case 't':
			palm_operation = palm_op_settime;
			break;
		case 'r':
			dirname = optarg;
			palm_operation = palm_op_restore;
			break;
		case 'i':
			dbname = optarg;
			palm_operation = palm_op_install;
                        if (verbose)
                                printf("Option -i with value: %s\n", optarg);
			break;
		case 'm':
			Merge(optarg);
			break;
		case 'f':
			dbname = optarg;
			palm_operation = palm_op_fetch;
			break;
		case 'd':
			Delete(optarg);
			break;
		case 'e':
			MakeExcludeList(optarg);
			break;
		case 'P':
			Purge();
			break;
		case 'l':
			media_type = palm_media_ram;
                        palm_operation = palm_op_list;
			break;
		case 'L':
			media_type = palm_media_rom;
			palm_operation = palm_op_list;
			break;
		case 'a':
			archive_dir = optarg;
			break;
		case 'x':
			if (system(optarg)) {
				fprintf(stderr,"system() failed, aborting.\n");
				return -1;
			}
			break;
		case 'F':
			media_type = palm_media_flash;
			break;
		case 'O':
			media_type = palm_media_rom;
			break;
		case 'I':
			unsaved = 1;
			break;
		case 'S':
			novsf = 1;
			break;
		default:
			break;
		}
	}

	switch (palm_operation) {
		struct 	stat sbuf;
		case palm_op_backup: 
			if (stat (dirname, &sbuf) == 0) {
				if (!S_ISDIR (sbuf.st_mode)) {
					fprintf(stderr, "   ERROR: '%s' is a file, and not a directory.\n"
						"   Please supply a directory name when performing a "
						"backup operation and try again.\n\n",
						dirname);
					fprintf(stderr, "   Thank you for using pilot-link.\n");
					exit(EXIT_FAILURE);
				}
			}
			Connect();
			Backup(dirname, sync_flags, media_type, unsaved, archive_dir);		
			break;
		case palm_op_install:
			Connect();
			Install(dbname);
			break;
		case palm_op_restore:
			Connect();
			Restore(dirname);
			break;
		case palm_op_fetch:
			Connect();
			Fetch(dbname);
			break;
		case palm_op_list:
			Connect();
			List(media_type);
			break;
		case palm_op_settime:
			Connect();
			palm_time();
			break;
		case palm_op_noop:
			printf("   Insufficient or invalid options supplied.\n");
			printf("   Please use 'pilot-xfer --help' for more info.\n\n");
			break;
        }

	pi_close(sd);
	printf("   Thank you for using pilot-link.\n");
	return 0;
}
