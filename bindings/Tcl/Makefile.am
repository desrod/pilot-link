INCLUDES = 				\
	-I../../include			\
	-I$(top_srcdir)/include		\
	-DPACKAGE_VERSION=\"1.0\"	\
	$(TCL_INCLUDES)

bin_PROGRAMS = pitclsh
lib_LTLIBRARIES = libpitcl.la

libpitcl_la_SOURCES = pitcl.c

libpitcl_la_LIBADD = 				\
	$(TCL_LIB_SPEC)				\
	$(top_builddir)/libpisock/libpisock.la

libpitcl_la_LDFLAGS = \
	-export-dynamic -version-info 0:0:0

pitclsh_SOURCES = tclAppInit.c

pitclsh_LDADD = 				\
	$(TCL_LIB_SPEC)				\
	$(top_builddir)/libpisock/libpisock.la	\
	libpitcl.la
