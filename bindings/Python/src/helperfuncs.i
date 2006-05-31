/*
 * helperfuncs.i
 *
 * Factored-out functions to build return values from common structures
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

static PyObject *ConvertFromEncoding(const char *data, const char *encoding, const char *errors, int allowErrors)
{
    PyObject *buffer, *string = NULL;

    buffer = PyBuffer_FromMemory((void *)data, strlen(data));
    if (buffer == NULL)
    {
        if (allowErrors)
        {
            PyErr_Clear();
            Py_INCREF(Py_None);
            return Py_None;
        }
        return NULL;
    }

    string = PyUnicode_FromEncodedObject(buffer, encoding, errors);
    if (string == NULL)
        goto error;

    Py_DECREF(buffer);
    return string;

error:
    Py_XDECREF(buffer);
    Py_XDECREF(string);
    if (allowErrors)
    {
        PyErr_Clear();
        Py_INCREF(Py_None);
        return Py_None;
    }
    return NULL;
}

static int ConvertToEncoding(PyObject *object, const char *encoding, const char *errors, int allowErrors, char *buffer, int maxBufSize)
{
    int len;
    char *s;
    PyObject *encoded = NULL;

    if (PyString_Check(object))
        encoded = PyString_AsEncodedObject(object, encoding, errors);
    else if (PyUnicode_Check(object))
        encoded = PyUnicode_AsEncodedString(object, encoding, errors);

    if (encoded == NULL)
        goto error;

    s = PyString_AsString(encoded);
    if (s == NULL)
        goto error;
    len = strlen(s);
    if (len)
    {
        if (len >= maxBufSize)
            len = maxBufSize - 1;
        memcpy(buffer, s, len);
    }
    buffer[len] = 0;

    Py_DECREF(encoded);
    return 1;

error:
    Py_XDECREF(encoded);
    if (allowErrors)
    {
        PyErr_Clear();
        memset(buffer, 0, maxBufSize);
    }
    return 0;
}

static PyObject *PyObjectFromDBInfo(const struct DBInfo *dbi)
{
    PyObject *returnObj;
    PyObject *nameObj = ConvertFromEncoding(dbi->name, "palmos", "replace", 1);

	returnObj = Py_BuildValue("{sisisisOsOsislslslslsisOsisisisisisisisisisisisisisisi}",
			"more", dbi->more,
			"flags", dbi->flags,
			"miscFlags", dbi->miscFlags,
			"type", PyString_FromStringAndSize(printlong(dbi->type), 4),
			"creator", PyString_FromStringAndSize(printlong(dbi->creator), 4),
			"version", dbi->version,
			"modnum", dbi->modnum,
			"createDate", dbi->createDate,
			"modifyDate", dbi->modifyDate,
			"backupDate", dbi->backupDate,
			"index", dbi->index,
			"name", nameObj,

			"flagResource", !!(dbi->flags & dlpDBFlagResource),
			"flagReadOnly", !!(dbi->flags & dlpDBFlagReadOnly),
			"flagAppInfoDirty", !!(dbi->flags & dlpDBFlagAppInfoDirty),
			"flagBackup", !!(dbi->flags & dlpDBFlagBackup),
			"flagLaunchable", !!(dbi->flags & dlpDBFlagLaunchable),
			"flagOpen", !!(dbi->flags & dlpDBFlagOpen),
			"flagNewer", !!(dbi->flags & dlpDBFlagNewer),
			"flagReset", !!(dbi->flags & dlpDBFlagReset),
			"flagCopyPrevention", !!(dbi->flags & dlpDBFlagCopyPrevention),
			"flagStream", !!(dbi->flags & dlpDBFlagStream),
			"flagExcludeFromSync", !!(dbi->miscFlags & dlpDBMiscFlagExcludeFromSync),
			
			"flagSchema", !!(dbi->flags & dlpDBFlagSchema),
			"flagSecure", !!(dbi->flags & dlpDBFlagSecure),
			"flagExtended", !!(dbi->flags & dlpDBFlagExtended),
			"flagFixedUp", !!(dbi->flags & dlpDBFlagFixedUp));

    Py_DECREF(nameObj);
    return returnObj;
}

/* unused at that time
static int PyObjectToDBInfo(PyObject *o, struct DBInfo *di)
{
    PyObject *nameObj;

	di->more = (int) DGETLONG(o, "more", 0);
    di->type = makelong(DGETSTR(o, "type", "    "));
    di->creator = makelong(DGETSTR(o, "creator", "    "));
    di->version = DGETLONG(o, "version", 0);
    di->modnum = DGETLONG(o, "modnum", 0);
    di->createDate = DGETLONG(o, "createDate", 0);
    di->modifyDate = DGETLONG(o, "modifyDate", 0);
    di->backupDate = DGETLONG(o, "backupDate", 0);
    di->index = DGETLONG(o, "index", 0);
    memset(di->name, 0, sizeof(di->name));
    nameObj = PyDict_GetItemString(o,"name");
    if (nameObj) {
        if (!ConvertToEncoding(nameObj, "palmos", "replace", 1, di->name, sizeof(di->name)))
            return 0;
    }
    di->flags = 0;
    if (DGETLONG(o,"flagResource",0)) di->flags |= dlpDBFlagResource;
    if (DGETLONG(o,"flagReadOnly",0)) di->flags |= dlpDBFlagReadOnly;
    if (DGETLONG(o,"flagAppInfoDirty",0)) di->flags |= dlpDBFlagAppInfoDirty;
    if (DGETLONG(o,"flagBackup",0)) di->flags |= dlpDBFlagBackup;
    if (DGETLONG(o,"flagLaunchable",0)) di->flags |= dlpDBFlagLaunchable;
    if (DGETLONG(o,"flagOpen",0)) di->flags |= dlpDBFlagOpen;
    if (DGETLONG(o,"flagNewer",0)) di->flags |= dlpDBFlagNewer;
    if (DGETLONG(o,"flagReset",0)) di->flags |= dlpDBFlagReset;
    if (DGETLONG(o,"flagCopyPrevention",0)) di->flags |= dlpDBFlagCopyPrevention;
    if (DGETLONG(o,"flagStream",0)) di->flags |= dlpDBFlagStream;
	if (DGETLONG(o,"flagSchema",0)) di->flags |= dlpDBFlagSchema;
	if (DGETLONG(o,"flagSecure",0)) di->flags |= dlpDBFlagSecure;
	if (DGETLONG(o,"flagExtended",0)) di->flags |= dlpDBFlagExtended;
	if (DGETLONG(o,"flagFixedUp",0)) di->flags |= dlpDBFlagFixedUp;
    di->miscFlags = 0;
    if (DGETLONG(o,"flagExcludeFromSync",0)) di->miscFlags |= dlpDBMiscFlagExcludeFromSync;
    return 1;
}
*/

