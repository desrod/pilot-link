// -*- C -*-

%{
typedef int DLPERROR;
typedef int DLPDBERROR;
%}


%typemap (python,out) DLPERROR {
    if ($1 < 0) {
      // Would be nice to check pi_palmos_error here if necessary, but we don't know the sd :-(
      PyErr_SetObject(DLPError, Py_BuildValue("(is)", $1, dlp_strerror($1)));
      return NULL;
    }
    $result = Py_None;
    Py_INCREF(Py_None);
}


%typemap (python,out) DLPDBERROR {
    if ($1 == -5) {
      /* Why do we allow -5 as a non-error again ? NCP */
	Py_INCREF(Py_None);
	$result = Py_None;
    } else if ($1 < 0) {
	PyErr_SetObject(DLPError, Py_BuildValue("(is)", $1, dlp_strerror($1)));
	return NULL;
    }
    Py_INCREF(Py_None);
    $result = Py_None;
}



/* Here we go for a list of all the functions which need handling as
   if they return DLPERROR or DLPDBERROR */

%typemap (out) int dlp_GetSysDateTime = DLPERROR;
%typemap (out) int dlp_SetSysDateTime = DLPERROR;
%typemap (out) int dlp_ReadStorageInfo = DLPERROR;
%typemap (out) int dlp_ReadSysInfo = DLPERROR;
%typemap (out) int dlp_ReadDBList = DLPERROR;
%typemap (out) int dlp_FindDBInfo = DLPERROR;
%typemap (out) int dlp_OpenDB = DLPERROR;
%typemap (out) int dlp_CloseDB = DLPERROR;
%typemap (out) int dlp_CloseDB_All = DLPERROR;
%typemap (out) int dlp_DeleteDB = DLPERROR;
%typemap (out) int dlp_CreateDB = DLPERROR;
%typemap (out) int dlp_ResetSystem = DLPERROR;
%typemap (out) int dlp_AddSyncLogEntry = DLPERROR;
%typemap (out) int dlp_OpenConduit = DLPERROR;
%typemap (out) int dlp_EndOfSync = DLPERROR;
%typemap (out) int dlp_AbortSync = DLPERROR;
%typemap (out) int dlp_ReadOpenDBInfo = DLPERROR;
%typemap (out) int dlp_MoveCategory = DLPERROR;
%typemap (out) int dlp_WriteUserInfo = DLPERROR;
%typemap (out) int dlp_ReadUserInfo = DLPERROR;
%typemap (out) int dlp_ResetLastSyncPC = DLPERROR;
%typemap (out) int dlp_ReadAppBlock = DLPERROR;
%typemap (out) int dlp_WriteAppBlock = DLPERROR;
%typemap (out) int dlp_ReadSortBlock = DLPERROR;
%typemap (out) int dlp_WriteSortBlock = DLPERROR;
%typemap (out) int dlp_ResetDBIndex = DLPERROR;
%typemap (out) int dlp_WriteRecord = DLPDBERROR;
%typemap (out) int dlp_DeleteRecord = DLPERROR;
%typemap (out) int dlp_DeleteCategory = DLPERROR;
%typemap (out) int dlp_ReadResourceByType = DLPDBERROR;
%typemap (out) int dlp_ReadResourceByIndex = DLPDBERROR;
%typemap (out) int dlp_WriteResource = DLPDBERROR;
%typemap (out) int dlp_DeleteResource = DLPERROR;
%typemap (out) int dlp_ReadNextModifiedRec = DLPDBERROR;
%typemap (out) int dlp_ReadNextModifiedRecInCategory = DLPDBERROR;
%typemap (out) int dlp_ReadNextRecInCategory = DLPDBERROR;
%typemap (out) int dlp_ReadRecordById = DLPDBERROR;
%typemap (out) int dlp_ReadRecordByIndex = DLPDBERROR;
%typemap (out) int dlp_CleanUpDatabase = DLPERROR;
%typemap (out) int dlp_ResetSyncFlags = DLPERROR;
%typemap (out) int dlp_ReadFeature = DLPERROR;
%typemap (out) int dlp_ReadNetSyncInfo = DLPERROR;
%typemap (out) int dlp_WriteNetSyncInfo = DLPERROR;
%typemap (out) int dlp_ReadAppPreference = DLPERROR;
%typemap (out) int dlp_WriteAppPreference = DLPERROR;
