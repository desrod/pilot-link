#include "Python.h"
#include "pi-source.h"
#include "pi-dlp.h"
#include "pi-file.h"
#include "pi-mail.h"
#include "pi-memo.h"
#include "pi-todo.h"
#include "pi-socket.h"
#include "pi-syspkt.h"

extern char * printlong (unsigned long val);
extern unsigned long makelong (char * c);

static PyObject * Error;

static PyObject * DBClasses;
static PyObject * PrefClasses;

static PyMethodDef PiFile_methods[];

static PyMethodDef Dlp_methods[];

static PyMethodDef DlpDB_methods[];

typedef struct {
	PyObject_HEAD
	struct pi_file	*pf;
	
	PyObject *Class;
} PiFileObject;

staticforward PyTypeObject PiFile_Type;

#define PiFileObject_Check(v) ((v)->ob_type == &PiFile_Type)

typedef struct {
	PyObject_HEAD

	struct RPC_params * p;
} RpcObject;

staticforward PyTypeObject Rpc_Type;

#define RpcObject_Check(v) ((v)->ob_type == &Rpc_Type)

typedef struct {
	PyObject_HEAD
	void * buffer;
	int socket;
} DlpObject;

typedef struct {
	PyObject_HEAD
	DlpObject * socket;
	int handle;
	
	PyObject *Class;
} DlpDBObject;

staticforward PyTypeObject Dlp_Type;

#define DlpObject_Check(v) ((v)->ob_type == &Dlp_Type)

staticforward PyTypeObject DlpDB_Type;

#define DlpDBObject_Check(v) ((v)->ob_type == &DlpDB_Type)

static void
PiFile_dealloc(self)
	PiFileObject *self;
{
	Py_XDECREF(self->Class);
	if (self->pf)
		pi_file_close(self->pf);
	PyMem_DEL(self);
}

static void
Rpc_dealloc(self)
	RpcObject *self;
{
	if (self->p)
		free(self->p);
	PyMem_DEL(self);
}

static void
Dlp_dealloc(self)
	DlpObject *self;
{
	if (self->buffer)
		free(self->buffer);
	if (self->socket)
		pi_close(self->socket);
	PyMem_DEL(self);
}

static void
DlpDB_dealloc(self)
	DlpDBObject *self;
{
	Py_XDECREF(self->Class);
	if (self->handle)
		dlp_CloseDB(self->socket->socket, self->handle);
	if (self->socket)
		Py_DECREF(self->socket);
	PyMem_DEL(self);
}

static PyObject *
PiFile_getattr(self, name)
	PiFileObject * self;
	char * name;
{
	if ((name[0] == 'C') && (strcmp(name, "Class")==0)) {
		Py_INCREF(self->Class);
		return self->Class;
	}
	return Py_FindMethod(PiFile_methods, (PyObject *)self, name);
}

int
PiFile_setattr(self, name, v)
	PiFileObject * self;
	char * name;
	PyObject * v;
{
	if ((name[0] == 'C') && (strcmp(name, "Class")==0)) {
		if (!v)  {
			PyErr_SetString(PyExc_AttributeError,
				"class attribute may not be deleted");
			return -1;
		}
		Py_DECREF(self->Class);
		self->Class = v;
		Py_INCREF(self->Class);
		return 0;
	}
	PyErr_SetString(PyExc_AttributeError,
		"attribute not settable");
	return -1;
}


staticforward PyTypeObject PiFile_Type = {
        PyObject_HEAD_INIT(&PyType_Type)
        0,			/*ob_size*/
        "pdapilot.file",		/*tp_name*/
	sizeof(PiFileObject),	/*tp_basicsize*/
        0,			/*tp_itemsize*/
				/* methods */
	(destructor)PiFile_dealloc,	/*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)PiFile_getattr,	/*tp_getattr*/
	(setattrfunc)PiFile_setattr,	/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};

