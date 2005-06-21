# Only build the XML targets if we're using CVS
if CVS_BUILD
SUBDIRS = xml man
else 
SUBDIRS = man
endif

EXTRA_DIST = 		 \
	CodingGuidelines \
	README.debugging \
	README.freebsd	 \
	README.usb	 \
	TODO		 \
	syncabs.sgml

DIST_CLEAN_FILES=doxygen.cfg

doc: doxygen.cfg
	rm -rf html refman.pdf
	doxygen doxygen.cfg
