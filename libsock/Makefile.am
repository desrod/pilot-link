INCLUDES = \
	-I$(top_srcdir)/include

serial_FILES = \
	unixserial.c 	\
	winserial.c	\
	os2serial.c 
serial_SOURCE = unixserial.c

lib_LTLIBRARIES = libpisock.la

libpisock_la_SOURCES =  \
	$(serial_SOURCE)\
	address.c	\
	appinfo.c	\
	cmp.c		\
	datebook.c	\
	dlp.c		\
	expense.c	\
	hinote.c	\
	inet.c		\
	inetserial.c	\
	mail.c		\
	md5.c		\
	memo.c		\
	money.c		\
	padp.c		\
	pi-file.c	\
	pi-header.c	\
	serial.c	\
	slp.c		\
	socket.c	\
	sync.c		\
	syspkt.c	\
	todo.c		\
	util.c		\
	utils.c

libpisock_la_LDFLAGS = -export-dynamic -version-info 4:0:0

EXTRA_DIST = $(serial_FILES)