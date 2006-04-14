use Config;
print $Config{startperl}, "\n";
print <DATA>;
__DATA__;

use IO::Socket;
use IO::Select;
use Time::Local;
use Digest::MD5;
use PDA::Pilot;
use Carp;
use strict;

my ($controldir, $dlp, $info, $db, $port);
my (%control, %pilothash, %pilotID, %planID, %exceptID, %planRecord,
    %dbname, %sawName);
my ($slowsync, $file, $pilotname, $maxseed, $netplanversion);

my $PREFS = {
	     NetplanPort => 5444,
	     Debug       => 1,
	    };

my @plversion;          # pilot-link version (version, major, minor, patch)

# any or alll of these may be undefined, depending on the
# pilot-link version.
eval {
    $plversion[0] = PDA::Pilot::PILOT_LINK_VERSION();
    $plversion[1] = PDA::Pilot::PILOT_LINK_MAJOR();
    $plversion[2] = PDA::Pilot::PILOT_LINK_MINOR();
    $plversion[3] = PDA::Pilot::PILOT_LINK_PATCH();
};

# msg and status are here to localize the differences between the
# standalone sync-plan.PL and the SyncPlan.pm module for PilotManager.

############################################################
#
############################################################
sub msg {
  print @_;
}

sub status {
}

############################################################
# CheckErrNotFound: Argument is a PDA::Pilot::DLP or a
# PDA::Pilot::DLP::DB.  It's in its own package so that croak will
# give more useful information.  I'm not using the equivalent function
# from the PilotMgr package because there is a stand-alone version of
# this conduit in the pilot-link distribution.
############################################################
BEGIN {
  package ErrorCheck;
  use Carp;
  sub checkErrNotFound
    {
      my($obj) = @_;
      my $errno = $obj->errno();
      if (defined $plversion[0]) { # pilot-link version is >= 0.12.0-pre2
        if ($errno != PDA::Pilot::PI_ERR_DLP_PALMOS()) {
	  croak "Error $errno";
        }
        if (($errno = $obj->palmos_errno()) != PDA::Pilot::dlpErrNotFound()) {
	  croak "Error $errno: " . PDA::Pilot::errorText($errno);
        }
      } else {
        croak "Error $errno" if ($errno != -5); # dlpErrNotFound
      }
    }
}
*checkErrNotFound = \&ErrorCheck::checkErrNotFound;


############################################################
#
############################################################
sub DatePlanToPerl {
	my ($PlanDate)	= @_;
	my ($m,$d,$y)	= split(m!/!,$PlanDate);
	if ($y < 40) {
		$y += 100;
	}
	if ($y > 1900) {
		$y -= 1900;
	}
	$m--;

	timegm(0,0,0,$d,$m,$y);
}

############################################################
#
############################################################
sub TimePlanToPerl {
	my ($PlanTime)	= @_;
	my ($h,$m,$s)	= split(m!:!,$PlanTime);
	
	return undef if $h == 99 and $m == 99 and $s == 99;
	
	$s + ($m * 60) + ($h * 60 * 60);
}

############################################################
#
############################################################
sub TimePerlToPlan {
	my ($PerlDT) = @_;
	return "99:99:99" if not defined $PerlDT;

	my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) =
	    gmtime($PerlDT);
	
	"$hour:$min:$sec";
}

############################################################
#
############################################################
sub TimeRelPerlToPlan {
	my ($PerlDT) = @_;
	return "99:99:99" if not defined $PerlDT;

	my ($sec,$min,$hour);
	
	$hour = int($PerlDT/ (60*60));
	$PerlDT -= $hour*60*60;

	$min = int($PerlDT/ (60));
	$PerlDT -= $min*60;

	$sec = int($PerlDT);
	$PerlDT -= $sec;
	
	"$hour:$min:$sec";
}

############################################################
#
############################################################
sub DatePilotToPerl {
	my ($s,$m,$h, $mday,$mon,$year) = @_;

	if (ref $s eq 'ARRAY') {
	    ($s,$m,$h, $mday,$mon,$year) = @$s;
	}
	my ($date, $time);

	if ($year >= 70 and $year <= 138) {
	    $date = eval { timegm($s,$m,$h,$mday,$mon,$year) };
	    msg("Trouble converting date: $mon/$mday/$year $h:$m$s")
	      if $@;
	    $time = $s + 60 * ($m + 60 * $h);
	}
	else {
	    msg("Bad year: $year");
	}

	return wantarray ? ($date, $time) : $date;
}

############################################################
#
############################################################
sub DatePerlToPlan {
	my ($PerlDT) = @_;
	my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) =
	    gmtime($PerlDT);
	
	$year += 1900;
	$mon++;
	
	"$mon/$mday/$year";
}

############################################################
#
############################################################
sub RecordPlanToPilot {
	my ($plan,$pilot) = @_;
	if (not defined $pilot) {
		$pilot = PDA::Pilot::AppointmentDatabase->record;
	}
	
	$pilot->{'id'} = $plan->{'pilotid'};
	$pilot->{'description'} = join("\xA", @{$plan->{'note'}}) if defined $plan->{'note'};
	$pilot->{'note'} = join("\xA", @{$plan->{'message'}}) if defined $plan->{'message'};
	$pilot->{'description'} ||= "";

	if (defined $plan->{'time'}) {
		$pilot->{'begin'} = [gmtime($plan->{'date'}+$plan->{'time'})];
		$pilot->{'end'} = [gmtime($plan->{'date'}+$plan->{'time'}+$plan->{'length'})];
		$pilot->{'event'}=0;
	} else {
		$pilot->{'begin'} = [gmtime($plan->{'date'})];
		$pilot->{'event'}	= 1;
		$plan->{'early'} 	= 0;
		$plan->{'late'} 	= 0;
	}
	
	if ($plan->{'early'} and $plan->{'late'} and ($plan->{'early'} != $plan->{'late'})) {
		msg( "Two alarms - using earlier one." );
		$plan->{'late'} = $plan->{'early'};
	}
	if ($plan->{'early'} or $plan->{'late'}) {
		my ($alarm) = $plan->{'early'} || $plan->{'late'};
		if ($alarm > (60*60*24)) {
			$pilot->{'alarm'}->{'units'} = "days";
			$pilot->{'alarm'}->{'advance'} = int($alarm / (60*60*24));
		} elsif ($alarm > (60*60)) {
			$pilot->{'alarm'}->{'units'} = "hours";
			$pilot->{'alarm'}->{'advance'} = int($alarm / (60*60));
		} else {
			$pilot->{'alarm'}->{'units'} = "minutes";
			$pilot->{'alarm'}->{'advance'} = int($alarm / 60);
		}
	}
	
	if (defined $plan->{'exceptions'}) {
		foreach (@{$plan->{'exceptions'}}) {
			push @{$pilot->{'exceptions'}}, [gmtime($_)];
		}
	} else {
		delete $pilot->{'exceptions'};
	}

	if (defined $plan->{'repeat'}) {
		msg( "Converting repetition...\n" ) if ($PREFS->{'Debug'} > 2);
		delete $pilot->{'repeat'};
		if ($plan->{'repeat'}->[1]) {
			$pilot->{'repeat'}->{'end'} = [gmtime($plan->{'repeat'}->[1])];
		}
		my ($days,$end,$weekday,$mday,$yearly) = @{$plan->{'repeat'}};
		msg( "Days: $days, End: $end, Weekday: $weekday, Mday: $mday, Yearly: $yearly\n" ) if ($PREFS->{'Debug'} > 2);
		$pilot->{'repeat'}->{'weekstart'} = 0;
		$pilot->{'repeat'}->{'frequency'} = 1;
		if ($days and !$weekday and !$mday and !$yearly) {
			$pilot->{'repeat'}->{'type'} = "Daily";
			$pilot->{'repeat'}->{'frequency'} = $days / (60*60*24);
		} elsif(!$days and !$weekday and !$mday and $yearly) {
			$pilot->{'repeat'}->{'type'} = "Yearly";
		} elsif(!$days and !$weekday and ($mday == (1 << $pilot->{'begin'}[3])) and !$yearly) {
			$pilot->{'repeat'}->{'type'} = "MonthlyByDate";
			
		} elsif(!$days and $weekday and (($weekday & 0xff80) == 0) and !$mday and !$yearly) {
			$pilot->{'repeat'}->{'type'} = "Weekly";
			foreach my $i (0..6) {
				$pilot->{'repeat'}->{'days'}[$i] = !! ($weekday & (1<<$i));
			}
			# If the weekday list does include the day the event is one, abort
			if (!$pilot->{'repeat'}{'days'}[$pilot->{'begin'}[6]]) {
				return undef;
			}
		} elsif(not $days and $weekday and not $mday and not $yearly) {
			my ($wday) = $pilot->{'begin'}[6];
			my ($week) = int(($pilot->{'begin'}[3]-1)/7);
			msg( "weekday = $weekday, wday = $wday, week = $week\n" ) if ($PREFS->{'Debug'} > 2);
			if (($weekday & 0x7f) != (1<<$wday)) {
				return undef;
			}
			if (($weekday & 4096) and ($weekday & 8192)) {
				$weekday &= ~4096;
			}
			if ($week == 4) {
				$week = 5;
			}
			if (($weekday & 0xff00) != (256<<$week)) {
				return undef;
			}
			if ($week == 5) {
				$week = 4;
			}
			
			$pilot->{'repeat'}->{'type'} = "MonthlyByDay";
			$pilot->{'repeat'}->{'day'} = $week*7+$wday;
		} else {
			return undef;
		}
	} else {
		delete $pilot->{'repeat'};
	}
	
	$pilot;
}

