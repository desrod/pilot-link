
try:
	import pdapilot
except:
	import sys
	sys.path.append('./.libs/')
	import pdapilot

from sys import stdin

print 'Please enter the serial port [/dev/pilot]: ',
port = stdin.readline()
port = port[0:len(port)-1]

if len(port) == 0:
  port = '/dev/pilot'

print 'Using port',port

socket = pdapilot.openPort(port)

# OpenPort is the equivalent of
#
#socket = pdapilot.socket(pdapilot.PI_AF_SLP, pdapilot.PI_SOCK_STREAM, pdapilot.PI_PF_PADP);
#
#pdapilot.bind(socket, {'family': pdapilot.PI_AF_SLP, 'device': port})
#
#pdapilot.listen(socket,1)

print "Now press the HotSync button"

dlp = pdapilot.accept(socket)

print dlp.getDBInfo(0)

ui = dlp.getUserInfo()

b = dlp.getBattery()

print "Battery voltage is ", b[0], " (warning marker is ", b[1],", critical marker ", b[2], ")\n"

rpc = pdapilot.PackRPC(0xA0B6, "i", ("b", "&s", "&s", "&s", "&b", "&b"),
                                    (0,   0,    0,    0,    0,    0))
#b = dlp.RPC(rpc)

print "Battery results through Python RPC:", b

# Looks like this broke. Oh well.
#rpc = pdapilot.PackRPC(0xA220, "i", ("&*", "s", "s", "s"), 
#                                 ("Woo woo!", 8, 100, 0))
#
#dlp.RPC(rpc)

print "At open"

try:
	db = dlp.open('MailDB')
	
	appinfo = db.getAppBlock()
	
	print 'App block:', appinfo
	
	r = db.getRecord(0)
	
	print 'Record 0:', r
	
	s = db.getPref(1)

	print 'Pref 1:', s
	
	s = db.getPref(3)
	
	print 'Pref 3:', s
	
	r = db.getRecord(1)
	
	print 'Record 1:', r
	
	q = db.newPref(1)
	
	print 'Blank pref 1:', q
	
	
	p = dlp.getPref(pdapilot.Mail.creator, 1)
	
	print p
	print "Repacked: ", `p.pack()`
	
	p = dlp.getPref(pdapilot.Mail.creator, 3)
	
	print p
	print "Repacked: ", `p.pack()`
	
	# Construct a blank preference object
	p = dlp.newPref(pdapilot.Mail.creator, 1)
	print p

	p = dlp.getPrefRaw(pdapilot.Mail.creator, 1)
	print p

	db.close()
except pdapilot.error:
	0

db = dlp.open("MemoDB")

print "Class: ", db.Class

print "At getrecord"

r = db.getRecord(0)

print "Memo: ", r

x = db.newRecord()
x.text = 'a-aFooFoo!'
x.id = None
print x.pack()
print x

db.setRecord(x)

print "New memo: ", x

r = db.getAppBlock()
print "Got app block", r

r = db.newAppBlock()

print "New app block: ", r

del db # Close database

del dlp; # Close connection

print "Your name is ", ui["name"], "\n";

