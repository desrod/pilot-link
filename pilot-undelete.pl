
use PDA::Pilot;
use Getopt::Std;

$opts{p} = $ENV{PILOTPORT} if length $ENV{PILOTPORT};

if (not getopts('p:d:',\%opts) or not exists $opts{p} or not exists $opts{d}) {
	print "Usage: $0 -p port -d dbname\n";
	print "\n  $0 will scan through dbname on your Pilot and turn all archived\n";
	print "  records into normal records, thus \"undeleting\" them.\n";
	exit;
}

$socket = PDA::Pilot::OpenPort($opts{p});

print "Please start HotSync on port $opts{p} now.\n";

$dlp = PDA::Pilot::Accept($socket);

if (defined $dlp) {
	print "\nConnection established. Opening $opts{d}...\n";
	
	$dlp->Status;
	
	$db = $dlp->Open($opts{d}, 0x40|0x80);
	
	if (defined $db) {
		print "\nDatabase opened.\n";
		
		$i = 0;
		$c = 0;
		for(;;) {
			@r = $db->GetRecord($i);
			last if not defined($r[0]); #no more records
			if ($r[3] & 0x08) {
				print "Record $i is archived, un-archiving.\n";
				#Archived
				$r[3] &= ~0x88; # Clear deleted and archived attribute
				$db->SetRecord($r[0], $r[2], $r[3], $r[4]); # Re-store record
				
				$c++;
			}
			$i++;
		}
		
		$db->Close;
		print "Done. $c record", ($c == 1 ? "" : "s"), " unarchived.\n";
	} else {
		print "Unable to open database\n";
	}
	
	$dlp->Close;
}
PDA::Pilot::Close($socket);
exit(0);