############################################################
#
############################################################
sub RecordPilotToPlan {
	my ($pilot,$plan) = @_;
	$plan = {color => 0} if not defined $plan;
	
	$plan->{'pilotid'} = $pilot->{'id'};
	$plan->{'id'} ||= 0;
	$plan->{'message'} = [split("\xA", $pilot->{'note'})] if defined $pilot->{'note'};
	$plan->{'note'} = [split("\xA", $pilot->{'description'})] if defined $pilot->{'description'};

	my ($date, $time) = DatePilotToPerl($pilot->{'begin'});
	unless ($date) {
		msg("Begin time in Palm record untranslatable.");
		return undef;
	}

	$plan->{'date'} = $date;
	if ($pilot->{'event'}) {
		$plan->{'time'} = undef;
		$plan->{'length'} = 0;
	} else {
		$plan->{'time'} = $time;
		my $end = DatePilotToPerl($pilot->{'end'});
		unless ($end) {
		    msg("End time in Palm record untranslatable.");
		    return undef;
		}
		$plan->{'length'} = $end - $date;
	}
	
	if (exists $pilot->{'alarm'}) {
		my($alarm) = 0;
		if ($pilot->{'alarm'}{'units'} eq "days") {
			$alarm = $pilot->{'alarm'}->{'advance'} * (60*60*24);
		} elsif ($pilot->{'alarm'}{'units'} eq "hours") {
			$alarm = $pilot->{'alarm'}->{'advance'} * (60*60);
		} elsif ($pilot->{'alarm'}{'units'} eq "minutes") {
			$alarm = $pilot->{'alarm'}->{'advance'} * (60);
		}
		if ($plan->{'late'}) {
			$plan->{'late'} = $alarm;
			$plan->{'early'} = 0;
		} else {
			$plan->{'late'} = 0;
			$plan->{'early'} = $alarm;
		}
	} else {
		$plan->{'late'}=0;
		$plan->{'early'}=0;
	}
	
	if (exists $pilot->{'exceptions'}) {
		# Plan records can only deal with four exceptions, 
		if (@{$pilot->{'exceptions'}} > 4) {
			msg("Too many exceptions.");
			return undef;
		}
		foreach (@{$pilot->{'exceptions'}}) {
			push @{$plan->{'exceptions'}}, timegm(@{$_});
		}
	}

	delete $plan->{'repeat'};
	
	if (exists $pilot->{'repeat'}) {
		$plan->{'repeat'} = [0,0,0,0,0];
		if ($pilot->{'repeat'}->{'type'} eq "Daily") {
			$plan->{'repeat'}->[0] = (60*60*24) * $pilot->{'repeat'}->{'frequency'};
			$plan->{'repeat'}->[4] = 0;
		} elsif ($pilot->{'repeat'}->{'type'} eq "Yearly" and ($pilot->{'repeat'}->{'frequency'}==1)) {
			$plan->{'repeat'}->[4] = 1;
		
		} elsif ($pilot->{'repeat'}->{'type'} eq "Weekly" and ($pilot->{'repeat'}->{'frequency'}==1)) {
			my ($r) = 0;
			foreach my $i (0..6) {
				if ($pilot->{'repeat'}->{'days'}[$i]) {
					$r |= (1<<$i);
				}
			}
			$plan->{'repeat'}->[2] = $r;
		} elsif ($pilot->{'repeat'}->{'type'} eq "Weekly" and ($pilot->{'repeat'}->{'frequency'}>1)) {
		        # Weekly repeat, not every week.  If it repeats only once per week, convert it to a daily
		        # repeat with frequency a multiple of 7.  If it repeats more than once a week, bail.
			my $count = 0;
			foreach my $i (0..6) {
				$count ++ if ($pilot->{repeat}->{days}[$i]);
			}
			if ($count == 1) {
				$plan->{'repeat'}->[0] = (60*60*24) * $pilot->{'repeat'}->{'frequency'} * 7;
				$plan->{'repeat'}->[4] = 0;
			} else {
				msg("Repeat pattern too complex.");
				return undef;
			}
		} elsif ($pilot->{'repeat'}->{'type'} eq "MonthlyByDate" and ($pilot->{'repeat'}->{'frequency'}==1)) {
			$plan->{'repeat'}->[3] = 1 << $pilot->{'begin'}[3];
		} elsif ($pilot->{'repeat'}->{'type'} eq "MonthlyByDay" and ($pilot->{'repeat'}->{'frequency'}==1)) {
			my ($day) = $pilot->{'repeat'}{'day'} % 7;
			my ($week) = int($pilot->{'repeat'}{'day'} / 7);
			$week = 5 if $week == 4;
			$plan->{'repeat'}->[2] = (1 << $day) | (256 << $week);
		} else {
			msg("Repeat pattern too complex.");
			return undef;
		}
		if (defined $pilot->{'repeat'}->{'end'}) {
			$plan->{'repeat'}->[1] = timegm(@{$pilot->{'repeat'}->{'end'}});
		}
	}
	
	$plan;
}

