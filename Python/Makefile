# Generated automatically from Makefile.in by configure.
# Generic Makefile for dynamically linked extension modules.
#
# Jim Fulton, Digital Creations, jim@digicool.com


# Uncomment and modify these lines if you want to fix the location of
# the PYTHON installation and the python version.  Otherwise, set the
# environment variables before using this Makefile.

PYTHON_PREFIX = /usr/local
PYTHON_EXEC_PREFIX = /usr/local
PYTHONINCDIR = /usr/local/include/python1.4
PYTHONLIBDIR = /usr/local/lib/python1.4
PYTHONVERSION = 1.4

prefix = /usr/pilot
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin
libdir = ${exec_prefix}/lib

# The following lines should be left as is:
VERSION=	$(PYTHONVERSION)
pyinstalldir=	$(PYTHON_PREFIX)
installdir=	$(exec_prefix)
exec_installdir=$(PYTHON_EXEC_PREFIX)
INCLUDEPY=	$(PYTHONINCDIR)
LIBP=		$(PYTHONLIBDIR)
LIBPL=		$(LIBP)/config
PYMAKE=		$(MAKE) -f $(LIBPL)/Makefile

# LIBSO is the location of platform-dependent dynamically linked 
# extension libraries.  This can be handy when you need to build 
# shared libraries that are not extensions but want to store them
# with other extensions and need to know where they are.
# Leave this line as it is.
LIBSO=		`$(PYMAKE) -s echodestshared`

# Put your module name here:
MODULE=_pdapilot

# Put the object files for your module here:
OBS=$(MODULE).o

# Put extra linker options, such as libraries here:
EXTRALD= -L$(libdir) -lpisock

# Put Extra compiler options, such as extra -I options, here
CFLAGS=-O -I../include

# If you have any Python modules, include them here, so that they
# can get installed.
PYMODULES=pdapilot

build:
	if [ "$(MODULE)" != your-module ]; then \
	  $(PYMAKE) INCLDIR=$(INCLUDEPY) CONFIGINCLDIR=$(LIBPL) \
		ASHAREDMODULE=$(MODULE) \
		'ASHAREDMODULESOBS=$(OBS)' \
		'ASHAREDMODULESEXTRA=$(EXTRALD)' \
		'OPT=$(CFLAGS)' \
		asharedmodule; \
	fi

install: installso installpyc

uninstall: uninstallso uninstallpyc

installso: build
	if [ "$(MODULE)" != your-module ]; then \
	  $(PYMAKE) exec_prefix=$(installdir) \
		ASHAREDMODULE=$(MODULE) asharedinstall; \
	fi

installpyc:
	for m in $(PYMODULES) the-end; do \
	  if [ "$$m" != the-end ]; then \
	    python -c "import $$m"; \
	    cp $$m.py $(installdir)/lib/python$(VERSION)/; \
	    cp $$m.pyc $(installdir)/lib/python$(VERSION)/; \
	  fi; \
	done

uninstallpyc:
	for m in $(PYMODULES) the-end; do \
	  if [ "$$m" != the-end ]; then \
	    rm -f $(installdir)/lib/python$(VERSION)/$$m.py; \
	    rm -f $(installdir)/lib/python$(VERSION)/$$m.pyc; \
	  fi; \
	done


uninstallso:
	if [ "$(MODULE)" != your-module ]; then \
		rm -f $(LIBP)/sharedmodules/$(MODULE).so;	\
	fi

test:
	python test.py

clean::
	-rm -f *.o *.so *.pyc *~ *# so_locations

distclean: clean
	rm Makefile