staticforward PyTypeObject Rpc_Type = {
        PyObject_HEAD_INIT(&PyType_Type)
        0,			/*ob_size*/
        "pdapilot.rpc",		/*tp_name*/
	sizeof(RpcObject),	/*tp_basicsize*/
        0,			/*tp_itemsize*/
				/* methods */
	(destructor)Rpc_dealloc,	/*tp_dealloc*/
	0,			/*tp_print*/
	0,			/*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};

static PyObject *
Dlp_getattr(self, name)
	PyObject * self;
	char * name;
{
	return Py_FindMethod(Dlp_methods, (PyObject *)self, name);
}

staticforward PyTypeObject Dlp_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/*ob_size*/
	"pdapilot.dlp",		/*tp_name*/
	sizeof(DlpObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
				/* methods */
	(destructor)Dlp_dealloc,	/*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)Dlp_getattr,	/*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};

static PyObject *
DlpDB_getattr(self, name)
	DlpDBObject * self;
	char * name;
{
	if ((name[0] == 'C') && (strcmp(name, "Class")==0)) {
		Py_INCREF(self->Class);
		return self->Class;
	}
	return Py_FindMethod(DlpDB_methods, (PyObject *)self, name);
}

int
DlpDB_setattr(self, name, v)
	DlpDBObject * self;
	char * name;
	PyObject * v;
{
	if ((name[0] == 'C') && (strcmp(name, "Class")==0)) {
		if (!v)  {
			PyErr_SetString(PyExc_AttributeError,
				"class attribute may not be deleted");
			return -1;
		}
		Py_DECREF(self->Class);
		self->Class = v;
		Py_INCREF(self->Class);
		return 0;
	}
	PyErr_SetString(PyExc_AttributeError,
		"attribute not settable");
	return -1;
}

staticforward PyTypeObject DlpDB_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/*ob_size*/
	"pdapilot.dlp.db",	/*tp_name*/
	sizeof(DlpDBObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
				/* methods */
	(destructor)DlpDB_dealloc,	/*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)DlpDB_getattr,	/*tp_getattr*/
	(setattrfunc)DlpDB_setattr,	/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};

static PyObject *
Socket(self, args)
	PyObject *self;
	PyObject *args;
{
	int domain,type,protocol,result;
	if (!PyArg_ParseTuple(args, "iii", &domain,&type,&protocol))
		return NULL;
	result = pi_socket(domain,type,protocol);
	if (result==-1) {
		PyErr_SetFromErrno(Error);
		return NULL;
	} else
		return Py_BuildValue("i", result);
}

static PyObject *
CloseSocket(self, args)
	PyObject *self;
	PyObject *args;
{
	int socket,result;
	if (!PyArg_ParseTuple(args, "i", &socket))
		return NULL;
	result = pi_close(socket);
	if (result==-1) {
		PyErr_SetFromErrno(Error);
		return NULL;
	} else
		return Py_BuildValue("i", result);
}

static PyObject *
Read(self, args)
	PyObject *self;
	PyObject *args;
{
	int length,socket,result;
	char * data;
	if (!PyArg_ParseTuple(args, "is#", &socket, &data, &length))
		return NULL;
	result = pi_read(socket,data,length);
	if (result==-1) {
		PyErr_SetFromErrno(Error);
		return NULL;
	} else
		return Py_BuildValue("i", result);
}

static PyObject *
Write(self, args)
	PyObject *self;
	PyObject *args;
{
	int length,socket, result;
	char * data;
	if (!PyArg_ParseTuple(args, "is#", &socket, &data, &length))
		return NULL;
	result = pi_write(socket,data,length);
	if (result==-1) {
		PyErr_SetFromErrno(Error);
		return NULL;
	} else
		return Py_BuildValue("i", result);
}

static PyObject *
Bind(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject * addr;
	int socket;
	struct pi_sockaddr * a;
	int len,i;
	if (!PyArg_ParseTuple(args, "iO", &socket, &addr))
		return NULL;
	if (PyString_Check(addr)) {
		a = (struct pi_sockaddr *)PyString_AsString(addr);
		len = PyString_Size(addr);
		i = pi_bind(socket,(struct sockaddr*)a,len);
	} else if (PyDict_Check(addr)) {
		PyObject * e = PyDict_GetItemString(addr, "device");
		if (e) {
			len = PyString_Size(e)+sizeof(struct pi_sockaddr);
			a = malloc(len);
			strcpy(a->pi_device, PyString_AsString(e));
			
			e = PyDict_GetItemString(addr, "family");
			a->pi_family = e ? PyInt_AsLong(e) : 0;
			
		}
		i = pi_bind(socket,(struct sockaddr*)a,len);
		free(a);
	} else {
		PyErr_SetString(Error, "second argument not string or dict");
		return NULL;
	}
	if (i==-1) {
		PyErr_SetFromErrno(Error);
		return NULL;
	} else
		return Py_BuildValue("i", i);
}

static PyObject *
Listen(self, args)
	PyObject *self;
	PyObject *args;
{
	int socket,backlog=1,result;
	if (!PyArg_ParseTuple(args, "i|i", &socket,&backlog))
		return NULL;
	result = pi_listen(socket,backlog);
	if (result==-1) {
		PyErr_SetFromErrno(Error);
		return NULL;
	} else
		return Py_BuildValue("i", result);
}

static PyObject *
Accept(self, args)
	PyObject *self;
	PyObject *args;
{
	int socket;
	int result;
	if (!PyArg_ParseTuple(args, "i", &socket))
		return NULL;
	
	result = pi_accept(socket,0,0);
	
	if (result>=0) {
		DlpObject * obj = PyObject_NEW(DlpObject, &Dlp_Type);
		obj->socket = result;
		obj->buffer = malloc(0xffff);
		return (PyObject*)obj;
	} else {
		PyErr_SetFromErrno(Error);
		return NULL;
	}
}

static PyObject *
OpenPort(self, args)
	PyObject *self;
	PyObject *args;
{
	char * port;
	PyObject *a, *b, *c;
	if (!PyArg_ParseTuple(args, "s", &port))
		return NULL;
	
	a = Py_BuildValue("(iii)", PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP);
	b = Socket(self, a);
	Py_DECREF(a);
	if (!b)
		return NULL;
	a = Py_BuildValue("(O{siss})", b, "family", PI_AF_SLP, "device", port);
	c = Bind(self, a);
	Py_DECREF(a);
	if (!c)
		return NULL;
	a = Py_BuildValue("(Oi)", b, 1);
	c = Listen(self, a);
	Py_DECREF(a);
	if (!c)
		return NULL;
	
	return b;
}

static int
ParseTm(o, v)
	PyObject * o;
	void *v;
{
	struct tm * t = v;
	
	if (!PyArg_ParseTuple(o, "iiiiiiiii", 
		&t->tm_year, 
		&t->tm_mon,
		&t->tm_mday,
		&t->tm_hour,
		&t->tm_min,
		&t->tm_sec,
		&t->tm_wday,
		&t->tm_yday,
		&t->tm_isdst))
		return 0;
	t->tm_year-=1900;
	t->tm_mon--;
	t->tm_wday = (t->tm_wday+8)%7;
	t->tm_yday--;
	
	return 1;
}

static PyObject *
BuildTm(v)
	void *v;
{
	struct tm * t = v;
	
	/* Obey the rules used by Python's timemodule */
	
	return Py_BuildValue("(iiiiiiiii)",
		t->tm_year+1900,
		t->tm_mon+1,
		t->tm_mday,
		t->tm_hour,
		t->tm_min,
		t->tm_sec,
		(t->tm_wday+6)%7,
		t->tm_yday+1,
		t->tm_isdst);
}

static int
ParseChar4(o, v)
	PyObject * o;
	void *v;
{
	if (PyString_Check(o)) {
		if (PyString_Size(o) != 4) {
			PyErr_SetString(Error, "code string is not four bytes long");
			return 0;
		}
		*(unsigned long*)v = makelong(PyString_AsString(o));
	} else if (PyInt_Check(o)) {
		*(unsigned long*)v = PyInt_AsLong(o);
	} else {
		PyErr_SetString(Error, "code is not string or int");
		return 0;
	}
	return 1;
}

static PyObject *
BuildChar4(v)
	void *v;
{
	char * l = printlong(*(unsigned long*)v);
	if (	(isalpha(l[0]) || (l[0] == ' ') || (l[0] == '_')) &&
		(isalpha(l[1]) || (l[1] == ' ') || (l[1] == '_')) &&
		(isalpha(l[2]) || (l[2] == ' ') || (l[2] == '_')) &&
		(isalpha(l[3]) || (l[3] == ' ') || (l[3] == '_')))
		return PyString_FromString(l);
	else
		return PyInt_FromLong(*(unsigned long*)v);
}

#define Dlp_CheckError(x) 	\
	if ((x)<0) { 	\
		PyErr_SetString(Error, dlp_strerror((x))); 	\
		return NULL;	\
	} else ;

#define DlpDB_CheckError(x) 			\
	if ((x)<0) { 				\
		if ((x)==-5) {			\
			Py_INCREF(Py_None);	\
			return Py_None;		\
		} else {			\
			PyErr_SetString(Error, dlp_strerror((x))); 	\
			return NULL;		\
		}				\
	} else ;

static PyObject *
ResetSystem(self, args)
	DlpObject *self;
	PyObject *args;
{
	int socket, result;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	result = dlp_ResetSystem(self->socket);
	Dlp_CheckError(result);
	return Py_BuildValue("i", result);
}

static PyObject *
Dirty(self, args)
	DlpObject *self;
	PyObject *args;
{
	int socket, result;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	result = dlp_ResetLastSyncPC(self->socket);
	Dlp_CheckError(result);
	return Py_BuildValue("i", result);
}

static PyObject *
OpenDB(self, args)
	DlpObject *self;
	PyObject *args;
{
	int mode = dlpOpenReadWrite, cardno = 0;
	char * name;
	int result, handle, raw = 0;
	PyObject * packer;
	DlpDBObject * obj;
	if (!PyArg_ParseTuple(args, "s|iii", &name, &mode, &cardno, &raw))
		return NULL;

	result = dlp_OpenDB(self->socket, cardno, mode, name, &handle);
	
	Dlp_CheckError(result);
	
	obj = PyObject_NEW(DlpDBObject, &DlpDB_Type);
	obj->socket = self;
	obj->handle = handle;
	Py_INCREF(self);
	
	obj->Class = 0;
	packer = PyDict_GetItemString(DBClasses, name);
	if (!packer)
		packer = PyDict_GetItemString(DBClasses, "");
	if (!packer) {
		PyErr_SetString(PyExc_ValueError, "pdapilot.DBClasses must contain a default");
		dlp_CloseDB(self->socket, handle);
		free(obj);
		return NULL;
	}
	if (packer) {
		obj->Class = packer;
		Py_XINCREF(packer);
	}
	
	printf("Done with openDB\n");
	
	return (PyObject*)obj;
}

static PyObject *
CreateDB(self, args)
	DlpObject *self;
	PyObject *args;
{
	char * name;
	long creator, type;
	int cardno=0, flags, version=1;
	int raw=0;
	int result;
	int handle;
	DlpDBObject * obj;
	PyObject * packer;
	if (!PyArg_ParseTuple(args, "sO&li|iii", &name, &ParseChar4, &creator, &type, &flags, &version, &cardno, &raw))
		return NULL;

	result = dlp_CreateDB(self->socket, creator, type, cardno, 
		flags, version, name, &handle);

	Dlp_CheckError(result);
	
	obj = PyObject_NEW(DlpDBObject, &DlpDB_Type);
	obj->socket = self;
	obj->handle = handle;
	Py_INCREF(self);

	/*obj->Pack = obj->Unpack = obj->PackAppBlock = obj->UnpackAppBlock = 
		obj->PackSortBlock = obj->UnpackSortBlock = 0;*/
	obj->Class = 0;
	packer = PyDict_GetItemString(DBClasses, name);
	if (!packer)
		packer = PyDict_GetItemString(DBClasses, "");
	if (!packer) {
		PyErr_SetString(PyExc_ValueError, "pdapilot.DBClasses must contain a default");
		dlp_CloseDB(self->socket, handle);
		free(obj);
		return NULL;
	}
	if (packer) {
		obj->Class = packer;
		Py_XINCREF(packer);
	}
	/*packer = PyDict_GetItemString(DBPackers, name);
	if (packer && PyTuple_Check(packer)) {
		PyArg_ParseTuple(packer, "|OOOOOO",
			&obj->Unpack, &obj->Pack,
			&obj->UnpackAppBlock, &obj->PackAppBlock,
			&obj->UnpackSortBlock, &obj->PackSortBlock);
		Py_XINCREF(obj->Pack);
		Py_XINCREF(obj->Unpack);
		Py_XINCREF(obj->PackAppBlock);
		Py_XINCREF(obj->UnpackAppBlock);
		Py_XINCREF(obj->PackSortBlock);
		Py_XINCREF(obj->UnpackSortBlock);
	}*/

	return (PyObject*)obj;
}

static PyObject *
CloseDB(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	
	if (self->handle) {
		int result = dlp_CloseDB(self->socket->socket, self->handle);
		self->handle = 0;		
		Dlp_CheckError(result);
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
DeleteRsc(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	unsigned long type;
	int id, result;
	
	if (!PyArg_ParseTuple(args, "O&i", &ParseChar4, &type, &id))
		return NULL;

	result = dlp_DeleteResource(self->socket->socket, self->handle, 0, type, id);

	Dlp_CheckError(result);

	return Py_BuildValue("i", result);
}

static PyObject *
DeleteAllRsc(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	int result;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	
	result = dlp_DeleteResource(self->socket->socket, self->handle, 1, 0, 0);
	
	Dlp_CheckError(result);
	
	return Py_BuildValue("i", result);
}

static PyObject *
DeleteRec(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	unsigned long id;
	int result;
	
	if (!PyArg_ParseTuple(args, "l", &id))
		return NULL;

	result = dlp_DeleteRecord(self->socket->socket, self->handle, 0, id);
	
	Dlp_CheckError(result);
	
	return Py_BuildValue("i", result);
}

static PyObject *
DeleteAllRec(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	int result;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
		
	result = dlp_DeleteRecord(self->socket->socket, self->handle, 1, 0);
	
	Dlp_CheckError(result);
	
	return Py_BuildValue("i", result);
}

static PyObject *
NextModRec(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	unsigned long id;
	int index, length, attr, category=-1;
	int result;
	PyObject * ret, *c, *callargs;
	if (!PyArg_ParseTuple(args, "|i", &category))
		return NULL;
	
	if (category == -1)
		result = dlp_ReadNextModifiedRec(self->socket->socket, self->handle, self->socket->buffer, &id, &index, &length, &attr, &category);
	else
		result = dlp_ReadNextModifiedRecInCategory(self->socket->socket, self->handle, index, self->socket->buffer, &id, &index, &length, &attr);
	
	DlpDB_CheckError(result);
	
	c = PyObject_GetAttrString(self->Class, "Record");
	callargs = Py_BuildValue("(s#Oilii)", self->socket->buffer, result, self, index, (long)id, attr, category);
	ret = PyEval_CallObject(c, callargs);
	Py_DECREF(callargs);
	return ret;
}

static PyObject *
NextCatRec(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	unsigned long id;
	int index, length, attr, category;
	int result;
	PyObject * ret, *callargs, *c;
	if (!PyArg_ParseTuple(args, "i", &category))
		return NULL;
	
	result = dlp_ReadNextRecInCategory(self->socket->socket, self->handle, index, self->socket->buffer, &id, &index, &length, &attr);
	
	DlpDB_CheckError(result);

	c = PyObject_GetAttrString(self->Class, "Record");
	callargs = Py_BuildValue("(s#Oilii)", self->socket->buffer, result, self, index, (long)id, attr, category);
	ret = PyEval_CallObject(c, callargs);
	Py_DECREF(callargs);
	return ret;
}

static PyObject *
GetRec(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	unsigned long id;
	int index, length, attr, category;
	int result;
	PyObject * ret, *c, *callargs;
	if (!PyArg_ParseTuple(args, "i", &index))
		return NULL;
	
	result = dlp_ReadRecordByIndex(self->socket->socket, self->handle, index, self->socket->buffer, &id, &length, &attr, &category);
	
	DlpDB_CheckError(result);
	
	c = PyObject_GetAttrString(self->Class, "Record");
	callargs = Py_BuildValue("(s#Oilii)", self->socket->buffer, result, self, index, (long)id, attr, category);
	ret = PyEval_CallObject(c, callargs);
	Py_DECREF(callargs);
	return ret;
}

static PyObject *
SetRec(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	unsigned long id=0;
	unsigned long newid;
	int index, length, attr=0, category=0;
	char * data;
	PyObject * h, *i;
	int result;

	if (!PyArg_ParseTuple(args, "O", &h))
		return NULL;
		
	i = PyObject_CallMethod(h, "pack", "O", self);
	data = PyString_AsString(i);
	length = PyString_Size(i);

	if (!(i=PyObject_GetAttrString(h, "id")))
		{ PyErr_SetString(PyExc_ValueError, "The record's .id attribute must be set");
			return NULL; } 
	else if (i == Py_None) 
		id = 0;
	else
		id = PyInt_AsLong(i);
	if (i=PyObject_GetAttrString(h, "attr")) attr = PyInt_AsLong(i);
	if (i=PyObject_GetAttrString(h, "category")) category = PyInt_AsLong(i);
	
	dlp_WriteRecord(self->socket->socket, self->handle, attr, id, category, data, length, &newid);
	
	DlpDB_CheckError(result);
	
	i = Py_BuildValue("l", (long)newid);
	PyObject_SetAttrString(h, "id", i);
	Py_INCREF(i);
	
	return i;
}

static PyObject *
SetRsc(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	unsigned long type;
	int id, length;
	char * data;
	int result;
	PyObject *h, *i;
	if (!PyArg_ParseTuple(args, "O", &h))
		return NULL;
		
	i = PyObject_CallMethod(h, "pack", "O", self);
	data = PyString_AsString(i);
	length = PyString_Size(i);

	if (!(i=PyObject_GetAttrString(h, "id")) || (i == Py_None))
		{ PyErr_SetString(PyExc_ValueError, "The resource's .id attribute must be set"); 
			return NULL; } 
	else
		id = PyInt_AsLong(i);
	if (!(i=PyObject_GetAttrString(h, "type")) || (i == Py_None))
		{ PyErr_SetString(PyExc_ValueError, "The resource's .type attribute must be set"); 
			return NULL; } 
	else
		type = ParseChar4(i);

	result = dlp_WriteResource(self->socket->socket, self->handle, type, id, data, length);

	DlpDB_CheckError(result);
	
	return Py_BuildValue("");
}

static PyObject *
GetRecById(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	unsigned long id;
	int index, result, length, attr, category;
	PyObject *ret, *c, *callargs;
	if (!PyArg_ParseTuple(args, "l", &id))
		return NULL;
	
	result = dlp_ReadRecordById(self->socket->socket, self->handle, id, self->socket->buffer, &index, &length, &attr, &category);
	
	DlpDB_CheckError(result);

	c = PyObject_GetAttrString(self->Class, "Record");
	callargs = Py_BuildValue("(s#Oilii)", self->socket->buffer, result, self, index, (long)id, attr, category);
	ret = PyEval_CallObject(c, callargs);
	Py_DECREF(callargs);
	return ret;
}

static PyObject *
GetRsc(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	unsigned long type;
	int id, length, index;
	int result;
	PyObject *ret, *c, *callargs;
	if (!PyArg_ParseTuple(args, "i", &index))
		return NULL;
	
	result = dlp_ReadResourceByIndex(self->socket->socket, self->handle, index, self->socket->buffer, &type, &id, &length);
	
	DlpDB_CheckError(result);

	c = PyObject_GetAttrString(self->Class, "Resource");
	callargs = Py_BuildValue("(s#OO&i)", self->socket->buffer, result, self, &BuildChar4, &type, id);
	ret = PyEval_CallObject(c, callargs);
	Py_DECREF(callargs);
	return ret;
}

static PyObject *
GetRscById(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	unsigned long type;
	int id, length, index;
	int result;
	PyObject *ret, *c, *callargs;
	if (!PyArg_ParseTuple(args, "O&i", &ParseChar4, &type, &id))
		return NULL;
	
	result = dlp_ReadResourceByType(self->socket->socket, self->handle, type, id, self->socket->buffer, &index, &length);

	DlpDB_CheckError(result);	

	c = PyObject_GetAttrString(self->Class, "Resource");
	callargs = Py_BuildValue("(s#OO&i)", self->socket->buffer, result, self, &BuildChar4, &type, id);
	ret = PyEval_CallObject(c, callargs);
	Py_DECREF(callargs);
	return ret;
}


static PyObject *
Records(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	int records, result;
	
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	
	result = dlp_ReadOpenDBInfo(self->socket->socket, self->handle, &records);
	
	Dlp_CheckError(result);
	
	return Py_BuildValue("i", records);
}

static PyObject *
RecordIDs(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	int sort=0, result;
	PyObject *list;
	int count, start;
	int i;
	recordid_t *id = (recordid_t*)self->socket->buffer;
	
	if (!PyArg_ParseTuple(args, "|i"), sort)
		return NULL;

	list = PyList_New(0);
	
	start = 0;
	for (;;) {
	  result = dlp_ReadRecordIDList(self->socket->socket, self->handle, 
	             sort, start, 0xFFFF/sizeof(recordid_t), id, &count);
	  if (result<0) {
	    Py_DECREF(list);
	    Dlp_CheckError(result);
	  } else {
	    for(i=0;i<count;i++)
	   	PyList_Append(list, PyInt_FromLong((long)id[i]));
	    if (count == (0xFFFF/sizeof(recordid_t)))
	      start = count;
	    else
	      break;
	  }
	}
	
	return list;
}

static PyObject *
BlankAppBlock(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	PyObject *ret;
	PyObject *c, *callargs;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	
	c = PyObject_GetAttrString(self->Class, "AppBlock");
	callargs = Py_BuildValue("()");
	ret = PyEval_CallObject(c, callargs);
	Py_DECREF(callargs);
	return ret;
}

static PyObject *
BlankSortBlock(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	PyObject *ret;
	PyObject *c, *callargs;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	
	c = PyObject_GetAttrString(self->Class, "SortBlock");
	callargs = Py_BuildValue("()");
	ret = PyEval_CallObject(c, callargs);
	Py_DECREF(callargs);
	return ret;
}

static PyObject *
BlankRecord(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	PyObject *ret;
	PyObject *c, *callargs;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	
	c = PyObject_GetAttrString(self->Class, "Record");
	callargs = Py_BuildValue("()");
	ret = PyEval_CallObject(c, callargs);
	Py_DECREF(callargs);
	return ret;
}

static PyObject *
BlankResource(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	PyObject *ret;
	PyObject *c, *callargs;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	
	c = PyObject_GetAttrString(self->Class, "Resource");
	callargs = Py_BuildValue("()");
	ret = PyEval_CallObject(c, callargs);
	Py_DECREF(callargs);
	return ret;
}


static PyObject *
GetAppBlock(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	int records;
	int length=0xffff,offset=0, result;
	PyObject *ret;
	PyObject *c, *callargs;
	if (!PyArg_ParseTuple(args, "|ii", &length, &offset))
		return NULL;
	
	result = dlp_ReadAppBlock(self->socket->socket, self->handle, offset, self->socket->buffer, length);
	
	Dlp_CheckError(result);
	
	c = PyObject_GetAttrString(self->Class, "AppBlock");
	callargs = Py_BuildValue("(s#O)", self->socket->buffer, result, self);
	ret = PyEval_CallObject(c, callargs);
	Py_DECREF(callargs);
	return ret;
}

static PyObject *
SetAppBlock(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	char * data;
	int length,result;
	PyObject * h, *i, *c;
	if (!PyArg_ParseTuple(args, "O", &h))
		return NULL;
		
	h = PyObject_CallMethod(h, "pack", "O", self);
	data = PyString_AsString(h);
	length = PyString_Size(h);

	fprintf(stderr, "App block to write:\n");
	dumpdata(data,length);
	result = dlp_WriteAppBlock(self->socket->socket, self->handle, data, length);
	
	Dlp_CheckError(result);
	
	return Py_BuildValue("i", result);
}

static PyObject *
GetSortBlock(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	int records;
	int length=0xffff,offset=0, result;
	PyObject *ret, *c, *callargs;
	if (!PyArg_ParseTuple(args, "|ii", &length, &offset))
		return NULL;
	
	result = dlp_ReadSortBlock(self->socket->socket, self->handle, offset, self->socket->buffer, length);
	
	Dlp_CheckError(result);
	
	c = PyObject_GetAttrString(self->Class, "SortBlock");
	callargs = Py_BuildValue("(s#O)", self->socket->buffer, result, self);
	ret = PyEval_CallObject(c, callargs);
	Py_DECREF(callargs);
	return ret;
}

static PyObject *
SetSortBlock(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	char * data;
	int length, result;
	PyObject *h, *i;
	if (!PyArg_ParseTuple(args, "O", &h))
		return NULL;
		
	i = PyObject_CallMethod(h, "pack", "O", self);
	data = PyString_AsString(i);
	length = PyString_Size(i);

	result = dlp_WriteSortBlock(self->socket->socket, self->handle, data, length);
	
	Dlp_CheckError(result);
	
	return Py_BuildValue("i", result);
}

static PyObject *
MoveCategory(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	int from,to, result;
	if (!PyArg_ParseTuple(args, "ii", &from, &to))
		return NULL;
	
	result = dlp_MoveCategory(self->socket->socket, self->handle, from, to);

	Dlp_CheckError(result);
	
	return Py_BuildValue("i", result);
}

static PyObject *
DeleteCategory(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	int category, result;
	if (!PyArg_ParseTuple(args, "i", &category))
		return NULL;
	
	result = dlp_DeleteCategory(self->socket->socket, self->handle, category);

	Dlp_CheckError(result);
	
	return Py_BuildValue("i", result);
}

static PyObject *
Purge(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	int result;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	
	result = dlp_CleanUpDatabase(self->socket->socket, self->handle);
	
	Dlp_CheckError(result);
	
	return Py_BuildValue("i", result);
}

static PyObject *
ResetNext(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	int result;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	
	result = dlp_ResetDBIndex(self->socket->socket, self->handle);
	
	Dlp_CheckError(result);
	
	return Py_BuildValue("i", result);
}

static PyObject *
ResetFlags(self, args)
	DlpDBObject *self;
	PyObject *args;
{
	int result;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
		
	result = dlp_ResetSyncFlags(self->socket->socket, self->handle);

	Dlp_CheckError(result);
	
	return Py_BuildValue("i", result);
}

static PyObject *
Close(self, args)
	DlpObject *self;
	PyObject *args;
{
	int status = 0;
	int result;
	if (!PyArg_ParseTuple(args, "|i", &status))
		return NULL;
	
	if (self->socket) {
		if (status) {
			result = dlp_EndOfSync(self->socket, status);
			Dlp_CheckError(result);
		}
		result = pi_close(self->socket);
		self->socket = 0;
		if (result == -1) {
			PyErr_SetFromErrno(Error);
			return NULL;
		}
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Abort(self, args)
	DlpObject *self;
	PyObject *args;
{
	int result;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	
	if (self->socket) {
		result = dlp_AbortSync(self->socket);
		Dlp_CheckError(result);
		result = pi_close(self->socket);
		self->socket = 0;
		if (result == -1) {
			PyErr_SetFromErrno(Error);
			return NULL;
		}
	}

	return Py_BuildValue("");
}

static PyObject *
GetAppPref(self, args)
	DlpObject *self;
	PyObject *args;
{
	unsigned long creator;
	int id, backup=1;
	int length, version, result;
	PyObject * ret, *p1, *p2, *f, *callargs;
	
	if (!PyArg_ParseTuple(args, "O&i|i", &ParseChar4, &creator, &id, &backup))
		return NULL;
	
	result = dlp_ReadAppPreference(self->socket, creator, id, backup,
		0xffff, self->buffer, &length, &version);
	
	Dlp_CheckError(result);
	
	if (p1 = PyDict_GetItem(PrefClasses, PyTuple_GetItem(args, 0))) {
		if (p2 = PyDict_GetItem(p1, PyTuple_GetItem(args, 1))) 
			f = p2;
		else
			if (p2 = PyDict_GetItemString(p1, ""))
				f = p2;
	} else if (p1 = PyDict_GetItemString(PrefClasses, ""))
			f = p1;

	if (!f) {
		PyErr_SetString(PyExc_ValueError, "pdapilot.PrefClasses does not contain a default");
		return NULL;
	}

	callargs = Py_BuildValue("(s#OO&iii)", self->buffer, length, self, &BuildChar4, (void*)&creator, id, version, backup, 0, 0);
	ret = PyEval_CallObject(f, callargs);
	Py_DECREF(callargs);
	return ret;
}

static PyObject *
GetAppPrefRaw(self, args)
	DlpObject *self;
	PyObject *args;
{
	unsigned long creator;
	int id, backup=1;
	int length, version, result;
	PyObject * ret, *p1, *p2, *f, *callargs;
	
	if (!PyArg_ParseTuple(args, "O&i|i", &ParseChar4, &creator, &id, &backup))
		return NULL;
	
	result = dlp_ReadAppPreference(self->socket, creator, id, backup,
		0xffff, self->buffer, &length, &version);
	
	Dlp_CheckError(result);
	
	return Py_BuildValue("s#O&iii", self->buffer, length,  &BuildChar4, (void*)&creator, id, version, backup);
}

static PyObject *
BlankAppPref(self, args)
	DlpObject *self;
	PyObject *args;
{
	unsigned long creator;
	int id;
	int length, version, result;
	PyObject * ret, *p1, *p2, *f, *callargs;
	
	if (!PyArg_ParseTuple(args, "O&i|i", &ParseChar4, &creator, &id))
		return NULL;
	
	if (p1 = PyDict_GetItem(PrefClasses, PyTuple_GetItem(args, 0))) {
		if (p2 = PyDict_GetItem(p1, PyTuple_GetItem(args, 1))) 
			f = p2;
		else
			if (p2 = PyDict_GetItemString(p1, ""))
				f = p2;
	} else if (p1 = PyDict_GetItemString(PrefClasses, ""))
			f = p1;

	if (!f) {
		PyErr_SetString(PyExc_ValueError, "pdapilot.PrefClasses does not contain a default");
		return NULL;
	}

	Py_INCREF(Py_None);
	callargs = Py_BuildValue("(OOO&i)", Py_None, self, &BuildChar4, (void*)&creator, id);
	ret = PyEval_CallObject(f, callargs);
	Py_DECREF(callargs);
	return ret;
}

static PyObject *
SetAppPref(self, args)
	DlpObject *self;
	PyObject *args;
{
	unsigned long creator;
	int id=0, length, version=0, backup=1, result;
	char * data;
	PyObject *h, *i;

	if (!PyArg_ParseTuple(args, "O", &h))
		return NULL;
		
	i = PyObject_CallMethod(h, "pack", "O", self);
	data = PyString_AsString(i);
	length = PyString_Size(i);

	if (!(i=PyObject_GetAttrString(h, "creator")) || (i == Py_None)) 
		{ PyErr_SetString(PyExc_ValueError, "The pref's .creator attribute must be set");
			return NULL; } 
	else
		creator = ParseChar4(i);
	if (i=PyObject_GetAttrString(h, "id")) id = PyInt_AsLong(i);
	if (i=PyObject_GetAttrString(h, "version")) version = PyInt_AsLong(i);
	if (i=PyObject_GetAttrString(h, "backup")) backup = PyInt_AsLong(i);
	
	result = dlp_WriteAppPreference(self->socket, creator, id, backup,
		version, data, length);
		
	Dlp_CheckError(result);

	return Py_BuildValue("i", result);
}

static PyObject *
SetAppPrefRaw(self, args)
	DlpObject *self;
	PyObject *args;
{
	unsigned long creator;
	int id=0, length, version=0, backup=1, result;
	char * data;
	PyObject *h, *i;

	if (!PyArg_ParseTuple(args, "s#O&iii", &data, &length, &ParseChar4, &creator, &id, &version, &backup))
		return NULL;
		
	result = dlp_WriteAppPreference(self->socket, creator, id, backup,
		version, data, length);
		
	Dlp_CheckError(result);

	return Py_BuildValue("i", result);
}

static PyObject *
DeleteDB(self, args)
	DlpObject *self;
	PyObject *args;
{
	char * name;
	int cardno = 0;
	int result;
	if (!PyArg_ParseTuple(args, "s|i", &name, &cardno))
		return NULL;

	result = dlp_DeleteDB(self->socket, cardno, name);
	
	Dlp_CheckError(result);

	return Py_BuildValue("i", result);
}

static PyObject *
Status(self, args)
	DlpObject *self;
	PyObject *args;
{
	int result;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	
	result = dlp_OpenConduit(self->socket);
	
	Dlp_CheckError(result);

	return Py_BuildValue("i", result);
}

static PyObject *
Battery(self, args)
	DlpObject *self;
	PyObject *args;
{
	int warn, critical, ticks, kind, AC;
	unsigned long voltage;
	int result;
	struct RPC_params p;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	PackRPC(&p,0xA0B6, RPC_IntReply,
		RPC_Byte(0), RPC_ShortPtr(&warn), RPC_ShortPtr(&critical),
		RPC_ShortPtr(&ticks), RPC_BytePtr(&kind), RPC_BytePtr(&AC), RPC_End);
	
	result = dlp_RPC(self->socket, &p, &voltage);
	
	return Py_BuildValue("(fffii)", (float)voltage/100, 
		(float)warn/100, (float)critical/100, kind, AC);
}

static PyObject *
GetTime(self, args)
	DlpObject *self;
	PyObject *args;
{
	unsigned long time;
	int result;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
		
	result = dlp_GetSysDateTime(self->socket, &time);
	
	Dlp_CheckError(result);

	return Py_BuildValue("l", (long)time);
}

static PyObject *
GetFeature(self, args)
	DlpObject *self;
	PyObject *args;
{
	unsigned long creator, feature;
	int result, number;
	if (!PyArg_ParseTuple(args, "O&i"), &ParseChar4, &creator, &number)
		return NULL;
		
	result = dlp_ReadFeature(self->socket, creator, number, &feature);
	
	Dlp_CheckError(result);

	return Py_BuildValue("l", (long)feature);
}

static PyObject *
CallApp(self, args)
	DlpObject *self;
	PyObject *args;
{
	unsigned long creator, type;
	int action, length = 0;
	char * data = 0;
	int result;
	unsigned long retcode;
	int maxlength=0xffff;
	if (!PyArg_ParseTuple(args, "O&O&i|s#l", &ParseChar4, &creator, &ParseChar4, &type, &action, &data, &length, &maxlength))
		return NULL;
	
	result = dlp_CallApplication(self->socket, creator, type, action, 
		length, data, &retcode, maxlength, &length, self->buffer);
	
	Dlp_CheckError(result);
	
	return Py_BuildValue("ls#", (long)retcode, self->buffer, length);
}

static PyObject *
Log(self, args)
	DlpObject *self;
	PyObject *args;
{
	char * string;
	int result;
	if (!PyArg_ParseTuple(args, "s", &string))
		return NULL;
	
	result = dlp_AddSyncLogEntry(self->socket, string);

	Dlp_CheckError(result);

	return Py_BuildValue("i", result);
}

static PyObject *
CardInfo(self, args)
	DlpObject *self;
	PyObject *args;
{
	int cardno=0;
	int result;
	struct CardInfo s;
	if (!PyArg_ParseTuple(args, "|i", &cardno))
		return NULL;

	result = dlp_ReadStorageInfo(self->socket, cardno, &s);
	
	Dlp_CheckError(result);
	
	return Py_BuildValue("{sisislslslslssss}",
		"cardno", s.cardno,
		"version", s.version,
		"created", (long)s.creation,
		"ROMsize", (long)s.ROMsize,
		"RAMsize", (long)s.RAMsize,
		"RAMfree", (long)s.RAMfree,
		"name", s.name,
		"manufacturer", s.manuf);
}

static PyObject *
GetUserInfo(self, args)
	DlpObject *self;
	PyObject *args;
{
	int result;
	struct PilotUser p;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	result = dlp_ReadUserInfo(self->socket, &p);
	
	Dlp_CheckError(result);
	
	return Py_BuildValue("{slslssslslslss#}",
		"userID", (long)p.userID,
		"viewerID", (long)p.viewerID,
		"name", p.username,
		"lastSyncPC", (long)p.lastSyncPC,
		"lastGoodSync", (long)p.succSyncDate,
		"lastSync", (long)p.lastSyncDate,
		"password", p.password, p.passwordLen
		);
}


static PyObject *
BuildDBInfo(i)
	struct DBInfo * i;
{
	return Py_BuildValue("{sisisisOsOsisislslslslss}",
		"more", i->more,
		"flags", i->flags,
		"miscflags", i->miscflags,
		"type", BuildChar4(&i->type),
		"creator", BuildChar4(&i->creator),
		"version", i->version,
		"index", i->index,
		"modnum", (long)i->modnum,
		"crdate", (long)i->crdate,
		"moddate", (long)i->moddate,
		"backupdate", (long)i->backupdate,
		"name", i->name
		);
}

static int ParseDBInfo(d, i)
	PyObject * d;
	struct DBInfo * i;
{
	PyObject * e;
	
	memset(i, '\0', sizeof(struct DBInfo));
	
	i->flags = (e=PyDict_GetItemString(d, "flags")) ? PyInt_AsLong(e) : 0;
	i->miscflags = (e=PyDict_GetItemString(d, "miscflags")) ? PyInt_AsLong(e) : 0;
	e=PyDict_GetItemString(d, "type");
	if (e) {
		if (ParseChar4(e, &i->type)==0)
			return 0;
	} else
		i->type = 0;
	e=PyDict_GetItemString(d, "creator");
	if (e) {
		if (ParseChar4(e, &i->creator)==0)
			return 0;
	} else
		i->creator = 0;
	i->more = (e=PyDict_GetItemString(d, "more")) ? PyInt_AsLong(e) : 0;
	i->version = (e=PyDict_GetItemString(d, "version")) ? PyInt_AsLong(e) : 0;
	i->modnum = (e=PyDict_GetItemString(d, "modnum")) ? PyInt_AsLong(e) : 0;
	i->index = (e=PyDict_GetItemString(d, "index")) ? PyInt_AsLong(e) : 0;
	i->crdate = (e=PyDict_GetItemString(d, "crdate")) ? PyInt_AsLong(e) : 0;
	i->moddate = (e=PyDict_GetItemString(d, "moddate")) ? PyInt_AsLong(e) : 0;
	i->backupdate = (e=PyDict_GetItemString(d, "backupdate")) ? PyInt_AsLong(e) : 0;
	strcpy(i->name, (e=PyDict_GetItemString(d, "name")) ? PyString_AsString(e) : "");
	
	return 1;
}
	

static PyObject *
SetUserInfo(self, args)
	DlpObject *self;
	PyObject *args;
{
	int result;
	PyObject * d, *e;
	struct PilotUser p;
	if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &d))
		return NULL;
	
	if (!PyDict_Check(d))
		return NULL;
		
	memset(&p, '\0', sizeof(struct PilotUser));
	
	p.userID = (e=PyDict_GetItemString(d, "userID")) ? PyInt_AsLong(e) : 0;
	p.viewerID = (e=PyDict_GetItemString(d, "viewerID")) ? PyInt_AsLong(e) : 0;
	p.lastSyncPC = (e=PyDict_GetItemString(d, "lastSyncPC")) ? PyInt_AsLong(e) : 0;
	p.succSyncDate = (e=PyDict_GetItemString(d, "lastGoodSync")) ? PyInt_AsLong(e) : 0;
	p.lastSyncDate = (e=PyDict_GetItemString(d, "lastSync")) ? PyInt_AsLong(e) : 0;
	strcpy(p.username, (e=PyDict_GetItemString(d, "name")) ? PyString_AsString(e) : "");

	result = dlp_WriteUserInfo(self->socket, &p);
	
	Dlp_CheckError(result);
	
	return Py_BuildValue(""); /* None */
}

static PyObject *
SysInfo(self, args)
	DlpObject *self;
	PyObject *args;
{
	int result;
	struct SysInfo s;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	result = dlp_ReadSysInfo(self->socket, &s);
	
	Dlp_CheckError(result);
	
	return Py_BuildValue("{slslss}",
		"ROMversion", (long)s.ROMVersion,
		"localizationID", (long)s.localizationID,
		"name", s.name);
}

static PyObject *
GetDBInfo(self, args)
	DlpObject *self;
	PyObject *args;
{
	int result;
	int db, where, cardno=0;
	struct DBInfo i;
	
	if (!PyArg_ParseTuple(args, "ii|i", &db, &where, &cardno))
		return NULL;

	result = dlp_ReadDBList(self->socket, cardno, where, db, &i);
	
	if (result == -5) {
		return Py_BuildValue(""); 
	}
	
	Dlp_CheckError(result);
	
	return BuildDBInfo(&i);
}

static PyObject *
FindDBInfo(self, args)
	DlpObject *self;
	PyObject *args;
{
	int result;
	int db, cardno=0;
	char * name;
	PyObject *creator, *type;
	unsigned long cl, tl;
	struct DBInfo i;
	
	if (!PyArg_ParseTuple(args, "izOO|i", &db, &name, &creator, &type, &cardno))
		return NULL;

	if (creator == Py_None)
		cl = 0;
	else
		if (ParseChar4(creator, &cl) == 0)
			return NULL;

	if (type == Py_None)
		tl = 0;
	else
		if (ParseChar4(type, &tl) == 0)
			return NULL;

	result = dlp_FindDBInfo(self->socket, cardno, db, name, tl, cl, &i);
	
	Dlp_CheckError(result);
	
	return BuildDBInfo(&i);
}

static PyObject *
SetTime(self, args)
	DlpObject *self;
	PyObject *args;
{
	unsigned long time;
	int result;
	if (!PyArg_ParseTuple(args, "l", &time))
		return NULL;
	
	result = dlp_SetSysDateTime(self->socket, time);
	
	Dlp_CheckError(result);

	return Py_BuildValue("l", (long)time);
}

static PyObject *
OpenFile(self, args)
	PyObject *self;
	PyObject *args;
{
	char * name;
	struct pi_file * pf;
	PiFileObject * retval;
	PyObject * packer;
	if (!PyArg_ParseTuple(args, "s", &name))
		return NULL;

	pf = pi_file_open(name);
	if (!pf) {
		PyErr_SetString(Error, "Unable to open file");
		return NULL;
	}
	
	retval = PyObject_NEW(PiFileObject, &PiFile_Type);
	retval->pf = pf;

	retval->Class = 0;
	packer = PyDict_GetItemString(DBClasses, name);
	if (!packer)
		packer = PyDict_GetItemString(DBClasses, "");
	if (!packer) {
		PyErr_SetString(PyExc_ValueError, "pdapilot.DBClasses must contain a default");
		pi_file_close(retval->pf);
		free(retval);
		return NULL;
	}
	if (packer) {
		retval->Class = packer;
		Py_XINCREF(packer);
	}

	return (PyObject*)retval;
}


static PyObject *
CreateFile(self, args)
	PyObject *self;
	PyObject *args;
{
	char * name;
	struct pi_file * pf;
	struct DBInfo i;
	PiFileObject * retval;
	PyObject * d, *packer;
	if (!PyArg_ParseTuple(args, "sO!", &name, &PyDict_Type, &d))
		return NULL;
	
	if (ParseDBInfo(d, &i)==0)
		return NULL;

	pf = pi_file_create(name, &i);
	if (!pf) {
		PyErr_SetString(Error, "Unable to create file");
		return NULL;
	}
	
	retval = PyObject_NEW(PiFileObject, &PiFile_Type);
	retval->pf = pf;

	retval->Class = 0;
	packer = PyDict_GetItemString(DBClasses, name);
	if (!packer)
		packer = PyDict_GetItemString(DBClasses, "");
	if (!packer) {
		PyErr_SetString(PyExc_ValueError, "pdapilot.DBClasses must contain a default");
		pi_file_close(retval->pf);
		free(retval);
		return NULL;
	}
	if (packer) {
		retval->Class = packer;
		Py_XINCREF(packer);
	}

	return (PyObject*)retval;
}

static PyObject *
FileClose(self, args)
	PiFileObject *self;
	PyObject *args;
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
		
	pi_file_close(self->pf);
	self->pf = 0;

	return Py_BuildValue("");
}

static PyObject *
FileRecords(self, args)
	PiFileObject *self;
	PyObject *args;
{
	int records;
	
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
		
	if (pi_file_get_entries(self->pf, &records)==-1)
		return Py_BuildValue("");
	else
		return Py_BuildValue("i", records);
}

static PyObject *
FileCheckID(self, args)
	PiFileObject *self;
	PyObject *args;
{
	int records;
	unsigned long id;
	
	if (!PyArg_ParseTuple(args, "l", id))
		return NULL;
		
	return Py_BuildValue("i", pi_file_id_used(self->pf, id));
}

static PyObject *
FileGetRec(self, args)
	PiFileObject *self;
	PyObject *args;
{
	int index, attr, category;
	void * c;
	int length;
	unsigned long id;
	if (!PyArg_ParseTuple(args, "i", &index))
		return NULL;
	
	if (pi_file_read_record(self->pf, index, &c, &length, &attr, &category, &id)==-1)
		return Py_BuildValue("");
	else	{
		PyObject * cl = PyObject_GetAttrString(self->Class, "Record");
		PyObject * callargs = Py_BuildValue("(s#Oilii)", c, length, self, index, (long)id, attr, category);
		PyObject * ret = PyEval_CallObject(cl, callargs);
		Py_DECREF(callargs);
		return ret;
	}
}

static PyObject *
FileGetRecById(self, args)
	PiFileObject *self;
	PyObject *args;
{
	int index, attr, category;
	void * c;
	int length;
	unsigned long id;
	if (!PyArg_ParseTuple(args, "l", &id))
		return NULL;
	
	if (pi_file_read_record_by_id(self->pf, id, &c, &length, &index, &attr, &category)==-1)
		return Py_BuildValue("");
	else {
		PyObject * cl = PyObject_GetAttrString(self->Class, "Record");
		PyObject * callargs = Py_BuildValue("(s#Oilii)", c, length, self, index, (long)id, attr, category);
		PyObject * ret = PyEval_CallObject(cl, callargs);
		Py_DECREF(callargs);
		return ret;
	}
}

static PyObject *
FileGetDBInfo(self, args)
	PiFileObject *self;
	PyObject *args;
{
	struct DBInfo i;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	
	if (pi_file_get_info(self->pf, &i)==-1) {
		PyErr_SetFromErrno(Error);
		return NULL;
	} else
		return BuildDBInfo(&i);
}

static PyObject *
FileSetDBInfo(self, args)
	PiFileObject *self;
	PyObject *args;
{
	PyObject *o;
	struct DBInfo i;
	if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &o))
		return NULL;
	
	if (ParseDBInfo(o, &i)==0)
		return NULL;
	
	if (pi_file_set_info(self->pf, &i)==-1) {
		PyErr_SetFromErrno(Error);
		return NULL;
	} else
		return Py_BuildValue("");
}



static PyObject *
FileGetAppBlock(self, args)
	PiFileObject *self;
	PyObject *args;
{
	void * c;
	int length;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	
	if (pi_file_get_app_info(self->pf, &c, &length)==-1) {
		PyErr_SetFromErrno(Error);
		return NULL;
	} else	{
		PyObject *	cl = PyObject_GetAttrString(self->Class, "AppBlock");
		PyObject * callargs = Py_BuildValue("(s#O)", c, length, self);
		PyObject * ret = PyEval_CallObject(cl, callargs);
		Py_DECREF(callargs);
		return ret;
	}
}

static PyObject *
FileSetAppBlock(self, args)
	PiFileObject *self;
	PyObject *args;
{
	char * c;
	int length;
	PyObject *h, *i;
	if (!PyArg_ParseTuple(args, "O", &h))
		return NULL;
		
	h = PyObject_CallMethod(h, "pack", "O", self);
	c = PyString_AsString(h);
	length = PyString_Size(h);

	if (pi_file_set_app_info(self->pf, c, length)==-1) {
		PyErr_SetFromErrno(Error);
		return NULL;
	} else
		return Py_BuildValue("");
}

static PyObject *
FileGetSortBlock(self, args)
	PiFileObject *self;
	PyObject *args;
{
	void * c;
	int length;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	
	if (pi_file_get_sort_info(self->pf, &c, &length)==-1) {
		PyErr_SetFromErrno(Error);
		return NULL;
	} else {
		PyObject *	cl = PyObject_GetAttrString(self->Class, "SortBlock");
		PyObject * callargs = Py_BuildValue("(s#O)", c, length, self);
		PyObject * ret = PyEval_CallObject(cl, callargs);
		Py_DECREF(callargs);
		return ret;
	}
}

static PyObject *
FileSetSortBlock(self, args)
	PiFileObject *self;
	PyObject *args;
{
	char * c;
	int length;
	PyObject *h, *i;
	if (!PyArg_ParseTuple(args, "O", &h))
		return NULL;
		
	h = PyObject_CallMethod(h, "pack", "O", self);
	c = PyString_AsString(h);
	length = PyString_Size(h);
	
	if (pi_file_set_sort_info(self->pf, c, length)==-1) {
		PyErr_SetFromErrno(Error);
		return NULL;
	} else
		return Py_BuildValue("");
}

static PyObject *
FileGetRsc(self, args)
	PiFileObject *self;
	PyObject *args;
{
	int index, id;
	void * c;
	int length;
	unsigned long type;
	if (!PyArg_ParseTuple(args, "i", &index))
		return NULL;
	
	if (pi_file_read_resource(self->pf, index, &c, &length, &type, &id)==-1)
		return Py_BuildValue("");
	else {
		PyObject * cl = PyObject_GetAttrString(self->Class, "Resource");
		PyObject * callargs = Py_BuildValue("(s#OO&i)", c, length, self, &BuildChar4, &type, id);
		PyObject * ret = PyEval_CallObject(cl, callargs);
		Py_DECREF(callargs);
		return ret;
	}
}

static PyObject *
FileAddRec(self, args)
	PiFileObject *self;
	PyObject *args;
{
	unsigned long id=0;
	unsigned long newid;
	int length, attr=0, category=0;
	char * data;
	int result;
	PyObject *h, *i;
	if (!PyArg_ParseTuple(args, "O", &h))
		return NULL;
		
	i = PyObject_CallMethod(h, "pack", "O", self);
	data = PyString_AsString(i);
	length = PyString_Size(i);

	if (!(i=PyObject_GetAttrString(h, "id")))
		{ PyErr_SetString(PyExc_ValueError, "The record's .id attribute must be set");
			return NULL; } 
	else if (i == Py_None) 
		id = 0;
	else
		id = PyInt_AsLong(i);
	if (i=PyObject_GetAttrString(h, "attr")) attr = PyInt_AsLong(i);
	if (i=PyObject_GetAttrString(h, "category")) category = PyInt_AsLong(i);
	
	if (pi_file_append_record(self->pf, data, length, attr, category, id)==-1) {
		PyErr_SetFromErrno(Error);
		return NULL;
	} 
	
	return Py_BuildValue("l", (long)id);
}

static PyObject *
FileAddRsc(self, args)
	PiFileObject *self;
	PyObject *args;
{
	unsigned long type;
	int id, length;
	char * data;
	int result;
	PyObject *h, *i;
	if (!PyArg_ParseTuple(args, "O", &h))
		return NULL;
		
	i = PyObject_CallMethod(h, "pack", "O", self);
	data = PyString_AsString(i);
	length = PyString_Size(i);

	if (!(i=PyObject_GetAttrString(h, "id")) || (i == Py_None))
		{ PyErr_SetString(PyExc_ValueError, "The resource's .id attribute must be set"); 
			return NULL; } 
	else
		id = PyInt_AsLong(i);
	if (!(i=PyObject_GetAttrString(h, "type")) || (i == Py_None))
		{ PyErr_SetString(PyExc_ValueError, "The resource's .type attribute must be set"); 
			return NULL; } 
	else
		type = ParseChar4(i);
	
	if (pi_file_append_resource(self->pf, data, length, type, id)==-1) {
		PyErr_SetFromErrno(Error);
		return NULL;
	}

	return Py_BuildValue("");
}

static PyObject *
FileInstall(self, args)
	PiFileObject *self;
	PyObject *args;
{
	DlpObject *socket;
	int result;
	int cardno=0;
	if (!PyArg_ParseTuple(args, "O!|i", &Dlp_Type, &socket, &cardno))
		return NULL;
	
	if (pi_file_install(self->pf, socket->socket, cardno)==-1) {
		PyErr_SetFromErrno(Error);
		return NULL;
	}

	return Py_BuildValue("");
}

static PyObject *
FileRetrieve(self, args)
	PiFileObject *self;
	PyObject *args;
{
	DlpObject *socket;
	int result;
	int cardno=0;
	if (!PyArg_ParseTuple(args, "O!|i", &Dlp_Type, &socket, &cardno))
		return NULL;
	
	if (pi_file_retrieve(self->pf, socket->socket, cardno)==-1) {
		PyErr_SetFromErrno(Error);
		return NULL;
	}

	return Py_BuildValue("");
}


static PyObject *
FileBlankAppBlock(self, args)
	PiFileObject *self;
	PyObject *args;
{
	PyObject *ret;
	PyObject *c, *callargs;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	
	c = PyObject_GetAttrString(self->Class, "AppBlock");
	callargs = Py_BuildValue("()");
	ret = PyEval_CallObject(c, callargs);
	Py_DECREF(callargs);
	return ret;
}

static PyObject *
FileBlankSortBlock(self, args)
	PiFileObject *self;
	PyObject *args;
{
	PyObject *ret;
	PyObject *c, *callargs;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	
	c = PyObject_GetAttrString(self->Class, "SortBlock");
	callargs = Py_BuildValue("()");
	ret = PyEval_CallObject(c, callargs);
	Py_DECREF(callargs);
	return ret;
}

static PyObject *
FileBlankRecord(self, args)
	PiFileObject *self;
	PyObject *args;
{
	PyObject *ret;
	PyObject *c, *callargs;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	
	c = PyObject_GetAttrString(self->Class, "Record");
	callargs = Py_BuildValue("()");
	ret = PyEval_CallObject(c, callargs);
	Py_DECREF(callargs);
	return ret;
}

static PyObject *
FileBlankResource(self, args)
	PiFileObject *self;
	PyObject *args;
{
	PyObject *ret;
	PyObject *c, *callargs;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	
	c = PyObject_GetAttrString(self->Class, "Resource");
	callargs = Py_BuildValue("()");
	ret = PyEval_CallObject(c, callargs);
	Py_DECREF(callargs);
	return ret;
}

static PyObject *
MemoUnpack(self, args)
	PyObject *self;
	PyObject *args;
{
	struct Memo m;

	char * data;
	int length;
	PyObject * result, *dict;
	
	if (!PyArg_ParseTuple(args, "Os#", &dict, &data, &length))
		return NULL;
	
	unpack_Memo(&m, data, length);
	
	PyDict_SetItemString(dict, "text", PyString_FromString(m.text));
	
	free_Memo(&m);
	
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
MemoPack(self, args)
	PyObject *self;
	PyObject *args;
{
	char * data;
	int length;
	struct Memo m;
	PyObject *dict, *result, *e;
	
	if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &dict))
		return NULL;
	
	data = malloc(0xffff);
	memset(&m, 0, sizeof(m));
	
	m.text = (e=PyDict_GetItemString(dict, "text")) ? PyString_AsString(e) : 0;
	
	pack_Memo(&m, data, &length);
	
	result = Py_BuildValue("s#", data, length);
	
	free(data);
	return result;
}

