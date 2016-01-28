#!/bin/bash
# Run this script to copy all needed files to the BBB.
# This will includes the compiled vintageBrain binary, as well as all needed source files for compiling vintageSoundEngine on the BBB.

echo "Copying files to the BB with IP address 192.168.7.2"
scp -r VintageBrain/* root@192.168.7.2:/usr/VintageToySynthProject/VintageBrain/
scp -r VintageSoundEngine/* root@192.168.7.2:/usr/VintageToySynthProject/VintageSoundEngine/

