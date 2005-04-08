import pisock
import datetime, warnings

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
        except pisock.dlperror:
            if pisock.pi_palmos_error(sd) == pisock.dlpErrNotFound:
                return ret
            raise

def pi_bind(sd, port="/dev/pilot"):
    try:
        pisock.pi_bind_(sd, port)
    except pisock.error, e:
        if e[0] == -502:
            # probably a bad port
            raise pisock.error( (e[0], 'generic error opening port %s' % port ) )
        else:
            raise # let the original exception carry on up the chain.

def dlp_GetSysDateTime(sd):
    r = pisock.dlp_GetSysDateTime_(sd)
    warnings.warn("dlp_GetSysDateTime() may be returning incorrect values?")
    return datetime.datetime.fromtimestamp(r)
