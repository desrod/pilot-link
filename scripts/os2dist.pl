#!/usr/bin/perl

# Conjure up OS/2 specific makefiles

%defs = ( '@RANLIB@', 'ar -s', '@CC@', 'gcc', '@CFLAGS@', '-g -O2 -fno-strength-reduce',
          '@CWFLAG@', '-Wall', '@CPLIB@', 'cp', '@YACC@', 'bison -y', 
          '@LIBS@', '-lsocket -los2', '@EXT@', '.EXE',
          '@SUBMAKE_COMM@', '$(MAKE) -C libsock -f Makefile.os2',
          '@SUBMAKE_COMM_CC@', '$(MAKE) -C libcc -f Makefile.os2',
          '@CXX@', 'gcc',
          '@CXXFLAGS@', '-g -O2 -fno-strength-reduce' ,
          '@CXXLIBS@', '-lstdcpp',
          '@ARFLAGS@', '-cur',
          '@cclib@', 'libpicc.a', '@libcclib@', 'libcc/libpicc.a',
          '@ccexecs@', '$(CCEXECS)',
	  '@LDFLAGS@', ''
        );
        
$defs{'@srcdir@'} = './';

open(STDIN,"<Makefile.in") or die "Unable to read Makefile.in";
open(STDOUT,">Makefile.os2") or die "Unable to write to Makefile.os2";
while(<>) {
    foreach $k (keys %defs) {
      s/$k/$defs{$k}/g;
    }
	print;
}

$defs{'@srcdir@'} = '../';

open(STDIN,"<libsock/Makefile.in") or die "Unable to read Makefile.in";
open(STDOUT,">libsock/Makefile.os2") or die "Unable to write to libsock/Makefile.os2";
while(<>) {
    foreach $k (keys %defs) {
      s/$k/$defs{$k}/g;
    }
	s#../include/pi-config.h# #g;
	s#unixserial#os2serial#g;
	print;
}

open(STDIN,"<libcc/Makefile.in") or die "Unable to read Makefile.in";
open(STDOUT,">libcc/Makefile.os2") or die "Unable to write to libcc/Makefile.os2";
while(<>) {
    foreach $k (keys %defs) {
      s/$k/$defs{$k}/g;
    }
	s#../include/pi-config.h# #g;
	print;
}