############################################################
#
############################################################
sub generaterecord {
	my ($rec) = @_;
	my (@output);
	
	#print "Generating Plan record: ", Dumper($rec),"\n";

	push(@output, DatePerlToPlan($rec->{'date'})." ".
				TimeRelPerlToPlan($rec->{'time'})." ".
				TimeRelPerlToPlan($rec->{'length'})." ".
				TimeRelPerlToPlan($rec->{'early'})." ".
				TimeRelPerlToPlan($rec->{'late'})." ".
				($rec->{'suspended'} ? "S" : "-").
				($rec->{'private'} ? "P" : "-").
				($rec->{'noalarm'} ? "N" : "-").
				($rec->{'hide_month'} ? "M" : "-").
				($rec->{'hide_year'} ? "Y" : "-").
				($rec->{'hide_week'} ? "W" : "-").
				($rec->{'hide_yearover'} ? "O" : "-").
				($rec->{'d_flag'} ? "D" : "-").
				"-".
				"-".
				" ".$rec->{'color'});

	if (defined $rec->{'repeat'}) {
		push @output, "R\t".join(" ",@{$rec->{'repeat'}});
	}
	if (defined $rec->{'exceptions'}) {
		foreach (@{$rec->{'exceptions'}}) {
			push @output, "E\t".DatePerlToPlan($_);
		}
	}
	if (defined $rec->{'note'}) {
		push @output, map("N\t$_", @{$rec->{'note'}});
	}
	if (defined $rec->{'message'}) {
		push @output, map("M\t$_", @{$rec->{'message'}});
	}
	if (defined $rec->{'script'}) {
		push @output, map("S\t$_", @{$rec->{'script'}});
	}
	if (defined $rec->{'other'}) {
		foreach (@{$rec->{'other'}}) {
			push @output, $_;
		}
	}

	my ($hash) = new Digest::MD5;
	foreach (@output) {
		#print "Adding |$_| to hash\n";
		$hash->add($_);
	}
	$rec->{'pilothash'} = $hash->hexdigest;
	{
		my ($i);
		for ($i=0;$i<@output;$i++) {
			last if $output[$i] =~ /^S/;
		}
		$rec->{'pilotexcept'} += 0;
		my (@US);
		@US = @{$rec->{'unhashedscript'}} if defined $rec->{'unhashedscript'};
		unshift @US, "S\t#Pilot: 1 $pilotname $rec->{'pilothash'} $rec->{'pilotexcept'} $rec->{'pilotid'}";
		splice @output, $i, 0, @US;
	}
	
	msg( "Generated record |" . join("\n", @output). "|\n" ) if ($PREFS->{'Debug'} > 2);

	join("\n",@output);
}

############################################################
#
############################################################
sub PrintPlanRecord {
	my ($rec) = @_;
	my ($output);
	
	my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) =
	    gmtime($rec->{'date'});
	$year += 1900;
	$mon++;
	$output = "$year/$mon/$mday";

	if ($rec->{'time'}) {
		my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = 
		  gmtime($rec->{'time'});
		$output .= sprintf(" %02d:%02d-", $hour, $min);

		($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) =
		  gmtime($rec->{'time'}+$rec->{'length'});
		$output .= sprintf("%02d:%02d", $hour, $min);
	}
	$output .= " '".join("\\n",@{$rec->{'note'}})."'" if defined $rec->{'note'};
	$output .= " (".join("\\n",@{$rec->{'message'}}).")" if defined $rec->{'message'};
	
	if ($rec->{'repeat'}) {
		my (@r);
		if ($rec->{'repeat'}[0]) {
			push @r, "every " . ($rec->{'repeat'}[0] / (60*60*24)) . " days";
		}
		
		if ($rec->{'repeat'}[4]) {
			push @r, "every year";
		}
		if ($rec->{'repeat'}[3]) {
			my ($i) = $rec->{'repeat'}[3];
			if ($i & 1) {
				push @r, "the last day of each month";
			}
			foreach (1..31) {
				push @r, "the $_ of each month" if $i & (1<<$_);
			}
		}
		if ($rec->{'repeat'}[2]) {
			push @r, "until ".scalar(gmtime($rec->{'repeat'}[2]));
		}
		if (@r) {
			$output .= " repeat ".join(", ", @r);
		}
	}
	
# 	$output .= " {ID:$rec->{'pilotid'}, Except:";
# 	$output .= $rec->{'pilotexcept'} if (defined $rec->{'pilotexcept'});
# 	$output .= ", Changed:";
# 	$output .= $rec->{'modified'} if (defined $rec->{'modified'});
# 	$output .= ", Deleted:";
# 	$output .= $rec->{'deleted'} if (defined $rec->{'deleted'});
# 	$output .= "}";
	
	$output;
}

############################################################
#
############################################################
sub PrintPilotRecord {
	my ($rec) = @_;
	my ($output);
	
	$output = ($rec->{'begin'}[5]+1900)."/".($rec->{'begin'}[4]+1)."/".$rec->{'begin'}[3];
	
	if (!$rec->{'event'}) {
		$output .= " ";
		$output .= sprintf("%02d:%02d-%02d:%02d",
				   $rec->{'begin'}[2],
				   $rec->{'begin'}[1],
				   $rec->{'end'}[2],
				   $rec->{'end'}[1]);
	}
	
	$output .= " '$rec->{'description'}'";
	$output .= " ($rec->{'message'})" if (defined $rec->{'message'});
	
# 	$output .= " {ID:$rec->{'id'}, Except:";
# 	$output .= $exceptID{$rec->{'id'}} if (defined $exceptID{$rec->{'id'}});
# 	$output .= ", Changed:";
# 	$output .= $rec->{'modified'} if (defined $rec->{'modified'});
# 	$output .= ", Deleted:";
# 	$output .= $rec->{'deleted'} if (defined $rec->{'deleted'});
# 	$output .= "}";

	$output =~ s/\r/\\r/g;
	$output =~ s/\n/\\n/g;
	
	$output;
}

############################################################
#
# Takes a Plan record in hash format
# 
############################################################
sub WritePlanRecord {
	my ($socket, $record) = @_; 
	my ($raw) = generaterecord($record);
	my ($reply);
	$record->{'id'} ||= 0;
	#print "ID is $record->{'id'}\n";
	$raw =~ s/\n/\\\n/g;
	$raw = "w$file $record->{'id'} $raw\n";
	$record->{'raw'} = $raw;
	SendPlanCommand($socket, $raw);
	$reply = ReadPlanReply($socket);
	#print "Installing record $record->{'id'} (PilotID: $record->{'pilotid'}) in Plan: ", Dumper($record);
#	syswrite $socket, $raw, length($raw);
#	sysread $socket, $reply, 1024;
#	print "Reply to installation: |$reply|\n";
	if ($reply =~ /^w[tf](\d+)/) {
		$record->{'id'} = $1;
		$planRecord{$1} = $record;
#		print "New record id: $1\n";
	} else {
		msg( "Failed write: $reply\n" );
	}	
}


############################################################
#
############################################################
sub LoadPilotRecord {
	my ($db, $i) = @_;
	my ($record) = $db->getRecord($i);
	if ($record) {
		$pilotID{$record->{'id'}} = $record;
	} else {
	        checkErrNotFound($db);
	}
	$record;
}

############################################################
#
# takes a Plan record in hash format
#
############################################################
sub DeletePlanRecord {
	my ($socket, $record) = @_; 
	my ($raw);
	$raw = "d$file $record->{'id'}\n";
#	print "Deleting record $record->{'id'} (PilotID: $record->{'pilotid'}) in Plan\n";
#	syswrite $socket, $raw, length($raw);
	SendPlanCommand($socket, $raw);
}

############################################################
#
# takes a Palm record in hash format
#
############################################################
sub WritePilotRecord {
	my ($db, $control, $record) = @_; 
	
	$record->{'id'} ||= 0;
	$record->{'category'} ||= 0;
	
	#print "Installing record in Palm: ",Dumper($record);
	
	my ($id) = $db->setRecord($record);
	
	if ($id) {
		$pilotID{$id} 	= $record;
		my ($hash) 	= HashPilotRecord($record);						
		$pilothash{$id} = $hash;
		$dbname{$id} 	= $control->{'name'};
		$record->{'id'} = $id;
		$exceptID{$id} 	= 0;
	}
	
	$id;
}

