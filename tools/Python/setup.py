
from distutils.core import setup, Extension

setup(name = "python-libpisock",
      version = "0.9.5.1",
      description = "Python binding for the pisock library.",
      author = "Rob Tillotson",
      author_email = "rob@pyrite.org",
      url = "http://www.pyrite.org/",

      ext_modules = [Extension("_pisock",["src/pisock_wrap.c"],
                               include_dirs=['../../include'], # for debian
                               library_dirs=['../../libsock/.libs'], # for debian
                               libraries=['pisock'],
                               )
                     ],
      py_modules = ["pisock"],
      )
