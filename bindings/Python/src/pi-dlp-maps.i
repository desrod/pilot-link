// -*- C -*-

%{
typedef int DLPERROR;
typedef int DLPDBERROR;
%}

// a char value that allows None for a null value.
%typemap (python,in) char *ALLOWNULL {
    if (!($input) || ($input == Py_None)) {
	$1 = NULL;
    } else {
	$1 = PyString_AsString($input);
    }
}



// struct PilotUser
%typemap (python,in) struct PilotUser * {
    static struct PilotUser temp;
    int l;
    PyObject *foo;

    temp.userID = DGETLONG($input,"userID",0);
    temp.viewerID = DGETLONG($input,"viewerID",0);
    temp.lastSyncPC = DGETLONG($input,"lastSyncPC",0);
    temp.successfulSyncDate = DGETLONG($input,"successfulSyncDate",0);
    temp.lastSyncDate = DGETLONG($input,"lastSyncDate",0);
    strncpy(temp.username, DGETSTR($input,"name",""), 128);

    foo = PyDict_GetItemString($input,"password");
    if (PyString_Check(foo)) {
	l = PyString_Size(foo);
	temp.passwordLength = l;
	memcpy(temp.password, PyString_AsString(foo), l);
    }
    
    $1 = &temp;    
}

%typemap (python,argout) struct PilotUser *OUTPUT {
    PyObject *o;
    
    if ($1) {
	o = Py_BuildValue("{slslslslslssss#}",
			  "userID", $1->userID,
			  "viewerID", $1->viewerID,
			  "lastSyncPC", $1->lastSyncPC,
			  "successfulSyncDate", $1->successfulSyncDate,
			  "lastSyncDate", $1->lastSyncDate,
			  "name", $1->username,
			  "password", $1->password, $1->passwordLength);
        $result = t_output_helper($result, o);
    }
}

%typemap (python,in,numinputs=0) struct PilotUser *OUTPUT (struct PilotUser temp) {
    $1 = &temp;
}

// struct SysInfo
%typemap (python,argout) struct SysInfo *OUTPUT {
    PyObject *o;
    
    if ($1) {
	o = Py_BuildValue("{slslss#}",
			  "romVersion", $1->romVersion,
			  "locale", $1->locale,
			  "name", $1->prodID, $1->prodIDLength);
	$result = t_output_helper($result, o);
    }
}

%typemap (python,in,numinputs=0) struct SysInfo *OUTPUT (struct SysInfo temp) {
    $1 = &temp;
}

%typemap (python,in,numinputs=0) (pi_buffer_t *OUTDBInfoList) {
  $1 = pi_buffer_new(0xFFFF);
}

%typemap (python,argout) (pi_buffer_t *OUTDBInfoList) {
  PyObject *o;
  int j;
  struct DBInfo	info;
  
  if ($1) {
    $result = PyList_New(($1->used / sizeof(struct DBInfo)));
    for (j=0; j < ($1->used / sizeof(struct DBInfo)); j++) {
      memcpy(&info, $1->data + j * sizeof(struct DBInfo), sizeof(struct DBInfo));
      o = Py_BuildValue("{sisisisOsOsislslslslsisssisisisisisisisisisisi}",
			"more", info.more,
			"flags", info.flags,
			"miscFlags", info.miscFlags,
			"type", PyString_FromStringAndSize(printlong(info.type), 4),
			"creator", PyString_FromStringAndSize(printlong(info.creator), 4),
			"version", info.version,
			"modnum", info.modnum,
			"createDate", info.createDate,
			"modifyDate", info.modifyDate,
			"backupDate", info.backupDate,
			"index", info.index,
			"name", info.name,
			
			"flagResource", !!(info.flags & dlpDBFlagResource),
			"flagReadOnly", !!(info.flags & dlpDBFlagReadOnly),
			"flagAppInfoDirty", !!(info.flags & dlpDBFlagAppInfoDirty),
			"flagBackup", !!(info.flags & dlpDBFlagBackup),
			"flagClipping", !!(info.flags & dlpDBFlagClipping),
			"flagOpen", !!(info.flags & dlpDBFlagOpen),
			"flagNewer", !!(info.flags & dlpDBFlagNewer),
			"flagReset", !!(info.flags & dlpDBFlagReset),
			"flagCopyPrevention", !!(info.flags & dlpDBFlagCopyPrevention),
			"flagStream", !!(info.flags & dlpDBFlagStream),
			"flagExcludeFromSync", !!(info.miscFlags & dlpDBMiscFlagExcludeFromSync));
      Py_INCREF(o);
      PyList_SET_ITEM($result, j, o);
    }
  }
}


