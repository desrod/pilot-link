INCLUDES = 			\
	-I../include		\
	-I$(top_srcdir)/include	\
	$(ICONV_CFLAGS)

lib_LTLIBRARIES = libpisync.la

libpisync_la_SOURCES =	\
	sync.c

libpisync_la_LIBADD = \
	$(top_builddir)/libpisock/libpisock.la \
	$(ICONV_LIBS)

libpisync_la_LDFLAGS = \
	-export-dynamic -version-info $(PISYNC_CURRENT):$(PISYNC_REVISION):$(PISYNC_AGE)
