use Config;
print $Config{startperl}, "\n";
print <DATA>;
__DATA__;

use IO::Socket;
use IO::Select;
use Time::Local;
use MD5;
use PDA::Pilot;

sub DatePlanToPerl {
	my($PlanDate) = @_;
	my($m,$d,$y) = split(m!/!,$PlanDate);
	if ($y<40) {
		$y+=100;
	}
	if ($y>1900) {
		$y-=1900;
	}
	$m--;

	timelocal(0,0,0,$d,$m,$y);
}

sub TimePlanToPerl {
	my($PlanTime) = @_;
	my($h,$m,$s) = split(m!:!,$PlanTime);
	
	return undef if $h == 99 and $m == 99 and $s == 99;
	
	$s + ($m*60) + ($h*60*60);
}

sub TimePerlToPlan {
	my($PerlDT) = @_;
	return "99:99:99" if not defined $PerlDT;

	my($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) =
	    localtime($PerlDT);
	
	"$hour:$min:$sec";
}

sub TimeRelPerlToPlan {
	my($PerlDT) = @_;
	return "99:99:99" if not defined $PerlDT;

	my($sec,$min,$hour);
	
	$hour = int($PerlDT/ (60*60));
	$PerlDT -= $hour*60*60;

	$min = int($PerlDT/ (60));
	$PerlDT -= $min*60;

	$sec = int($PerlDT);
	$PerlDT -= $sec;
	
	"$hour:$min:$sec";
}

sub DatePerlToPlan {
	my($PerlDT) = @_;
	my($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) =
	    localtime($PerlDT);
	
	$year += 1900;
	$mon++;
	
	"$mon/$mday/$year";
}

sub RecordPlanToPilot {
	my($plan,$pilot) = @_;
	if (not defined $pilot) {
		$pilot = PDA::Pilot::AppointmentDatabase->record;
	}
	
	$pilot->{id} = $plan->{pilotid};
	$pilot->{description} = join("\xA", @{$plan->{note}}) if defined $plan->{note};
	$pilot->{note} = join("\xA", @{$plan->{message}}) if defined $plan->{message};
	$pilot->{desription} ||= "";

	if (defined $plan->{time}) {
		$pilot->{begin} = [localtime($plan->{date}+$plan->{time})];
		$pilot->{end} = [localtime($plan->{date}+$plan->{time}+$plan->{length})];
		$pilot->{event}=0;
	} else {
		$pilot->{begin} = [localtime($plan->{date})];
		$pilot->{event}=1;
	}
	
	if ($plan->{early} and $plan->{late}) {
		return undef;
	}
	if ($plan->{early} or $plan->{late}) {
		my($alarm) = $plan->{early}+$plan->{late};
		if ($alarm > (60*60*24)) {
			$pilot->{alarm}->{units} = "days";
			$pilot->{alarm}->{advance} = int($alarm / (60*60*24));
		} elsif ($alarm > (60*60)) {
			$pilot->{alarm}->{units} = "hours";
			$pilot->{alarm}->{advance} = int($alarm / (60*60));
		} else {
			$pilot->{alarm}->{units} = "minutes";
			$pilot->{alarm}->{advance} = int($alarm / 60);
		}
	}
	
	if (defined $plan->{exceptions}) {
		foreach (@{$plan->{exceptions}}) {
			push @{$pilot->{exceptions}}, [localtime($_)];
		}
	} else {
		delete $pilot->{exceptions};
	}

	if (defined $plan->{repeat}) {
		if ($plan->{repeat}->[1]) {
			$pilot->{repeat}->{end} = [localtime($plan->{repeat}->[1])];
		}
		my($days,$end,$weekday,$mday,$yearly) = @{$plan->{repeat}};
		$pilot->{repeat}->{weekstart} = 0;
		if ($days and not $weekday and not $mday and not $yearly) {
			$pilot->{repeat}->{type} = "Daily";
			$pilot->{repeat}->{frequency} = $days / (60*60*24);
		} elsif(not $days and not $weekday and not $mday and $yearly) {
			$pilot->{repeat}->{type} = "Yearly";
			$pilot->{repeat}->{frequency} = 1;
		} elsif(not $days and not $weekday and ($mday == 1 << $pilot->{begin}[3]) and not $yearly) {
			$pilot->{repeat}->{type} = "MonthlyByDate";
			$pilot->{repeat}->{frequency} = 1;
		} else {
			return undef;
		}
	} else {
		delete $pilot->{repeat};
	}
	
	$pilot;
}

