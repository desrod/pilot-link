/*
 * pi-header.c:  Splash for the version/etc. 
 *
 * Copyright (c) 2000, David A. Desrosiers
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
