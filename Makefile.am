if WITH_CXX
CXX_DIR = libpisock++
else
CXX_DIR =
endif

SUBDIRS=libpisock $(CXX_DIR) include src bindings doc tests

m4dir = $(datadir)/aclocal
m4_DATA = pilot-link.m4

EXTRA_DIST = $(m4_DATA)

