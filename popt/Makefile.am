if INCLUDED_POPT

INCLUDES = -I$(top_srcdir)/include

noinst_HEADERS = findme.h poptint.h system.h popt.h

noinst_LTLIBRARIES = libpopt.la

libpopt_la_SOURCES = popt.c findme.c poptparse.c poptconfig.c popthelp.c

.PHONY: sources
sources:
	@echo $(libpopt_la_SOURCES:%=popt/%)
        
.PHONY: lclint
lclint:
	lclint ${DEFS} ${INCLUDES} ${libpopt_la_SOURCES}

endif

EXTRA_DIST =							\
	findme.c findme.h poptint.h system.h popt.c popt.h	\
	poptparse.c poptconfig.c popthelp.c

