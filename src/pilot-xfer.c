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

#include "popt.h"
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
#include <fcntl.h>

#include "pi-socket.h"
#include "pi-file.h"
#include "pi-header.h"
#include "pi-util.h"


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
  palm_op_noop = 0,
  palm_op_restore,
  palm_op_backup,
  palm_op_install,
  palm_op_merge,
  palm_op_fetch,
  palm_op_delete,
  palm_op_list
} palm_op_t;

int 	sd 	= 0;
char    *port 	= NULL;
char    *vfsdir = NULL;

#define MAXEXCLUDE 100
char 	*exclude[MAXEXCLUDE];
int 	numexclude = 0;

#define BACKUP      (0x0001)
#define UPDATE      (0x0002)
#define SYNC        (0x0004)

#define MEDIA_MASK  (0x0f00)
#define MEDIA_RAM   (0x0000)
#define MEDIA_ROM   (0x0100)
#define MEDIA_FLASH (0x0200)
#define MEDIA_VFS   (0x0400)

#define MIXIN_MASK  (0xf000)
#define PURGE       (0x1000)


static int findVFSPath(int verbose, const char *path, long *volume,
	char *rpath, int *rpathlen);


const char *media_name(int m)
{
	switch (m) {
	case MEDIA_RAM : return "RAM";
	case MEDIA_ROM : return "OS";
	case MEDIA_FLASH : return "Flash";
	case MEDIA_VFS : return "VFS";
	default : return NULL;
	}
}


/***********************************************************************
 *
 * Function:    make_excludelist
 *
 * Summar:      Excludes a list of dbnames from the operation called
 *
 * Parameters:  None
 *
 * Return:      Nothing
 *
 ***********************************************************************/
static void make_excludelist(const char *efile)
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
static void protect_name(char *d, const char *s)
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
 * Function:    palm_connect
 *
 * Summary:     Establish the connection with the device
 *
 * Parameters:  None
 *
 * Return:      Nothing
 *
 ***********************************************************************/
static void palm_connect(void)
 {
	if (sd != 0)
		return;

	sd = pilot_connect(port);
	if (sd < 0)
		exit(EXIT_FAILURE);
}



