// -*-C-*-
//
// $Id$
//
// Copyright 1999-2001 Rob Tillotson <rob@pyrite.org>
// All Rights Reserved
//
// Permission to use, copy, modify, and distribute this software and
// its documentation for any purpose and without fee or royalty is
// hereby granted, provided that the above copyright notice appear in
// all copies and that both the copyright notice and this permission
// notice appear in supporting documentation or portions thereof,
// including modifications, that you you make.
//
// THE AUTHOR ROB TILLOTSON DISCLAIMS ALL WARRANTIES WITH REGARD TO
// THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
// RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
// CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
// CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE!
//

// This is an attempt at using SWIG to generate wrappers for the
// pilot-link library.  As one look at this source should tell you, it
// turned out to be a non-trivial amount of work (but less than doing
// it by hand).  The semantics of the resulting module are not all
// that different from the original Python interface to pilot-link,
// but function names and arguments are closer to those in the raw
// pilot-link library.  In particular, no attempt is made to create
// Python types for connections, databases, etc.; if you want object
// structure, shadow classes should be easy to create.
//
// All of the dlp_* functions will throw an exception when the library
// returns a negative status; the value of this exception will be a
// tuple of the numeric error code and a message.


%module pisock
%{
#include <time.h>
#include "pi-args.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-file.h"
#include "pi-util.h"
#include "pi-header.h"    

extern char *printlong(unsigned long);
extern unsigned long makelong(char *c);

#define DGETLONG(src,key,default) (PyDict_GetItemString(src,key) ? \
				   PyInt_AsLong(PyDict_GetItemString(src,key)) : default)

#define DGETSTR(src,key,default) (PyDict_GetItemString(src,key) ? \
				  PyString_AsString(PyDict_GetItemString(src,key)) : default)

typedef int PIERROR;

static PyObject *PIError;
%}

%init %{
  PIError = PyErr_NewException("pisock.error", NULL, NULL);
  Py_INCREF(PIError);
  PyDict_SetItemString(d, "error", PIError);
%}

%pythoncode %{ 
error = _pisock.error 
%} 

%include typemaps.i

#define PI_DEPRECATED

%include general-maps.i
%include pi-socket-maps.i
%include pi-dlp-maps.i

%include ../../../include/pi-args.h
%include ../../../include/pi-header.h
%include ../../../include/pi-socket.h
%include ../../../include/pi-dlp.h
%include ../../../include/pi-socket.h
%include ../../../include/pi-file.h