sub RecordPilotToPlan {
	my($pilot,$plan) = @_;
	$plan = {color => 0} if not defined $plan;
	
	$plan->{pilotid} = $pilot->{id};
	$plan->{id} ||= 0;
	$plan->{message} = [split("\xA", $pilot->{note})] if defined $pilot->{note};
	$plan->{note} = [split("\xA", $pilot->{description})] if defined $pilot->{description};
	
	my($date) = timelocal(@{$pilot->{begin}});
	$plan->{date} = $date;
	if ($pilot->{event}) {
		$plan->{time} = undef;
		$plan->{length} = 0;
	} else {
		$plan->{time} = $date;
		$plan->{length} = timelocal(@{$pilot->{end}}) - $date;
	}
	
	if (exists $pilot->{alarm}) {
		my($alarm)=0;
		$plan->{noalarm} = 0;
		if ($pilot->{alarm}{units} eq "days") {
			$alarm = $pilot->{alarm}->{advance} * (60*60*24);
		} elsif ($pilot->{alarm}{units} eq "hours") {
			$alarm = $pilot->{alarm}->{advance} * (60*60);
		} elsif ($pilot->{alarm}{units} eq "minutes") {
			$alarm = $pilot->{alarm}->{advance} * (60);
		}
		if ($plan->{early}) {
			$plan->{early} = $alarm;
			$plan->{late} = 0;
		} else {
			$plan->{early} = 0;
			$plan->{late} = $alarm;
		}
	} else {
		$plan->{noalarm} = 1;
	}
	
	if (exists $pilot->{exceptions}) {
		# Plan records can only deal with four exceptions, 
		if (@{$pilot->{exceptions}} > 4) {
			return undef;
		}
		foreach (@{$pilot->{exceptions}}) {
			push @{$plan->{exceptions}}, timelocal(@{$_});
		}
	}

	delete $plan->{repeat};
	
	if (exists $pilot->{repeat}) {
		$plan->{repeat} = [0,0,0,0,0];
		if ($pilot->{repeat}->{type} eq "Daily") {
			$plan->{repeat}->[0] = (60*60*24) * $pilot->{repeat}->{frequency};
			$plan->{repeat}->[4] = 0;
		} elsif ($pilot->{repeat}->{type} eq "Yearly" and ($pilot->{repeat}->{frequency}==1)) {
			$plan->{repeat}->[4] = 1;
		} elsif ($pilot->{repeat}->{type} eq "Weekly" and ($pilot->{repeat}->{frequency}==1)) {
			my($r);
			foreach $i (0..6) {
				if ($plan->{repeat}->{repeatDays}[$i]) {
					$r |= 1<<$i;
				}
			}
			$plan->{repeat}->[2] = $r | 256|512|1024|2048|4096|8192;
		} elsif ($pilot->{repeat}->{type} eq "MonthlyByDate" and ($pilot->{repeat}->{frequency}==1)) {
			$plan->{repeat}->[3] = 1 << $pilot->{begin}[3];
		} elsif ($pilot->{repeat}->{type} eq "MonthlyByDay" and ($pilot->{repeat}->{frequency}==1)) {
			my($day) = $pilot->{repeat}{repeatDay} % 7;
			my($week) = $pilot->{repeat}{repeatDay} / 7;
			$week=5 if $week == 4;
			$plan->{repeat}->[2] = (1 << $day) | (1 << (8+$week));
		} else {
			return undef;
		}
		if (defined $pilot->{repeat}->{end}) {
			$plan->{repeat}->[1] = timelocal(@{$pilot->{repeat}->{end}});
		}
	}
	
	$plan;
}

