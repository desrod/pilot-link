#!/usr/bin/perl

# Conjure up OS/2 specific makefiles

%defs = ( '@RANLIB@', 'ar -s', '@CC@', 'gcc', 
	  '@CFLAGS@', '-g -Zcrtdll -O2 -fno-strength-reduce -D__ST_MT_ERRNO__',
          '@CWFLAG@', '-Wall', '@CPLIB@', 'cp', '@YACC@', 'bison -y', 
          '@LIBS@', '-lsocket -los2', '@EXT@', '.EXE',
          '@SUBMAKE_COMM@', '$(MAKE) -C $(top_builddir)/libsock -f Makefile.os2',
          '@SUBMAKE_COMM_CC@', '$(MAKE) -C $(top_builddir)/libcc -f Makefile.os2',
          '@CXX@', 'gcc',
          '@CXXFLAGS@', '-g -O2 -fno-strength-reduce' ,
          '@CXXLIBS@', '-lstdcpp',
          '@ARFLAGS@', '-cur',
          '@cclib@', 'libpicc.a', '@libcclib@', 'libcc/libpicc.a',
          '@ccexecs@', '$(CCEXECS)',
	  '@LDFLAGS@', '',
	  '@WITHTCL@', 'WITHOUTTCL',
	  '@WITHTK@', 'WITHOUTTK',
	  '@WITHPYTHON@', 'WITHOUTPYTHON',
	  '@WITHPERL5@', 'WITHOUTPERL5',
	  '@WITHJAVA@', 'WITHOUTJAVA',
	  '@TCLTKLIBS@', '',
	  '@TCLTKFLAGS@', '',
	  '@RLDEFS@', '',
	  '@RLLIBS@', '',
	  '@WITHCXX@', 'WITHCXX'
        );
        
$defs{'@srcdir@'} = './';

open(STDIN,"<Makefile.in") or die "Unable to read Makefile.in";
open(STDOUT,">Makefile.os2") or die "Unable to write to Makefile.os2";
while(<>) {
    foreach $k (keys %defs) {
      s/$k/$defs{$k}/g;
    }
    s/libpisock.la/libpisockdll.a/g;
    s/\.la/.a/g;
    s/^LIBTOOL = (.*)$/LIBTOOL =/;
    s/^LIBTOOLLINK = (.*)$/LIBTOOLLINK =/;
	print;
}

$defs{'@srcdir@'} = '../';

open(STDIN,"<libsock/Makefile.in") or die "Unable to read libsock/Makefile.in";
open(STDOUT,">libsock/Makefile.os2") or die "Unable to write to libsock/Makefile.os2";
while(<>) {
    foreach $k (keys %defs) {
      s/$k/$defs{$k}/g;
    }
    s#\.la#.a# if /^all:/;
	s#\.\./include/pi-config.h# #g;
	s#unixserial#os2serial#g;
	s#O = lo#O = o#g;
	print;
}

open(STDIN,"<libcc/Makefile.in") or die "Unable to read libcc/Makefile.in";
open(STDOUT,">libcc/Makefile.os2") or die "Unable to write to libcc/Makefile.os2";
while(<>) {
    foreach $k (keys %defs) {
      s/$k/$defs{$k}/g;
    }
	s#\.\./include/pi-config.h# #g;
	print;
}


open(STDIN,"<tests/Makefile.in") or die "Unable to read tests/Makefile.in";
open(STDOUT,">tests/Makefile.os2") or die "Unable to write to tests/Makefile.os2";
while(<>) {
    foreach $k (keys %defs) {
      s/$k/$defs{$k}/g;
    }
	s#\.\./include/pi-config.h# #g;
	print;
}
