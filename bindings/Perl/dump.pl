#!/usr/bin/perl

use PDA::Pilot;
use Data::Dumper;

$socket = PDA::Pilot::openPort("/dev/cua3");

print "Now press the HotSync button\n";

$dlp = PDA::Pilot::accept($socket);

$db = $dlp->open("DatebookDB");

$i=0;
while(defined($r = $db->getRecord($i++))) {
	print Dumper($r);
}
