python-libpisock -- Python binding for the pisock library
by Rob Tillotson <rob@pyrite.org>

This is a partial binding to allow the library component of pilot-link
(libpisock) to be used from Python.  It is, at present, simply a standalone
version of the glue module that was part of Pyrite; it will be extended in
the future to cover more of the pisock library and perhaps to have some
higher-level components like shadow classes for sockets and databases.

My intent is for this binding to eventually become part of pilot-link,
replacing the old Python binding which has not been updated since the days
of Python 1.4.  This binding differs from that one primarily in that there
is no attempt to do any fancy object-oriented stuff in the C module, nor
will the C module ever depend on the structure of the Python code that sits
atop it.  Instead, this binding is a fairly straightforward attempt to
expose the libpisock API to Python with minimal changes in data types,
function parameters, etc.

At present, this package only implements the dlp_* functions and a few of
the socket-related functions -- enough to open a connection and communicate
with the palmtop.  The most significant thing missing is prc/pdb file
access, which will be provided later by binding the pi-file functions as
well as a pure-Python prc/pdb access library. Also, if a future version of
pilot-link embeds conduit functionality into the library, this package will
be extended to support conduits written in Python.

The C code for the interface is generated using SWIG, but SWIG is not
required for building/installing it.  (To regenerate the wrapper from the
SWIG source, use "swig -python -dnone pisock.i".)

There is currently no documentation; that should be fixed soon.

* Installation:

This package uses the standard Python "distutils" for installation. If you
are running a version of Python >= 2.0 you have these already; for Python
1.5.2 you can download a distutils package from python.org. If you want to
build it for a Python version older than 1.5.2, you are on your own; it
probably won't work anyway.

To compile the package, run the command

    python setup.py build

You may safely ignore any warnings from the compiler.

To install the package, run the command

    python setup.py install

It will place a module called 'pisock.so' in Python's site-packages
directory.  To use the module, simply 'import pisock'.

Note: If you have more than one version of Python installed, replace
'python' in the above commands with the name of the interpreter you want to
use for building and installation.  (For example, 'python2' for Python 2.x
on some Linux distributions.) If you want to build/install for multiple
versions of Python, just repeat the above steps for each one.
