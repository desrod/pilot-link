# This file was created automatically by SWIG.
# Don't modify this file, modify the SWIG interface instead.
# This file is compatible with both classic and new-style classes.

import _pisock

def _swig_setattr(self,class_type,name,value):
    if (name == "this"):
        if isinstance(value, class_type):
            self.__dict__[name] = value.this
            if hasattr(value,"thisown"): self.__dict__["thisown"] = value.thisown
            del value.thisown
            return
    method = class_type.__swig_setmethods__.get(name,None)
    if method: return method(self,value)
    self.__dict__[name] = value

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


PI_AF_PILOT = _pisock.PI_AF_PILOT
PI_PF_DEV = _pisock.PI_PF_DEV
PI_PF_SLP = _pisock.PI_PF_SLP
PI_PF_SYS = _pisock.PI_PF_SYS
PI_PF_PADP = _pisock.PI_PF_PADP
PI_PF_NET = _pisock.PI_PF_NET
PI_PF_DLP = _pisock.PI_PF_DLP
PI_SOCK_STREAM = _pisock.PI_SOCK_STREAM
PI_SOCK_RAW = _pisock.PI_SOCK_RAW
PI_CMD_CMP = _pisock.PI_CMD_CMP
PI_CMD_NET = _pisock.PI_CMD_NET
PI_CMD_SYS = _pisock.PI_CMD_SYS
PI_MSG_PEEK = _pisock.PI_MSG_PEEK
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
PI_CMP_TYPE = _pisock.PI_CMP_TYPE
PI_CMP_FLAGS = _pisock.PI_CMP_FLAGS
PI_CMP_VERS = _pisock.PI_CMP_VERS
PI_CMP_BAUD = _pisock.PI_CMP_BAUD
PI_NET_TYPE = _pisock.PI_NET_TYPE
PI_SOCK_STATE = _pisock.PI_SOCK_STATE
PI_PilotSocketDLP = _pisock.PI_PilotSocketDLP
PI_PilotSocketConsole = _pisock.PI_PilotSocketConsole
PI_PilotSocketDebugger = _pisock.PI_PilotSocketDebugger
PI_PilotSocketRemoteUI = _pisock.PI_PilotSocketRemoteUI

pilot_connect = _pisock.pilot_connect

pi_socket = _pisock.pi_socket

pi_connect = _pisock.pi_connect

pi_bind = _pisock.pi_bind

pi_listen = _pisock.pi_listen

pi_accept = _pisock.pi_accept

pi_accept_to = _pisock.pi_accept_to

pi_send = _pisock.pi_send

pi_recv = _pisock.pi_recv

pi_read = _pisock.pi_read

pi_write = _pisock.pi_write

pi_getsockname = _pisock.pi_getsockname

pi_getsockpeer = _pisock.pi_getsockpeer

pi_setsockopt = _pisock.pi_setsockopt

pi_getsockopt = _pisock.pi_getsockopt

pi_version = _pisock.pi_version

pi_tickle = _pisock.pi_tickle

pi_watchdog = _pisock.pi_watchdog

pi_close = _pisock.pi_close
dlpDBFlagResource = _pisock.dlpDBFlagResource
dlpDBFlagReadOnly = _pisock.dlpDBFlagReadOnly
dlpDBFlagAppInfoDirty = _pisock.dlpDBFlagAppInfoDirty
dlpDBFlagBackup = _pisock.dlpDBFlagBackup
dlpDBFlagClipping = _pisock.dlpDBFlagClipping
dlpDBFlagOpen = _pisock.dlpDBFlagOpen
dlpDBFlagNewer = _pisock.dlpDBFlagNewer
dlpDBFlagReset = _pisock.dlpDBFlagReset
dlpDBFlagCopyPrevention = _pisock.dlpDBFlagCopyPrevention
dlpDBFlagStream = _pisock.dlpDBFlagStream
dlpDBMiscFlagExcludeFromSync = _pisock.dlpDBMiscFlagExcludeFromSync
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
dlpEndCodeNormal = _pisock.dlpEndCodeNormal
dlpEndCodeOutOfMemory = _pisock.dlpEndCodeOutOfMemory
dlpEndCodeUserCan = _pisock.dlpEndCodeUserCan
dlpEndCodeOther = _pisock.dlpEndCodeOther
dlpDBListRAM = _pisock.dlpDBListRAM
dlpDBListROM = _pisock.dlpDBListROM
dlpErrNoError = _pisock.dlpErrNoError
dlpErrSystem = _pisock.dlpErrSystem
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

dlp_strerror = _pisock.dlp_strerror

dlp_GetSysDateTime = _pisock.dlp_GetSysDateTime

dlp_SetSysDateTime = _pisock.dlp_SetSysDateTime

