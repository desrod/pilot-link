# Generated automatically from Makefile.PL.in by configure.
use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.

$prefix = "/usr/pilot";
$exec_prefix = "${prefix}";
$libdir = "${exec_prefix}/lib";

unless ((-e "$libdir/libpisock.la") or (-e "$libdir/libpisock.a")) {
	die "pilot-link must be installed before PDA::Pilot can be built\n";
}

$lib = "-L$libdir -lpisock";

WriteMakefile(
    'NAME'	=> 'PDA::Pilot',
    'VERSION'	=> '0.1',
    'LIBS'	=> $lib,
    'INC'	=> '-I../include',
);
