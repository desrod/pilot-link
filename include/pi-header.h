void PalmHeader(char *progname)
{
   fprintf(stderr,"
   ---o-o---  The Palm name and logo are copyright Palm Computing, Inc.
   ---o--o--  and subsidiaries. All rights reserved.
   ---o-o---\n");
#ifdef PILOT_LINK_PATCH
   fprintf(stderr,"   ---o-----  This is %s from pilot-link version %d.%d.%d.%d
   ---o-----\n", progname, PILOT_LINK_VERSION, PILOT_LINK_MAJOR, PILOT_LINK_MINOR, PILOT_LINK_PATCH);
   fprintf(stderr,"   ---o-----  pilot-link %d.%d.%d is licensed under the GPL\n               See the file COPYING for more details.\n\n", PILOT_LINK_VERSION, PILOT_LINK_MAJOR, PILOT_LINK_MINOR, PILOT_LINK_PATCH);
#else
   fprintf(stderr,"   ---o-----  This is %s from pilot-link version %d.%d.%d
   ---o-----\n", progname, PILOT_LINK_VERSION, PILOT_LINK_MAJOR, PILOT_LINK_MINOR);
   fprintf(stderr,"   ---o-----  pilot-link %d.%d.%d is covered under the GPL\n              See the file COPYING for more details.\n\n", PILOT_LINK_VERSION, PILOT_LINK_MAJOR, PILOT_LINK_MINOR);
#endif
}
