/*
 * pi-file-maps.i
 *
 * Provide our own wrappers for pi_file_install and pi_file_retrieve
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id$
 */

// TODO: handle callback functions (ignored for now)

%native(pi_file_install) PyObject *_wrap_pi_file_install(PyObject *, PyObject *);
%native(pi_file_retrieve) PyObject *_wrap_pi_file_retrieve(PyObject *, PyObject *);

%wrapper %{
/*
 * Python syntax: pi_file_install(sd, cardno, filename, callback)
 */
static PyObject *_wrap_pi_file_install (PyObject *self, PyObject *args)
{
	PyObject *obj1 = NULL;
	PyObject *obj2 = NULL;
	PyObject *obj3 = NULL;
	PyObject *cback = NULL;
	int sd, cardno, result;
	char *path = NULL;
	pi_file_t *pf = NULL;

	if (!PyArg_ParseTuple(args,(char *)"OOOO:pi_file_install",&obj1, &obj2, &obj3, &cback))
		return NULL;

	sd = (int)SWIG_As_int(obj1);
	cardno = (int)SWIG_As_int(obj2);
	if (!SWIG_AsCharPtr(obj3, (char**)&path)) {
		SWIG_arg_fail(3);
		return NULL;
	}

	pf = pi_file_open(path);
	if (pf == NULL) {
		PyErr_SetObject(PIError, Py_BuildValue("(is)", PI_ERR_FILE_INVALID, "invalid file"));
		return NULL;
	}

	{
		PyThreadState *save = PyEval_SaveThread();
		result = pi_file_install(pf, sd, cardno, NULL);
		PyEval_RestoreThread(save);
	}

	pi_file_close(pf);

	if (result < 0) {
		pythonWrapper_handlePiErr(sd, result);
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

/*
 * Python syntax: pi_file_retrieve(sd, cardno, dbname, storagepath, callback)
 */
static PyObject *_wrap_pi_file_retrieve (PyObject *self, PyObject *args)
{
	PyObject *obj1 = NULL;
	PyObject *obj2 = NULL;
	PyObject *obj3 = NULL;
	PyObject *obj4 = NULL;
	PyObject *cback = NULL;
	int sd, cardno, result;
	char *dbname = NULL;
	char *path = NULL;
	struct DBInfo dbi;
	pi_file_t *pf = NULL;
	PyThreadState *save;

	if (!PyArg_ParseTuple(args, (char *)"OOOOO:pi_file_retrieve",&obj1,&obj2,&obj3,&obj4,&cback))
		return NULL;

	sd = SWIG_As_int(obj1);
	cardno = SWIG_As_int(obj2);

	if (!SWIG_AsCharPtr(obj3, (char**)&dbname)) {
		SWIG_arg_fail(3);
		return NULL;
	}

	if (!SWIG_AsCharPtr(obj4, (char **)&path)) {
		SWIG_arg_fail(4);
		return NULL;
	}

	/* let other threads run */
 	save = PyEval_SaveThread();

 	memset(&dbi, 0, sizeof(dbi));
	result = dlp_FindDBByName(sd, cardno, dbname, NULL, NULL, &dbi, NULL);
	if (result < 0) {
		PyEval_RestoreThread(save);
		pythonWrapper_handlePiErr(sd, result);
		return NULL;
	}

	pf = pi_file_create(path, &dbi);
	if (pf == NULL) {
		PyEval_RestoreThread(save);
		PyErr_SetObject(PIError, Py_BuildValue("(is)", PI_ERR_FILE_INVALID, "invalid file"));
		return NULL;
	}

	result = pi_file_retrieve(pf, sd, cardno, NULL);
	if (result < 0) {
		PyEval_RestoreThread(save);
		pythonWrapper_handlePiErr(sd, result);
		return NULL;
	}

	result = pi_file_close(pf);
	PyEval_RestoreThread(save);
	if (result < 0) {
		pythonWrapper_handlePiErr(sd, result);
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

%}
