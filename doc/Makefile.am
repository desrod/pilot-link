if BUILD_XML
SUBDIRS = xml man
else
SUBDIRS = man
endif

EXTRA_DIST = 		 \
	CodingGuidelines \
	README.debugging \
	README.usb	 \
	TODO		 \
	syncabs.sgml  \
	doxygen.cfg.in

DIST_CLEAN_FILES=doxygen.cfg

doc: doxygen.cfg
	rm -rf html refman.pdf
	doxygen doxygen.cfg
