# This file was created automatically by SWIG 1.3.27.
# Don't modify this file, modify the SWIG interface instead.

import _pisock

# This file is compatible with both classic and new-style classes.
def _swig_setattr_nondynamic(self,class_type,name,value,static=1):
    if (name == "this"):
        if isinstance(value, class_type):
            self.__dict__[name] = value.this
            if hasattr(value,"thisown"): self.__dict__["thisown"] = value.thisown
            del value.thisown
            return
    method = class_type.__swig_setmethods__.get(name,None)
    if method: return method(self,value)
    if (not static) or hasattr(self,name) or (name == "thisown"):
        self.__dict__[name] = value
    else:
        raise AttributeError("You cannot add attributes to %s" % self)

def _swig_setattr(self,class_type,name,value):
    return _swig_setattr_nondynamic(self,class_type,name,value,0)

def _swig_getattr(self,class_type,name):
    method = class_type.__swig_getmethods__.get(name,None)
    if method: return method(self)
    raise AttributeError,name

import types
try:
    _object = types.ObjectType
    _newclass = 1
except AttributeError:
    class _object : pass
    _newclass = 0
del types


from pisockextras import *

error = _pisock.error 

PI_ERR_PROT_ABORTED = _pisock.PI_ERR_PROT_ABORTED
PI_ERR_PROT_INCOMPATIBLE = _pisock.PI_ERR_PROT_INCOMPATIBLE
PI_ERR_PROT_BADPACKET = _pisock.PI_ERR_PROT_BADPACKET
PI_ERR_SOCK_DISCONNECTED = _pisock.PI_ERR_SOCK_DISCONNECTED
PI_ERR_SOCK_INVALID = _pisock.PI_ERR_SOCK_INVALID
PI_ERR_SOCK_TIMEOUT = _pisock.PI_ERR_SOCK_TIMEOUT
PI_ERR_SOCK_CANCELED = _pisock.PI_ERR_SOCK_CANCELED
PI_ERR_SOCK_IO = _pisock.PI_ERR_SOCK_IO
PI_ERR_SOCK_LISTENER = _pisock.PI_ERR_SOCK_LISTENER
PI_ERR_DLP_BUFSIZE = _pisock.PI_ERR_DLP_BUFSIZE
PI_ERR_DLP_PALMOS = _pisock.PI_ERR_DLP_PALMOS
PI_ERR_DLP_UNSUPPORTED = _pisock.PI_ERR_DLP_UNSUPPORTED
PI_ERR_DLP_SOCKET = _pisock.PI_ERR_DLP_SOCKET
PI_ERR_DLP_DATASIZE = _pisock.PI_ERR_DLP_DATASIZE
PI_ERR_DLP_COMMAND = _pisock.PI_ERR_DLP_COMMAND
PI_ERR_FILE_INVALID = _pisock.PI_ERR_FILE_INVALID
PI_ERR_FILE_ERROR = _pisock.PI_ERR_FILE_ERROR
PI_ERR_FILE_ABORTED = _pisock.PI_ERR_FILE_ABORTED
PI_ERR_FILE_NOT_FOUND = _pisock.PI_ERR_FILE_NOT_FOUND
PI_ERR_FILE_ALREADY_EXISTS = _pisock.PI_ERR_FILE_ALREADY_EXISTS
PI_ERR_GENERIC_MEMORY = _pisock.PI_ERR_GENERIC_MEMORY
PI_ERR_GENERIC_ARGUMENT = _pisock.PI_ERR_GENERIC_ARGUMENT
PI_ERR_GENERIC_SYSTEM = _pisock.PI_ERR_GENERIC_SYSTEM
PI_AF_PILOT = _pisock.PI_AF_PILOT
PI_SOCK_STREAM = _pisock.PI_SOCK_STREAM
PI_SOCK_RAW = _pisock.PI_SOCK_RAW
PI_CMD_CMP = _pisock.PI_CMD_CMP
PI_CMD_NET = _pisock.PI_CMD_NET
PI_CMD_SYS = _pisock.PI_CMD_SYS
PI_MSG_PEEK = _pisock.PI_MSG_PEEK
PI_PF_DEV = _pisock.PI_PF_DEV
PI_PF_SLP = _pisock.PI_PF_SLP
PI_PF_SYS = _pisock.PI_PF_SYS
PI_PF_PADP = _pisock.PI_PF_PADP
PI_PF_NET = _pisock.PI_PF_NET
PI_PF_DLP = _pisock.PI_PF_DLP
PI_LEVEL_DEV = _pisock.PI_LEVEL_DEV
PI_LEVEL_SLP = _pisock.PI_LEVEL_SLP
PI_LEVEL_PADP = _pisock.PI_LEVEL_PADP
PI_LEVEL_NET = _pisock.PI_LEVEL_NET
PI_LEVEL_SYS = _pisock.PI_LEVEL_SYS
PI_LEVEL_CMP = _pisock.PI_LEVEL_CMP
PI_LEVEL_DLP = _pisock.PI_LEVEL_DLP
PI_LEVEL_SOCK = _pisock.PI_LEVEL_SOCK
PI_DEV_RATE = _pisock.PI_DEV_RATE
PI_DEV_ESTRATE = _pisock.PI_DEV_ESTRATE
PI_DEV_HIGHRATE = _pisock.PI_DEV_HIGHRATE
PI_DEV_TIMEOUT = _pisock.PI_DEV_TIMEOUT
PI_SLP_DEST = _pisock.PI_SLP_DEST
PI_SLP_LASTDEST = _pisock.PI_SLP_LASTDEST
PI_SLP_SRC = _pisock.PI_SLP_SRC
PI_SLP_LASTSRC = _pisock.PI_SLP_LASTSRC
PI_SLP_TYPE = _pisock.PI_SLP_TYPE
PI_SLP_LASTTYPE = _pisock.PI_SLP_LASTTYPE
PI_SLP_TXID = _pisock.PI_SLP_TXID
PI_SLP_LASTTXID = _pisock.PI_SLP_LASTTXID
PI_PADP_TYPE = _pisock.PI_PADP_TYPE
PI_PADP_LASTTYPE = _pisock.PI_PADP_LASTTYPE
PI_PADP_FREEZE_TXID = _pisock.PI_PADP_FREEZE_TXID
PI_PADP_USE_LONG_FORMAT = _pisock.PI_PADP_USE_LONG_FORMAT
PI_CMP_TYPE = _pisock.PI_CMP_TYPE
PI_CMP_FLAGS = _pisock.PI_CMP_FLAGS
PI_CMP_VERS = _pisock.PI_CMP_VERS
PI_CMP_BAUD = _pisock.PI_CMP_BAUD
PI_NET_TYPE = _pisock.PI_NET_TYPE
PI_NET_SPLIT_WRITES = _pisock.PI_NET_SPLIT_WRITES
PI_NET_WRITE_CHUNKSIZE = _pisock.PI_NET_WRITE_CHUNKSIZE
PI_SOCK_STATE = _pisock.PI_SOCK_STATE
PI_SOCK_HONOR_RX_TIMEOUT = _pisock.PI_SOCK_HONOR_RX_TIMEOUT
class pi_socket_t(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, pi_socket_t, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, pi_socket_t, name)
    def __repr__(self):
        return "<%s.%s; proxy of C pi_socket_t instance at %s>" % (self.__class__.__module__, self.__class__.__name__, self.this,)
    __swig_setmethods__["sd"] = _pisock.pi_socket_t_sd_set
    __swig_getmethods__["sd"] = _pisock.pi_socket_t_sd_get
    if _newclass:sd = property(_pisock.pi_socket_t_sd_get, _pisock.pi_socket_t_sd_set)
    __swig_setmethods__["type"] = _pisock.pi_socket_t_type_set
    __swig_getmethods__["type"] = _pisock.pi_socket_t_type_get
    if _newclass:type = property(_pisock.pi_socket_t_type_get, _pisock.pi_socket_t_type_set)
    __swig_setmethods__["protocol"] = _pisock.pi_socket_t_protocol_set
    __swig_getmethods__["protocol"] = _pisock.pi_socket_t_protocol_get
    if _newclass:protocol = property(_pisock.pi_socket_t_protocol_get, _pisock.pi_socket_t_protocol_set)
    __swig_setmethods__["cmd"] = _pisock.pi_socket_t_cmd_set
    __swig_getmethods__["cmd"] = _pisock.pi_socket_t_cmd_get
    if _newclass:cmd = property(_pisock.pi_socket_t_cmd_get, _pisock.pi_socket_t_cmd_set)
    __swig_setmethods__["laddr"] = _pisock.pi_socket_t_laddr_set
    __swig_getmethods__["laddr"] = _pisock.pi_socket_t_laddr_get
    if _newclass:laddr = property(_pisock.pi_socket_t_laddr_get, _pisock.pi_socket_t_laddr_set)
    __swig_setmethods__["laddrlen"] = _pisock.pi_socket_t_laddrlen_set
    __swig_getmethods__["laddrlen"] = _pisock.pi_socket_t_laddrlen_get
    if _newclass:laddrlen = property(_pisock.pi_socket_t_laddrlen_get, _pisock.pi_socket_t_laddrlen_set)
    __swig_setmethods__["raddr"] = _pisock.pi_socket_t_raddr_set
    __swig_getmethods__["raddr"] = _pisock.pi_socket_t_raddr_get
    if _newclass:raddr = property(_pisock.pi_socket_t_raddr_get, _pisock.pi_socket_t_raddr_set)
    __swig_setmethods__["raddrlen"] = _pisock.pi_socket_t_raddrlen_set
    __swig_getmethods__["raddrlen"] = _pisock.pi_socket_t_raddrlen_get
    if _newclass:raddrlen = property(_pisock.pi_socket_t_raddrlen_get, _pisock.pi_socket_t_raddrlen_set)
    __swig_setmethods__["protocol_queue"] = _pisock.pi_socket_t_protocol_queue_set
    __swig_getmethods__["protocol_queue"] = _pisock.pi_socket_t_protocol_queue_get
    if _newclass:protocol_queue = property(_pisock.pi_socket_t_protocol_queue_get, _pisock.pi_socket_t_protocol_queue_set)
    __swig_setmethods__["queue_len"] = _pisock.pi_socket_t_queue_len_set
    __swig_getmethods__["queue_len"] = _pisock.pi_socket_t_queue_len_get
    if _newclass:queue_len = property(_pisock.pi_socket_t_queue_len_get, _pisock.pi_socket_t_queue_len_set)
    __swig_setmethods__["cmd_queue"] = _pisock.pi_socket_t_cmd_queue_set
    __swig_getmethods__["cmd_queue"] = _pisock.pi_socket_t_cmd_queue_get
    if _newclass:cmd_queue = property(_pisock.pi_socket_t_cmd_queue_get, _pisock.pi_socket_t_cmd_queue_set)
    __swig_setmethods__["cmd_len"] = _pisock.pi_socket_t_cmd_len_set
    __swig_getmethods__["cmd_len"] = _pisock.pi_socket_t_cmd_len_get
    if _newclass:cmd_len = property(_pisock.pi_socket_t_cmd_len_get, _pisock.pi_socket_t_cmd_len_set)
    __swig_setmethods__["device"] = _pisock.pi_socket_t_device_set
    __swig_getmethods__["device"] = _pisock.pi_socket_t_device_get
    if _newclass:device = property(_pisock.pi_socket_t_device_get, _pisock.pi_socket_t_device_set)
    __swig_setmethods__["state"] = _pisock.pi_socket_t_state_set
    __swig_getmethods__["state"] = _pisock.pi_socket_t_state_get
    if _newclass:state = property(_pisock.pi_socket_t_state_get, _pisock.pi_socket_t_state_set)
    __swig_setmethods__["honor_rx_to"] = _pisock.pi_socket_t_honor_rx_to_set
    __swig_getmethods__["honor_rx_to"] = _pisock.pi_socket_t_honor_rx_to_get
    if _newclass:honor_rx_to = property(_pisock.pi_socket_t_honor_rx_to_get, _pisock.pi_socket_t_honor_rx_to_set)
    __swig_setmethods__["command"] = _pisock.pi_socket_t_command_set
    __swig_getmethods__["command"] = _pisock.pi_socket_t_command_get
    if _newclass:command = property(_pisock.pi_socket_t_command_get, _pisock.pi_socket_t_command_set)
    __swig_setmethods__["accept_to"] = _pisock.pi_socket_t_accept_to_set
    __swig_getmethods__["accept_to"] = _pisock.pi_socket_t_accept_to_get
    if _newclass:accept_to = property(_pisock.pi_socket_t_accept_to_get, _pisock.pi_socket_t_accept_to_set)
    __swig_setmethods__["dlprecord"] = _pisock.pi_socket_t_dlprecord_set
    __swig_getmethods__["dlprecord"] = _pisock.pi_socket_t_dlprecord_get
    if _newclass:dlprecord = property(_pisock.pi_socket_t_dlprecord_get, _pisock.pi_socket_t_dlprecord_set)
    __swig_setmethods__["dlpversion"] = _pisock.pi_socket_t_dlpversion_set
    __swig_getmethods__["dlpversion"] = _pisock.pi_socket_t_dlpversion_get
    if _newclass:dlpversion = property(_pisock.pi_socket_t_dlpversion_get, _pisock.pi_socket_t_dlpversion_set)
    __swig_setmethods__["maxrecsize"] = _pisock.pi_socket_t_maxrecsize_set
    __swig_getmethods__["maxrecsize"] = _pisock.pi_socket_t_maxrecsize_get
    if _newclass:maxrecsize = property(_pisock.pi_socket_t_maxrecsize_get, _pisock.pi_socket_t_maxrecsize_set)
    __swig_setmethods__["last_error"] = _pisock.pi_socket_t_last_error_set
    __swig_getmethods__["last_error"] = _pisock.pi_socket_t_last_error_get
    if _newclass:last_error = property(_pisock.pi_socket_t_last_error_get, _pisock.pi_socket_t_last_error_set)
    __swig_setmethods__["palmos_error"] = _pisock.pi_socket_t_palmos_error_set
    __swig_getmethods__["palmos_error"] = _pisock.pi_socket_t_palmos_error_get
    if _newclass:palmos_error = property(_pisock.pi_socket_t_palmos_error_get, _pisock.pi_socket_t_palmos_error_set)
    def __init__(self, *args):
        _swig_setattr(self, pi_socket_t, 'this', _pisock.new_pi_socket_t(*args))
        _swig_setattr(self, pi_socket_t, 'thisown', 1)
    def __del__(self, destroy=_pisock.delete_pi_socket_t):
        try:
            if self.thisown: destroy(self)
        except: pass


