#!/usr/bin/perl -w
# pback - wrapper for pilot-xfer to streamline Palm syncs.  12/14/98 by CRJ
#
# This script is meant to make using pilot-xfer on a daily basis as
# painless as possible.  You set the PILOTPORT and PILOTRATE environment
# variables (or hack the copies in the setup section, below), and create
# a "Pilot" directory under your home directory, and then you can type:
#
#	pback sync
#
# to create (or update) a backup of the RAM and Flash ROM (if any) in
# your Palm.  At the same time that it does the backup, it will
# install any files that have previously fed to pback with, e.g.,
#
#	pback install Frotz.prc zork1.pdb
#
# This allows one to HotSync in a way more closely resembling Palm's own
# Install and HotSync programs.
#
# Version 1.8.  Written by Carl Jacobsen (carl@ucsd.edu)

use strict;
use Cwd;
use File::Compare;

(my $prog_name = $0) =~ s#.*/##;

my $Usage = <<EOT;
Usage: $prog_name [ -q ] COMMAND
   or: $prog_name [ -q ] install file...
Where COMMAND is one of:
     sync
	  Synchronizes backup of both RAM and non-OS Flash DBs (by
	  default, configurable with \$PBACK_DEFAULT_SYNC),

     syncRAM / syncFlash / syncOS
	  Synchronizes backup of RAM only, or Non-OS Flash DBs only,
	  or OS DBs (from Flash or ROM) only,

     newdirRAM / newdirFlash / newdirMemo
	  Saves a snapshot of existing RAM / Flash / Memos backup dir,

     syncmemos
	  Updates Memos directory tree with all memos from latest backup,

     lsinstall
	  Display contents of the directory holding files to be installed.
EOT

# - - - - - - - - - - - - - - - - - - - - Miscellaneous setup

# These few things you may want to mess with...

$ENV{PILOTPORT} = '/dev/tty00'	unless defined $ENV{PILOTPORT};
$ENV{PILOTRATE} = '57600'	unless defined $ENV{PILOTRATE};

my $default_sync = 'ram,flash';
	# Set to comma separated list of words "ram", "flash", "os",
	# representing things to sync by default when "sync" issued with
	# no nodifiers.  Can be changed with $PBACK_DEFAULT_SYNC env var.

if (defined $ENV{PBACK_DEFAULT_SYNC}) {
     $default_sync = $ENV{PBACK_DEFAULT_SYNC};

     die "$prog_name: \$PBACK_DEFAULT_SYNC exists but contains no words\n"
	  if $default_sync eq '';
     foreach my $word (split ',', $default_sync) {
	  die "$prog_name: don't grok \"$word\" in \$PBACK_DEFAULT_SYNC\n"
	       unless $word =~ /^(ram|flash|os)$/i;
     }
}

my $base = $ENV{PBACK_BASE_DIR} || (
     defined $ENV{HOME} ? "$ENV{HOME}/Pilot"
			: die "$prog_name: \$HOME not set\n" );


# Below here, best to leave alone...

my($dirRAM, $dirFlash, $dirOS, $dirInst, $dirInsted, $dirDel, $dirMemo ) = qw(
       RAM      Flash      OS   Install   Installed   Deleted     Memos
);
my @dirs = ($dirRAM, $dirFlash, $dirOS, $dirInst, $dirInsted, $dirDel,$dirMemo);

die "$prog_name: create the directory $base first, please\n"
     unless -d $base;

foreach my $dir (map { "$base/$_" } @dirs) {
     mkdir700($dir)  unless -d $dir;
}


# - - - - - - - - - - - - - - - - - - - - Parse command line, do requested work

my $pilot_xfer = 'pilot-xfer';
my $quiet      = 0;

while (@ARGV) {
     if    ($ARGV[0] eq '-x') { $pilot_xfer = 'pilot-xfer.debug'; }
     elsif ($ARGV[0] eq '-q') { $quiet      = 1;                  }
     else                     { last;                             }
     shift;
}

# Handle install command first, since it's the only one with variable args...

my $cmd = @ARGV ? shift : '--nocmd--';	# 1st arg or something that will fail

if ($cmd eq 'install') {
     die qq($prog_name: "install" option makes no sense without filenames\n)
	  unless @ARGV;
     my $got = 0;
     foreach my $file (@ARGV) {
	  if (-f $file) {
	       my @cpcmd = ('cp', '-pi', $file, "$base/$dirInst");
	       $cpcmd[1] =~ s/i//  if $quiet;
	       cmd(@cpcmd);
	       $got = 1;
	  } else {
	       warn "$prog_name: can't find $file\n";
	  }
     }
     print qq[File(s) copied.  Do "$prog_name sync" to transfer.\n]
	  if $got and not $quiet;

     exit 0;
} else {
     die qq($prog_name: only "install" option takes additional arguments.\n)
	  if @ARGV;
}

