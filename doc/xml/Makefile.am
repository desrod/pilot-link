# Autoconf breaks distcheck when passing in these variables. To get it
# to "mostly" build, use the following:
#
# make distcheck DISTCHECK_CONFIGURE_FLAGS="XSLTPROC=xsltproc
# 
# You'll also need to add ../ to the docbook.xsl and pilot-link.docbook
# to get past a bug in autoconf when using the distcheck target here.
# Sigh.

SUBDIRS= .

EXTRA_DIST =			\
	docbook.xsl		\
	pilot-link.docbook

all:
	$(XSLTPROC) $(XSLTPROC_FLAGS) -o $(top_srcdir)/doc/man/ \
	$(top_srcdir)/doc/xml/docbook.xsl \
	$(top_srcdir)/doc/xml/pilot-link.docbook

distclean-local:
	$(RM) -f xml/catalog.xml
	$(RM) -f xml/docbook.xsl
