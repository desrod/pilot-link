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

#include "pi-debug.h"
#include "pi-socket.h"
#include "pi-file.h"
#include "pi-header.h"
#include "pi-util.h"
#include "pi-error.h"

#include "pi-userland.h"

/* unsigned char typedef byte; */
typedef unsigned char byte;

typedef struct {
	byte	data[4];
	char	attr;
	byte	id[3];
} recInfo_t;

typedef struct {
	char	name[32];
	byte	attr[2];
	byte	version[2];
	byte	cdate[4];
	byte	mdate[4];
	byte	backupdate[4];
	byte	modno[4];
	byte	appinfo[4];
	byte	sortinfo[4];
	char	dbType[4];
	char	dbCreator[4];
	byte	seed[4];

	byte	nextRecList[4];
	char	nRec[2];
} pdb_t;

typedef enum {
	palm_op_noop = 0,
	palm_op_restore = 277, /* keep rest out of ASCII range */
	palm_op_backup,
	palm_op_update,
	palm_op_sync,
	palm_op_install,
	palm_op_merge,
	palm_op_fetch,
	palm_op_delete,
	palm_op_list,
	palm_op_cardinfo
} palm_op_t;

/* Flags specifying various bits of behavior. Should be
   different from the palm_ops and non-ASCII if they can
   be set explicitly from the command-line. */

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




int		sd	= -1;
char    *vfsdir = NULL;

#define MAXEXCLUDE 100
char	*exclude[MAXEXCLUDE];
int		numexclude = 0;



static int findVFSPath(const char *path, long *volume, char *rpath,
		int *rpathlen);


