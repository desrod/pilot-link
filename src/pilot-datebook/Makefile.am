INCLUDES = 				\
	-I../../include			\
	-I.				\
	-I$(top_srcdir)/include

bin_PROGRAMS = 				\
	pilot-datebook	

pilot_datebook_SOURCES = 		\
	parsedate.y			\
	pilot-datebook.c		\
	pilot-datebook-compare.c	\
	pilot-datebook-csv.c		\
	pilot-datebook-data.c		\
	pilot-datebook-hotsync.c	\
	pilot-datebook-ical.c		\
	pilot-datebook-io.c		\
	pilot-datebook-job.c		\
	pilot-datebook-joblist.c	\
	pilot-datebook-longtxt.c	\
	pilot-datebook-pdb.c		\
	pilot-datebook-remind.c		\
	pilot-datebook-shorttxt.c	\
	pilot-datebook-sort.c		\
	pilot-datebook-update.c		\
	pilot-datebook-windat.c	

pilot_datebook_LDADD =			\
	@LIBOBJS@			\
	$(top_builddir)/libpisock/libpisock.la	

EXTRA_DIST =				\
        getopt.h