class pi_socket_tPtr(pi_socket_t):
    def __init__(self, this):
        _swig_setattr(self, pi_socket_t, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, pi_socket_t, 'thisown', 0)
        self.__class__ = pi_socket_t
_pisock.pi_socket_t_swigregister(pi_socket_tPtr)
dlp_ReadRecordIDList = _pisock.dlp_ReadRecordIDList

pi_file_install = _pisock.pi_file_install

pi_file_retrieve = _pisock.pi_file_retrieve


class pi_socket_list_t(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, pi_socket_list_t, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, pi_socket_list_t, name)
    def __repr__(self):
        return "<%s.%s; proxy of C pi_socket_list_t instance at %s>" % (self.__class__.__module__, self.__class__.__name__, self.this,)
    __swig_setmethods__["ps"] = _pisock.pi_socket_list_t_ps_set
    __swig_getmethods__["ps"] = _pisock.pi_socket_list_t_ps_get
    if _newclass:ps = property(_pisock.pi_socket_list_t_ps_get, _pisock.pi_socket_list_t_ps_set)
    __swig_setmethods__["next"] = _pisock.pi_socket_list_t_next_set
    __swig_getmethods__["next"] = _pisock.pi_socket_list_t_next_get
    if _newclass:next = property(_pisock.pi_socket_list_t_next_get, _pisock.pi_socket_list_t_next_set)
    def __init__(self, *args):
        _swig_setattr(self, pi_socket_list_t, 'this', _pisock.new_pi_socket_list_t(*args))
        _swig_setattr(self, pi_socket_list_t, 'thisown', 1)
    def __del__(self, destroy=_pisock.delete_pi_socket_list_t):
        try:
            if self.thisown: destroy(self)
        except: pass


