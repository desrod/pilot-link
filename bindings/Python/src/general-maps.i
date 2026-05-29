/*
 * general-maps.i
 * Map types used throughout libpisock
 *
 * Copyright (c) 2005, Florent Pillet.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

/* It would be nice to use python time types ? NCP */
%apply long { time_t };
%apply unsigned long { recordid_t };

// ------------------------------------------------------
// Proper pi_buffer_t management, specifically designed
// as to not leak in case of early termination (hence the
// freearg part)
// ------------------------------------------------------
%typemap (in,numinputs=0) (pi_buffer_t *) {
	$1 = pi_buffer_new(0xFFFF);
}

%typemap (freearg) (pi_buffer_t *) {
	if ($1) {
		pi_buffer_free($1);
	}
}

%typemap (argout) (pi_buffer_t *) {
	if ($1) {
		PyObject *o1 = Py_BuildValue("s#", $1->data, $1->used);
		$result = SWIG_Python_AppendOutput($result, o1, 0);
	}
}

%typemap (in,numinputs=0) (size_t *OUTBUFLEN) (size_t outbuflen) {
  outbuflen = 0xFFFF;
  $1 = &outbuflen;
}

// -------------------------------------
// struct DBInfo
// -------------------------------------
%typemap (in,numinputs=0) struct DBInfo *OUTPUT (struct DBInfo temp) %{
    $1 = &temp;
%}

%typemap (argout) struct DBInfo *OUTPUT %{
    if ($1) $result = SWIG_Python_AppendOutput($result, PyObjectFromDBInfo($1), 0);
%}

// ------------------------------------------------------------------
// Type/creator strings
// a generic 4-character string type, for use as a type or creator ID
// ------------------------------------------------------------------
%typemap (in) unsigned long STR4 {
	if (!($input) || ($input == Py_None)) {
		$1 = 0;
	} else {
		if (!PyBytes_Check($input) || (PyBytes_Size($input) != 4)) {
			PyErr_SetString(PyExc_ValueError, "argument must be a 4-character string");
			return 0;
		}
		$1 = makelong(PyBytes_AsString($input));
	}
}

%typemap (in) long STR4 {
	if (!($input) || ($input == Py_None)) {
		$1 = 0;
	} else {
		if (!PyBytes_Check($input) || (PyBytes_Size($input) != 4)) {
			PyErr_SetString(PyExc_ValueError, "argument must be a 4-character string");
			return 0;
		}
		$1 = makelong(PyBytes_AsString($input));
	}
}

%typemap (argout) unsigned long *OUTSTR4 {
	if ($1) {
		PyObject *o = PyBytes_FromStringAndSize(printlong(*$1), 4);
		$result = SWIG_Python_AppendOutput($result, o, 0);
	}
}

%typemap (in,numinputs=0) unsigned long *OUTSTR4 (unsigned long temp) {
	$1 = &temp;
}

