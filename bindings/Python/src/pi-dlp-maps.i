/*
 * pi-dlp-maps.i
 *
 * Maps for the DLP function arguments, as well as some custom implementations
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
 */

// TODO: convert PI_CONST char *dbname parameters using the ConvertToEncoding() function
// TODO: map output parameters for dlp_ExpCardInfo()
// TODO: map output parameters for dlp_ExpSlotMediaType()
// TODO: map output parameters for VFS functions
// TODO: map 'unsigned long *restype' output parameter

// -----------------------------------------------
// a char value that allows None for a null value.
// -----------------------------------------------
%typemap (python,in) char *ALLOWNULL {
    if (!($input) || ($input == Py_None))
		$1 = NULL;
    else
		$1 = PyString_AsString($input);
}

// -------------------------------------
//  type/creator pair
// -------------------------------------
%typemap (python,in) unsigned long creator, unsigned long type {
    if (PyString_Check($input))
        $1 = makelong(PyString_AS_STRING($input));
    else if (PyInt_Check($input))
        $1 = PyInt_AsLong($input);
    else {
        PyErr_SetString(PyExc_TypeError,"You must specify a type/creator");
        SWIG_fail;
    }
}

// ----------------------------------------------------------
// output parameters. Output parameters should be ommitted
// when writing Python code, the result is either a single
// value or a list (depending on whether there is one or more
// output parameters) containing the output values
// ----------------------------------------------------------
%apply int *OUTPUT { int *dbhandle };
%apply int *OUTPUT { int *numrecs };
%apply int *OUTPUT { int *recattrs };
%apply int *OUTPUT { int *category };
%apply int *OUTPUT { int *cardno };
%apply int *OUTPUT { int *recindex };
%apply int *OUTPUT { int *resindex };
%apply int *OUTPUT { int *resid };

%apply long *OUTPUT { long *usedbytes };
%apply long *OUTPUT { long *totalbytes };

%apply unsigned long *OUTPUT { unsigned long *feature };
%apply unsigned long *OUTPUT { recordid_t *recuid };
%apply unsigned long *OUTPUT { recordid_t *newrecuid };
%apply unsigned long *OUTPUT { unsigned long *localid };
%apply unsigned long *OUTPUT { unsigned long *retcode };

// -------------------------------------
// time_t *time
// -------------------------------------
%rename(dlp_GetSysDateTime_) dlp_GetSysDateTime;

%typemap (python,in,numinputs=0) time_t *time (time_t time) {
    $1 = (time_t *)&time;
}
%typemap (python,argout) time_t *time (time_t time) {
    if ($1) $result = PyInt_FromLong((unsigned long ) $1);
}

// -------------------------------------
// struct PilotUser
// -------------------------------------
%typemap (python,in) const struct PilotUser* (struct PilotUser temp) %{
	if (!PyObjectToPilotUser($input, &temp))
	    SWIG_fail;
	$1 = &temp;
%}

%typemap (python,in,numinputs=0) struct PilotUser* (struct PilotUser temp) %{
    $1 = &temp;
%}

%typemap (python,argout) struct PilotUser* %{
    if ($1) $result = t_output_helper($result, PyObjectFromPilotUser($1));
%}

// -------------------------------------
// struct SysInfo
// -------------------------------------
%typemap (in,numinputs=0) struct SysInfo* (struct SysInfo temp) %{
    $1 = &temp;
%}

%typemap (python,argout) struct SysInfo * %{
    if ($1) $result = t_output_helper($result, PyObjectFromSysInfo($1));
%}

// -------------------------------------
// struct CardInfo
// -------------------------------------
%typemap (python,argout) (struct CardInfo *cardinfo) %{
    if ($1) $result = t_output_helper($result, PyObjectFromCardInfo($1));
%}

%typemap (python,in,numinputs=0) struct CardInfo *cardinfo (struct CardInfo temp) %{
    $1 = &temp;
%}

// -------------------------------------
// struct NetSyncInfo
// -------------------------------------
%typemap (python,in,numinputs=0) struct NetSyncInfo *OUTPUT (struct NetSyncInfo temp) {
    $1 = &temp;
}

%typemap (python,argout) struct NetSyncInfo *OUTPUT %{
    if ($1) $result = t_output_helper($result, PyObjectFromNetSyncInfo($1));
%}

%typemap (python,in) struct NetSyncInfo *INPUT (struct NetSyncInfo temp) %{
	PyObjectToNetSyncInfo($input, &temp);
    $1 = &temp;
%}