// struct DBInfo
%typemap (python,argout) struct DBInfo *OUTPUT {
    PyObject *o;

    if ($1) {
	o = Py_BuildValue("{sisisisOsOsislslslslsisssisisisisisisisisisisi}",
			  "more", $1->more,
			  "flags", $1->flags,
			  "miscFlags", $1->miscFlags,
			  "type", PyString_FromStringAndSize(printlong($1->type), 4),
			  "creator", PyString_FromStringAndSize(printlong($1->creator), 4),
			  "version", $1->version,
			  "modnum", $1->modnum,
			  "createDate", $1->createDate,
			  "modifyDate", $1->modifyDate,
			  "backupDate", $1->backupDate,
			  "index", $1->index,
			  "name", $1->name,

			  "flagResource", !!($1->flags & dlpDBFlagResource),
			  "flagReadOnly", !!($1->flags & dlpDBFlagReadOnly),
			  "flagAppInfoDirty", !!($1->flags & dlpDBFlagAppInfoDirty),
			  "flagBackup", !!($1->flags & dlpDBFlagBackup),
			  "flagClipping", !!($1->flags & dlpDBFlagClipping),
			  "flagOpen", !!($1->flags & dlpDBFlagOpen),
			  "flagNewer", !!($1->flags & dlpDBFlagNewer),
			  "flagReset", !!($1->flags & dlpDBFlagReset),
			  "flagCopyPrevention", !!($1->flags & dlpDBFlagCopyPrevention),
			  "flagStream", !!($1->flags & dlpDBFlagStream),
			  "flagExcludeFromSync", !!($1->miscFlags & dlpDBMiscFlagExcludeFromSync));
	$result = t_output_helper($result, o);
    }
}

%typemap (python,in,numinputs=0) struct DBInfo *OUTPUT (struct DBInfo temp) {
    $1 = &temp;
}

%typemap (python,in) struct DBInfo * {
    static struct DBInfo temp;

    temp.more = (int) DGETLONG($input, "more", 0);
    temp.type = makelong(DGETSTR($input, "type", "    "));
    temp.creator = makelong(DGETSTR($input, "creator", "    "));
    temp.version = DGETLONG($input, "version", 0);
    temp.modnum = DGETLONG($input, "modnum", 0);
    temp.createDate = DGETLONG($input, "createDate", 0);
    temp.modifyDate = DGETLONG($input, "modifyDate", 0);
    temp.backupDate = DGETLONG($input, "backupDate", 0);
    temp.index = DGETLONG($input, "index", 0);
    strncpy(temp.name, DGETSTR($input,"name",""), 34);
    temp.flags = 0;
    if (DGETLONG($input,"flagResource",0)) temp.flags |= dlpDBFlagResource;
    if (DGETLONG($input,"flagReadOnly",0)) temp.flags |= dlpDBFlagReadOnly;
    if (DGETLONG($input,"flagAppInfoDirty",0)) temp.flags |= dlpDBFlagAppInfoDirty;
    if (DGETLONG($input,"flagBackup",0)) temp.flags |= dlpDBFlagBackup;
    if (DGETLONG($input,"flagClipping",0)) temp.flags |= dlpDBFlagClipping;
    if (DGETLONG($input,"flagOpen",0)) temp.flags |= dlpDBFlagOpen;
    if (DGETLONG($input,"flagNewer",0)) temp.flags |= dlpDBFlagNewer;
    if (DGETLONG($input,"flagReset",0)) temp.flags |= dlpDBFlagReset;
    if (DGETLONG($input,"flagCopyPrevention",0)) temp.flags |= dlpDBFlagCopyPrevention;
    if (DGETLONG($input,"flagStream",0)) temp.flags |= dlpDBFlagStream;
    temp.miscFlags = 0;
    if (DGETLONG($input,"flagExcludeFromSync",0)) temp.miscFlags |= dlpDBMiscFlagExcludeFromSync;
    $1 = &temp;
}
    
    
// struct CardInfo
%typemap (python,argout) struct CardInfo *OUTPUT {
    PyObject *o;

    if ($1) {
	o = Py_BuildValue("{sisislslslslsssssi}",
			  "card", $1->card,
			  "version", $1->version,
			  "creation", $1->creation,
			  "romSize", $1->romSize,
			  "ramSize", $1->ramSize,
			  "ramFree", $1->ramFree,
			  "name", $1->name,
			  "manufacturer", $1->manufacturer,
			  "more", $1->more);
	$result = t_output_helper($result, o);
    }
}