static PyObject *
MemoUnpackAppBlock(self, args)
	PyObject *self;
	PyObject *args;
{
	char * data;
	int length;
	PyObject *names, *ids, *h, *raw, *dict;
	struct MemoAppInfo m;
	int i;
	
	if (!PyArg_ParseTuple(args, "Os#", &dict, &data, &length))
		return NULL;
	
	unpack_MemoAppInfo(&m, data, length);
	
	names = PyList_New(16);
	for (i=0;i<16;i++)
		PyList_SetItem(names, i, PyString_FromString(m.CategoryName[i]));

	ids = PyList_New(16);
	for (i=0;i<16;i++)
		PyList_SetItem(ids, i, PyInt_FromLong(m.CategoryID[i]));	

	PyDict_SetItemString(dict, "categoryname", names);
	PyDict_SetItemString(dict, "categoryID", ids);
	PyDict_SetItemString(dict, "lastID", PyInt_FromLong(m.lastUniqueID));
	PyDict_SetItemString(dict, "sortOrder", PyInt_FromLong(m.sortOrder));
	
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
MemoPackAppBlock(self, args)
	PyObject *self;
	PyObject *args;
{
	char * data;
	int length;
	PyObject *names, *ids;
	struct MemoAppInfo m;
	int i;
	PyObject *dict, *e, *c, *result;

	if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &dict))
		return NULL;
	
	data = calloc(0xFFFF, 1);
	memset(&m, 0, sizeof(m));
	
	i = 0;
	if (e=PyDict_GetItemString(dict, "categoryname")) {
		for(i=0;i<16;i++) {
			c = PyList_GetItem(e, i);
			if (c)
				strcpy(m.CategoryName[i], PyString_AsString(c));
			else
				break;
		}
	}
	for(;i<16;i++)
		m.CategoryName[i][0] = '\0';

	i = 0;
	if (e=PyDict_GetItemString(dict, "categoryID")) {
		for(i=0;i<16;i++) {
			c = PyList_GetItem(e, i);
			if (c)
				m.CategoryID[i] = PyInt_AsLong(c);
			else
				break;
		}
	}
	for(;i<16;i++)
		m.CategoryID[i] = 0;

	m.sortOrder = (e=PyDict_GetItemString(dict, "sortOrder")) ? PyInt_AsLong(e) : 0;
	m.lastUniqueID = (e=PyDict_GetItemString(dict, "lastID")) ? PyInt_AsLong(e) : 0;
	
	pack_MemoAppInfo(&m, data, &length);
	
	result = Py_BuildValue("s#", data, length);
	free(data);
	
	fprintf(stderr,"Returning from MemoPackAppBlock\n");
	return result;
}

