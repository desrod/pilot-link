
try:
	import pdapiot
except:
	import sys
	sys.path.append('./.libs/')
	import pdapilot

from sys import stdin

print 'Please enter the serial port [/dev/cua3]: ',
port = stdin.readline()
port = port[0:len(port)-1]

if len(port) == 0:
  port = '/dev/cua3'

print 'Using port',port

socket = pdapilot.OpenPort(port)

# OpenPort is the equivalent of
#
#socket = pdapilot.socket(pdapilot.PI_AF_SLP, pdapilot.PI_SOCK_STREAM, pdapilot.PI_PF_PADP);
#
#pdapilot.Bind(socket, {'family': pdapilot.PI_AF_SLP, 'device': port})
#
#pdapilot.Listen(socket,1)

print "Now press the HotSync button\n"

dlp = pdapilot.Accept(socket)

ui = dlp.GetUserInfo()

b = dlp.Battery()

print "Battery voltage is ", b[0], " (warning marker is ", b[1],", critical marker ", b[2], ")\n"

rpc = pdapilot.PackRPC(0xA0B6, "i", ("b", "&s", "&s", "&s", "&b", "&b"),
                                    (0,   0,    0,    0,    0,    0))

b = dlp.RPC(rpc)

print "Battery results through Python RPC:", b

# Looks like this broke. Oh well.
#rpc = pdapilot.PackRPC(0xA220, "i", ("&*", "s", "s", "s"), 
#                                 ("Woo woo!", 8, 100, 0))
#
#dlp.RPC(rpc)

print "At open"

p = dlp.GetAppPref(pdapilot.Mail.creator, 1)

print p
print "Repacked: ", `p.pack()`

p = dlp.GetAppPref(pdapilot.Mail.creator, 3)

print p
print "Repacked: ", `p.pack()`

# Construct a blank preference object
p = dlp.NewAppPref(pdapilot.Mail.creator, 1)
print p

p = dlp.GetAppPrefRaw(pdapilot.Mail.creator, 1)
print p

db = dlp.Open("MemoDB")

print "Class: ", db.Class

print "At getrecord"

r = db.GetRecord(0)

print "Memo: ", r

x = db.NewRecord()
x.text = 'a-aFooFoo!'
x.id = None
print x.pack()
print x

db.SetRecord(x)

print "New memo: ", x

r = db.GetAppBlock()
print "Got app block", r

r = db.NewAppBlock()

print "New app block: ", r

del db # Close database

del dlp; # Close connection

print "Your name is ", ui["name"], "\n";

