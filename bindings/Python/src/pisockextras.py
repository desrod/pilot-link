import pisock
import datetime

def dlp_ReadDBList(sd, cardno=0, flags=None):
    ret = []
    i = 0
    if flags is None:
        flags = pisock.dlpDBListRAM
    while True:
        try:
            lst = pisock.dlp_ReadDBList_(sd, cardno, pisock.dlpDBListMultiple | flags, i)
            if (lst is None) or (len(lst) == 0):
                return ret
            for db in lst:
                i = db['index'] + 1
                ret.append(db)
        except pisock.error:
            if pisock.pi_palmos_error(sd)==pisock.dlpErrNotFound:
                return ret
            raise

def dlp_GetSysDateTime(sd):
    r = pisock.dlp_GetSysDateTime_(sd)
    return datetime.datetime.fromtimestamp(r)
