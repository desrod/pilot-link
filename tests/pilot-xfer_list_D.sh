echo '****** pilot-xfer -p /dev/ttyUSB1 -C ******'
sleep 3
dist/bin/pilot-xfer -p /dev/ttyUSB1 -C
echo '****** pilot-xfer -p /dev/ttyUSB1 -l -D "/BUILTIN" ******'
sleep 8
dist/bin/pilot-xfer -p /dev/ttyUSB1 -l -D "/BUILTIN"
echo '****** pilot-xfer -p /dev/ttyUSB1 -l -D "/BUILTIN/BLAZER" ******'
sleep 8
dist/bin/pilot-xfer -p /dev/ttyUSB1 -l -D "/BUILTIN/BLAZER"
echo '****** pilot-xfer -p /dev/ttyUSB1 -l -D "/BUILTIN/PALM_DM" ******'
sleep 8
dist/bin/pilot-xfer -p /dev/ttyUSB1 -l -D "/BUILTIN/PALM_DM"
echo '****** pilot-xfer -p /dev/ttyUSB1 -l -D "/BUILTIN/PALM_DM/ReserveFile" ******'
sleep 8
dist/bin/pilot-xfer -p /dev/ttyUSB1 -l -D "/BUILTIN/PALM_DM/ReserveFile"
echo '****** pilot-xfer -p /dev/ttyUSB1 -l -D "/BUILTIN/SpaceHolderFile" ******'
sleep 8
dist/bin/pilot-xfer -p /dev/ttyUSB1 -l -D "/BUILTIN/SpaceHolderFile"
echo '****** pilot-xfer -p /dev/ttyUSB1 -l -D "/BUILTIN/Photos & Videos" ******'
sleep 8
dist/bin/pilot-xfer -p /dev/ttyUSB1 -l -D "/BUILTIN/Photos & Videos"
echo '****** pilot-xfer -p /dev/ttyUSB1 -l -D "/" ******'
sleep 8
dist/bin/pilot-xfer -p /dev/ttyUSB1 -l -D "/"
echo '****** pilot-xfer -p /dev/ttyUSB1 -l -D "/PALM" ******'
sleep 8
dist/bin/pilot-xfer -p /dev/ttyUSB1 -l -D "/PALM"
echo '****** pilot-xfer -p /dev/ttyUSB1 -l -D "/PALM/Launcher" ******'
sleep 8
dist/bin/pilot-xfer -p /dev/ttyUSB1 -l -D "/PALM/Launcher"
echo '****** pilot-xfer -p /dev/ttyUSB1 -l -D "/DCIM" ******'
sleep 8
dist/bin/pilot-xfer -p /dev/ttyUSB1 -l -D "/DCIM"
echo '****** pilot-xfer -p /dev/ttyUSB1 -l -D "/DCIM/137DSCIM" ******'
sleep 8
dist/bin/pilot-xfer -p /dev/ttyUSB1 -l -D "/DCIM/137DSCIM"