static PyObject *
TodoUnpack(self, args)
	PyObject *self;
	PyObject *args;
{
	char * data;
	int length;
	struct ToDo m;
	PyObject * result, *dict;
	
	if (!PyArg_ParseTuple(args, "Os#", &dict, &data, &length))
		return NULL;
	
	unpack_ToDo(&m, data, length);
	
	PyDict_SetItemString(dict, "due", m.indefinite ? Py_BuildValue("") : BuildTm(&m.due));
	PyDict_SetItemString(dict, "priority", PyInt_FromLong(m.priority));
	PyDict_SetItemString(dict, "complete", PyInt_FromLong(m.complete));
	PyDict_SetItemString(dict, "description", m.description ? PyString_FromString(m.description) : Py_BuildValue(""));
	PyDict_SetItemString(dict, "description", m.note ? PyString_FromString(m.note) : Py_BuildValue(""));
	
	/*result = Py_BuildValue("{sOsisiszsz}", 
		"due", ,
		"priority",m.priority,
		"complete",m.complete,
		"description", m.description,
		"note", m.note
		);*/
	
	free_ToDo(&m);
	
	return Py_BuildValue("");
}

static PyObject *
TodoPack(self, args)
	PyObject *self;
	PyObject *args;
{
	char * data;
	int length;
	struct ToDo m;
	PyObject * dict, *e, *o;
	
	if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &dict))
		return NULL;
	
	data = malloc(0xffff);

	m.description = (e=PyDict_GetItemString(o, "description")) ? PyString_AsString(e) : 0;
	m.note = (e=PyDict_GetItemString(o, "note")) ? PyString_AsString(e) : 0;
	m.complete = (e=PyDict_GetItemString(o, "complete")) ? PyInt_AsLong(e) : 0;
	m.priority = (e=PyDict_GetItemString(o, "priority")) ? PyInt_AsLong(e) : 0;
	m.indefinite = 1;
	memset(&m.due, '\0', sizeof(struct tm));
	if (e=PyDict_GetItemString(o, "due"))
		if (PyTuple_Check(e)) {
			ParseTm(e, &m.due);
			m.indefinite=0;
		}
	
	pack_ToDo(&m, data, &length);
	
	o = Py_BuildValue("s#", data, length);
	free(data);
	
	return o;
}

