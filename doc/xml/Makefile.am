SUBDIRS= .

all:
	$(XSLTPROC) $(XSLTPROC_FLAGS) -o $(top_srcdir)/doc/man/ \
	$(top_srcdir)/doc/xml/docbook.xsl \
	$(top_srcdir)/doc/xml/pilot-link.docbook

distclean-local:
	$(RM) -f xml/catalog.xml
	$(RM) -f xml/docbook.xsl
