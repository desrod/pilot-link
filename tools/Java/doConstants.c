/*
 * doConstants.c:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. *
 */

#include <stdio.h>
#include "pi-source.h"
#include "pi-dlp.h"
#include "pi-socket.h"

FILE *f;

void
w(char *name, int value)
{
   fprintf(f, "	public static int %s = %d;\n", name, value);
}

#define W(x) w(#x, x);

int
main(int argc, char *argv[])
{
   f = fopen("Pdapilot/constants.java", "w");

   fprintf(f, "\
 package Pdapilot; public class constants { ");
   W(dlpDBFlagResource) W(dlpDBFlagReadOnly)
      W(dlpDBFlagAppInfoDirty)
      W(dlpDBFlagBackup)
      W(dlpDBFlagOpen)
      W(dlpDBFlagNewer)
      W(dlpDBFlagReset)
      W(dlpDBFlagCopyPrevention)
      W(dlpDBFlagStream)

      W(dlpRecAttrDeleted)
      W(dlpRecAttrDirty)
      W(dlpRecAttrBusy)
      W(dlpRecAttrSecret)
      W(dlpRecAttrArchived)

      W(dlpOpenRead)
      W(dlpOpenWrite)
      W(dlpOpenExclusive)
      W(dlpOpenSecret)
      W(dlpOpenReadWrite)

      W(dlpEndCodeNormal)
      W(dlpEndCodeOutOfMemory)
      W(dlpEndCodeUserCan)
      W(dlpEndCodeOther)

      W(dlpDBListRAM)
      W(dlpDBListROM)

      W(PILOT_LINK_VERSION)
      W(PILOT_LINK_MAJOR)
      W(PILOT_LINK_MINOR)

      W(PI_AF_SLP)
      W(PI_AF_INETSLP)
      W(PI_PF_SLP)
      W(PI_PF_PADP)
      W(PI_PF_LOOP)
      W(PI_SOCK_STREAM)
      W(PI_SOCK_DGRAM)
      W(PI_SOCK_RAW)
      W(PI_SOCK_SEQPACKET)

      W(AF_INET)

      fprintf(f, "}\n");
   return 0;
}
