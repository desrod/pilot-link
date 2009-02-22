INCLUDES = 			\
	-I../include		\
	-I$(top_srcdir)/include

bluetooth_FILES =	\
	bluetooth.c

serial_FILES = 		\
	unixserial.c
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

if WITH_BLUEZ
bluetooth_SOURCE = bluetooth.c
endif

lib_LTLIBRARIES = libpisock.la

# All new sources MUST be added here, or anything 
# building against libpisock will fail!
libpisock_la_SOURCES =  \
	$(bluetooth_SOURCE)	\
	$(serial_SOURCE)	\
	$(usb_SOURCE)		\
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
	location.c	\
	blob.c	\
	calendar.c	\
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
	threadsafe.c	\
	todo.c		\
	utils.c		\
	veo.c		\
	versamail.c

# Including PTHREAD_CFLAGS here is a dirty ugly kluge.  It works.
libpisock_la_LIBADD = \
	@usb_libs@ @PTHREAD_LIBS@ @PTHREAD_CFLAGS@ @BLUEZ_LIBS@

libpisock_la_LDFLAGS = \
	-export-dynamic -version-info $(PISOCK_CURRENT):$(PISOCK_REVISION):$(PISOCK_AGE)
libpisock_la_CFLAGS = $(PIC_LIBS) @PTHREAD_CFLAGS@ @BLUEZ_CFLAGS@

EXTRA_DIST = $(bluetooth_FILES) $(serial_FILES) $(usb_FILES)
