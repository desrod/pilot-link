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
#pdapilot.Bind(socket, {'family': pdapilot.PI_AF_SLP, 'port': pdapilot.PI_PilotSocketDLP,
#	'device': port})
#
#pdapilot.Listen(socket,1)

print "Now press the HotSync button\n"

dlp = pdapilot.Accept(socket)

ui = dlp.GetUserInfo()

b = dlp.Battery()

print "Battery voltage is ", b[0], " (warning marker is ", b[1],", critical marker ", b[2], ")\n"

db = dlp.Open("MemoDB")

r = db.GetRecord(0)

print "Memo record 0 has ID ", r[2], " attribue ", r[3], ", and category ",r[4],"\n";

r2 = pdapilot.MemoUnpack(r[0]);

print "Contents: '", r2["text"], "'\n";

r3 = db.Unpack(r[0])

print "Contents (through default unpacker):", r3

app = db.GetAppBlock()

app2 = pdapilot.MemoUnpackAppBlock(app)

print "Categories are", app2["category"],"\n"

app3 = db.UnpackAppBlock(app)

print "Categories (through default unpacker) are:", app3

del db # Close database

del dlp; # Close connection

print "Your name is ", ui["name"], "\n";

