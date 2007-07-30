if BUILD_XML
if ENABLE_XSLTPROC
SUBDIRS = xml man
endif
else
SUBDIRS = man
endif

EXTRA_DIST = 		 	\
	CodingGuidelines 	\
	README.debugging 	\
	README.libusb		\
	README.usb	 	\
	TODO		 	\
	60-libpisock.rules	\
	doxygen.cfg.in

udevdir = $(datadir)/pilot-link/udev
udev_DATA = 60-libpisock.rules

DIST_CLEAN_FILES=doxygen.cfg

doc: doxygen.cfg
	rm -rf html refman.pdf
	doxygen doxygen.cfg
