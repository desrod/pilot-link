# Generated automatically from Makefile.in by configure.

PACKAGE_VERSION = 1.0

prefix = /usr/pilot
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin
libdir = ${exec_prefix}/lib
pixdir = ${libdir}/pix
includedir = ${prefix}/include
mandir = ${prefix}/man/man1
srcdir = ../.

TCL_BIN = /usr/local/tcl8.0/bin
TCL_VERSION = 8.0

TK_VERSION = 8.0
TCL_SH = /usr/local/tcl8.0/bin/tclsh8.0

TCLTK_FLAGS = -DTCL -DTK -I/usr/local/tcl8.0/include

TCL_LIBS = -L/usr/local/tcl8.0/lib -ltcl8.0 -ldl  -lm
TK_LIBS = -L/usr/local/tcl8.0/lib -ltk8.0 -L/usr/local/tcl8.0/lib -ltcl8.0 -L/usr/X11R6/lib -lX11 -ldl  -lm

PACKAGES = /usr/local/tcl8.0/lib

top_builddir = ..

CC = gcc
CDEFS = -DPACKAGE_VERSION=\"$(PACKAGE_VERSION)\" -DLIBDIR=\"$(libdir)\"
CFLAGS = -I../include -I$(srcdir)/include -g -O2 -Wall $(CDEFS) $(TCLTK_FLAGS)
# -DDEBUG
LDFLAGS = 
LIBS =  -lm
# -lefence


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

CCLINK = $(LIBTOOL) --mode=link $(PURELINK) $(PURIFY) $(CC) $(LDFLAGS) $(CFLAGS)

OBJS = pdapilot.lo

LIB = pdapilot$(PACKAGE_VERSION).$(TCL_VERSION).la

TCLINTERP = pitclsh$(PACKAGE_VERSION).$(TCL_VERSION)
TKINTERP = piwish$(PACKAGE_VERSION).$(TK_VERSION)

WITHTCL = $(TCLINTERP)
WITHTK = $(TKINTERP)

INTERPS = $(WITHTCL) $(WITHTK)

PACKLIB = $(PACKAGES)/pdapilot$(PACKAGE_VERSION)

.SUFFIXES: .out .o .lo .cc .c .e .r .f .y .l .s .p

.c.lo:
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) -c $<

all: $(LIB) $(INTERPS)

$(TCLINTERP): tclAppInit.o $(LIB)
	$(CCLINK) -o $(TCLINTERP) tclAppInit.o $(LIB) $(TCL_LIBS)

$(TKINTERP): tkAppInit.o $(LIB)
	$(CCLINK) -o $(TKINTERP) tkAppInit.o $(LIB) $(TK_LIBS)

installdirs:
	$(SHELL) ${srcdir}/scripts/mkinstalldirs $(PACKAGES) $(PACKLIB)

install: installdirs $(LIB)
	$(INSTALL_DATA) $(LIB) $(PACKAGES)/$(LIB)
	echo 'package ifneeded Pdapilot $(PACKAGE_VERSION).$(TCL_VERSION) [list load "[file join [file dirname $$dir] pdapilot$(PACKAGE_VERSION).$(TCL_VERSION).so]" Pdapilot]' >> $(PACKLIB)/pkgIndex.tcl 
	$(INSTALL_PROGRAM) $(TCLINTERP) $(TCL_BIN)/$(TCLINTERP)
	$(INSTALL_PROGRAM) $(TKINTERP) $(TCL_BIN)/$(TKINTERP)
	$(LIBTOOL) -n --finish $(PACKAGES)

uninstall: 
	-$(LIBTOOL) $(RM) $(PACKAGES)/$(LIB)
	grep -v '$(PACKAGE_VERSION).$(TCL_VERSION)' < $(PACKLIB)/pkgIndex.tcl > /tmp/plpkgIdx; cat /tmp/plpkgIdx > $(PACKLIB)/pkgIndex.tcl ; rm /tmp/plpkgIdx
	$(LIBTOOL) $(RM) $(TCL_BIN)/$(TCLINTERP)
	$(LIBTOOL) $(RM) $(TCL_BIN)/$(TKINTERP)
	$(LIBTOOL) -n --finish $(PACKAGES)

$(LIB): $(OBJS) Makefile
	$(LIBTOOL) --mode=link $(CC) $(CFLAGS) \
		-export-dynamic -rpath $(PACKAGES) \
		-o $(LIB) $(OBJS) -L$(libdir) -lpisock

test: $(TCLINTERP) $(LIB)
	$(TCLINTERP) test.tcl

clean:
	$(RM) *.o *.a *.lo *.la core a.out *.so pitclsh* piwish*
	$(RM) -rf .libs _libs
	$(RM) *~
