/* 
 * vfs-test.c:  VFS Regression Test
 *
 * (c) 2002, JP Rosevear.
 * (c) 2004, Florent Pillet
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
 *
 * This regression test calls every DLP function except the following:
 *    dlp_CallApplication
 *    dlp_ResetSystem
 *    dlp_ProcessRPC
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "pi-debug.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-error.h"
#include "pi-source.h"
#include "pi-header.h"


/* For various protocol versions, set to 0 to not test those versions */
#define DLP_1_1 1
#define DLP_1_2 1
#define DLP_1_3 1

/* Logging defines */
#define CHECK_RESULT(func) \
	if (result < 0) { \
		LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "VFSTEST " #func " failed (%d, last palmos error: 0x%04x)\n", result, pi_palmos_error(sd))); \
		if (result == PI_ERR_SOCK_DISCONNECTED) \
			goto error; \
	}

#define TEST_VFS_DIR			"/vfs-test"
#define TEST_VFS_FILE			"/vfs-test/test.dat"
#define TEST_VFS_FILE2			"/vfs-test/test2.dat"
#define TEST_VFS_FILE2_SHORT	"test2.dat"
#define BIG_FILE_SIZE			(2 * 1024 * 1024)			/* test vfs read/write with 1Mb data */

int main (int argc, char **argv)
{
	int 	i,
		j,
		sd,
		result,
		refs[200],
		ref_length,
		len,
		numStrings;
	char *strings;
	FileRef fileRef;
	unsigned long fileAttrs;
	unsigned char *buf1;

	time_t t1, t2;
	char name[255],
		oldName[255];

	unsigned long flags;

	sd = pilot_connect (argv[1]);
	if (sd < 0) return 1;

	t1 = time (NULL);
	LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "VFSTEST Starting at %s", ctime (&t1)));

	/*********************************************************************
	 *
	 * Test: Expansion
	 *
	 * Direct Testing Functions:
	 *   dlp_ExpSlotEnumerate
	 *   dlp_ExpCardPresent
	 *   dlp_ExpCardInfo
	 *
	 * Indirect Testing Functions:
	 *   None
	 *
	 *********************************************************************/
#if DLP_1_3
	ref_length = sizeof (refs) / sizeof (refs[0]);
	result = dlp_ExpSlotEnumerate (sd, &ref_length, refs);
	CHECK_RESULT(dlp_ExpSlotEnumerate);

	for (i = 0; i < ref_length; i++) {
		result = dlp_ExpCardPresent (sd, refs[i]);
		if (result == PI_ERR_DLP_PALMOS) { 
			LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "* dlp_ExpCardPresent: card not present in slot %d, PalmOS err 0x%04x\n", i, pi_palmos_error(sd)));
		} else {
			CHECK_RESULT(dlp_ExpCardPresent);
			if (result >= 0) {
				strings = NULL;
				result = dlp_ExpCardInfo (sd, refs[i], &flags, &numStrings, &strings);
				CHECK_RESULT(dlp_ExpCardInfo);

				LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* Expansion card {%d}: "
					"capabilities = 0x%08lx, numStrings = %d\n", refs[i], flags, numStrings));

				if (strings) {
					char *p = strings;
					for (j = 0; j < numStrings; j++) {
						LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "\tstring[%d] = '%s'\n", j, p));
						p += strlen (p) + 1;
					}
					free (strings);
				}
			}
		}
	}
#endif

	/*********************************************************************
	 *
	 * Test: VFS Volumes, files and directories
	 *
	 * Direct Testing Functions:
	 *   dlp_VFSDirCreate
	 *   dlp_VFSDirEntryEnumerate
	 *   dlp_VFSFileClose
	 *   dlp_VFSFileCreate
	 *   dlp_VFSFileDelete
	 *   dlp_VFSFileEOF
	 *   dlp_VFSFileGetAttributes
	 *   dlp_VFSFileGetDate
	 *   dlp_VFSFileOpen
	 *   dlp_VFSFileRead
	 *   dlp_VFSFileRename
	 *   dlp_VFSFileResize
	 *   dlp_VFSFileSeek
	 *   dlp_VFSFileSetAttributes
	 *   dlp_VFSFileSetDate
	 *   dlp_VFSFileSize
	 *   dlp_VFSFileTell
	 *   dlp_VFSFileWrite
	 *   dlp_VFSGetDefaultDir
	 *   dlp_VFSVolumeEnumerate
	 *   dlp_VFSVolumeGetLabel
	 *   dlp_VFSVolumeInfo
	 *   dlp_VFSVolumeSetLabel
	 *   dlp_VFSVolumeSize
	 *
	 * Indirect Testing Functions:
	 *   None
	 *
	 *********************************************************************/