static PyObject *
TodoUnpackAppBlock(self, args)
	PyObject *self;
	PyObject *args;
{
	char * data;
	int length;
	PyObject *names, *ids, *dict;
	struct ToDoAppInfo m;
	int i;
	
	if (!PyArg_ParseTuple(args, "Os#", &dict, &data, &length))
		return NULL;
	
	unpack_ToDoAppInfo(&m, data, length);
	
	names = PyList_New(16);
	for (i=0;i<16;i++)
		PyList_SetItem(names, i, PyString_FromString(m.CategoryName[i]));

	ids = PyList_New(16);
	for (i=0;i<16;i++)
		PyList_SetItem(ids, i, PyInt_FromLong(m.CategoryID[i]));	
	
	PyDict_SetItemString(dict, "categoryname", names);
	PyDict_SetItemString(dict, "categoryID", ids);
	PyDict_SetItemString(dict, "lastID", PyInt_FromLong(m.lastUniqueID));
	PyDict_SetItemString(dict, "sortByPriority", PyInt_FromLong(m.sortByPriority));
	PyDict_SetItemString(dict, "dirty", PyInt_FromLong(m.dirty));

	/*return Py_BuildValue("{sOsOsisisi}", 
		"category", names,
		"categoryID", ids,
		"lastID",	m.lastUniqueID,
		"sortByPriority",	m.sortByPriority,
		"dirty",	m.dirty
		 );*/
	
	return Py_BuildValue("");
}

