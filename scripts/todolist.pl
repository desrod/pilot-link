#!/usr/bin/perl -w
# todolist - display Palm ToDo list from most recent backup.  1/16/2000 by CRJ
#
# Workable options to review various date ranges:
#	Past Week:	todolist -d-7,-1 -c
#	Yesterday:	todolist -d-1,-1 -c
#	Today:		todolist -d0 -p
#	Tomorrow:	todolist -d1,1
#	Coming Week:	todolist -d6 -p -r
# ...of course just plain "todolist" is always good...
#
# Version 1.7.  Written by Carl Jacobsen (carl@ucsd.edu)
#
# Note: this script's notion of "today" is based upon the modification time
#	on the specified ToDo database, and this is what is used in deciding
#	what records are for today, tomorrow, past_due, etc.  This is on
#	purpose, to treat the database as a snapshot in time, as it were;
#	if you want to rely on it for making decisions of the moment, you
#	ought to be working off of a fairly recent backup...
#
# TODO:
#	Biggest limitations at this point are:
#	* Notes are not shown (an "N" will appear at the end of the line
#	  to indicate that a note is attached).
#	* No options to control display of specific priorities or categories.
#	* Might want something to search for words in description.

use strict;
use PDA::Pilot;

(my $prog_name = $0) =~ s#.*/##;

my $todoname = PDA::Pilot::ToDoDatabase::dbname();
my $todofile = '~/Pilot/RAM/' . $todoname . '.pdb';


my $Usage = <<EOT;
Usage: $prog_name [ -c -d[S,][E] -p -s -r -w -u -P -n -f ToDoDB.pdb_filename ]
Function:
     Displays a ToDo list summary, from a PalmOS ToDo database file
Options:
     -c		Show completed items (normally only non-completed are shown),
     -d[S,][E]	Hide undated items (normally both dated and undated are shown),
		S and E, if given, are start and end of date range to display,
		in the form of integer date offsets,
     -p		Hide past-due items (normally all dates are shown),
     -s		Show secret ("Private") records (normally suppressed),
     -r		Show relative dates (Mon, Tue, etc.) where possible,
     -w		Wrap long descriptions to multiple lines (normally truncated),
     -u		Don't Uppercase "Unfiled" (normally done to make more obvious),
     -P		Sort by priority-then-due-date (normally the other way around),
     -n		Don't filter output through PAGER (default pages if terminal),
     -f file	Specifies ToDo database file (default $todofile).

     Default behavior may be modified with environment variables
     TODOLIST_OPTS and TODOLIST_FILE (and PBACK_BASE_DIR) -- see
     source code for details.
EOT

# - - - - - - - - - - - - - - - - - - - - Miscellaneous setup

my @month_abbvs = qw( Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec );
my @day_abbvs   = qw( Sun Mon Tue Wed Thu Fri Sat );

(undef, my $NoDate) = appdates(2038, 1, 15);	# near end of Unix time_t

# - - - - - - - - - - - - - - - - - - - - Parse command line

# Default values for options...

my $show = {
     completed   => 0,
     undated     => 1,
     past_due    => 1,
     secret      => 0,
     relative    => 0,
     wrap        => 0,
     up_unfiled  => 1,
     page_output => 1,
     priority1st => 0,
     start_date  => undef,
     end_date    => undef,
};

# Options may be modified with "setenv TODOLIST_OPTS undated=0:relative=1", etc.

if (defined $ENV{TODOLIST_OPTS}) {
     foreach my $part (split ':', $ENV{TODOLIST_OPTS}) {

	  die "$prog_name: ill-formed entry \"$part\" in TODOLIST_OPTS\n"
	       unless $part =~ /^(\w+)=(\d+)$/;
	  my($opt, $value) = ($1, $2);

	  die "$prog_name: unknown option \"$opt\" in TODOLIST_OPTS\n"
	       unless exists $show->{$opt};
	  $show->{$opt} = $value;
     }
}

# File may be specified with "setenv TODOLIST_FILE /foo/bar/ToDoDB.pdb", etc.
# Or, if "pback" is being used here, borrow its environment variable...

if (defined $ENV{TODOLIST_FILE}) {
     $todofile = $ENV{TODOLIST_FILE};

} elsif (defined $ENV{PBACK_BASE_DIR}) {
     $todofile = $ENV{PBACK_BASE_DIR} . '/RAM/' . $todoname . '.pdb';
}

# - - Process options...