# All remaining commands are relative to Pilot dir, so go there...

chdir $base  or die "$prog_name: can't chdir $base: $!\n";

if ($cmd =~ /^newdir(RAM|FLASH|OS|MEMO)(-n)?$/i) {
     my $dir;
     if    (uc($1) eq 'RAM')   { $dir = $dirRAM;   }
     elsif (uc($1) eq 'FLASH') { $dir = $dirFlash; }
     elsif (uc($1) eq 'OS')    { $dir = $dirOS;    }
     elsif (uc($1) eq 'MEMO')  { $dir = $dirMemo;  }
     else		       { die "$prog_name: Huh? dunno [$cmd]\n"; }

     newcopydir($dir, $2);

} elsif ($cmd =~ /^syncmemos(-v)?$/) {
     my $verbose = defined($1) && $1 eq '-v';
     $| = 1  if $verbose;	# for intermingling diff output
     sync_memos("$dirRAM/MemoDB.pdb", $dirMemo, $verbose);

} elsif ($cmd eq 'lsinstall') {
     cmd('ls', '-ls', $dirInst);

} elsif ($cmd =~ /^sync(RAM|Flash|OS)?$/i) {
     my $suffix = lc((defined $1 && $1 ne '') ? $1 : $default_sync);

     my @xfer_cmd = ( $pilot_xfer, '-c', '-a', $dirDel );
     push @xfer_cmd,       '-s', $dirRAM    if $suffix =~ /ram/;
     push @xfer_cmd, '-F', '-s', $dirFlash  if $suffix =~ /flash/;
     push @xfer_cmd, '-O', '-s', $dirOS     if $suffix =~ /os/;

     # If there's anything in the Install dir, append install to command line...

     opendir INSTALL, $dirInst
	  or die "$prog_name: can't opendir $dirInst: $!\n";
     my @newfiles = map { "$dirInst/$_" }
		    sort
		    grep { $_ ne '.' && $_ ne '..' }
		    readdir INSTALL;
     closedir INSTALL;

     push @xfer_cmd, '-i', @newfiles  if @newfiles;

     # Run the pilot-xfer command, and move any installed files into Installed

     if (cmd(@xfer_cmd) == 0) {
	  cmd('mv', @newfiles, $dirInsted)  if @newfiles;
     }
} else {
     print $Usage;
     exit 1;
}

# ------------------------------------------------------------------------

# SUB: newcopydir($dir, $no_copy_opt)
#	Make a new copy of directory $dir, renaming the old one to "$dir,1",
#	and "pushing down" any earlier copies (i.e. "foo,1" --> "foo,2",
#	"foo,2" -> "foo,3", etc.).  If $no_copy_opt is "-n", then $dir
#	will be a new, empty directory, else it will be a fresh copy of
#	what was there before...  Make sense?

