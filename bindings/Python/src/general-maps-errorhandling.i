// -*- C -*-

%{
typedef int PIERROR;
%}

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