class pi_socket_list_tPtr(pi_socket_list_t):
    def __init__(self, this):
        _swig_setattr(self, pi_socket_list_t, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, pi_socket_list_t, 'thisown', 0)
        self.__class__ = pi_socket_list_t
_pisock.pi_socket_list_t_swigregister(pi_socket_list_tPtr)


pi_socket = _pisock.pi_socket

pi_socket_setsd = _pisock.pi_socket_setsd

pi_getsockname = _pisock.pi_getsockname

pi_getsockpeer = _pisock.pi_getsockpeer

pi_getsockopt = _pisock.pi_getsockopt

pi_setsockopt = _pisock.pi_setsockopt

pi_protocol = _pisock.pi_protocol

pi_protocol_next = _pisock.pi_protocol_next

pi_socket_connected = _pisock.pi_socket_connected

pi_connect = _pisock.pi_connect

pi_bind = _pisock.pi_bind

pi_listen = _pisock.pi_listen

pi_accept = _pisock.pi_accept

pi_accept_to = _pisock.pi_accept_to

pi_close = _pisock.pi_close

pi_send = _pisock.pi_send

pi_recv = _pisock.pi_recv

pi_read = _pisock.pi_read

pi_write = _pisock.pi_write

pi_flush = _pisock.pi_flush

pi_error = _pisock.pi_error

pi_set_error = _pisock.pi_set_error

pi_palmos_error = _pisock.pi_palmos_error

pi_set_palmos_error = _pisock.pi_set_palmos_error

pi_reset_errors = _pisock.pi_reset_errors

pi_version = _pisock.pi_version

pi_maxrecsize = _pisock.pi_maxrecsize

pi_tickle = _pisock.pi_tickle

pi_watchdog = _pisock.pi_watchdog
PI_DLP_VERSION_MAJOR = _pisock.PI_DLP_VERSION_MAJOR
PI_DLP_VERSION_MINOR = _pisock.PI_DLP_VERSION_MINOR
vfsMountFlagsUseThisFileSystem = _pisock.vfsMountFlagsUseThisFileSystem
vfsMAXFILENAME = _pisock.vfsMAXFILENAME
vfsInvalidVolRef = _pisock.vfsInvalidVolRef
vfsInvalidFileRef = _pisock.vfsInvalidFileRef
class VFSDirInfo(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, VFSDirInfo, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, VFSDirInfo, name)
    def __repr__(self):
        return "<%s.%s; proxy of C VFSDirInfo instance at %s>" % (self.__class__.__module__, self.__class__.__name__, self.this,)
    __swig_setmethods__["attr"] = _pisock.VFSDirInfo_attr_set
    __swig_getmethods__["attr"] = _pisock.VFSDirInfo_attr_get
    if _newclass:attr = property(_pisock.VFSDirInfo_attr_get, _pisock.VFSDirInfo_attr_set)
    __swig_setmethods__["name"] = _pisock.VFSDirInfo_name_set
    __swig_getmethods__["name"] = _pisock.VFSDirInfo_name_get
    if _newclass:name = property(_pisock.VFSDirInfo_name_get, _pisock.VFSDirInfo_name_set)
    def __init__(self, *args):
        _swig_setattr(self, VFSDirInfo, 'this', _pisock.new_VFSDirInfo(*args))
        _swig_setattr(self, VFSDirInfo, 'thisown', 1)
    def __del__(self, destroy=_pisock.delete_VFSDirInfo):
        try:
            if self.thisown: destroy(self)
        except: pass


class VFSDirInfoPtr(VFSDirInfo):
    def __init__(self, this):
        _swig_setattr(self, VFSDirInfo, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, VFSDirInfo, 'thisown', 0)
        self.__class__ = VFSDirInfo
_pisock.VFSDirInfo_swigregister(VFSDirInfoPtr)

class VFSAnyMountParam(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, VFSAnyMountParam, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, VFSAnyMountParam, name)
    def __repr__(self):
        return "<%s.%s; proxy of C VFSAnyMountParam instance at %s>" % (self.__class__.__module__, self.__class__.__name__, self.this,)
    __swig_setmethods__["volRefNum"] = _pisock.VFSAnyMountParam_volRefNum_set
    __swig_getmethods__["volRefNum"] = _pisock.VFSAnyMountParam_volRefNum_get
    if _newclass:volRefNum = property(_pisock.VFSAnyMountParam_volRefNum_get, _pisock.VFSAnyMountParam_volRefNum_set)
    __swig_setmethods__["reserved"] = _pisock.VFSAnyMountParam_reserved_set
    __swig_getmethods__["reserved"] = _pisock.VFSAnyMountParam_reserved_get
    if _newclass:reserved = property(_pisock.VFSAnyMountParam_reserved_get, _pisock.VFSAnyMountParam_reserved_set)
    __swig_setmethods__["mountClass"] = _pisock.VFSAnyMountParam_mountClass_set
    __swig_getmethods__["mountClass"] = _pisock.VFSAnyMountParam_mountClass_get
    if _newclass:mountClass = property(_pisock.VFSAnyMountParam_mountClass_get, _pisock.VFSAnyMountParam_mountClass_set)
    def __init__(self, *args):
        _swig_setattr(self, VFSAnyMountParam, 'this', _pisock.new_VFSAnyMountParam(*args))
        _swig_setattr(self, VFSAnyMountParam, 'thisown', 1)
    def __del__(self, destroy=_pisock.delete_VFSAnyMountParam):
        try:
            if self.thisown: destroy(self)
        except: pass


class VFSAnyMountParamPtr(VFSAnyMountParam):
    def __init__(self, this):
        _swig_setattr(self, VFSAnyMountParam, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, VFSAnyMountParam, 'thisown', 0)
        self.__class__ = VFSAnyMountParam
_pisock.VFSAnyMountParam_swigregister(VFSAnyMountParamPtr)

