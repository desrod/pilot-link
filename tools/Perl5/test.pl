sub FooBar { print "Foo: $_[0]\n"; $x = $_[0]; $x =~ s/[aeiou]/\U$&/g; return $x };

use strict;
# use diagnostics;
use PDA::Pilot;
use Data::Dumper;

my ($port, $db, $socket, $app, $dlp, $ui, $info, $rec);

if ($ARGV[0]) {
	$port = $ARGV[0];
} else {
	print "What port should I use [/dev/pilot]: ";
	$port = <STDIN>;
	chop $port;
	$port ||= "/dev/pilot";
}

$socket = PDA::Pilot::openPort($port);

# openPort is the equivalent of
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

print Dumper($dlp);

my @pref = $dlp->getPref('mail', 3);
@pref = "not available" if not defined $pref[0];

print "Mail preferences: @pref\n";

$ui = $dlp->getUserInfo;

my @b = $dlp->getBattery;

$info = $dlp->getDBInfo(0);

print Dumper($info);
print "Battery voltage is $b[0], (warning marker $b[1], critical marker $b[2])\n";

$dlp->tickle;

$db = $dlp->open("MemoDB");

print "db class is ", ref $db, "\n";

$rec = $db->getRecord(0);

print "Contents: '$rec->{text}'\n";

$app = $db->getAppBlock;

print Dumper($app);
print "Categories: @{$app->{categoryName}}\n";
print Dumper($db->getPref(0));
print Dumper($db->getPref(1));

$db->close();

$db = $dlp->open("DatebookDB");

print "db class is ", ref $db, "\n";

$rec = $db->getRecord(0);

print "Contents: ", Dumper($rec);

$app = $db->getAppBlock;

print Dumper($app);
print "Categories: @{$app->{categoryName}}\n";

$db->close();

$db = $dlp->open("MailDB");

if ($db) {
	print "db class is ", ref $db, "\n";
	$rec = $db->getRecord(0);
	print "Contents: ", Dumper($rec);
	$app = $db->getAppBlock;
	print Dumper($app);
	print "Categories: @{$app->{categoryName}}\n";
	$db->close();
}

$db = $dlp->open("ExpenseDB");
if ($db) {
	print "db class is ", ref $db, "\n";
	$rec = $db->getRecord(0);
	print "Contents: ", Dumper($rec);
	$app = $db->getAppBlock;
	print Dumper($app);
	print "Categories: @{$app->{categoryName}}\n";
}

undef $db; 	# Close database
undef $dlp; 	# Close connection
print "Your name is $ui->{name}\n";