while (@ARGV) {
     $_ = shift;
     if    (/^-c$/) { $show->{completed}   = 1; }
     elsif (/^-p$/) { $show->{past_due}    = 0; }
     elsif (/^-s$/) { $show->{secret}      = 1; }
     elsif (/^-r$/) { $show->{relative}    = 1; }
     elsif (/^-w$/) { $show->{wrap}        = 1; }
     elsif (/^-u$/) { $show->{up_unfiled}  = 0; }
     elsif (/^-P$/) { $show->{priority1st} = 1; }
     elsif (/^-n$/) { $show->{page_output} = 0; }

     elsif (/^-d((-?\d+),)?(-?\d+)?$/) {
	  $show->{undated} = 0;
	  $show->{start_date} = $2  if defined $2 and $2 ne '';
	  $show->{end_date}   = $3  if defined $3 and $3 ne '';

     } elsif (/^--(\w+)=(\d+)$/ and defined $show->{$1}) {	# "long opts"
	  $show->{$1} = $2;

     } elsif (/^-f$/) {
	  die "$prog_name: -f requires a filename argument\n"  unless @ARGV;
	  $todofile = shift;

     } elsif (/^--$/) {			# international sign for end of opts...
	  last;
     } elsif (/^-./ and /^-\\?\??$/) {	# if usage requested (-?, -\?, etc.) ...
	  print $Usage;
	  exit 0;
     } elsif (/^--help$/i) {		# if usage requested (GNU-style) ...
	  print $Usage;
	  exit 0;
     } elsif (/^-/) {			# unknown option...
	  die "$prog_name: don't understand \"$_\".",
	      "  Try \"$prog_name -\\?\" for help.\n";
     } else {
	  unshift @ARGV, $_;		# reached first non-option argument...
	  last;
     }
}
die "$prog_name: too many arguments.  Try \"$prog_name -\\?\" for help.\n"
     if @ARGV;

# Translate past_due flag into a start_date...

if (not $show->{past_due}) {
     $show->{start_date} = 0  unless defined $show->{start_date}
					 and $show->{start_date} > 0;
}

# - - - - - - - - - - - - - - - - - - - - Do the real work...

# Validate existence of the ToDo database...

if ($todofile =~ m#^~/#) {
     die "$prog_name: no HOME environment variable?\n"
	  unless defined $ENV{HOME};

     $todofile =~ s#^~/#$ENV{HOME}/#;
}

die "$prog_name: $todofile not found\n"           unless -e $todofile;
die "$prog_name: $todofile isn't a plain file\n"  unless -f $todofile;
die "$prog_name: can't read $todofile\n"          unless -r $todofile;

# Get $mtime from modify time of ToDo database, and tweak it to ensure
# that adding and subtracting days from it will work despite DST...

my $mtime = (stat _)[9];	# must immediately follow file tests above

(undef, undef, my $HH, my $dd, my $mm, my $yy) = localtime $mtime;
$mtime += 5 * 60 * 60  if $HH < 5;	# hack to avoid DST 2-3am crossovers

# Build %weekdays and $Today from mtime of ToDo database...

my %weekdays;
my $Today;

foreach my $day (0..6) {
     (undef, undef, $HH, $dd, $mm, $yy, my $dow)
	  = localtime $mtime + ($day * 24 * 60 * 60);
     my($show_date, $sort_date) = appdates($yy, $mm, $dd);

     my $day_name = $day_abbvs[$dow];
     if ($day == 0) {
	  $day_name = uc $day_name;
	  $Today = $sort_date;
     }
     $weekdays{$show_date} = $day_name;
}

# Translate start and end dates (if any) from day offsets to yyyy/mm/dd

foreach my $key ('start_date', 'end_date') {
     if (defined $show->{$key}) {
	  (undef, undef, $HH, $dd, $mm, $yy, my $dow)
	       = localtime $mtime + ($show->{$key} * 24 * 60 * 60);
	  (undef, $show->{$key}) = appdates($yy, $mm, $dd);
     }
}

# Open the ToDo database...

my $pif = PDA::Pilot::File::open($todofile);

die "$prog_name: can't open $todofile: $!\n"  unless defined $pif;

# Build categories hash (1=>'Personal', 2=>'Business', etc.) ...

my $ai = $pif->getAppBlock();
PDA::Pilot::ToDo::UnpackAppBlock($ai);

my %categories;
foreach my $n (0 .. $#{$ai->{categoryName}}) {
     my $id   = $ai->{categoryID}[$n];
     my $name = $ai->{categoryName}[$n];

     $name = uc $name  if $name eq 'Unfiled'  and $show->{up_unfiled};

     $categories{$id} = $name  if $name ne '';
}

# Read through the ToDo database, building @todos list...

my @todos;
for (my $idx = 0; my $r = $pif->getRecord($idx); $idx++) {
     PDA::Pilot::ToDo::Unpack($r);

     next  if $r->{archived} or $r->{deleted};

     my($date, $sort_date) = ( '--', $NoDate );
     ($date, $sort_date) = appdates($r->{due}[5], $r->{due}[4], $r->{due}[3])
	  if defined $r->{due};

     my $todo = {
	  category => $r->{category},
	  priority => $r->{priority},
	  complete => $r->{complete},
	  secret   => $r->{secret},
	  due      => $date,
	  due_sort => $sort_date,
     };
     $todo->{description} = $r->{description}  if defined $r->{description};
     $todo->{note}        = $r->{note}         if defined $r->{note};

     push @todos, $todo;
}
$pif->close();

