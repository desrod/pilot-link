/* 
 * vfs-test.c:  VFS Regression Test
 *
 * (c) 2002, JP Rosevear.
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
#include "pi-debug.h"
#include "pi-socket.h"
#include "pi-dlp.h"

/* Prototypes */
int pilot_connect(char *port);

/* For various protocol versions, set to 0 to not test those versions */
#define DLP_1_1 1
#define DLP_1_2 1
#define DLP_1_3 1

/* Logging defines */
#define CHECK_RESULT(func) \
  if (result < 0) { \
	LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "VFSTEST " #func " failed (%d)\n", result)); \
	goto error; \
  }

int main (int argc, char **argv)
{
	int 	i,
		sd,
		result,
		refs[255],
		ref_length,
		len;

	time_t t1;
	char name[255];

	unsigned long flags;

	sd = pilot_connect (argv[1]);

	t1 = time (NULL);
	LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "VFSTEST Starting at %s", ctime (&t1)));

#if DLP_1_3
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
	ref_length = 256;
	result = dlp_ExpSlotEnumerate (sd, &ref_length, refs);
	CHECK_RESULT(dlp_ExpSlotEnumerate);

	for (i = 0; i < ref_length; i++) {
		result = dlp_ExpCardPresent (sd, refs[i]);
		if (result == -3) { 
			LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "VFSTEST dlp_ExpCardPresent "
			     "Card Not Present in Slot %d\n", refs[i]));
		} else {
			CHECK_RESULT(dlp_ExpCardPresent);

			result = dlp_ExpCardInfo (sd, refs[i], &flags, &len);
			CHECK_RESULT(dlp_ExpCardInfo);
		}
	}

	/*********************************************************************
	 *
	 * Test: VFS Volumes
	 *
	 * Direct Testing Functions:
	 *   dlp_VFSVolumeEnumerate
	 *   dlp_VFSVolumeInfo
	 *   dlp_VFSVolumeSetLabel
	 *   dlp_VFSVolumeGetLabel
	 *   dlp_VFSVolumeSize
	 *
	 * Indirect Testing Functions:
	 *   None
	 *
	 *********************************************************************/
	ref_length = 256;
	result = dlp_VFSVolumeEnumerate (sd, &ref_length, refs);
	CHECK_RESULT(dlp_VFSVolumeEnumerate);
	
	for (i = 0; i < ref_length; i++) {
		struct VFSInfo vfs;
		long used, total;
		
		result = dlp_VFSVolumeSize (sd, refs[i], &used, &total);
		CHECK_RESULT(dlp_VFSVolumeInfo);

		result = dlp_VFSVolumeSetLabel (sd, refs[i], "Test");
		CHECK_RESULT(dlp_VFSVolumeInfo);

		len = 255;
		result = dlp_VFSVolumeGetLabel (sd, refs[i], &len, name);
		CHECK_RESULT(dlp_VFSVolumeInfo);

		if (strcmp("Test", name)) {
			LOG((PI_DBG_USER, PI_DBG_LVL_ERR, "VFSTEST Label Mismatch\n"));
			goto error;
		}
		
		result = dlp_VFSVolumeInfo (sd, refs[i], &vfs);
		CHECK_RESULT(dlp_VFSVolumeInfo);
	}

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
#endif

	t1 = time (NULL);
	LOG((PI_DBG_USER, PI_DBG_LVL_INFO, "VFSTEST Ending at %s", ctime (&t1)));

 error:
	pi_close (sd);

	return 0;
}

