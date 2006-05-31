/*
 * pi-socket-maps.i
 *
 * Provide our own wrappers for pi_file_install and pi_file_retrieve
 *
 * Copyright (c) 2004, Rob Tillotson
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
 * $Id$
 */

//
// pi-sockaddr... the real structure might be defined in one of two
// different ways, but luckily SWIG doesnt really care.
//
%typemap (python,in) struct sockaddr *INPUT {
    static struct pi_sockaddr temp;
    char *dev;
    int len;

    if (!PyArg_ParseTuple($input, "is#", &temp.pi_family, &dev, &len)) 
		return NULL;
    if (len > 255) {
      // Should really raise an exception
      len = 255;
    }
    strncpy(temp.pi_device, dev, len);
    temp.pi_device[len] = 0;
    $1 = (struct sockaddr *)&temp;
}

%typemap (python, argout,fragment="t_output_helper") struct sockaddr *remote_addr {
    PyObject *o;

    if ($1) {
		o = Py_BuildValue("(is)", (int)((struct pi_sockaddr *)$1)->pi_family,
				  ((struct pi_sockaddr *)$1)->pi_device);
		$result = t_output_helper($result, o);
    }
}

%typemap (python,in,numinputs=0) struct sockaddr *remote_addr (struct pi_sockaddr temp) {
    $1 = (struct sockaddr *)&temp;
}

%typemap (python,in,numinputs=0) size_t *namelen (size_t temp) {
    $1 = (size_t *)&temp;
}

