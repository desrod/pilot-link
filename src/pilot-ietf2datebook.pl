#!/usr/bin/env perl 

######################################################################
# ietf2datebook.pl -- Convert IETF agenda to suitable format for
#		      install-datebook
#
# Copyright (c) 1997 Tero Kivinen
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc., 59
# Temple Place, Suite 330, Boston, MA 02111-1307 USA
######################################################################

use strict;
# use warnings;
# use diagnostics;

print <DATA>;

__DATA__;
# -*- perl -*-

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
