use Config;
print $Config{startperl}, " -w\n";
print <DATA>;
__DATA__;
# -*- perl -*-
######################################################################
# ietf2datebook.pl -- Convert IETF agenda to suitable format for
#		      install-datebook 
# Copyright (c) 1997 Tero Kivinen
#
# This is free software, licensed under the GNU Public License V2.
# See the file COPYING for details.
######################################################################
#         Program: ietf2datebook.pl
#	  $Source$
#	  Author : $Author$
#
#	  (C) Tero Kivinen 1997 <kivinen@iki.fi>
#
#	  Creation          : 03:20 Aug  8 1997 kivinen
#	  Last Modification : 04:13 Aug  8 1997 kivinen
#	  Last check in     : $Date$
#	  Revision number   : $Revision$
#	  State             : $State$
#	  Version	    : 1.4
#	  Edit time	    : 3 min
#
#	  Description       : Convert IETF agenda to suitable format for
#		      	      install-datebook 
#
######################################################################

while (<>) {
    chomp;
    if (/^(MONDAY|TUESDAY|WEDNESDAY|THURSDAY|FRIDAY|SATURDAY|SUNDAY)\s*,\s*(January|February|March|April|June|July|August|September|October|November|December)\s*(\d+)\s*,\s*(\d+)\s*$/) {
	$date = "$2 $3, $4";
    } elsif (/^(\d\d\d\d)-(\d\d\d\d)\s*(.*)$/) {
	$timestart = $1;
	$timeend = $2;
	$header = $3;
	printf("$date $timestart GMT+300\t$date $timeend GMT+300\t\t$header\n");
    } elsif (/^\s*$/) {
    } elsif (/^\s*(.*)$/) {
	printf("$date $timestart GMT+300\t$date $timeend GMT+300\t\t$header: $1\n");
    } else {
	die "Internal error";
    }
}
