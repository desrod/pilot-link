import pisock

def dlp_ReadDBList(sd, cardno=0, flags=None):
    ret = []
    i = 0
    if flags is None:
        flags = pisock.dlpDBListRAM
    while True:
        try:
            for db in pisock.dlp_ReadDBList_(sd, cardno, pisock.dlpDBListMultiple | flags, i):
                i = db['index'] + 1
                ret.append(db)
        except pisock.dlperror:
            if pisock.pi_palmos_error(sd) == pisock.dlpErrNotFound:
                return ret
            else:
                raise

