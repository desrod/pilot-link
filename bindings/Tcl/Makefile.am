# $Id$

INCLUDES = 					\
	-I../../include				\
	-I$(top_srcdir)/include			\
	-DPACKAGE_VERSION=\"1.0\"		\
	$(POPT_INCLUDES)			\
	$(TCL_INCLUDES)

noinst_LTLIBRARIES = libpiuserland.la

libpiuserland_la_SOURCES =			\
	$(top_builddir)/src/plu_args.c		\
	$(top_builddir)/src/userland.c

libpiuserland_la_LDFLAGS = -static

bin_PROGRAMS = pitclsh

lib_LTLIBRARIES = libpitcl.la

libpitcl_la_SOURCES = pitcl.c

libpitcl_la_LIBADD = 				\
	$(TCL_LIB_SPEC)				\
	$(POPT_LIBS)				\
	$(top_builddir)/src/libpiuserland.la	\
	$(top_builddir)/libpisock/libpisock.la

libpitcl_la_LDFLAGS = \
	-export-dynamic -version-info 0:0:0

pitclsh_SOURCES = tclAppInit.c

pitclsh_LDADD = 				\
	$(TCL_LIB_SPEC)				\
	$(POPT_LIBS)				\
	$(top_builddir)/src/libpiuserland.la	\
	$(top_builddir)/libpisock/libpisock.la	\
	libpitcl.la