############################################################
#
############################################################
sub DeletePilotRecord {
	my ($db, $id) = @_; 
	my ($result) = $db->deleteRecord($id);
	if ($result>=0) {
		delete $pilothash{$id};
		delete $pilotID{$id};
		delete $dbname{$id};
		delete $exceptID{$id};
	}
	$result;
}


$maxseed = 0;

############################################################
#
############################################################
sub dorecord {
	my ($db,$socket,$control, $i,$r) = @_;
#	print "Record: $r\n";
	my (@l) = split(/\n/,$r);
	my ($rec) = { raw => [@l], other => [] };
	my (@E,@R,@N,@M,@S,@US);
	my ($hash) = new Digest::MD5;
	$l[0] =~ s/\s+/ /g;
	$hash->add($l[0]);
	my ($date, $time, $length, $early, $late, $flags, $color) = split(/\s+/, shift @l);
	$rec->{'pilotrec'} = "";
	foreach (@l) {
		if (/^E\t/) {
			push @E, $';
		} elsif (/^M\t/) {
			push @M, $';
		} elsif (/^N\t/) {
			push @N, $';
		} elsif (/^S\t/) {
			my ($s) = $';
			if ($s =~ /^\s*#Pilot:\s+(\d+)\s*(.*)$/) {
				if ($1 == 1) { # version number
					my ($name,$hash,$except,$id) = split(/\s+/, $2);
					#print Dumper({Name=>$name,Hash=>$hash,Except=>$except,ID=>$id});
					if ($name eq $pilotname) {
						$rec->{'pilotid'} = $id;
						$rec->{'pilotexcept'} = $except || 0;
						$rec->{'pilothash'} = $hash;
						$planID{$id} = $rec;
						next; 
					}
				}
				push @US, $_;
				next; # skip hash add
			} else {
				push @S, $s;
			}
		} elsif (/^R\t/) {
			my ($r) = $';
			$r =~ s/\s+/ /g;
			$rec->{'repeat'} = [split(/\s+/, $r)];
		} else {
			push @{$rec->{'other'}}, $_;
		}
		#print "Adding |$_| to hash\n";
		$hash->add($_);
	}
	$hash = $hash->hexdigest;
	#print "Old hash: $hash, New hash: $rec->{'pilothash'}\n";
	$rec->{'modified'} 	= (!defined($rec->{'pilothash'}) ||
				   ($rec->{'pilothash'} ne $hash));
	$rec->{'note'} 		= \@N if @N;
	$rec->{'script'} 		= \@S if @S;
	$rec->{'unhashedscript'} 	= \@US if @US;
	$rec->{'message'} 	= \@M if @M;
	$rec->{'date'} 		= DatePlanToPerl($date);
	$rec->{'time'} 		= TimePlanToPerl($time);
	$rec->{'length'} 		= TimePlanToPerl($length);
	$rec->{'early'} 		= TimePlanToPerl($early);
	$rec->{'late'} 		= TimePlanToPerl($late);
	$rec->{'color'}		= $color;

	$rec->{'suspended'} 	= substr($flags,0,1) ne "-";
	$rec->{'private'} 	= substr($flags,1,1) ne "-";
	$rec->{'noalarm'} 	= substr($flags,2,1) ne "-";
	$rec->{'hide_month'} 	= substr($flags,3,1) ne "-";
	$rec->{'hide_year'} 	= substr($flags,4,1) ne "-";
	$rec->{'hide_week'} 	= substr($flags,5,1) ne "-";
	$rec->{'hide_yearover'} 	= substr($flags,6,1) ne "-";
	$rec->{'d_flag'} 		= substr($flags,7,1) ne "-";
	$rec->{'locked'} 		= 1;
	$rec->{'id'} 		= $i;
	
	$rec->{'exceptions'} = [map(DatePlanToPerl($_), @E)] if @E;
	
	$planRecord{$i} = $rec;
	
	#print "Read plan record:\n";
	#print Dumper($rec);
}

############################################################
#
############################################################
sub HashPilotRecord {
	my ($record) = @_;
	my ($hash) = new Digest::MD5;
	$hash->add($record->{'raw'});
	$hash->hexdigest;
}


