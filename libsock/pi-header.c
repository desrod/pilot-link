#include <stdio.h>

#include "pi-version.h"

void
PalmHeader(char *progname)
{
   fprintf(stderr, "\n");
   fprintf(stderr, "   ---o-o---  The Palm name and logo are copyright Palm Computing, Inc.\n");
   fprintf(stderr, "   ---o--o--  and subsidiaries. All rights reserved.\n");
   fprintf(stderr, "   ---o-o---\n");
#ifdef PILOT_LINK_PATCH
   fprintf(stderr, "   ---o-----  This is %s from pilot-link version %d.%d.%d%s\n", progname, PILOT_LINK_VERSION, PILOT_LINK_MAJOR, PILOT_LINK_MINOR, PILOT_LINK_PATCH);
   fprintf(stderr, "   ---o-----\n");
   fprintf(stderr, "   ---o-----  pilot-link %d.%d.%d%s is covered under the GPL\n", PILOT_LINK_VERSION, PILOT_LINK_MAJOR, PILOT_LINK_MINOR, PILOT_LINK_PATCH);
#else
   fprintf(stderr, "   ---o-----  This is %s from pilot-link version %d.%d.%d\n", progname, PILOT_LINK_VERSION, PILOT_LINK_MAJOR, PILOT_LINK_MINOR);
   fprintf(stderr, "   ---o-----\n");
   fprintf(stderr, "   ---o-----  pilot-link %d.%d.%d is covered under the GPL\n", PILOT_LINK_VERSION, PILOT_LINK_MAJOR, PILOT_LINK_MINOR);
#endif
   fprintf(stderr, "              See the file COPYING for more details.\n\n");
}
