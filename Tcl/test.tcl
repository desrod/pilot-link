#!/usr/local/tcl8.0/bin/tclsh8.0

load ./PilotLink.so

set bar 0

proc accept {socket} {
	Pi::Dlp::Status $socket
	puts "Status $socket\n"
	set o [Pi::Dlp::Open $socket "MemoDB" 0 "rs"]
	puts "Open $o"
	set r [Pi::Dlp::GetRecord $socket $o 0]
	puts "Record $r"
	close $socket
}

puts [Pi::Open -server accept /dev/cua3]

vwait bar
