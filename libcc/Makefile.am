lib_LTLIBRARIES = libpicc.la

INCLUDES = -I$(top_srcdir)/libsock

libpiccdir = $(includedir)

libpicc_HEADERS = \
	pi-address.hxx		\
	pi-appinfo.hxx		\
	pi-datebook.hxx		\
	pi-dlp.hxx		\
	pi-iambicExpense.hxx	\
	pi-memo.hxx		\
	pi-todo.hxx

libpicc_la_SOURCES = \
	address.cc	\
	appInfo.cc	\
	datebook.cc	\
	dlp.cc		\
	iambicExpense.cc\
	memo.cc		\
	todo.cc

libpicc_la_LDFLAGS = -version-info 4:0:0