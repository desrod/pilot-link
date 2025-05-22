echo '****** pilot-xfer -p /dev/ttyUSB1 -C ******'
echo '************ Hit HotSync button ! ************'
sleep 3
dist/bin/pilot-xfer -p /dev/ttyUSB1 -C
echo '****** pilot-xfer -p /dev/ttyUSB1 -l -D "BUILTIN" ******'
echo '************ Hit HotSync button ! ************'
sleep 8
dist/bin/pilot-xfer -p /dev/ttyUSB1 -l -D "BUILTIN"
echo '****** pilot-xfer -p /dev/ttyUSB1 -l -D "/BUILTIN/BLAZER" ******'
echo '************ Hit HotSync button ! ************'
sleep 8
dist/bin/pilot-xfer -p /dev/ttyUSB1 -l -D "/BUILTIN/BLAZER"
echo '****** pilot-xfer -p /dev/ttyUSB1 -l -D "/BUILTIN/PALM_DM/" ******'
echo '************ Hit HotSync button ! ************'
sleep 8
dist/bin/pilot-xfer -p /dev/ttyUSB1 -l -D "/BUILTIN/PALM_DM/"
echo '****** pilot-xfer -p /dev/ttyUSB1 -l -D "/BUILTIN/PALM_DM/ReserveFile" ******'
echo '************ Hit HotSync button ! ************'
sleep 8
dist/bin/pilot-xfer -p /dev/ttyUSB1 -l -D "/BUILTIN/PALM_DM/ReserveFile"
echo '****** pilot-xfer -p /dev/ttyUSB1 -l -D "/BUILTIN/SpaceHolderFile/" ******'
echo '************ Hit HotSync button ! ************'
sleep 8
dist/bin/pilot-xfer -p /dev/ttyUSB1 -l -D "/BUILTIN/SpaceHolderFile/"
echo '****** pilot-xfer -p /dev/ttyUSB1 -l -D "/BUILTIN/Photos & Videos" ******'
echo '************ Hit HotSync button ! ************'
sleep 8
dist/bin/pilot-xfer -p /dev/ttyUSB1 -l -D "/BUILTIN/Photos & Videos"
echo '****** pilot-xfer -p /dev/ttyUSB1 -l -D "/" ******'
echo '************ Hit HotSync button ! ************'
sleep 8
dist/bin/pilot-xfer -p /dev/ttyUSB1 -l -D "/"
echo '****** pilot-xfer -p /dev/ttyUSB1 -l -D "/PALM" ******'
echo '************ Hit HotSync button ! ************'
sleep 8
dist/bin/pilot-xfer -p /dev/ttyUSB1 -l -D "/PALM"
echo '****** pilot-xfer -p /dev/ttyUSB1 -l -D "/PALM/Launcher" ******'
echo '************ Hit HotSync button ! ************'
sleep 8
dist/bin/pilot-xfer -p /dev/ttyUSB1 -l -D "/PALM/Launcher"
echo '****** pilot-xfer -p /dev/ttyUSB1 -l -D "/DCIM" ******'
echo '************ Hit HotSync button ! ************'
sleep 8
dist/bin/pilot-xfer -p /dev/ttyUSB1 -l -D "/DCIM"
echo '****** pilot-xfer -p /dev/ttyUSB1 -l -D "/DCIM/137DSCIM" ******'
echo '************ Hit HotSync button ! ************'
sleep 8
dist/bin/pilot-xfer -p /dev/ttyUSB1 -l -D "/DCIM/137DSCIM"
echo '****** pilot-xfer -p /dev/ttyUSB1 -l -D "card1/not_existing" ******'
echo '************ Hit HotSync button ! ************'
sleep 8
dist/bin/pilot-xfer -p /dev/ttyUSB1 -l -D "card1/not_existing"
echo '****** pilot-xfer -p /dev/ttyUSB1 -l -D "card9/test" ******'
echo '************ Hit HotSync button ! ************'
sleep 8
dist/bin/pilot-xfer -p /dev/ttyUSB1 -l -D "card9/test"
echo '****** pilot-xfer -p /dev/ttyUSB1 -l -D "/non/sense" ******'
echo '************ Hit HotSync button ! ************'
sleep 8
dist/bin/pilot-xfer -p /dev/ttyUSB1 -l -D "/non/sense"