static PyObject* PyObjectFromDBSizeInfo(const struct DBSizeInfo *si)
{
	return Py_BuildValue("{slslslslslsl}",
			"numRecords", si->numRecords,
			"totalBytes", si->totalBytes,
			"dataBytes", si->dataBytes,
			"appBlockSize", si->appBlockSize,
			"sortBlockSize", si->sortBlockSize,
			"maxRecSize", si->maxRecSize);
}

static PyObject* PyObjectFromCardInfo(const struct CardInfo *ci)
{
    PyObject *returnObj, *nameObj, *manufacturerObj;

    nameObj = ConvertFromEncoding(ci->name, "cp1252", "replace", 1);
    manufacturerObj = ConvertFromEncoding(ci->manufacturer, "cp1252", "replace", 1);

	returnObj = Py_BuildValue("{sisislslslslsOsOsi}",
					      "card", ci->card,
					      "version", ci->version,
			    		  "creation", ci->creation,
						  "romSize", ci->romSize,
					      "ramSize", ci->ramSize,
					      "ramFree", ci->ramFree,
					      "name", nameObj,
					      "manufacturer", manufacturerObj,
					      "more", ci->more);

    Py_DECREF(nameObj);
    Py_DECREF(manufacturerObj);
    return returnObj;
}

static PyObject* PyObjectFromSysInfo(const struct SysInfo *si)
{
	return Py_BuildValue("{slslss#}",
						  "romVersion", si->romVersion,
						  "locale", si->locale,
						  "name", si->prodID, si->prodIDLength);
}

static PyObject* PyObjectFromPilotUser(const struct PilotUser *pi)
{
    PyObject *nameObj, *passwordObj, *returnObj;

    nameObj = ConvertFromEncoding(pi->username, "palmos", "replace", 1);
    passwordObj = ConvertFromEncoding(pi->password, "palmos", "strict", 1);

    returnObj = Py_BuildValue("{slslslslslsOsO}",
						"userID", pi->userID,
						"viewerID", pi->viewerID,
						"lastSyncPC", pi->lastSyncPC,
						"successfulSyncDate", pi->successfulSyncDate,
						"lastSyncDate", pi->lastSyncDate,
						"name", nameObj,
						"password", passwordObj);

    Py_DECREF(nameObj);
    Py_DECREF(passwordObj);
    return returnObj;
}

static int PyObjectToPilotUser(PyObject *o, struct PilotUser *pi)
{
    /* return 0 if string conversion to palmos charset failed, otherwise return 1 */
    PyObject *stringObj;

    pi->userID = DGETLONG(o,"userID",0);
    pi->viewerID = DGETLONG(o,"viewerID",0);
    pi->lastSyncPC = DGETLONG(o,"lastSyncPC",0);
    pi->successfulSyncDate = DGETLONG(o,"successfulSyncDate",0);
    pi->lastSyncDate = DGETLONG(o,"lastSyncDate",0);

    memset(pi->username, 0, sizeof(pi->username));
    stringObj = PyDict_GetItemString(o,"name");
    if (stringObj) {
        if (!ConvertToEncoding(stringObj, "palmos", "replace", 0, pi->username, sizeof(pi->username)))
            return 0;
    }

    memset(pi->password, 0, sizeof(pi->password));
    stringObj = PyDict_GetItemString(o,"password");
    if (stringObj) {
        if (!ConvertToEncoding(stringObj, "palmos", "strict", 0, pi->password, sizeof(pi->password)))
            return 0;
    }

    return 1;
}

static PyObject* PyObjectFromNetSyncInfo(const struct NetSyncInfo *ni)
{
	return Py_BuildValue("{sissssss}",
						  "lanSync", ni->lanSync,
						  "hostName", ni->hostName,
						  "hostAddress", ni->hostAddress,
						  "hostSubnetMask", ni->hostSubnetMask);
}

static void PyObjectToNetSyncInfo(PyObject *o, struct NetSyncInfo *ni)
{
    ni->lanSync = (int) DGETLONG(o,"lanSync",0);
    strncpy(ni->hostName, DGETSTR(o,"hostName",""), sizeof(ni->hostName));
    strncpy(ni->hostAddress, DGETSTR(o,"hostAddress",""), sizeof(ni->hostAddress));
    strncpy(ni->hostSubnetMask, DGETSTR(o,"hostSubnetMask",""), sizeof(ni->hostSubnetMask));
}

%}


