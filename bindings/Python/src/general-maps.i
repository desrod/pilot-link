// -*- C -*-

/* It would be nice to use python time types ? NCP */
%apply long { time_t }


%typemap (python,in,numinputs=0) (pi_buffer_t *OUTBUF) {
  $1 = pi_buffer_new(0xFFFF);
}

%typemap (python,in,numinputs=0) (size_t *OUTBUFLEN) (size_t outbuflen) {
  outbuflen = 0xFFFF;
  $1 = &outbuflen;
}

%typemap (python,argout) (pi_buffer_t *OUTBUF) {
  PyObject *o1;
  if ($1) {
    o1 = Py_BuildValue("s#", $1->data, $1->used);
    pi_buffer_free($1);
    $result = t_output_helper($result, o1);
  }
}


%apply unsigned long { recordid_t };



// a generic 4-character string type, for use as a type or creator ID
%typemap (python,in) unsigned long STR4 {
    if (!($input) || ($input == Py_None)) {
	$1 = 0;
    } else {
	if (!PyString_Check($input) || (PyString_Size($input) != 4)) {
	    PyErr_SetString(PyExc_ValueError, "argument must be a 4-character string");
	    return 0;
	}
	$1 = makelong(PyString_AsString($input));
    }
}

%typemap (python,in) long STR4 {
    if (!($input) || ($input == Py_None)) {
	$1 = 0;
    } else {
	if (!PyString_Check($input) || (PyString_Size($input) != 4)) {
	    PyErr_SetString(PyExc_ValueError, "argument must be a 4-character string");
	    return 0;
	}
	$1 = makelong(PyString_AsString($input));
    }
}

%typemap (python,argout) unsigned long *OUTSTR4 {
    PyObject *o;
    if ($1) {
	o = PyString_FromStringAndSize(printlong(*$1), 4);
	$result = t_output_helper($result, o);
    }
}

%typemap (python,in,numinputs=0) unsigned long *OUTSTR4 (unsigned long temp) {
    $1 = &temp;
}


// for functions that return 0 on success or something else on error
%typemap (python,out) PIERROR {
    int *res_pointer, res;
    res_pointer = (int *) $1;
    res = *res_pointer;
    if (res != 0) {
	PyErr_SetObject(PIError, Py_BuildValue("(is)", res,
					     "pisock error"));
	return NULL;
    }
    $result = Py_None;
    Py_INCREF(Py_None);
}