/***********************************************************************
 *
 * Function:    list_remove
 *
 * Summary:     Remove the excluded files from the op list
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void list_remove(char *name, char **list, int max)
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
 * Function:    palm_creator
 *
 * Summary:     Skip Palm files which match the internal Palm CreatorID
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int palm_creator(unsigned long creator)
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
 * Function:    palm_backup
 *
 * Summary:     Build a file list and back up the Palm to destination
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void palm_backup(const char *dirname, unsigned long int flags,
	int unsaved, const char *archive_dir)
{

	int	i		= 0,
		ofile_len	= 0,
		ofile_total	= 0,
		filecount 	= 1,	/* File counts start at 1, of course */
		failed		= 0,
		skipped		= 0;

	static int totalsize;

	char 	**orig_files = NULL,
		*name,
		synclog[70];

	const char *synctext = (flags & UPDATE) ? "Synchronizing" : "Backing up";

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
	name = (char *)malloc(strlen(dirname) + 1 + 256);

	for(;;) {
		struct 	DBInfo info;
		struct 	pi_file *f;
		struct 	utimbuf times;

		int 	skip 	= 0;
		int 	excl	= 0;

		struct 	stat sbuf;
		char crid[5];

		if (!pi_socket_connected(sd)) {
			printf("\n   Connection broken - Exiting. All data was not backed up\n");
			exit(EXIT_FAILURE);
		}

		if (dlp_ReadDBList(sd, 0, ((flags & MEDIA_MASK) ? 0x40 : 0x80), i, buffer) < 0)
			break;

		memcpy(&info, buffer->data, sizeof(struct DBInfo));
		i = info.index + 1;

		pi_untag(crid,info.creator);

		if (dlp_OpenConduit(sd) < 0) {
			printf("\n   Exiting on cancel, all data was not backed up"
			       "\n   Stopped before backing up: '%s'\n\n", info.name);
			sprintf(synclog, "\npilot-xfer was cancelled by the user "
				"before backing up '%s'.", info.name);
			dlp_AddSyncLogEntry(sd, synclog);
			exit(EXIT_FAILURE);
		}

		strcpy(name, dirname);
		strcat(name, "/");
		protect_name(name + strlen(name), info.name);

		if (palm_creator(info.creator)) {
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
				list_remove(name, orig_files, ofile_total);
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

	        list_remove(name, orig_files, ofile_total);
		if (stat(name, &sbuf) && (flags & UPDATE) == 0) {
			if (info.modifyDate == sbuf.st_mtime) {
				printf("   [-][unch] Unchanged, skipping %s\n",
					name);
				continue;
			}
		}

		/* Ensure that DB-open and DB-ReadOnly flags are not kept */
		info.flags &= ~(dlpDBFlagOpen | dlpDBFlagReadOnly);

		printf("   [+][%-4d]", filecount);
		printf("[%s] %s '%s'", crid, synctext, info.name);
		fflush(NULL);

		setlocale(LC_ALL, "");

		f = pi_file_create(name, &info);

		if (f == 0) {
			printf("\nFailed, unable to create file.\n");
			break;
		} else if (pi_file_retrieve(f, sd, 0, NULL) < 0) {
			printf("\n   [-][fail][%s] Failed, unable to retrieve '%s' from the Palm.",
				crid, info.name);
			failed++;
			pi_file_close(f);
			unlink(name);
		} else {
			pi_file_close(f);		/* writes the file to disk so we can stat() it */
			stat(name, &sbuf);
			totalsize += sbuf.st_size;
			printf(", %'ld bytes, %'ld KiB... ",
				(long)sbuf.st_size, (long)totalsize/1024);
			fflush(NULL);
		}

		filecount++;

		printf("\n");

		times.actime 	= info.createDate;
		times.modtime 	= info.modifyDate;
		utime(name, &times);
	}
	pi_buffer_free(buffer);

	if (orig_files) {
		int     i = 0;
		int 	dirname_len = strlen(dirname);

		for (i = 0; i < ofile_total; i++) {
			if (orig_files[i] != NULL) {
				if (flags & SYNC) {
					if (archive_dir) {
						printf("Archiving '%s'", orig_files[i]);

						sprintf(name, "%s/%s", archive_dir,
							&orig_files[i] [dirname_len + 1]);

						if (rename (orig_files[i], name) != 0) {
							printf("rename(%s, %s) ", orig_files [i],
								name);
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

	free(name);


	printf("\n   %s backup complete.", media_name(flags & MEDIA_MASK));

	printf(" %d files backed up, %d skipped, %d file%s failed.\n",
		(filecount ? filecount - 1 : 0), skipped, failed, (failed == 1) ? "" : "s");

	sprintf(synclog, "%d files successfully backed up.\n\n"
		"Thank you for using pilot-link.", filecount - 1);
	dlp_AddSyncLogEntry(sd, synclog);
}


/***********************************************************************
 *
 * Function:    palm_fetch
 *
 * Summary:     Grab a file from the Palm, write to disk
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void palm_fetch(const char *dbname)
{
	struct 	DBInfo info;
	char 	name[256],
		synclog[512];

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
	} else if (pi_file_retrieve(f, sd, 0, NULL) < 0) {
		printf("Failed, unable to fetch database from the Palm.\n");
	}

	printf("complete.\n\n");

        snprintf(synclog, sizeof(synclog)-1,
				"File '%s' successfully fetched.\n\n"
				"Thank you for using pilot-link.", name);
        dlp_AddSyncLogEntry(sd, synclog);

	pi_file_close(f);
}


/***********************************************************************
 *
 * Function:    palm_delete
 *
 * Summary:     Delete a database from the Palm
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void palm_delete(const char *dbname)
{
	struct 	DBInfo info;

	palm_connect();

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
 * Function:    palm_restore
 *
 * Summary:     Send files to the Palm from disk, restoring Palm
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void palm_restore(const char *dirname)
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

		if (pi_file_install(f, sd, 0, NULL) < 0) {
			printf("failed.\n");
		} else {
			printf("OK\n");
		}

		pi_file_close(f);
	}

	for (i = 0; i < dbcount; i++) {
		free(db[i]);
	}
	free(db);

	printf("Restore done\n");
}


/***********************************************************************
 *
 * Function:    palm_install_internal
 *
 * Summary:     Push file(s) to the Palm
 *
 * Parameters:  filename --> local filesystem path for file to install.
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void palm_install_internal(const char *filename)
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

	if (pi_file_install(f, sd, 0, NULL) < 0) {
		fprintf(stderr, "failed.\n");

	} else if (stat(filename, &sbuf) == 0) {
		printf("(%lu bytes, %ld KiB total)\n\n",
			(unsigned long)sbuf.st_size, (totalsize == 0)
			? (long)sbuf.st_size/1024
			: totalsize/1024);
		totalsize += sbuf.st_size;
	}

	pi_file_close(f);
}

/***********************************************************************
 *
 * Function:    palm_install_VFS
 *
 * Summary:     Push file(s) to the Palm
 *
 * Parameters:  filename --> local filesystem path for file to install.
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void palm_install_VFS(const char *localfile, const char *vfspath)
{
	static unsigned long totalsize = 0;
	long volume = -1;
	long used,total,freespace;
	char rpath[vfsMAXFILENAME];
	int rpathlen = vfsMAXFILENAME;
	const char *basename;
	FileRef file;
	long attributes;
	char *filebuffer;
	int fd,writesize,offset;
	size_t readsize;

	struct stat sbuf;

	if (NULL == vfspath) {
		/* how the heck did we get here then? */
		fprintf(stderr,"\n   No VFS path given.\n");
		return;
	}

	if (dlp_OpenConduit(sd) < 0) {
                fprintf(stderr, "\nExiting on cancel, some files were not"
			       "installed\n\n");
		exit(EXIT_FAILURE);
	}

	if (findVFSPath(0,vfspath,&volume,rpath,&rpathlen) < 0) {
		fprintf(stderr,"\n   VFS path '%s' does not exist.\n\n",vfspath);
		return;
	}

	fprintf(stderr, "   Installing '%s'... ", localfile);

	if (stat(localfile, &sbuf) < 0) {
		fprintf(stderr,"   Unable to open '%s'!\n", localfile);
		return;
	}
	if (S_IFREG != (sbuf.st_mode & S_IFREG))
	{
		fprintf(stderr,"   Not a regular file.\n");
		return;
	}

	if (dlp_VFSVolumeSize(sd,volume,&used,&total)<0) {
		fprintf(stderr,"   Unable to get volume size.\n");
		return;
	}
	/* Calculate free space but leave last 64k free on card */
	freespace  = total - used - 65536 ;

	if (sbuf.st_size > freespace) {
		fprintf(stderr, "\n\n");
		fprintf(stderr, "   Insufficient space to install this file on your Palm.\n");
		fprintf(stderr, "   We needed %lu and only had %lu available..\n\n",
			(unsigned long)sbuf.st_size, freespace);
		return;
	}
#define APPEND_BASENAME fd-=1; \
			basename = strrchr(localfile,'/'); \
			if (NULL == basename) basename = localfile; else basename++; \
			if (rpath[rpathlen-1] != '/') { \
				rpath[rpathlen++]='/'; \
				rpath[rpathlen]=0; \
			} \
			strncat(rpath,basename,vfsMAXFILENAME-rpathlen-1); \
			rpathlen = strlen(rpath);

	fd = 0;
	while (fd<2)
	{
		/* Don't retry by default. APPEND_BASENAME changes
		the file being tries, so it decrements fd again.
		Because we add _two_ here, (two steps fwd, one step back)
		we try at most twice anyway. */
		fd+=2;

		if (dlp_VFSFileOpen(sd,volume,rpath,dlpVFSOpenRead,&file) < 0)
		{
			/* Target doesn't exist. If it ends with a /, try to
			create the directory and then act as if the existing
			directory was given as target. If it doesn't, carry
			on, it's a regular file to create. */
			if ('/' == rpath[rpathlen-1]) {
				/* directory, doesn't exist. Don't try to mkdir /. */
				if ((rpathlen > 1) &&
					(dlp_VFSDirCreate(sd,volume,rpath) < 0)) {
					fprintf(stderr,"  Could not create destination directory.\n");
					return;
				}
				APPEND_BASENAME
			}
			if (dlp_VFSFileCreate(sd,volume,rpath) < 0) {
				fprintf(stderr,"  Cannot create destination file '%s'.\n",rpath);
				return;
			}
		}
		else
		{
			/* Exists, and may be a directory, or a filename. If it's
			a filename, that's fine as long as we're installing
			just a single file. */
			if (dlp_VFSFileGetAttributes(sd,file,&attributes) < 0)
			{
				fprintf(stderr,"   Could not get attributes for destination.\n");
				(void) dlp_VFSFileClose(sd,file);
				return;
			}

			if (attributes & vfsFileAttrDirectory) {
				APPEND_BASENAME
				dlp_VFSFileClose(sd,file);
				/* Now for sure it's a filename in a directory. */
			}
			else {
				dlp_VFSFileClose(sd,file);
				if ('/' == rpath[rpathlen-1]) {
					/* was expecting a directory */
					fprintf(stderr,"   Destination is not a directory.\n");
					return;
				}
				if (totalsize > 0) {
					fprintf(stderr,"   Must specify directory when installing multiple files.\n");
					return;
				}
			}
		}
	}
#undef APPEND_BASENAME

	if (dlp_VFSFileOpen(sd,volume,rpath,0x7,&file) < 0) {
		fprintf(stderr,"  Cannot open destination file '%s'.\n",rpath);
		return;
	}

	fd = open(localfile,O_RDONLY);
	if (fd < 0) {
		fprintf(stderr,"  Cannot open local file for reading.\n");
		dlp_VFSFileClose(sd,file);
		return;
	}
#define FBUFSIZ 16384
	filebuffer = (char *)malloc(FBUFSIZ);
	if (NULL == filebuffer) {
		fprintf(stderr,"  Cannot allocate memory for file copy.\n");
		dlp_VFSFileClose(sd,file);
		close(fd);
		return;
	}

	writesize = 0;
	while (writesize >= 0) {
		readsize = read(fd,filebuffer,FBUFSIZ);
		if (readsize <= 0) break;
		offset=0;
		while (readsize > 0) {
			writesize = dlp_VFSFileWrite(sd,file,filebuffer+offset,readsize);
			if (writesize < 0) {
				fprintf(stderr,"  Error while writing file.\n");
				break;
			}
			readsize -= writesize;
			totalsize += writesize;
			offset += writesize;
		}
	}

	free(filebuffer);
	dlp_VFSFileClose(sd,file);
	close(fd);

	printf("(%lu bytes, %ld KiB total)\n",
		(unsigned long)sbuf.st_size, totalsize/1024);
	/* Advancing totalsize already done by write loop.
	totalsize += sbuf.st_size; */
}

static void palm_install(unsigned long int flags,const char *localfile)
{
	palm_connect();
	switch(flags & MEDIA_MASK) {
	case MEDIA_RAM :
	case MEDIA_ROM :
	case MEDIA_FLASH :
		palm_install_internal(localfile);
		break;
	case MEDIA_VFS :
		palm_install_VFS(localfile,vfsdir);
		break;
	default :
		fprintf(stderr,"   ERROR: Unknown media type %lx\n",(flags & MEDIA_MASK));
		break;
	}
}

/***********************************************************************
 *
 * Function:    palm_merge
 *
 * Summary:     Adds the records in <file> into the corresponding
 *		Palm database
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void palm_merge(const char *filename)
{
	struct pi_file *f;

	palm_connect();

	f = pi_file_open(filename);

	if (f == 0) {
		printf("Unable to open '%s'!\n", filename);
		return;
	}

	printf("Merging %s... ", filename);
	fflush(stdout);
	if (pi_file_merge(f, sd, 0, NULL) < 0)
		printf("failed.\n");
	else
		printf("OK\n");
	pi_file_close(f);
	free(f);

	printf("Merge done\n");
}


/***********************************************************************
 *
 * Function:    palm_list_internal
 *
 * Summary:     List the databases found on the Palm device's internal
 * 		memory.
 *
 * Parameters:  Media type
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void palm_list_internal(unsigned long int flags)
{
	int 	i		= 0,
		j,
		dbcount 	= 0;
	struct 	DBInfo info;
	char synclog[68];
	pi_buffer_t *buffer;

	printf("   Reading list of databases in RAM%s...\n",
		(flags & MEDIA_MASK) ? " and ROM" : "");

	buffer = pi_buffer_new(sizeof(struct DBInfo));

	for (;;) {
		if (dlp_ReadDBList
		    (sd, 0, ((flags & MEDIA_MASK) ? 0x40 : 0) | 0x80 | dlpDBListMultiple, i, buffer) < 0)
			break;
		for (j=0; j < (buffer->used / sizeof(struct DBInfo)); j++) {
			memcpy(&info, buffer->data + j * sizeof(struct DBInfo), sizeof(struct DBInfo));
			dbcount++;
			i = info.index + 1;
			printf("   %s\n", info.name);
		}
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
 * Function:    print_volumeinfo
 *
 * Summary:     Show information about the given @p volume; the
 *              VFSInfo structure @p info should already have been
 *              filled in with a call to VFSVolumeInfo, and buf
 *              should contain the volume label (if any).
 *
 * Parameters:  buf         --> volume label. May be NULL.
 *              volume      --> volume ref number.
 *              info        --> volume info
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void print_volumeinfo(const char *buf, long volume, struct VFSInfo *info)
{
	long size_used, size_total;

	/* Only relevant when listing dir / */
	printf("   /slot%d\n",info->slotRefNum);
	if (buf && (buf[0])) printf("   /%s\n",buf);
	printf("      ");
	switch(info->fsType) {
	case pi_mktag('v','f','a','t') :
		printf("VFAT");
		break;
	default:
		printf("<unknown>");
	}
	printf(" filesysytem on ");
	switch(info->mediaType) {
	case pi_mktag('s','d','i','g') :
		printf("SD card");
		break;
	default:
		printf("<unknown>");
	}
	printf("\n");

	if (dlp_VFSVolumeSize(sd,volume,&size_used,&size_total) >= 0) {
		printf("      Used: %ld Total: %ld bytes.\n",size_used,size_total);
	}
}

/***********************************************************************
 *
 * Function:    print_fileinfo
 *
 * Summary:     Show information about the given @p file (which is
 *              assumed to have VFS path @p path).
 *
 * Parameters:  path        --> path to file in VFS volume.
 *              file        --> FileRef for already opened file.
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void print_fileinfo(const char *path, FileRef file)
{
	int size;
	time_t date;
	char *s;
	(void) dlp_VFSFileSize(sd,file,&size);
	(void) dlp_VFSFileGetDate(sd,file,vfsFileDateModified,&date);
	s = ctime(&date);
	s[24]=0;
	printf("   %8d %s  %s\n",size,s,path);
}

/***********************************************************************
 *
 * Function:    print_dir
 *
 * Summary:     Show information about the given @p dir on VFS
 *              volume @p volume. The directory is assumed to have
 *              path @p path on that volume.
 *
 * Parameters:  volume      --> volume ref number.
 *              path        --> path to directory.
 *              dir         --> file ref to already opened dir.
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void print_dir(long volume, const char *path, FileRef dir)
{
	unsigned long it = 0;
	int max = 64;
	struct VFSDirInfo infos[64];
	int i;
	FileRef file;
	char buf[vfsMAXFILENAME];
	int pathlen = strlen(path);

	/* Set up buf so it contains path with trailing / and
	   buflen points to the terminating NUL. */
	if (pathlen<1) {
		printf("   NULL path.\n");
		return;
	}

	memset(buf,0,vfsMAXFILENAME);
	strncpy(buf,path,vfsMAXFILENAME-1);

	if (buf[pathlen-1] != '/') {
		buf[pathlen]='/';
		pathlen++;
	}

	if (pathlen>vfsMAXFILENAME-2) {
		printf("   Path too long.\n");
		return;
	}

	while (dlp_VFSDirEntryEnumerate(sd,dir,&it,&max,infos) >= 0) {
		if (max<1) break;
		for (i = 0; i<max; i++) {
			memset(buf+pathlen,0,vfsMAXFILENAME-pathlen);
			strncpy(buf+pathlen,infos[i].name,vfsMAXFILENAME-pathlen);
			if (dlp_VFSFileOpen(sd,volume,buf,dlpVFSOpenRead,&file) < 0) {
				printf("   %s: No such file or directory.\n",infos[i].name);
			} else {
				print_fileinfo(infos[i].name, file);
				dlp_VFSFileClose(sd,file);
			}
		}
	}
}


/***********************************************************************
 *
 * Function:    findVFSRoot_clumsy
 *
 * Summary:     For internal use only. May contain live weasels.
 *
 * Parameters:  root_component --> root path to search for.
 *              list_root      --> list contents? (otherwise just search)
 *              match          <-> volume matching root_component.
 *
 * Returns:     -2 on VFSVolumeEnumerate error,
 *              -1 if no match was found,
 *              0 if a match was found and @p match is set,
 *              1 if no match but only one VFS volume exists and
 *                match is set.
 *
 ***********************************************************************/
static int findVFSRoot_clumsy(const char *root_component, int list_root, long *match)
{
	int volume_count=16;
	int volumes[16];
	struct VFSInfo info;
	int i;
	int buflen;
	char buf[vfsMAXFILENAME];
	long matched_volume = -1;

	if (dlp_VFSVolumeEnumerate(sd,&volume_count,volumes) < 0) {
		return -2;
	}

	/* Here we scan the "root directory" of the Pilot.  We will fake out
	   a bunch of directories pointing to the various slots on the
	   device. If we're listing, print everything out, otherwise remain
	   silent and just set matched_volume if there's a match in the
	   first filename component. */
	for (i = 0; i<volume_count; ++i) {
		if (dlp_VFSVolumeInfo(sd,volumes[i],&info) < 0) continue;

		buflen=vfsMAXFILENAME;
		buf[0]=0;
		(void) dlp_VFSVolumeGetLabel(sd,volumes[i],&buflen,buf);

		if (!list_root) {
			/* Not listing, so just check matches and continue. */
			if (0 == strcmp(root_component,buf)) {
				matched_volume = volumes[i];
				break;
			}
			/* volume label no longer important, overwrite */
			sprintf(buf,"slot%d",info.slotRefNum);

			if (0 == strcmp(root_component,buf)) {
				matched_volume = volumes[i];
				break;
			}
		}
		else print_volumeinfo(buf,volumes[i],&info);
	}

	if (matched_volume >= 0) {
		*match = matched_volume;
		return 0;
	}

	if ((matched_volume < 0) && (1 == volume_count)) {
		/* Assume that with one slot, just go look there. */
		*match = volumes[0];
		return 1;
	}
	return -1;
}

/***********************************************************************
 *
 * Function:    findVFSPath
 *
 * Summary:     Twofold: if @p verbose is non-zero, list the "fake root"
 *              directory containing all the VFS volumes. Otherwise
 *              search the VFS volumes for @p path. Sets @p volume
 *              equal to the VFS volume matching @p path (if any) and
 *              fills buffer @p rpath with the path to the file relative
 *              to the volume.
 *
 *              Acceptable root components are /slotX/ for slot indicators
 *              or /volumename/ for for identifying VFS volumes by their
 *              volume name. In the special case that there is only one
 *              VFS volume, no root component need be specified, and
 *              "/DCIM/" will map to "/slot1/DCIM/".
 *
 * Parameters:  verbose        --> list root instead of searching.
 *              path           --> path to search for.
 *              volume         <-> volume containing path.
 *              rpath          <-> buffer for path relative to volume.
 *              rpathlen       <-> in: length of buffer; out: length of
 *                                 relative path.
 *
 * Returns:     -2 on VFSVolumeEnumerate error,
 *              -1 if no match was found,
 *              0 if a match was found.
 *
 ***********************************************************************/
static int findVFSPath(int verbose, const char *path, long *volume,
	char *rpath, int *rpathlen)
{
	char *s;
	int r;

	if ((NULL == path) || (NULL == rpath) || (NULL == rpathlen)) return -1;
	if (*rpathlen < strlen(path)) return -1;

	memset(rpath,0,*rpathlen);
	if ('/'==path[0]) strncpy(rpath,path+1,*rpathlen-1);
	else strncpy(rpath,path,*rpathlen-1);
	s = strchr(rpath,'/');
	if (NULL != s) *s=0;


	r = findVFSRoot_clumsy(rpath,verbose,volume);
	if (verbose) return 1;
	if (r < 0) return r;

	if (0 == r) {
		/* Path includes slot/volume label. */
		r = strlen(rpath);
		if ('/'==path[0]) ++r; /* adjust for stripped / */
		memset(rpath,0,*rpathlen);
		strncpy(rpath,path+r,*rpathlen-1);
	} else {
		/* Path without slot label */
		memset(rpath,0,*rpathlen);
		strncpy(rpath,path,*rpathlen-1);
	}

	if (!rpath[0]) {
		rpath[0]='/';
		rpath[1]=0;
	}
	*rpathlen = strlen(rpath);
	return 0;
}

/***********************************************************************
 *
 * Function:    palm_list_VFSDir
 *
 * Summary:     Dispatch listing for given @p path to either
 *              print_dir or print_fileinfo, depending on type.
 *
 * Parameters:  volume      --> volume ref number.
 *              path        --> path to file or directory.
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void palm_list_VFSDir(long volume, const char *path)
{
	FileRef file;
	unsigned long attributes;

	if (dlp_VFSFileOpen(sd,volume,path,dlpVFSOpenRead,&file) < 0) {
		printf("   %s: No such file or directory.\n",path);
		return;
	}

	if (dlp_VFSFileGetAttributes(sd,file,&attributes) < 0) {
		printf("   %s: Cannot get attributes.\n",path);
		return;
	}

	if (vfsFileAttrDirectory == (attributes & vfsFileAttrDirectory)) {
		/* directory */
		print_dir(volume,path,file);
	} else {
		/* file */
		print_fileinfo(path,file);
	}

	(void) dlp_VFSFileClose(sd,file);
}

