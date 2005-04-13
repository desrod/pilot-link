//
// Factored-out functions to build return values from common structures
//

%{

static PyObject *PyObjectFromDBInfo(const struct DBInfo *dbi)
{
	return Py_BuildValue("{sisisisOsOsislslslslsisssisisisisisisisisisisisisisisi}",
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
			"name", dbi->name,
			
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
}

static void PyObjectToDBInfo(PyObject *o, struct DBInfo *di)
{
	di->more = (int) DGETLONG(o, "more", 0);
    di->type = makelong(DGETSTR(o, "type", "    "));
    di->creator = makelong(DGETSTR(o, "creator", "    "));
    di->version = DGETLONG(o, "version", 0);
    di->modnum = DGETLONG(o, "modnum", 0);
    di->createDate = DGETLONG(o, "createDate", 0);
    di->modifyDate = DGETLONG(o, "modifyDate", 0);
    di->backupDate = DGETLONG(o, "backupDate", 0);
    di->index = DGETLONG(o, "index", 0);
    strncpy(di->name, DGETSTR(o,"name",""), 34);
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
}

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
	return Py_BuildValue("{sisislslslslsssssi}",
					      "card", ci->card,
					      "version", ci->version,
			    		  "creation", ci->creation,
						  "romSize", ci->romSize,
					      "ramSize", ci->ramSize,
					      "ramFree", ci->ramFree,
					      "name", ci->name,
					      "manufacturer", ci->manufacturer,
					      "more", ci->more);
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
		return Py_BuildValue("{slslslslslssss#}",
							"userID", pi->userID,
							"viewerID", pi->viewerID,
							"lastSyncPC", pi->lastSyncPC,
							"successfulSyncDate", pi->successfulSyncDate,
							"lastSyncDate", pi->lastSyncDate,
							"name", pi->username,
							"password", pi->password, pi->passwordLength);
}

static void PyObjectToPilotUser(PyObject *o, struct PilotUser *pi)
{
    PyObject *foo;

    pi->userID = DGETLONG(o,"userID",0);
    pi->viewerID = DGETLONG(o,"viewerID",0);
    pi->lastSyncPC = DGETLONG(o,"lastSyncPC",0);
    pi->successfulSyncDate = DGETLONG(o,"successfulSyncDate",0);
    pi->lastSyncDate = DGETLONG(o,"lastSyncDate",0);
    strncpy(pi->username, DGETSTR(o,"name",""), 128);

    foo = PyDict_GetItemString(o,"password");
    if (PyString_Check(foo)) {
		int l = PyString_Size(foo);
		pi->passwordLength = l;
		memcpy(pi->password, PyString_AsString(foo), l);
    }
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
    strncpy(ni->hostName, DGETSTR(o,"hostName",""), 256);
    strncpy(ni->hostAddress, DGETSTR(o,"hostAddress",""), 40);
    strncpy(ni->hostSubnetMask, DGETSTR(o,"hostSubnetMask",""), 40);
}

%}


