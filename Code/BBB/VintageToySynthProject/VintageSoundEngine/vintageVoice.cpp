/*
 vintageVoice.cpp
 
 Created by Liam Lacey on 29/01/2016.
 Copyright 2016 Liam Lacey. All rights reserved.
 
 This is a file for the vintageSoundEngine application for the Vintage Toy Piano Synth project.
 This application is the audio synthesis engine for the synthesiser, using the Maximilian synthesis library.
 
 This particular file/class handles the audio processing for each voice.
 */

//FIXME: do I need to do any thread locking when setting variables that are accessed in processAudio (called by the audio callback function)?

#include "vintageVoice.h"

//==========================================================
//==========================================================
//==========================================================

VintageVoice::VintageVoice (uint8_t voice_num)
{
    std::cout << "[VV] Initing vintageVoice num " << (int)voice_num << std::endl;
    
    //init variables
    oscPitch = 200;
}

VintageVoice::~VintageVoice()
{
    
}

//==========================================================
//==========================================================
//==========================================================
//This is called by the audio processing callback function,
//and is where all audio processing is done

void VintageVoice::processAudio (double *output)
{
    output[0] = oscSquare.square (oscPitch);
    output[1] = output[0];
}

//==========================================================
//==========================================================
//==========================================================
//Sets the oscillator pitch based in the incoming MIDI note number

void VintageVoice::setOscPitch (uint8_t midi_note_num)
{
    convert mtof;
    oscPitch = mtof.mtof(midi_note_num);
}
