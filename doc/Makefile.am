if BUILD_XML
SUBDIRS = xml man
else
SUBDIRS = man
endif

EXTRA_DIST = 		 	\
	CodingGuidelines 	\
	README.debugging 	\
	README.usb	 	\
	TODO		 	\
	syncabs.sgml	 	\
	60-libpisock.rules	\
	doxygen.cfg.in

udevdir = /etc/udev/rules.d
udev_DATA =		 \
	60-libpisock.rules

DIST_CLEAN_FILES=doxygen.cfg

doc: doxygen.cfg
	rm -rf html refman.pdf
	doxygen doxygen.cfg