############################################################
#
############################################################
sub doafterplan {
	my ($db,$socket,$control) = @_;
	msg( "After stuff:\n" ) if ($PREFS->{'Debug'} > 2);

	##################################################################
	# This batch of code scans for Plan records with identical Pilot
	# IDs, presumambly caused by duplicating a plan record. We remove
	# the ids from the duplicates.  The weird sort is magic to prefer
	# keeping the id (and thus leaving unmodified) of an otherwise
	# unmodified record.
	##################################################################
	
	my (@uniq) = sort {$a->{'pilotid'} <=> $b->{'pilotid'} or $a->{'modified'} <=> $b->{'modified'}} grep {exists $_->{'pilotid'}} values %planRecord;
	my ($i) = 0;
	for($i=@uniq-1;$i>=1;$i--) {
		#print "Checking plan record: ", Dumper($uniq[$i]),"\n";
		if ($uniq[$i]->{'pilotid'} == $uniq[$i-1]->{'pilotid'}) {
			delete $uniq[$i]->{'pilotid'};
			$planID{$uniq[$i-1]->{'pilotid'}} = $uniq[$i-1];
			#print "... A dup, blessed be ye without id, and be ye modified.\n";
			$uniq[$i]->{'modified'} = 1;
		}
	}

	######################################################################
	# Use our saved Pilot ID cache to detect deleted Plan records.  This
	# will not catch deleted Plan records that were never assigned a
	# Pilot ID, but that is OK because such records do not have to be
	# removed from the Palm.
	######################################################################
	my ($loop_count) = (0);

	my ($del) = -1;
	foreach (keys %pilothash) {

		# Palm records originally downloaded from a different Plan database
		# are off-limits during this pass.
		
		next if $dbname{$_} ne $control->{'name'}; 
		

#		print "Palm cached ID: $_\n";
		if (not defined $planID{$_} and not $exceptID{$_}) {
			#print "Deleted plan record, with Pilot ID $_\n";
			$planID{$_}->{'deleted'} = 1;
			$planID{$_}->{'pilotid'} = $_;
			$planID{$_}->{'id'} = $del;
			$planRecord{$del} = $planID{$_};
			$del--;
		}
	}

	msg( "Palm loop\n" ) if ($PREFS->{'Debug'} > 2);

	foreach (keys %pilotID) {
		$dlp->tickle unless (++$loop_count % 50);

		# Palm records originally downloaded from a different Plan database
		# are off-limits during this pass.
		
		next if $dbname{$_} ne $control->{'name'}; 
		
		
		msg( "Palm record: " . PrintPilotRecord($pilotID{$_}) . "\n" ) if ($PREFS->{'Debug'} > 1);
		#print "Palm record: ",Dumper($pilotID{$_}),"\n";
		if ($pilotID{$_}->{'deleted'} || $pilotID{$_}->{'archived'}) {
		#	
		#	# At this point are seeing Palm records marked as deleted or
		#	# archived.  In the case of a slow sync, deleted records may not
		#	# be seen until a later pass.
		#	
		#	# Action: If there is an associated Plan record that has not
		#	# already been deleted, delete it.
		#	
		#	if (defined $planID{$_} and not $planID{$_}->{'deleted'}) {
		#		DeletePlanRecord($planID{$_});
		#		delete $planRecord{$planID{$_}->{'id'}};
		#		delete $planID{$_};
		#	}
		#
		#	# Remove the Pilot ID from the exception cache, if present
		#	delete $exceptID{$_};
		#	
		#	delete $lastID{$_};
		#
		#	delete $pilothash{$_};
		} else {
			my ($hash) = HashPilotRecord($pilotID{$_});
			
			######################################################
			# If the pilot record ID is not cached, then it is
			# definitely new.  If the MD5 hash of the record is
			# different from the cached hash, then it is
			# definitely different. These checks are only needed
			# during a slow sync (which will have inaccurate
			# flags), but are harmless during a fast sync.
			######################################################
			
			#print "Old hash: $pilothash{$_}, new hash: $hash\n";
			if ((not exists $pilothash{$_}) or ($hash ne $pilothash{$_})) {
				$pilotID{$_}->{'modified'} = 1;
				#print "Note: cache indicates record is changed\n";
			}
			$pilothash{$_} = $hash; # Record the hash and ID for the next sync
			
			# Remove the record from the exception cache if it has been
			# modified: perhaps it is not exceptional any more

			delete $exceptID{$_} if $pilotID{$_}->{'modified'};
			
			#print "Matching plan record: ", Dumper($planID{$_}),"\n";
			
			if (not defined $planID{$_}) {
				if (!$exceptID{$_}) {
					# The Palm record has no matching Plan record
					
					# Action: Install the Palm record in Plan, regardless of
					# changed status
					
					msg( "Installing Palm record in Plan: ".
						PrintPilotRecord($pilotID{$_}). "\n" ) if ($PREFS->{'Debug'});
					
					#print "Installing pilot record in plan: ",Dumper($pilotID{$_});
					
					my ($record) = RecordPilotToPlan($pilotID{$_});
					if (not defined $record) {
						# The record is not translatable to a Plan record. 
						
						# Action: Abort the install, and mark the record as
						# uninstallable so that it will not be tried each sync.
						# Code above will remove the exception flag when the
						# record is changed.
						
						$exceptID{$_} = 1;
	
						msg( "Palm record unsyncable\n" );
	
					} else {
					
						WritePlanRecord($socket, $record);
					}
				}
			} elsif ($pilotID{$_}->{'modified'} and $planID{$_}->{'deleted'}) {

				############################################
				# The Palm record has a matching _deleted_
				# Plan record.
				
				# This is collision, with a relatively
				# simple solution.  replace the Plan record
				# with the Palm record. As the Plan record
				# has already been permanently deleted, we
				# need only copy the Palm record over.
				
				# Action: Install the Palm record in Plan
				############################################

								
				my ($record) = RecordPilotToPlan($pilotID{$_}, $planID{$_});
				if (not defined $record) {
					# The record is not translatable to a Plan record. 
					
					# Action: Abort the install, and mark the record as
					# uninstallable so that it will not be tried each sync.
					
					$exceptID{$_} = 1;
					
					msg( "Palm record modified while Plan record deleted, but new Palm record unsyncable\n" );
				} else {

					WritePlanRecord($socket, $record);

					msg( "Palm record modified while Plan record deleted\n" ) if ($PREFS->{'Debug'} > 1);
				}
				
			} elsif ($pilotID{$_}->{'modified'} and $planID{$_}->{'modified'}) {


				############################################
				# The Palm record has a matching _modified_
				# Plan record.
				
				# TODO: Use a comparator function to verify
				# that the records are actually
				# substantially different. If not, simply
				# skip any action.
				
				# This is collision with an ugly, but
				# lossless, solution.  Neither the Palm or
				# Plan record is inherantly preferable, so
				# we duplicate each record on the other
				# side, severing the link between the
				# original new records, forging two new
				# links and two new records, one on each
				# side.
				
				# Action: Install the Palm record in Plan as
				# a new, distinct, record, and install the
				# Plan record on the Palm as a new,
				# distinct, record.
				############################################

				
				msg( "Conflicting modified Plan and Palm records\n" );
				
				{
					my ($record) = RecordPlanToPilot($planID{$_});
					if (not defined $record) {
						# The Plan record is not translatable to a Palm record. 
						
						# Action: Abort the install.
	
						msg( "Conflicting Plan record unsyncable.\n" );
					} else {
						$record->{'id'} = 0;
						my ($id) = WritePilotRecord($db, $control, $record);
						
						#$db->setRecord($record);
						#
						#my ($hash) = HashPilotRecord($record);						
						#$pilothash{$id} = $hash;
						#
						#$record->{'id'} = $id;
						#$pilotID{$id} = $record;
						#$dbname{$id} = $dbname;
						
						$planID{$_}->{'pilotid'} = $id;
						
						$planID{$_}->{'modified'} = 0;
			
						WritePlanRecord($socket, $planID{$_});
						
						msg( "ID of new Palm record is $id\n" ) if ($PREFS->{'Debug'} > 2);
					}
				}
				
				{
					my ($record) = RecordPilotToPlan($pilotID{$_});
					if (not defined $record) {
						# The Palm record is not translatable to a Plan record. 
						
						# Action: Abort the install.
	
						$exceptID{$_} = 1;
	
						msg( "Conflicting Palm record unsyncable.\n" );
					} else {
					
						$record->{'modified'} = 0;
						
						my ($id) = WritePlanRecord($socket, $record);

						msg( "ID of new Plan record is $id\n" ) if ($PREFS->{'Debug'} > 2);

					}
				}
			} elsif($pilotID{$_}->{'modified'}) {
			
				##########################################
				# At this point, we have a changed Palm
				# record with an existing unmodified Plan
				# record.
				
				# Action: Install the Palm record in Plan,
				# overwriting the Plan record.
				##########################################
								
				my ($record) = RecordPilotToPlan($pilotID{$_}, $planID{$_});
				if (not defined $record) {
					# The record is not translatable to a Plan record. 
					
					# Action: Abort the install, and mark the record as
					# uninstallable so that it will not be tried each sync.
					# Code above will remove the exception flag when the
					# record is changed.
					
					$exceptID{$_} = 1;
					DeletePlanRecord($socket, $planID{$_});
					
					msg( "Palm record modified while Plan record unchanged, but new Palm record unsyncable. Plan record has been deleted.\n" );
				} else {
				
					#print "Overwriting plan record: ",Dumper($planID{$_});
					#print "With pilot record: ",Dumper($pilotID{$_});
					#print "As plan record: ",Dumper($record);
				
					WritePlanRecord($socket, $record);
					msg( "Updating Plan record with modified Palm record: ".PrintPilotRecord($pilotID{$_})."\n" ) if ($PREFS->{'Debug'});
					#print "New plan record state: ",Dumper($planID{$_}),"\n";
				}
			}
		}
	}
	$dlp->tickle;
	msg( "Plan loop\n" ) if ($PREFS->{'Debug'} > 2);

	foreach (keys %planRecord) {
		$dlp->tickle unless (++$loop_count % 100);

		msg( "Plan record: " . PrintPlanRecord($planRecord{$_}),"\n" ) if ($PREFS->{'Debug'} > 1);
		my ($record) = $planRecord{$_};
		my ($pid) = $planRecord{$_}->{'pilotid'};
		
		#print "Plan record: ",Dumper($record),"\n";
		if ($record->{'deleted'}) {
		#	
		#	# At this point are seeing Palm records marked as deleted or
		#	# archived.  In the case of a slow sync, deleted records may not
		#	# be seen until a later pass.
		#	
		#	# Action: If there is an associated Plan record that has not
		#	# already been deleted, delete it.
		#	
		#	if (defined $planID{$_} and not $planID{$_}->{'deleted'}) {
		#		DeletePlanRecord($planID{$_});
		#		delete $planRecord{$planID{$_}->{'id'}};
		#		delete $planID{$_};
		#	}
		#
		#	# Remove the Pilot ID from the exception cache, if present
		#	delete $exceptID{$_};
		#	
		#	delete $lastID{$_};
		#
		#	delete $pilothash{$_};
		} else {

			# Remove the record from the exception cache if it has been
			# modified: perhaps it is not exceptional any more

			delete $record->{'pilotexcept'}  if $record->{'modified'};
			
			# If this is a fast sync, it's possible the record hasn't been
			# fetched yet.

			# This is dead code.  Fast sync was never
			# implemented, so $slowsync is always 1. I'm
			# leaving it here as a hint in case someone
			# ever gets around to implementing fast sync.
			# But it looks incorrect to me:
			# LoadPilotRecord takes an index, not an
			# id. -ANK

			if (!$slowsync and defined $pid and not exists $pilotID{$pid}) {
				my ($precord) = LoadPilotRecord($db, $pid);
				#$db->getRecord($pid);
				if (defined $precord) {
					if (not defined $dbname{$pid}) {
						$dbname{$pid} = $control->{'defaultname'};
					}
					$pilotID{$pid} = $precord;
				}
			}
			
			if (defined $pid and defined $pilotID{$pid} and ($dbname{$pid} ne $control->{'name'})) {
				msg( "Weird: Plan database $control->{'name'} claims to own Palm record $pid,\n" );
				msg( "but my ID database says it is owned by $dbname{$pid}. I'll skip it.\n" );
				next;
			}
			
			#print "Matching pilot record: ", Dumper($pilotID{$pid}),"\n";
			
			if (not defined $pid or not defined $pilotID{$pid}) {
				if (!$record->{'pilotexcept'}) {
					# The Plan record has no matching Palm record
					
					# Action: Install the Plan record in Palm, regardless of
					# changed status
					
					msg( "Installing Plan record in Palm: ".
						PrintPlanRecord($record). "\n" ) if ($PREFS->{'Debug'});

					#print "Installing plan record in pilot: ",Dumper($record);
					#print "Trying to install Plan record: ",Dumper($record),"\n";
					
					my ($newrecord) = RecordPlanToPilot($record);
					if (not defined $newrecord) {
						# The record is not translatable to a Palm record. 
						
						# Action: Abort the install, and mark the record as
						# uninstallable so that it will not be tried each sync.
						# Code above will remove the exception flag when the
						# record is changed.
						
						$record->{'pilotexcept'} = 1;
						$record->{'modified'} = 1;
						
						msg( "Plan record unsyncable\n" );
	
					} else {
						#print "Installing Palm record: ", Dumper($newrecord),"\n";
						
						$newrecord->{'id'} = 0;
						$newrecord->{'secret'} = 0;
						my ($id) = WritePilotRecord($db,$control,$newrecord);
						#$db->setRecord($newrecord);

						msg( "ID of new Palm record is $id\n" ) if ($PREFS->{'Debug'} > 2);
						
						#my ($hash) = HashPilotRecord($newrecord);						
						#$pilothash{$id} = $hash;
						#
						#$newrecord->{'id'} = $id;
						#$pilotID{$id} = $newrecord;
						#$dbname{$id} = $dbname;
						
						$record->{'pilotid'} = $id; # Match the Palm record to the Plan record
						$record->{'modified'} = 1;  # and make sure it is written back out
					}
				}
			} elsif ($record->{'modified'} and $pilotID{$pid}->{'deleted'}) {

				# The Plan record has a matching _deleted_ Palm record.
				
				# This is collision, with a relatively simple solution.
				# replace the Palm record with the Plan record. 
				
				# Action: Install the Plan record in Palm
								
				my ($newrecord) = RecordPlanToPilot($record, $pilotID{$pid});
				if (not defined $newrecord) {
					# The record is not translatable to a Palm record. 
					
					# Action: Abort the install, and mark the record as
					# uninstallable so that it will not be tried each sync.
					
					$record->{'pilotexcept'} = 1;
					
					msg( "Plan record modified while Palm record deleted, but new Plan record unsyncable\n" );
				} else {

					#print "Installing Palm record: ", Dumper($newrecord),"\n";
					WritePilotRecord($db,$control,$newrecord);
					#$db->setRecord($newrecord);
					#my ($hash) = HashPilotRecord($newrecord);						
					#$pilothash{$pid} = $hash;

					msg( "Plan record modified while Palm record deleted\n" ) if ($PREFS->{'Debug'} > 1);
				}
				
			} elsif ($record->{'modified'} and $pilotID{$pid}->{'modified'}) {
				croak("This shouldn't happen...");
			} elsif ($record->{'modified'}) {
			
				# At this point, we have a changed Plan record with an
				# existing unmodified Palm record.
				
				# Action: Install the Plan record in the Palm, overwriting the
				# Palm record.
				
				#print "Trying to install Plan record: ",Dumper($record),"\n";
				my ($newrecord) = RecordPlanToPilot($record, $pilotID{$pid});
				if (not defined $newrecord) {
					# The record is not translatable to a Plan record. 
					
					# Action: Abort the install, and mark the record as
					# uninstallable so that it will not be tried each sync.
					# Code above will remove the exception flag when the
					# record is changed.
					
					$record->{'pilotexcept'} = 1;
					
					DeletePilotRecord($db,$pid);
					#$db->deleteRecord($record->{'pilotid'});
					#delete $pilothash{$record->{'pilotid'}};
					#delete $exceptID{$record->{'pilotid'}};
					
					msg( "Plan record modified while Palm record unchanged, but new Plan record unsyncable. Palm record has been deleted.\n" );
				} else {

					#print "Overwriting pilot record: ",Dumper($pilotID{$_});
					#print "With plan record: ",Dumper($record);
					#print "As pilot record: ",Dumper($newrecord);

					#print "Installing Palm record: ", Dumper($newrecord),"\n";
					WritePilotRecord($db,$control,$newrecord);
					#$db->setRecord($newrecord);
					#my ($hash) = HashPilotRecord($newrecord);						
					#$pilothash{$pid} = $hash;
					
					msg( "Updating Palm record with modified Plan record: ".PrintPlanRecord($record)."\n" ) if ($PREFS->{'Debug'});
				}
			}
		}
		if ($record->{'modified'}) {
			WritePlanRecord($socket, $record);
		}
	}

	msg( "Palm delete loop\n" ) if ($PREFS->{'Debug'} > 2);

	foreach (keys %pilotID) {
		$dlp->tickle unless (++$loop_count % 100);

		############################################################
		# Palm records originally downloaded from a different Plan
		# database are off-limits during this pass.
		############################################################		
		next if $dbname{$_} ne $control->{'name'}; 

		#print "Palm record: ",Dumper($pilotID{$_}),"\n";
		msg( "Palm record: " . PrintPilotRecord($pilotID{$_}) . "\n" ) if ($PREFS->{'Debug'} > 1);
		if ($pilotID{$_}->{'deleted'} || $pilotID{$_}->{'archived'}) {
			
			# At this point are seeing Palm records marked as deleted or
			# archived.  In the case of a slow sync, deleted records may not
			# be seen until a later pass.
			
			# Action: If there is an associated Plan record that has not
			# already been deleted, delete it.
			
			msg( "Deleting Palm record.\n" ) if ($PREFS->{'Debug'} > 1);
			
			if (defined $planID{$_} and not $planID{$_}->{'deleted'}) {
				msg( "... and associated Plan record.\n" ) if ($PREFS->{'Debug'} > 1);
				msg( "Deleting from Plan: ". PrintPlanRecord($planRecord{$planID{$_}->{'id'}}) ."\n") if ($PREFS->{'Debug'});
				DeletePlanRecord($socket, $planID{$_});
				delete $planRecord{$planID{$_}->{'id'}};
				delete $planID{$_};
			}
		
			# Remove the Pilot ID from the exception cache, if present
			delete $exceptID{$_};
			
			delete $pilotID{$_};
			
			delete $dbname{$_};
		
			delete $pilothash{$_};
		}
	}
	
	msg( "Plan delete loop\n" ) if ($PREFS->{'Debug'} > 2);

	foreach (keys %planRecord) {
		$dlp->tickle unless (++$loop_count % 100);
	
		my ($record) = $planRecord{$_};
		my ($pid) = $planRecord{$_}->{'pilotid'};
		#print "Plan record: ",Dumper($record),"\n";
		msg( "Plan record: " . PrintPlanRecord($planRecord{$_}) . "\n" ) if ($PREFS->{'Debug'} > 1);
	
		# In a fast sync, we might not have loaded the record yet.
		
		# This is dead code.  Fast sync was never implemented,
		# so $slowsync is always 1. I'm leaving it here as a
		# hint in case someone ever gets around to
		# implementing fast sync.  But it looks incorrect to
		# me: LoadPilotRecord takes an index, not an id. -ANK

		if (!$slowsync and defined $pid and not exists $pilotID{$pid}) {
			my ($precord) = LoadPilotRecord($db, $pid);
			#$db->getRecord($pid);
			if (defined $precord) {
				if (not defined $dbname{$pid}) {
					$dbname{$pid} = $control->{'defaultname'};
				}
				$pilotID{$pid} = $precord;
			}
		}
		
		if (defined $pid and defined $pilotID{$pid} and ($dbname{$pid} ne $control->{'name'})) {
			msg( "Weird: Plan database $control->{'name'} claims to own Palm record $pid,\n" );
			msg( "but my ID database says it is owned by $dbname{$pid}. I'll skip it.\n" );
			next;
		}
		
		if ($record->{'deleted'}) {
			
			# At this point are seeing Palm records marked as deleted or
			# archived.  In the case of a slow sync, deleted records may not
			# be seen until a later pass.
			
			# Action: If there is an associated Plan record that has not
			# already been deleted, delete it.
			
			msg( "Deleting Plan record.\n" ) if ($PREFS->{'Debug'} > 1);
			if (defined $pid and defined $pilotID{$pid} and not $pilotID{$_}->{'deleted'}) {
				msg( "... and associated Palm record.\n" ) if ($PREFS->{'Debug'} > 1);
				msg( "Deleting from Palm: " . PrintPilotRecord($pilotID{$pid}) ."\n" ) if ($PREFS->{'Debug'});
				DeletePilotRecord($db, $pid);
				#$db->deleteRecord($pid);
				#delete $pilotID{$pid};
				#delete $pilothash{$pid};
				#delete $exceptID{$pid};
			}
		
			# Remove the Pilot ID from the exception cache, if present
			
			delete $planRecord{$_};
		}
	}
	

}