class VFSSlotMountParam(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, VFSSlotMountParam, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, VFSSlotMountParam, name)
    def __repr__(self):
        return "<%s.%s; proxy of C VFSSlotMountParam instance at %s>" % (self.__class__.__module__, self.__class__.__name__, self.this,)
    __swig_setmethods__["vfsMountParam"] = _pisock.VFSSlotMountParam_vfsMountParam_set
    __swig_getmethods__["vfsMountParam"] = _pisock.VFSSlotMountParam_vfsMountParam_get
    if _newclass:vfsMountParam = property(_pisock.VFSSlotMountParam_vfsMountParam_get, _pisock.VFSSlotMountParam_vfsMountParam_set)
    __swig_setmethods__["slotLibRefNum"] = _pisock.VFSSlotMountParam_slotLibRefNum_set
    __swig_getmethods__["slotLibRefNum"] = _pisock.VFSSlotMountParam_slotLibRefNum_get
    if _newclass:slotLibRefNum = property(_pisock.VFSSlotMountParam_slotLibRefNum_get, _pisock.VFSSlotMountParam_slotLibRefNum_set)
    __swig_setmethods__["slotRefNum"] = _pisock.VFSSlotMountParam_slotRefNum_set
    __swig_getmethods__["slotRefNum"] = _pisock.VFSSlotMountParam_slotRefNum_get
    if _newclass:slotRefNum = property(_pisock.VFSSlotMountParam_slotRefNum_get, _pisock.VFSSlotMountParam_slotRefNum_set)
    def __init__(self, *args):
        _swig_setattr(self, VFSSlotMountParam, 'this', _pisock.new_VFSSlotMountParam(*args))
        _swig_setattr(self, VFSSlotMountParam, 'thisown', 1)
    def __del__(self, destroy=_pisock.delete_VFSSlotMountParam):
        try:
            if self.thisown: destroy(self)
        except: pass


class VFSSlotMountParamPtr(VFSSlotMountParam):
    def __init__(self, this):
        _swig_setattr(self, VFSSlotMountParam, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, VFSSlotMountParam, 'thisown', 0)
        self.__class__ = VFSSlotMountParam
_pisock.VFSSlotMountParam_swigregister(VFSSlotMountParamPtr)

class VFSInfo(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, VFSInfo, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, VFSInfo, name)
    def __repr__(self):
        return "<%s.%s; proxy of C VFSInfo instance at %s>" % (self.__class__.__module__, self.__class__.__name__, self.this,)
    __swig_setmethods__["attributes"] = _pisock.VFSInfo_attributes_set
    __swig_getmethods__["attributes"] = _pisock.VFSInfo_attributes_get
    if _newclass:attributes = property(_pisock.VFSInfo_attributes_get, _pisock.VFSInfo_attributes_set)
    __swig_setmethods__["fsType"] = _pisock.VFSInfo_fsType_set
    __swig_getmethods__["fsType"] = _pisock.VFSInfo_fsType_get
    if _newclass:fsType = property(_pisock.VFSInfo_fsType_get, _pisock.VFSInfo_fsType_set)
    __swig_setmethods__["fsCreator"] = _pisock.VFSInfo_fsCreator_set
    __swig_getmethods__["fsCreator"] = _pisock.VFSInfo_fsCreator_get
    if _newclass:fsCreator = property(_pisock.VFSInfo_fsCreator_get, _pisock.VFSInfo_fsCreator_set)
    __swig_setmethods__["mountClass"] = _pisock.VFSInfo_mountClass_set
    __swig_getmethods__["mountClass"] = _pisock.VFSInfo_mountClass_get
    if _newclass:mountClass = property(_pisock.VFSInfo_mountClass_get, _pisock.VFSInfo_mountClass_set)
    __swig_setmethods__["slotLibRefNum"] = _pisock.VFSInfo_slotLibRefNum_set
    __swig_getmethods__["slotLibRefNum"] = _pisock.VFSInfo_slotLibRefNum_get
    if _newclass:slotLibRefNum = property(_pisock.VFSInfo_slotLibRefNum_get, _pisock.VFSInfo_slotLibRefNum_set)
    __swig_setmethods__["slotRefNum"] = _pisock.VFSInfo_slotRefNum_set
    __swig_getmethods__["slotRefNum"] = _pisock.VFSInfo_slotRefNum_get
    if _newclass:slotRefNum = property(_pisock.VFSInfo_slotRefNum_get, _pisock.VFSInfo_slotRefNum_set)
    __swig_setmethods__["mediaType"] = _pisock.VFSInfo_mediaType_set
    __swig_getmethods__["mediaType"] = _pisock.VFSInfo_mediaType_get
    if _newclass:mediaType = property(_pisock.VFSInfo_mediaType_get, _pisock.VFSInfo_mediaType_set)
    __swig_setmethods__["reserved"] = _pisock.VFSInfo_reserved_set
    __swig_getmethods__["reserved"] = _pisock.VFSInfo_reserved_get
    if _newclass:reserved = property(_pisock.VFSInfo_reserved_get, _pisock.VFSInfo_reserved_set)
    def __init__(self, *args):
        _swig_setattr(self, VFSInfo, 'this', _pisock.new_VFSInfo(*args))
        _swig_setattr(self, VFSInfo, 'thisown', 1)
    def __del__(self, destroy=_pisock.delete_VFSInfo):
        try:
            if self.thisown: destroy(self)
        except: pass


class VFSInfoPtr(VFSInfo):
    def __init__(self, this):
        _swig_setattr(self, VFSInfo, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, VFSInfo, 'thisown', 0)
        self.__class__ = VFSInfo
_pisock.VFSInfo_swigregister(VFSInfoPtr)

class PilotUser(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, PilotUser, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, PilotUser, name)
    def __repr__(self):
        return "<%s.%s; proxy of C PilotUser instance at %s>" % (self.__class__.__module__, self.__class__.__name__, self.this,)
    __swig_setmethods__["passwordLength"] = _pisock.PilotUser_passwordLength_set
    __swig_getmethods__["passwordLength"] = _pisock.PilotUser_passwordLength_get
    if _newclass:passwordLength = property(_pisock.PilotUser_passwordLength_get, _pisock.PilotUser_passwordLength_set)
    __swig_setmethods__["username"] = _pisock.PilotUser_username_set
    __swig_getmethods__["username"] = _pisock.PilotUser_username_get
    if _newclass:username = property(_pisock.PilotUser_username_get, _pisock.PilotUser_username_set)
    __swig_setmethods__["password"] = _pisock.PilotUser_password_set
    __swig_getmethods__["password"] = _pisock.PilotUser_password_get
    if _newclass:password = property(_pisock.PilotUser_password_get, _pisock.PilotUser_password_set)
    __swig_setmethods__["userID"] = _pisock.PilotUser_userID_set
    __swig_getmethods__["userID"] = _pisock.PilotUser_userID_get
    if _newclass:userID = property(_pisock.PilotUser_userID_get, _pisock.PilotUser_userID_set)
    __swig_setmethods__["viewerID"] = _pisock.PilotUser_viewerID_set
    __swig_getmethods__["viewerID"] = _pisock.PilotUser_viewerID_get
    if _newclass:viewerID = property(_pisock.PilotUser_viewerID_get, _pisock.PilotUser_viewerID_set)
    __swig_setmethods__["lastSyncPC"] = _pisock.PilotUser_lastSyncPC_set
    __swig_getmethods__["lastSyncPC"] = _pisock.PilotUser_lastSyncPC_get
    if _newclass:lastSyncPC = property(_pisock.PilotUser_lastSyncPC_get, _pisock.PilotUser_lastSyncPC_set)
    __swig_setmethods__["successfulSyncDate"] = _pisock.PilotUser_successfulSyncDate_set
    __swig_getmethods__["successfulSyncDate"] = _pisock.PilotUser_successfulSyncDate_get
    if _newclass:successfulSyncDate = property(_pisock.PilotUser_successfulSyncDate_get, _pisock.PilotUser_successfulSyncDate_set)
    __swig_setmethods__["lastSyncDate"] = _pisock.PilotUser_lastSyncDate_set
    __swig_getmethods__["lastSyncDate"] = _pisock.PilotUser_lastSyncDate_get
    if _newclass:lastSyncDate = property(_pisock.PilotUser_lastSyncDate_get, _pisock.PilotUser_lastSyncDate_set)
    def __init__(self, *args):
        _swig_setattr(self, PilotUser, 'this', _pisock.new_PilotUser(*args))
        _swig_setattr(self, PilotUser, 'thisown', 1)
    def __del__(self, destroy=_pisock.delete_PilotUser):
        try:
            if self.thisown: destroy(self)
        except: pass


