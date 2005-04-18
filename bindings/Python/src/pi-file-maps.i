// Provide our own wrappers for pi_file_install and pi_file_retrieve
// TODO: handle callback functions (ignored for now)

%native(pi_file_install) PyObject *_wrap_pi_file_install(PyObject *, PyObject *);
%native(pi_file_retrieve) PyObject *_wrap_pi_file_retrieve(PyObject *, PyObject *);

%{
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

	if (!PyArg_ParseTuple(args,(char *)"OOOO",&obj1, &obj2, &obj3, &cback))
		return NULL;

	sd = (int)(SWIG_As_int(obj1));
	cardno = (int)(SWIG_As_int(obj2));
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
        PyThreadState *__save = PyEval_SaveThread();
        result = pi_file_install(pf, sd, cardno, NULL);
        PyEval_RestoreThread(__save);
	}

	pi_file_close(pf);

    if (result < 0)
		return pythonWrapper_handlePiErr(sd, result);

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

	if (!PyArg_ParseTuple(args, (char *)"OOOOO",&obj1,&obj2,&obj3,&obj4,&cback))
		return NULL;
	sd = (int)(SWIG_As_int(obj1));
	cardno = (int)(SWIG_As_int(obj2));
    if (!SWIG_AsCharPtr(obj3, (char**)&dbname)) {
        SWIG_arg_fail(3);
		return NULL;
    }
	if (!SWIG_AsCharPtr(obj4, (char **)&path)) {
		SWIG_arg_fail(4);
		return NULL;
	}

	memset(&dbi, 0, sizeof(dbi));
	result = dlp_FindDBByName(sd, cardno, dbname, NULL, NULL, &dbi, NULL);
	if (result < 0)
		return pythonWrapper_handlePiErr(sd, result);

	pf = pi_file_create(path, &dbi);
	if (pf == NULL) {
		PyErr_SetObject(PIError, Py_BuildValue("(is)", PI_ERR_FILE_INVALID, "invalid file"));
		return NULL;
	}

	{
        PyThreadState *__save = PyEval_SaveThread();
        result = pi_file_retrieve(pf, sd, cardno, NULL);
        PyEval_RestoreThread(__save);
	}

    if (result < 0)
		return pythonWrapper_handlePiErr(sd, result);

	result = pi_file_close(pf);
	if (result < 0)
		return pythonWrapper_handlePiErr(sd, result);

    Py_INCREF(Py_None);
	return Py_None;
}

%}
