INCLUDES = 			\
	-I../include		\
	-I$(top_srcdir)/include	\
	$(ICONV_CFLAGS)

serial_FILES = 		\
	unixserial.c 	\
	winserial.c	\
	os2serial.c 
serial_SOURCE = unixserial.c

usb_FILES = 		\
	usb.c		\
	linuxusb.c	\
	freebsdusb.c

if WITH_LINUXUSB
usb_SOURCE = usb.c linuxusb.c
else
if WITH_FREEBSDUSB
usb_SOURCE = usb.c freebsdusb.c
else
usb_SOURCE =
endif
endif
lib_LTLIBRARIES = libpisock.la

libpisock_la_SOURCES =  \
	$(serial_SOURCE)\
	$(usb_SOURCE)	\
	address.c	\
	appinfo.c	\
	connect.c	\
	cmp.c		\
	datebook.c	\
	debug.c		\
	dlp.c		\
	expense.c	\
	hinote.c	\
	inet.c		\
	mail.c		\
	md5.c		\
	memo.c		\
	money.c		\
	net.c		\
	notepad.c	\
	padp.c		\
	palmpix.c	\
	pi-file.c	\
	pi-header.c	\
	serial.c	\
	slp.c		\
	sys.c		\
	socket.c	\
	syspkt.c	\
	todo.c		\
	util.c		\
	utils.c

libpisock_la_LIBADD = \
	$(ICONV_LIBS)

libpisock_la_LDFLAGS = \
	-export-dynamic -version-info $(PISOCK_CURRENT):$(PISOCK_REVISION):$(PISOCK_AGE)

EXTRA_DIST = $(serial_FILES) $(usb_FILES)