sub generaterecord {
	my($rec) = @_;
	my(@output);
	
#	print "Plan record: ", Dumper($rec),"\n";

	push(@output, DatePerlToPlan($rec->{date})." ".
				TimePerlToPlan($rec->{time})." ".
				TimeRelPerlToPlan($rec->{length})." ".
				TimeRelPerlToPlan($rec->{early})." ".
				TimeRelPerlToPlan($rec->{late})." ".
				($rec->{suspended} ? "S" : "-").
				($rec->{private} ? "P" : "-").
				($rec->{noalarm} ? "N" : "-").
				($rec->{hide_month} ? "M" : "-").
				($rec->{hide_year} ? "Y" : "-").
				($rec->{hide_week} ? "W" : "-").
				($rec->{hide_yearover} ? "O" : "-").
				($rec->{d_flag} ? "D" : "-").
				"-".
				"-".
				" ".$rec->{color});

	if (defined $rec->{repeat}) {
		push @output, "R\t".join(" ",@{$rec->{repeat}});
	}
	if (defined $rec->{exceptions}) {
		foreach (@{$rec->{exceptions}}) {
			push @output, "E\t".DatePerlToPlan($_);
		}
	}
	if (defined $rec->{note}) {
		push @output, map("N\t$_", @{$rec->{note}});
	}
	if (defined $rec->{message}) {
		push @output, map("M\t$_", @{$rec->{message}});
	}
	if (defined $rec->{pilotid}) {
		push @output, "S\t#PilotID: $rec->{pilotid}";
	}
	if ($rec->{pilotexcept}) {
		push @output, "S\t#PilotExcept";
	}
	if (defined $rec->{script}) {
		push @output, map("S\t$_", @{$rec->{script}});
	}
	if (defined $rec->{other}) {
		foreach (@{$rec->{other}}) {
			push @output, $_;
		}
	}

	my($hash) = new MD5;
	foreach (@output) {
		$hash->add($_);
	}
	$rec->{pilothash} = $hash->hexdigest;
	{
		my($i);
		for($i=0;$i<@output;$i++) {
			last if $output[$i] =~ /^S/;
		}
		splice @output, $i, 0, "S\t#PilotHash: $rec->{pilothash}";
	}

	join("\n",@output);
}

sub PrintPlanRecord {
	my($rec) = @_;
	my($output);
	
	$output = DatePerlToPlan($rec->{date});
	if ($rec->{time}) {
		$output .= " ".TimePerlToPlan($rec->{time})."-".
				TimePerlToPlan($rec->{time}+$rec->{length});
	}
	$output .= " '".join("\\n",@{$rec->{note}})."'";
	$output .= " (".join("\\n",@{$rec->{message}}).")" if defined $rec->{message};
	
	$output .= " {ID:$rec->{pilotid}, Except:$rec->{pilotexcept}, Changed:$rec->{modified}, Deleted:$rec->{deleted}}";
	
	$output;
}

sub PrintPilotRecord {
	my($rec) = @_;
	my($output);
	
	$output = ($rec->{begin}[5]+1900)."/".($rec->{begin}[4]+1)."/".$rec->{begin}[3];
	
	if (!$rec->{event}) {
		$output .= " ";
		$output .= ($rec->{begin}[2]).":".($rec->{begin}[1]).":".$rec->{begin}[0];
		$output .= "-";
		$output .= ($rec->{end}[2]).":".($rec->{end}[1]).":".$rec->{end}[0];
	}
	
	$output .= " '$rec->{note}'";
	$output .= " ($rec->{message})" if not defined $rec->{message};
	
	$output .= " {ID:$rec->{id}, Except:$exceptID{$rec->{id}}, Changed:$rec->{modified}, Deleted:$rec->{deleted}}";

	$output =~ s/\r/\\r/g;
	$output =~ s/\n/\\n/g;
	
	$output;
}

# takes a Plan record in hash format
sub WritePlanRecord {
	my($record) = @_; 
	my($raw) = generaterecord($record);
	my($reply);
	$record->{id} ||= 0;
	#print "ID is $record->{id}\n";
	$raw =~ s/\n/\\\n/g;
	$raw = "w$file $record->{id} $raw\n";
	$record->{raw} = $raw;
#	print "Installing record $record->{id} (PilotID: $record->{pilotid}) in Plan: $raw";
	syswrite $socket, $raw, length($raw);
	sysread $socket, $reply, 1024;
#	print "Reply to installation: |$reply|\n";
	if ($reply =~ /^w[tf](\d+)/) {
		$record->{id} = $1;
		$planRecord{$1} = $record;
#		print "New record id: $1\n";
	}
}

# takes a Plan record in hash format
sub DeletePlanRecord {
	my($record) = @_; 
	my($raw);
	$raw = "d$file $record->{id}\n";
#	print "Deleting record $record->{id} (PilotID: $record->{pilotid}) in Plan\n";
	syswrite $socket, $raw, length($raw);
}

sub syncplanrecord {
	my($plan) = @_;
#	print "Read plan record: ", Dumper($plan),"\n";

}