/***********************************************************************
 *
 * Function:    palm_list_VFS
 *
 * Summary:     Show information about the directory or file specified
 *              in global vfsdir. Listing / will always tell you the
 *              physical VFS slots present; other paths should specify
 *              either a slot as /slotX/ in the path of a volume label.
 *              As a special case, /dir/ is mapped to /slot1/dir/ if
 *              there is only one slot.
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void palm_list_VFS()
{
	char root_component[vfsMAXFILENAME];
	int rootlen = vfsMAXFILENAME;
	int list_root = 0;
	long matched_volume = -1;
	int r;

	/* Listing the root directory / has special handling. Detect that
	   here. */
	if (NULL == vfsdir) vfsdir="/";
	if (0 == strcmp(vfsdir,"/")) list_root = 1;

	/* Find the given directory. Will list the VFS root dir if the
	   requested dir is "/" */
	printf("   Directory of %s...\n",vfsdir);

	r = findVFSPath(list_root,vfsdir,&matched_volume,root_component,&rootlen);
	if (list_root) return;


	/* Failures and "/" mean we can quit now. */
	if (-2 == r) {
		printf("   Not ready reading drive C:\n");
		return;
	}
	if (r < 0) {
		printf("   No such directory, list directory / for slot names.\n");
		return;
	}

	/* printf("   Reading card dir %s on volume %ld\n",root_component,matched_volume); */
	palm_list_VFSDir(matched_volume,root_component);
}

