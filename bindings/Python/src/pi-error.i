// -*- C -*-
//
// pi-error.i
//
// Error handling. Every function that returns a PI_ERR takes a socket
// descriptor as first argument. It is therefore easy to just use
// the first arg to query the sd about the error.
//

%{
/* pythonWrapper_handlePiErr
 * called by each function that handles a PI_ERR return code
 */
static void* pythonWrapper_handlePiErr(int sd, int err)
{
	if (err == PI_ERR_DLP_PALMOS) {
		int palmerr = pi_palmos_error(sd);
		if (palmerr == dlpErrNoError || palmerr == dlpErrNotFound) {
			Py_INCREF(Py_None);
			return Py_None;
		}
		if (palmerr > dlpErrNoError && palmerr <= dlpErrUnknown) {
			PyErr_SetObject(PIError,
				Py_BuildValue("(is)", palmerr, dlp_strerror(palmerr)));
			return NULL;
		}
	}

	if (IS_PROT_ERR(err)) {
	  PyErr_SetObject(PIError, Py_BuildValue("(is)", err, "protocol error"));
	} else if (IS_SOCK_ERR(err)) {
	  PyErr_SetObject(PIError, Py_BuildValue("(is)", err, "socket error"));
	} else if (IS_DLP_ERR(err)) {
	  PyErr_SetObject(PIError, Py_BuildValue("(is)", err, "dlp error"));
	} else if (IS_FILE_ERR(err)) {
	  PyErr_SetObject(PIError, Py_BuildValue("(is)", err, "file error"));
	} else if (IS_GENERIC_ERR(err)) {
	  PyErr_SetObject(PIError, Py_BuildValue("(is)", err, "generic error"));
	} else {
	  PyErr_SetObject(PIError, Py_BuildValue("(is)", err, "pisock error"));
	}	
	return NULL;
}
%}


// -------------------------------------
// Returned errors: we pass them to our
// static function for handling (reduces
// the total wrapper code size)
// -------------------------------------
%typemap (python,out) PI_ERR {
	if ($1 < 0) {
		return pythonWrapper_handlePiErr(arg1, $1);
	}
	$result = Py_None;
	Py_INCREF(Py_None);
}