static PyObject *
TodoPackAppBlock(self, args)
	PyObject *self;
	PyObject *args;
{
	char * data;
	int length;
	PyObject *names, *ids;
	struct ToDoAppInfo m;
	int i;
	PyObject *result, *o, *e, *c;
	
	if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &o))
		return NULL;
	
	data = malloc(0xFFFF);
	
	i = 0;
	if (e=PyDict_GetItemString(o, "categoryname")) {
		for(i=0;i<16;i++) {
			c = PyList_GetItem(e, i);
			if (c)
				strcpy(m.CategoryName[i], PyString_AsString(c));
			else
				break;
		}
	}
	for(;i<16;i++)
		m.CategoryName[i][0] = '\0';

	i = 0;
	if (e=PyDict_GetItemString(o, "categoryID")) {
		for(i=0;i<16;i++) {
			c = PyList_GetItem(e, i);
			if (c)
				m.CategoryID[i] = PyInt_AsLong(c);
			else
				break;
		}
	}
	for(;i<16;i++)
		m.CategoryID[i] = 0;

	m.sortByPriority = (e=PyDict_GetItemString(o, "sortByPriority")) ? PyInt_AsLong(e) : 0;
	m.dirty = (e=PyDict_GetItemString(o, "dirty")) ? PyInt_AsLong(e) : 0;
	m.lastUniqueID = (e=PyDict_GetItemString(o, "lastID")) ? PyInt_AsLong(e) : 0;
	
	pack_ToDoAppInfo(&m, data, &length);
	
	result = Py_BuildValue("s#", data, length);
	
	free(data);
	return result;
}

