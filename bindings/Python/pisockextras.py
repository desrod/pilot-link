import pisock

def dlp_ReadDBList(sd, cardno=0, flags=None):
    ret = []
    i = 0
    if flags is None:
        flags = pisock.dlpDBListRAM
    while True:
        try:
            lst = pisock.dlp_ReadDBList_(sd, cardno, pisock.dlpDBListMultiple | flags, i)
            if lst==None or len(lst)==0:
                return ret
            for db in lst:
                i = db['index'] + 1
                ret.append(db)
        except pisock.dlperror:
            if pisock.pi_palmos_error(sd) == pisock.dlpErrNotFound:
                return ret
            raise