############################################################
#
############################################################
sub loadpilotrecords {
	msg( "Loading pilot records:\n" );

	if ($dlp->getStatus<0) {
		croak "Cancelled.\n";
	}
	
	msg( "Synchronizing pilot called '$pilotname'\n" ) if ($PREFS->{'Debug'} > 1);
	
	if (not defined $control{$pilotname}) {
		msg( "Database access list for Palm has not been defined!\n\n" );
		msg( "Palm '$pilotname' has been added to $controldir/control.\n" );
		msg( "Please edit $controldir/control and add the names of the Plan databases\n" );
		msg( "that this Palm should synchronize with.\n" );
		
		open (C, ">>$controldir/control");
		print C "$pilotname\n";
		close (C);
		return 0;
	}
	
	$db = $dlp->open("DatebookDB");

	my ($r, $i);
	$i=0;
	my $max = $db->getRecords();
	$max ||= 1;
	status("Reading Palm Appointments", 0);
	while(defined($r = LoadPilotRecord($db,$i++))) {
		status("Reading Palm Appointments", int(100*$i/$max))
		    if ($i % (int($max/20)+1) == 0);
	}
	status("Reading Palm Appointments", 100);
	msg( "Done reading records\n" ) if ($PREFS->{'Debug'} > 1);

	$slowsync = 1;

	if ($slowsync) {
		foreach (keys %pilothash) {
			if (not exists $pilotID{$_}) {
				$pilotID{$_}->{'deleted'} = 1;
			}
		}
	}
	return 1;
}

