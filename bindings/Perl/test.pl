#!/usr/bin/env perl 

use strict;
use warnings;
# use diagnostics;
use PDA::Pilot;
use Data::Dumper;

# Test::Harness will hijack STDERR and STDOUT, waiting for an 'ok' on a test
# result. If you run this non-interatively, it will seem to hang, just run
# it interactively to test it until this test is corrected.

my ($port,      # device port
    $db,        # database to dump
    $socket,    # socket descriptor
    $app,       # AppInfo block
    $dlp,       # sd = pilot_connect(port);
    $ui,        # dlp_ReadUserInfo
    $info,      # dlp_ReadOpenDBInfo
    $rec);      # record count

if ($ARGV[0]) {
        $port = $ARGV[0];
} else {
        print "What port should I use [/dev/pilot]: ";
        $port = <STDIN>;
        chomp $port;
        $port ||= "/dev/pilot";
}

$socket = PDA::Pilot::openPort($port) or die "$!";

#
# openPort is the equivalent of:
#
# $socket = PDA::Pilot::socket(PI_AF_PILOT, PI_SOCK_STREAM, PI_PF_DLP);
# PDA::Pilot::bind($socket, {family => PI_AF_PILOT, device => $port});
# PDA::Pilot::listen($socket, 1);
#

print "Please press the HotSync button\n";

$dlp = PDA::Pilot::accept($socket) or die "$!";

print Dumper($dlp);

# dump_sysinfo();
dump_memopad();
dump_datebook();
dump_mail();
dump_expense();

undef $db; 	# Close database
undef $dlp; 	# Close connection

############################################################
# 
# Read UserInfo, SysInfo, DBInfo
#
############################################################
sub dump_sysinfo {
	$ui = $dlp->getUserInfo;
	my @b = $dlp->getBattery;
	$info = $dlp->getDBInfo(0);

	print "-"x40, "\n";
	print "Dumping SysInfo\n";
	print "-"x40, "\n";
	print Dumper($info);

	print "Battery voltage is $b[0], (warning marker $b[1], critical marker $b[2])\n";
	print "Your name is $ui->{name}\n";
}


############################################################
#
# Dump MemoDB.pdb
#
############################################################
sub dump_memopad {
	$db = $dlp->open("MemoDB") or die "$!";

	if ($db) {
		print "-"x40, "\n";
		print "Dumping MemoDB\n";
		print "-"x40, "\n";
		print "db class is ", ref $db, "\n";

		# $rec = $db->getRecord(0);
		# print "Contents: '$rec->{text}'\n";

		my $app = $db->getAppBlock;

                # Dump all records to STDOUT
		# print Dumper($app);

		print "Categories: @{$app->{categoryName}}\n";
		print Dumper($db->getPref(0));
		print Dumper($db->getPref(1));
		$db->close();
	}
}

############################################################
#
# Dump DatebookDB.pdb
#
############################################################
sub dump_datebook {
	$db = $dlp->open("DatebookDB") or die "$!";
	if ($db) {
        	print "-"x40, "\n";
	        print "Dumping DatebookDB\n";
        	print "-"x40, "\n";
		print "db class is ", ref $db, "\n";

		# $rec = $db->getRecord(0);
		# print "Contents: ", Dumper($rec);

		my $app = $db->getAppBlock;

                # Dump all records to STDOUT
		# print Dumper($app);

		print "Categories: @{$app->{categoryName}}\n";

		$db->close();
	}
}

############################################################
#
# Dump MailDB.pdb
#
############################################################
sub dump_mail {
	$PDA::Pilot::UnpackPref{mail}->{3} = sub { $_[0] . "x"};
	#
	# $pref[0] == "Confirm deleted message"
	# $pref[1] == "Default Mail"
	# $pref[2] == "Signature text"
	#
	my @pref = $dlp->getPref('mail', 0);
	@pref = "not available" if (!defined $pref[0]);
	print "Mail preferences: @pref\n";


	$db = $dlp->open("MailDB") or die "$!";
	if ($db) {
	        print "-"x40, "\n";
        	print "Dumping MailDB\n";     
	        print "-"x40, "\n";
        	print "db class is ", ref $db, "\n";

		# $rec = $db->getRecord(0);
		# print "Contents: ", Dumper($rec);

		$app = $db->getAppBlock;

		# Dump all records to STDOUT
		# print Dumper($app);
		$db->close();
	}
}

############################################################
#
# Dump ExpenseDB.pdb
#
############################################################
sub dump_expense {
	$db = $dlp->open("ExpenseDB") or die "$!";
	if ($db) {
                print "-"x40, "\n";
                print "Dumping ExpenseDB\n";
                print "-"x40, "\n";
                print "db class is ", ref $db, "\n";

		# $rec = $db->getRecord(0);
		# print "Contents: ", Dumper($rec);

		$app = $db->getAppBlock;

                # Dump all records to STDOUT
		# print Dumper($app);

		print "Categories: @{$app->{categoryName}}\n";
	}
}

