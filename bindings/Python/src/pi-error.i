// -*- C -*-
//
// pi-error.i
//
// Error handling. Every function that returns a PI_ERR takes a socket
// descriptor as first argument. It is therefore easy to just use
// the first arg to query the sd about the error.
//

// -------------------------------------
// Returned errors
// -------------------------------------
%typemap (python,out) PI_ERR {
	if ($1 < 0) {
		if ($1 == PI_ERR_DLP_PALMOS) {
			int palmerr = pi_palmos_error(arg1);
			if (palmerr == dlpErrNoError || palmerr == dlpErrNotFound) {
				Py_INCREF(Py_None);
				return Py_None;
			}
			if (palmerr > dlpErrNoError && palmerr <= dlpErrUnknown) {
				PyErr_SetObject(PIError,
					Py_BuildValue("(is)", $1, dlp_strerror($1)));
				return NULL;
			}
		}
		
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
		  PyErr_SetObject(PIError, Py_BuildValue("(is)", $1, "pisock error"));
		}	
		return NULL;
	}
	$result = Py_None;
	Py_INCREF(Py_None);
}

