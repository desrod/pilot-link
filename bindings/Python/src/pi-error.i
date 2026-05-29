/*
 * pi-error.i
 *
 * Error handling. Every function that returns a PI_ERR takes a socket
 * descriptor as first argument. It is therefore easy to just use
 * the first arg to query the sd about the error.
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
 * $Id$
 */

%{
static int pythonWrapper_handlePiErr(int sd, int err)
{
    /* This function is called by each function
     * which receives a PI_ERR return code
     */
	if (err == PI_ERR_DLP_PALMOS) {
		int palmerr = pi_palmos_error(sd);
		if (palmerr == dlpErrNoError)
			return 0;
		if (palmerr > dlpErrNoError && palmerr <= dlpErrUnknown) {
			PyErr_SetObject(PIError,
				Py_BuildValue("(is)", palmerr, dlp_strerror(palmerr)));
			return err;
		}
	}

	if (IS_PROT_ERR(err))
		PyErr_SetObject(PIError, Py_BuildValue("(is)", err, "protocol error"));
	else if (IS_SOCK_ERR(err))
		PyErr_SetObject(PIError, Py_BuildValue("(is)", err, "socket error"));
	else if (IS_DLP_ERR(err))
		PyErr_SetObject(PIError, Py_BuildValue("(is)", err, "DLP error"));
	else if (IS_FILE_ERR(err))
		PyErr_SetObject(PIError, Py_BuildValue("(is)", err, "file error"));
	else if (IS_GENERIC_ERR(err))
		PyErr_SetObject(PIError, Py_BuildValue("(is)", err, "generic error"));
	else
		PyErr_SetObject(PIError, Py_BuildValue("(is)", err, "pisock error"));

	return err;
}
%}


// -----------------------------------------------------------------
// Returned errors: we pass them to our static function for handling
// (reduces the total wrapper code size)
// -----------------------------------------------------------------
%typemap (out) PI_ERR %{
	if ($1 < 0 && pythonWrapper_handlePiErr(arg1, $1) != 0)
        SWIG_fail;
	$result = Py_None;
	Py_INCREF(Py_None);
%}