# Sort the collected ToDo records...

if ($show->{priority1st}) {
     @todos = sort priority_then_due_date @todos;
} else {
     @todos = sort due_date_then_priority @todos;
}

# Pipe output through $pager if not otherwise directed...

my $paging = 0;
if (-t STDOUT and $show->{page_output}) {
     my $pager = $ENV{PAGER} || 'more';
     open STDOUT, "| $pager"
	  or die "$prog_name: can't pipe output through $pager: $!\n";
     $paging = 1;
}

# Print the collected ToDo records...

print_todos($show, \%categories, \%weekdays, \@todos);

close STDOUT  if $paging;

# ------------------------------------------------------------------------

sub due_date_then_priority
{
        $a->{due_sort}    cmp $b->{due_sort}
     or $a->{priority}    <=> $b->{priority}
     or $a->{category}    <=> $b->{category}
     or $a->{description} cmp $b->{description};
}

# ------------------------------------------------------------------------

sub priority_then_due_date
{
        $a->{priority}    <=> $b->{priority}
     or $a->{due_sort}    cmp $b->{due_sort}
     or $a->{category}    <=> $b->{category}
     or $a->{description} cmp $b->{description};
}

# ------------------------------------------------------------------------

# Given year, month(0..11), and date, returns show_date (in form "ddMmmyy"),
# and sort_date (in form "yyyy/mm/dd").

sub appdates
{
     my($yy, $mm, $dd) = @_;

     my $show_date = sprintf "%02d%-3s%02d", $dd, $month_abbvs[$mm], $yy % 100;
     my $sort_date = sprintf "%04d/%02d/%02d", $yy + 1900, $mm + 1, $dd;

     ( $show_date, $sort_date );
}

# ------------------------------------------------------------------------

sub print_todos
{
     my($show, $categories, $weekdays, $todos) = @_;
     my($descIndent, $descWidth) = (" " x 10, 47);
     my $print_div = $show->{priority1st} ? 'priority' : 'due';
     my $last_div  = undef;

     foreach my $todo (@$todos) {
	  # Skip entry if it doesn't meet all our criteria...

	  next  if $todo->{complete}           and not $show->{completed};
	  next  if $todo->{due} eq '--'        and not $show->{undated};
	  next  if $todo->{secret}             and not $show->{secret};

	  next  if defined $show->{start_date}
		       and $show->{start_date} gt $todo->{due_sort};

	  next  if defined $show->{end_date}
		       and $show->{end_date} lt $todo->{due_sort};

	  # Build various print-worthy strings...

	  my $priority =  '.' x ($todo->{priority} - 1) . $todo->{priority}
		        . '.' x (5 - $todo->{priority});

	  my $date = $todo->{due};
	  $date = $weekdays->{$date}
	       if $show->{relative} and defined $weekdays->{$date};

	  my $desctxt = defined($todo->{description})
			      ? $todo->{description} : '?';
	  my @desc = ( $desctxt );

	  if ($show->{wrap}) {		# split $desctxt into lines in @desc
	       @desc = ();
	       my $buf = undef;
	       while ($desctxt =~ s/^(\s*)(\S+)//) {
		    if (defined $buf and length($buf . $1 . $2) <= $descWidth) {
			 $buf .= $1 . $2;
		    } else {
			 push @desc, $buf  if defined $buf and $buf ne '';
			 $buf = $2;
		    }
	       }
	       push @desc, $buf  if defined $buf and $buf ne '';
	  }

	  # Print blank lines between priorities or dates (whichever is 1st)...

	  if (not defined($last_div) or $last_div ne $todo->{$print_div}) {
	       print "\n"  if defined $last_div;
	       $last_div = $todo->{$print_div};
	  }

	  printf "[%s] %5s %-${descWidth}.${descWidth}s %s%-7s  %-8s %s%s\n",
		 ($todo->{complete} ? 'X' : '_'), $priority, $desc[0],
		 (($todo->{due_sort} lt $Today) ? '!' : ' '), $date,
		 ($categories->{$todo->{category}} || "?$todo->{category}?"),
		 ((defined $todo->{note} and $todo->{note} ne '') ? 'N' : ' '),
		 ($todo->{secret} ? 'S' : ' ');

	  if ($show->{wrap}) {	# if wrapping descript, print remaining parts
	       shift @desc;
	       print map { $descIndent . $_ . "\n" } @desc;
	  }
     }
}

# ------------------------------------------------------------------------
