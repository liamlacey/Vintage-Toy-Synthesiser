# Makefile to build the vintageSoundEngine application for the BeagleBone Black, using the native BBB compiler
# Command to call this makefile: make

all: vintageSoundEngine

vintageSoundEngine: vintageSoundEngine.cpp vintageVoice.cpp  Maximilian/maximilian.cpp Maximilian/RtAudio.cpp
	g++ -Wall -D__LINUX_ALSA__ -o vintageSoundEngine Maximilian/RtAudio.cpp vintageSoundEngine.cpp vintageVoice.cpp Maximilian/maximilian.cpp -lasound -lpthread -O3 -funroll-loops -finline-functions

clean:
	rm -f vintageSoundEngine

# -std=c++0x OR -std=gnu++0x <--- goes after -WALL