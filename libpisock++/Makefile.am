if WITH_CXX

INCLUDES = 			\
	-I../include		\
	-I$(top_srcdir)/include

lib_LTLIBRARIES = libpisock++.la

libpisock___la_SOURCES = 	\
	address.cc	\
	appInfo.cc	\
	datebook.cc	\
	dlp.cc		\
	memo.cc		\
	todo.cc

libpisock___la_LIBADD = \
	$(top_builddir)/libpisock/libpisock.la

libpisock___la_LDFLAGS = \
	-export-dynamic -version-info $(PISOCKPP_CURRENT):$(PISOCKPP_REVISION):$(PISOCKPP_AGE)

libpisock___la_CXXFLAGS = $(PIC_LIBS)
endif

EXTRA_DIST =							\
	address.cc appInfo.cc datebook.cc dlp.cc memo.cc	\
	todo.cc
