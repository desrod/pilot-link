if WITH_JAVA
JAVA_DIR=Java
else
JAVA_DIR=
endif

SUBDIRS = $(JAVA_DIR)

#
# Python Building
#
python-build: Python/setup.py
	cd $(srcdir)/Python && LD_LIBRARY_PATH=$(top_buildir)/libsock/.libs/ $(PYTHON) setup.py build

python-install: python-build
	cd $(srcdir)/Python && $(PYTHON) setup.py install

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
perl5-make: Perl5/Makefile.PL
	cd Perl5 && $(PERL5) Makefile.PL

perl5-build: perl5-make
	cd Perl5 && $(MAKE)

perl5-install: perl5-build
	cd Perl5 && $(MAKE) install

perl5-uninstall: perl5-make
	cd Perl5 && $(MAKE) uninstall

perl5-clean: perl5-make
	cd Perl5 && $(MAKE) realclean

if WITH_PERL5
PERL5_ALL = perl5-build
PERL5_INSTALL = perl5-install
PERL5_CLEAN = perl5-clean
else
PERL5_ALL = 
PERL5_INSTALL = 
PERL5_MCLEAN = 
PERL5_CLEAN = 
endif

all-local: $(PERL5_ALL) $(PYTHON_ALL)

install-exec-local: $(PYTHON_INSTALL) $(PERL5_INSTALL)

uninstall-local: $(PYTHON_INSTALL) $(PERL5_UNINSTALL)

clean-local: $(PERL5_CLEAN) $(PYTHON_CLEAN)

distclean-local: $(PERL5_CLEAN) $(PYTHON_CLEAN)

maintainer-clean-local: $(PERL5_CLEAN) $(PYTHON_CLEAN)

EXTRA_DIST = Python Java Perl5 Tcl
