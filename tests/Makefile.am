INCLUDES = 			\
	-I../include		\
	-I$(top_srcdir)/include \
	$(POPT_INCLUDES)

noinst_PROGRAMS =		\
	calendardb-test 	\
	locationdb-test 	\
	contactsdb-test		\
	dlp-test		\
	versamail-test		\
	vfs-test		\
	contactsdb-test

contactsdb_jps_SOURCES =	\
	contactsdb-jps.c
contactsdb_jps_LDADD =		\
	$(top_builddir)/libpisock/libpisock.la

calendardb_test_SOURCES =	\
	calendardb-test.c
calendardb_test_LDADD =		\
	$(top_builddir)/libpisock/libpisock.la

locationdb_test_SOURCES =	\
	locationdb-test.c
locationdb_test_LDADD =		\
	$(top_builddir)/libpisock/libpisock.la

contactsdb_test_SOURCES =	\
	contactsdb-test.c
contactsdb_test_LDADD =		\
	$(top_builddir)/libpisock/libpisock.la

dlp_test_SOURCES =		\
	dlp-test.c
dlp_test_LDADD =		\
	$(top_builddir)/libpisock/libpisock.la

vfs_test_SOURCES =		\
	vfs-test.c
vfs_test_LDADD =		\
	$(top_builddir)/libpisock/libpisock.la

versamail_test_SOURCES =	\
	versamail-test.c
versamail_test_LDADD =		\
	$(top_builddir)/libpisock/libpisock.la

check_PROGRAMS =  		\
	packers

packers_SOURCES = 		\
	packers.c
packers_LDADD = 		\
	$(top_builddir)/libpisock/libpisock.la

TESTS = packers