/***********************************************************************
 *
 * Function:    palm_list
 *
 * Summary:     List the databases found on the Palm device.
 *              Dispatches to previous List*() functions.
 *
 * Parameters:  Media type
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void palm_list(unsigned long int flags)
{
	palm_connect();
	switch(flags & MEDIA_MASK) {
	case MEDIA_RAM :
	case MEDIA_ROM :
	case MEDIA_FLASH :
		palm_list_internal(flags);
		break;
	case MEDIA_VFS :
		palm_list_VFS();
		break;
	default :
		fprintf(stderr,"   ERROR: Unknown media type %lx.\n",flags & MEDIA_MASK);
		break;
	}
}


/***********************************************************************
 *
 * Function:    palm_purge
 *
 * Summary:     Purge any deleted data that hasn't been cleaned up
 *              by a sync
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void palm_purge(void)
{
	int 	i	= 0,
		h;
	struct 	DBInfo info;
	pi_buffer_t *buffer;

	palm_connect();

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

	printf("Purge complete.\n");
}

/***********************************************************************
 *
 * Summary:     Argument handling data structures and functions.
 *
 ***********************************************************************/


static void set_operation(int opt, palm_op_t *op, unsigned long int *flags)
{
	switch(opt) {
	case 'b' :
		*op = palm_op_backup;
		*flags = BACKUP;
		break;
	case 'u' :
		*op = palm_op_backup;
		*flags = UPDATE;
		break;
	case 's' :
		*op = palm_op_backup;
		*flags = UPDATE | SYNC;
		break;
	case 'r' :
		*op = palm_op_restore;
		*flags = BACKUP;
		break;
	case 'i' :
		*op = palm_op_install;
		break;
	case 'm' :
		*op = palm_op_merge;
		break;
	case 'f' :
		*op = palm_op_fetch;
		break;
	case 'd' :
		*op = palm_op_delete;
		break;
	case 'l' :
		*op = palm_op_list;
		*flags |= MEDIA_RAM;
		break;
	case 'L' :
		*op = palm_op_list;
		*flags |= MEDIA_ROM;
		break;
	default:
		fprintf(stderr,"Error: unknown operation %c.\n",opt);
		exit(1);
	}
}