############################################################
#
############################################################
sub SendPlanCommand {
	my ($socket,$text) = @_;
	my ($len);
	#print "Sending |$text|\n";
	while (length($text)) {
		$len = syswrite $socket, $text, length($text);
		$text = substr($text,$len);
	}
}

my ($partialReply) = "";

############################################################
#
############################################################
sub ReadPlanReply {
	my ($socket) = @_;
	my ($reply) = "";
	my ($buf);

	while (1) {
		while ($partialReply =~ /\A(.*?)(\\)?\n/m) {
			$reply .= $1."\n";
			$partialReply = $';
			if (not defined($2)) {
				$reply =~ s/\\\n/\n/sg;
				$reply =~ s/\n$//sg;
				
				if ($reply =~ /\AR/) {	# Discard 
					next;
				} elsif ($reply =~ /\A\?/) {	# Discard
					msg( "Plan message: $'" );
					next;
				} else {
					#print "Reply: |$reply|\n";
					return $reply;
				}
				$reply = "";
			}
		}
		do {
		    sysread($socket,$buf,1024);
		    $partialReply .= $buf;
		} while ($buf !~ /[^\\]\n|\A\n/);
		# ^^ the regexp matches if $buf contains an unescaped
		# newline, i.e. a newline that's either the first
		# character, or preceded by a non-escape character.
	}
}
	

