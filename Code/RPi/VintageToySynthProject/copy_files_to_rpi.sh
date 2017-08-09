#!/bin/bash
# Run this script to copy all needed files to the RPi.
# This will includes the compiled vintageBrain binary, as well as all needed source files for compiling vintageSoundEngine on the RPi.

# The scripts expects the IP address of the target RPi as an argument.
# http://stackoverflow.com/questions/6482377/bash-shell-script-check-input-argument

if [ -z "$1" ]
then
echo "No IP address supplied!"
exit 1
fi

echo "Copying files to the RPi with IP address $1"
scp -r "/Users/Liam/Documents/Vintage toy synth project/Code/RPi/VintageToySynthProject/VintageBrain/"* root@$1:/usr/VintageToySynthProject/VintageBrain/

scp -r "/Users/Liam/Documents/Vintage toy synth project/Code/RPi/VintageToySynthProject/VintageSoundEngine/"* root@$1:/usr/VintageToySynthProject/VintageSoundEngine/

scp -r "/Users/Liam/Documents/Vintage toy synth project/Code/RPi/VintageToySynthProject/globals.h" "/Users/Liam/Documents/Vintage toy synth project/Code/RPi/VintageToySynthProject/vintageToySynthStartup.sh" "/Users/Liam/Documents/Vintage toy synth project/Code/RPi/VintageToySynthProject/vintageToySynthStartup.service" "/Users/Liam/Documents/Vintage toy synth project/Code/RPi/VintageToySynthProject/start.sh" "/Users/Liam/Documents/Vintage toy synth project/Code/RPi/VintageToySynthProject/stop.sh" root@$1:/usr/VintageToySynthProject/