sub newcopydir
{
     my $dir = shift;
     my $no_copy_opt = shift;
     my $max_copies = 50;

     die qq($prog_name: <newcopydir> can't see directory "$dir"\n)
	  unless -d $dir;

     foreach my $n (reverse 0..$max_copies) {
	  my($current, $older) = ( "$dir,$n", "$dir," . ($n + 1) );
	  $current = $dir  if $n == 0;

	  if (-e $current) {
	       if (-e $older) {
		    cmd('rm', '-ri', $older) == 0
			 or die "$prog_name: rm $older returned $?\n";
	       }
	       cmd('mv', '-i', $current, $older) == 0
		    or die "$prog_name: mv $current $older returned $?\n";
	  }
     }

     if (defined $no_copy_opt and $no_copy_opt eq '-n') {
	  mkdir700($dir);
     } else {
	  cmd('cp', '-pr', "$dir,1", $dir);
     }
}

# ------------------------------------------------------------------------

# SUB: cmd(@cmd)
#	Simple shorthand to run a command after reporting the action,
#	"sh -x" style.  Returns the result from system().

sub cmd
{
     my @cmd = @_;

     print "+ @cmd\n"  unless $quiet;

     system @cmd;
}

# ------------------------------------------------------------------------

# SUB: mkdir700($dir)
#	Simple shorthand to make a directory and report the action.

sub mkdir700
{
     my $dir = shift;

     print "+ mkdir $dir\n";

     mkdir $dir, 0700
	  or die "$prog_name: can't mkdir $dir: $!\n";
}

# ------------------------------------------------------------------------

# SUB: sync_memos($pdb, $dir, $verbose)
#	Use ctimedir() to note filenames (memo names) and directory names
#	(memo category names) in memo backup directory $dir, then use the
#	'memos' program to unpack the memo database named $pdb into a
#	temporary directory and merge it back into the specified backup
#	directory $dir, noting along the way which memos and/or categories
#	where added, updated, or deleted, and reporting same to the user.
#	If $verbose is non-zero, diff the contents of memos which changed.

sub sync_memos
{
     my($pdb, $dir, $verbose) = @_;
     my (%old, %olddir, %new, %newdir);

     my $basedir = cwd();

     chdir $dir      or die "$prog_name: can't chdir $dir: $!\n";
     ctimedir('.', \%old, \%olddir);
     chdir $basedir  or die "$prog_name: can't chdir $basedir: $!\n";

     my $tmpdir = "$dir/.tmp$$";
     mkdir $tmpdir, 0700  or die "$prog_name: can't mkdir $tmpdir: $!\n";

     system 'memos', '-Q', '-f', $pdb, '-d', $tmpdir;
     die "$prog_name: memos command returned $?\n"  if $? != 0;

     chdir $tmpdir   or die "$prog_name: can't chdir $tmpdir: $!\n";
     ctimedir('.', \%new, \%newdir);
     chdir $basedir  or die "$prog_name: can't chdir $basedir: $!\n";

     my %updated;
     foreach my $memo (keys %new) {
	  if (defined $old{$memo}) {
	       if (compare("$dir/$memo\0", "$tmpdir/$memo\0") != 0) {
		    $updated{$memo} = $new{$memo};
	       }
	       delete $old{$memo};
	       delete $new{$memo};
	  }
     }
     # Memo names: %old contains deleted, %new contains added,
     # %updated contains modified.

     foreach my $dir (keys %newdir) {
	  if (defined $olddir{$dir}) {
	       delete $olddir{$dir};
	       delete $newdir{$dir};
	  }
     }
     # Category names: %olddir contains deleted, %newdir contains added.

     my $printed = 0;

     foreach my $memo (sort keys %updated) {
	  $printed = 1;
	  print "Updated memo: $memo\n";
	  if ($verbose) {
	       print ">>>>>>>>>>>>>>>>>>>>\n";
	       system 'diff', "$dir/$memo", "$tmpdir/$memo";
	       print "<<<<<<<<<<<<<<<<<<<<\n";
	  }
	  rename "$tmpdir/$memo", "$dir/$memo"
	      or warn "$prog_name: can't rename $tmpdir/$memo $dir/$memo: $!\n";
     }

     foreach my $d (sort keys %newdir) {
	  $printed = 1;
	  print "Added category: $d\n";
	  mkdir "$dir/$d", 0700
	      or warn "$prog_name: can't mkdir $dir/$d: $!\n";
     }

     foreach my $memo (sort keys %new) {
	  $printed = 1;
	  print "Added memo: $memo\n";
	  rename "$tmpdir/$memo", "$dir/$memo"
	      or warn "$prog_name: can't rename $tmpdir/$memo $dir/$memo: $!\n";
     }

     foreach my $memo (sort keys %old) {
	  $printed = 1;
	  print "Deleted memo: $memo\n";
	  unlink "$dir/$memo"
	       or warn "$prog_name: can't unlink $dir/$memo: $!\n";
     }

     foreach my $d (sort keys %olddir) {
	  $printed = 1;
	  print "Deleted category: $d\n";
	  rmdir "$dir/$d"
	      or warn "$prog_name: can't rmdir $dir/$d: $!\n";
     }

     unless ($printed) {
	  print "No memo changes.\n";
     }

     system 'rm', '-r', $tmpdir;
     warn "$prog_name: rm -r $tmpdir returned $?\n"  if $? != 0;
}

# ------------------------------------------------------------------------

# SUB: ctimedir($dirname, $filehash, $dirhash)
#	Given a directory name $dirname, descend directory tree, collecting
#	ctimes for files found, in %$filehash (filename => ctime), and 
#	(sub-) directory names in %$dirhash (dirname => 1).

sub ctimedir
{
     my($dirname, $filehash, $dirhash) = @_;
     local *DIR;

     opendir DIR, $dirname  or die "$prog_name: can't open dir $dirname: $!\n";
     my @files = map { "$dirname/$_" } sort grep { !/^\.\.?$/ } readdir DIR;
     closedir DIR;

     foreach my $file (@files) {
	  $file =~ s#^\./##;
	  if (-d $file) {
	       $dirhash->{$file} = 1;
	       ctimedir($file, $filehash, $dirhash);
	  } elsif (-f $file) {
	       $filehash->{$file} = (stat _)[10];
	  } elsif (-e $file) {
	       warn "$prog_name: $file is neither file nor directory\n";
	  } else {
	       warn "$prog_name: $file disappeared\n";
	  }
     }
}

# ------------------------------------------------------------------------
