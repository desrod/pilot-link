INCLUDES = 			\
	-I../include		\
	-I$(top_srcdir)/include	\
	$(ICONV_CFLAGS)

serial_FILES = 		\
	unixserial.c 	\
	os2serial.c
serial_SOURCE = unixserial.c

usb_FILES = 		\
	usb.c		\
	libusb.c	\
	linuxusb.c	\
	freebsdusb.c	\
	darwinusb.c

if WITH_LIBUSB
usb_SOURCE = usb.c libusb.c
endif
if WITH_LINUXUSB
usb_SOURCE = usb.c linuxusb.c
endif

if WITH_FREEBSDUSB
usb_SOURCE = usb.c freebsdusb.c
endif

if WITH_DARWINUSB
usb_SOURCE = usb.c darwinusb.c
endif

lib_LTLIBRARIES = libpisock.la

libpisock_la_SOURCES =  \
	$(serial_SOURCE)\
	$(usb_SOURCE)	\
	address.c	\
	appinfo.c	\
	connect.c	\
	contact.c	\
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
	pi-buffer.c	\
	pi-file.c	\
	pi-header.c	\
	serial.c	\
	slp.c		\
	sys.c		\
	socket.c	\
	syspkt.c	\
	todo.c		\
	util.c		\
	utils.c		\
	veo.c		\
	versamail.c

libpisock_la_LIBADD = \
	$(ICONV_LIBS) @usb_libs@

libpisock_la_LDFLAGS = \
	-export-dynamic -version-info $(PISOCK_CURRENT):$(PISOCK_REVISION):$(PISOCK_AGE)
libpisock_la_CFLAGS = $(PIC_LIBS)

EXTRA_DIST = $(serial_FILES) $(usb_FILES)
