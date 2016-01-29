/*
 vintageVoice.cpp
 
 Created by Liam Lacey on 29/01/2016.
 Copyright 2016 Liam Lacey. All rights reserved.
 */

#ifndef ____vintageVoice__
#define ____vintageVoice__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <sched.h>
#include <time.h>
#include <netdb.h>
#include <memory.h>
#include <ifaddrs.h>
#include <stdarg.h>
#include <math.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/termios.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <linux/spi/spidev.h>
#include <linux/sockios.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <iostream>

#include "../globals.h"
#include "Maximilian/maximilian.h"

class VintageVoice
{
public:
    VintageVoice (int voice_num);
    ~VintageVoice();
    
    void processAudio (double *output);
    
    void setNotePitch (uint8_t midi_note_num);
    
private:
    
    //maximilian objects
    maxiOsc oscSquare;
    
    //'patch' parameters
    
    //other parameters
    double oscPitch;
    
//    //These are the synthesiser bits
//    maxiOsc VCO1[NUM_OF_VOICES],VCO2[NUM_OF_VOICES],LFO1[NUM_OF_VOICES];
//    maxiSVF svfVcf[NUM_OF_VOICES];
//    maxiEnv ADSR[NUM_OF_VOICES];
//    
//    //This is a bunch of control signals so that we can hear something
//    maxiOsc timer;//this is the metronome
//    int currentCount,lastCount,voice;//these values are used to check if we have a new beat this sample
//    
//    //and these are some variables we can use to pass stuff around
//    double VCO1out[NUM_OF_VOICES],VCO2out[NUM_OF_VOICES],LFO1out[NUM_OF_VOICES],VCFout[NUM_OF_VOICES],ADSRout[NUM_OF_VOICES],mix,pitch[NUM_OF_VOICES];

};

#endif /* defined(____vintageVoice__) */