static PyObject *
MailUnpackPref(self, args)
	PyObject *self;
	PyObject *args;
{

	char * data;
	int length;
	int id;
	PyObject * result, *dict;
	
	if (!PyArg_ParseTuple(args, "Os#i", &dict, &data, &length, &id))
		return NULL;
		
	if (id == 1) {
		struct MailPrefs m;
		unpack_MailPrefs(&m, data, length);
		
		PyDict_SetItemString(dict, "synctype", PyInt_FromLong(m.synctype));
		PyDict_SetItemString(dict, "gethigh", PyInt_FromLong(m.gethigh));
		PyDict_SetItemString(dict, "getcontaining", PyInt_FromLong(m.getcontaining));
		PyDict_SetItemString(dict, "truncate", PyInt_FromLong(m.truncate));
		PyDict_SetItemString(dict, "filterto", Py_BuildValue("z", m.filterto));
		PyDict_SetItemString(dict, "filterfrom", Py_BuildValue("z", m.filterfrom));
		PyDict_SetItemString(dict, "filtersubject", Py_BuildValue("z", m.filtersubject));
	
		free_MailPrefs(&m);
	} else if (id == 3) {
		PyDict_SetItemString(dict, "signature", PyString_FromString(data));
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
MailPackPref(self, args)
	PyObject *self;
	PyObject *args;
{
	char * data;
	int length;
	int id;
	PyObject *dict, *result, *e;
	
	if (!PyArg_ParseTuple(args, "O!i", &PyDict_Type, &dict, &id))
		return NULL;
		
	data = 0;
	length = 0;
	
	if (id == 3) {
		e = PyDict_GetItemString(dict, "signature");
		if (e) 
			length = PyString_Size(e); 
		data = malloc(length+1);
		if (length)
			memcpy(data, PyString_AsString(e), length);
		data[length] = 0;
		length++;
	}
	/*else if (id == 1) {  	Oops, I never wrote pack_MailPrefs...
		struct MailPrefs m;
	
		data = malloc(0xffff);
		memset(&m, 0, sizeof(m));
	
		m.synctype = PyInt_AsLong(PyDict_GetItemString(dict, "synctype"));
		m.gethigh = PyInt_AsLong(PyDict_GetItemString(dict, "gethigh"));
		m.getcontaining = PyInt_AsLong(PyDict_GetItemString(dict, "getcontaining"));
		m.truncate = PyInt_AsLong(PyDict_GetItemString(dict, "truncate"));
		m.filterto = PyString_AsString(PyDict_GetItemString(dict, "filterto"));
		m.filterfrom = PyString_AsString(PyDict_GetItemString(dict, "filterfrom"));
		m.filtersubject = PyString_AsString(PyDict_GetItemString(dict, "filtersubject"));
	
		pack_MailPrefs(&m, data, &length);
	}*/
	else {
		e = PyDict_GetItemString(dict, "raw");
		Py_XINCREF(e);
		return e;
	}
	
	
	result = Py_BuildValue("s#", data, length);
	
	if (data)	
		free(data);

	return result;
}


static PyObject *
RPCPack(self, args)
	PyObject *self;
	PyObject *args;
{
	long trap;
	char * reply;
	int r;
	PyObject *rpcargs, *rpctypes;
	RpcObject * result;
	int i;
	
	if (!PyArg_ParseTuple(args, "lzO!O!", &trap, &reply, &PyTuple_Type, &rpctypes, &PyTuple_Type, &rpcargs))
		return NULL;


	if (PyTuple_Size(rpcargs) != PyTuple_Size(rpctypes)) {
		PyErr_SetString(Error, "types and arguments must match");
		return NULL;
	}

	for(i=0;i<PyList_Size(rpcargs);i++) {
		PyObject * type = PyTuple_GetItem(rpctypes, i);
		PyObject * value = PyTuple_GetItem(rpcargs, i);
		if (!PyString_Check(type)) {
			PyErr_SetString(Error, "type must be string");
			return NULL;
		} else if (!PyInt_Check(value) && !PyString_Check(value)) {
			PyErr_SetString(Error, "argument must be string or integer");
			return NULL;
		}
	}
	
	result = PyObject_NEW(RpcObject, &Rpc_Type);
	result->p = malloc(sizeof(struct RPC_params));
	/*result->rpcargs = rpcargs;
	Py_INCREF(rpcargs);*/

	result->p->trap = trap;
	
	if ((reply == 0) || (strlen(reply)==0))
		r = RPC_NoReply;
	else
		switch (reply[0]) {
			case 'i': case 'l': case 's': case 'b': case 'c':
				r = RPC_IntReply;
				break;
			case 'p': case 'h': case '*': case '&':
				r = RPC_PtrReply;
				break;
			default:
				r = RPC_NoReply;
		}
		
	result->p->reply = r;
	for(i=0;i<PyTuple_Size(rpcargs);i++) {
		char * type = PyString_AsString(PyTuple_GetItem(rpctypes, i));
		PyObject * value = PyTuple_GetItem(rpcargs, i);
		char * data = 0;
		int len = 0;
		unsigned long arg;
		int ref=0;
		if (type[0] == '&') {
			result->p->param[i].byRef = 1;	
			type++;
		} else 
			result->p->param[i].byRef = 0;	
		result->p->param[i].invert = 0;
		result->p->param[i].data = &result->p->param[i].arg;

		switch(type[0]) {
		case '*': case 'p':
			result->p->param[i].data = malloc(PyString_Size(value)+1);
			memcpy(result->p->param[i].data, PyString_AsString(value), PyString_Size(value)+1);
			result->p->param[i].size = PyString_Size(value);
			result->p->param[i].invert = 0;
			break;
		case 'b': case 'c':
			result->p->param[i].arg = PyInt_AsLong(value);
			result->p->param[i].size = 2;
			result->p->param[i].invert = 2;
			break;
		case 's':
			result->p->param[i].arg = PyInt_AsLong(value);
			result->p->param[i].size = 2;
			result->p->param[i].invert = 1;
			break;
		case 'i': case 'l': 
			result->p->param[i].arg = PyInt_AsLong(value);
			result->p->param[i].size = 4;
			result->p->param[i].invert = 1;
			break;
		}
	}
	result->p->args = i;
	
	return (PyObject*)result;
}

static PyObject *
DlpRPC(self, args)
	DlpObject *self;
	PyObject *args;
{
	long trap;
	char * reply;
	int r;
	RpcObject * rpc;
	int i;
	int err;
	long result;
	PyObject * out;
	
	if (!PyArg_ParseTuple(args, "O!", &Rpc_Type, &rpc))
		return NULL;
	
	err = dlp_RPC(self->socket, rpc->p, &result);

	out = PyTuple_New(rpc->p->args);
	
	for(i=0;i<rpc->p->args;i++) {
		struct RPC_param * p = &rpc->p->param[i];
		if (p->invert == 0) {
			PyTuple_SetItem(out, i, PyString_FromStringAndSize(p->data,p->size));
		} else {
			PyTuple_SetItem(out, i, PyInt_FromLong(p->arg));
		}
	}
	
	return Py_BuildValue("(liO)", result, err, out);
}

static PyMethodDef PiFile_methods[] = {
	{"Records",	FileRecords, 1},
	{"CheckID",	FileCheckID, 1},
	{"GetRecord",	FileGetRec, 1},
	{"GetRecordByID",	FileGetRecById, 1},
	{"GetResource",	FileGetRsc, 1},
	{"AddRecord",	FileAddRec, 1},
	{"AddResource",	FileAddRsc, 1},
	{"GetAppBlock",	FileGetAppBlock, 1},
	{"SetAppBlock",	FileGetAppBlock, 1},
	{"GetSortBlock",FileGetSortBlock, 1},
	{"SetSortBlock",FileSetSortBlock, 1},
	{"GetDBInfo",	FileGetDBInfo, 1},
	{"SetDBInfo",	FileSetDBInfo, 1},
	{"Install",	FileInstall, 1},
	{"Retrieve",	FileRetrieve, 1},
	{"Close",	FileClose, 1},
{"NewAppBlock", FileBlankAppBlock, 1},
{"NewSortBlock", FileBlankSortBlock, 1},
{"NewRecord", FileBlankRecord, 1},
{"NewResource", FileBlankResource, 1},
	{NULL,	NULL}
};

static PyMethodDef Dlp_methods[] = {
	{"Open",	OpenDB, 1},
	{"Create",	CreateDB, 1},
	{"Delete",	DeleteDB, 1},
	{"Status",	Status, 1},
	{"Dirty",	Dirty, 1},
	{"Battery",	Battery, 1},
	{"Reset",	ResetSystem, 1},
	{"Close",	Close, 1},
	{"Abort",	Abort, 1},
	{"Log",		Log, 1},
	{"GetTime",	GetTime, 1},
	{"SetTime",	GetTime, 1},
	{"CardInfo",	CardInfo, 1},
	{"SysInfo",	SysInfo, 1},
	{"GetUserInfo",	GetUserInfo, 1},
	{"SetUserInfo",	SetUserInfo, 1},
	{"GetAppPref",	GetAppPref, 1},
	{"NewAppPref",	BlankAppPref, 1},
	{"SetAppPref",	SetAppPref, 1},
	{"GetAppPrefRaw",	GetAppPrefRaw, 1},
	{"SetAppPrefRaw",	SetAppPrefRaw, 1},
	{"GetFeature",	GetFeature, 1},
	{"GetDBInfo",	GetDBInfo, 1},
	{"FindDBInfo",	FindDBInfo, 1},
	{"Call",	CallApp, 1},
	{"RPC",		DlpRPC, 1},
	{NULL,	NULL}
};

static PyMethodDef DlpDB_methods[] = {
	{"SetRecord",	SetRec, 1},
	{"SetResource",	SetRsc, 1},
	{"GetNextInCategory",	NextCatRec, 1},
	{"GetNextChanged",	NextModRec, 1},
	{"GetResource",	GetRsc, 1},
	{"GetResourceByID",	GetRscById, 1},
	{"GetRecord",	GetRec, 1},
	{"GetRecordByID",	GetRecById, 1},
	{"DeleteRecord",	DeleteRec, 1},
	{"DeleteRecords",	DeleteAllRec, 1},
	{"DeleteResource",	DeleteRsc, 1},
	{"DeleteResources",	DeleteAllRsc, 1},
	{"Close",	CloseDB, 1},
	{"Records",	Records, 1},
	{"RecordIDs",	RecordIDs, 1},
	{"GetAppBlock",	GetAppBlock, 1},
	{"SetAppBlock",	SetAppBlock, 1},
	{"GetSortBlock",GetSortBlock, 1},
	{"SetSortBlock",SetSortBlock, 1},
	{"MoveCategory",MoveCategory, 1},
	{"DeleteCategory",DeleteCategory, 1},
	{"Purge",	Purge, 1},
	{"ResetNext",	ResetNext, 1},
	{"ResetFlags",	ResetFlags, 1},
{"NewAppBlock", BlankAppBlock, 1},
{"NewSortBlock", BlankSortBlock, 1},
{"NewRecord", BlankRecord, 1},
{"NewResource", BlankResource, 1},
	{NULL,	NULL}
};



static PyMethodDef Methods[] = {
	{"Socket",	Socket, 1},
	{"Bind",	Bind, 1},
	{"Read",	Read, 1},
	{"Write",	Write, 1},
	{"Accept",	Accept, 1},
	{"Close",	CloseSocket, 1},
	{"Listen",	Listen, 1},
	{"FileOpen",	OpenFile, 1},
	{"FileCreate",	CreateFile, 1},
	{"OpenPort",	OpenPort, 1},
	{"MemoUnpack",	MemoUnpack, 1},
	{"MemoUnpackAppBlock",	MemoUnpackAppBlock, 1},
	{"MemoPack",	MemoPack, 1},
	{"MemoPackAppBlock",	MemoPackAppBlock, 1},
	{"ToDoUnpack",	TodoUnpack, 1},
	{"ToDoUnpackAppBlock",	TodoUnpackAppBlock, 1},
	{"ToDoPack",	TodoPack, 1},
	{"ToDoPackAppBlock",	TodoPackAppBlock, 1},
	{"MailUnpackPref",	MailUnpackPref, 1},
	{"MailPackPref",	MailPackPref, 1},
	{"PackRPC",	RPCPack, 1},
	{NULL, NULL}
};

static PyMethodDef MemoAppBlock_methods[] = {
	{"unpack",	MemoUnpackAppBlock, 1},
	{NULL, NULL}
};


#define SetDictInt(string,ch) \
        PyDict_SetItemString(d,string,PyInt_FromLong((long) (ch)));

void
init_pdapilot()
{
	PyObject *m, *d, *main, *t;
	main = m = Py_InitModule("_pdapilot", Methods);
	d = PyModule_GetDict(m);
	Error = PyString_FromString("pdapilot.error");
	PyDict_SetItemString(d, "error", Error);
	
	DBClasses = PyDict_New();
	PyDict_SetItemString(d, "DBClasses", DBClasses);

	PrefClasses = PyDict_New();
	PyDict_SetItemString(d, "PrefClasses", PrefClasses);

	SetDictInt("PI_AF_SLP", PI_AF_SLP);

	SetDictInt("PI_PF_SLP", PI_PF_SLP);
	SetDictInt("PI_PF_PADP", PI_PF_PADP);
	SetDictInt("PI_PF_LOOP", PI_PF_LOOP);

	SetDictInt("PI_SOCK_STREAM", PI_SOCK_STREAM);
	SetDictInt("PI_SOCK_DGRAM", PI_SOCK_DGRAM);
	SetDictInt("PI_SOCK_RAW", PI_SOCK_RAW);
	SetDictInt("PI_SOCK_SEQPACKET", PI_SOCK_SEQPACKET);

	SetDictInt("PI_PilotSocketDLP", PI_PilotSocketDLP);
	SetDictInt("PI_PilotSocketConsole", PI_PilotSocketConsole);
	SetDictInt("PI_PilotSocketDebugger", PI_PilotSocketDebugger);
	SetDictInt("PI_PilotSockerRemoteUI", PI_PilotSocketRemoteUI);	

	SetDictInt("DBResource", dlpDBFlagResource);
	SetDictInt("DBReadOnly", dlpDBFlagReadOnly);
	SetDictInt("DBAppBlockDirty", dlpDBFlagAppInfoDirty);
	SetDictInt("DBBackup", dlpDBFlagBackup);
	SetDictInt("DBOpen", dlpDBFlagOpen);
	SetDictInt("DBNew", dlpDBFlagNewer);
	SetDictInt("DBReset", dlpDBFlagReset);
	
	SetDictInt("OpenDBRead", dlpOpenRead);
	SetDictInt("OpenDBWrite", dlpOpenWrite);
	SetDictInt("OpenDBReadWrite", dlpOpenReadWrite);
	SetDictInt("OpenDBExclusive", dlpOpenExclusive);
	SetDictInt("OpenDBSecret", dlpOpenSecret);

	SetDictInt("EndNormal", dlpEndCodeNormal);
	SetDictInt("EndMemory", dlpEndCodeOutOfMemory);
	SetDictInt("EndCancelled", dlpEndCodeUserCan);
	SetDictInt("EndOther", dlpEndCodeOther);
	
	SetDictInt("RecDeleted", dlpRecAttrDeleted);
	SetDictInt("RecDirty", dlpRecAttrDirty);
	SetDictInt("RecBusy", dlpRecAttrBusy);
	SetDictInt("RecSecret", dlpRecAttrSecret);
	SetDictInt("RecArchived", dlpRecAttrArchived);

}
