
from distutils.core import setup, Extension

setup(name = "python-libpisock",
      version = "0.10.99",
      description = "Python binding for the pisock library.",
      author = "Rob Tillotson",
      author_email = "rob@pyrite.org",
      url = "http://www.pyrite.org/",

      ext_modules = [Extension("_pisock",["src/pisock_wrap.c"],
                               include_dirs=['../../include'],
                               library_dirs=['../../libpisock/.libs'],
                               libraries=['pisock'],
                               )
                     ],
      py_modules = ["pisock"],
      )