#if DLP_1_3
	ref_length = sizeof (refs) / sizeof (refs[0]);
	result = dlp_VFSVolumeEnumerate (sd, &ref_length, refs);
	CHECK_RESULT(dlp_VFSVolumeEnumerate);

	for (i = 0; i < ref_length; i++) {
		struct VFSInfo vfs;
		long used, total;

		LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "\nTESTING VOLUME %d/%d {ref=%d}\n\n", i+1, ref_length, refs[i]));

		result = dlp_VFSVolumeInfo (sd, refs[i], &vfs);
		CHECK_RESULT(dlp_VFSVolumeInfo);
		if (result >= 0) {
			name[0] = 0;
			if (vfs.attributes & 0x00000004)
				strcpy (name, "hidden ");
			if (vfs.attributes & 0x00000002)
				strcat (name, "read-only ");
			if (vfs.attributes & 0x00000001)
				strcat (name, "slot-based");
			LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* Volume attributes:\n\tattributes: %s\n", name));
			LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "\tfsType: '%4.4s'\n\tfsCreator: '%4.4s'\n\tmountClass: '%4.4s'\n", &vfs.fsType, &vfs.fsCreator, &vfs.mountClass));
			LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "\tslotLibRefNum: %d\n\tslotRefNum: %d\n\tmediaType: '%4.4s'\n\n", vfs.slotLibRefNum, vfs.slotRefNum, &vfs.mediaType));
		}

		result = dlp_VFSVolumeSize (sd, refs[i], &used, &total);
		CHECK_RESULT(dlp_VFSVolumeSize);
		if (result >= 0) {
			LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* volume used: %ld / %ld bytes\n", used, total));
		}

		/* volume label test */
		len = sizeof (name);
		result = dlp_VFSVolumeGetLabel (sd, refs[i], &len, name);
		CHECK_RESULT(dlp_VFSVolumeGetLabel);
		if (result >= 0) {
			LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* volume label: '%s'\n", name));
		}
		strcpy (oldName, name);

		result = dlp_VFSVolumeSetLabel (sd, refs[i], "Test");
		CHECK_RESULT(dlp_VFSVolumeSetLabel);
		if (result >= 0) {
			len = sizeof (name);
			result = dlp_VFSVolumeGetLabel (sd, refs[i], &len, name);
			CHECK_RESULT(dlp_VFSVolumeGetLabel);
			if (result >= 0) {
				if (strcmp("Test", name)) {
					LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "* ERROR: Label change mismatch\n"));
				} else {
					LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* Label change successful\n"));
				}
			}

			result = dlp_VFSVolumeSetLabel (sd, refs[i], oldName);
			CHECK_RESULT(dlp_VFSVolumeSetLabel);
		}

		/* directory listing test */
		len = sizeof (name);
		result = dlp_VFSGetDefaultDir (sd, refs[i], ".prc", name, &len);
		CHECK_RESULT(dlp_VFSGetDefaultDir);
		if (result >= 0) {
			LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* Listing directory contents at '%s'\n", name));

			result = dlp_VFSFileOpen (sd, refs[i], name, 0x0007 /* vfsModeReadWrite */, &fileRef);
			CHECK_RESULT(dlp_VFSFileOpen);
			if (result >= 0) {
				unsigned long dirIterator = vfsIteratorStart;
				do {
					struct VFSDirInfo dirItems[16];
					int dirCount;
					
					memset (dirItems, 0, sizeof(dirItems));
					dirCount = 16;
					result = dlp_VFSDirEntryEnumerate (sd, fileRef, &dirIterator, &dirCount, dirItems);
					CHECK_RESULT(dlp_VFSDirEntryEnumerate);

					if (result >= 0) {
						for (j = 0; j < dirCount; j++) {
							LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "\t'%s' attrs = 0x%08lx\n", dirItems[j].name, dirItems[j].attr));
						}
					}
				} while (dirIterator != vfsIteratorStop && result >= 0);

				result = dlp_VFSFileClose (sd, fileRef);
				CHECK_RESULT(dlp_VFSFileClose);
				LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* Closed directory %s\n", name));
			}
		}
		
		/* directory creation test */
		result = dlp_VFSDirCreate (sd, refs[i], "/vfs-test");
		CHECK_RESULT(dlp_VFSDirCreate);
		if (result >= 0) {
			LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* Created directory %s\n", TEST_VFS_DIR));

			/* file access tests */
			result = dlp_VFSFileCreate (sd, refs[i], TEST_VFS_FILE);
			CHECK_RESULT(dlp_VFSFileCreate);
			LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* Created file %s\n", TEST_VFS_FILE));

			result = dlp_VFSFileOpen (sd, refs[i], TEST_VFS_FILE, 0x0007 /* read-write */, &fileRef);
			CHECK_RESULT(dlp_VFSFileOpen);
			if (result >= 0) {
				pi_buffer_t *fileBuf =
					pi_buffer_new (BIG_FILE_SIZE);

				LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* Opened file %s\n", TEST_VFS_FILE));

				strcpy (name, "a test string written to a file\n");
				result = dlp_VFSFileWrite (sd, fileRef, (unsigned char *)name, strlen (name));
				CHECK_RESULT(dlp_VFSFileWrite);
				if (result >= 0) {
					LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* Wrote small string to file %s\n", TEST_VFS_FILE));

					result = dlp_VFSFileTell (sd, fileRef, &j);
					CHECK_RESULT(dlp_VFSFileTell);
					if (result >= 0) {
						LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* Current seek position: %d\n", j));
					}

					result = dlp_VFSFileSeek (sd, fileRef, vfsOriginBeginning, 0);
					CHECK_RESULT(dlp_VFSFileSeek);
					if (result >= 0) {
						LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* Seeked to beginning of file\n", j));
					}

					result = dlp_VFSFileRead (sd, fileRef, fileBuf, strlen(name));
					CHECK_RESULT(dlp_VFSFileRead);
					if (fileBuf->used != strlen (name)) {
						LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* ERROR: File read: read %d instead of the expected %d\n", len, strlen (name)));
					} else if (memcmp (name, fileBuf->data, fileBuf->used)) {
						LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* ERROR: File read: read data mismatch\n", fileBuf->used, strlen (name)));
						dumpdata (name, fileBuf->used);
						dumpdata (fileBuf->data, fileBuf->used);
					} else {
						LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* File read: successfully read the data back\n"));
					}
				}

				/* huge file write/read test - allocate a big buffer,
				   don't clean it (keep the garbage), write it to the
				   file and read it back them compare */
				LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* Please wait, writing %ld bytes of data to the file...\n", (long)BIG_FILE_SIZE));
				buf1 = (unsigned char *)malloc (BIG_FILE_SIZE);
				if (buf1) {
					result = dlp_VFSFileWrite (sd, fileRef, buf1, BIG_FILE_SIZE);
					CHECK_RESULT(dlp_VFSFileWrite);
					if (result >= 0) {
						LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* Successful, now reading it back...\n"));

						result = dlp_VFSFileSeek (sd, fileRef, vfsOriginCurrent, -BIG_FILE_SIZE);
						CHECK_RESULT(dlp_VFSFileSeek);

						result = dlp_VFSFileRead (sd, fileRef, fileBuf, BIG_FILE_SIZE);
						CHECK_RESULT(dlp_VFSFileRead);
						if (result >= 0) {
							if (fileBuf->used != BIG_FILE_SIZE || memcmp (buf1, fileBuf->data, BIG_FILE_SIZE))
								LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* ERROR, data differs!\n"));
							else
								LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* Successful, all bytes are the same...\n"));
						}
					}
				}
				if (buf1)
					free (buf1);
				pi_buffer_free (fileBuf);

				result = dlp_VFSFileEOF (sd, fileRef);
				if (pi_palmos_error(sd) == 0x2A07) {
					LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* File EOF: YES\n"));
				} else if (result >= 0) {
					LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* File EOF: NO\n"));
				} else {
					CHECK_RESULT(dlp_VFSFileEOF);
				}
				
				result = dlp_VFSFileSize (sd, fileRef, &len);
				CHECK_RESULT(dlp_VFSFileSize);
				if (result >= 0) {
					LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* File size: %d bytes\n", len));
				}

				result = dlp_VFSFileGetAttributes (sd, fileRef, &fileAttrs);
				CHECK_RESULT(dlp_VFSFileGetAttributes);
				if (result >= 0) {
					LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* File attributes: 0x%08lx\n", fileAttrs));
				}
				
				fileAttrs |= 0x00000001L;
				result = dlp_VFSFileSetAttributes (sd, fileRef, fileAttrs);
				CHECK_RESULT(dlp_VFSFileSetAttributes);

				result = dlp_VFSFileGetAttributes (sd, fileRef, &fileAttrs);
				CHECK_RESULT(dlp_VFSFileGetAttributes);
				if (fileAttrs & 0x00000001L) {
					LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* Successfully set read-only bit on file attributes\n"));
				}

				fileAttrs &= ~0x00000001L;
				result = dlp_VFSFileSetAttributes (sd, fileRef, fileAttrs);
				CHECK_RESULT(dlp_VFSFileSetAttributes);

				result = dlp_VFSFileResize (sd, fileRef, 1024);
				CHECK_RESULT(dlp_VFSFileResize);
				
				result = dlp_VFSFileSize (sd, fileRef, &len);
				CHECK_RESULT(dlp_VFSFileGetSize);
				if (result >= 0) {
					LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* File size after resize: %d\n", len));
				}

				result = dlp_VFSFileGetDate (sd, fileRef, vfsFileDateCreated, &t1);
				CHECK_RESULT(dlp_VFSFileGetDate);
				if (result >= 0) {
					ctime_r (&t1, name);
					LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* Date created: %s", name));
				}

				result = dlp_VFSFileGetDate (sd, fileRef, vfsFileDateModified, &t2);
				CHECK_RESULT(dlp_VFSFileGetDate);
				if (result >= 0) {
					ctime_r (&t2, name);
					LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* Date modified: %s", name));
				}

				result = dlp_VFSFileGetDate (sd, fileRef, vfsFileDateAccessed, &t2);
				CHECK_RESULT(dlp_VFSFileGetDate);
				if (result >= 0) {
					ctime_r (&t2, name);
					LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* Date accessed: %s", name));
				}

				t1 -= 3600 * 24;
				result = dlp_VFSFileSetDate (sd, fileRef, vfsFileDateCreated, t1);
				CHECK_RESULT(dlp_VFSFileSetDate);
				if (result >= 0) {
					result = dlp_VFSFileGetDate (sd, fileRef, vfsFileDateCreated, &t2);
					CHECK_RESULT(dlp_VFSFileGetDate);
					if (result >= 0) {
						if (t1 == t2)
							LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* dlp_VFSFileSetDate successful\n"));
						else
							LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* BUG: dlp_VFSFileSetDate FAILED\n"));
					}
				}

				result = dlp_VFSFileClose (sd, fileRef);
				CHECK_RESULT(dlp_VFSFileClose);
				if (result >= 0) {
					LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* Closed file %s\n", TEST_VFS_FILE));
				}
			}

			result = dlp_VFSFileRename (sd, refs[i], TEST_VFS_FILE, TEST_VFS_FILE2_SHORT);
			CHECK_RESULT(dlp_VFSFileRename);
			if (result >= 0) {
				LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* Renamed file %s to %s\n", TEST_VFS_FILE, TEST_VFS_FILE2_SHORT));
			}

			result = dlp_VFSFileDelete (sd, refs[i], TEST_VFS_FILE2);
			CHECK_RESULT(dlp_VFSFileDelete);
			if (result >= 0) {
				LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* Deleted file %s\n", TEST_VFS_FILE2));
			}
		}

		result = dlp_VFSFileDelete (sd, refs[i], "/vfs-test");
		CHECK_RESULT(dlp_VFSFileDelete);
		LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "* Deleted directory /vfs-test\n"));
	}
#endif

	/*********************************************************************
	 *
	 * Test: VFS Import/Export Databases
	 *
	 * Direct Testing Functions:
	 *   dlp_VFSImportDatabaseFromFile
	 *   dlp_VFSExportDatabaseFromFile
	 *
	 * Indirect Testing Functions:
	 *   None
	 *
	 *********************************************************************/

	t1 = time (NULL);
	LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "VFSTEST Ending at %s", ctime (&t1)));

error:
	pi_close (sd);

	return 0;
}