class PilotUserPtr(PilotUser):
    def __init__(self, this):
        _swig_setattr(self, PilotUser, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, PilotUser, 'thisown', 0)
        self.__class__ = PilotUser
_pisock.PilotUser_swigregister(PilotUserPtr)

class SysInfo(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, SysInfo, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, SysInfo, name)
    def __repr__(self):
        return "<%s.%s; proxy of C SysInfo instance at %s>" % (self.__class__.__module__, self.__class__.__name__, self.this,)
    __swig_setmethods__["romVersion"] = _pisock.SysInfo_romVersion_set
    __swig_getmethods__["romVersion"] = _pisock.SysInfo_romVersion_get
    if _newclass:romVersion = property(_pisock.SysInfo_romVersion_get, _pisock.SysInfo_romVersion_set)
    __swig_setmethods__["locale"] = _pisock.SysInfo_locale_set
    __swig_getmethods__["locale"] = _pisock.SysInfo_locale_get
    if _newclass:locale = property(_pisock.SysInfo_locale_get, _pisock.SysInfo_locale_set)
    __swig_setmethods__["prodIDLength"] = _pisock.SysInfo_prodIDLength_set
    __swig_getmethods__["prodIDLength"] = _pisock.SysInfo_prodIDLength_get
    if _newclass:prodIDLength = property(_pisock.SysInfo_prodIDLength_get, _pisock.SysInfo_prodIDLength_set)
    __swig_setmethods__["prodID"] = _pisock.SysInfo_prodID_set
    __swig_getmethods__["prodID"] = _pisock.SysInfo_prodID_get
    if _newclass:prodID = property(_pisock.SysInfo_prodID_get, _pisock.SysInfo_prodID_set)
    __swig_setmethods__["dlpMajorVersion"] = _pisock.SysInfo_dlpMajorVersion_set
    __swig_getmethods__["dlpMajorVersion"] = _pisock.SysInfo_dlpMajorVersion_get
    if _newclass:dlpMajorVersion = property(_pisock.SysInfo_dlpMajorVersion_get, _pisock.SysInfo_dlpMajorVersion_set)
    __swig_setmethods__["dlpMinorVersion"] = _pisock.SysInfo_dlpMinorVersion_set
    __swig_getmethods__["dlpMinorVersion"] = _pisock.SysInfo_dlpMinorVersion_get
    if _newclass:dlpMinorVersion = property(_pisock.SysInfo_dlpMinorVersion_get, _pisock.SysInfo_dlpMinorVersion_set)
    __swig_setmethods__["compatMajorVersion"] = _pisock.SysInfo_compatMajorVersion_set
    __swig_getmethods__["compatMajorVersion"] = _pisock.SysInfo_compatMajorVersion_get
    if _newclass:compatMajorVersion = property(_pisock.SysInfo_compatMajorVersion_get, _pisock.SysInfo_compatMajorVersion_set)
    __swig_setmethods__["compatMinorVersion"] = _pisock.SysInfo_compatMinorVersion_set
    __swig_getmethods__["compatMinorVersion"] = _pisock.SysInfo_compatMinorVersion_get
    if _newclass:compatMinorVersion = property(_pisock.SysInfo_compatMinorVersion_get, _pisock.SysInfo_compatMinorVersion_set)
    __swig_setmethods__["maxRecSize"] = _pisock.SysInfo_maxRecSize_set
    __swig_getmethods__["maxRecSize"] = _pisock.SysInfo_maxRecSize_get
    if _newclass:maxRecSize = property(_pisock.SysInfo_maxRecSize_get, _pisock.SysInfo_maxRecSize_set)
    def __init__(self, *args):
        _swig_setattr(self, SysInfo, 'this', _pisock.new_SysInfo(*args))
        _swig_setattr(self, SysInfo, 'thisown', 1)
    def __del__(self, destroy=_pisock.delete_SysInfo):
        try:
            if self.thisown: destroy(self)
        except: pass


class SysInfoPtr(SysInfo):
    def __init__(self, this):
        _swig_setattr(self, SysInfo, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, SysInfo, 'thisown', 0)
        self.__class__ = SysInfo
_pisock.SysInfo_swigregister(SysInfoPtr)

class DBInfo(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, DBInfo, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, DBInfo, name)
    def __repr__(self):
        return "<%s.%s; proxy of C DBInfo instance at %s>" % (self.__class__.__module__, self.__class__.__name__, self.this,)
    __swig_setmethods__["more"] = _pisock.DBInfo_more_set
    __swig_getmethods__["more"] = _pisock.DBInfo_more_get
    if _newclass:more = property(_pisock.DBInfo_more_get, _pisock.DBInfo_more_set)
    __swig_setmethods__["name"] = _pisock.DBInfo_name_set
    __swig_getmethods__["name"] = _pisock.DBInfo_name_get
    if _newclass:name = property(_pisock.DBInfo_name_get, _pisock.DBInfo_name_set)
    __swig_setmethods__["flags"] = _pisock.DBInfo_flags_set
    __swig_getmethods__["flags"] = _pisock.DBInfo_flags_get
    if _newclass:flags = property(_pisock.DBInfo_flags_get, _pisock.DBInfo_flags_set)
    __swig_setmethods__["miscFlags"] = _pisock.DBInfo_miscFlags_set
    __swig_getmethods__["miscFlags"] = _pisock.DBInfo_miscFlags_get
    if _newclass:miscFlags = property(_pisock.DBInfo_miscFlags_get, _pisock.DBInfo_miscFlags_set)
    __swig_setmethods__["version"] = _pisock.DBInfo_version_set
    __swig_getmethods__["version"] = _pisock.DBInfo_version_get
    if _newclass:version = property(_pisock.DBInfo_version_get, _pisock.DBInfo_version_set)
    __swig_setmethods__["type"] = _pisock.DBInfo_type_set
    __swig_getmethods__["type"] = _pisock.DBInfo_type_get
    if _newclass:type = property(_pisock.DBInfo_type_get, _pisock.DBInfo_type_set)
    __swig_setmethods__["creator"] = _pisock.DBInfo_creator_set
    __swig_getmethods__["creator"] = _pisock.DBInfo_creator_get
    if _newclass:creator = property(_pisock.DBInfo_creator_get, _pisock.DBInfo_creator_set)
    __swig_setmethods__["modnum"] = _pisock.DBInfo_modnum_set
    __swig_getmethods__["modnum"] = _pisock.DBInfo_modnum_get
    if _newclass:modnum = property(_pisock.DBInfo_modnum_get, _pisock.DBInfo_modnum_set)
    __swig_setmethods__["index"] = _pisock.DBInfo_index_set
    __swig_getmethods__["index"] = _pisock.DBInfo_index_get
    if _newclass:index = property(_pisock.DBInfo_index_get, _pisock.DBInfo_index_set)
    __swig_setmethods__["createDate"] = _pisock.DBInfo_createDate_set
    __swig_getmethods__["createDate"] = _pisock.DBInfo_createDate_get
    if _newclass:createDate = property(_pisock.DBInfo_createDate_get, _pisock.DBInfo_createDate_set)
    __swig_setmethods__["modifyDate"] = _pisock.DBInfo_modifyDate_set
    __swig_getmethods__["modifyDate"] = _pisock.DBInfo_modifyDate_get
    if _newclass:modifyDate = property(_pisock.DBInfo_modifyDate_get, _pisock.DBInfo_modifyDate_set)
    __swig_setmethods__["backupDate"] = _pisock.DBInfo_backupDate_set
    __swig_getmethods__["backupDate"] = _pisock.DBInfo_backupDate_get
    if _newclass:backupDate = property(_pisock.DBInfo_backupDate_get, _pisock.DBInfo_backupDate_set)
    def __init__(self, *args):
        _swig_setattr(self, DBInfo, 'this', _pisock.new_DBInfo(*args))
        _swig_setattr(self, DBInfo, 'thisown', 1)
    def __del__(self, destroy=_pisock.delete_DBInfo):
        try:
            if self.thisown: destroy(self)
        except: pass


