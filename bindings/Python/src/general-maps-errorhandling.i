// -*- C -*-

%{
typedef int PIERROR;
%}

// for functions that return 0 on success or something else on error
%typemap (python,out) PIERROR {
    if ($1 != 0) {
      if (IS_PROT_ERR($1)) {
	PyErr_SetObject(PIError, Py_BuildValue("(is)", $1, "protocol error"));
      } else if (IS_SOCK_ERR($1)) {
	PyErr_SetObject(PIError, Py_BuildValue("(is)", $1, "socket error"));
      } else if (IS_DLP_ERR($1)) {
	PyErr_SetObject(PIError, Py_BuildValue("(is)", $1, "dlp error"));
      } else if (IS_FILE_ERR($1)) {
	PyErr_SetObject(PIError, Py_BuildValue("(is)", $1, "file error"));
      } else if (IS_GENERIC_ERR($1)) {
	PyErr_SetObject(PIError, Py_BuildValue("(is)", $1, "generic error"));
      } else {
	PyErr_SetObject(PIError, Py_BuildValue("(is)", $1,"pisock error"));
      }	
      return NULL;
    }
    $result = Py_None;
    Py_INCREF(Py_None);
}

