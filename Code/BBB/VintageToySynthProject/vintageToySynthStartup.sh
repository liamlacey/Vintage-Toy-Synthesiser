#! /bin/bash

#This is the script run on startup to prep and start the vintage toy synth applciations.

#To get the startup script and service working, do the following:
# 1. Place this file in /usr/bin/ on the BBB
# 2. Grant the file execute permissions - chmod u+x /usr/bin/vintageToySynthStartup.sh
# 3. Place vintageToySynthStartup.service into /lib/systemd/system/ on the BBB
# 4. Create a symbolic link to the service file:
#       - cd /etc/systemd/system/ 
#       - ln -s /lib/systemd/system/vintageToySynthStartup.service vintageToySynthStartup.service
# 5. Make systemd take notice of it, activate the service immediately and enable the service to be started on boot-up:
#       - systemctl daemon-reload
#       - systemctl start vintageToySynthStartup.service
#       - ystemctl enable vintageToySynthStartup.service

#The guide I followed to get the startup script working: 
# - http://www.nunoalves.com/open_source/?p=308
# - http://mybeagleboneblackfindings.blogspot.co.uk/2013/10/running-script-on-beaglebone-black-boot.html

#FIXME: should I do what's described here for VB and VSE and remove the sleeps? 
#http://askubuntu.com/questions/137776/starting-multiple-applications-using-a-shell-script

#set CPU frequency
echo "Setting CPU frequency"
cpufreq-set -g userspace
cpufreq-set -f 1000Mhz

#init serial ports
echo "Initing serial ports"
/usr/VintageToySynthProject/VintageBrain/initSerialPorts

sleep 1

#start vintageBrain application
echo "Starting vintageBrain"
/usr/VintageToySynthProject/VintageBrain/vintageBrain

sleep 1

#start vintageSoundEngine application
echo "Starting vintageSoundEngine"
/usr/VintageToySynthProject/VintageBrain/vintageSoundEngine
