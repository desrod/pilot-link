INCLUDES = 				\
	-I../../include			\
	-I.				\
	-I$(top_srcdir)/include

bin_PROGRAMS = 				\
	pilot-datebook	

if NEED_GETOPT
getopt_sources = getopt.h getopt.c getopt1.c
else
getopt_sources = 
endif

pilot_datebook_SOURCES = 		\
	$(getopt_sources)		\
	parsedate.y			\
	pilot-datebook.c		\
	pilot-datebook.h		\
	pilot-datebook-compare.c	\
	pilot-datebook-compare.h	\
	pilot-datebook-csv.c		\
	pilot-datebook-csv.h		\
	pilot-datebook-data.c		\
	pilot-datebook-data.h		\
	pilot-datebook-hotsync.c	\
	pilot-datebook-hotsync.h	\
	pilot-datebook-ical.c		\
	pilot-datebook-ical.h		\
	pilot-datebook-io.c		\
	pilot-datebook-io.h		\
	pilot-datebook.h		\
	pilot-datebook-job.c		\
	pilot-datebook-job.h		\
	pilot-datebook-joblist.c	\
	pilot-datebook-joblist.h	\
	pilot-datebook-longtxt.c	\
	pilot-datebook-longtxt.h	\
	pilot-datebook-pdb.c		\
	pilot-datebook-pdb.h		\
	pilot-datebook-remind.c		\
	pilot-datebook-remind.h		\
	pilot-datebook-shorttxt.c	\
	pilot-datebook-shorttxt.h	\
	pilot-datebook-sort.c		\
	pilot-datebook-sort.h		\
	pilot-datebook-update.c		\
	pilot-datebook-update.h		\
	pilot-datebook-windat.c		\
	pilot-datebook-windat.h

pilot_datebook_LDADD =			\
	$(top_builddir)/libpisock/libpisock.la	

EXTRA_DIST = 			\
	getopt.h		\
	getopt.c		\
	getopt1.c