sub dorecord {
	my($i,$r) = @_;
#	print "Record: $r\n";
	my(@l) = split(/\n/,$r);
	my($rec) = { raw => [@l], other => [] };
	my(@E,@R,@N,@M,@S);
	my($hash) = new MD5;
	$l[0] =~ s/\s+/ /g;
	$hash->add($l[0]);
	my($date, $time, $length, $early, $late, $flags, $color) = split(/\s+/, shift @l);
	$rec->{pilotrec} = "";
	foreach (@l) {
		if (/^E\t/) {
			push @E, $';
		} elsif (/^M\t/) {
			push @M, $';
		} elsif (/^N\t/) {
			push @N, $';
		} elsif (/^S\t/) {
			my ($s) = $';
			if ($s =~ /^\s*#PilotID: (\S+)/) {
				$rec->{pilotid} = $1;
				$planID{$1} = $rec;
			} elsif ($s =~ /^\s*#PilotHash: (\S+)/) {
				$rec->{pilothash} = $1;
				next; # Skip hash add
			} elsif ($s =~ /^\s*#PilotExcept/) {
				$rec->{pilotexcept} = 1;
			} else {
				push @S, $s;
			}
		} elsif (/^R\t/) {
			my($r) = $';
			$r =~ s/\s+/ /g;
			$rec->{repeat} = [split(/\s+/, $r)];
		} else {
			push @{$rec->{other}}, $_;
		}
		$hash->add($_);
	}
	$hash = $hash->hexdigest;
	#print "Old hash: $hash, New hash: $rec->{pilothash}\n";
	$rec->{modified} = ($rec->{pilothash} ne $hash);
	$rec->{note} = \@N if @N;
	$rec->{script} = \@S if @S;
	$rec->{message} = \@M if @M;
	$rec->{date} = DatePlanToPerl($date);
	$rec->{time} = TimePlanToPerl($time);
	$rec->{length} = TimePlanToPerl($length);
	$rec->{early} = TimePlanToPerl($early);
	$rec->{late} = TimePlanToPerl($late);
	$rec->{color} = $color;

	$rec->{suspended} = substr($flags,0,1) ne "-";
	$rec->{private} = substr($flags,1,1) ne "-";
	$rec->{noalarm} = substr($flags,2,1) ne "-";
	$rec->{hide_month} = substr($flags,3,1) ne "-";
	$rec->{hide_year} = substr($flags,4,1) ne "-";
	$rec->{hide_week} = substr($flags,5,1) ne "-";
	$rec->{hide_yearover} = substr($flags,6,1) ne "-";
	$rec->{d_flag} = substr($flags,7,1) ne "-";
	$rec->{locked} = 1;
	$rec->{id} = $i;
	
	$rec->{exceptions} = [map(DatePlanToPerl($_), @E)] if @E;
	
	$planRecord{$i} = $rec;
	
#	print Dumper($rec);
	#$_ = generaterecord($rec);
	#s/\n/\\\n/g;
	#$_ = "w$file $i $_\n";
	#print "|$_|\n";
#	print Dumper(RecordPlanToPilot($rec));
	
	#syswrite $socket, $_, length($_);
	#sysread $socket, $_, 1024;
	#print "Resp: $_\n";
	
#	print "do sync\n";
	syncplanrecord($rec);
	
}

sub HashPilotRecord {
	my($record) = @_;
	my($hash) = new MD5;
	$hash->add($record->{raw});
	$hash->hexdigest;
}


