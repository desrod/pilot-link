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

class OnlineTestCase(unittest.TestCase):
    def setUp(self):
        pass
        
    def tearDown(self):
        pass

    def testdlp_AddSyncLogEntry(self):
        pisock.dlp_AddSyncLogEntry(sd, "Test Completed.")

    def testdlp_ReadUserInfo(self):
        assert len(pisock.dlp_ReadUserInfo(sd)) > 3


    def testdlp_ReadDBList(self):
        lst = pisock.dlp_ReadDBList(sd,0,pisock.dlpDBListRAM)
        assert len(lst) > 3
        assert lst[0].has_key('name')

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
    (options, args) = parser.parse_args()

    runner = unittest.TextTestRunner()

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
        pisock.dlp_ReadSysInfo(sd)
        pisock.dlp_OpenConduit(sd)
        print "Connected"
        runner.run(combinedSuite)
        pisock.pi_close(sd)
        print "Disconnected"
    else:
        print "Running offline tests only"
        runner.run(offlineSuite)        
    
