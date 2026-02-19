AUTOMAKE_OPTIONS = 1.6 subdir-objects

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

# Clean up files created by build-deb.sh (Debian package build)
clean-deb:
	@if test -f debian/rules; then \
		$(MAKE) -f debian/rules clean; \
	fi

clean-deb-packages:
	rm -f $(top_builddir)/../pilot-link_*.deb \
	      $(top_builddir)/../pilot-link_*.changes \
	      $(top_builddir)/../pilot-link_*.dsc \
	      $(top_builddir)/../pilot-link_*.buildinfo \
	      $(top_builddir)/../pilot-link_*.tar.xz \
	      $(top_builddir)/../pilot-link-dbgsym_*.ddeb

