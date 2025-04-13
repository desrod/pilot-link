/*
 * $Id$ 
 *
 * pilot-650foto.c: Grab photos and video off a Treo 650 camera phone.
 *
 * (c) 2004, Matthew Allum <mallum@o-hand.com>
 * (c) 2006, Angus Ainslie <angusa@deltatee.com>
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

#define MEDIA_PATH "/Photos & Videos"
#define MEDIA_VOLUME 1

static int
  pi_file_retrieve_VFS(const int fd, const int socket, FileRef file, const char *rpath )
{
   int          rpathlen = vfsMAXFILENAME;
   long         attributes;
   pi_buffer_t  *buffer;
   ssize_t      readsize,writesize;
   int          filesize;
   int          original_filesize;
   int          written_so_far;
   pi_progress_t progress;

   enum
     {
	bad_parameters=-1,
	  cancel=-2,
	  bad_vfs_path=-3,
	  bad_local_file=-4,
	  insufficient_space=-5,
	  internal_=-6
     };

   rpathlen=strlen(rpath);

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

   memset(&progress, 0, sizeof(progress));
   progress.type = PI_PROGRESS_RECEIVE_VFS;
   progress.data.vfs.path = (char *)rpath;
   progress.data.vfs.total_bytes = filesize;

#define FBUFSIZ 65536

   buffer = pi_buffer_new(FBUFSIZ);
   readsize = 0;
   written_so_far = 0;
   while ((filesize > 0) && (readsize >= 0))
     {
	int offset;

	pi_buffer_clear(buffer);
	readsize = dlp_VFSFileRead(socket,file,buffer,
				   ( filesize > FBUFSIZ ? FBUFSIZ : filesize));

	/*f printf(stderr,"*  Read %ld bytes.\n",readsize); */

	if (readsize <= 0) break;
	filesize -= readsize;
	offset = 0;
	while (readsize > 0)
	  {
	     writesize = write(fd,buffer->data+offset,readsize);
	     /* fprintf(stderr,"*  Wrote %ld bytes.\n",writesize); */
	     if (writesize < 0)
	       {
		  fprintf(stderr,"Error while writing file.\n");
		  goto cleanup;
	       }

	     written_so_far += writesize;
	     progress.transferred_bytes += writesize;

	     if ((filesize > 0) || (readsize > 0))
	       {
/*				if (f && (f(socket,&progress) == PI_TRANSFER_STOP)) {
					written_so_far = cancel;
					pi_set_error(socket,PI_ERR_FILE_ABORTED);
					goto cleanup;
				}*/
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
  print_fileinfo(int sd, const char *path, FileRef file)
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
 * Summary:     Output all the files in the MEDIA_DIR
 *
 * Parameters:  volume      --> volume ref number.
 *              path        --> path to directory.
 *              dir         --> file ref to already opened dir.
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void
  dump_dir(int sd, long volume, const char *path, FileRef dir)
{
   int					it						= vfsIteratorStart;
   int					max						= 64;
   struct VFSDirInfo	infos[64];
   int					i;
   FileRef				file;
   char				buf[vfsMAXFILENAME];
   int					pathlen					= strlen(path);
   int fd;
   char *index;

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
	       }
	     else
	       {
		  if( index = rindex( infos[i].name, '.' ))
		    {
//		       printf( "index: %s %d strlen %d\n", infos[i].name, index, strlen( infos[i].name ) );
		       if(( index + 4 ) == (infos[i].name + strlen( infos[i].name )))
			 {
			    print_fileinfo(sd,infos[i].name, file);

			    if( !strcmp( index,".jpg" ) ||  !strcmp( index,".3gp" ))
			      {
				 fd = open( infos[i].name, O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);
				 pi_file_retrieve_VFS( fd, sd, file, buf );
				 close(fd);
			      }
			 }
		    }

		  dlp_VFSFileClose(sd,file);
	       }
	  }
     }
}

int main(int argc, const char *argv[])
{
   int sd = -1;
   int c;
   int ret = 1;
   FileRef file;
   unsigned long attributes;

   enum { mode_none, mode_write = 257 } run_mode = mode_none;

   struct poptOption options[] =
     {
	USERLAND_RESERVED_OPTIONS
	  {"write",'w', POPT_ARG_NONE,NULL,mode_write,"Write data"},
	POPT_TABLEEND
     };

   poptContext po = poptGetContext("pilot-650foto",argc,argv,options,0);
   poptSetOtherOptionHelp(po,"\n\n"
			  "   Copies Treo 650 photos and videos to current directory\n");

   if ((argc < 2))
     {
	poptPrintUsage(po,stderr,0);
	return 1;
     }

   while ((c = poptGetNextOpt(po)) >= 0)
     {
	switch(c)
	  {
	   case mode_write :
	     if (run_mode == mode_none)
	       {
		  run_mode = c;
	       }
	     else
	       {
		  if (c != run_mode)
		    {
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

   if (c < -1)
     {
	plu_badoption(po,c);
     }

   if (mode_none == run_mode)
     {
	fprintf(stderr,"   ERROR: Specify --write (-w) to output data.\n");
	return 1;
     }

   if ((sd = plu_connect()) < 0)
     {
	return 1;
     }

   if (dlp_VFSFileOpen(sd,MEDIA_VOLUME,MEDIA_PATH,dlpVFSOpenRead,&file) < 0)
     {
	printf("   %s: No such file or directory.\n",MEDIA_PATH);
	goto cleanup;
     }

   if (dlp_VFSFileGetAttributes(sd,file,&attributes) < 0)
     {
	printf("   %s: Cannot get attributes.\n",MEDIA_PATH);
	goto cleanup;
     }

   if (vfsFileAttrDirectory == (attributes & vfsFileAttrDirectory))
     {
		/* directory */
	dump_dir(sd,MEDIA_VOLUME,MEDIA_PATH,file);
     }

   ret=0;

   cleanup:
   if( file >= 0 )
     (void) dlp_VFSFileClose(sd,file);

   if (sd >= 0)
     {
	dlp_EndOfSync(sd, 0);
	pi_close(sd);
     }

   return ret;

}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
/* ex: set tabstop=4 expandtab: */
/* Local Variables: */
/* indent-tabs-mode: t */
/* c-basic-offset: 8 */
/* End: */