int main(int argc, const char *argv[])
{
	int 	optc,		/* switch */
		unsaved 	= 0,
                verbose         = 0;

	const char *archive_dir = NULL, *dirname = NULL, *progname = argv[0];

	unsigned long int sync_flags = palm_op_backup;
	palm_op_t palm_operation     = palm_op_noop;

	const char *gracias = "   Thank you for using pilot-link.\n";

	const char **rargv; /* for scanning remaining arguments */
	int rargc;
	poptContext pc;

	struct poptOption options[] = {
		{"port",     'p', POPT_ARG_STRING, &port, 0, "Use device file <port> to communicate with Palm", "port"},
		{"version",  'v', POPT_ARG_NONE, NULL, 'v', "Show program version information", NULL},
		{"verbose",  'V', POPT_ARG_NONE, &verbose, 0, "Print  verbose  information - normally routine progress messages will be displayed.", NULL},

		/* action indicators that take a <dir> argument */
		{"backup",   'b', POPT_ARG_STRING, &dirname, 'b', "Back up your Palm to <dir>", "dir"},
		{"update",   'u', POPT_ARG_STRING, &dirname, 'u', "Update <dir> with newer Palm data", "dir"},
		{"sync",     's', POPT_ARG_STRING, &dirname, 's', "Same as -u option, but removes local files if data is removed from your Palm", "dir"},
		{"restore",  'r', POPT_ARG_STRING, &dirname, 'r', "Restore backupdir <dir> to your Palm", "dir"},

		/* action indicators that take no argument, but eat the remaining command-line arguments. */
		{"install",  'i', POPT_ARG_NONE, NULL, 'i', "Install local prc, pdb, pqa files to your Palm", "file"},
		{"fetch",    'f', POPT_ARG_NONE, NULL, 'f', "Retrieve [db] from your Palm", "db"},
		{"merge",    'm', POPT_ARG_NONE, NULL, 'm', "Adds the records in <file> into the corresponding Palm database", "file"},
		{"delete",   'd', POPT_ARG_NONE, NULL, 'd', "Delete (permanently) [db] from your Palm", "db"},

		/* action indicators that take no arguments. */
		{"list",     'l', POPT_ARG_NONE, NULL, 'l', "List all application and 3rd party Palm data/apps", NULL},
		{"Listall",  'L', POPT_ARG_NONE, NULL, 'L', "List all data, internal and external on the Palm", NULL},

		/* action indicators that may be mixed in with the others */
		{"Purge",    'P', POPT_ARG_NONE, NULL, 'P', "Purge any deleted data that hasn't been cleaned up", NULL},

		/* modifiers for the various actions */
		{"archive",  'a', POPT_ARG_STRING, &archive_dir, 0, "Modifies -s to archive deleted files in directory <dir>", "dir"},
		{"exclude",  'e', POPT_ARG_STRING, NULL, 'e', "Exclude databases listed in <file> from being included", "file"},
		{"vfsdir",   'D', POPT_ARG_STRING, &vfsdir, 'D', "Modifies all of -lLi to use VFS <dir> instead of internal storage", "dir"},
		{"Flash",    'F', POPT_ARG_NONE, NULL, 'F', "Modifies -b, -u, and -s, to back up non-OS dbs from Flash ROM", NULL},
		{"OsFlash",  'O', POPT_ARG_NONE, NULL, 'O', "Modifies -b, -u, and -s, to back up OS dbs from Flash ROM", NULL},
		{"Illegal",  'I', POPT_ARG_NONE, &unsaved, 0, "Modifies -b, -u, and -s, to back up the illegal database Unsaved Preferences.prc (normally skipped)", NULL},

		/* misc */
		{"exec", 'x', POPT_ARG_STRING, NULL, 'x', "Execute a shell command for intermediate processing", "command"},
		POPT_AUTOHELP
		POPT_TABLEEND
	};
	const char *help_header_text =
		" [-p  <port>] [--help] <options> <actions>\n"
		"   Sync, backup, install, delete and more from your Palm device.\n"
		"   This is the swiss-army-knife of the entire pilot-link suite.\n\n"
		"   Use exactly one of -brsudfimlL; mix in -aexDFIOPV.\n\n";

	const char *listalias = "--List";

	/* Backwards compatibility */
	struct poptAlias listall_alias = {
		"List", '\0', 0, &listalias
	};

	pc = poptGetContext("pilot-xfer", argc, argv, options, 0);

	poptSetOtherOptionHelp(pc, help_header_text);
	poptAddAlias(pc, listall_alias, 0);

	poptSetOtherOptionHelp(pc, help_header_text);
	while ((optc = poptGetNextOpt(pc)) >= 0) {
		switch (optc) {
		case 'v':
			print_splash(progname);
			return 0;

		/* Actions with a dir argument */

		case 'b':
		case 'u':
		case 's':
		case 'r':
			if (palm_op_noop != palm_operation) {
				fprintf(stderr,"Error: specify only one of -brsuimfdlL.\n");
				return 1;
			}
			set_operation(optc,&palm_operation,&sync_flags);
			if (verbose)
				printf("Option -%c with value: %s\n", optc, dirname);
			break;


		/* Actions without an argument */

		case 'i':
		case 'm':
		case 'f':
		case 'd':
		case 'l':
		case 'L':
			if (palm_op_noop != palm_operation) {
				fprintf(stderr,"Error: specify only one of -brsuimfdlL.\n");
				return 1;
			}
			set_operation(optc,&palm_operation,&sync_flags);
			break;

		/* Modifiers */

		case 'P':
			sync_flags |= PURGE;
			break;
		case 'D':
			sync_flags |= MEDIA_VFS;
			break;
		case 'F':
			sync_flags |= MEDIA_FLASH;
			break;
		case 'O':
			sync_flags |= MEDIA_ROM;
			break;



		/* Misc */

		case 'e':
			make_excludelist(poptGetOptArg(pc));
			break;
		case 'x':
			if (system(poptGetOptArg(pc))) {
				fprintf(stderr,"system() failed, aborting.\n");
				return -1;
			}
			break;
		default:
			printf("got option %d, arg %s\n", optc, poptGetOptArg(pc));
			break;
		}
	}

	if (optc < -1) {
		/* an error occurred during option processing */
		fprintf(stderr, "%s: %s\n",
			poptBadOption(pc, POPT_BADOPTION_NOALIAS),
			poptStrerror(optc));
		return 1;
	}

	/* collect and count remaining arguments */
	rargc = 0;
	rargv = poptGetArgs(pc);
	if (rargv) {
		const char **s = rargv;
		while (*s) {
			if (verbose) printf("Remaining argument: %s\n",*s);
			s++;
			rargc++;
		}
	}

	/* sanity checking */
	switch(sync_flags & MEDIA_MASK) {
	case MEDIA_RAM:
	case MEDIA_ROM:
	case MEDIA_FLASH:
	case MEDIA_VFS:
		/* These are all clearly OK */
		break;
	default:
		/* VFS can be combined with -l or -L, both of which set the
		   media type (a bad idea, IMO), so let VFS override. */
		if ((palm_op_list == palm_operation) &&
			(sync_flags & MEDIA_VFS)) {
			sync_flags &= ~(MEDIA_MASK);
			sync_flags |= MEDIA_VFS;
		}
		else {
			char media_names[64];
			memset(media_names,0,sizeof(media_names));
			if (sync_flags & MEDIA_ROM) {
				strncat(media_names,media_name(MEDIA_ROM),sizeof(media_names));
				strncat(media_names,", ",sizeof(media_names));
			}
			if (sync_flags & MEDIA_FLASH) {
				strncat(media_names,media_name(MEDIA_FLASH),sizeof(media_names));
				strncat(media_names,", ",sizeof(media_names));
			}
			if (sync_flags & MEDIA_VFS) {
				strncat(media_names,media_name(MEDIA_VFS),sizeof(media_names));
			}

			fprintf(stderr,"   ERROR: Combining media types %s not supported.\n",media_names);
			return 1;
		}
		break;
	}

	switch(palm_operation) {
		struct stat sbuf;
	case palm_op_backup:
	case palm_op_restore:
		if (stat (dirname, &sbuf) == 0) {
			if (!S_ISDIR (sbuf.st_mode)) {
				fprintf(stderr, "   ERROR: '%s' is not a directory or does not exist.\n"
					"   Please supply a directory name when performing a "
					"backup or restore and try again.\n\n",
					dirname);
				fprintf(stderr,gracias);
				return 1;
			}
		}
		/* FALLTHRU */
	case palm_op_list:
		if (rargc > 0) {
			fprintf(stderr,"   ERROR: Do not pass additional arguments to -busrlL.\n");
			fprintf(stderr,gracias);
			return 1;
		}
		break;
	case palm_op_noop:
		fprintf(stderr,"   ERROR: Must specify one of -bursimfdlL.\n");
		fprintf(stderr,gracias);
		return 1;
		break;
	case palm_op_install:
	case palm_op_merge:
	case palm_op_fetch:
	case palm_op_delete:
		if (rargc < 1) {
			fprintf(stderr,"   ERROR: -imfd require additional arguments.\n");
			return 1;
		}
		if ((MEDIA_VFS == (sync_flags & MEDIA_VFS)) &&
			(palm_op_merge == palm_operation)) {
			fprintf(stderr,"   ERROR: cannot merge to VFS.\n");
			return 1;
		}
		break;
	}

	/* actual operation */
	palm_connect();
	switch(palm_operation) {
	case palm_op_noop: /* handled above */
		exit(1);
		break;
	case palm_op_backup:
		palm_backup(dirname, sync_flags, unsaved, archive_dir);
		break;
	case palm_op_restore:
		palm_restore(dirname);
		break;
	case palm_op_merge:
	case palm_op_install:
	case palm_op_fetch:
	case palm_op_delete:
		while (rargv && *rargv) {
			switch(palm_operation) {
			case palm_op_merge: palm_merge(*rargv); break;
			case palm_op_fetch: palm_fetch(*rargv); break;
			case palm_op_delete: palm_delete(*rargv); break;
			case palm_op_install: palm_install(sync_flags,*rargv); break;
			default: /* impossible */ break;
			}
			rargv++;
		}
		break;
	case palm_op_list:
		palm_list(sync_flags);
		break;
	}

	if (sync_flags & PURGE) palm_purge();

	pi_close(sd);
	puts(gracias);
	return 0;
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
