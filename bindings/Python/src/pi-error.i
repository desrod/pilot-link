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
		PyErr_SetObject(PIError, Py_BuildValue("(is)", $1, "");
	}
    $result = Py_None;
    Py_INCREF(Py_None);
}

