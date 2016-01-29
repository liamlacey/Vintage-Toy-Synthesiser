/*
 vintageVoice.cpp
 
 Created by Liam Lacey on 29/01/2016.
 Copyright 2016 Liam Lacey. All rights reserved.
 */

#ifndef ____vintageVoice__
#define ____vintageVoice__

#include <stdio.h>
#include "../globals.h"
#include "Maximilian/maximilian.h"

class VintageVoice
{
public:
    VintageVoice (int voice_num);
    ~VintageVoice();
    
    void processAudio (double *output);
    
private:
    
    //These are the synthesiser bits
    maxiOsc VCO1[NUM_OF_VOICES],VCO2[NUM_OF_VOICES],LFO1[NUM_OF_VOICES];
    maxiSVF svfVcf[NUM_OF_VOICES];
    maxiEnv ADSR[NUM_OF_VOICES];
    
    //This is a bunch of control signals so that we can hear something
    maxiOsc timer;//this is the metronome
    int currentCount,lastCount,voice;//these values are used to check if we have a new beat this sample
    
    //and these are some variables we can use to pass stuff around
    double VCO1out[NUM_OF_VOICES],VCO2out[NUM_OF_VOICES],LFO1out[NUM_OF_VOICES],VCFout[NUM_OF_VOICES],ADSRout[NUM_OF_VOICES],mix,pitch[NUM_OF_VOICES];

};

#endif /* defined(____vintageVoice__) */