class DBInfoPtr(DBInfo):
    def __init__(self, this):
        _swig_setattr(self, DBInfo, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, DBInfo, 'thisown', 0)
        self.__class__ = DBInfo
_pisock.DBInfo_swigregister(DBInfoPtr)

class DBSizeInfo(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, DBSizeInfo, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, DBSizeInfo, name)
    def __repr__(self):
        return "<%s.%s; proxy of C DBSizeInfo instance at %s>" % (self.__class__.__module__, self.__class__.__name__, self.this,)
    __swig_setmethods__["numRecords"] = _pisock.DBSizeInfo_numRecords_set
    __swig_getmethods__["numRecords"] = _pisock.DBSizeInfo_numRecords_get
    if _newclass:numRecords = property(_pisock.DBSizeInfo_numRecords_get, _pisock.DBSizeInfo_numRecords_set)
    __swig_setmethods__["totalBytes"] = _pisock.DBSizeInfo_totalBytes_set
    __swig_getmethods__["totalBytes"] = _pisock.DBSizeInfo_totalBytes_get
    if _newclass:totalBytes = property(_pisock.DBSizeInfo_totalBytes_get, _pisock.DBSizeInfo_totalBytes_set)
    __swig_setmethods__["dataBytes"] = _pisock.DBSizeInfo_dataBytes_set
    __swig_getmethods__["dataBytes"] = _pisock.DBSizeInfo_dataBytes_get
    if _newclass:dataBytes = property(_pisock.DBSizeInfo_dataBytes_get, _pisock.DBSizeInfo_dataBytes_set)
    __swig_setmethods__["appBlockSize"] = _pisock.DBSizeInfo_appBlockSize_set
    __swig_getmethods__["appBlockSize"] = _pisock.DBSizeInfo_appBlockSize_get
    if _newclass:appBlockSize = property(_pisock.DBSizeInfo_appBlockSize_get, _pisock.DBSizeInfo_appBlockSize_set)
    __swig_setmethods__["sortBlockSize"] = _pisock.DBSizeInfo_sortBlockSize_set
    __swig_getmethods__["sortBlockSize"] = _pisock.DBSizeInfo_sortBlockSize_get
    if _newclass:sortBlockSize = property(_pisock.DBSizeInfo_sortBlockSize_get, _pisock.DBSizeInfo_sortBlockSize_set)
    __swig_setmethods__["maxRecSize"] = _pisock.DBSizeInfo_maxRecSize_set
    __swig_getmethods__["maxRecSize"] = _pisock.DBSizeInfo_maxRecSize_get
    if _newclass:maxRecSize = property(_pisock.DBSizeInfo_maxRecSize_get, _pisock.DBSizeInfo_maxRecSize_set)
    def __init__(self, *args):
        _swig_setattr(self, DBSizeInfo, 'this', _pisock.new_DBSizeInfo(*args))
        _swig_setattr(self, DBSizeInfo, 'thisown', 1)
    def __del__(self, destroy=_pisock.delete_DBSizeInfo):
        try:
            if self.thisown: destroy(self)
        except: pass


class DBSizeInfoPtr(DBSizeInfo):
    def __init__(self, this):
        _swig_setattr(self, DBSizeInfo, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, DBSizeInfo, 'thisown', 0)
        self.__class__ = DBSizeInfo
_pisock.DBSizeInfo_swigregister(DBSizeInfoPtr)

class CardInfo(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, CardInfo, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, CardInfo, name)
    def __repr__(self):
        return "<%s.%s; proxy of C CardInfo instance at %s>" % (self.__class__.__module__, self.__class__.__name__, self.this,)
    __swig_setmethods__["card"] = _pisock.CardInfo_card_set
    __swig_getmethods__["card"] = _pisock.CardInfo_card_get
    if _newclass:card = property(_pisock.CardInfo_card_get, _pisock.CardInfo_card_set)
    __swig_setmethods__["version"] = _pisock.CardInfo_version_set
    __swig_getmethods__["version"] = _pisock.CardInfo_version_get
    if _newclass:version = property(_pisock.CardInfo_version_get, _pisock.CardInfo_version_set)
    __swig_setmethods__["more"] = _pisock.CardInfo_more_set
    __swig_getmethods__["more"] = _pisock.CardInfo_more_get
    if _newclass:more = property(_pisock.CardInfo_more_get, _pisock.CardInfo_more_set)
    __swig_setmethods__["creation"] = _pisock.CardInfo_creation_set
    __swig_getmethods__["creation"] = _pisock.CardInfo_creation_get
    if _newclass:creation = property(_pisock.CardInfo_creation_get, _pisock.CardInfo_creation_set)
    __swig_setmethods__["romSize"] = _pisock.CardInfo_romSize_set
    __swig_getmethods__["romSize"] = _pisock.CardInfo_romSize_get
    if _newclass:romSize = property(_pisock.CardInfo_romSize_get, _pisock.CardInfo_romSize_set)
    __swig_setmethods__["ramSize"] = _pisock.CardInfo_ramSize_set
    __swig_getmethods__["ramSize"] = _pisock.CardInfo_ramSize_get
    if _newclass:ramSize = property(_pisock.CardInfo_ramSize_get, _pisock.CardInfo_ramSize_set)
    __swig_setmethods__["ramFree"] = _pisock.CardInfo_ramFree_set
    __swig_getmethods__["ramFree"] = _pisock.CardInfo_ramFree_get
    if _newclass:ramFree = property(_pisock.CardInfo_ramFree_get, _pisock.CardInfo_ramFree_set)
    __swig_setmethods__["name"] = _pisock.CardInfo_name_set
    __swig_getmethods__["name"] = _pisock.CardInfo_name_get
    if _newclass:name = property(_pisock.CardInfo_name_get, _pisock.CardInfo_name_set)
    __swig_setmethods__["manufacturer"] = _pisock.CardInfo_manufacturer_set
    __swig_getmethods__["manufacturer"] = _pisock.CardInfo_manufacturer_get
    if _newclass:manufacturer = property(_pisock.CardInfo_manufacturer_get, _pisock.CardInfo_manufacturer_set)
    def __init__(self, *args):
        _swig_setattr(self, CardInfo, 'this', _pisock.new_CardInfo(*args))
        _swig_setattr(self, CardInfo, 'thisown', 1)
    def __del__(self, destroy=_pisock.delete_CardInfo):
        try:
            if self.thisown: destroy(self)
        except: pass


class CardInfoPtr(CardInfo):
    def __init__(self, this):
        _swig_setattr(self, CardInfo, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, CardInfo, 'thisown', 0)
        self.__class__ = CardInfo
_pisock.CardInfo_swigregister(CardInfoPtr)

