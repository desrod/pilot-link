
set bar 0

proc accept {socket} {
	dlpStatus $socket
	puts "Status $socket\n"
	set o [dlpOpen $socket "MemoDB" 0 "rs"]
	puts "Open $o"
	set p [dlpPackers $socket]
	puts "Packers $p"
	set r [dlpGetRecord $socket $o 0]
	puts "Record $r"
	close $socket
}

puts [piOpen -server accept /dev/cua3]

vwait bar
