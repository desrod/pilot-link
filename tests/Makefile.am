INCLUDES = 			\
	-I../include		\
	-I$(top_srcdir)/include

noinst_PROGRAMS =	\
	dlp-test

dlp_test_SOURCES =	\
	dlp-test.c
dlp_test_LDADD =	\
	$(top_builddir)/libpisock/libpisock.la

check_PROGRAMS =  \
	packers

packers_SOURCES = \
	packers.c
packers_LDADD = \
	$(top_builddir)/libpisock/libpisock.la

TESTS = packers

