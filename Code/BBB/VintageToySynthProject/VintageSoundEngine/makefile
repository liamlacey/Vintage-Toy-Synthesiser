# Makefile to build the vintageSoundEngine application for the BeagleBone Black, using the native BBB compiler
# Command to call this makefile: make

all: vintageSoundEngine

#TODO: add all needed files for the vintageSoundEngine target 
vintageSoundEngine: vintageSoundEngine.cpp
	#TODO: add/corrected all needed files
	g++ -Wall -D__LINUX_ALSA__ -o -O3 vintageSoundEngine main.cpp Maximilian/RtAudio.cpp Maximilian/player.cpp Maximilian/maximilian.cpp -lasound -lpthread

clean:
	rm -f vintageSoundEngine
