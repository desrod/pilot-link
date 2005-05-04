import unittest
import sys,glob,os
from optparse import OptionParser

builds = glob.glob("../build/lib*")
if len(builds) != 1:
    print "This little hack only works when you've got one build compiled."
    print "python setup.py clean; python setup.py build"
    print "and try again."
    os.exit(10)
sys.path.insert(0,os.path.join(os.path.abspath("."),builds[0]))

import pisock

def dumpDBInfo(dbi):
    print '\tDatabase info:'
    print '\t\tname:',dbi['name']
    print '\t\ttype:',dbi['type']
    print '\t\tcreator:',dbi['creator']
    print '\t\tversion:',dbi['version']
    print '\t\tflags: 0x%08lx' % (dbi['flags'])
    print '\t\tmiscFlags: 0x%08lx' % (dbi['miscFlags'])
    print '\t\tmod. count:',dbi['modnum']
    print '\t\tcreated:',dbi['createDate']
    print '\t\tmodified',dbi['modifyDate']
    print '\t\tbackup:',dbi['backupDate']

def dumpDBSizeInfo(si):
    print '\tDBSizeInfo:'
    print '\t\tnumRecords:',si['numRecords']
    print '\t\ttotalBytes:',si['totalBytes']
    print '\t\tdataBytes:',si['dataBytes']
    print '\t\tappBlockSize:',si['appBlockSize']
    print '\t\tsortBlockSize:',si['sortBlockSize']
    print '\t\tmaxRecSize:',si['maxRecSize']

class OnlineTestCase(unittest.TestCase):
    def setUp(self):
        try:
            pisock.dlp_DeleteDB(sd,0,'PythonTestSuite')
        except:
            pass

    def tearDown(self):
        pass

    def testGetSysDateTime(self):
        res = pisock.dlp_GetSysDateTime(sd)
        if VERBOSE:
            print 'GetSysDateTime:',res

    def testAddSyncLogEntry(self):
        pisock.dlp_AddSyncLogEntry(sd, "Python test.")

    def testReadSysInfo(self):
        res = pisock.dlp_ReadSysInfo(sd)
        assert res!=None and res.has_key('romVersion')
        if VERBOSE:
            print 'ReadSysInfo: romVersion=%s locale=%s name=%s' % (
                hex(res['romVersion']),
                hex(res['locale']),
                res['name'])

    def testReadStorageInfo(self):
        res = pisock.dlp_ReadStorageInfo(sd,0)
        assert res.has_key('manufacturer')
        if VERBOSE:
            print 'ReadStorageInfo: card 0, romSize=%s ramSize=%s ramFree=%s manufacturer=%s' % (
                res['romSize'],
                res['ramSize'],
                res['ramFree'],
                res['manufacturer'])

    def testReadUserInfo(self):
        res = pisock.dlp_ReadUserInfo(sd)
        assert res!=None and res.has_key('name')
        if VERBOSE:
            print 'ReadUserInfo: username=%s' % res['name']

    def testReadNetSyncInfo(self):
        res = pisock.dlp_ReadNetSyncInfo(sd)
        assert res!=None and res.has_key('hostName')
        if VERBOSE:
            print "ReadNetSyncInfo: lanSync=%d hostname='%s' hostaddress='%s' subnetmask='%s'" % (
                res['lanSync'],
                res['hostName'],
                res['hostAddress'],
                res['hostSubnetMask'])

    def testReadFeature(self):
        res = pisock.dlp_ReadFeature(sd,'psys',2)
        assert res!=None
        if VERBOSE: 
            print "ReadFeature: processor type=%s" % hex(res)

    def testReadStorageInfo(self):
        more = 1
        i = 0
        while more:
            res = pisock.dlp_ReadStorageInfo(sd,i)
            assert res!=None and res.has_key('name')
            if VERBOSE:
                print "ReadStorageInfo: card=%d version=%d creation=%ld romSize=%ld ramSize=%ld ramFree=%ld name='%s' manufacturer='%s'" % (
                    res['card'],
                    res['version'],
                    res['creation'],
                    res['romSize'],
                    res['ramSize'],
                    res['ramFree'],
                    res['name'],
                    res['manufacturer'])
            if res['more'] == 0:
                break

    def testReadDBList(self):
        res = pisock.dlp_ReadDBList(sd,0,pisock.dlpDBListRAM)
        assert len(res) > 3
        assert res[0].has_key('name')
        if VERBOSE:
            print "ReadDBList: %s entries" % len(res)

    def testDatabaseCreationAndSearch(self):
        db = pisock.dlp_CreateDB(sd,'test','DATA',0,0,1,'PythonTestSuite')
        assert db != None
        pisock.dlp_SetDBInfo(sd, db, pisock.dlpDBFlagBackup, pisock.dlpDBFlagCopyPrevention, 0, 0, 0, 0, 'data', 0)
        pisock.dlp_CloseDB(sd,db)
        db = pisock.dlp_OpenDB(sd, 0, pisock.dlpOpenReadWrite, 'PythonTestSuite')
        assert db != None
        info = pisock.dlp_FindDBByOpenHandle(sd, db)
        if VERBOSE:
            print 'FindDBByOpenHandle:'
            print '\tcardNo',info[0]
            print '\tlocalID',info[1]
            dumpDBInfo(info[2])
            dumpDBSizeInfo(info[3])
        pisock.dlp_CloseDB(sd,db)
        pisock.dlp_DeleteDB(sd,0,'PythonTestSuite')

class OfflineTestCase(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    def testBadPort(self):
        sd = pisock.pi_socket(pisock.PI_AF_PILOT,
                              pisock.PI_SOCK_STREAM,
                              pisock.PI_PF_DLP)
        self.assertRaises(pisock.error, pisock.pi_bind, sd, "/dev/nosuchport")

onlineSuite = unittest.makeSuite(OnlineTestCase,'test')
offlineSuite = unittest.makeSuite(OfflineTestCase,'test')
combinedSuite = unittest.TestSuite((onlineSuite, offlineSuite))

if __name__ == "__main__":
    parser = OptionParser()
    parser.add_option("-p", "--port", dest="pilotport",
                      help="Perform online tests using port", metavar="PORT")
    parser.add_option("-v", "--verbose", dest="verbose", action="store_true",
                      help="Print more output", default=0)
    (options, args) = parser.parse_args()

    runner = unittest.TextTestRunner()

    VERBOSE = options.verbose

    if options.pilotport:
        
        pilotport = options.pilotport
        print "Running online and offline tests using port %s" % pilotport
        print "Connecting"
        sd = pisock.pi_socket(pisock.PI_AF_PILOT,
                              pisock.PI_SOCK_STREAM,
                              pisock.PI_PF_DLP)
        
        pisock.pi_bind(sd, pilotport)
        pisock.pi_listen(sd, 1)
        pisock.pi_accept(sd)
        if VERBOSE:
            print pisock.dlp_ReadSysInfo(sd)
        else:
            pisock.dlp_ReadSysInfo(sd)
        pisock.dlp_OpenConduit(sd)
        print "Connected"
        runner.run(combinedSuite)
        pisock.pi_close(sd)
        print "Disconnected"
    else:
        print "Running offline tests only"
        runner.run(offlineSuite)        
    
