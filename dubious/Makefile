# Generated automatically from Makefile.in by configure.

prefix = /usr/pilot
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin
libdir = ${exec_prefix}/lib
pixdir = ${libdir}/pix
includedir = ${prefix}/include
mandir = ${prefix}/man/man1
srcdir = .

top_builddir = ..

PURELINK =
PURIFY =

# Libraries needed to link with Tcl & (optionally) Tk
TCLTK_LIBS = -L/usr/local/tcl8.0/lib -ltk8.0 -L/usr/local/tcl8.0/lib -ltcl8.0 -L/usr/X11R6/lib -lX11 -ldl  -lm
# Include directives needed to compile with Tcl & Tk, and
# -DTCL if Tcl is available, and
# -DTK if Tk is available.
TCLTK_FLAGS = -DTCL -DTK -I/usr/local/tcl8.0/include

# Libraries needed to link with GNU readline
RLLIBS = -lreadline -ltermcap
# Include directives needed to compile with GNU readline, and
# -DREADLINE_2_0 if GNU readline 2.0 is available, or 
# -DREADLINE_2_1 if GNU readline 2.1 (or later) is available.
RLDEFS = -DREADLINE_2_0

PERL5 = /usr/local/bin/perl

CC = gcc
CXX = c++
CDEFS = -DLIBDIR=\"$(libdir)\" 
# -DDEBUG
CFLAGS = -I../include -I$(srcdir)/include -g -O2 -Wall $(RLDEFS) $(CDEFS) $(TCLTK_FLAGS)
CXXFLAGS = -I../include -I$(srcdir)/include -g -O2 -Wall $(RLDEFS) $(CDEFS) $(TCLTK_FLAGS)
LDFLAGS = 
LIBS =  -lm
# -lefence
CXXLIBS =  -lg++

libcclib = libcc/libpicc.a
cclib = libpicc.a

INSTALL = $(LIBTOOL) /usr/bin/ginstall -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
YACC = bison -y
RANLIB = ranlib
RM = rm -f
STRIP = strip
SHELL = /bin/sh
CPLIB = cp
EXT = 
SUBMAKE_COMM = cd $(top_builddir)/libsock ; $(MAKE) 
SUBMAKE_COMM_CC = cd $(top_builddir)/libcc ; $(MAKE) 

LIBTOOL = $(top_builddir)/libtool
LIBTOOLLINK = $(LIBTOOL) --mode=link

CCLINK = $(LIBTOOLLINK)  $(PURELINK) $(PURIFY) $(CC) $(LDFLAGS) $(CFLAGS)
CXXLINK = $(LIBTOOLLINK) $(PURELINK) $(PURIFY) $(CXX) $(LDFLAGS) $(CXXFLAGS)

.SUFFIXES: .out .o .cc .c .e .r .f .y .l .s .p

.c.o:
	$(CC) $(CFLAGS) -c $<

.cc.o:
	$(CXX) $(CXXFLAGS) -c $<


WITHTCL = $(TCLEXECS)
WITHCXX = $(CXXEXECS)
WITHPERL5 = $(PERLEXECS)

CEXECS = sync-memodir$(EXT) sync-ical$(EXT) \
		test-connector$(EXT) test-acceptor$(EXT)

EXECS = $(CEXECS) $(WITHTCL) $(WITHCXX) $(WITHPERL5)

PILIB = ../libsock/libpisock.la

all: $(EXECS)

$(PILIB): FORCE
	$(SUBMAKE_COMM) libpisock.la

sync-memodir$(EXT): $(PILIB) sync-memodir.o
	$(CCLINK) sync-memodir.o $(PILIB) -o $@ $(LIBS)

sync-ical$(EXT): $(PILIB) sync-ical.o
	$(CCLINK) sync-ical.o $(PILIB) -o $@ $(LIBS)

test-connector$(EXT): $(PILIB) test-connector.o
	$(CCLINK) test-connector.o $(PILIB) -o $@ $(LIBS)

test-acceptor$(EXT): $(PILIB) test-acceptor.o
	$(CCLINK) test-acceptor.o $(PILIB) -o $@ $(LIBS)


FORCE:

depend:
	perl -i~ -ne 'print if 1../^#Depend/' Makefile.in ; gcc -MM -I../include *.c >> Makefile.in

distclean: clean
	$(RM) Makefile *~

clean:
	$(RM) *.o *.a *.lo *.la core a.out
	$(RM) -rf .libs _libs
	$(RM) $(CEXECS) $(TCLEXECS) $(CXXEXECS) $(PERLEXECS) validate 

#Depend information starts here. Do not edit the text beyond this point!
sync-ical.o: sync-ical.c ../include/pi-source.h ../include/pi-config.h \
 ../include/pi-socket.h ../include/pi-args.h ../include/pi-version.h \
 ../include/pi-sockaddr.h ../include/pi-macros.h ../include/pi-sync.h \
 ../include/pi-datebook.h ../include/pi-appinfo.h ../include/pi-todo.h \
 ../include/pi-dlp.h
sync-memodir.o: sync-memodir.c ../include/pi-source.h \
 ../include/pi-config.h ../include/pi-socket.h ../include/pi-args.h \
 ../include/pi-version.h ../include/pi-sockaddr.h \
 ../include/pi-macros.h ../include/pi-datebook.h \
 ../include/pi-appinfo.h ../include/pi-todo.h ../include/pi-dlp.h \
 ../include/pi-sync.h
test-acceptor.o: test-acceptor.c ../include/pi-source.h \
 ../include/pi-config.h ../include/pi-socket.h ../include/pi-args.h \
 ../include/pi-version.h ../include/pi-sockaddr.h \
 ../include/pi-macros.h ../include/pi-dlp.h
test-connector.o: test-connector.c ../include/pi-source.h \
 ../include/pi-config.h ../include/pi-socket.h ../include/pi-args.h \
 ../include/pi-version.h ../include/pi-sockaddr.h \
 ../include/pi-macros.h ../include/pi-dlp.h