sub doafterplan {
	print "After stuff:\n";
#	foreach (keys %ID) {
#		if (not defined $planID{$_}) {
#			#record deleted on plan
#			delete $ID{$_};
#		}
#		if (not defined $pilotID{$_}) {
#			#record deleted on pilot
#			delete $ID{$_};
#		}
#	}

	# Use our saved Pilot ID cache to detect deleted Plan records.
	# This will not catch deleted Plan records that were never assigned
	# a Pilot ID, but that is OK because such records do not have to be removed
	# from the Pilot.
	
	my($del)=-1;
	foreach (keys %pilothash) {
#		print "Pilot cached ID: $_\n";
		if (not defined $planID{$_} and not $exceptID{$_}) {
			#print "Deleted plan record, with Pilot id $_\n";
			$planID{$_}->{deleted} = 1;
			$planID{$_}->{pilotid} = $_;
			$planID{$_}->{id} = $del;
			$planRecord{$del} = $planID{$_};
			$del--;
		}
	}

	print "Pilot loop\n";	

	foreach (keys %pilotID) {
		print "Pilot record: ",PrintPilotRecord($pilotID{$_}),"\n";
		#print "Pilot record: ",Dumper($pilotID{$_}),"\n";
		if ($pilotID{$_}->{deleted} || $pilotID{$_}->{archived}) {
		#	
		#	# At this point are seeing Pilot records marked as deleted or
		#	# archived.  In the case of a slow sync, deleted records may not
		#	# be seen until a later pass.
		#	
		#	# Action: If there is an associated Plan record that has not
		#	# already been deleted, delete it.
		#	
		#	if (defined $planID{$_} and not $planID{$_}->{deleted}) {
		#		DeletePlanRecord($planID{$_});
		#		delete $planRecord{$planID{$_}->{id}};
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
			my($hash) = HashPilotRecord($pilotID{$_});
			
			# If the pilot record ID is not cached, then it is definitely
			# new.  If the MD5 hash of the record is different from the
			# cached hash, then it is definitely different. These checks are
			# only needed during a slow sync (which will have inaccurate
			# flags), but are harmless during a fast sync.
			
			#print "Old hash: $pilothash{$_}, new hash: $hash\n";
			if ((not exists $pilothash{$_}) or ($hash ne $pilothash{$_})) {
				$pilotID{$_}->{modified} = 1;
				#print "Note: cache indicates record is changed\n";
			}
			$pilothash{$_} = $hash; # Record the hash and ID for the next sync
			
			# Remove the record from the exception cache if it has been
			# modified: perhaps it is not exceptional any more

			delete $exceptID{$_} if $pilotID{$_}->{modified};
			
			#print "Matching plan record: ", Dumper($planID{$_}),"\n";
			
			if (not defined $planID{$_}) {
				if (!$exceptID{$_}) {
					# The Pilot record has no matching Plan record
					
					# Action: Install the Pilot record in Plan, regardless of
					# changed status
					
					print "Action: Install Pilot record in Plan.\n";
					
					my($record) = RecordPilotToPlan($pilotID{$_});
					if (not defined $record) {
						# The record is not translatable to a Plan record. 
						
						# Action: Abort the install, and mark the record as
						# uninstallable so that it will not be tried each sync.
						# Code above will remove the exception flag when the
						# record is changed.
						
						$exceptID{$_} = 1;
	
						print "Log: Pilot record unsyncable\n";
	
					} else {
					
						WritePlanRecord($record);
					}
				}
			} elsif ($pilotID{$_}->{modified} and $planID{$_}->{deleted}) {

				# The Pilot record has a matching _deleted_ Plan record.
				
				# This is collision, with a relatively simple solution.
				# replace the Plan record with the Pilot record. As the Plan
				# record has already been permanently deleted, we need only copy
				# the Pilot record over.
				
				# Action: Install the Pilot record in Plan
								
				my($record) = RecordPilotToPlan($pilotID{$_}, $planID{$_});
				if (not defined $record) {
					# The record is not translatable to a Plan record. 
					
					# Action: Abort the install, and mark the record as
					# uninstallable so that it will not be tried each sync.
					
					$exceptID{$_} = 1;
					
					print "Log: Pilot record modified while Plan record deleted, but new Pilot record unsyncable\n";
				} else {

					WritePlanRecord($record);

					print "Log: Pilot record modified while Plan record deleted\n";
				}
				
			} elsif ($pilotID{$_}->{modified} and $planID{$_}->{modified}) {

				# The Pilot record has a matching _modified_ Plan record.
				
				# TODO: Use a comparator function to verify that the records
				# are actually substantially different. If not, simply skip
				# any action.
				
				# This is collision with an ugly, but lossless, solution. 
				# Neither the Pilot or Plan record is inherantly preferable,
				# so we duplicate each record on the other side, severing
				# the link between the original new records, forging two new
				# links and two new records, one on each side.
				
				# Action: Install the Pilot record in Plan as a new,
				# distinct, record, and install the Plan record on the Pilot
				# as a new, distinct, record.
				
				print "Log: Conflicting modified Plan and Pilot records\n";
				
				{
					my($record) = RecordPlanToPilot($planID{$_});
					if (not defined $record) {
						# The Plan record is not translatable to a Pilot record. 
						
						# Action: Abort the install.
	
						print "Log: Conflicting Plan record unsyncable.\n";
					} else {
						$record->{id} = 0;
						my($id) = $db->setRecord($record);
						
						my ($hash) = HashPilotRecord($record);						
						$pilothash{$id} = $hash;
						
						$record->{id} = $id;
						$pilotID{$id} = $record;
						
						$planID{$_}->{pilotid} = $id;
						
						$planID{$_}->{modified} = 0;
			
						WritePlanRecord($planID{$_});
					}
				}
				
				{
					my($record) = RecordPilotToPlan($pilotID{$_});
					if (not defined $record) {
						# The Pilot record is not translatable to a Plan record. 
						
						# Action: Abort the install.
	
						$exceptID{$_} = 1;
	
						print "Log: Conflicting Pilot record unsyncable.\n";
					} else {
					
						$record->{modified} = 0;
						
						my($id) = WritePlanRecord($record);
					}
				}
			} elsif($pilotID{$_}->{modified}) {
			
				# At this point, we have a changed Pilot record with an
				# existing unmodified Plan record.
				
				# Action: Install the Pilot record in Plan, overwriting the
				# Plan record.
								
				my($record) = RecordPilotToPlan($pilotID{$_}, $planID{$_});
				if (not defined $record) {
					# The record is not translatable to a Plan record. 
					
					# Action: Abort the install, and mark the record as
					# uninstallable so that it will not be tried each sync.
					# Code above will remove the exception flag when the
					# record is changed.
					
					$exceptID{$_} = 1;
					DeletePlanRecord($planID{$_});
					
					print "Log: Pilot record modified while Plan record unchanged, but new Pilot record unsyncable. Plan record has been deleted.\n";
				} else {
				
					WritePlanRecord($record);
					print "Log: Overwriting unchanged Plan record with modified Pilot record.\n";
					#print "New plan record state: ",Dumper($planID{$_}),"\n";
				}
			}
		}
	}
	
	print "Plan loop\n";

	foreach (keys %planRecord) {
		print "Plan record: ",PrintPlanRecord($planRecord{$_}),"\n";
		my($record) = $planRecord{$_};
		my($pid) = $planRecord{$_}->{pilotid};
		#print "Plan record: ",Dumper($record),"\n";
		if ($record->{deleted}) {
		#	
		#	# At this point are seeing Pilot records marked as deleted or
		#	# archived.  In the case of a slow sync, deleted records may not
		#	# be seen until a later pass.
		#	
		#	# Action: If there is an associated Plan record that has not
		#	# already been deleted, delete it.
		#	
		#	if (defined $planID{$_} and not $planID{$_}->{deleted}) {
		#		DeletePlanRecord($planID{$_});
		#		delete $planRecord{$planID{$_}->{id}};
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

			delete $record->{pilotexcept}  if $record->{modified};
			
			#print "Matching pilot record: ", Dumper($pilotID{$pid}),"\n";
			
			if (not defined $pid or not defined $pilotID{$pid}) {
				if (!$record->{pilotexcept}) {
					# The Plan record has no matching Pilot record
					
					# Action: Install the Plan record in Pilot, regardless of
					# changed status
					
					print "Action: Install Plan record in Pilot.\n";
					
					my($newrecord) = RecordPlanToPilot($record);
					if (not defined $newrecord) {
						# The record is not translatable to a Pilot record. 
						
						# Action: Abort the install, and mark the record as
						# uninstallable so that it will not be tried each sync.
						# Code above will remove the exception flag when the
						# record is changed.
						
						$record->{pilotexcept} = 1;
						$record->{modified} = 1;
						
						print "Log: Plan record unsyncable\n";
	
					} else {
						#print "Installing Pilot record: ", Dumper($newrecord),"\n";
						$newrecord->{id} = 0;
						$newrecord->{secret} = 0;
						my($id) = $db->setRecord($newrecord);

						my ($hash) = HashPilotRecord($newrecord);						
						$pilothash{$id} = $hash;
						
						$newrecord->{id} = $id;
						$pilotID{$id} = $newrecord;
						
						$record->{pilotid} = $id; # Match the Pilot record to the Plan record
						$record->{modified} = 1;  # and make sure it is written back out
					}
				}
			} elsif ($record->{modified} and $pilotID{$pid}->{deleted}) {

				# The Plan record has a matching _deleted_ Pilot record.
				
				# This is collision, with a relatively simple solution.
				# replace the Pilot record with the Plan record. 
				
				# Action: Install the Plan record in Pilot
								
				my($newrecord) = RecordPlanToPilot($record, $pilotID{$pid});
				if (not defined $newrecord) {
					# The record is not translatable to a Pilot record. 
					
					# Action: Abort the install, and mark the record as
					# uninstallable so that it will not be tried each sync.
					
					$record->{pilotexcept} = 1;
					
					print "Log: Plan record modified while Pilot record deleted, but new Plan record unsyncable\n";
				} else {

					#print "Installing Pilot record: ", Dumper($newrecord),"\n";
					$db->setRecord($newrecord);
					my ($hash) = HashPilotRecord($newrecord);						
					$pilothash{$pid} = $hash;

					print "Log: Plan record modified while Pilot record deleted\n";
				}
				
			} elsif ($record->{modified} and $pilotID{$pid}->{modified}) {
				die("This shouldn't happen...");
			} elsif ($record->{modified}) {
			
				# At this point, we have a changed Plan record with an
				# existing unmodified Pilot record.
				
				# Action: Install the Plan record in the Pilot, overwriting the
				# Pilot record.
								
				my($newrecord) = RecordPlanToPilot($record, $pilotID{$pid});
				if (not defined $newrecord) {
					# The record is not translatable to a Plan record. 
					
					# Action: Abort the install, and mark the record as
					# uninstallable so that it will not be tried each sync.
					# Code above will remove the exception flag when the
					# record is changed.
					
					$record->{pilotexcept} = 1;
					$db->deleteRecord($record->{pilotid});
					
					print "Log: Plan record modified while Pilot record unchanged, but new Plan record unsyncable. Pilot record has been deleted.\n";
				} else {
					#print "Installing Pilot record: ", Dumper($newrecord),"\n";
					$db->setRecord($newrecord);
					my ($hash) = HashPilotRecord($newrecord);						
					$pilothash{$pid} = $hash;
					
					print "Log: Overwriting unchanged Pilot record with modified Plan record.\n";
				}
			}
		}
		if ($record->{modified}) {
			WritePlanRecord($record);
		}
	}

	print "Pilot delete loop\n";	

	foreach (keys %pilotID) {
		#print "Pilot record: ",Dumper($pilotID{$_}),"\n";
		print "Pilot record: ",PrintPilotRecord($pilotID{$_}),"\n";
		if ($pilotID{$_}->{deleted} || $pilotID{$_}->{archived}) {
			
			# At this point are seeing Pilot records marked as deleted or
			# archived.  In the case of a slow sync, deleted records may not
			# be seen until a later pass.
			
			# Action: If there is an associated Plan record that has not
			# already been deleted, delete it.
			
			print "Log: Deleting Pilot record.\n";
			
			if (defined $planID{$_} and not $planID{$_}->{deleted}) {
				print "Log: ... and associated Plan record.\n";
				DeletePlanRecord($planID{$_});
				delete $planRecord{$planID{$_}->{id}};
				delete $planID{$_};
			}
		
			# Remove the Pilot ID from the exception cache, if present
			delete $exceptID{$_};
			
			delete $lastID{$_};
		
			delete $pilothash{$_};
		}
	}
	
	print "Plan delete loop\n";

	foreach (keys %planRecord) {
		my($record) = $planRecord{$_};
		my($pid) = $planRecord{$_}->{pilotid};
		#print "Plan record: ",Dumper($record),"\n";
		print "Plan record: ",PrintPlanRecord($planRecord{$_}),"\n";
		if ($record->{deleted}) {
			
			# At this point are seeing Pilot records marked as deleted or
			# archived.  In the case of a slow sync, deleted records may not
			# be seen until a later pass.
			
			# Action: If there is an associated Plan record that has not
			# already been deleted, delete it.
			
			print "Log: Deleting Plan record.\n";
			if (defined $pid and defined $pilotID{$pid} and not $pilotID{$_}->{deleted}) {
				print "Log: ... and associated Pilot record.\n";
				$db->deleteRecord($pid);
				delete $pilotID{$pid};
				delete $pilothash{$pid};
				delete $exceptID{$pid};
			}
		
			# Remove the Pilot ID from the exception cache, if present
			
			DeletePlanRecord($record);
		}
	}

	# Delete deleted & archived records
	$db->purge;
	
	# Clear modified flags, and set last sync time to now
	$db->resetFlags;
}

