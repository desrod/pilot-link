#!/usr/bin/env python

import sys

import os
import os.path

import glob

builds = glob.glob("build/lib*")
if len(builds) != 1:
    print "This little hack only works when you've got one build compiled."
    print "python setup.py clean; python setup.py build"
    print "and try again."
    os.exit(10)

sys.path.insert(0,os.path.join(os.path.abspath("."),builds[0]))

import pisock

port = "/dev/pilot"


print "Press HotSync now"
sd = pisock.pi_socket(pisock.PI_AF_PILOT,
                      pisock.PI_SOCK_STREAM,
                      pisock.PI_PF_DLP)
print sd
result = pisock.pi_bind(sd, port);
# should throw exception ?
print result
if result < 0:
    print "Unable to bind to port."
    sys.exit(10)
result = pisock.pi_listen(sd,1)
if result < 0:
    print "Unable to listen on port."
    sys.exit(10)
sd, address = pisock.pi_accept(sd);

if sd < 0:
    print "Unable to accept data on port."
    sys.exit(10)

print "Going to ReadSysInfo"
print pisock.dlp_ReadSysInfo(sd)
print "Done ReadSysInfo"

try:
    print "Going to OpenConduit"
    pisock.dlp_OpenConduit(sd)
    print "Done OpenConduit"
except pisock.error, e:
    print "Exception in OpenConduit:", e
    sys.exit(10)
    
print pisock.dlp_ReadUserInfo(sd)
i = 0
while True:
    try:
        for db in pisock.dlp_ReadDBList(sd, 0, pisock.dlpDBListMultiple | pisock.dlpDBListRAM, i):
            print db['name']
            i = db['index'] + 1
    except:
        break
pisock.dlp_AddSyncLogEntry(sd, "Test Completed.")
pisock.pi_close(sd)