%typemap (python,in,numinputs=0) struct CardInfo *OUTPUT (struct CardInfo temp) {
    $1 = &temp;
}

%typemap (python,argout) struct NetSyncInfo *OUTPUT {
    PyObject *o;
    if ($1){
	o = Py_BuildValue("{sissssss}",
			  "lanSync", $1->lanSync,
			  "hostName", $1->hostName,
			  "hostAddress", $1->hostAddress,
			  "hostSubnetMask", $1->hostSubnetMask);
	$result = t_output_helper($result, o);
    }
}

%typemap (python,in,numinputs=0) struct NetSyncInfo *OUTPUT (struct NetSyncInfo temp) {
    $1 = &temp;
}

%typemap (python,in) struct NetSyncInfo * {
    static struct NetSyncInfo temp;

    temp.lanSync = (int) DGETLONG($input,"lanSync",0);
    strncpy(temp.hostName, DGETSTR($input,"hostName",""), 256);
    strncpy(temp.hostAddress, DGETSTR($input,"hostAddress",""), 40);
    strncpy(temp.hostSubnetMask, DGETSTR($input,"hostSubnetMask",""), 40);

    $1 = &temp;
}


%typemap (python,out) DLPERROR {
    if ($1 < 0) {
	PyErr_SetObject(PIError, Py_BuildValue("(is)", $1,
					     dlp_strerror($1)));
	return NULL;
    }
    $result = Py_None;
    Py_INCREF(Py_None);
}
%typemap (python,out) DLPDBERROR {
    if ($1 == -5) {
	Py_INCREF(Py_None);
	return Py_None;
    } else if ($1 < 0) {
	PyErr_SetObject(PIError, Py_BuildValue("(is)", $1,
					     dlp_strerror($1)));
	return NULL;
    }
    $result = Py_None;
    Py_INCREF(Py_None);
}

// XXX
%typemap (python,in) (const void *INBUF, size_t INBUFLEN) {
  $1 = (void *)PyString_AsString($input);
  $2 = PyString_Size($input);
}

// Used by dlp_ReadAppPreference
%typemap (python,argout) (void *OUTBUF, size_t *OUTBUFLEN, int *OUTPUT) {
  PyObject *o;
  if ($1) {
    o = Py_BuildValue("is#", $3, $1, $2);
    $result = t_output_helper($result, o);
  }
}

%typemap (python,in,numinputs=0) (int reqbytes, pi_buffer_t *OUTBUF) {
  $1 = 0xFFFF;
  $2 = pi_buffer_new($1);
}


%{

/* sd, dbf, sort, start, max, recordid_t *IDS, int *count
 */
static PyObject *_wrap_dlp_ReadRecordIDList (PyObject *self, PyObject *args) {
    int sd, dbf, sort, start, max;
    int ret;
    recordid_t *buf;
    int count, i;
    PyObject *list;

    buf = (recordid_t *)PyMem_Malloc(0xFFFF);    

    if (!PyArg_ParseTuple(args, "iiiii", &sd, &dbf, &sort, &start, &max))
	return NULL;

/* this is a rather simplistic wrapper.  if max is too big, we just
 * refuse to do it; we don't loop, figuring that that is the job of
 * the python wrapper.
 */
    if (max > (0xFFFF/sizeof(recordid_t))) {
	PyErr_SetString(PyExc_ValueError, "can only return about 64k worth of ids at once");
	return NULL;
    }

    ret = dlp_ReadRecordIDList(sd, dbf, sort, start, max, buf, &count);

    if (ret < 0) {
	PyErr_SetObject(PIError, Py_BuildValue("(is)", ret, dlp_strerror(ret)));
	PyMem_Free(buf);
	return NULL;
    } else {
	list = PyList_New(0);
	for (i=0; i<count; i++)
	    PyList_Append(list, PyInt_FromLong((long)buf[i]));
	PyMem_Free(buf);
	return list;
    }
}

%}