############################################################
#
############################################################
sub SyncDB {
	my ($db, $control) = @_;

	my $dbname = $control->{'dbname'};
	
	#print "Opening database $control->{'name'}\@$control->{'host'}:$control->{'port'}.\n";

	my $socket = IO::Socket::INET->new(PeerPort => $control->{'port'}, PeerAddr => $control->{'host'}, Proto => 'tcp');

	if (not defined $socket) {
		croak "Unable to open plan socket on $control->{'host'}:$control->{'port'}\n";
	}

	$socket->autoflush(1);

	my $select = IO::Select->new();
   
	$select->add($socket);

	my $reply=ReadPlanReply($socket);

	if ($reply !~ /^!/) {	
		croak "Unknown response from netplan: $reply\n";
	}

	$netplanversion = $reply;

	# Authenticate
	SendPlanCommand($socket, "=sync-plan<uid=$<,gid=$>,pid=$$>\n");

	SendPlanCommand($socket, "o$dbname\n");
	$reply = ReadPlanReply($socket);
	
	if ($reply !~ /^otw(\d+)/) {
		croak "Failed to open database $control->{'name'}\@$control->{'host'}:$control->{'port'}.\n";
	}
	$file = $1;
	
	SendPlanCommand($socket, "n$file\n");
	$reply = ReadPlanReply($socket);
	
	if ($reply !~ /^n\d+\s+(\d+)/) {
		croak "Failed to get record count.\n";
	}
	my $records = $1;


	my @id= ();
		
	SendPlanCommand($socket, "r$file 0\n");
	while ($records) {
		$reply = ReadPlanReply($socket);
		if ($reply =~ /\Art\d+\s+(\d+)\s+/) {
			push @id, $1;
			#print "Got ID $1\n";
			$records--;
		}
	}

	my ($loop_count) = (0);
	foreach (@id) {
		$dlp->tickle unless (++$loop_count % 50);
		SendPlanCommand($socket, "l$file $_\n");
		$reply = ReadPlanReply($socket);
		
		if ($reply !~ /^lt/) {
			croak "Failed to lock record $_.\n";
		}
	
		SendPlanCommand($socket, "r$file $_\n");
		$reply = ReadPlanReply($socket);
		
		if ($reply !~ /\Art\d+\s+(\d+)\s+/s) {
			croak "Didn't get record I was looking for.\n";
		}
		
		dorecord($db, $socket, $control, $_, $');
	}
	
	doafterplan($db, $socket, $control);

	%planRecord = ();  # Flush plan records

	SendPlanCommand($socket, "c$file\n");

	$socket->close;
}

############################################################
#
############################################################
sub readControlfile
{
    if (! -d $controldir) {
	croak "Directory $controldir does not exist. It must be created before $0 is run.\n\n";
    }

    if (! -f "$controldir/control") {
	open(C, ">$controldir/control") || croak "Unable to write to $controldir/control";
	print C "# this file is used to control which Palms are allowed to sync, and what databases\n";
	print C "# each Palm will sync with. Each line consists of whitespace-separated fields, the\n";
	print C "# first one being the name (and ID) of the Palm, and subsequent fields listing\n";
	print C "# all plan databases that Palm will synchronize with.\n";
	print C "#\n";
	print C "# For example: Foo_s_Pilot_1234 myname\@localhost group\@host.io ro:all\@localhostn";
	print C "#\n";
	print C "# New entries on the Palm are installed in the first database listed.\n";
	print C "# Records will not exchanged between separate plan datatabses.\n";
	print C "# A database may be prefixed with 'rw:' or 'ro:' to indicate read/write (the\n";
	print C "# default) or read only access. If a database is read-only, any record changes\n";
	print C "# on the Palm will be discarded. However, for technical reasons, you must have\n";
	print C "# read/write access to the plan database itself.\n";
	close(C);
    }

    open(C,"<$controldir/control");
    while (<C>) {
	chomp;
	next if /^#/;
	my ($i,@i) = split(/\s+/, $_);
	my (@I);
	my ($first) = 1;
	my ($defaultname);
	foreach (@i) {
	    my ($mode, $name, $host) = m/^(?:(wr|ro|rw):)?([^\@]+)(?:\@(.+))?$/;
	    if (not defined $mode) {
		$mode = "rw";
	    }
	    if (not defined $host) {
		$host = "localhost";
	    }
	    if ($mode !~ /^rw$/) {
		croak "Access mode $mode (for Palm '$i') at line $. of $controldir/control unknown or unsupported.\n";
	    }
	    if ($first) {
		$defaultname = $name.'@'.$host;
	    }
	    push @I, {mode => $mode, name => $name.'@'.$host, dbname => $name, host => $host, port => $PREFS->{'NetplanPort'}, 'read' => ($mode =~ /r/), 'write' => ($mode =~ /w/), default => $first, defaultname => $defaultname};
	    $first = 0;
	}
	$control{$i} = [@I];
    }
    close(C);
}

############################################################
#
############################################################
sub conduitSync
{
    $dlp = $_[1];
    $info = $_[2];

    # initialize variables that may still be set from last sync (which
    # can happen when conduitSync is called from PilotManager).
    %control = ();
    %pilothash = ();
    %pilotID = ();
    %planID = ();
    %exceptID = ();
    %planRecord = ();
    %dbname = ();
    %sawName = ();
    $pilotname = $db = $slowsync = $file = $maxseed = $netplanversion = undef;

    readControlfile;

    $pilotname = $info->{'name'} . "_ " . $info->{'userID'};
    $pilotname =~ s/[^A-Za-z0-9]+/_/g;

    foreach (@{$control{$pilotname}}) {
	$sawName{$_->{'name'}} = 1;
    }

    if (open (I, "<$controldir/ids.$pilotname")) {
	foreach (<I>) {
	    chop;
	    my ($id, $hash, $except, $dbname) = split(/\s+/, $_);
	    $pilothash{$id} = $hash;
	    $exceptID{$id} = $except;
	    if (not defined $dbname or not length $dbname) {
		$dbname = $control{$pilotname}->[0]->{'name'};
	    }
	    $dbname{$id} = $dbname if defined $dbname and length $dbname;
	    #print Dumper({dbname=>$dbname{$id}});
	    if (not defined $sawName{$dbname}) {
		msg( "Warning! The ID file, $controldir/ids.$pilotname, lists a record as belonging\n" );
		msg( "to database $dbname, but the control file $controldir/control does not list this\n" );
		msg( "this database. If you have renamed a database, please edit $controldir/ids.$pilotname\n" );
		msg( "so all references to this database match the new name.\n" );
		msg( "\nIf you wish to delete all on the Palm that were originally from $dbname, then\n" );
		msg( "delete the database name from the end of each record's line.\n" );
		msg( "To merge the records into the default database, delete each affected line entirely.\n" );
			
		$sawName{$dbname} = 1;
	    }
	}

	close (I);
    }
	

    if (loadpilotrecords) {

	if (!@{$control{$pilotname}}) {
	    msg( "No plan databases are registered for the '$pilotname' Palm. Please\n" );
	    msg( "edit $controldir/control and add one or more databases.\n" );
	}

	foreach (keys %pilotID) {
	    if (not defined $dbname{$_}) {
		$dbname{$_} = $control{$pilotname}->[0]->{'name'};
	    }
	}
	
	foreach (@{$control{$pilotname}}) {
	    next if not defined $_->{'host'}; # Sigh. Autoviv problem.
	    SyncDB($db, $_);
	}

	# Delete deleted & archived records
	$db->purge;
	
	# Clear modified flags, and set last sync time to now
	$db->resetFlags;

	$db->close;

	open (I, ">$controldir/ids.$pilotname");
	foreach (keys %pilothash) {
	    if ($dbname{$_} eq $control{$pilotname}->[0]{'name'}) {
		$dbname{$_}="";
	    }
	    $exceptID{$_} = 0 unless (defined $exceptID{$_});
	    print I "$_ $pilothash{$_} $exceptID{$_} $dbname{$_}\n";
	}
	close(I);


    }
}

############################################################
# main
############################################################

my ($tempdlp, $tempinfo);

if (@ARGV<2) {
    croak "Usage: $0 <pilot port> <control directory>\n\n<control directory> is where various information is stored.\nYou might wish to use " .
      (getpwuid($>))[7] . "/.sync-plan\n";
}

$port = $ARGV[0];
$controldir = $ARGV[1];

$controldir =~ s/\/+$//;

msg "Please start HotSync now.\n";
my $psocket = PDA::Pilot::openPort($port);

if (!$psocket) {
    croak "Unable to open port $port\n";
}
($tempdlp = PDA::Pilot::accept($psocket)) || croak "Can't connect to Palm";

($tempinfo = $tempdlp->getUserInfo) || croak "Lost connection to Palm";

conduitSync(undef, $tempdlp, $tempinfo);

$dlp->close();
PDA::Pilot::close($psocket);
