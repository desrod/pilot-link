if WITH_CXX
CXX_DIR = libpisock++
else
CXX_DIR =
endif

SUBDIRS=popt libpisock libpisync $(CXX_DIR) include src bindings doc tests

m4dir = $(datadir)/aclocal
m4_DATA = pilot-link.m4 tcl.m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pilot-link.pc 

if WITH_CXX
pppkgconfigdir = $(libdir)/pkgconfig
pppkgconfig_DATA = pilot-link-pp.pc
endif

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = 			\
	$(m4_DATA)		\
	pilot-link.pc.in	\
	pilot-link-pp.pc.in	\
	darwin/build_framework.sh