class NetSyncInfo(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, NetSyncInfo, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, NetSyncInfo, name)
    def __repr__(self):
        return "<%s.%s; proxy of C NetSyncInfo instance at %s>" % (self.__class__.__module__, self.__class__.__name__, self.this,)
    __swig_setmethods__["lanSync"] = _pisock.NetSyncInfo_lanSync_set
    __swig_getmethods__["lanSync"] = _pisock.NetSyncInfo_lanSync_get
    if _newclass:lanSync = property(_pisock.NetSyncInfo_lanSync_get, _pisock.NetSyncInfo_lanSync_set)
    __swig_setmethods__["hostName"] = _pisock.NetSyncInfo_hostName_set
    __swig_getmethods__["hostName"] = _pisock.NetSyncInfo_hostName_get
    if _newclass:hostName = property(_pisock.NetSyncInfo_hostName_get, _pisock.NetSyncInfo_hostName_set)
    __swig_setmethods__["hostAddress"] = _pisock.NetSyncInfo_hostAddress_set
    __swig_getmethods__["hostAddress"] = _pisock.NetSyncInfo_hostAddress_get
    if _newclass:hostAddress = property(_pisock.NetSyncInfo_hostAddress_get, _pisock.NetSyncInfo_hostAddress_set)
    __swig_setmethods__["hostSubnetMask"] = _pisock.NetSyncInfo_hostSubnetMask_set
    __swig_getmethods__["hostSubnetMask"] = _pisock.NetSyncInfo_hostSubnetMask_get
    if _newclass:hostSubnetMask = property(_pisock.NetSyncInfo_hostSubnetMask_get, _pisock.NetSyncInfo_hostSubnetMask_set)
    def __init__(self, *args):
        _swig_setattr(self, NetSyncInfo, 'this', _pisock.new_NetSyncInfo(*args))
        _swig_setattr(self, NetSyncInfo, 'thisown', 1)
    def __del__(self, destroy=_pisock.delete_NetSyncInfo):
        try:
            if self.thisown: destroy(self)
        except: pass


class NetSyncInfoPtr(NetSyncInfo):
    def __init__(self, this):
        _swig_setattr(self, NetSyncInfo, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, NetSyncInfo, 'thisown', 0)
        self.__class__ = NetSyncInfo
_pisock.NetSyncInfo_swigregister(NetSyncInfoPtr)

dlpDBFlagResource = _pisock.dlpDBFlagResource
dlpDBFlagReadOnly = _pisock.dlpDBFlagReadOnly
dlpDBFlagAppInfoDirty = _pisock.dlpDBFlagAppInfoDirty
dlpDBFlagBackup = _pisock.dlpDBFlagBackup
dlpDBFlagHidden = _pisock.dlpDBFlagHidden
dlpDBFlagLaunchable = _pisock.dlpDBFlagLaunchable
dlpDBFlagRecyclable = _pisock.dlpDBFlagRecyclable
dlpDBFlagBundle = _pisock.dlpDBFlagBundle
dlpDBFlagOpen = _pisock.dlpDBFlagOpen
dlpDBFlagNewer = _pisock.dlpDBFlagNewer
dlpDBFlagReset = _pisock.dlpDBFlagReset
dlpDBFlagCopyPrevention = _pisock.dlpDBFlagCopyPrevention
dlpDBFlagStream = _pisock.dlpDBFlagStream
dlpDBFlagSchema = _pisock.dlpDBFlagSchema
dlpDBFlagSecure = _pisock.dlpDBFlagSecure
dlpDBFlagExtended = _pisock.dlpDBFlagExtended
dlpDBFlagFixedUp = _pisock.dlpDBFlagFixedUp
dlpDBMiscFlagExcludeFromSync = _pisock.dlpDBMiscFlagExcludeFromSync
dlpDBMiscFlagRamBased = _pisock.dlpDBMiscFlagRamBased
dlpRecAttrDeleted = _pisock.dlpRecAttrDeleted
dlpRecAttrDirty = _pisock.dlpRecAttrDirty
dlpRecAttrBusy = _pisock.dlpRecAttrBusy
dlpRecAttrSecret = _pisock.dlpRecAttrSecret
dlpRecAttrArchived = _pisock.dlpRecAttrArchived
dlpOpenRead = _pisock.dlpOpenRead
dlpOpenWrite = _pisock.dlpOpenWrite
dlpOpenExclusive = _pisock.dlpOpenExclusive
dlpOpenSecret = _pisock.dlpOpenSecret
dlpOpenReadWrite = _pisock.dlpOpenReadWrite
dlpDBListRAM = _pisock.dlpDBListRAM
dlpDBListROM = _pisock.dlpDBListROM
dlpDBListMultiple = _pisock.dlpDBListMultiple
dlpFindDBOptFlagGetAttributes = _pisock.dlpFindDBOptFlagGetAttributes
dlpFindDBOptFlagGetSize = _pisock.dlpFindDBOptFlagGetSize
dlpFindDBOptFlagMaxRecSize = _pisock.dlpFindDBOptFlagMaxRecSize
dlpFindDBSrchFlagNewSearch = _pisock.dlpFindDBSrchFlagNewSearch
dlpFindDBSrchFlagOnlyLatest = _pisock.dlpFindDBSrchFlagOnlyLatest
dlpEndCodeNormal = _pisock.dlpEndCodeNormal
dlpEndCodeOutOfMemory = _pisock.dlpEndCodeOutOfMemory
dlpEndCodeUserCan = _pisock.dlpEndCodeUserCan
dlpEndCodeOther = _pisock.dlpEndCodeOther
dlpExpCapabilityHasStorage = _pisock.dlpExpCapabilityHasStorage
dlpExpCapabilityReadOnly = _pisock.dlpExpCapabilityReadOnly
dlpExpCapabilitySerial = _pisock.dlpExpCapabilitySerial
vfsVolAttrSlotBased = _pisock.vfsVolAttrSlotBased
vfsVolAttrReadOnly = _pisock.vfsVolAttrReadOnly
vfsVolAttrHidden = _pisock.vfsVolAttrHidden
vfsOriginBeginning = _pisock.vfsOriginBeginning
vfsOriginCurrent = _pisock.vfsOriginCurrent
vfsOriginEnd = _pisock.vfsOriginEnd
dlpVFSOpenExclusive = _pisock.dlpVFSOpenExclusive
dlpVFSOpenRead = _pisock.dlpVFSOpenRead
dlpVFSOpenWrite = _pisock.dlpVFSOpenWrite
dlpVFSOpenReadWrite = _pisock.dlpVFSOpenReadWrite
vfsModeExclusive = _pisock.vfsModeExclusive
vfsModeRead = _pisock.vfsModeRead
vfsModeWrite = _pisock.vfsModeWrite
vfsModeReadWrite = _pisock.vfsModeReadWrite
vfsModeCreate = _pisock.vfsModeCreate
vfsModeTruncate = _pisock.vfsModeTruncate
vfsModeLeaveOpen = _pisock.vfsModeLeaveOpen
vfsFileAttrReadOnly = _pisock.vfsFileAttrReadOnly
vfsFileAttrHidden = _pisock.vfsFileAttrHidden
vfsFileAttrSystem = _pisock.vfsFileAttrSystem
vfsFileAttrVolumeLabel = _pisock.vfsFileAttrVolumeLabel
vfsFileAttrDirectory = _pisock.vfsFileAttrDirectory
vfsFileAttrArchive = _pisock.vfsFileAttrArchive
vfsFileAttrLink = _pisock.vfsFileAttrLink
vfsFileDateCreated = _pisock.vfsFileDateCreated
vfsFileDateModified = _pisock.vfsFileDateModified
vfsFileDateAccessed = _pisock.vfsFileDateAccessed
vfsIteratorStart = _pisock.vfsIteratorStart
vfsIteratorStop = _pisock.vfsIteratorStop
dlpErrNoError = _pisock.dlpErrNoError
dlpErrSystem = _pisock.dlpErrSystem
dlpErrIllegalReq = _pisock.dlpErrIllegalReq
dlpErrMemory = _pisock.dlpErrMemory
dlpErrParam = _pisock.dlpErrParam
dlpErrNotFound = _pisock.dlpErrNotFound
dlpErrNoneOpen = _pisock.dlpErrNoneOpen
dlpErrAlreadyOpen = _pisock.dlpErrAlreadyOpen
dlpErrTooManyOpen = _pisock.dlpErrTooManyOpen
dlpErrExists = _pisock.dlpErrExists
dlpErrOpen = _pisock.dlpErrOpen
dlpErrDeleted = _pisock.dlpErrDeleted
dlpErrBusy = _pisock.dlpErrBusy
dlpErrNotSupp = _pisock.dlpErrNotSupp
dlpErrUnused1 = _pisock.dlpErrUnused1
dlpErrReadOnly = _pisock.dlpErrReadOnly
dlpErrSpace = _pisock.dlpErrSpace
dlpErrLimit = _pisock.dlpErrLimit
dlpErrSync = _pisock.dlpErrSync
dlpErrWrapper = _pisock.dlpErrWrapper
dlpErrArgument = _pisock.dlpErrArgument
dlpErrSize = _pisock.dlpErrSize
dlpErrUnknown = _pisock.dlpErrUnknown

