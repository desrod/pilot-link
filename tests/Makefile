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

CC = gcc
CXX = c++
CDEFS = -DLIBDIR=\"$(libdir)\" 
# -DDEBUG
CFLAGS = -I$(top_builddir)/include -I$(srcdir)/include -g -O2 -Wall $(RLDEFS) $(CDEFS) $(TCLTK_FLAGS)
CXXFLAGS = -I$(top_builddir)/include -I$(srcdir)/include -g -O2 -Wall $(RLDEFS) $(CDEFS) $(TCLTK_FLAGS)
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

CEXECS = packers$(EXT)

CXXEXECS =

TCLEXECS =

PERLEXECS = 

EXECS = $(CEXECS) $(WITHTCL) $(WITHCXX) $(WITHPERL5)

WITHCXXLIB = ../libcc/libpicc.la
PILIB = ../libsock/libpisock.la
PICCLIB = $(WITHCXXLIB)

all: $(PILIB) $(PICCLIB) $(EXECS) 

$(PILIB): FORCE
	$(SUBMAKE_COMM)

$(WITHCXXLIB): FORCE
	$(SUBMAKE_COMM_CC)

tests test: $(EXECS)
	packers$(EXT)

packers$(EXT): $(PILIB) packers.o
	$(CCLINK) packers.o $(PILIB) -o $@ $(LIBS)

distclean: clean
	$(RM) Makefile

clean:
	$(RM) *.o *.a *.lo *.la core a.out test_s test_c install-prc$(EXT) pilot-debug$(EXT)
	$(RM) -rf .libs _libs
	$(RM) $(CEXECS) $(TCLEXECS) $(CXXEXECS) $(PERLEXECS)
	
FORCE:
