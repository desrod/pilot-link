#!/usr/bin/env python

import pisock
import sys   

port = "/dev/pilot"

print "Press HotSync now"
sd = pisock.pilot_connect(port)
pisock.dlp_OpenConduit(sd)
print pisock.dlp_ReadUserInfo(sd)
pisock.dlp_AddSyncLogEntry(sd, "Test Completed.")
pisock.pi_close(sd)
