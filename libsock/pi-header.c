#include <stdio.h>
#include "pi-version.h"

void PalmHeader(char *progname)
{
        char *patchlevel = "";

#ifdef PILOT_LINK_PATCH
        patchlevel = PILOT_LINK_PATCH;
#endif

        fprintf(stderr, "\n");
        fprintf(stderr, "   (c) Copyright 1996-2001, pilot-link team \n");
        fprintf(stderr,
                "       Join the pilot-unix list to contribute.\n\n");
        fprintf(stderr,
                "   This is %s from pilot-link version %d.%d.%d%s\n\n",
                progname, PILOT_LINK_VERSION, PILOT_LINK_MAJOR,
                PILOT_LINK_MINOR, patchlevel);
        fprintf(stderr,
                "   pilot-link %d.%d.%d%s is covered under the GPL\n",
                PILOT_LINK_VERSION, PILOT_LINK_MAJOR, PILOT_LINK_MINOR,
                patchlevel);
        fprintf(stderr, "   See the file COPYING for more details.\n\n");
}
