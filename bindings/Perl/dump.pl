#!/usr/bin/env perl

use strict;
# use warnings;
# use diagnostics;
use PDA::Pilot;
use Data::Dumper;

print "Now press the HotSync button (defaults to /dev/pilot)\n";
my $socket 	= PDA::Pilot::openPort("/dev/ttyqe") or die "$!";
my $dlp 	= PDA::Pilot::accept($socket);
my $db 		= $dlp->open("DatebookDB");
my $r;
my $i		= 0;

while(defined($r = $db->getRecord($i++))) {
	print Dumper($r);
}
