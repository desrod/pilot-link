if WITH_CXX
CXX_DIR = libpisock++
else
CXX_DIR =
endif

SUBDIRS=libpisock libpisync $(CXX_DIR) include src bindings doc tests

m4dir = $(datadir)/aclocal
m4_DATA = pilot-link.m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = 			\
	pilot-link.pc			\
	pilot-link-pp.pc

EXTRA_DIST = 				\
	$(m4_DATA)			\
	$(pkgconfig_DATA:.pc=.pc.in)