dlp_set_protocol_version = _pisock.dlp_set_protocol_version

dlp_ptohdate = _pisock.dlp_ptohdate

dlp_htopdate = _pisock.dlp_htopdate

dlp_GetSysDateTime_ = _pisock.dlp_GetSysDateTime_

dlp_SetSysDateTime = _pisock.dlp_SetSysDateTime

dlp_ReadSysInfo = _pisock.dlp_ReadSysInfo

dlp_ReadStorageInfo = _pisock.dlp_ReadStorageInfo

dlp_ReadUserInfo = _pisock.dlp_ReadUserInfo

dlp_WriteUserInfo = _pisock.dlp_WriteUserInfo

dlp_ResetLastSyncPC = _pisock.dlp_ResetLastSyncPC

dlp_ReadNetSyncInfo = _pisock.dlp_ReadNetSyncInfo

dlp_WriteNetSyncInfo = _pisock.dlp_WriteNetSyncInfo

dlp_OpenConduit = _pisock.dlp_OpenConduit

dlp_EndOfSync = _pisock.dlp_EndOfSync

dlp_AbortSync = _pisock.dlp_AbortSync

dlp_ReadFeature = _pisock.dlp_ReadFeature

dlp_GetROMToken = _pisock.dlp_GetROMToken

dlp_AddSyncLogEntry = _pisock.dlp_AddSyncLogEntry

dlp_CallApplication = _pisock.dlp_CallApplication

dlp_ReadAppPreference = _pisock.dlp_ReadAppPreference

dlp_WriteAppPreference = _pisock.dlp_WriteAppPreference

dlp_ResetSystem = _pisock.dlp_ResetSystem

dlp_ReadDBList_ = _pisock.dlp_ReadDBList_

dlp_FindDBByName = _pisock.dlp_FindDBByName

dlp_FindDBByOpenHandle = _pisock.dlp_FindDBByOpenHandle

dlp_FindDBByTypeCreator = _pisock.dlp_FindDBByTypeCreator

dlp_FindDBInfo = _pisock.dlp_FindDBInfo

dlp_OpenDB = _pisock.dlp_OpenDB

dlp_CloseDB = _pisock.dlp_CloseDB

dlp_CloseDB_All = _pisock.dlp_CloseDB_All

dlp_DeleteDB = _pisock.dlp_DeleteDB

dlp_CreateDB = _pisock.dlp_CreateDB

dlp_ReadOpenDBInfo = _pisock.dlp_ReadOpenDBInfo

dlp_SetDBInfo = _pisock.dlp_SetDBInfo

dlp_DeleteCategory = _pisock.dlp_DeleteCategory

dlp_MoveCategory = _pisock.dlp_MoveCategory

dlp_ReadAppBlock = _pisock.dlp_ReadAppBlock

dlp_WriteAppBlock = _pisock.dlp_WriteAppBlock

dlp_ReadSortBlock = _pisock.dlp_ReadSortBlock

dlp_WriteSortBlock = _pisock.dlp_WriteSortBlock

dlp_CleanUpDatabase = _pisock.dlp_CleanUpDatabase

dlp_ResetSyncFlags = _pisock.dlp_ResetSyncFlags

dlp_ResetDBIndex = _pisock.dlp_ResetDBIndex

dlp_ReadRecordById = _pisock.dlp_ReadRecordById

dlp_ReadRecordByIndex = _pisock.dlp_ReadRecordByIndex

dlp_ReadNextModifiedRec = _pisock.dlp_ReadNextModifiedRec

dlp_ReadNextModifiedRecInCategory = _pisock.dlp_ReadNextModifiedRecInCategory

dlp_ReadNextRecInCategory = _pisock.dlp_ReadNextRecInCategory

dlp_WriteRecord = _pisock.dlp_WriteRecord

dlp_DeleteRecord = _pisock.dlp_DeleteRecord

dlp_ReadResourceByType = _pisock.dlp_ReadResourceByType

dlp_ReadResourceByIndex = _pisock.dlp_ReadResourceByIndex

dlp_WriteResource = _pisock.dlp_WriteResource

dlp_DeleteResource = _pisock.dlp_DeleteResource

dlp_ExpSlotEnumerate = _pisock.dlp_ExpSlotEnumerate

dlp_ExpCardPresent = _pisock.dlp_ExpCardPresent

dlp_ExpCardInfo = _pisock.dlp_ExpCardInfo

dlp_ExpSlotMediaType = _pisock.dlp_ExpSlotMediaType

dlp_VFSVolumeEnumerate = _pisock.dlp_VFSVolumeEnumerate

dlp_VFSVolumeInfo = _pisock.dlp_VFSVolumeInfo

dlp_VFSVolumeGetLabel = _pisock.dlp_VFSVolumeGetLabel

dlp_VFSVolumeSetLabel = _pisock.dlp_VFSVolumeSetLabel

dlp_VFSVolumeSize = _pisock.dlp_VFSVolumeSize

dlp_VFSVolumeFormat = _pisock.dlp_VFSVolumeFormat

dlp_VFSGetDefaultDir = _pisock.dlp_VFSGetDefaultDir

dlp_VFSDirEntryEnumerate = _pisock.dlp_VFSDirEntryEnumerate

dlp_VFSDirCreate = _pisock.dlp_VFSDirCreate

dlp_VFSImportDatabaseFromFile = _pisock.dlp_VFSImportDatabaseFromFile

dlp_VFSExportDatabaseToFile = _pisock.dlp_VFSExportDatabaseToFile

dlp_VFSFileCreate = _pisock.dlp_VFSFileCreate

dlp_VFSFileOpen = _pisock.dlp_VFSFileOpen

dlp_VFSFileClose = _pisock.dlp_VFSFileClose

dlp_VFSFileWrite = _pisock.dlp_VFSFileWrite

dlp_VFSFileRead = _pisock.dlp_VFSFileRead

dlp_VFSFileDelete = _pisock.dlp_VFSFileDelete

dlp_VFSFileRename = _pisock.dlp_VFSFileRename

dlp_VFSFileEOF = _pisock.dlp_VFSFileEOF

dlp_VFSFileTell = _pisock.dlp_VFSFileTell

dlp_VFSFileGetAttributes = _pisock.dlp_VFSFileGetAttributes

dlp_VFSFileSetAttributes = _pisock.dlp_VFSFileSetAttributes

dlp_VFSFileGetDate = _pisock.dlp_VFSFileGetDate

dlp_VFSFileSetDate = _pisock.dlp_VFSFileSetDate

dlp_VFSFileSeek = _pisock.dlp_VFSFileSeek

dlp_VFSFileResize = _pisock.dlp_VFSFileResize

dlp_VFSFileSize = _pisock.dlp_VFSFileSize


