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
sd = pisock.pilot_connect(port)
if sd == -1:
    print "Unable to connect."
    sys.exit(10)

try:
    pisock.dlp_OpenConduit(sd)
except pisock.error, e:
    print e
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
