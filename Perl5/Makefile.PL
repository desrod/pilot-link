# Generated automatically from Makefile.PL.in by configure.
use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.

if (! -e "../libtool") {
   	die "pilot-link must be installed before PDA::Pilot can be built\n";
}

$dlname = `../libtool --mode=dlname ../libsock/libpisock.la`;
chop $dlname;
if ($?) {
	print "(Ignore warning)\n";
	$lib = "-L../libsock/.libs -lpisock";
} else {
    if (! -e $dlname) {
    	die "pilot-link must be installed before PDA::Pilot can be built\n";
    }
	$dlname =~ s#/[^/]*$##;
	$lib = "-L$dlname -lpisock";
}

WriteMakefile(
    'NAME'	=> 'PDA::Pilot',
    'VERSION'	=> '0.1',
    'LIBS'	=> $lib,
    'INC'	=> '-I../include',
);
