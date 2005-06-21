AUTOMAKE_OPTIONS = 1.6

SUBDIRS=popt 		\
	libpisock 	\
	libpisync 	\
	include 	\
	src 		\
	bindings 	\
	doc 		\
	tests 		\
	darwin

m4dir = $(datadir)/aclocal
m4_DATA = pilot-link.m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pilot-link.pc 

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = 			\
	autogen.sh		\
	$(m4_DATA)		\
	pilot-link.pc.in	\
	pilot-link-pp.pc.in

