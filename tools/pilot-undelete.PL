use Config;
print $Config{startperl}, "\n";
print <DATA>;
__DATA__;

use PDA::Pilot;
use Getopt::Std;

$opts{p} = $ENV{PILOTPORT} if length $ENV{PILOTPORT};

if (not getopts('p:d:',\%opts) or not exists $opts{p} or not exists $opts{d}) {
	print "Usage: $0 -p port -d dbname\n";
	print "\n  $0 will scan through dbname on your Pilot and turn all archived\n";
	print "  records into normal records, thus \"undeleting\" them.\n";
	exit;
}

$socket = PDA::Pilot::openPort($opts{p});

print "Please start HotSync on port $opts{p} now.\n";

$dlp = PDA::Pilot::accept($socket);

if (defined $dlp) {
	print "\nConnection established. Opening $opts{d}...\n";
	
	$dlp->getStatus;
	
	$db = $dlp->open($opts{d}, "rwsx");
	
	if (defined $db) {
		print "\nDatabase opened.\n";
		
		$i = 0;
		$c = 0;
		for(;;) {
			$r = $db->getRecord($i);
			last if not defined($r); #no more records
			if ($r->{archived}) {
				print "Record $i is archived, un-archiving.\n";
				$r->{archived} = 0;
				$r->{deleted} = 0;
				$db->setRecord($r); # Re-store record
				
				$c++;
			}
			$i++;
		}
		
		$db->close;
		print "Done. $c record", ($c == 1 ? "" : "s"), " unarchived.\n";
	} else {
		print "Unable to open database\n";
	}
	
	$dlp->close;
}
PDA::Pilot::close($socket);
exit(0);
