sub FooBar { print "Foo: $_[0]\n"; $x = $_[0]; $x =~ s/[aeiou]/\U$&/g; return $x };

use PDA::Pilot;

if ($ARGV[0]) {
	$port = $ARGV[0];
} else {
	print "What port should I use [/dev/cua3]: ";
	$port = <STDIN>;
	chop $port;
	$port ||= "/dev/cua3";
}

$socket = PDA::Pilot::openPort($port);

# OpenPort is the equivalent of
#
#$socket = PDA::Pilot::socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP);
#
#PDA::Pilot::bind($socket,
#	{family => PI_AF_SLP, device => $port});
#
#PDA::Pilot::listen($socket, 1);

print "Now press the HotSync button\n";

$dlp = PDA::Pilot::accept($socket);

$PDA::Pilot::UnpackPref{mail}->{3} = sub { $_[0] . "x"};

@pref = $dlp->getPref('mail', 3);

@pref = "Not available" if not defined $pref[0];

print "Mail preferences: @pref\n";

$ui = $dlp->getUserInfo;

@b = $dlp->getBattery;

print "Battery voltage is $b[0], (warning marker $b[1], critical marker $b[2])\n";

$dlp->tickle;

$db = $dlp->open("MemoDB");

print "db class is ", ref $db, "\n";

$r = $db->getRecord(0);

print "Contents: '$r->{text}'\n";

use Data::Dumper;

$app = $db->getAppBlock;

print Dumper($app);

print "Categories: @{$app->{categoryName}}\n";

#@r = $db->getResource(0);

#print "Resource: @r, error: ", ($db->errno()),"\n";

undef $db; # Close database

undef $dlp; # Close connection

print "Your name is $ui->{name}\n";