sub loadpilotrecords {
	print "Loading pilot records:\n";

	print "Please start HotSync now.\n";
	$psocket = PDA::Pilot::openPort($port);
	if (!$psocket) {
		die "Unable to open Pilot port $port\n";
	}
	$dlp = PDA::Pilot::accept($psocket);
	$db = $dlp->open("DatebookDB");
		
	$i=0;
	while(defined($r = $db->getRecord($i++))) {
		push @pilotRecord, $r;
#		print "Pilot Record: ",Dumper($r),"\n";
		$pilotID{$r->{id}} = $r;
	}
	print "Done reading records\n";
	
	$slowsync = 1;

	if ($slowsync) {
		foreach (keys %pilothash) {
			if (not exists $pilotID{$_}) {
				$pilotID{$_}->{deleted} = 1;
			}
		}
	}
}

sub readcommand {
	my($socket) = @_;
	local($in,$_);
	
	#print "Reading from socket\n";
	while (sysread($socket,$in,1024,length($in))==1024 or /\\\Z/) {
	}
	
	while ($in =~ /^(.+?)(\\)?$/m) {
		$_ .= $1."\n";
		$in = $';
		if (not defined($2)) {
			s/\\\n/\n/sg;
			s/\n$//sg;
#			print "Read |$_|\n";
	
			if (/^[rR]t(\d+)\s+(\d+)\s+/s) {
				if ($records and not exists $newPlanRecord{$2}) {
					$records--;
				}
				$newPlanRecord{$2} = $';
#				print "Read record $2 from file $1: $'\n";
				if ($state == 5 and $records<=0) {
					$state = 6;
				}
				if ($state == 9) {
					dorecord($2,$');
					delete $newPlanRecord{$2};
					$state = 10;
				}
			}
			elsif ($state == 7 and /^l[tf]/s) {
				$state = 8;
#				print "Locked record $key\n";
			}
			elsif ($state == 1 and /^o[tf][rw](\d+)/s) {
				$file = $1;
#				print "Opened $file\n";
				$state = 2;
			}
			elsif ($state == 3 and /^n\d+\s+(\d+)/s) {
				$records = $1;
#				print "Records: $records\n";
				$state = 4;
			}
			elsif ($state == 0 and /^![tf]/s) {
#				print "Status: $'\n";
				$state = 1;
			}
			$_ = "";
		}
	}
}



if (@ARGV<3) {
	die "Usage: $0 <plan host> <plan database name> <pilot port>\n";
}


$name = $ARGV[1];

$port = $ARGV[2];

open (I, "<pilotids.$name");
foreach (<I>) {
	chop;
	my($id, $hash, $except) = split(/\s+/, $_);
	$pilothash{$id} = $hash;
	$exceptID{$id} = $except;
}
close (I);

loadpilotrecords;

$socket = IO::Socket::INET->new(PeerPort => '5444', PeerAddr => $ARGV[0], Proto => 'tcp');

if (not defined $socket) {
	die "Unable to open plan socket on $ARGV[0]:5444\n";
}

$socket->autoflush(1);

$select = IO::Select->new();
    
$select->add($socket);

$state = 0;



for (;;) {
	if ($state == 1) {
		syswrite $socket, "o$name\n", length("o$name\n");
	}
	elsif ($state == 2) {
		syswrite $socket, "n$file\n", length("n$file\n");
		$state = 3;
	}
	elsif ($state == 4) {
		if ($records == 0) {
			$state = 11;
		} else {
			syswrite $socket, "r$file 0\n", length("r$file 0\n");
			$state = 5;
		}
	}
	elsif ($state == 6) {
		@k = sort {$a<=>$b} keys %newPlanRecord;
		if (!@k) {
			$state = 11;
		} else {
			$key = $k[0];
#			print "Deleting $key\n";
			delete $newPlanRecord{$key};
			syswrite $socket, "l$file $key\n", length("l$file $key\n");
			$state=7;
		}
	}
	elsif ($state == 8) {
		syswrite $socket, "r$file $key\n", length("r$file $key\n");
		$state = 9;
	}
	elsif ($state == 10) {
		# Don't unlock yet, leave to later.
		#syswrite $socket, "u$file $key\n", length("u$file $key\n");
		$state = 6;
	}
	elsif ($state == 11) {
		doafterplan;
		$state = -1;
	}
	elsif ($state == -1) {
		if (defined $file) {
			# Automatically unlocks all records
			syswrite $socket, "c$file\n", length("c$file\n");
		}
		last;
	}
	if ($select->can_read(0)) {
		readcommand($socket);
	}
}

$db->close;
$dlp->close;

open (I, ">pilotids.$name");
foreach (keys %pilothash) {
	print I "$_ $pilothash{$_} $exceptID{$_}\n";
}
close(I);

$socket->close;