// -------------------------------------
// Passing data as parameter
// -------------------------------------
%typemap (python,in) (const void *databuf, size_t datasize) %{
	$1 = (void *)PyString_AsString($input);
	$2 = PyString_Size($input);
%}

%typemap (python,in) (size_t datasize, const void *databuf) %{
    $1 = PyString_Size($input);
    $2 = (void *)PyString_AsString($input);
%}

// -------------------------------------
// Used by dlp_ReadAppPreference
// -------------------------------------
%typemap (python,argout) (void *databuf, size_t *datasize, int *version) %{
	if ($1) $result = t_output_helper($result, Py_BuildValue("is#", $3, $1, $2));
%}

// ---------------------------------------------------------
// Finding databases: proper wrapping of dlp_Find* functions
// so that they do not need to be passed a pointer
// ---------------------------------------------------------
%typemap (python,in,numinputs=0)  (struct DBInfo *dbInfo, struct DBSizeInfo *dbSize)
		(struct DBInfo temp1, struct DBSizeInfo temp2) %{
	$1 = &temp1;
	$2 = &temp2;
%}

%typemap (python,argout) (struct DBInfo *dbInfo, struct DBSizeInfo *dbSize) %{
	if ($1) $result = t_output_helper($result, PyObjectFromDBInfo($1));
	if ($2) $result = t_output_helper($result, PyObjectFromDBSizeInfo($2));
%}

// -------------------------------------
// Processing of a buffer containing a
// database list
// -------------------------------------
// we provide a python implementation for dlp_ReadDBList (see ../pisockextras.py)
%rename(dlp_ReadDBList_) dlp_ReadDBList;

%typemap (python,argout) (pi_buffer_t *dblist) %{
	if ($1) {
		int j;
		struct DBInfo info;
		$result = PyList_New(($1->used / sizeof(struct DBInfo)));
		for (j=0; j < ($1->used / sizeof(struct DBInfo)); j++) {
			memcpy(&info, $1->data + j * sizeof(struct DBInfo), sizeof(struct DBInfo));
			PyObject *o = PyObjectFromDBInfo(&info);
			Py_INCREF(o);
			PyList_SET_ITEM($result, j, o);
		}
	}
%}

// -----------------------------------------
// Mapping for dlp_ExpSlotEnumerate
// -----------------------------------------
%typemap (python,in,numinputs=0) (int *numslots, int *slotrefs)
		(int numSlots, int slotRefs[16]) %{
	numSlots = sizeof(slotRefs) / sizeof(slotRefs[0]);
	$1 = &numSlots;
	$2 = &slotRefs[0];
%}

%typemap (python,argout) (int *numslots, int *slotrefs) %{
	if ($1 && $2) {
		int slotIndex;
		for (slotIndex=0; slotIndex < *$1; slotIndex++)
			t_output_helper($result, PyInt_FromLong($2[slotIndex]));
	}
%}

// -----------------------------------------
// Custom wrappers for some of the functions
// -----------------------------------------
%native(dlp_ReadRecordIDList) PyObject *_wrap_dlp_ReadRecordIDList(PyObject *, PyObject *);

%{
static PyObject *_wrap_dlp_ReadRecordIDList (PyObject *self, PyObject *args) {
	int sd, dbhandle, sort, start, max;
	int ret;
	recordid_t *buf;
	int count, i;
	PyObject *list;
	
	buf = (recordid_t *)PyMem_Malloc(0xFFFF);    
	
	if (!PyArg_ParseTuple(args, "iiiii", &sd, &dbhandle, &sort, &start, &max))
		return NULL;
	
	/* this is a rather simplistic wrapper.  if max is too big, we just
		* refuse to do it; we don't loop, figuring that that is the job of
		* the python wrapper.
		*/
	if (max > (0xFFFF/sizeof(recordid_t))) {
		PyErr_SetString(PyExc_ValueError, "can only return about 64k worth of ids at once");
		return NULL;
	}
	
	{
		PyThreadState *save = PyEval_SaveThread();
		ret = dlp_ReadRecordIDList(sd, dbhandle, sort, start, max, buf, &count);
		PyEval_RestoreThread(save);
	}
	
	if (ret < 0) {
		PyErr_SetObject(PIError, Py_BuildValue("(is)", ret, dlp_strerror(ret)));
		PyMem_Free(buf);
		return NULL;
	}

	list = PyList_New(0);
	for (i=0; i<count; i++)
		PyList_Append(list, PyInt_FromLong((long)buf[i]));

	PyMem_Free(buf);
	return list;
}
%}