dlp_ReadStorageInfo = _pisock.dlp_ReadStorageInfo

dlp_ReadSysInfo = _pisock.dlp_ReadSysInfo

dlp_ReadDBList = _pisock.dlp_ReadDBList

dlp_FindDBInfo = _pisock.dlp_FindDBInfo

dlp_OpenDB = _pisock.dlp_OpenDB

dlp_CloseDB = _pisock.dlp_CloseDB

dlp_CloseDB_All = _pisock.dlp_CloseDB_All

dlp_DeleteDB = _pisock.dlp_DeleteDB

dlp_CreateDB = _pisock.dlp_CreateDB

dlp_ResetSystem = _pisock.dlp_ResetSystem

dlp_AddSyncLogEntry = _pisock.dlp_AddSyncLogEntry

dlp_OpenConduit = _pisock.dlp_OpenConduit

dlp_EndOfSync = _pisock.dlp_EndOfSync

dlp_AbortSync = _pisock.dlp_AbortSync

dlp_ReadOpenDBInfo = _pisock.dlp_ReadOpenDBInfo

dlp_MoveCategory = _pisock.dlp_MoveCategory

dlp_WriteUserInfo = _pisock.dlp_WriteUserInfo

dlp_ReadUserInfo = _pisock.dlp_ReadUserInfo

dlp_ResetLastSyncPC = _pisock.dlp_ResetLastSyncPC

dlp_WriteAppBlock = _pisock.dlp_WriteAppBlock

dlp_WriteSortBlock = _pisock.dlp_WriteSortBlock

dlp_ResetDBIndex = _pisock.dlp_ResetDBIndex

dlp_WriteRecord = _pisock.dlp_WriteRecord

dlp_DeleteRecord = _pisock.dlp_DeleteRecord

dlp_DeleteCategory = _pisock.dlp_DeleteCategory

dlp_ReadResourceByType = _pisock.dlp_ReadResourceByType

dlp_ReadResourceByIndex = _pisock.dlp_ReadResourceByIndex

dlp_WriteResource = _pisock.dlp_WriteResource

dlp_DeleteResource = _pisock.dlp_DeleteResource

dlp_ReadNextModifiedRec = _pisock.dlp_ReadNextModifiedRec

dlp_ReadNextModifiedRecInCategory = _pisock.dlp_ReadNextModifiedRecInCategory

dlp_ReadNextRecInCategory = _pisock.dlp_ReadNextRecInCategory

dlp_ReadRecordById = _pisock.dlp_ReadRecordById

dlp_ReadRecordByIndex = _pisock.dlp_ReadRecordByIndex

dlp_CleanUpDatabase = _pisock.dlp_CleanUpDatabase

dlp_ResetSyncFlags = _pisock.dlp_ResetSyncFlags

dlp_ReadFeature = _pisock.dlp_ReadFeature

dlp_ReadNetSyncInfo = _pisock.dlp_ReadNetSyncInfo

dlp_WriteNetSyncInfo = _pisock.dlp_WriteNetSyncInfo

dlp_ReadAppPreference = _pisock.dlp_ReadAppPreference

dlp_WriteAppPreference = _pisock.dlp_WriteAppPreference

pi_file_open = _pisock.pi_file_open

pi_file_close = _pisock.pi_file_close

pi_file_get_info = _pisock.pi_file_get_info

pi_file_get_app_info = _pisock.pi_file_get_app_info

pi_file_get_sort_info = _pisock.pi_file_get_sort_info

pi_file_read_resource = _pisock.pi_file_read_resource

pi_file_read_resource_by_type_id = _pisock.pi_file_read_resource_by_type_id

pi_file_type_id_used = _pisock.pi_file_type_id_used

pi_file_read_record = _pisock.pi_file_read_record

pi_file_get_entries = _pisock.pi_file_get_entries

pi_file_read_record_by_id = _pisock.pi_file_read_record_by_id

pi_file_id_used = _pisock.pi_file_id_used

pi_file_create = _pisock.pi_file_create

pi_file_set_info = _pisock.pi_file_set_info

pi_file_set_app_info = _pisock.pi_file_set_app_info

pi_file_set_sort_info = _pisock.pi_file_set_sort_info

pi_file_append_resource = _pisock.pi_file_append_resource

pi_file_append_record = _pisock.pi_file_append_record

pi_file_retrieve = _pisock.pi_file_retrieve

pi_file_install = _pisock.pi_file_install

pi_file_merge = _pisock.pi_file_merge
cvar = _pisock.cvar
dlp_ReadAppBlock = _pisock.dlp_ReadAppBlock

dlp_ReadSortBlock = _pisock.dlp_ReadSortBlock

dlp_ReadRecordIDList = _pisock.dlp_ReadRecordIDList