const char
*media_name(int m)
{
	switch (m)
	{
		case MEDIA_RAM:
			return "RAM";
		case MEDIA_ROM:
			return "OS";
		case MEDIA_FLASH:
			return "Flash";
		case MEDIA_VFS:
			return "VFS";
		default:
			return NULL;
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
static void
make_excludelist(const char *efile)
{
	char	temp[1024];
	FILE	*f = fopen(efile, "r");

	if (!f)
	{
		printf("   Unable to open exclude list file '%s'.\n", efile);
		exit(EXIT_FAILURE);
	}

	while ((fgets(temp, sizeof(temp), f)) != NULL)
	{
		temp[strlen(temp) - 1] = '\0';
		printf("Now excluding: %s\n", temp);
		exclude[numexclude++] = strdup(temp);
		if (numexclude == MAXEXCLUDE)
		{
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
static void
protect_name(char *d, const char *s)
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
 * Function:    list_remove
 *
 * Summary:     Remove the excluded files from the op list
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void
list_remove(char *name, char **list, int max)
{
	int		i;

	for (i = 0; i < max; i++)
	{
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

	union		buf;
	int			n;
	static long	special_cases[] = {
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
static void
palm_backup(const char *dirname, unsigned long int flags, int unsaved,
		const char *archive_dir)
{

	int			i			= 0,
				ofile_len	= 0,
				ofile_total	= 0,
				filecount	= 1,	/* File counts start at 1, of course */
				failed		= 0,
				skipped		= 0;
	static int	totalsize;
	char		**orig_files = NULL,
				*name,
				synclog[70];

	const char	*synctext = (flags & UPDATE) ? "Synchronizing" : "Backing up";
	DIR			*dir;
	pi_buffer_t	*buffer;

	/* Check if the directory exists before writing to it. If it doesn't
	   exist as a directory, and it isn't a file, create it. */
	if (access(dirname, F_OK) == -1)
	{
		fprintf(stderr, "   Creating directory '%s'...\n", dirname);
		mkdir(dirname, 0700);
	} else if (access(dirname, R_OK|W_OK|X_OK) != 0)
	{
		fprintf(stderr, "\n");
		fprintf(stderr, "   ERROR: %s\n", strerror(errno));
		fprintf(stderr, "   Please check ownership and permissions"
				" on %s.\n\n", dirname);
		return;
	} else if (flags & UPDATE)
	{
		if ((dir = opendir(dirname)) == NULL)
		{
			fprintf(stderr, "\n");
			fprintf(stderr, "   ERROR: %s\n", strerror(errno));
			fprintf(stderr, "   Does the directory %s exist?\n\n",
					dirname);
			return;
		} else {
			struct dirent	*dirent;

			while ((dirent = readdir(dir)))
			{
				int		dirnamelen;

				dirnamelen = strlen(dirname);

				if (dirent->d_name[0] == '.')
					continue;

				if (ofile_total >= ofile_len)
				{
					ofile_len += 256;
					orig_files = realloc(orig_files,
							(sizeof (char *)) * ofile_len);
				}
				name = malloc(dirnamelen + strlen (dirent->d_name) + 2);
				if (name == NULL)
				{
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

	for (;;)
	{
		struct DBInfo	info;
		struct pi_file	*f;
		struct utimbuf	times;
		int				skip	= 0;
		int				excl	= 0;
		struct stat		sbuf;
		char			crid[5];

		if (!pi_socket_connected(sd))
		{
			printf("\n   Connection broken - Exiting. All data was not backed up\n");
			exit(EXIT_FAILURE);
		}

		if (dlp_ReadDBList(sd, 0, ((flags & MEDIA_MASK)
						? dlpDBListROM : dlpDBListRAM), i, buffer) < 0)
			break;

		memcpy(&info, buffer->data, sizeof(struct DBInfo));
		i = info.index + 1;

		pi_untag(crid,info.creator);

		if (dlp_OpenConduit(sd) < 0)
		{
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

		if (palm_creator(info.creator))
		{
			printf("   [-][skip][%s] Skipping OS file '%s'.\n",
					crid, info.name);
			continue;
		}

		if (info.flags & dlpDBFlagResource)
		{
			strcat(name, ".prc");
		} else if ((info.flags & dlpDBFlagLaunchable) &&
				   info.type == pi_mktag('p','q','a',' '))
		{
			strcat(name, ".pqa");
		} else {
			strcat(name, ".pdb");
		}

		for (excl = 0; excl < numexclude; excl++)
		{
			if (strcmp(exclude[excl], info.name) == 0)
			{
				printf("   [-][excl] Excluding '%s'...\n", name);
				list_remove(name, orig_files, ofile_total);
				skip = 1;
			}
		}

		if (info.creator == pi_mktag('a', '6', '8', 'k'))
		{
			printf("   [-][a68k][PACE] Skipping '%s'\n", info.name);
			skipped++;
			continue;
		}

		if (skip == 1)
			continue;

		if (!unsaved
				&& strcmp(info.name, "Unsaved Preferences") == 0)
		{
			printf("   [-][unsv] Skipping '%s'\n", info.name);
			continue;
		}

			list_remove(name, orig_files, ofile_total);
		if ((0 == stat(name, &sbuf)) && ((flags & UPDATE) == UPDATE))
		{
			if (info.modifyDate == sbuf.st_mtime)
			{
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

		if (f == 0)
		{
			printf("\nFailed, unable to create file.\n");
			break;
		} else if (pi_file_retrieve(f, sd, 0, NULL) < 0)
		{
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

		times.actime	= info.createDate;
		times.modtime	= info.modifyDate;
		utime(name, &times);
	}
	pi_buffer_free(buffer);

	if (orig_files)
	{
		int     i = 0;
		int		dirname_len = strlen(dirname);

		for (i = 0; i < ofile_total; i++)
		{
			if (orig_files[i] != NULL)
			{
				if (flags & SYNC)
				{
					if (archive_dir)
					{
						printf("Archiving '%s'", orig_files[i]);

						sprintf(name, "%s/%s", archive_dir,
							&orig_files[i] [dirname_len + 1]);

						if (rename (orig_files[i], name) != 0)
						{
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
			(filecount ? filecount - 1 : 0),
			skipped, failed, (failed == 1) ? "" : "s");

	sprintf(synclog, "%d files successfully backed up.\n\n"
			"Thank you for using pilot-link.", filecount - 1);
	dlp_AddSyncLogEntry(sd, synclog);
}

/***********************************************************************
 *
 * Function:    fetch_progress
 *
 * Summary:     Sample progress output for the pi_file_retrieve*
 *              functions.
 *
 * Parameters:  see pi_file_install docs.
 *
 * Returns:     PI_TRANSFER_CONTINUE or PI_TRANSFER_STOP if it seems
 *              that the Palm has canceled on us.
 *
 ***********************************************************************/

static int fetch_progress(int sd, pi_file_t *pf, int total, int xfer, int records)
{
	const char *filename = NULL;

	if (pf && pf->file_name) {
		filename = pf->file_name;
	}
	if (!filename) {
		filename="<unnamed>";
	}

	fprintf(stdout,"\r   Fetching '%s' ... (%d bytes)",filename,xfer);
	fflush(stdout);

	if (!pi_socket_connected(sd)) {
		return PI_TRANSFER_STOP;
	} else {
		return PI_TRANSFER_CONTINUE;
	}
}


/***********************************************************************
 *
 * Function:    palm_fetch_internal
 *
 * Summary:     Grab a file from the Palm, write to disk
 *
 * Parameters:  dbname --> name of database on Palm to fetch.
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void
palm_fetch_internal(const char *dbname)
{
	struct DBInfo	info;
	char			name[256],
					synclog[512];
	struct pi_file	*f;

	if (access(dbname, F_OK) == 0 && access(dbname, R_OK|W_OK) != 0)
	{
		fprintf(stderr, "\n   Unable to write to %s, check "
				"ownership and permissions.\n\n", dbname);
		exit(EXIT_FAILURE);
	}

	printf("   Parsing list of files from handheld... ");
	fflush(stdout);
	if (dlp_FindDBInfo(sd, 0, 0, dbname, 0, 0, &info) < 0)
	{
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
	if (strcmp(name, "Graffiti ShortCuts ") == 0)
		strncpy(name, "Graffiti ShortCuts", sizeof(name));

	if (info.flags & dlpDBFlagResource)
	{
		strcat(name, ".prc");
	} else if ((info.flags & dlpDBFlagLaunchable) &&
			   info.type == pi_mktag('p','q','a',' ')) 
	{
		strcat(name, ".pqa");
	} else {
		strcat(name, ".pdb");
	}

	printf("   Fetching %s... ", name);
	fflush(stdout);

	info.flags &= 0x2fd;

	/* Write the file records to disk as 'dbname'	*/
	f = pi_file_create(name, &info);
	if (f == 0)
	{
		printf("Failed, unable to create file.\n");
		return;
	} else if (pi_file_retrieve(f, sd, 0, plu_quiet ? NULL : fetch_progress) < 0)
	{
		printf("Failed, unable to fetch database from the Palm.\n");
	}

	printf("complete.\n\n");

	snprintf(synclog, sizeof(synclog)-1,
			"File '%s' successfully fetched.\n\n"
			"Thank you for using pilot-link.", name);
	dlp_AddSyncLogEntry(sd, synclog);

	pi_file_close(f);
}

static int
pi_file_retrieve_VFS(const int fd, const char *basename, const int socket, const char *vfspath, progress_func f)
{
	long         volume = -1;
	char         rpath[vfsMAXFILENAME];
	int          rpathlen = vfsMAXFILENAME;
	FileRef      file;
	long         attributes;
	pi_buffer_t  *buffer;
	ssize_t      readsize,writesize;
	int          filesize;
	int          original_filesize;
	int          written_so_far;
	pi_file_t    fake_file;

	enum { bad_parameters=-1,
	       cancel=-2,
	       bad_vfs_path=-3,
	       bad_local_file=-4,
	       insufficient_space=-5,
	       internal_=-6
	} ;


	if (findVFSPath(vfspath,&volume,rpath,&rpathlen) < 0)
	{
		fprintf(stderr,"\n   VFS path '%s' does not exist.\n\n", vfspath);
		return bad_vfs_path;
	}

	if (rpath[rpathlen-1] != '/') {
		rpath[rpathlen] = '/';
		rpathlen++;
	}

	strncat(rpath,basename,sizeof(rpath)-rpathlen-1);
	rpathlen=strlen(rpath);

	/* fprintf(stderr,"* Reading %s on volume %ld\n",rpath,volume); */

	if (dlp_VFSFileOpen(socket,volume,rpath,dlpVFSOpenRead,&file) < 0) {
		fprintf(stderr,"\n   Cannot open file '%s' on VFS.\n",rpath);
		return bad_vfs_path;
	}

	if (dlp_VFSFileGetAttributes(socket,file,&attributes) < 0)
	{
		fprintf(stderr,"   Could not get attributes of VFS file.\n");
		(void) dlp_VFSFileClose(socket,file);
		return bad_vfs_path;
	}

	if (attributes & vfsFileAttrDirectory)
	{
		/* Clear case for later feature. */
		fprintf(stderr,"   Cannot retrieve a directory.\n");
		dlp_VFSFileClose(socket,file);
		return bad_vfs_path;
	}

	dlp_VFSFileSize(socket,file,&filesize);
	original_filesize = filesize;

#define FBUFSIZ 65536
	memset(&fake_file,0,sizeof(fake_file));
	fake_file.file_name = /* const_cast */ (char *) basename;

	buffer = pi_buffer_new(FBUFSIZ);
	readsize = 0;
	written_so_far = 0;
	while ((filesize > 0) && (readsize >= 0))
	{
		int offset;

		pi_buffer_clear(buffer);
		readsize = dlp_VFSFileRead(socket,file,buffer,
			( filesize > FBUFSIZ ? FBUFSIZ : filesize));

		/* fprintf(stderr,"*  Read %ld bytes.\n",readsize); */

		if (readsize <= 0) break;
		filesize -= readsize;
		offset=0;
		while (readsize > 0)
		{
			writesize = write(fd,buffer->data+offset,readsize);
			if (writesize < 0)
			{
				fprintf(stderr,"   Error while writing file.\n");
				goto cleanup;
			}

			written_so_far += writesize;

			if ((filesize > 0) || (readsize > 0)) {
				if (f && (f(socket,&fake_file,original_filesize,
					(int)written_so_far,0) == PI_TRANSFER_STOP)) {
					written_so_far = cancel;
					pi_set_error(socket,PI_ERR_FILE_ABORTED);
					goto cleanup;
				}
			}

			readsize -= writesize;
			offset += writesize;
		}
	}
cleanup:
	pi_buffer_free(buffer);
#undef FBUFSIZ
	dlp_VFSFileClose(socket,file);

	return written_so_far;
}

static void
palm_fetch_VFS(const char *dbname, const char *vfspath)
{
	static unsigned long totalsize = 0;
	int fd = -1;
	int filesize;

	if (NULL == vfspath)
	{
		/* how the heck did we get here then? */
		fprintf(stderr,"\n   No VFS path given.\n");
		return;
	}

	if (access(dbname, F_OK) == 0 && access(dbname, R_OK|W_OK) != 0)
	{
		fprintf(stderr, "\n   Unable to write to %s, check "
				"ownership and permissions.\n\n", dbname);
		exit(EXIT_FAILURE);
	}

	if (dlp_OpenConduit(sd) < 0)
	{
		fprintf(stderr, "\nExiting on cancel, some files were not"
				"fetched.\n\n");
		exit(EXIT_FAILURE);
	}

	fprintf(stdout, "   Fetching '%s'... ", dbname);
	fflush(stdout);

	/* Calculate basename, perhaps? */
	fd = open(dbname,O_WRONLY | O_CREAT | O_TRUNC,0666);
	if (fd < 0) {
		fprintf(stderr,"\n   Cannot open local file for '%s'.\n",dbname);
		return;
	}

	if ((filesize = pi_file_retrieve_VFS(fd,dbname,sd,vfspath,plu_quiet ? NULL : fetch_progress)) < 0) {
		fprintf(stderr,"   ERROR: pi_file_retrieve_VFS failed.\n");
		/* is the semantics of unlink-open-file standard? */
		unlink(dbname);
	} else {
		totalsize += filesize;
		printf("   %ld KiB total.\n", totalsize/1024);
		fflush(stdout);
	}
	close(fd);
}

static void
palm_fetch(unsigned long int flags,const char *dbname)
{
	switch(flags & MEDIA_MASK)
	{
	case MEDIA_RAM:
	case MEDIA_ROM:
	case MEDIA_FLASH:
		palm_fetch_internal(dbname);
		break;
	case MEDIA_VFS:
		palm_fetch_VFS(dbname,vfsdir);
		break;
	default:
		fprintf(stderr,"   ERROR: Unknown media type %lx\n",
				(flags & MEDIA_MASK));
		break;
	}
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
	struct DBInfo	info;

	dlp_FindDBInfo(sd, 0, 0, dbname, 0, 0, &info);

	printf("Deleting '%s'... ", dbname);
	if (dlp_DeleteDB(sd, 0, dbname) >= 0)
	{
		if (info.type == pi_mktag('b', 'o', 'o', 't'))
		{
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
	int				flags,
					maxblock;
	char			name[256];
	unsigned long	creator, type;
};

static int
compare(struct db *d1, struct db *d2)
{
	/* types of 'appl' sort later then other types */
	if (d1->creator == d2->creator)
		if (d1->type != d2->type)
		{
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
static void
palm_restore(const char *dirname)
{
	int				dbcount		= 0,
					i,
					j,
					max,
					save_errno	= errno;
	size_t			size;
	DIR				*dir;
	struct dirent	*dirent;
	struct DBInfo	info;
	struct db		**db		= NULL;
	struct pi_file	*f;
	struct stat		sbuf;

	struct  CardInfo Card;

	Card.card = -1;
	Card.more = 1;

	if ((dir = opendir(dirname)) == NULL)
	{
		fprintf(stderr, "\n");
		perror("   ERROR");
		fprintf(stderr, "   opendir() failed. Cannot open directory %s.\n",
				dirname);
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

	if (!db)
	{
		printf("Unable to allocate memory for directory entry table\n");
		exit(EXIT_FAILURE);
	}

	dbcount = 0;
	rewinddir(dir);

	while ((dirent = readdir(dir)) != NULL)
	{
		if (dirent->d_name[0] == '.')
			continue;

		db[dbcount] = (struct db *) malloc(sizeof(struct db));

		sprintf(db[dbcount]->name, "%s/%s", dirname,
			dirent->d_name);

		f = pi_file_open(db[dbcount]->name);
		if (f == 0)
		{
			printf("Unable to open '%s'!\n",
				   db[dbcount]->name);
			break;
		}

		pi_file_get_info(f, &info);

		db[dbcount]->creator	= info.creator;
		db[dbcount]->type		= info.type;
		db[dbcount]->flags		= info.flags;
		db[dbcount]->maxblock	= 0;

		pi_file_get_entries(f, &max);

		for (i = 0; i < max; i++)
		{
			if (info.flags & dlpDBFlagResource)
			{
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

	for (i = 0; i < dbcount; i++)
	{
		for (j = i + 1; j < dbcount; j++)
		{
			if (compare(db[i], db[j]) > 0)
			{
				struct db *temp = db[i];

				db[i] = db[j];
				db[j] = temp;
			}
		}
	}

	for (i = 0; i < dbcount; i++)
	{

		f = pi_file_open(db[i]->name);
		if (f == 0) {
			printf("Unable to open '%s'!\n", db[i]->name);
			break;
		}
		printf("Restoring %s... ", db[i]->name);
		fflush(stdout);

		stat(db[i]->name, &sbuf);

		while (Card.more)
		{
			if (dlp_ReadStorageInfo(sd, Card.card + 1, &Card) < 0)
				break;
		}

		if (sbuf.st_size > Card.ramFree)
		{
			fprintf(stderr, "\n\n");
			fprintf(stderr, "   Insufficient space to install this file on your Palm.\n");
			fprintf(stderr, "   We needed %lu and only had %lu available..\n\n",
				(unsigned long)sbuf.st_size, Card.ramFree);
			exit(EXIT_FAILURE);
		}

		if (pi_file_install(f, sd, 0, NULL) < 0)
		{
			printf("failed.\n");
		} else {
			printf("OK\n");
		}

		pi_file_close(f);
	}

	for (i = 0; i < dbcount; i++)
	{
		free(db[i]);
	}
	free(db);

	printf("Restore done\n");
}


/***********************************************************************
 *
 * Function:    install_progress
 *
 * Summary:     Sample progress output for the pi_file_install*
 *              functions.
 *
 * Parameters:  see pi_file_install docs.
 *
 * Returns:     PI_TRANSFER_CONTINUE or PI_TRANSFER_STOP if it seems
 *              that the Palm has canceled on us.
 *
 ***********************************************************************/

static int install_progress(int sd, pi_file_t *pf, int total, int xfer, int records)
{
	const char *filename = NULL;

	if (pf && pf->file_name) {
		filename = pf->file_name;
	}
	if (!filename) {
		filename="<unnamed>";
	}

	fprintf(stdout,"\r   Installing '%s' ... (%d bytes)",filename,xfer);
	fflush(stdout);

	if (!pi_socket_connected(sd)) {
		return PI_TRANSFER_STOP;
	} else {
		return PI_TRANSFER_CONTINUE;
	}
}

/***********************************************************************
 *
 * Function:    pi_file_install_VFS
 *
 * Summary:     Push file(s) to the Palm's VFS (parameters intentionally
 *              similar to pi_file_install).
 *
 * Parameters:  fd       --> open file descriptor for file
 *              basename --> filename or description of file
 *              socket   --> sd, connection to Palm
 *              vfspath  --> target in VFS, may be dir or filename
 *              f        --> progress function, in the style of pi_file_install
 *
 * Returns:     -1 on bad parameters
 *              -2 on cancelled sync
 *              -3 on bad vfs path
 *              -4 on bad local file
 *              -5 on insufficient VFS space for the file
 *              -6 on memory allocation error
 *              >=0 if all went well (size of installed file)
 *
 * Note:        Should probably return an ssize_t and refuse to do files >
 *              2Gb, due to signedness.
 *
 ***********************************************************************/
static int pi_file_install_VFS(const int fd, const char *basename, const int socket, const char *vfspath, progress_func f)
{
	enum { bad_parameters=-1,
	       cancel=-2,
	       bad_vfs_path=-3,
	       bad_local_file=-4,
	       insufficient_space=-5,
	       internal_=-6
	} ;

	char        rpath[vfsMAXFILENAME];
	int         rpathlen = vfsMAXFILENAME;
	FileRef     file;
	long        attributes;
	char        *filebuffer = NULL;
	long        volume = -1;
	long        used,
	            total,
	            freespace;
	int
	            writesize,
	            offset;
	size_t      readsize;
	size_t      written_so_far = 0;
	enum { no_path=0, appended_filename=1, retried=2, done=3 } path_steps;
	struct stat sbuf;
	pi_file_t   fake_file;

	if (fstat(fd,&sbuf) < 0) {
		fprintf(stderr,"   ERROR: Cannot stat '%s'.\n",basename);
		return bad_local_file;
	}

	if (findVFSPath(vfspath,&volume,rpath,&rpathlen) < 0)
	{
		fprintf(stderr,"\n   VFS path '%s' does not exist.\n\n", vfspath);
		return bad_vfs_path;
	}

	if (dlp_VFSVolumeSize(socket,volume,&used,&total)<0)
	{
		fprintf(stderr,"   Unable to get volume size.\n");
		return bad_vfs_path;
	}

	/* Calculate free space but leave last 64k free on card */
	freespace  = total - used - 65536 ;

	if (sbuf.st_size > freespace)
	{
		fprintf(stderr, "\n\n");
		fprintf(stderr, "   Insufficient space to install this file on your Palm.\n");
		fprintf(stderr, "   We needed %lu and only had %lu available..\n\n",
				(unsigned long)sbuf.st_size, freespace);
		return insufficient_space;
	}
#define APPEND_BASENAME path_steps-=1; \
			if (rpath[rpathlen-1] != '/') { \
				rpath[rpathlen++]='/'; \
					rpath[rpathlen]=0; \
			} \
			strncat(rpath,basename,vfsMAXFILENAME-rpathlen-1); \
			rpathlen = strlen(rpath);

	path_steps = no_path;

	while (path_steps<retried)
	{
		/* Don't retry by default. APPEND_BASENAME changes
		   the file being tries, so it decrements fd again.
		   Because we add _two_ here, (two steps fwd, one step back)
		   we try at most twice anyway.
		      no_path->retried
		      appended_filename -> done
		   Note that APPEND_BASENAME takes one off, so
		      retried->appended_basename
		*/
		path_steps+=2;

		if (dlp_VFSFileOpen(socket,volume,rpath,dlpVFSOpenRead,&file) < 0)
		{
			/* Target doesn't exist. If it ends with a /, try to
			create the directory and then act as if the existing
			directory was given as target. If it doesn't, carry
			on, it's a regular file to create. */
			if ('/' == rpath[rpathlen-1])
			{
				/* directory, doesn't exist. Don't try to mkdir /. */
				if ((rpathlen > 1)
						&& (dlp_VFSDirCreate(socket,volume,rpath) < 0))
				{
					fprintf(stderr,"   Could not create destination directory.\n");
					return bad_vfs_path;
				}
				APPEND_BASENAME
			}
			if (dlp_VFSFileCreate(socket,volume,rpath) < 0)
			{
				fprintf(stderr,"   Cannot create destination file '%s'.\n",
						rpath);
				return bad_vfs_path;
			}
		}
		else
		{
			/* Exists, and may be a directory, or a filename. If it's
			a filename, that's fine as long as we're installing
			just a single file. */
			if (dlp_VFSFileGetAttributes(socket,file,&attributes) < 0)
			{
				fprintf(stderr,"   Could not get attributes for destination.\n");
				(void) dlp_VFSFileClose(socket,file);
				return bad_vfs_path;
			}

			if (attributes & vfsFileAttrDirectory)
			{
				APPEND_BASENAME
				dlp_VFSFileClose(socket,file);
				/* Now for sure it's a filename in a directory. */
			} else {
				dlp_VFSFileClose(socket,file);
				if ('/' == rpath[rpathlen-1])
				{
					/* was expecting a directory */
					fprintf(stderr,"   Destination is not a directory.\n");
					return bad_vfs_path;
				}
			}
		}
	}
#undef APPEND_BASENAME

	if (dlp_VFSFileOpen(socket,volume,rpath,0x7,&file) < 0)
	{
		fprintf(stderr,"   Cannot open destination file '%s'.\n",rpath);
		return bad_vfs_path;
	}

#define FBUFSIZ 65536
	filebuffer = (char *)malloc(FBUFSIZ);
	if (NULL == filebuffer)
	{
		fprintf(stderr,"   Cannot allocate memory for file copy.\n");
		dlp_VFSFileClose(socket,file);
		close(fd);
		return internal_;
	}

	memset(&fake_file,0,sizeof(fake_file));

	fake_file.file_name = /* const_cast */ (char *) basename;


	writesize = 0;
	written_so_far = 0;
	while (writesize >= 0)
	{
		readsize = read(fd,filebuffer,FBUFSIZ);
		if (readsize <= 0) break;
		offset=0;
		while (readsize > 0)
		{
			writesize = dlp_VFSFileWrite(socket,file,filebuffer+offset,readsize);
			if (writesize < 0)
			{
				fprintf(stderr,"   Error while writing file.\n");
				break;
			}
			readsize -= writesize;
			offset += writesize;
			written_so_far += writesize;

			if ((writesize>0) || (readsize > 0)) {
				if (f && (f(socket,&fake_file,(int)sbuf.st_size,
					(int)written_so_far,0) == PI_TRANSFER_STOP)) {
					sbuf.st_size = 0;
					pi_set_error(socket,PI_ERR_FILE_ABORTED);
					goto cleanup;
				}
			}
		}
	}

cleanup:
	free(filebuffer);
	dlp_VFSFileClose(socket,file);
	close(fd);


	return sbuf.st_size;
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
	static unsigned long totalsize;
	struct pi_file	*f;
	struct stat		sbuf;
	struct CardInfo	Card;
	const char *basename = strrchr(filename,'/');
	if (basename) {
		basename = basename+1;
	} else {
		basename = filename;
	}

	Card.card = -1;
	Card.more = 1;

	f = pi_file_open(filename);

	if (f == NULL)
	{
		fprintf(stderr,"   ERROR: Unable to open '%s'!\n", filename);
		return;
	}
	if (f->file_name == NULL) {
		/* pi_file_open doesn't set the filename (or not to anything nice),
		   so set it instead; need strdup because the file struct owns the
		   string. */
		f->file_name = strdup(basename);
	}

	if (dlp_OpenConduit(sd) < 0)
	{
		fprintf(stderr, "   CANCEL: Exiting on cancel, some files were not "
				"installed\n\n");
		exit(EXIT_FAILURE);
	}

	fprintf(stdout, "   Installing '%s'... ", basename);
	fflush(stdout);

	stat(filename, &sbuf);

	while (Card.more)
	{
		if (dlp_ReadStorageInfo(sd, Card.card + 1, &Card) < 0)
			break;
	}

	if (sbuf.st_size > Card.ramFree)
	{
		fprintf(stderr, "   ERROR: Insufficient space to install this file on your Palm.\n");
		fprintf(stderr, "          We needed %lu and only had %lu available..\n\n",
			(unsigned long)sbuf.st_size, Card.ramFree);
		return;
	}

	/* TODO: shouldn't this use Card.card? If we're looking for _a_
	   card that can hold the file, shouldn't we check in the while
	   loop above?  */
	if (pi_file_install(f, sd, 0, plu_quiet ? NULL : install_progress) < 0) {
		/* TODO: Does pi_file_install print a diagnostic? */
		fprintf(stderr,
				"\n   ERROR: pi_file_install failed "
				"(%i, PalmOS 0x%04x).\n",
				pi_error(sd), pi_palmos_error(sd));
	} else {
		totalsize += sbuf.st_size;
		printf("   %ld KiB total.\n", totalsize/1024);
		fflush(stdout);
	}

	pi_file_close(f);
}

/***********************************************************************
 *
 * Function:    palm_install_VFS
 *
 * Summary:     Push file to the Palm's VFS.
 *
 * Parameters:  localfile --> local filesystem path for file to install.
 *              vfspath   --> target for file (may be dir or filename).
 *
 * Returns:     -1 on bad parameters
 *              -2 on cancelled sync
 *              -3 on bad vfs path
 *              -4 on bad local file
 *              -5 on insufficient VFS space for the file
 *              -6 on memory allocation error
 *              0 if all went well
 *
 ***********************************************************************/
static int
palm_install_VFS(const char *localfile, const char *vfspath)
{
	enum { bad_parameters=-1,
	       cancel=-2,
	       bad_vfs_path=-3,
	       bad_local_file=-4,
	       insufficient_space=-5,
	       internal_=-6
	} ;

	static unsigned long totalsize = 0;

	int fd = -1;
	struct stat sbuf;
	const char *basename = strrchr(localfile,'/');
	if (basename) {
		basename = basename+1;
	} else {
		basename = localfile;
	}

	if (NULL == vfspath)
	{
		/* how the heck did we get here then? */
		fprintf(stderr,"\n   ERROR: No VFS path given.\n");
		return bad_parameters;
	}

	if (stat(localfile, &sbuf) < 0)
	{
		fprintf(stderr,"   ERROR: Unable to open '%s'!\n", localfile);
		return bad_local_file;
	}
	if (S_IFREG != (sbuf.st_mode & S_IFREG))
	{
		fprintf(stderr,"   ERROR: Not a regular file.\n");
		return bad_local_file;
	}

	fd = open(localfile,O_RDONLY);
	if (fd < 0)
	{
		fprintf(stderr,"   ERROR: Cannot open local file for reading.\n");
		return bad_local_file;
	}

	if (dlp_OpenConduit(sd) < 0)
	{
		fprintf(stderr, "   ERROR: Exiting on cancel, some files were not"
				"installed\n\n");
		return cancel;
	}

	fprintf(stdout, "   Installing '%s'... ", basename);
	fflush(stdout);

	if(pi_file_install_VFS(fd,basename,sd,vfspath,plu_quiet ? NULL : install_progress) < 0) {
		fprintf(stderr,"   ERROR: pi_file_install_VFS failed.\n");
	} else {
		totalsize += sbuf.st_size;
		printf("   %ld KiB total.\n", totalsize/1024);
		fflush(stdout);
	}

	close(fd);
	return 0;
}



static void
palm_install(unsigned long int flags,const char *localfile)
{
	switch(flags & MEDIA_MASK)
	{
	case MEDIA_RAM:
	case MEDIA_ROM:
	case MEDIA_FLASH:
		palm_install_internal(localfile);
		break;
	case MEDIA_VFS:
		palm_install_VFS(localfile,vfsdir);
		break;
	default:
		fprintf(stderr,"   ERROR: Unknown media type %lx\n",
				(flags & MEDIA_MASK));
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
static void
palm_merge(const char *filename)
{
	struct pi_file *f;

	f = pi_file_open(filename);

	if (f == 0)
	{
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
 *		memory.
 *
 * Parameters:  Media type
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void
palm_list_internal(unsigned long int flags)
{
	int				i		= 0,
					j,
					dbcount	= 0;
	struct DBInfo	info;
	char			synclog[68];
	pi_buffer_t		*buffer;

	printf("   Reading list of databases in RAM%s...\n",
			(flags & MEDIA_MASK) ? " and ROM" : "");

	buffer = pi_buffer_new(sizeof(struct DBInfo));

	for (;;)
	{
		if (dlp_ReadDBList
				(sd, 0, ((flags & MEDIA_MASK) ? 0x40 : 0) | 0x80 |
				 dlpDBListMultiple, i, buffer) < 0)
			break;
		for (j=0; j < (buffer->used / sizeof(struct DBInfo)); j++)
		{
			memcpy(&info, buffer->data + j * sizeof(struct DBInfo),
					sizeof(struct DBInfo));
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
static void
print_fileinfo(const char *path, FileRef file)
{
	int		size;
	time_t	date;
	char	*s;

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
static void
print_dir(long volume, const char *path, FileRef dir)
{
	unsigned long		it						= 0;
	int					max						= 64;
	struct VFSDirInfo	infos[64];
	int					i;
	FileRef				file;
	char				buf[vfsMAXFILENAME];
	int					pathlen					= strlen(path);

	/* Set up buf so it contains path with trailing / and
	   buflen points to the terminating NUL. */
	if (pathlen<1)
	{
		printf("   NULL path.\n");
		return;
	}

	memset(buf,0,vfsMAXFILENAME);
	strncpy(buf,path,vfsMAXFILENAME-1);

	if (buf[pathlen-1] != '/')
	{
		buf[pathlen]='/';
		pathlen++;
	}

	if (pathlen>vfsMAXFILENAME-2)
	{
		printf("   Path too long.\n");
		return;
	}

	while (dlp_VFSDirEntryEnumerate(sd,dir,&it,&max,infos) >= 0)
	{
		if (max<1) break;
		for (i = 0; i<max; i++)
		{
			memset(buf+pathlen,0,vfsMAXFILENAME-pathlen);
			strncpy(buf+pathlen,infos[i].name,vfsMAXFILENAME-pathlen);
			if (dlp_VFSFileOpen(sd,volume,buf,dlpVFSOpenRead,&file) < 0)
			{
				printf("   %s: No such file or directory.\n",infos[i].name);
			} else {
				print_fileinfo(infos[i].name, file);
				dlp_VFSFileClose(sd,file);
			}
		}
	}
}



static int
numdigits (int num)
{
	int				digits = 1;

	while ((num /= 10) > 0)
		digits++;

	return digits;
}

static char *
mediatype (struct VFSInfo *info)
{
	size_t                  ret_size;
	char			*fs,
					*media,
					*ret;
	static char		crid1[] = "<    >",
					crid2[] = "on <    >";

	switch(info->fsType) {
		case pi_mktag('v','f','a','t'):
			fs = "VFAT";
			break;
		case pi_mktag('f','a','t','s'):
			fs = "FAT";
			break;
		case pi_mktag('t','w','M','F'):
			fs = "Tapwave";
			break;
		default:
			/* This sortof-untag is gross, but useful */
			crid1[1] = (char)(info->mediaType >> 24) & 0xff;
			crid1[2] = (char)(info->mediaType >> 16) & 0xff;
			crid1[3] = (char)(info->mediaType >> 8) & 0xff;
			crid1[4] = (char)(info->mediaType) & 0xff;
			fs = crid1;
	}
	switch(info->mediaType) {
		case pi_mktag('m','m','c','d'):
			media = "on MMC";
			break;
		case pi_mktag('s','d','i','g'):
			media = "on SD";
			break;
		case pi_mktag('m','s','t','k'):
			media = "on MS";
			break;
		case pi_mktag('c','f','s','h'):
			media = "on CF";
			break;
		case pi_mktag('T','F','F','S'):
			media = "in RAM";
			break;
		case pi_mktag('t','w','M','F'):
			media = "MFS";
			break;
		default:
			/* It's not less gross the second time... */
			crid2[4] = (char)(info->mediaType >> 24) & 0xff;
			crid2[5] = (char)(info->mediaType >> 16) & 0xff;
			crid2[6] = (char)(info->mediaType >> 8) & 0xff;
			crid2[7] = (char)(info->mediaType) & 0xff;
			media = crid2;
	}
	ret_size = strlen(fs) + strlen(media) + 8;
	ret = (char *)malloc(ret_size);
	if (ret == NULL) {
		return NULL;
	}

	memset(ret,0,ret_size);
	snprintf (ret, ret_size-1, "%s %s", fs, media);
	return ret;
}

typedef struct cardreport_s {
	char				*type;
	long				size_total;
	long				size_used;
	long				size_free;
	int					cardnum;
	char				*name;
	struct cardreport_s	*next;
} cardreport_t;

static int
palm_cardinfo ()
{
	int				i,
					j,
					ret,
					volume_count = 16,
					volumes[16];
	cardreport_t	*cards = NULL,
					*t,
					*t2;
	struct VFSInfo	info;
	char			buf[vfsMAXFILENAME],
					fmt[64];
	long			size_used,
					size_total;
	int				len;					/* should be size_t in dlp.c? */
	
	size_t			digits_type = 10,
					digits_total = 4,
					digits_used = 4,
					digits_free = 4,
					digits_cardnum = 1;
	static const char unknown_type[] = "<unknown>";

	/* VFS info */
	if ((ret = dlp_VFSVolumeEnumerate(sd,&volume_count,volumes)) < 0) {
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR,
				"palm_cardinfo: dlp_VFSVolumeEnumerate returned %i\n", ret));
		goto cleanup;
	}

	for (i = 0; i < volume_count; i++)
	{
		if ((ret = dlp_VFSVolumeInfo(sd, volumes[i], &info)) < 0) {
			LOG ((PI_DBG_USER, PI_DBG_LVL_ERR,
					"palm_cardinfo: dlp_VFSVolumeInfo returned %i\n", ret));
			goto cleanup;
		}
		if ((ret = dlp_VFSVolumeSize(sd, volumes[i], &size_used,
						&size_total)) < 0)
		{
			LOG ((PI_DBG_USER, PI_DBG_LVL_ERR,
					"palm_cardinfo: dlp_VFSVolumeSize returned %i\n", ret));
			goto cleanup;
		}

		len = sizeof(buf);
		buf[0] = '\0';
		dlp_VFSVolumeGetLabel (sd, volumes[i], &len, buf);

		t = malloc (sizeof (cardreport_t));
		t->size_used = size_used;
		t->size_total = size_total;
		t->size_free = size_total - size_used;
		t->type = mediatype(&info);
		t->cardnum = info.slotRefNum;
		t->name = malloc (strlen(buf) + 1);
		strcpy (&t->name[1], buf);
		t->name[0] = '/';

		/* Insert into sorted order */
		if (cards == NULL || cards->cardnum > t->cardnum) {
			t->next = cards;
			cards = t;
		} else {
			t2 = cards;
			while (t2->next != NULL && t2->next->cardnum < t->cardnum)
				t2 = t2->next;
			t->next = t2->next;
			t2->next = t;
		}

		/* Determine field widths */
#define FIELDWIDTH(NAME, FUNC)		\
		j = FUNC;					\
		if (j > NAME)				\
				NAME = j
		FIELDWIDTH (digits_type, (t->type==NULL ? sizeof(unknown_type) : strlen(t->type)));
		FIELDWIDTH (digits_used, numdigits(t->size_used));
		FIELDWIDTH (digits_total, numdigits(t->size_total));
		FIELDWIDTH (digits_free, numdigits(t->size_free));
		FIELDWIDTH (digits_cardnum, numdigits(t->cardnum));
#undef FIELDWIDTH
	}

	memset(fmt,0,sizeof(fmt));
	snprintf (fmt, sizeof(fmt)-1, "%%-%zus  %%%zus  %%%zus  %%%zus  %%-%zus  %%s\n",
			digits_type, digits_used, digits_total, digits_free,
			digits_cardnum);
	
	printf (fmt, "Filesystem", "Size", "Used", "Free", "#", "Card name");

	memset(fmt,0,sizeof(fmt));
	snprintf (fmt, sizeof(fmt)-1, "%%-%zus  %%%zuli  %%%zuli  %%%zuli  %%%zui  %%s\n",
			digits_type, digits_used, digits_total, digits_free,
			digits_cardnum);

	for (t = cards; t != NULL; t = t->next) {
		printf (fmt, t->type==NULL ? unknown_type : t->type, t->size_used, t->size_total,
				t->size_free, t->cardnum, t->name);
	}

cleanup:
	t = cards;
	while (t != NULL)
	{
		cards = t->next;
		free (t->name);
		if (t->type != NULL) 
			free (t->type);
		free (t);
		t = cards;
	}
	return 0;
}


/***********************************************************************
 *
 * Function:    findVFSRoot_clumsy
 *
 * Summary:     For internal use only. May contain live weasels.
 *
 * Parameters:  root_component --> root path to search for.
 *              match          <-> volume matching root_component.
 *
 * Returns:     -2 on VFSVolumeEnumerate error,
 *              -1 if no match was found,
 *              0 if a match was found and @p match is set,
 *              1 if no match but only one VFS volume exists and
 *                match is set.
 *
 ***********************************************************************/
static int
findVFSRoot_clumsy(const char *root_component, long *match)
{
	int				volume_count		= 16;
	int				volumes[16];
	struct VFSInfo	info;
	int				i;
	int				buflen;
	char			buf[vfsMAXFILENAME];
	long			matched_volume		= -1;

	if (dlp_VFSVolumeEnumerate(sd,&volume_count,volumes) < 0)
	{
		return -2;
	}

	/* Here we scan the "root directory" of the Pilot.  We will fake out
	   a bunch of directories pointing to the various "cards" on the
	   device. If we're listing, print everything out, otherwise remain
	   silent and just set matched_volume if there's a match in the
	   first filename component. */
	for (i = 0; i<volume_count; ++i)
	{
		if (dlp_VFSVolumeInfo(sd,volumes[i],&info) < 0)
			continue;

		buflen=vfsMAXFILENAME;
		buf[0]=0;
		(void) dlp_VFSVolumeGetLabel(sd,volumes[i],&buflen,buf);

		/* Not listing, so just check matches and continue. */
		if (0 == strcmp(root_component,buf)) {
			matched_volume = volumes[i];
			break;
		}
		/* volume label no longer important, overwrite */
		sprintf(buf,"card%d",info.slotRefNum);

		if (0 == strcmp(root_component,buf)) {
			matched_volume = volumes[i];
			break;
		}
	}

	if (matched_volume >= 0) {
		*match = matched_volume;
		return 0;
	}

	if ((matched_volume < 0) && (1 == volume_count)) {
		/* Assume that with one card, just go look there. */
		*match = volumes[0];
		return 1;
	}
	return -1;
}

/***********************************************************************
 *
 * Function:    findVFSPath
 *
 * Summary:     Search the VFS volumes for @p path. Sets @p volume
 *              equal to the VFS volume matching @p path (if any) and
 *              fills buffer @p rpath with the path to the file relative
 *              to the volume.
 *
 *              Acceptable root components are /cardX/ for card indicators
 *              or /volumename/ for for identifying VFS volumes by their
 *              volume name. In the special case that there is only one
 *              VFS volume, no root component need be specified, and
 *              "/DCIM/" will map to "/card1/DCIM/".
 *
 * Parameters:  path           --> path to search for.
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
static int
findVFSPath(const char *path, long *volume, char *rpath, int *rpathlen)
{
	char	*s;
	int		r;

	if ((NULL == path) || (NULL == rpath) || (NULL == rpathlen))
		return -1;
	if (*rpathlen < strlen(path))
		return -1;

	memset(rpath,0,*rpathlen);
	if ('/'==path[0])
		strncpy(rpath,path+1,*rpathlen-1);
	else
		strncpy(rpath,path,*rpathlen-1);
	s = strchr(rpath,'/');
	if (NULL != s)
		*s=0;


	r = findVFSRoot_clumsy(rpath,volume);
	if (r < 0)
		return r;

	if (0 == r)
	{
		/* Path includes card/volume label. */
		r = strlen(rpath);
		if ('/'==path[0])
			++r; /* adjust for stripped / */
		memset(rpath,0,*rpathlen);
		strncpy(rpath,path+r,*rpathlen-1);
	} else {
		/* Path without card label */
		memset(rpath,0,*rpathlen);
		strncpy(rpath,path,*rpathlen-1);
	}

	if (!rpath[0])
	{
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

	if (dlp_VFSFileOpen(sd,volume,path,dlpVFSOpenRead,&file) < 0)
	{
		printf("   %s: No such file or directory.\n",path);
		return;
	}

	if (dlp_VFSFileGetAttributes(sd,file,&attributes) < 0)
	{
		printf("   %s: Cannot get attributes.\n",path);
		return;
	}

	if (vfsFileAttrDirectory == (attributes & vfsFileAttrDirectory))
	{
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
 *              physical VFS cards present; other paths should specify
 *              either a card as /cardX/ in the path of a volume label.
 *              As a special case, /dir/ is mapped to /card1/dir/ if
 *              there is only one card.
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void
palm_list_VFS()
{
	char	root_component[vfsMAXFILENAME];
	int		rootlen							= vfsMAXFILENAME;
	long	matched_volume					= -1;
	int		r;

	/* Listing the root directory / has special handling. Detect that
	   here. */
	if (NULL == vfsdir)
		vfsdir="/";

	/* Find the given directory. Will list the VFS root dir if the
	   requested dir is "/" */
	printf("   Directory of %s...\n",vfsdir);

	r = findVFSPath(vfsdir,&matched_volume,root_component,&rootlen);

	if (-2 == r)
	{
		fprintf(stderr,"   Not ready reading drive C:\n");
		return;
	}
	if (r < 0)
	{
		fprintf(stderr,"   No such directory, use pilot-xfer -C to list volumes.\n");
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
static void
palm_list(unsigned long int flags)
{
	switch(flags & MEDIA_MASK)
	{
		case MEDIA_RAM:
		case MEDIA_ROM:
		case MEDIA_FLASH:
			palm_list_internal(flags);
			break;
		case MEDIA_VFS:
			palm_list_VFS();
			break;
		default:
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
static void
palm_purge(void)
{
	int				i	= 0,
					h;
	struct DBInfo	info;
	pi_buffer_t		*buffer;

	printf("Reading list of databases to purge...\n");

	buffer = pi_buffer_new(sizeof(struct DBInfo));

	for (;;)
	{
		if (dlp_ReadDBList(sd, 0, 0x80, i, buffer) < 0)
			break;

		memcpy (&info, buffer->data, sizeof(struct DBInfo));
		i = info.index + 1;

		if (info.flags & 1)
			continue;	/* skip resource databases */

		printf("Purging deleted records from '%s'... ", info.name);

		h = 0;
		if ((dlp_OpenDB(sd, 0, 0x40 | 0x80, info.name, &h) >= 0)
				&& (dlp_CleanUpDatabase(sd, h) >= 0)
				&& (dlp_ResetSyncFlags(sd, h) >= 0))
		{
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



int
main(int argc, const char *argv[])
{
	int					optc,			/* switch */
						unsaved			= 0;
	const char			*archive_dir    = NULL,
		                *dirname        = NULL;
	unsigned long int	sync_flags		= 0;
	palm_op_t			palm_operation	= palm_op_noop;
	const char			*gracias		= "\n   Thank you for using pilot-link.\n";

	const char			**rargv;		/* for scanning remaining arguments */
	int					rargc;
	poptContext			pc;

	struct poptOption	options[]		=
	{
		USERLAND_RESERVED_OPTIONS

		/* action indicators that take a <dir> argument */
		{"backup",   'b', POPT_ARG_STRING, &dirname, palm_op_backup, "Back up your Palm to <dir>", "dir"},
		{"update",   'u', POPT_ARG_STRING, &dirname, palm_op_update, "Update <dir> with newer Palm data", "dir"},
		{"sync",     's', POPT_ARG_STRING, &dirname, palm_op_sync, "Same as -u option, but removes local files if data is removed from your Palm", "dir"},
		{"restore",  'r', POPT_ARG_STRING, &dirname, palm_op_restore, "Restore backupdir <dir> to your Palm", "dir"},

		/* action indicators that take no argument, but eat the remaining command-line arguments. */
		{"install",  'i', POPT_ARG_NONE, NULL, palm_op_install, "Install local prc, pdb, pqa files to your Palm", NULL},
		{"fetch",    'f', POPT_ARG_NONE, NULL, palm_op_fetch, "Retrieve databases from your Palm", NULL},
		{"merge",    'm', POPT_ARG_NONE, NULL, palm_op_merge, "Adds records from local files into the corresponding Palm databases", NULL},
		{"delete",   '\0', POPT_ARG_NONE, NULL, palm_op_delete, "Delete (permanently) databases from your Palm", NULL},

		/* action indicators that take no arguments. */
		{"list",     'l', POPT_ARG_NONE, NULL, palm_op_list, "List all application and 3rd party Palm data/apps", NULL},
		{"cardinfo", 'C', POPT_ARG_NONE, NULL, palm_op_cardinfo, "Show information on available cards", NULL},

		/* action indicators that may be mixed in with the others */
		{"Purge",    'P', POPT_BIT_SET, &sync_flags, PURGE, "Purge any deleted data that hasn't been cleaned up", NULL},

		/* modifiers for the various actions */
		{"archive",  'a', POPT_ARG_STRING, &archive_dir, 0, "Modifies -s to archive deleted files in directory <dir>", "dir"},
		{"exclude",  'e', POPT_ARG_STRING, NULL, 'e', "Exclude databases listed in <file> from being included", "file"},
		{"vfsdir",   'D', POPT_ARG_STRING, &vfsdir, MEDIA_VFS, "Modifies -lif to use VFS <dir> instead of internal storage", "dir"},
		{"rom",       0 , POPT_ARG_NONE, NULL, MEDIA_FLASH, "Modifies -b, -u, and -s, to back up non-OS dbs from Flash ROM", NULL},
		{"with-os",   0 , POPT_ARG_NONE, NULL, MEDIA_ROM, "Modifies -b, -u, and -s, to back up OS dbs from Flash ROM", NULL},
		{"illegal",   0 , POPT_ARG_NONE, &unsaved, 0, "Modifies -b, -u, and -s, to back up the illegal database Unsaved Preferences.prc (normally skipped)", NULL},

		/* misc */
		{"exec",     'x', POPT_ARG_STRING, NULL, 'x', "Execute a shell command for intermediate processing", "command"},
		POPT_TABLEEND
	};

	const char			*help_header_text	=
		"\n"
		"   Sync, backup, install, delete and more from your Palm device.\n"
		"   This is the swiss-army-knife of the entire pilot-link suite.\n\n"
		"   Use exactly one of -brsudfimlI; mix in -aexDPv, --rom and --with-os.\n\n";

	pc = poptGetContext("pilot-xfer", argc, argv, options, 0);

	poptSetOtherOptionHelp(pc, help_header_text);
	/* can't alias both short and long in one go, hence "dupes" */
	plu_popt_alias(pc,"List",0,"--bad-option --list --rom");
	plu_popt_alias(pc,"Listall",0,"--bad-option --list --rom");
	plu_popt_alias(pc,NULL,'L',"--bad-option --list --rom");
	plu_popt_alias(pc,"Flash",0,"--bad-option --rom");
	plu_popt_alias(pc,NULL,'F',"--bad-option --rom");
	plu_popt_alias(pc,"OsFlash", 0,"--bad-option --with-os");
	plu_popt_alias(pc,NULL, 'O',"--bad-option --with-os");
	plu_popt_alias(pc,"Illegal", 0,"--bad-option --illegal");
	plu_popt_alias(pc,NULL, 'I',"--bad-option --illegal");
	plu_set_badoption_help(
		"       --rom instead of -F, --Flash\n"
		"       --with-os instead of -O, --OsFlash\n"
		"       --illegal instead of -I, --Illegal\n"
		"       --list --rom instead of -L, --List, --Listall\n\n");

	if (argc < 2) {
		poptPrintUsage(pc,stderr,0);
		return 1;
	}

	while ((optc = poptGetNextOpt(pc)) >= 0)
	{
		switch (optc)
		{
			/* Actions with a dir argument */

			case palm_op_backup:
			case palm_op_update:
			case palm_op_sync:
			case palm_op_restore:
			case palm_op_install:
			case palm_op_merge:
			case palm_op_fetch:
			case palm_op_delete:
			case palm_op_list:
			case palm_op_cardinfo:
				if (palm_op_noop != palm_operation)
				{
					fprintf(stderr,"   ERROR: specify only one of -brsuimfdlLC.\n");
					return 1;
				}
				palm_operation = optc;
				break;

			/* Modifiers */
			case MEDIA_VFS:
			case MEDIA_ROM:
			case MEDIA_FLASH:
				if ((sync_flags & MEDIA_MASK) != 0)
				{
					fprintf(stderr,"   ERROR: specify only one media type (-DFO).\n");
					return 1;
				}
				sync_flags |= optc;
				break;

			/* Misc */
			case 'e':
				make_excludelist(poptGetOptArg(pc));
				break;
			case 'x':
				if (system(poptGetOptArg(pc)))
				{
					fprintf(stderr,"   ERROR: system() failed, aborting.\n");
					return -1;
				}
				break;
			default:
				/* popt handles all other arguments internally by
				   setting values, setting bits, etc., only options
				   with field val != 0 come though here, and
				   all of them should special processing. */
				fprintf(stderr,"   ERROR: got option %d, arg %s\n", optc,
						poptGetOptArg(pc));
				return 1;
				break;
		}
	}

	if (optc < -1)
	{
		/* an error occurred during option processing */
		fprintf(stderr, "%s: %s\n",
				poptBadOption(pc, POPT_BADOPTION_NOALIAS),
				poptStrerror(optc));
		return 1;
	}

	/* collect and count remaining arguments */
	rargc = 0;
	rargv = poptGetArgs(pc);
	if (rargv)
	{
		const char	**s	= rargv;
		while (*s)
		{
			s++;
			rargc++;
		}
	}

	/* sanity checking */

	switch(palm_operation)
	{
		struct stat sbuf;

		case palm_op_backup:
		case palm_op_restore:
		case palm_op_update:
		case palm_op_sync:
			if (sync_flags & MEDIA_VFS)
			{
				fprintf(stderr,"   ERROR: Cannot combine -burs with VFS (-D).\n\n");
				return 1;
			}
			if (stat (dirname, &sbuf) == 0)
			{
				if (!S_ISDIR (sbuf.st_mode))
				{
					fprintf(stderr, "   ERROR: '%s' is not a directory or does not exist.\n"
							"   Please supply a directory name when performing a "
							"backup or restore and try again.\n\n", dirname);
					fprintf(stderr,gracias);
					return 1;
				}
			}
			/* FALLTHRU */
		case palm_op_cardinfo:
		case palm_op_list:
			if (rargc > 0)
			{
				fprintf(stderr,"   ERROR: Do not pass additional arguments to -busrlLC.\n");
				fprintf(stderr,gracias);
				return 1;
			}
			break;
		case palm_op_noop:
			fprintf(stderr,"   ERROR: Must specify one of -bursimfdlC.\n");
			fprintf(stderr,gracias);
			return 1;
			break;
		case palm_op_merge:
		case palm_op_delete:
			if (MEDIA_VFS == (sync_flags & MEDIA_VFS))
			{
				fprintf(stderr,"   ERROR: cannot merge or delete with VFS.\n");
				return 1;
			}
			/* FALLTHRU */
		case palm_op_fetch:
		case palm_op_install:
			if (rargc < 1)
			{
				fprintf(stderr,"   ERROR: -imfd require additional arguments.\n");
				return 1;
			}
			break;
	}

	/* plu_connect() prints diagnostics as needed, returns -1 on
	failure so just bail in that case. */
	sd = plu_connect();
	if (sd < 0)
		return 1;

	/* actual operation */
	switch(palm_operation)
	{
		case palm_op_noop: /* handled above */
			exit(1);
			break;
		case palm_op_backup:
		case palm_op_update:
		case palm_op_sync:
			if (palm_op_backup == palm_operation)
				sync_flags |= BACKUP;
			if (palm_op_update == palm_operation)
				sync_flags |= UPDATE;
			if (palm_op_sync == palm_operation)
				sync_flags |= UPDATE | SYNC;
			palm_backup(dirname, sync_flags, unsaved, archive_dir);
			break;
		case palm_op_restore:
			palm_restore(dirname);
			break;
		case palm_op_merge:
		case palm_op_install:
		case palm_op_fetch:
		case palm_op_delete:
			while (rargv && *rargv)
			{
				switch(palm_operation)
				{
					case palm_op_merge:
						palm_merge(*rargv);
						break;
					case palm_op_fetch:
						palm_fetch(sync_flags,*rargv);
						break;
					case palm_op_delete:
						palm_delete(*rargv);
						break;
					case palm_op_install:
						palm_install(sync_flags,*rargv);
						break;
					default:
						/* impossible */
						break;
				}
				rargv++;
			}
			break;
		case palm_op_list:
			palm_list(sync_flags);
			break;
		case palm_op_cardinfo:
			palm_cardinfo();
			break;
	}

	if (sync_flags & PURGE)
		palm_purge();

	pi_close(sd);
	puts(gracias);
	return 0;
}

/* vi: set ts=4 sw=4 sts=4 noexpandtab: cin */
