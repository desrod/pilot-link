python-build: Python/setup.py
	cd $(srcdir)/Python && $(PYTHON) setup.py build

python-install: python-build
	cd $(srcdir)/Python && $(PYTHON) setup.py install

python-clean: Python/setup.py
	cd $(srcdir)/Python && $(PYTHON) setup.py clean && rm -rf build

if WITH_PYTHON
PYTHON_ALL = python-build
PYTHON_INSTALL = python-install
PYTHON_CLEAN = python-clean
else
PYTHON_ALL = 
PYTHON_INSTALL = 
PYTHON_CLEAN = 
endif

perl5-make: $(srcdir)/Perl5/Makefile.PL
	cd $(srcdir)/Perl5 && $(PERL5) Makefile.PL

perl5-build: perl5-make
	cd $(srcdir)/Perl5 && $(MAKE)

perl5-install: perl5-build
	cd $(srcdir)/Perl5 && $(MAKE) install

perl5-clean: perl5-make
	cd $(srcdir)/Perl5 && $(MAKE) realclean

if WITH_PERL5
PERL5_ALL = perl5-build
PERL5_INSTALL = perl5-install
PERL5_CLEAN = perl5-clean
else
PERL5_ALL = 
PERL5_INSTALL =
PERL5_CLEAN =
endif

if WITH_JAVA
JAVA_DIR = Java
else
JAVA_DIR =
endif

if WITH_TCL
TCL_DIR = Tcl
else
TCL_DIR =
endif

all-local: 

install-exec-local: $(PYTHON_INSTALL) $(PERL5_INSTALL)

clean-local: $(PERL5_CLEAN) $(PYTHON_CLEAN)

distclean-local: $(PERL5_CLEAN) $(PYTHON_CLEAN)

EXTRA_DIST = Python Java Perl5 Tcl
