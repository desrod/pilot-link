#!/usr/bin/perl

# Conjure up OS/2 specific makefiles

open(STDIN,"<Makefile.in");
open(STDOUT,">Makefile.os2");
while(<>) {
	s/\@RANLIB\@/ar -s/g;
	s/\@CC\@/gcc/g;
	s/\@CFLAGS\@/-g -O2 -fno-strength-reduce/g;
	s/\@CWFLAG\@/-Wall/g;
	s/\@CPLIB\@/cp/g;
	s/\@YACC\@/bison -y/g;
	s/^LIBS\s+=.*/$& -lsocket -los2/;
	s/^EXT\s+=.*/EXT = .EXE/;
	s/^SUBMAKE_COMM\s+=.*/SUBMAKE_COMM = \$(MAKE) -C lib -f Makefile.os2/;
	print;
}

open(STDIN,"<lib/Makefile.in");
open(STDOUT,">lib/Makefile.os2");
while(<>) {
	s/\@RANLIB\@/ar -s/g;
	s/\@CC\@/gcc/g;
	s/\@CFLAGS\@/-g -O2 -fno-strength-reduce/g;
	s/\@CWFLAG\@/-Wall/g;
	s/\@ARFLAGS\@/-cur/g;
	s#../include/pi-config.h# #g;
	print;
}
