
use PDA::Pilot;
use Data::Dumper;

$socket = PDA::Pilot::pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP);

print "socket: $socket\n";

$addr = pack("S S a12", PI_AF_SLP, PI_PilotSocketDLP, "/dev/ttyd2");

print "addr: $addr\n";
$value = PDA::Pilot::pi_bind($socket, $addr, length($addr));

print "bind: $value\n";

$value = PDA::Pilot::pi_listen($socket, 5);

print "listen: $value\n";

$len = 0;
$data = PDA::Pilot::pi_accept($socket, $addr, $len);

print "accept: $data\n";
print "addr: $addr\n";

$handle = 0;

$value = PDA::Pilot::dlp_OpenDB($data, 0, 0x80, "MemoDB", $handle);

print "opendb: $value\n";
print "handle: $handle\n";

$r_id = 0;
$r_len = 0xffff;
$r_attr = 0;
$r_cat = 0;
$index = 0;
$retval = PDA::Pilot::dlp_ReadRecordByIndex($data, $handle, $index, $dbuf, $r_id, $r_len, $r_attr, $r_cat);
print "Record 0: $retval, $r_id, $dbuf\n";

$value = PDA::Pilot::dlp_CloseDB($data, $handle);

print "closedb: $value\n";

__END__;

@d = qw(10 00 12 00 ba 47 74 5e 07 01 02 00 ba 47 02 22
        00 00 54 65 73 74 00 54 68 69 73 20 61 20 6e 6f
        74 65 2e 00);
  
@a = qw(00 00 55 6e 66 69 6c 65 64 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   
00 00 00 00 00 00 00 00);


$x = join("",map(chr(hex($_)), @d));
$xa = join("",map(chr(hex($_)), @a));  

$y = PDA::Pilot::unpack_Appointment($x);
$z = PDA::Pilot::unpack_AppointmentAppInfo($xa);

print Dumper($y);
#print Dumper($z);

