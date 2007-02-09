if WITH_JAVA
JAVA_DIR=Java
else
JAVA_DIR=
endif

if WITH_TCL
TCL_DIR=Tcl
else
TCL_DIR=
endif

SUBDIRS = $(JAVA_DIR) $(TCL_DIR)

#
# Python Building
#
python-build: Python/setup.py
	cd $(srcdir)/Python && $(PYTHON) setup.py build

python-install: python-build
	cd $(srcdir)/Python && $(PYTHON) setup.py install --root=$(DESTDIR) --prefix=$(prefix)

python-clean: Python/setup.py
	-cd $(srcdir)/Python && $(PYTHON) setup.py clean 
	-cd $(srcdir)/Python && rm -rf build

if WITH_PYTHON
PYTHON_ALL = python-build
PYTHON_INSTALL = python-install
PYTHON_CLEAN = python-clean
else
PYTHON_ALL = 
PYTHON_INSTALL = 
PYTHON_MCLEAN = 
PYTHON_CLEAN = 
endif

#
# Perl Building
#

Perl/Makefile: Perl/Makefile.PL
	cd Perl && $(PERL) Makefile.PL

perl-build: Perl/Makefile
	cd Perl && $(MAKE) PREFIX=$(DESTDIR)$(prefix)

perl-install:
	cd Perl && $(MAKE) install

perl-uninstall: Perl/Makefile
	cd Perl && $(MAKE) uninstall

perl-clean: Perl/Makefile
	cd Perl && $(MAKE) realclean && rm Makefile.PL

if WITH_PERL
PERL_ALL = perl-build
PERL_INSTALL = perl-install
PERL_CLEAN = perl-clean
else
PERL_ALL = 
PERL_INSTALL = 
PERL_MCLEAN = 
PERL_CLEAN = 
endif

all-local: $(PERL_ALL) $(PYTHON_ALL)

install-exec-local: $(PYTHON_INSTALL) $(PERL_INSTALL)

uninstall-local: $(PYTHON_INSTALL) $(PERL_UNINSTALL)

clean-local: $(PERL_CLEAN) $(PYTHON_CLEAN)

distclean-local: $(PERL_CLEAN) $(PYTHON_CLEAN)

maintainer-clean-local: $(PERL_CLEAN) $(PYTHON_CLEAN)

dist-hook:
	rm -rf `find $(distdir) -name CVS`

EXTRA_DIST = Python Java Perl
