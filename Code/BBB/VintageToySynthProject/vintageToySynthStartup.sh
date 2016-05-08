#! /bin/bash

#This is the script run on startup to prep and start the vintage toy synth applications.

#To get the startup script and service working using systemd, do the following on the BBB:
# 1. Grant this file execute permissions - chmod u+x /usr/VintageToySynthProject/vintageToySynthStartup.sh
# 2. Copy the vintageToySynthStartup.service file into /lib/systemd/system/
# 3. Enable the service to be started on boot-up - sudo systemctl enable vintageToySynthStartup.service
# 4. Start the service - sudo systemctl start vintageToySynthStartup.service
# 5. reboot BBB to make sure the service starts on boot

# To check the status of the service - sudo systemctl status vintageToySynthStartup.service
# To check all services currently running - systemctl

#The guide I followed to get the startup script working: 
# https://learn.adafruit.com/running-programs-automatically-on-your-tiny-computer/systemd-writing-and-enabling-a-service

# Tutorial on writing service Unit files:
# https://coreos.com/docs/launching-containers/launching/getting-started-with-systemd/

#set CPU frequency
echo "Setting CPU frequency"
cpufreq-set -g userspace
cpufreq-set -f 1000Mhz
cpufreq-info

#init serial ports
echo "Initing serial ports"
/usr/VintageToySynthProject/VintageBrain/initSerialPorts

sleep 1

#start vintageBrain application
echo "Starting vintageBrain"
/usr/VintageToySynthProject/VintageBrain/vintageBrain & 

#start vintageSoundEngine application
echo "Starting vintageSoundEngine"
/usr/VintageToySynthProject/VintageSoundEngine/vintageSoundEngine & 

exit 0
